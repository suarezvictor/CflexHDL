# Integration of graphics generators into LiteX, supporting HDMI output
# Only requirement is a verilog module called M_frame_display__display
#
# Copyright (c) 2022 Victor Suarez Rovere <suarezvictor@gmail.com>
# code portions from LiteX framework (C) Enjoy-Digital https://github.com/enjoy-digital/litex

import sys

from migen import *
from litex.soc.cores.clock import *
from litex.soc.cores.video import VideoTimingGenerator, video_timing_layout, video_data_layout
from litex.soc.interconnect import stream
from litex.soc.integration.builder import *
from litex.soc.integration.soc_core import *
from litex.build.generic_platform import *

DVI = False

class VideoGenericPHY_SDR(Module):
    def __init__(self, pads, clock_domain="sys"):
        self.sink = sink = stream.Endpoint(video_data_layout)

        # # #

        # Always ack Sink, no backpressure.
        self.comb += sink.ready.eq(1)

        # Drive Clk.
        if hasattr(pads, "clk"):
            self.comb += pads.clk.eq(ClockSignal(clock_domain))

        # Drive Controls.
        if hasattr(pads, "de"):
            self.comb += pads.de.eq(sink.de)

        if hasattr(pads, "hsync_n"):
            self.comb += pads.hsync.eq(~sink.hsync)
        else:
            self.comb += pads.hsync.eq(sink.hsync)

        if hasattr(pads, "vsync_n"):
            self.comb += pads.vsync.eq(~sink.vsync)
        else:
            self.comb += pads.vsync.eq(sink.vsync)

        # Drive Datas.
        cbits  = len(pads.r)
        cshift = (8 - cbits)
        for i in range(cbits):
            self.comb += pads.r[i].eq(sink.r[cshift + i] & sink.de)
            self.comb += pads.g[i].eq(sink.g[cshift + i] & sink.de)
            self.comb += pads.b[i].eq(sink.b[cshift + i] & sink.de)


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
            i_in_pix_vblank = vtg_sink.vsync,
            o_out_pix_r = source.r,
            o_out_pix_g = source.g,
            o_out_pix_b = source.b,
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
    if clock_domain != "sys":
      #vtg = ClockDomainsRenamer(clock_domain)(vtg)
      pass
    setattr(soc.submodules, f"{name}_vtg", vtg)

    graphics = GraphicsGenerator()
    if clock_domain != "sys":
      #graphics = ClockDomainsRenamer(clock_domain)(graphics)
      pass
    setattr(soc.submodules, name, graphics)

    # Connect Video Timing Generator to GraphicsGenerator
    soc.comb += [
        vtg.source.connect(graphics.vtg_sink),
        graphics.source.connect(phy if isinstance(phy, stream.Endpoint) else phy.sink)
    ]


def build_de0nano(args):
	from litex_boards.platforms import de0nano as board
	sys.path.append("../../external/litex-boards")
	from terasic_de0nano import VideoHDMIPHY, _CRG

	# GPDI using LVDS outputs
	#FIXME: use platform.add_extension
	board._io += (("gpdi", 0, #NOTE: negative LVDS output seems automatic
		Subsignal("clk_p",   Pins("R16"), IOStandard("LVDS")), #JP2.23=GPIO.118=R16 orange box 
	   #Subsignal("clk_n",   Pins("P16"), IOStandard("LVDS")), #JP2.26=GPIO.121=P16 orange 
		Subsignal("data2_p", Pins("N15"), IOStandard("LVDS")), #JP2.31=GPIO.124=N15 red
	   #Subsignal("data2_n", Pins("N16"), IOStandard("LVDS")), #JP2.28=GPIO.123=N16 red    box (NOTE INVERSION)
		Subsignal("data1_p", Pins("L15"), IOStandard("LVDS")), #JP2.24=GPIO.119=L15 green  box
	   #Subsignal("data1_n", Pins("L16"), IOStandard("LVDS")), #JP2.21=GPIO.116=L16 green
		Subsignal("data0_p", Pins("K15"), IOStandard("LVDS")), #JP2.38=GPIO.131=K15 blue   box
	   #Subsignal("data0_n", Pins("K16"), IOStandard("LVDS")), #JP2.22=GPIO.117=K16 blue
	),)

	platform = board.Platform()

	sys_clk_freq = int(50e6)
	soc = SoCCore(platform, sys_clk_freq, **soc_core_argdict(args))
	soc.submodules.crg = _CRG(soc.platform, sys_clk_freq, sdram_rate="1:1")
	soc.submodules.videophy = VideoHDMIPHY(soc.platform.request("gpdi"), clock_domain="hdmi")
	add_video_custom_generator(soc, phy=soc.videophy, timings="640x480@60Hz", clock_domain="hdmi")
	return soc


