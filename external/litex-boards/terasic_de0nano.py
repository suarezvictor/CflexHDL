#!/usr/bin/env python3

#
# This file is part of LiteX-Boards.
#
# Copyright (c) 2015-2020 Florent Kermarrec <florent@enjoy-digital.fr>
# Copyright (c) 2021 Victor Suarez Rovere <suarezvictor@gmail.com>
#
# SPDX-License-Identifier: BSD-2-Clause


"""

HIGH RESOLUTION VIDEO DRIVER USING 16-bit SDRAM CHIP (1024x768@32bpp and 1280x720@16bpp)
Based on DE0-Nano board (Cyclone IV FPGA with a 143Mhz capable SDRAM, 16-bit bus)
HDMI output using LVDS outputs + decoupling capacitors

1024x768 characteristics:
===============================
Name: "1024x768@50Hz(RB)"
Pixel clock: 46.42MHz
Color depth: 32-bit (RGBA888)
Vertical rate: 50Hz
Sync polarity*: -hsync -vsync
Sys clock: 72.2MHz
SDRAM clock: 144.4MHz
Build Command: ./terasic_de0nano.py --with-video-framebuffer --sys-clk-freq=72222222 --sdram-rate=1:2  --build


1280x720 characteristics:
===============================
Name: "1280x720@60Hz(RB)"
Pixel clock: 61.9 MHz
Color depth: 16-bit (RGB565)
Vertical rate: 60Hz
Sync polarity: -hsync -vsync
Sys clock: 100MHz
SDRAM clock: 100MHz
Build command: ./terasic_de0nano.py --with-video-framebuffer --sys-clk-freq=100000000 --build 


NOTES:
===============================

Timings are based on CVT standard with Reduced Blanking Time (put less pressure in pixel clock)
The timings are obtained by linux "cvt" tool, for example:
#cvt -r 1024 768 60
outputs:
Modeline "1024x768R"   56.00  1024 1072 1104 1184  768 771 775 790 +hsync -vsync

NOTE: besides +hsync is specified, testing shows we beed -hsync
Pixel clock is adjusted to 46.42MHz (instead of 56Mhz) to put less pressure on SDRAM and to mantain < 2/3 rate with SDRAM clock (unavoidable in current design)
The result is that framerate gets to 50Hz and not to the 60Hz standard. Anyways most (if not all) monitors will be happy to show 50Hz Vsync if they're capable of 60Hz.
The 50Hz obtained is not exact but very close, maybe output is 49ish-Hz (pixel clock is set to 46.42 instead of 46.77). To more exact adjusting, some more PLL trickery would be needed or blanking timings could be tweaked to sightly off-standard values. In regards to 1280x720, the pixel clock is also not exact but very close (61.9 is used instead of 64.02). Same criteria applies.

In case of 1280x720, the SDRAM is not maxed to their utmost clock capacity since that would require the 1:2 logic but in that case pixel clock would be too close to sys clock. Some trickery with clock domain crossig would be needed to achieve higher SDRAM clock rates (HIGHLY DESIRED FEATURE).

"""

import os
import argparse

from migen import *
from migen.genlib.resetsync import AsyncResetSynchronizer

from litex.build.io import DDROutput

from litex_boards.platforms import de0nano

from litex.soc.cores.clock import CycloneIVPLL
from litex.soc.integration.soc_core import *
from litex.soc.integration.builder import *
#from litex.soc.cores.video import VideoHDMIPHY
from litex.soc.cores.code_tmds import TMDSEncoder
from litex.soc.cores.led import LedChaser

from litedram.modules import IS42S16160
from litedram.phy import GENSDRPHY, HalfRateGENSDRPHY

from litex.soc.interconnect import stream
from litex.soc.interconnect.csr import *
from litex.soc.cores.video import video_timings, video_timing_layout, video_data_layout

from migen.genlib.cdc import MultiReg

class VideoHDMI10to1Serializer(Module):
    def __init__(self, data_i, data_o, clock_domain):
        # Clock Domain Crossing.
        self.submodules.cdc = stream.ClockDomainCrossing([("data", 10)], cd_from=clock_domain, cd_to=clock_domain + "5x")
        self.comb += self.cdc.sink.valid.eq(1)
        self.comb += self.cdc.sink.data.eq(data_i)

        # 10:2 Gearbox.
        self.submodules.gearbox = ClockDomainsRenamer(clock_domain + "5x")(stream.Gearbox(i_dw=10, o_dw=2, msb_first=False))
        self.comb += self.cdc.source.connect(self.gearbox.sink)

        # 2:1 Output DDR.
        self.comb += self.gearbox.source.ready.eq(1)
        self.specials += DDROutput(
            clk = ClockSignal(clock_domain + "5x"),
            i1  = self.gearbox.source.data[0],
            i2  = self.gearbox.source.data[1],
            o   = data_o,
        )

