/*
Copyright (c) 2019 Sylvain Lefebvre and contributors
Copyright (c) 2022 Victor Suarez Rovere <suarezvictor@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

List of other contributors at Silice's repo https://github.com/sylefeb/Silice:
 (i.e. git shortlog -n -s -- <filename>)
*/

// Implements a pipeline ray-marching a fractal while racing the beam.
// The marching algorithm is the classical "Fast voxel traversal"
// by Amanatides and Woo 1987 (a dda traversal, ala Wolfenstein3D)
// http://www.cse.yorku.ca/~amana/research/grid.pdf
//
// The fractal is a Menger sponge https://en.wikipedia.org/wiki/Menger_sponge
// with three levels. It is produced by looking up a 64 bits constant defining
// which of the 4x4x4 (=64) voxels are solid.

/*
$$if ECPIX5 then
$$  N_steps       = 128
$$  delay         = 45751
$$  VGA_1024_768  = 1
$$elseif DE10NANO then
$$  N_steps       = 190
$$  delay         = 93065
$$  VGA_1920_1080 = 1
$$elseif VERILATOR then
$$  N_steps       = 64
$$  delay         = 93191
$$  VGA_1920_1080 = 1
$$else
$$  error('sorry, this design is currently only for the ECPIX5 and de10-nano')
$$end

$include('vga_demo_main.si')
*/
#include "cflexhdl.h"
#include "vga_config.h"
#ifndef M_PI
#define M_PI 3.14159265
#ifdef CFLEX_SIMULATION
extern "C" double cos(double);
extern "C" double sin(double);
//#define DITHER
#endif
#endif

//Silice compatibility macros
//#define dualport_bram
//#define bram
#define __display(fmt, ...) //printf(fmt "\n", __VA_ARGS__)
#define __signed(x) int(x)
typedef signed char int2;
typedef int int24;
typedef short int11, int12;
typedef unsigned char uint6;
typedef unsigned short uint12, uint14;
typedef unsigned uint18, uint20, uint32;
typedef unsigned long long uint64;

#define N_steps 190

// ----------------------------------------------------------------------------
// Below we define circuitries for our multipliers.
//
// Note that circuitries containing a pipeline are automatically concatenated
// to the parent pipeline, thus adding stages as expected.
/*
// ----------------------------------------------------------------------------
$$if ECPIX5 then
// ----------------------------------------------------------------------------
// NOTE: on the ECP5 the pipelined multipliers do not result in better fmax
//       which seems suprising, still investigating!
circuitry mul_14_18 (output result,input a,input b) { result = a*b; }
circuitry mul_8_8   (output result,input a,input b) { result = a*b; }
circuitry imul_24_24(output result,input a,input b) { result = a*b; }
// ----------------------------------------------------------------------------
$$else
// ----------------------------------------------------------------------------
// NOTE: On the de10nano, we use pipelined multipliers to improve fmax.
// TODO: Generic version of these, see tests/circuits13.si
circuitry mul_14_18(output result,input a,input b)
{
  uint16 ahbl(0); uint16 albh(0); uint16 albl(0); uint16 ahbh(0);
  ahbl   = a[7,7] * b[0,9];  albh   = a[0,7] * b[9,9];
  albl   = a[0,7] * b[0,9];  ahbh   = a[7,7] * b[9,9];
 ->
  result = {ahbh,16b0}+{ahbl,7b0}+{albh,9b0}+{16b0,albl};
 ->
}

circuitry mul_8_8(output result,input a,input b)
{
  uint8 albh(0); uint8 ahbl(0); uint8 albl(0); uint8 ahbh(0);
  ahbl   = a[4,4] * b[0,4];  albh   = a[0,4] * b[4,4];
  albl   = a[0,4] * b[0,4];  ahbh   = a[4,4] * b[4,4];
 ->
  result = { {ahbh,4b0} + ahbl+albh+albl[4,4] , albl[0,4] };
 ->
}

circuitry mul_12_12(output result,input a,input b)
{
  uint12 albh(0); uint12 ahbl(0); uint12 albl(0); uint12 ahbh(0);
  ahbl   = a[6,6] * b[0,6];  albh   = a[0,6] * b[6,6];
  albl   = a[0,6] * b[0,6];  ahbh   = a[6,6] * b[6,6];
 ->
  result = { {ahbh,6b0} + ahbl+albh+albl[6,6] , albl[0,6] };
 ->
}

circuitry imul_24_24(output result,input a,input b)
{
  uint11 albh(0);  uint11 ahbl(0);  uint23 albl(0);  uint23 aa(0);
  //  ^^ result is clamped at 24, so we can clamp albh ahbl
  uint23 bb(0);    uint1  na(0);    uint1  nb(0);    uint23 m(0);
  na = a < 0; nb = b < 0; aa = na ? -a : a; bb = nb ? -b : b;
 ->
  uint12 ah = {1b0,aa[12,11]};  uint12 al = aa[0,12];
  uint12 bh = {1b0,bb[12,11]};  uint12 bl = bb[0,12];
(ahbl) = mul_12_12(ah,bl); (albh) = mul_12_12(al,bh); (albl) = mul_12_12(al,bl);
  m = {ahbl+albh+albl[12,11],albl[0,12]};
 ->
  result = (na ^ nb) ? -m : m;
 ->
}
$$end
*/

