# Copyright (c) 2026 Victor Suarez Rovere <suarezvictor@gmail.com>

from migen import *
from litex.soc.integration.soc import AutoCSR, CSRStorage, CSRStatus
from litex.soc.interconnect import wishbone

class AccelIDCT(Module):
    def __init__(self):
        self.name = "idct_kernel"

        self.di_reg = [Signal(16) for x in range(8)]

        self.is_y_reg = Signal()
        self.run_reg = Signal(reset=0)
        self.done_reg = Signal()

        self.do_reg = [Signal(16) for x in range(8)]

    def do_finalize(self):
        super().do_finalize()
        self.specials += Instance("M_idct_kernel",
            i_in_data_in_0 = self.di_reg[0],
            i_in_data_in_1 = self.di_reg[1],
            i_in_data_in_2 = self.di_reg[2],
            i_in_data_in_3 = self.di_reg[3],
            i_in_data_in_4 = self.di_reg[4],
            i_in_data_in_5 = self.di_reg[5],
            i_in_data_in_6 = self.di_reg[6],
            i_in_data_in_7 = self.di_reg[7],

            o_out_data_out_0 = self.do_reg[0],
            o_out_data_out_1 = self.do_reg[1],
            o_out_data_out_2 = self.do_reg[2],
            o_out_data_out_3 = self.do_reg[3],
            o_out_data_out_4 = self.do_reg[4],
            o_out_data_out_5 = self.do_reg[5],
            o_out_data_out_6 = self.do_reg[6],
            o_out_data_out_7 = self.do_reg[7],

            i_in_is_y = self.is_y_reg,
            i_in_run  = self.run_reg,
            o_out_done = self.done_reg,

            o_out_clock = Signal(),
            i_reset = ResetSignal("sys"),
            i_clock = ClockSignal("sys")
        )

#this permits to access data input of a IDCT block as  matrix transpose
class RemapIDCT(Module, AutoCSR):
	def __init__(self, instances, second_source = None, second_target = None):
		self.name = "idct_kernel_remap"
		assert instances == 8
		assert second_source is None or len(second_source)==1024
		assert second_target is not None and len(second_target)==512

		idcts = [AccelIDCT() for i in range(instances)]
		self.submodules += idcts

		if second_source is None:
			self.mapped = []
			for m in range(instances):
				row = []
				for n in range(4):
					name = f"map_din{m}_{n}";
					r = CSRStorage(32, name=name)
					setattr(self, name, r)
					row += [r]
				self.mapped += [row]

			for m in range(8):
				self.sync += [
					If(self.mapped[m][0].re, idcts[0].di_reg[m].eq(self.mapped[m][0].storage[:16])),
					If(self.mapped[m][0].re, idcts[1].di_reg[m].eq(self.mapped[m][0].storage[16:])),
					If(self.mapped[m][1].re, idcts[2].di_reg[m].eq(self.mapped[m][1].storage[:16])),
					If(self.mapped[m][1].re, idcts[3].di_reg[m].eq(self.mapped[m][1].storage[16:])),
					If(self.mapped[m][2].re, idcts[4].di_reg[m].eq(self.mapped[m][2].storage[:16])),
					If(self.mapped[m][2].re, idcts[5].di_reg[m].eq(self.mapped[m][2].storage[16:])),
					If(self.mapped[m][3].re, idcts[6].di_reg[m].eq(self.mapped[m][3].storage[:16])),
					If(self.mapped[m][3].re, idcts[7].di_reg[m].eq(self.mapped[m][3].storage[16:])),
				]

		self.omapped = []
		for m in range(instances):
			row = []
			for n in range(2): #outputs are byte packed so 2 are enough for 6 pixel
				name = f"map_dout{m}_{n}";
				r = CSRStatus(32, name=name)
				setattr(self, name, r)
				row += [r]
			self.omapped += [row]

		self.is_y = CSRStorage()
		self.run = CSRStorage(8, reset=0)
		self.done = CSRStatus(8)

		for m in range(instances):
			self.sync += [
				If(idcts[m].run_reg, self.omapped[m][0].status[:16].eq(idcts[m].do_reg[0])),
				If(idcts[m].run_reg, self.omapped[m][0].status[16:].eq(idcts[m].do_reg[1])),
				If(idcts[m].run_reg, self.omapped[m][1].status[:16].eq(idcts[m].do_reg[2])),
				If(idcts[m].run_reg, self.omapped[m][1].status[16:].eq(idcts[m].do_reg[3])),
			]
			self.comb += [
				second_target[(m*2+0)*32:(m*2+0)*32+32].eq(self.omapped[m][0].status),
				second_target[(m*2+1)*32:(m*2+1)*32+32].eq(self.omapped[m][1].status),
			]
			self.comb += [
				self.done.status[m].eq(idcts[m].done_reg),
				idcts[m].run_reg.eq(self.run.storage[m]),
				idcts[m].is_y_reg.eq(self.is_y.storage),
			]

		#control register:
		# bit 0: instant copying of (transposed) outputs to inputs
		# bit 1: use 2nd source (transposed) as input

		self.ctrl = CSRStorage(2)
		for m in range(8):
			self.sync +=  [If(self.ctrl.re & self.ctrl.storage[0], idcts[m].di_reg[n].eq(idcts[n].do_reg[m])) for n in range(8)]

		if second_source is not None:
			c = self.ctrl.re & self.ctrl.storage[1]
			for m in range(8):
				self.sync += [
					If(c, idcts[0].di_reg[m].eq(second_source[(m*8+0)*16:(m*8+0)*16+16])),
					If(c, idcts[1].di_reg[m].eq(second_source[(m*8+1)*16:(m*8+1)*16+16])),
					If(c, idcts[2].di_reg[m].eq(second_source[(m*8+2)*16:(m*8+2)*16+16])),
					If(c, idcts[3].di_reg[m].eq(second_source[(m*8+3)*16:(m*8+3)*16+16])),
					If(c, idcts[4].di_reg[m].eq(second_source[(m*8+4)*16:(m*8+4)*16+16])),
					If(c, idcts[5].di_reg[m].eq(second_source[(m*8+5)*16:(m*8+5)*16+16])),
					If(c, idcts[6].di_reg[m].eq(second_source[(m*8+6)*16:(m*8+6)*16+16])),
					If(c, idcts[7].di_reg[m].eq(second_source[(m*8+7)*16:(m*8+7)*16+16])),
				]
      

