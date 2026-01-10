# Copyright (c) 2026 Victor Suarez Rovere <suarezvictor@gmail.com>

from migen import *
from litex.soc.integration.soc import AutoCSR, CSRStorage, CSRStatus

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
    def __init__(self, instances):
        self.name = "idct_kernel_remap"
        assert instances == 8
        
        idcts = [AccelIDCT() for i in range(instances)]
        self.submodules += idcts

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

        """
        self.remapped = []
        for m in range(8):
          row = []
          for n in range(4):
            name = f"remap_din{m}_{n}";
            r = CSRStorage(32, name=name)
            setattr(self, name, r)
            row += [r]
          self.remapped += [row]
            
        for m in range(8):
          self.sync += [
            If(self.remapped[m][0].re, idcts[m].di_reg[0].eq(self.remapped[m][0].storage[:16])),
            If(self.remapped[m][0].re, idcts[m].di_reg[1].eq(self.remapped[m][0].storage[16:])),
            If(self.remapped[m][1].re, idcts[m].di_reg[2].eq(self.remapped[m][1].storage[:16])),
            If(self.remapped[m][1].re, idcts[m].di_reg[3].eq(self.remapped[m][1].storage[16:])),
            If(self.remapped[m][2].re, idcts[m].di_reg[4].eq(self.remapped[m][2].storage[:16])),
            If(self.remapped[m][2].re, idcts[m].di_reg[5].eq(self.remapped[m][2].storage[16:])),
            If(self.remapped[m][3].re, idcts[m].di_reg[6].eq(self.remapped[m][3].storage[:16])),
            If(self.remapped[m][3].re, idcts[m].di_reg[7].eq(self.remapped[m][3].storage[16:])),
          ]
        """

        self.omapped = []
        for m in range(instances):
          row = []
          for n in range(4):
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
            If(idcts[m].run_reg, self.omapped[m][2].status[:16].eq(idcts[m].do_reg[4])),
            If(idcts[m].run_reg, self.omapped[m][2].status[16:].eq(idcts[m].do_reg[5])),
            If(idcts[m].run_reg, self.omapped[m][3].status[:16].eq(idcts[m].do_reg[6])),
            If(idcts[m].run_reg, self.omapped[m][3].status[16:].eq(idcts[m].do_reg[7])),
          ]
          self.comb += [
            self.done.status[m].eq(idcts[m].done_reg),
            idcts[m].run_reg.eq(self.run.storage[m]),
            idcts[m].is_y_reg.eq(self.is_y.storage),
          ]
        
        #control register for instant copying of (transposed) outputs to inputs
        self.ctrl = CSRStorage(1)
        for m in range(8):
          self.sync +=  [If(self.ctrl.re, idcts[m].di_reg[n].eq(idcts[n].do_reg[m])) for n in range(8)]
      

