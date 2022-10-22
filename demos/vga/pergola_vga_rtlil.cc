// Machine-generated using Migen2CflexHDL - do not edit! -- thru RTLIL
// check source license to know the license of this file

#include "cflexhdl.h"

MODULE frame_display(
	const uint12& pix_x,
	const uint12& pix_y,
	const uint1& pix_active,
	const uint1& pix_vblank,
	uint8& pix_r,
	uint8& pix_g,
	uint8& pix_b
) {
	uint1 clk = 0;
	uint1 rst = 0;
//WIRES
  uint1 var1;
  uint8 var10;
  uint8 var12;
  uint20 var15;
  uint8 var16;
  uint1 var17;
  uint16 var18;
  uint16 var20;
  uint16 var22;
  uint16 var24;
  uint20 var28;
  uint1 var3;
  uint25 var30;
  uint17 var31;
  uint8 var32;
  uint8 var34;
  uint16 var36;
  uint16 var38;
  uint16 var40;
  uint16 var42;
  uint17 var44;
  uint25 var47;
  uint17 var5;
  uint17 var6;
  uint8 var8;
  uint8 var9;
  uint16 frame;
  uint16 frame_next;
  uint8 pix_b_next;
  uint8 pix_g_next;
  uint8 pix_r_next;
  uint1 vsync_r;
  uint1 vsync_r_next;
//INIT
  vsync_r = 0;
  frame = 0;
  pix_r = 0;
  pix_g = 0;
  pix_b = 0;
//-------LOOP-----------
while(always(clk))
{
//CELLS
   var10 = ~ frame;
  var12 = frame ? var10 : frame;
   var8 = + var9;
  var18 = pix_x + frame;
   var1 = ~ vsync_r;
  var20 = pix_x - frame;
  var22 = pix_y ? var18 : var20;
  var24 = var22 & pix_y;
   var17 = var24  != 0;
  var16 = var17 ? 255 : 0;
  var28 = pix_y * var16;
   var32 = ~ frame;
  var34 = frame ? var32 : frame;
  var36 = pix_x + frame;
  var38 = pix_x - frame;
  var3 = var1 & pix_vblank;
  var40 = pix_y ? var36 : var38;
  var42 = var40 ^ pix_y;
  var44 = var34 + var42;
   var31 = ~ var44;
  var47 = var31 * 255;
  var6 = frame + 1;
//CONNECTIONS
   var5 = var6;
   var9 = var12;
   var15 = var28;
   var30 = var47;
//CASES
  vsync_r_next = vsync_r;
  vsync_r_next = pix_vblank;
  if(rst)
  {
    vsync_r_next = 0;
  }
  frame_next = frame;
  if(var3)
  {
    frame_next = var5;
  }
  if(rst)
  {
    frame_next = 0;
  }
  pix_r_next = pix_r;
  pix_r_next = var8;
  if(rst)
  {
    pix_r_next = 0;
  }
  pix_g_next = pix_g;
  pix_g_next = var15;
  if(rst)
  {
    pix_g_next = 0;
  }
  pix_b_next = pix_b;
  pix_b_next = var30;
  if(rst)
  {
    pix_b_next = 0;
  }
//SYNC
  vsync_r = vsync_r_next;
  frame = frame_next;
  pix_r = pix_r_next;
  pix_g = pix_g_next;
  pix_b = pix_b_next;
}
}
//END MODULE

#include "vga_config.h"
