// Generic simulator for graphics projects (for verilator & regular compiler)
// Copyright (C) 2022 Victor Suarez Rovere <suarezvictor@gmail.com>
//
// just copy your frame_display() render_pixel logic to build/compiled_simulator.cpp
// or call verilator to generate VM_vga_demo class

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480

#include <stdio.h>
#include <stdint.h>
#include "sim_fb.h"

fb_handle_t fb;
unsigned framecount = 0;
static uint32_t pixels[FRAME_HEIGHT][FRAME_WIDTH];

#include "cflexhdl.h"

#ifndef CFLEX_VERILATOR
struct VM_vga_demo //mimics Verilator variables
{
  uint10 out_pix_x, out_pix_y;
  uint1 out_video_hs, out_video_vs, out_video_de;
  uint8 out_video_r, out_video_g, out_video_b;
} top_instance;
#define top (&top_instance)
#else
#include "VM_vga_demo.h"
VM_vga_demo *top = new VM_vga_demo;
#endif

inline void render_pixel()
{
  //printf("x, y %d, %d\n", top->out_pix_x, top->out_pix_y);
  if(top->out_video_de)
    pixels[top->out_pix_y][top->out_pix_x] =
      ((top->out_video_b) | (top->out_video_g << 8) | (top->out_video_r << 16)) << 2;  //original color: 6-bit
}

inline bool simulation_display()
{
    if(top->out_pix_x == 0 && top->out_pix_y == 0) //if about to start frame
    {
	  if(fb_should_quit())
		return false;
	  fb_update(&fb, pixels, sizeof(pixels[0]));
      ++framecount;
    }
	return true;
}

#ifndef CFLEX_VERILATOR
#include "vga.cc"
#ifdef CFLEX_NO_COROUTINES
void wait_clk()
{
  render_pixel();
  vga_timing(top->out_video_hs, top->out_video_vs, top->out_video_de, top->out_pix_x, top->out_pix_y);
  if(!simulation_display())
	throw false;
}
#endif
#else
#endif

#include "build/compiled_simulator.cpp"

void run()
{
#ifdef CFLEX_VERILATOR
  for(;;)
  {
	top->clock = 0; top->eval();
	top->clock = 1; top->eval();
	render_pixel();
	if(!simulation_display())
		break;
  }
#else
#ifdef CFLEX_NO_COROUTINES
  //runs in loop until the vga controller throws exception
  try { 
    frame_display(top->out_pix_x, top->out_pix_y, top->out_video_de, top->out_video_vs,
      top->out_video_r, top->out_video_g, top->out_video_b);
  } catch(...) {} //exits by exception
#else
	printf("init\n");

  auto fd = frame_display(top->out_pix_x, top->out_pix_y, top->out_video_de, top->out_video_vs,
    top->out_video_r, top->out_video_g, top->out_video_b);
  auto vt = vga_timing(top->out_video_hs, top->out_video_vs, top->out_video_de, top->out_pix_x, top->out_pix_y);

  for(;;)
  {
    fd.clock();
	vt.clock();
	render_pixel();
    if(!simulation_display())
	  break;
  }
#endif
#endif
}

int main()
{
	fb_init(FRAME_WIDTH, FRAME_HEIGHT, false, &fb); //set vsync parameter to true to limit FPS
	uint64_t start_time = higres_ticks();

	run();

	printf("FPS %f\n", float(framecount)*higres_ticks_freq()/(higres_ticks()-start_time));
	fb_deinit(&fb);
	return 0;
}

