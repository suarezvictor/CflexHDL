# Copyright (c) 2026 Victor Suarez Rovere <suarezvictor@gmail.com>

from migen import *
from litex.soc.integration.soc import AutoCSR, CSRStorage, CSRStatus

class AccelIDCT(Module, AutoCSR):
    def __init__(self, mergein=False, mergeout=False):
        self.name = "idct_kernel"
        self.mergein, self.mergeout = mergein, mergeout

        if mergein:
            self.din01 = CSRStorage(32)
            self.din23 = CSRStorage(32)
            self.din45 = CSRStorage(32)
            self.din67 = CSRStorage(32)
        else:
            self.din0 = CSRStorage(32)
            self.din1 = CSRStorage(32)
            self.din2 = CSRStorage(32)
            self.din3 = CSRStorage(32)
            self.din4 = CSRStorage(32)
            self.din5 = CSRStorage(32)
            self.din6 = CSRStorage(32)
            self.din7 = CSRStorage(32)

        if mergeout:
            self.dout01 = CSRStatus(32)
            self.dout23 = CSRStatus(32)
            self.dout45 = CSRStatus(32)
            self.dout67 = CSRStatus(32)
        else:
            self.dout0 = CSRStatus(32)
            self.dout1 = CSRStatus(32)
            self.dout2 = CSRStatus(32)
            self.dout3 = CSRStatus(32)
            self.dout4 = CSRStatus(32)
            self.dout5 = CSRStatus(32)
            self.dout6 = CSRStatus(32)
            self.dout7 = CSRStatus(32)
        self.is_y = CSRStorage(16)

        self.run = CSRStorage(reset=0)
        self.done  = CSRStatus()

    def do_finalize(self):
        super().do_finalize()
        self.specials += Instance("M_idct_kernel",
            i_in_data_in_0 = self.din01.storage[:16] if self.mergein else self.din0.storage,
            i_in_data_in_1 = self.din01.storage[16:] if self.mergein else self.din1.storage,
            i_in_data_in_2 = self.din23.storage[:16] if self.mergein else self.din2.storage,
            i_in_data_in_3 = self.din23.storage[16:] if self.mergein else self.din3.storage,
            i_in_data_in_4 = self.din45.storage[:16] if self.mergein else self.din4.storage,
            i_in_data_in_5 = self.din45.storage[16:] if self.mergein else self.din5.storage,
            i_in_data_in_6 = self.din67.storage[:16] if self.mergein else self.din6.storage,
            i_in_data_in_7 = self.din67.storage[16:] if self.mergein else self.din7.storage,

            o_out_data_out_0 = self.dout01.status[:16] if self.mergeout else self.dout0.status,
            o_out_data_out_1 = self.dout01.status[16:] if self.mergeout else self.dout1.status,
            o_out_data_out_2 = self.dout23.status[:16] if self.mergeout else self.dout2.status,
            o_out_data_out_3 = self.dout23.status[16:] if self.mergeout else self.dout3.status,
            o_out_data_out_4 = self.dout45.status[:16] if self.mergeout else self.dout4.status,
            o_out_data_out_5 = self.dout45.status[16:] if self.mergeout else self.dout5.status,
            o_out_data_out_6 = self.dout67.status[:16] if self.mergeout else self.dout6.status,
            o_out_data_out_7 = self.dout67.status[16:] if self.mergeout else self.dout7.status,

            i_in_is_y = self.is_y.storage,

            i_in_run  = self.run.storage,
            o_out_done = self.done.status,
            o_out_clock = Signal(),
            i_reset = ResetSignal("sys"),
            i_clock = ClockSignal("sys")
        )