class _CRG_arty(Module):
    def __init__(self, platform, sys_clk_freq, with_rst=True):
        self.rst = Signal()
        self.clock_domains.cd_sys       = ClockDomain()

        self.submodules.pll = pll = S7PLL(speedgrade=-1)
        rst    = ~platform.request("cpu_reset") if with_rst else 0
        self.comb += pll.reset.eq(rst | self.rst)
        pll.register_clkin(platform.request("clk100"), 100e6)
        pll.create_clkout(self.cd_sys,       sys_clk_freq)

        video_clock = 25e6 #"800x600@75Hz" =>  49.5e6, "640x480@60Hz" => 25.175e6 "800x600@60Hz"  => 40e6, 1280x720@60Hz(RB) => 61.9e6 1024 46.42e6
        if DVI:
            self.clock_domains.cd_hdmi   = ClockDomain()
            self.clock_domains.cd_hdmi5x = ClockDomain()
            pll.create_clkout(self.cd_hdmi,     video_clock, margin=1e-3)
            pll.create_clkout(self.cd_hdmi5x, 5*video_clock, margin=1e-3)
        else:
            if int(sys_clk_freq) == int(video_clock):
              self.clock_domains.cd_vga = self.cd_sys
            else:
              #self.clock_domains.cd_vga       = ClockDomain(reset_less=True)
              self.clock_domains.cd_vga       = ClockDomain(reset_less=False) #TODO: chech why True brings errors
              pll.create_clkout(self.cd_vga, video_clock, margin=1e-3)
        platform.add_false_path_constraints(self.cd_sys.clk, pll.clkin) # Ignore sys_clk to pll.clkin path created by SoC's rst.


def build_arty(args, toolchain):
	from litex_boards.platforms import digilent_arty as board
	if toolchain is None:
		platform = board.Platform() #default
	else:
		platform = board.Platform(toolchain=toolchain) #"yosys+nextpnr" brings errors about usage of DSP48E1 (* operator) and of ODDR

	if toolchain == "yosys+nextpnr":
		#this is needed to avoid the unsupported clocked DSP48E1s
		platform.toolchain._yosys_cmds.append("scratchpad -set xilinx_dsp.multonly 1")
		pass
      
	sys_clk_freq = int(25e6) #int(100e6)
	soc = SoCCore(platform, sys_clk_freq, **soc_core_argdict(args))
	soc.submodules.crg = _CRG_arty(platform, sys_clk_freq, False)

	if DVI:
		from litex.soc.cores.video import VideoS7HDMIPHY
		platform.add_extension([("hdmi_out", 0, #DVI pmod breakout on pmod C (seems not working in others than C)
			Subsignal("data0_p", Pins("pmodc:0"), IOStandard("TMDS_33")),
			Subsignal("data0_n", Pins("pmodc:1"), IOStandard("TMDS_33")),
			Subsignal("data1_p", Pins("pmodc:2"), IOStandard("TMDS_33")),
			Subsignal("data1_n", Pins("pmodc:3"), IOStandard("TMDS_33")),
			Subsignal("data2_p", Pins("pmodc:4"), IOStandard("TMDS_33")),
			Subsignal("data2_n", Pins("pmodc:5"), IOStandard("TMDS_33")),
			Subsignal("clk_p",   Pins("pmodc:6"), IOStandard("TMDS_33")),
			Subsignal("clk_n",   Pins("pmodc:7"), IOStandard("TMDS_33")))])
		soc.submodules.videophy = VideoS7HDMIPHY(platform.request("hdmi_out"), clock_domain="hdmi")
		add_video_custom_generator(soc, phy=soc.videophy, timings="640x480@60Hz", clock_domain="hdmi")
	else:
		from litex.soc.cores.video import VideoVGAPHY
		platform.add_extension([("vga", 0, #PMOD VGA on pmod B & C
			Subsignal("hsync", Pins("U14")), #pmodc.4
			Subsignal("vsync", Pins("V14")), #pmodc.5
			Subsignal("r", Pins("E15 E16 D15 C15")), #pmodb.0-3
			Subsignal("g", Pins("U12 V12 V10 V11")), #pmodc.0-3
			Subsignal("b", Pins("J17 J18 K15 J15")), #pmodb.4-7
			IOStandard("LVCMOS33"))])
		cd_vga = "vga" if soc.crg.cd_vga != soc.crg.cd_sys else "sys"
		soc.submodules.videophy = VideoGenericPHY_SDR(platform.request("vga"), clock_domain=cd_vga)
		add_video_custom_generator(soc, phy=soc.videophy, timings="640x480@60Hz", clock_domain=cd_vga	)
	return soc #TODO: review code on pipelinec-graphics repo


if __name__ == "__main__":
	import sys
	boardname = sys.argv[1]
	sys.argv = sys.argv[1:] #remove first argument
	toolchain = None
	if len(sys.argv) > 2:
		toolchain=sys.argv[2]
		sys.argv = sys.argv[:2]

	import argparse
	parser = argparse.ArgumentParser()
	soc_core_args(parser)
	args = parser.parse_args()

	if boardname == "de0nano": soc = build_de0nano(args)
	if boardname == "arty": soc = build_arty(args, toolchain)

	soc.platform.add_source("build/top_instance.v") #generated verilog for graphics

	builder = Builder(soc)
	builder.build(run=True)



