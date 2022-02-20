# Integration of graphics generators into LiteX, supporting HDMI output
# Only requirement is a verilog module called M_frame_display__display
#
# Copyright (c) 2022 Victor Suarez Rovere <suarezvictor@gmail.com>
# code portions from LiteX framework (C) Enjoy-Digital https://github.com/enjoy-digital/litex

import sys
sys.path.append("../../external/litex-boards")

from migen import *
from litex.soc.cores.video import VideoTimingGenerator, video_timing_layout, video_data_layout
from litex_boards.platforms import de0nano
from terasic_de0nano import VideoHDMIPHY, _CRG
from litex.soc.interconnect import stream
from litex.soc.integration.builder import *
from litex.soc.integration.soc_core import *
from litex.build.generic_platform import *

class GraphicsGenerator(Module):
    def __init__(self):
        self.enable   = Signal(reset=1)
        self.vtg_sink = vtg_sink   = stream.Endpoint(video_timing_layout)
        self.source   = source = stream.Endpoint(video_data_layout)
        self.comb += vtg_sink.connect(source, keep={"valid", "ready", "last", "de", "hsync", "vsync"}),

        framedisplay = Module()
        self.done = Signal()
        self.out_clock = Signal()
        framedisplay.specials += Instance("M_frame_display__display",
            i_in_pix_x = vtg_sink.hcount[0:10],
            i_in_pix_y = vtg_sink.vcount[0:10],
            i_in_pix_active = vtg_sink.de,
            i_in_pix_vblank = ~vtg_sink.vsync, #FIXME: hack to fix bugs about "de" and "vsync" signals
            o_out_pix_r = source.r[2:8], #original color 6-bit
            o_out_pix_g = source.g[2:8],
            o_out_pix_b = source.b[2:8],
            o_out_done = self.done,
            i_reset = ResetSignal("sys"),
            o_out_clock = self.out_clock,
            i_clock = ClockSignal("sys") #results in "hdmi" clock
        )
    
        self.framedisplay = framedisplay 
        self.submodules += framedisplay

def add_video_custom_generator(soc, name="video", phy=None, timings="800x600@60Hz", clock_domain="sys"):
    # Video Timing Generator.
    soc.check_if_exists(f"{name}_vtg")
    vtg = VideoTimingGenerator(default_video_timings=timings)
    vtg = ClockDomainsRenamer(clock_domain)(vtg)
    setattr(soc.submodules, f"{name}_vtg", vtg)

    graphics = GraphicsGenerator()
    graphics = ClockDomainsRenamer(clock_domain)(graphics)
    setattr(soc.submodules, name, graphics)

    # Connect Video Timing Generator to GraphicsGenerator
    soc.comb += [
        vtg.source.connect(graphics.vtg_sink),
        graphics.source.connect(phy if isinstance(phy, stream.Endpoint) else phy.sink)
    ]

def add_io():
    # GPDI using LVDS outputs
    de0nano._io += (("gpdi", 0, #NOTE: negative LVDS output seems automatic
        Subsignal("clk_p",   Pins("R16"), IOStandard("LVDS")), #JP2.23=GPIO.118=R16 orange box 
       #Subsignal("clk_n",   Pins("P16"), IOStandard("LVDS")), #JP2.26=GPIO.121=P16 orange 
        Subsignal("data2_p", Pins("N15"), IOStandard("LVDS")), #JP2.31=GPIO.124=N15 red
       #Subsignal("data2_n", Pins("N16"), IOStandard("LVDS")), #JP2.28=GPIO.123=N16 red    box (NOTE INVERSION)
        Subsignal("data1_p", Pins("L15"), IOStandard("LVDS")), #JP2.24=GPIO.119=L15 green  box
       #Subsignal("data1_n", Pins("L16"), IOStandard("LVDS")), #JP2.21=GPIO.116=L16 green
        Subsignal("data0_p", Pins("K15"), IOStandard("LVDS")), #JP2.38=GPIO.131=K15 blue   box
       #Subsignal("data0_n", Pins("K16"), IOStandard("LVDS")), #JP2.22=GPIO.117=K16 blue
    ),)


import argparse

def main():
    add_io()
    parser = argparse.ArgumentParser()
    soc_core_args(parser)
    args = parser.parse_args()

    #soc = CustomSoC(**soc_core_argdict(args))
    sys_clk_freq = int(50e6)
    soc = SoCCore(de0nano.Platform(), sys_clk_freq, **soc_core_argdict(args))
    soc.submodules.crg = _CRG(soc.platform, sys_clk_freq, sdram_rate="1:1")
    soc.platform.add_source("build/top_instance.v") #generated verilog for graphics
    soc.submodules.videophy = VideoHDMIPHY(soc.platform.request("gpdi"), clock_domain="hdmi")

    add_video_custom_generator(soc, phy=soc.videophy, timings="640x480@60Hz", clock_domain="hdmi")
    builder = Builder(soc)
    builder.build(run=True)

if __name__ == "__main__":
    main()


