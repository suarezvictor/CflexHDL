// Simple mandelbrot demo
// (C) 2023 Victor Suarez Rovere <suarezvictor@gmail.com>

#include "cflexhdl.h"
#include "vga_config.h"

MODULE frame_display(
  const uint10 &pix_x,
  const uint10 &pix_y,
  const uint1  &pix_active,
  const uint1  &pix_vblank,
  uint8 &pix_r,
  uint8 &pix_g,
  uint8 &pix_b
) {

   // display frame
   uint16 frame = 0;
   for(;;)
   {

     while(pix_vblank)
       wait_clk();
       
     int16 xc, yc;

     const float xscale = 1./FRAME_WIDTH;
     const float yscale = 1./FRAME_HEIGHT;

     for(yc = 0; yc < FRAME_HEIGHT; yc=yc+1)
     {
       for(xc = 0; xc < FRAME_WIDTH; xc=xc+1)
       {
       
         //mandelbrot generator
		float x0 = xc*xscale*4-2;
		float y0 = yc*yscale*4-2;


		float x=0;
		float y=0;

		int16 i = 0;
		int16 c;
		for (c = 0; c < 16; c = c + 1)
		{
			float xsq = x * x;
			float ysq = y * y;

			float sum = xsq + ysq;

			if(sum < 4)
			{
				y = 2*x * y + y0;
				x = xsq - ysq + x0;
				i = i + 1;
			}
		} 
		pix_r = i * 16;
		pix_g = 32;
		pix_b = 0;

         wait_clk();
       }

       while(!pix_active)
         wait_clk();
     }

     frame = frame + 1;
   }

}

// -------------------------

