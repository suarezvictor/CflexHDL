// Simple video pattern generation demo
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
   uint16 x;
   uint16 y;
   
   for(;;)
   {
     while(pix_vblank != 0)
       wait_clk();

     for(y = 0; y < FRAME_HEIGHT; y=y+1)
     {
       while(!pix_active)
         wait_clk();

       for(x = 0; x < FRAME_WIDTH; x=x+1)
       {
         pix_r = x;
         pix_g = y + frame;
         pix_b = x ^ y;

         wait_clk();
       }
     }
     frame = frame + 1;
   }

}

// -------------------------

