// Flying over 3D planes demo
// ported* from Silice HDL to C (compatible with CFlexHDL)
//
// (C) 2022 Victor Suarez Rovere <suarezvictor@gmail.com>
// (C) 2020 Sylvain Lefebvre (original code, MIT LICENSE)
// *https://github.com/sylefeb/Silice/blob/master/projects/vga_demo/vga_flyover3d.ice

#include "cflexhdl.h"
#include "vga_config.h"

typedef uint8 uint_color_depth;
MODULE frame_display(
  const uint10 &pix_x,
  const uint10 &pix_y,
  const uint1  &pix_active,
  const uint1  &pix_vblank,
  uint_color_depth &pix_r,
  uint_color_depth &pix_g,
  uint_color_depth &pix_b
) {

   // display frame
   uint16 frame = 0;
   uint16 x;
   uint16 y;

   while (1)
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
         pix_b = x^y;

         wait_clk();
       }
     }
     frame = frame + 1;
   }

}

// -------------------------

