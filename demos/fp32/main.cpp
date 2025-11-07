#include <stdio.h>
#include <string.h>
#include <generated/csr.h>

#define DISABLE_HARDWARE_ACCEL
#include "moduledef.h"
#include "types.h"

#define CFLEX_SIMULATION
#include "mandel_fp32.cc"



#ifndef VIDEO_FRAMEBUFFER_BASE
#error VIDEO_FRAMEBUFFER_BASE not defined
#endif


int main()
{
	uint32_t *fb = (uint32_t *) VIDEO_FRAMEBUFFER_BASE;
	printf("Framebuffer (%dx%d) at %p\n", VIDEO_FRAMEBUFFER_HRES, VIDEO_FRAMEBUFFER_VRES, fb);
	memset(fb, 0x10, VIDEO_FRAMEBUFFER_HRES*VIDEO_FRAMEBUFFER_HRES*4);

	for(int y = 0; y < VIDEO_FRAMEBUFFER_VRES; y += 10)
	{
		for(int x = 0; x < VIDEO_FRAMEBUFFER_HRES; x += 10)
		{
			uint32 r;
			_mandel(x - VIDEO_FRAMEBUFFER_HRES/2, y - VIDEO_FRAMEBUFFER_VRES/2, r);
			fb[y*VIDEO_FRAMEBUFFER_HRES+x] = (r<<8) | 0xC0;
		}
	}
	flush_l2_cache();

	printf("Done\n");
	return 0;
}
