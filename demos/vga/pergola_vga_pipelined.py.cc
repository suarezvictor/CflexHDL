// Machine-generated using Migen2CflexHDL - do not edit!
// check source license to know the license of this file

/*
Copyright (C) 2020 Konrad Beckmann
Copyright (C) 2023 Victor Suarez Rovere <suarezvictor@gmail.com>

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Other authors retain ownership of their contributions. If a submission can reasonably be considered independently copyrightable, it's yours and we encourage you to claim it with appropriate copyright notices. This submission then falls under the "otherwise noted" category. All submissions are strongly encouraged to use the two-clause BSD license reproduced above.
*/
//original source from https://github.com/kbeckmann/pergola_projects/blob/master/pergola/gateware/vga_testimage.py

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
int32 f_slice_proxy4;
int31 rhs_slice_proxy0;
int31 rhs_slice_proxy1;

while(always()) {
	 pipeline_stage();
	 int31 YtimesY = Y*Y;
     pipeline_stage(); //this avoid issues with yosys toolchain
	 XX = pix_x;
	 YY = pix_y;
	 TT = bitslice(9, 0, slice_proxy);
	 X = (XX - /*signed*/320);
	 Y = (YY - /*signed*/240);
	 T = (TT - /*signed*/512);
	 slice_proxy = (bitslice(10, 10, frame) ? (~frame) : frame);
	 complexslicelowerer_slice_proxy1 = ((X * X) + (Y * Y));
	 complexslicelowerer_slice_proxy0 = (bitslice(31, 8, complexslicelowerer_slice_proxy1) ^ /*signed*/255);
	 f_slice_proxy1 = (Y * T);
	 f_slice_proxy2 = (X * T);
	 f_slice_proxy0 = ((bitslice(30, 6, f_slice_proxy1) + X) & (bitslice(30, 6, f_slice_proxy2) - Y));
	 f_slice_proxy3 = ((X * X) + YtimesY);
	 f_slice_proxy4 = ((X * X) + YtimesY);
	 rhs_slice_proxy0 = (Y * T);
	 rhs_slice_proxy1 = (X * T);

	if (((!vsync_r) & pix_vblank)) {
		frame = (frame + 2);
	}
	vsync_r = pix_vblank;
	if (bitslice(23, 8, complexslicelowerer_slice_proxy0)) {
		pix_r = 0;
		pix_g = 0;
	} else {
		pix_g = ((bitslice(8, 2, f_slice_proxy0) != /*signed*/0) ? (bitslice(31, 8, f_slice_proxy3) ^ /*signed*/255) : /*signed*/0);
		pix_b = (bitslice(31, 8, f_slice_proxy4) ^ /*signed*/255);
	}
	pix_r = ((bitslice(30, 9, rhs_slice_proxy0) - X) ^ (bitslice(30, 9, rhs_slice_proxy1) + Y));
	if (sys_rst) {
		pix_r = 0;
		pix_g = 0;
		pix_b = 0;
		frame = 0;
		vsync_r = 0;
	}
}

}


#include "vga_config.h"