class VideoHDMIPHY(Module):
    def __init__(self, pads, clock_domain="sys", pn_swap=[]):
        self.sink = sink = stream.Endpoint(video_data_layout)

        # # #

        # Always ack Sink, no backpressure.
        self.comb += sink.ready.eq(1)

        # Clocking + Pseudo Differential Signaling.
        self.specials += DDROutput(i1=1, i2=0, o=pads.clk_p, clk=ClockSignal(clock_domain))

        # Encode/Serialize Datas.
        for color in ["r", "g", "b"]:

            # TMDS Encoding.
            encoder = ClockDomainsRenamer(clock_domain)(TMDSEncoder())
            setattr(self.submodules, f"{color}_encoder", encoder)
            self.comb += encoder.d.eq(getattr(sink, color))
            self.comb += encoder.c.eq(Cat(sink.hsync, sink.vsync) if color == "b" else 0) #PATCHED: BGR is correct
            self.comb += encoder.de.eq(sink.de)

            # 10:1 Serialization + Pseudo Differential Signaling.
            c2d  = {"b": 0, "g": 1, "r": 2} #PATCHED: BGR is correct
            data = encoder.out if color not in pn_swap else ~encoder.out
            serializer = VideoHDMI10to1Serializer(
                data_i       = data,
                data_o       = getattr(pads, f"data{c2d[color]}_p"),
                clock_domain = clock_domain,
            )
            setattr(self.submodules, f"{color}_serializer", serializer)


# Video FrameBuffer --------------------------------------------------------------------------------

