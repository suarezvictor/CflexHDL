#!/usr/bin/env python3

# Copyright (c) 2025 Victor Suarez Rovere <suarezvictor@gmail.com>
# SPDX-License-Identifier: AGPL-3.0-only
#
# Copyright (c) 2015-2020 Florent Kermarrec <florent@enjoy-digital.fr>
# Copyright (c) 2020 Antmicro <www.antmicro.com>
# Copyright (c) 2017 Pierre-Olivier Vauboin <po@lambdaconcept>


import sys
import argparse

from migen import *

from litex.build.generic_platform import *
from litex.build.sim import SimPlatform
from litex.build.sim.config import SimConfig
from litex.build.sim.verilator import verilator_build_args, verilator_build_argdict

from litex.soc.interconnect.csr import *
from litex.soc.integration.common import *
from litex.soc.integration.soc_core import *
from litex.soc.integration.builder import *
from litex.soc.integration.soc import *
from litex.soc.cores.cpu import CPUS

from litedram import modules as litedram_modules
from litedram.modules import parse_spd_hexdump
from litedram.phy.model import sdram_module_nphases, get_sdram_phy_settings
from litedram.phy.model import SDRAMPHYModel

from litex.soc.cores.video import video_data_layout

SYS_CLK_FREQ = 1e6

# IOs ----------------------------------------------------------------------------------------------

_io = [
    # Rst. (clk is set by the clocker)
    ("sys_rst", 0, Pins(1)),

    # Serial.
    ("serial", 0,
        Subsignal("source_valid", Pins(1)),
        Subsignal("source_ready", Pins(1)),
        Subsignal("source_data",  Pins(8)),

        Subsignal("sink_valid",   Pins(1)),
        Subsignal("sink_ready",   Pins(1)),
        Subsignal("sink_data",    Pins(8)),
    ),
    # Video
    ("vga", 0,
        Subsignal("clk",   Pins(1)), #use pixel clock
        Subsignal("hsync", Pins(1)),
        Subsignal("vsync", Pins(1)),
        Subsignal("de",    Pins(1)),
        Subsignal("r",     Pins(8)),
        Subsignal("g",     Pins(8)),
        Subsignal("b",     Pins(8)),
        Subsignal("valid", Pins(1)), #handles backpressure
    )
]

# Platform -----------------------------------------------------------------------------------------

class Platform(SimPlatform):
    def __init__(self):
        SimPlatform.__init__(self, "SIM", _io)

# Video
class VideoPHYModel(Module, AutoCSR):
    def __init__(self, pads, clock_domain="sys"):
        self.sink = sink = stream.Endpoint(video_data_layout)

        # # #

        # Always ack Sink, no backpressure.
        self.comb += sink.ready.eq(1)

        # Drive Clk.
        if hasattr(pads, "clk"):
            self.comb += pads.clk.eq(ClockSignal(clock_domain))

        # Drive Controls.
        self.comb += pads.valid.eq(1) #may be overriden with underflow from the framebuffer
        self.comb += pads.de.eq(sink.de)
        self.comb += pads.hsync.eq(sink.hsync)
        self.comb += pads.vsync.eq(sink.vsync)

        # Drive Datas.
        cbits  = len(pads.r)
        cshift = (8 - cbits)
        for i in range(cbits):
            self.comb += pads.r[i].eq(sink.r[cshift + i] & sink.de)
            self.comb += pads.g[i].eq(sink.g[cshift + i] & sink.de)
            self.comb += pads.b[i].eq(sink.b[cshift + i] & sink.de)


# Clocks -------------------------------------------------------------------------------------------

class Clocks(dict):
    # FORMAT: {name: {"freq_hz": _, "phase_deg": _}, ...}
    def names(self):
        return list(self.keys())

    def add_io(self, io):
        for name in self.names():
            io.append((name + "_clk", 0, Pins(1)))

    def add_clockers(self, sim_config):
        for name, desc in self.items():
            sim_config.add_clocker(name + "_clk", **desc)

class _CRG(Module):
    def __init__(self, platform, domains=None):
        if domains is None:
            domains = ["sys"]
        # request() before clreating domains to avoid signal renaming problem
        domains = {name: platform.request(name + "_clk") for name in domains}

        self.clock_domains.cd_por = ClockDomain(reset_less=True)
        for name in domains.keys():
            setattr(self.clock_domains, "cd_" + name, ClockDomain(name=name))

        int_rst = Signal(reset=1)
        self.sync.por += int_rst.eq(0)
        self.comb += self.cd_por.clk.eq(self.cd_sys.clk)

        for name, clk in domains.items():
            cd = getattr(self, "cd_" + name)
            self.comb += cd.clk.eq(clk)
            self.comb += cd.rst.eq(int_rst)


# Simulation SoC -----------------------------------------------------------------------------------

