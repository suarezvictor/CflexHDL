#include <stdio.h>
#include <string.h>
#include <generated/csr.h>

#define DISABLE_HARDWARE_ACCEL
#include "moduledef.h"
#include "types.h"

#define CFLEX_SIMULATION
#include "mandel_fp32.cc"


void run_mandel(uint32_t *fb, bool accel)
{
	for(int y = 0; y < VIDEO_FRAMEBUFFER_VRES; ++y)
	{
		for(int x = 0; x < VIDEO_FRAMEBUFFER_HRES; ++x)
		{
			uint32_t *pix = &fb[y*VIDEO_FRAMEBUFFER_HRES+x];
			int32 xc = x - VIDEO_FRAMEBUFFER_HRES/2;
			int32 yc = y - VIDEO_FRAMEBUFFER_VRES/2;
			uint32 r;
			
			if(accel)
			{
				fp32_ua_write(xc);
				fp32_ub_write(yc);
				fp32_run_write(1);
				while(!fp32_done_read());
				r = fp32_result_read();
				fp32_run_write(0);
			}
			else
				_mandel(xc, yc, r); //software version

			uint32 color = (r<<8) | (r > 255 ? 0 : 0xC0);
			
			if(accel)
				*pix = color;
			else
				*pix = (*pix == color) ? 0xFF0000 : 0xFFFFFF;
		}
	}
}

int main()
{
	uint32_t *fb = (uint32_t *) VIDEO_FRAMEBUFFER_BASE;
	printf("Framebuffer (%dx%d) at %p\n", VIDEO_FRAMEBUFFER_HRES, VIDEO_FRAMEBUFFER_VRES, fb);
	memset(fb, 0x10, VIDEO_FRAMEBUFFER_HRES*VIDEO_FRAMEBUFFER_HRES*4);

	for(;;)
	{
		run_mandel(fb, true);
		run_mandel(fb, false);
		flush_l2_cache();
		printf("Done\n");
	}

	return 0;
}
