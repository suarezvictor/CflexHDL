#include <stdio.h>
#include <string.h>
#include <generated/csr.h>

#define DISABLE_HARDWARE_ACCEL
#include "moduledef.h"
#include "types.h"

#define CFLEX_SIMULATION

#ifdef MAIN_SRC
#define STR(x)  #x
#define XSTR(x) STR(x)
#include XSTR(MAIN_SRC) //include depends on a macro
#else
#include "human.cc"
#endif


static inline uint32_t floatBitsToUInt(float x) { return *(uint32_t*) &x; }

unsigned run_shader(uint32_t *fb, uint32_t frame, bool accel)
{
	unsigned errors = 0;
	for(int y = 0; y < VIDEO_FRAMEBUFFER_VRES; ++y)
	{
		for(int x = 0; x < VIDEO_FRAMEBUFFER_HRES; ++x)
		{
			uint32_t *pix = &fb[y*VIDEO_FRAMEBUFFER_HRES+x];
			float xc = 2.f*x/VIDEO_FRAMEBUFFER_HRES - 1.f;
			float yc = 2.f*y/VIDEO_FRAMEBUFFER_VRES - 1.f;
			uint32 color;
			
			if(accel)
			{
				fp32_ua_write(floatBitsToUInt(xc));
				fp32_ub_write(floatBitsToUInt(yc));
				fp32_frame_write(frame);
				fp32_run_write(1);
				while(!fp32_done_read());
				color = fp32_result_read();
				fp32_run_write(0);
			}
			else
				_shader(xc, yc, frame, color); //software version

			if(accel)
				*pix = color;
			else
			{
				bool ok = *pix == color;
				if(!ok)
				{
					printf("results does not match, inputs %d, %d\n", x, y);
					++errors;
				}
				*pix = ok ? 0xFF0000 : 0xFFFFFF;
			}
		}
	}
	return errors;
}

#ifndef SHADER_MAXFRAMES
#define SHADER_MAXFRAMES 1
#endif

int main()
{
	//TODO: handle serial IRQ

	uint32_t *fb = (uint32_t *) VIDEO_FRAMEBUFFER_BASE;
	memset(fb, 0x10, VIDEO_FRAMEBUFFER_HRES*VIDEO_FRAMEBUFFER_HRES*4);

	uint32_t frame = SHADER_MAXFRAMES;
	for(;;)
	{
		printf("Frame %ld\n", frame);
		run_shader(fb, frame, true);
		if(frame >= SHADER_MAXFRAMES)
		{
			int err = run_shader(fb, frame, false);
			printf("Accelerator vs. software errors: %d\n", err);
			frame = 0;
		}
		else
			++frame;
			
		flush_l2_cache();
	}

	return 0;
}