//#define SIMULATION
#define H_RES FRAME_WIDTH
#define V_RES FRAME_HEIGHT


#define imul_24_24(a, b) ((a)*(b))
#define mul_12_12(a, b) ((a)*(b))
#define mul_8_8(a, b) ((a)*(b))
#define mul_14_18(a, b) ((a)*(b))

// ----------------------------------------------------------------------------
// display unit
// ----------------------------------------------------------------------------
/*
unit frame_display(
  input   uint11 pix_x,       input   uint11 pix_y,
  input   uint1  pix_active,  input   uint1  pix_vblank,
  input   uint1  vga_hs,      input   uint1  vga_vs,
  output! uint$color_depth$ pix_r,
  output! uint$color_depth$ pix_g,
  output! uint$color_depth$ pix_b
*/
MODULE frame_display(
  const   uint11& pix_x,       const   uint11& pix_y,
  const   uint1&  pix_active,  const   uint1&  vga_vs,
  uint8& pix_r,
  uint8& pix_g,
  uint8& pix_b
) {

  int24   frame(200);

  // --- Below we define a number of BRAMS used by the pipeline stages
  // A given BRAM can only be used by a single stage (all stages access every
  // cycle), but we can share a single BRAM with two stages using a dual port
  // BRAM. Since we have to duplicate anyway, we also make different tables
  // for cosine and sine (instead of reusing a same table for both).
  // --- precomputed cosine table (in BRAM)
/*
  dualport_bram int24 cos[512] = {
$$for i=0,511 do
    $math.floor(1024.0 * math.cos(2*math.pi*i/512))$,
$$end
  };
  // --- precomputed sine table (in BRAM)
  dualport_bram int24 sin[512] = {
$$for i=0,511 do
    $math.floor(1024.0 * math.sin(2*math.pi*i/512))$,
$$end
  };
  // --- precomputed table division (1/x, x in [0,2047], 1/0 returns max)
  // we need access to inv from 3 stages: we create one dual and one simple BRAM
  dualport_bram uint18 invA[2048] = { // dual
    131071,131071,131071,
$$for i=3,2047 do
    $262144//i$,
$$end
  };
  bram          uint18 invB[2048] = { // simple
    131071,131071,131071,
$$for i=3,2047 do
    $262144//i$,
$$end
  };
*/
  int24 cos_table[512];
  int24 sin_table[512];
  int11 invA_addr0;
  int11 invA_addr1;
  int11 invB_addr;
  uint9 cos_addr0;
  uint9 cos_addr1;
  uint9 sin_addr0;
  uint9 sin_addr1;
  uint18 invA[2048]={131071,131071,131071};
  uint18 invB[2048]={131071,131071,131071};
#ifdef CFLEX_SIMULATION
  for(int i=0;i<512;++i)
  {
	  cos_table[i] = 1024*cos(2*M_PI*i/512);
	  sin_table[i] = 1024*sin(2*M_PI*i/512);
  }
  for(int i=3;i<2048;++i)
  {
	  invA[i]=262144/i;
	  invB[i]=262144/i;
  }
#endif
/*
  // --- this 64 bits vector represents 8x8x8 voxels, 1 bit per voxel
  uint64 tile <:: { // do you see the fractal shape? it's in there!
         1b1,1b1,1b1,1b1,
         1b1,1b0,1b0,1b1,
         1b1,1b0,1b0,1b1,
         1b1,1b1,1b1,1b1,

         1b1,1b0,1b0,1b1,
         1b0,1b0,1b0,1b0,
         1b0,1b0,1b0,1b0,
         1b1,1b0,1b0,1b1,

         1b1,1b0,1b0,1b1,
         1b0,1b0,1b0,1b0,
         1b0,1b0,1b0,1b0,
         1b1,1b0,1b0,1b1,

         1b1,1b1,1b1,1b1,
         1b1,1b0,1b0,1b1,
         1b1,1b0,1b0,1b1,
         1b1,1b1,1b1,1b1
  };
*/
  uint64 tile = 0b1111100110011111100100000000100110010000000010011111100110011111ull;

  //_ --- 4x4 matrix for dithering down to 4bpp on some hardware
  // https://en.wikipedia.org/wiki/Ordered_dithering
  uint4 bayer_4x4[16] = {
     0,  8,  2, 10,
    12,  4, 14,  6,
     3, 11,  1,  9,
    15,  7, 13,  5
  };

/*
  // --- always_before block, performed every cycle before anything else
  always_before {
    pix_r = 0; pix_g = 0; pix_b = 0; // maintain RGB at zero, important during
  }                                  // vga blanking for screen to adjust

  // --- algorithm containing the pipeline
  algorithm <autorun> {
*/
    while (1) { // forever
      pix_r = 0; pix_g = 0; pix_b = 0; // maintain RGB at zero, important during
                                       // vga blanking for screen to adjust
      // a 'voxel' is 1<<12 (constant, so tracker can be here)
      // lvl 0 is 1<<14 (4x4x4), lvl 1 is 1<<16 (4x4x4), lvl 2 is 1<<18 (4x4x4)
      uint14 vxsz = 1<<12;

      // ===== Here we synch the pipeline with the vertical sync.
      //       The pipeline starts during vblank so latency is hidden and
      //       the first pixel is ready exactly at the right moment.

      while(!vga_vs) { wait_clk(); }
      while( vga_vs) { wait_clk(); }
/*

      // Wait the 'perfect' delay (obtained in simulation, see marker [1] below)
      // (adjust delay if number of steps is changed).
      uint17 wait = 0; while (wait != DELAY) { wait = wait + 1; }
*/
      // ----- start the pipeline! -----
      // This loop feeds pixel coordinates to the pipeline, the pipeline outputs
      // pixels directly into the VGA module in the last stage. The delay above
      // (while (wait ...)) is just right so that the first pixel exits the
      // pipeline zhen it is needed.
      // Note that the pipeline computes value for entire VGA rows including
      // during h-sync, but these pixels in h-sync are discarded (I found it
      // simpler to do that, and it uses slightly less logic).
/*
      uint12 x = -1; uint12 y = -1;
      while ( ! (x == H_RES-1 && y == V_RES-1))) {
*/
      while(!vga_vs) {
        wait_clk();
        uint12 x = pix_x; uint12 y = pix_y;
        // ----- pipeline starts here -----

        int24 view_x = ((__signed(x) - __signed(H_RES>>1)));
        int24 view_y = ((__signed(y) - __signed(V_RES>>1)));
        int24 view_z = 384;

        // lookup cosine/sine for rotations (4x lookups)
        cos_addr0 = frame>>1;              sin_addr0 = frame>>1;
        cos_addr1 = (frame+(frame<<1))>>3; sin_addr1 = (frame+(frame<<3))>>4;
        // per-pixel state vars updated through the pipeline
        uint1 inside = 0; uint8 dist = 239; uint8 clr = 0;
/*
        // increment pixel coordinates
        y = x == H_END-1 ? (y + 1) : y;
        x = x == H_END-1 ? 0 : (x + 1);
*/
    //->  // --- next pipeline stage

        int24 cs0 = cos_table[cos_addr0 & 0x1FF];   int24 ss0 = sin_table[sin_addr0 & 0x1FF];
        int24 cs1 = cos_table[cos_addr1 & 0x1FF];   int24 ss1 = sin_table[sin_addr1 & 0x1FF];
        int24 rot_x(0);           int24 rot_y(0);

    rot_x = imul_24_24(view_x,cs1); // these circuits may be pipelines
    rot_y = imul_24_24(view_x,ss1); // this is why I indent aligned with ->


        int24 ycs(0); int24 yss(0);

    yss = imul_24_24(view_y,ss1);
    ycs = imul_24_24(view_y,cs1);

        view_x  = rot_x - yss;
        view_y  = rot_y + ycs;

    //->  // --- spliting in stages relaxes fmax

        view_x = view_x >> 10;
        view_y = view_y >> 10;

    //->

        // compute the ray direction (through rotations)
        int24 xcs(0);
    xcs = imul_24_24(view_x,cs0);
        int24 xss(0);
    xss = imul_24_24(view_x,ss0);
        int24 zcs(0);
    zcs = imul_24_24(view_z,cs0);
        int24 zss(0);
    zss = imul_24_24(view_z,ss0);

        int24 r_x_delta = (xcs - zss);
        int24 r_z_delta = (xss + zcs);

    //->

        // ray dir is (r_x_delta, view_y, r_z_delta)
        int16 rd_x = r_x_delta>>10;
        int16 rd_y = view_y;
        int16 rd_z = r_z_delta>>10;
        // initialize voxel traversal
        // -> steps
        int2 s_x = rd_x<0?-1:1; int2 s_y = rd_y<0?-1:1; int2 s_z = rd_z<0?-1:1;
        // -> lookup inverses (1/x)
        invA_addr0 = rd_x<0 ? -rd_x : rd_x; invA_addr1 = rd_y<0 ? -rd_y : rd_y;
        invB_addr  = rd_z<0 ? -rd_z : rd_z;
        // -> position
        int24 p_x  = (68<<11);
        int24 p_y  = (12<<11);
        int24 p_z  = (frame<<9);
        // -> start voxel
        int12 v_x = p_x >> 12; int12 v_y = p_y >> 12; int12 v_z = p_z >> 12;
        // distance to border
        uint14 brd_x = (p_x - (v_x<<12));
        uint14 brd_y = (p_y - (v_y<<12));
        uint14 brd_z = (p_z - (v_z<<12));

    //->

        brd_x = (rd_x < 0 ? (brd_x) : (vxsz - brd_x))&0x3FFF;
        brd_y = (rd_y < 0 ? (brd_y) : (vxsz - brd_y))&0x3FFF;
        brd_z = (rd_z < 0 ? (brd_z) : (vxsz - brd_z))&0x3FFF;

    //->

        // inv dot products
        uint18 inv_x = invA[invA_addr0 & 0x7FF];
        uint18 inv_y = invA[invA_addr1 & 0x7FF];
        uint18 inv_z = invA[invB_addr  & 0x7FF];

        // -> tmax

      uint32 tm_x_(0);  uint32 tm_y_(0);  uint32 tm_z_(0);

    tm_x_ = mul_14_18(brd_x, inv_x);
    tm_y_ = mul_14_18(brd_y, inv_y);
    tm_z_ = mul_14_18(brd_z, inv_z);

        uint20 tm_x = tm_x_>>12;
        uint20 tm_y = tm_y_>>12;
        uint20 tm_z = tm_z_>>12;

        // -> delta

      uint32 dt_x_(0);  uint32 dt_y_(0);  uint32 dt_z_(0);

    dt_x_ = mul_14_18(vxsz, inv_x);
    dt_y_ = mul_14_18(vxsz, inv_y);
    dt_z_ = mul_14_18(vxsz, inv_z);

        uint20 dt_x = (dt_x_>>12)-1; // keep at 20 bits
        uint20 dt_y = (dt_y_>>12)-1;
        uint20 dt_z = (dt_z_>>12)-1;

    // ----- now we generate the marching stages -----

//$$for i=0,N_steps-1 do
	for(int i=0; i < N_steps; ++i)
	{
    //->  // --- each marching stage is almost identical, see only diff below (D)
        uint6 tex   = ((v_x) ^ (v_y) ^ (v_z))&0x3F;
		#define _concat_2(z, y, x) ((((z)&3)<<4) | (((y)&3)<<2) | ((x)&3))
        uint6 vnum0 = _concat_2(v_z>>0, v_y>>0, v_x>>0); //{v_z[0,2],v_y[0,2],v_x[0,2]};
        uint6 vnum1 = _concat_2(v_z>>2, v_y>>2, v_x>>2); //{v_z[2,2],v_y[2,2],v_x[2,2]};
        uint6 vnum2 = _concat_2(v_z>>4, v_y>>4, v_x>>4); //{v_z[4,2],v_y[4,2],v_x[4,2]};
        #undef _concat_2
        if (!inside && ((tile>>vnum0)&1) & ((tile>>vnum1)&1) & ((tile>>vnum2)&1)) {
          clr = tex;  dist = (i * 239) / N_steps;
          inside = 1;
          //  (D)            ^^^^^^^^^^ each stage has its own value of dist
          //  this is used for a depth effect (it is not the correct distance!)
          break;
        }
        // select smallest
        int21 cmp_yx = tm_y - tm_x;
        int21 cmp_zx = tm_z - tm_x;
        int21 cmp_zy = tm_z - tm_y;
        uint1 x_sel  = !(cmp_yx<0) && !(cmp_zx<0); // uses sign bit for
        uint1 y_sel  =  (cmp_yx<0) && !(cmp_zy<0); // comparisons, < and >=
        uint1 z_sel  =  (cmp_zx<0) &&  (cmp_zy<0);
        if (x_sel) { // tm_x smallest
          v_x  = v_x  + s_x;  tm_x = tm_x + dt_x;
        }
        if (y_sel) { // tm_y smallest
          v_y  = v_y  + s_y;  tm_y = tm_y + dt_y;
        }
        if (z_sel) { // tm_z smallest
          v_z  = v_z  + s_z;  tm_z = tm_z + dt_z;
        }
//$$end
    }
    //->
        // compute color from hit distance
        uint8  fog   = dist;
        uint8  light = 239 - fog;
        uint16 shade(0);
    shade = mul_8_8(light,clr);
        uint8  clr_r = ((shade>>7)&0xFF) + fog;
        uint8  clr_g = ((shade>>7)&0xFF) + fog;
        uint8  clr_b = ((shade>>8)&0xFF) + fog;

    //->
#ifdef SIMULATION
        // to verify/adjust pixel synch [1]
        if (pix_y == 0) {
          __display("x = %d  pix_x = %d (diff: %d)",x,pix_x,__signed(x-pix_x));
          __display("y = %d  pix_y = %d (diff: %d)",y,pix_y,__signed(y-pix_y));
        }
#endif
        if (x < H_RES) { // do not produce color out of bound, screen may
                           // otherwise produce weird color artifacts
#ifndef DITHER
          pix_r = clr_r; pix_g = clr_g; pix_b = clr_b;
          //           ^^^ framework uses 8 bits per component for RGB
#else
          uint4 dth_r = bayer_4x4[((y&3)<<2) | (x&3)] < (clr_r&15) ? (clr_r>>4)+1 : clr_r>>4;
          uint4 dth_g = bayer_4x4[((x&3)<<2) | (y&3)] < (clr_g&15) ? (clr_g>>4)+1 : clr_g>>4;
          uint4 dth_b = bayer_4x4[((y&3)<<2) | (x&3)] < (clr_b&15) ? (clr_b>>4)+1 : clr_b>>4;
          pix_r       = dth_r<<4; pix_g = dth_g<<4; pix_b = dth_b<<4;
#endif
        }

        // ----- pipeline ends here -----

      } // while x,y

      frame = (frame - 1);

    //} // while (1)

  }
}