class WBDMAReadWrite(Module, AutoCSR):
    def __init__(self, rd_signal, wr_signal, bus_target_width=32):
        assert wr_signal is None or rd_signal is None or len(wr_signal) == len(rd_signal)
        size = len(rd_signal) if rd_signal is not None else len(wr_signal)

        self.start     = CSRStorage(2, description="bit 0: Trigger read or write, bit 1: is_write", reset=0)
        self.base_addr = CSRStorage(32, description="Aligned address")
        self.done      = CSRStatus(description="Read completed", reset=1)

        # wishbone master
        self.wb = wishbone.Interface(
            adr_width  = 32, #FIXME: take from external bus width
            data_width = size
        )
        # size converter
        self.wb_bus_target = wishbone.Interface(
            adr_width  = 32, #FIXME: take from external bus width
            data_width = bus_target_width
        )
        self.submodules.converter = wishbone.Converter(
            master = self.wb,
            slave  = self.wb_bus_target
        )

        self.comb += [
            self.wb.cyc.eq(~self.done.status),
            self.wb.stb.eq(~self.done.status),
            self.wb.adr.eq(self.base_addr.storage>>log2_int(size//8)),
            self.wb.sel.eq((2**(size // 8)) - 1),
            self.wb.cti.eq(wishbone.CTI_BURST_INCREMENTING), # configure SOC with --bus-bursting
            self.wb.bte.eq(0),
            self.wb.we.eq(0),
        ]
        if wr_signal is not None:
            self.comb += self.wb.we.eq(self.start.storage[1]),
            self.comb += self.wb.dat_w.eq(wr_signal)

        # ---------------- Control logic ----------------
        self.sync += [
            # Start transaction
            If(self.start.re, self.done.status.eq(~self.start.storage[0])),
            #If(~self.done.status, Display("WE %d, addr %08X, smaller cti %d stb %d ack %d, larger stb %d ack %d", 
	        #    self.wb_bus_target.we, 
	        #    self.wb_bus_target.adr, self.wb_bus_target.cti, self.wb_bus_target.stb, self.wb_bus_target.ack,
	        #    self.wb.stb, self.wb.ack)), #debug
        ]

        if rd_signal is not None:
            # Capture data on acknowledge (every cycle if bursting enabled and SRAM source)
            self.sync +=  If(self.wb.stb & ~self.wb.we & self.wb.ack, rd_signal.eq(self.wb.dat_r), self.done.status.eq(1));

        # Also set done when write ack
        self.sync +=  If(self.wb.stb & self.wb.we & self.wb.ack, self.done.status.eq(1))

        
