#include <stdio.h>
#include <string.h>
#include <generated/csr.h>

#define DISABLE_HARDWARE_ACCEL
#include "moduledef.h"
#include "types.h"

#define CFLEX_SIMULATION
//#include "mandel_fp32.cc"
#include "human.cc"

unsigned run_mandel(uint32_t *fb, bool accel)
{
	unsigned errors = 0;
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

			//uint32 color = (r<<8) | (r > 255 ? 0 : 0xC0);
			uint32 color = r;
			
			if(accel)
				*pix = color;
			else
			{
				bool ok = *pix == color;
				if(!ok)
				{
					printf("results does not match, inputs %d, %d\n", xc, yc);
					++errors;
				}
				*pix = ok ? 0xFF0000 : 0xFFFFFF;
			}
		}
	}
	return errors;
}

int main()
{
	//TODO: handle serial IRQ

	uint32_t *fb = (uint32_t *) VIDEO_FRAMEBUFFER_BASE;
	memset(fb, 0x10, VIDEO_FRAMEBUFFER_HRES*VIDEO_FRAMEBUFFER_HRES*4);

	for(;;)
	{
		run_mandel(fb, true);
		int err = run_mandel(fb, false);
		printf("Accelerator vs. software errors: %d\n", err);
		flush_l2_cache();
	}

	return 0;
}
