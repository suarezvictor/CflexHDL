# Glue for led demo & generic SoC builder
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

def build_board(verilogsrc, boardmodule):
	import importlib
	board = importlib.import_module(boardmodule)
	build(board, verilogsrc)

if __name__ == "__main__":
	import sys
	build_board(verilogsrc=sys.argv[1], boardmodule=sys.argv[2])


