#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "sim_fb.h"

#define VIDEO_FRAMEBUFFER_HRES 640
#define VIDEO_FRAMEBUFFER_VRES 480
#define CFLEX_SIMULATION
#define CFLEX_NO_COROUTINES
#include "cflexhdl.h"

fb_handle_t fb;
unsigned framecount = 0;
static uint32_t pixels[VIDEO_FRAMEBUFFER_VRES][VIDEO_FRAMEBUFFER_HRES];

#include "human.cc"

void run_shader(uint32_t *fbuf, uint32_t framecount)
{
	for(int y = 0; y < VIDEO_FRAMEBUFFER_VRES; ++y)
	{
		for(int x = 0; x < VIDEO_FRAMEBUFFER_HRES; ++x)
		{
			uint32_t *pix = &fbuf[y*VIDEO_FRAMEBUFFER_HRES+x];
			int32 xc = x - VIDEO_FRAMEBUFFER_HRES/2;
			int32 yc = y - VIDEO_FRAMEBUFFER_VRES/2;
			uint32 color;
			_mandel(xc, yc, framecount, color); //software version
			*pix = color;
		}
	}
}

inline bool simulation_display()
{
  if(fb_should_quit())
	return false;

  fb_update(&fb, pixels, sizeof(pixels[0]));
  return true;
}

int main()
{
	fb_init(VIDEO_FRAMEBUFFER_HRES, VIDEO_FRAMEBUFFER_VRES, false, &fb); //set vsync parameter to true to limit FPS
	
	for(;;)
	{
		uint64_t start_time = higres_ticks();
		run_shader(&pixels[0][0], framecount);

		float dt = (higres_ticks()-start_time)/float(higres_ticks_freq());
		float fps = framecount/dt;
		printf("FPS %.1f, frame %d\n", 1./dt, framecount);

		if(!simulation_display())
		  break;
		
		++framecount;
	}

	fb_deinit(&fb);
	return 0;
}

#include "sim_fb.c"

