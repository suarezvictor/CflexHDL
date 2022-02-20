# Copyright (C) 2022 Victor Suarez Rovere <suarezvictor@gmail.com>

from migen import *

def build_top_instance(platform):
	done = Signal();
	out_clock = Signal()
	pin = platform.request("user_led", 0)
	return Instance(
    	"M_led_glow",
	    o_out_led = pin,
	    i_in_run = ~ResetSignal("sys"),
	    o_out_done = done,
	    i_reset = ResetSignal("sys"),
	    o_out_clock = out_clock,
	    i_clock = ClockSignal("sys")
    	)

def build(board, top_v):
	platform = board.Platform()
	platform.add_source(top_v)
	top = Module()
	top.specials += build_top_instance(platform)
	platform.build(top)

import litex_boards.platforms.de0nano as board # select for Terasic DE0-Nano board
#import litex_boards.platforms.arty as board # select for Digilent Arty board
build(board, "./build/top_instance.v")