class VideoFrameBufferN(Module, AutoCSR):
    """Video FrameBuffer"""
    def __init__(self, dram_port, hres=800, vres=600, base=0x00000000, fifo_depth=65536, clock_domain="sys", clock_faster_than_sys=False, bit_depth = 32):
        self.vtg_sink  = vtg_sink = stream.Endpoint(video_timing_layout)
        self.source    = source   = stream.Endpoint(video_data_layout)
        self.underflow = Signal()

        # # #

        # Video DMA.
        from litedram.frontend.dma import LiteDRAMDMAReader
        self.submodules.dma = LiteDRAMDMAReader(dram_port, fifo_depth=fifo_depth//(dram_port.data_width//8), fifo_buffered=True)
        self.dma.add_csr(
            default_base   = base,
            default_length = hres*vres*bit_depth//8,
            default_enable = 0,
            default_loop   = 1
        )

        # If DRAM Data Width > N-bit and Video clock is faster than sys_clk:
        if (dram_port.data_width > bit_depth) and clock_faster_than_sys:
            # Do Clock Domain Crossing first...
            self.submodules.cdc = stream.ClockDomainCrossing([("data", dram_port.data_width)], cd_from="sys", cd_to=clock_domain)
            self.comb += self.dma.source.connect(self.cdc.sink)
            # ... and then Data-Width Conversion.
            if bit_depth > 16:
              self.submodules.conv = stream.Converter(dram_port.data_width, bit_depth)
              self.comb += self.cdc.source.connect(self.conv.sink)
              video_pipe_source = self.conv.source
            else:
              video_pipe_source = self.cdc.source #direct 1:1 connection

        # Elsif DRAM Data Widt < N-bit or Video clock is slower than sys_clk:
        else:
            # Do Data-Width Conversion first...
            if bit_depth > 16:
              self.submodules.conv = stream.Converter(dram_port.data_width, bit_depth)
              self.comb += self.dma.source.connect(self.conv.sink)
              # ... and then Clock Domain Crossing.
              self.submodules.cdc = stream.ClockDomainCrossing([("data", bit_depth)], cd_from="sys", cd_to=clock_domain)
              self.comb += self.conv.source.connect(self.cdc.sink)
              self.comb += If(dram_port.data_width < 32, # FIXME.
                self.cdc.sink.data[ 0: 8].eq(self.conv.source.data[16:24]),
                self.cdc.sink.data[16:24].eq(self.conv.source.data[ 0: 8]),
              )
              video_pipe_source = self.cdc.source
            else:
              # Do Clock Domain Crossing first...
              self.submodules.cdc = stream.ClockDomainCrossing([("data", dram_port.data_width)], cd_from="sys", cd_to=clock_domain)
              self.comb += self.dma.source.connect(self.cdc.sink)
              video_pipe_source = self.cdc.source #direct 1:1 connection

        # Video Generation.
        self.comb += [
            vtg_sink.ready.eq(1),
            If(vtg_sink.valid & vtg_sink.de,
                video_pipe_source.connect(source, keep={"valid", "ready"}),
                vtg_sink.ready.eq(source.valid & source.ready),

            ),
            vtg_sink.connect(source, keep={"de", "hsync", "vsync"}),
            source.r.eq(video_pipe_source.data[16:24] if bit_depth > 16 else Cat(Signal(3, reset = 0), video_pipe_source.data[11:16])),
            source.g.eq(video_pipe_source.data[ 8:16] if bit_depth > 16 else Cat(Signal(2, reset = 0), video_pipe_source.data[ 5:11])),
            source.b.eq(video_pipe_source.data[ 0: 8] if bit_depth > 16 else Cat(Signal(3, reset = 0), video_pipe_source.data[ 0: 5])),
        ]

        # Underflow.
        self.comb += self.underflow.eq(~source.valid)



class _CRG(Module):
    def __init__(self, platform, sys_clk_freq, sdram_rate="1:1"):
        self.rst = Signal()
        self.clock_domains.cd_sys    = ClockDomain()
        if sdram_rate == "1:2":
            self.clock_domains.cd_sys2x    = ClockDomain()
            self.clock_domains.cd_sys2x_ps = ClockDomain(reset_less=True)
        else:
            self.clock_domains.cd_sys_ps = ClockDomain(reset_less=True)

        # # #

        # Clk / Rst
        clk50 = platform.request("clk50")

        # PLL
        self.submodules.pll = pll = CycloneIVPLL(speedgrade="-6")
        self.comb += pll.reset.eq(self.rst)
        pll.register_clkin(clk50, 50e6)
        pll.create_clkout(self.cd_sys,    sys_clk_freq)
        if sdram_rate == "1:2":
            pll.create_clkout(self.cd_sys2x,    2*sys_clk_freq)
            pll.create_clkout(self.cd_sys2x_ps, 2*sys_clk_freq, phase=180)  # Idealy 90° but needs to be increased.
        else:
            pll.create_clkout(self.cd_sys_ps, sys_clk_freq, phase=90)

        video_clock = 25e6 #"800x600@75Hz" =>  49.5e6, "640x480@60Hz" => 25.175e6 "800x600@60Hz"  => 40e6, 1280x720@60Hz(RB) => 61.9e6 1024 46.42e6
        if True: #video clock
            self.clock_domains.cd_hdmi   = ClockDomain()
            self.clock_domains.cd_hdmi5x = ClockDomain()
            pll.create_clkout(self.cd_hdmi,     video_clock, margin=1e-3)
            pll.create_clkout(self.cd_hdmi5x, 5*video_clock, margin=1e-3)

        # SDRAM clock
        sdram_clk = ClockSignal("sys2x_ps" if sdram_rate == "1:2" else "sys_ps")
        self.specials += DDROutput(1, 0, platform.request("sdram_clock"), sdram_clk)

# BaseSoC ------------------------------------------------------------------------------------------

class BaseSoC(SoCCore):
    fifo_depth = 8192
    bit_depth = 16#32

    def __init__(self, sys_clk_freq=int(50e6), with_video_terminal=False, with_video_framebuffer=False, sdram_rate="1:1", **kwargs):
        platform = de0nano.Platform()
        #self.platform = platform #This is by default

        # SoCCore ----------------------------------------------------------------------------------
        SoCCore.__init__(self, platform, sys_clk_freq, **kwargs)

        # CRG --------------------------------------------------------------------------------------
        self.submodules.crg = _CRG(platform, sys_clk_freq, sdram_rate=sdram_rate)

        # SDR SDRAM --------------------------------------------------------------------------------
        if not self.integrated_main_ram_size:
            sdrphy_cls = HalfRateGENSDRPHY if sdram_rate == "1:2" else GENSDRPHY
            self.submodules.sdrphy = sdrphy_cls(platform.request("sdram"), sys_clk_freq)
            self.add_sdram("sdram",
                phy           = self.sdrphy,
                module        = IS42S16160(sys_clk_freq, sdram_rate),
                l2_cache_size = kwargs.get("l2_size", 1024),
                l2_cache_reverse = False
            )
        # Video Terminal ---------------------------------------------------------------------------
        if with_video_terminal:
            self.submodules.videophy = VideoHDMIPHY(platform.request("gpdi"), clock_domain="hdmi")
            self.add_video_terminal(phy=self.videophy, timings="640x480@75Hz", clock_domain="hdmi")
        # Video Framebuffer  ---------------------------------------------------------------------------
        if with_video_framebuffer:
            self.submodules.videophy = VideoHDMIPHY(platform.request("gpdi"), clock_domain="hdmi")
            self.add_video_framebuffer(phy=self.videophy, timings="640x480@60Hz", clock_domain="hdmi")

        # Leds -------------------------------------------------------------------------------------
        self.submodules.leds = LedChaser(
            pads         = platform.request_all("user_led"),
            sys_clk_freq = sys_clk_freq)



    def add_video_framebuffer(self, name="video_framebuffer", phy=None, timings="800x600@60Hz", clock_domain="sys"):
        # Imports.
        from litex.soc.cores.video import VideoTimingGenerator, VideoFrameBuffer

        # Video Timing Generator.
        vtg = VideoTimingGenerator(default_video_timings=timings if isinstance(timings, str) else timings[1])
        vtg = ClockDomainsRenamer(clock_domain)(vtg)
        setattr(self.submodules, f"{name}_vtg", vtg)

        # Video FrameBuffer.
        timings = timings if isinstance(timings, str) else timings[0]
        base = self.mem_map.get(name, 0x41800000)
        hres = int(timings.split("@")[0].split("x")[0])
        vres = int(timings.split("@")[0].split("x")[1])
        vfb = VideoFrameBufferN(self.sdram.crossbar.get_port(),
            hres = hres,
            vres = vres,
            base = base,
            clock_domain          = clock_domain,
            clock_faster_than_sys = vtg.video_timings["pix_clk"] > self.sys_clk_freq,
            fifo_depth = self.fifo_depth,
            bit_depth = self.bit_depth
        )
        setattr(self.submodules, name, vfb)

        # Connect Video Timing Generator to Video FrameBuffer.
        self.comb += vtg.source.connect(vfb.vtg_sink)

        # Connect Video FrameBuffer to Video PHY.
        self.comb += vfb.source.connect(phy if isinstance(phy, stream.Endpoint) else phy.sink)

        # Constants.
        self.add_constant("VIDEO_FRAMEBUFFER_BASE", base)
        self.add_constant("VIDEO_FRAMEBUFFER_HRES", hres)
        self.add_constant("VIDEO_FRAMEBUFFER_VRES", vres)
        self.add_constant("VIDEO_FRAMEBUFFER_DEPTH", self.bit_depth)


from litex.build.generic_platform import *

def main():
    parser = argparse.ArgumentParser(description="LiteX SoC on DE0-Nano")
    parser.add_argument("--build",        action="store_true", help="Build bitstream")
    parser.add_argument("--load",         action="store_true", help="Load bitstream")
    parser.add_argument("--sys-clk-freq", default=50e6,        help="System clock frequency (default: 50MHz)")
    parser.add_argument("--sdram-rate",   default="1:1",       help="SDRAM Rate: 1:1 Full Rate (default), 1:2 Half Rate")
    parser.add_argument("--with-video-terminal", action="store_true", help="Enable Video Terminal (VGA)")
    parser.add_argument("--with-video-framebuffer", action="store_true", help="Enable Video Framebuffer (VGA)")
    builder_args(parser)
    soc_core_args(parser)
    args = parser.parse_args()

    soc = BaseSoC(
        sys_clk_freq = int(float(args.sys_clk_freq)),
        with_video_terminal = args.with_video_terminal,
        with_video_framebuffer = args.with_video_framebuffer,
        sdram_rate   = args.sdram_rate,
        **soc_core_argdict(args)
    )
    builder = Builder(soc, **builder_argdict(args))
    builder.build(run=args.build)

    if args.load:
        prog = soc.platform.create_programmer()
        prog.load_bitstream(os.path.join(builder.gateware_dir, soc.build_name + ".sof"))

if __name__ == "__main__":
    main()


