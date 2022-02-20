// Generic simulator for graphics projects (for verilator & regular compiler)
// Copyright (C) 2022 Victor Suarez Rovere <suarezvictor@gmail.com>
//
// just copy your frame_display() render logic to build/compiled_simulator.cpp
// or call verilator to generate VM_frame_display class

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480

#include <stdio.h>
#include <stdint.h>
#include "sim_fb.h"

#ifndef CFLEX_VERILATOR
#define CFLEX_SIMULATION
void wait_clk(); //TODO: make inline
#include "build/compiled_simulator.cpp"
struct
{
  uint10 out_pix_x, out_pix_y;
  uint1 out_video_hs, out_video_vs, out_video_de;
  uint_color_depth out_video_r, out_video_g, out_video_b;
} top;
#else
#include "VM_vga_demo.h"
VM_vga_demo top;
#endif

fb_handle_t fb;
unsigned framecount = 0;

inline void wait_clk() //vga controller logic & display
{
  static uint32_t pixels[FRAME_HEIGHT][FRAME_WIDTH];

  if(top.out_video_de)
  {
    pixels[top.out_pix_y][top.out_pix_x] =
      ((top.out_video_b) | (top.out_video_g << 8) | (top.out_video_r << 16)) << 2;  //original color: 6-bit
  }

#ifdef CFLEX_VERILATOR
  top.clock = 0; top.eval();
  top.clock = 1; top.eval();
#else
  vga_timing(top.out_video_hs, top.out_video_vs, top.out_video_de, top.out_pix_x, top.out_pix_y);
#endif

  if(top.out_pix_x == 0 && top.out_pix_y == 0) //if about to start frame
  {
      if(fb_should_quit())
        throw false;
	  fb_update(&fb, pixels, sizeof(pixels[0]));
      ++framecount;
  }
}

#ifdef CFLEX_VERILATOR
void run()
{
  for(;;)
    wait_clk();
}
#else
//#include "build/compiled_simulator.cpp"
void run()
{
  //runs in loop until the vga controller throws exception
  frame_display(top.out_pix_x, top.out_pix_y, top.out_video_de, top.out_video_vs,
    top.out_video_r, top.out_video_g, top.out_video_b);
}
#endif

int main()
{
	fb_init(FRAME_WIDTH, FRAME_HEIGHT, false, &fb);
	uint64_t start_time = higres_ticks();

	try { run(); } catch(...) {} //exits by exception

	printf("FPS %f\n", float(framecount)*higres_ticks_freq()/(higres_ticks()-start_time));
	fb_deinit(&fb);
	return 0;
}

