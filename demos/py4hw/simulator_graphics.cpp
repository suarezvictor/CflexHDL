// Generic simulator for py4hw graphics projects
// Copyright (C) 2023 Victor Suarez Rovere <suarezvictor@gmail.com>

#include <stdio.h>
#include <stdint.h>
#include "sim_fb.h"

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480


fb_handle_t fb;
unsigned framecount = 0;
static uint32_t pixels[FRAME_HEIGHT][FRAME_WIDTH];


#include "cflexhdl.h"
struct vga_demo
{
  uint10 x, y;
  bool hactive, vactive;
  uint8 r, g, b;
} top_instance;
#define top (&top_instance)
static uint64_t clk_count = 0;

inline void render_pixel()
{
  //printf("x, y %d, %d, hactive %d, vactive %d\n", top->x, top->y, top->hactive, top->vactive);
  if(!top->hactive || !top->vactive)
    return;

  pixels[top->y][top->x] = (top->b) | (top->g << 8) | (top->r << 16);
}

inline bool simulation_display()
{
  if(fb_should_quit())
	return false;

  fb_update(&fb, pixels, sizeof(pixels[0]));
  ++framecount;
  return true;
}

inline void wait_clk()
{
  render_pixel();
  if(top->hactive)
    ++top->x;
  else
  {
      if(top->vactive)
      {
        if(top->x != 0)
          ++top->y;
      }
      else
      {
        if(top->y != 0)
        {
          if(!simulation_display())
              throw false;
        }
        top->y = 0;
      }
      top->x = 0;
  }
  ++clk_count;
}

#include "vga_py4hw.cc" //generated source

void run()
{
	bool reset = false;
    VGATestPattern(reset, top->hactive, top->vactive, top->r, top->g, top->b);
}

int main()
{
	fb_init(FRAME_WIDTH, FRAME_HEIGHT, false, &fb); //set vsync parameter to true to limit FPS
	uint64_t start_time = higres_ticks();

    try {
	  run();
    }
    catch(...) {} //only exits by exception
    
    float dt = (higres_ticks()-start_time)/float(higres_ticks_freq());
	float fps = framecount/dt;
	printf("time %.1fs, FPS %.1f, clock %.1f MHz\n", dt, fps, clk_count*1.e-6/dt);
	fb_deinit(&fb);
	return 0;
}