class SimSoC(SoCCore):
    def __init__(self, clocks,
        with_sdram            = False,
        sdram_module          = "MT48LC16M16",
        sdram_init            = [],
        with_video_framebuffer = False,
        no_compile_gateware   = False,
        **kwargs):
        platform     = Platform()
        sys_clk_freq = int(SYS_CLK_FREQ)

        # CRG --------------------------------------------------------------------------------------
        self.submodules.crg = _CRG(platform, clocks.names())


        # SoCCore ----------------------------------------------------------------------------------
        SoCCore.__init__(self, platform, clk_freq=sys_clk_freq,
            ident = "LiteX Simulation",
            **kwargs)


        # SDRAM ------------------------------------------------------------------------------------
        if not self.integrated_main_ram_size and with_sdram:
            sdram_clk_freq = int(100e6) # FIXME: use 100MHz timings
            if True:
                sdram_module_cls = getattr(litedram_modules, sdram_module)
                sdram_rate       = "1:{}".format(sdram_module_nphases[sdram_module_cls.memtype])
                sdram_module     = sdram_module_cls(sdram_clk_freq, sdram_rate)
            self.submodules.sdrphy = SDRAMPHYModel(
                module     = sdram_module,
                data_width = 128,
                clk_freq   = sdram_clk_freq,
                verbosity  = False,
                init       = sdram_init,
                )
            self.add_sdram("sdram",
                phy                     = self.sdrphy,
                module                  = sdram_module,
            )

            if sdram_init != []:
                # Skip SDRAM test to avoid corrupting pre-initialized contents.
                self.add_constant("SDRAM_TEST_DISABLE")
            else:
                # Reduce memtest size for simulation speedup
                self.add_constant("MEMTEST_DATA_SIZE", 8*1024)
                self.add_constant("MEMTEST_ADDR_SIZE", 8*1024)


        # Video --------------------------------------------------------------------------------------
        if with_video_framebuffer:
            video_pads = platform.request("vga")
            self.submodules.videophy = VideoPHYModel(video_pads, clock_domain="pix")
            self.add_video_framebuffer(phy=self.videophy, timings="640x480@60Hz", format="rgb888", clock_domain="pix") #
            self.videophy.comb += video_pads.valid.eq(~self.video_framebuffer.underflow)

        # Accelerator --------------------------------------------------------------------------------------
        from videocodecs import WBDMAReadWrite
        rdbuf = Signal(1024)
        wrbuf = Signal(512)
        self.submodules.idctdma_rd = rd = WBDMAReadWrite(rdbuf, None, bus_target_width=self.bus.data_width)
        self.bus.add_master(name="idctdma_rd_master", master=rd.wb_bus_target)

        from videocodecs import AccelIDCT, RemapIDCT
        self.add_constant("IDCT_MERGE_IN_FIELDS")  #deprecated
        self.add_constant("IDCT_MERGE_OUT_FIELDS") #deprecated
        instances = 8
        self.add_constant("IDCT_INSTANCE_COUNT", instances)
        self.submodules.idct_kernel_remap = RemapIDCT(instances, second_source=rdbuf, second_target=wrbuf)

        self.submodules.idctdma_wr = wr = WBDMAReadWrite(None, wrbuf, bus_target_width=self.bus.data_width)
        self.bus.add_master(name="idctdma_wr", master=wr.wb_bus_target)
        
        



# Build --------------------------------------------------------------------------------------------

def sim_args(parser):
    builder_args(parser)
    soc_core_args(parser)
    verilator_build_args(parser)
    parser.add_argument("--sdram-init",           default=None,            help="SDRAM init file (.bin or .json).")


def main():
    from litex.soc.integration.soc import LiteXSoCArgumentParser
    parser = LiteXSoCArgumentParser(description="LiteX SoC Simulation utility")
    sim_args(parser)
    args = parser.parse_args()

    soc_kwargs             = soc_core_argdict(args)
    builder_kwargs         = builder_argdict(args)
    verilator_build_kwargs = verilator_build_argdict(args)

    sys_clk_freq = int(SYS_CLK_FREQ)
    
    clocks = Clocks({
        "sys":         dict(freq_hz=sys_clk_freq),
        "pix":         dict(freq_hz=int(sys_clk_freq/4)), #pixel clock is slower to save RAM bandwidth
    })
    clocks.add_io(_io)
        
    sim_config   = SimConfig()
    clocks.add_clockers(sim_config)

    # Configuration --------------------------------------------------------------------------------

    cpu            = CPUS.get(soc_kwargs.get("cpu_type", "vexriscv"))
    bus_data_width = int(soc_kwargs["bus_data_width"])

    # UART.
    if soc_kwargs["uart_name"] == "serial":
        soc_kwargs["uart_name"] = "sim"
        sim_config.add_module("serial2console", "serial")

    # Create config SoC that will be used to prepare/configure real one.
    conf_soc = SimSoC(clocks, **soc_kwargs)


    # RAM / SDRAM.
    ram_boot_address = None
    soc_kwargs["integrated_main_ram_size"] = args.integrated_main_ram_size
    if args.sdram_init is not None:
        soc_kwargs["sdram_init"] = get_mem_data(args.sdram_init,
            data_width = bus_data_width,
            endianness = cpu.endianness,
            offset     = conf_soc.mem_map["main_ram"]
        )
        ram_boot_address         = get_boot_address(args.sdram_init)

	
    # Video.
    sim_config.add_module("video", "vga")

    # SoC ------------------------------------------------------------------------------------------
    soc = SimSoC(clocks,
        with_sdram             = True,
        with_video_framebuffer = True,
        no_compile_gateware    = args.no_compile_gateware,
        **soc_kwargs)

    if ram_boot_address is not None:
        if ram_boot_address == 0:
            ram_boot_address = conf_soc.mem_map["main_ram"]
        soc.add_constant("ROM_BOOT_ADDRESS", ram_boot_address)

    soc.add_constant("LITEX_SIMULATION") #this is to make the software know if it's simulation
    soc.platform.add_source("build/top_instance.v")

    builder = Builder(soc, **builder_kwargs)
    builder.build(
        sim_config       = sim_config,
        **verilator_build_kwargs,
    )

if __name__ == "__main__":
    main()
