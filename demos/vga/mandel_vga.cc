// Simple mandelbrot demo
// (C) 2025 Victor Suarez Rovere <suarezvictor@gmail.com>

#include "cflexhdl.h"
#include "vga_config.h"
#include "../fp32/mandel_fp32.cc"


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


     for(yc = -240; yc < 240; yc=yc+1)
     {
       for(xc = -320; xc < 320; xc=xc+1)
       {
       
		uint32 i = 0;
		_mandel(xc, yc, i);
		pix_r = i == 16 ? 0 : 0xC0;
		pix_g = i << 4;
		pix_b = 0;

         wait_clk();
       }

       while(!pix_active)
         wait_clk();
     }

     frame = frame + 1;
   }

}

