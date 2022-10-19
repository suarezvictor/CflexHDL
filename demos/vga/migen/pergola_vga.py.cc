// Machine-generated using Migen2CflexHDL - do not edit!
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
	uint1 sys_clk = 0;
	uint1 sys_rst = 0;
uint11 frame = 0;
uint1 vsync_r = 0;
int16 X;
int16 Y;
int16 T;
int16 XX;
int16 YY;
int16 TT;
uint11 slice_proxy;
uint24 complexslicelowerer_slice_proxy0;
int32 complexslicelowerer_slice_proxy1;
int27 f_slice_proxy0;
int31 f_slice_proxy1;
int31 f_slice_proxy2;
int32 f_slice_proxy3;
int27 f_slice_proxy4;
int31 f_slice_proxy5;
int31 f_slice_proxy6;
int32 f_slice_proxy7;

while(always()) {
	 XX = pix_x;
	 YY = pix_y;
	 TT = bitslice(9, 0, slice_proxy);
	 X = (XX - /*signed*/320);
	 Y = (YY - /*signed*/240);
	 T = (TT - /*signed*/512);
	 slice_proxy = (bitslice(10, 10, frame) ? (~frame) : frame);
	 complexslicelowerer_slice_proxy1 = ((X * X) + (Y * Y));
	 complexslicelowerer_slice_proxy0 = bitslice(31, 8, complexslicelowerer_slice_proxy1);
	 f_slice_proxy1 = (Y * T);
	 f_slice_proxy2 = (X * T);
	 f_slice_proxy0 = ((X + bitslice(30, 6, f_slice_proxy1)) & (bitslice(30, 6, f_slice_proxy2) - Y));
	 f_slice_proxy3 = ((X * X) + (Y * Y));
	 f_slice_proxy5 = (Y * T);
	 f_slice_proxy6 = (X * T);
	 f_slice_proxy4 = ((X + bitslice(30, 6, f_slice_proxy5)) & (bitslice(30, 6, f_slice_proxy6) - Y));
	 f_slice_proxy7 = ((X * X) + (Y * Y));

	if (((!vsync_r) & pix_vblank)) {
		frame = (frame + 2);
	}
	vsync_r = pix_vblank;
	if (bitslice(23, 8, complexslicelowerer_slice_proxy0)) {
		pix_r = 0;
		pix_g = 0;
		pix_b = 0;
	} else {
		pix_r = ((bitslice(8, 3, f_slice_proxy0) == /*signed*/0) ? (bitslice(31, 8, f_slice_proxy3) ^ /*signed*/255) : /*signed*/0);
		pix_g = ((bitslice(8, 3, f_slice_proxy4) == /*signed*/0) ? 128 : 0);
		pix_b = (bitslice(31, 8, f_slice_proxy7) ^ /*signed*/255);
	}
	if (sys_rst) {
		pix_r = 0;
		pix_g = 0;
		pix_b = 0;
		frame = 0;
		vsync_r = 0;
	}
}

}


