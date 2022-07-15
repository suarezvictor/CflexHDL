// Flying over 3D planes demo
// ported* from Silice HDL to C (compatible with CFlexHDL)
//
// (C) 2022 Victor Suarez Rovere <suarezvictor@gmail.com>
// (C) 2020 Sylvain Lefebvre (original code, MIT LICENSE)
// *https://github.com/sylefeb/Silice/blob/master/projects/vga_demo/vga_flyover3d.ice

#include "cflexhdl.h"
#include "vga_config.h"

/*
$include('vga_demo_main.ice')

$$div_width    = 16
$$div_unsigned = 1
$$div_shrink   = 3
$include('../../common/divint_any.ice')
*/

struct div_any : silice_module {}; //TODO: automatic import

typedef uint16 uint_div_width;
class div16 : div_any
{
 uint_div_width& _ret;
public:
 div16(uint_div_width& ret) : _ret(ret) {}
 void operator()(int16 n, int16 d) { _ret = n/d; }
};

// -------------------------
/*
algorithm frame_display(
  input   uint10 pix_x,
  input   uint10 pix_y,
  input   uint1  pix_active,
  input   uint1  pix_vblank,
  output! uint$color_depth$ pix_r,
  output! uint$color_depth$ pix_g,
  output! uint$color_depth$ pix_b
) <autorun> {
*/

typedef uint8 uint_color_depth; //FIXME: should match 6 bits
MODULE frame_display(
  const uint10 &pix_x,
  const uint10 &pix_y,
  const uint1  &pix_active,
  const uint1  &pix_vblank,
  uint_color_depth &pix_r,
  uint_color_depth &pix_g,
  uint_color_depth &pix_b
) {
#if 0
  //quick pattern test
  while (always())
  {
    pix_r = pix_x;
    pix_g = pix_y;
    pix_b = pix_x+pix_y;
  }
#else
  //uint$div_width$ cur_inv_y = 0;
  uint_div_width inv_y = 0;

  union {
    uint_div_width cur_inv_y;
    //field definitions for later access of internal bits
    struct { uint_div_width :3; uint_div_width _3_7 :7; } cur_inv_y_bitsA;
    struct { uint_div_width _0_6 :6; } cur_inv_y_bitsB; //maybe A and B are not both needed
  };

  uint8 u      = 0;
  uint8 v      = 0;

  uint15 maxv  = 22000;

  uint16 pos_u = 0;
  uint16 pos_v = 0;

  uint8 lum    = 0;
  uint1 floor  = 0;
  uint9 offs_y = 0;

/*
  div$div_width$ div(
    ret :> inv_y
  );
*/
  div16 div(inv_y);

  //NOTE: assignments put after declarations (a Silice requirement)
  cur_inv_y = 0;
  offs_y = 0;
  u = 0;
  v = 0;
  lum = 0;

  //pix_r  := 0; pix_g := 0; pix_b := 0; //statement to be run "always" moved inside the inner while

  // ---------- show time!
  while (always()) { // while(1)
	  // display frame
	  //while (pix_vblank == 0) {
	  while (always() && pix_vblank == 0) {

      pix_r = 0; pix_g = 0; pix_b = 0;

      if (pix_x < 640) { //if (pix_active) { //FIXME: problems with LiteX and pix_active
        if (pix_y < 240) {
          offs_y = (240 + 32) - pix_y;
          floor  = 0;
        } else {
          offs_y = pix_y - (240 - 32);
          floor  = 1;
        }

        if (offs_y >= (32 + 3) && offs_y < 200) {
          if (pix_x == 0) {
            // read result from previous
            cur_inv_y = inv_y;
            if (cur_inv_y_bitsA._3_7 <= 70) { //cur_inv_y[3,7]
              lum = 70 - cur_inv_y_bitsA._3_7; //cur_inv_y[3,7]
              if (lum > 63) {
                lum = 63;
              }
            }
            else
              lum = 0;

            // divide for next line
            div(maxv, offs_y); //div <- (maxv,offs_y);
          }

          u = (pos_u + ((pix_x - 320) * cur_inv_y)) >> 8; //u = pos_u + ((pix_x - 320)>>1); //this to test without multipliers
          v = pos_v+ cur_inv_y_bitsB._0_6; //cur_inv_y[0,6]

          if ((u ^ v) & (1<<5)) { //u[5,1] ^ v[5,1]
            if ((u ^ v) & (1<<4)) { //u[4,1] ^ v[4,1]
              pix_r = lum << 2;
              pix_g = lum << 2;
              pix_b = lum << 2;
            } else {
              pix_r = lum << 1; //lum[1,6]
              pix_g = lum << 1 ; //lum[1,6]
              pix_b = lum << 1; //lum[1,6]
            }
          } else {
            if ((u ^ v) & (1<<4)) { //u[4,1] ^ v[4,1]
              if (floor)
                pix_r = lum << 2; //pix_g = lum;
              else
                pix_b = lum << 2;
            } else {
              if (floor)
                pix_g = lum << 1; //lum[1,6]
              else
                pix_b = lum << 1; //lum[1,6]
            }
          }
        }
      }
    }
    // prepare next
    pos_u = pos_u + 1024;
    pos_v = pos_v + 1;

    // wait for sync
    //wait(pix_vblank == 0); //alternate syntax
    while (pix_vblank == 1)
        wait_clk();

  }
#endif  
}

// -------------------------

