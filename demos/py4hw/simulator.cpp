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

inline void render_pixel()
{
  //printf("x, y %d, %d, hactive %d, vactive %d\n", top->x, top->y, top->hactive, top->vactive);
  if(!top->hactive || !top->vactive)
    return;

  pixels[top->y][top->x] = (top->b) | (top->g << 8) | (top->r << 16);
}

inline bool simulation_display()
{
  if(top->x != 0 || top->y != 0) //if not about to start
    return true;

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
        top->y = 0;
      top->x = 0;
  }
  if(!simulation_display())
	throw false;
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
    
	float fps = float(framecount)*higres_ticks_freq()/(higres_ticks()-start_time);
	printf("FPS %.1f, pixel clock %.1f MHz\n", fps, fps*FRAME_WIDTH*FRAME_HEIGHT*1e-6);
	fb_deinit(&fb);
	return 0;
}

