// SPDX-License-Identifier: Apache-2.0
//(C) 2025-2026 Victor Suarez Rovere <suarezvictor@gmail.com>
//(C) 2020 Ultraembedded https://github.com/ultraembedded/core_jpeg

#include "cflexhdl.h"
#define int int32
#define short int16

//select IDCT algorithm
#define idct_kernel_aan _idct_kernel //for JPEGDEC
//#define idct_kernel_loeffler _idct_kernel //for Ultraembedded

#define idct_stage() pipeline_stage() //enable for pipelined implementation in hardware
//#define idct_stage()

void idct_kernel_loeffler(
	short data_in_0,
	short data_in_1,
	short data_in_2,
	short data_in_3,
	short data_in_4,
	short data_in_5,
	short data_in_6,
	short data_in_7,
	short& data_out_0,
	short& data_out_1,
	short& data_out_2,
	short& data_out_3,
	short& data_out_4,
	short& data_out_5,
	short& data_out_6,
	short& data_out_7,
	short is_y
)
{
    //constant factors: cos(N*pi/16)*2^12
    const short C1 = 4017;
    const short C2 = 3784;
    const short C3 = 3406;
    const short C4 = 2896;
    const short C5 = 2276;
    const short C6 = 1567;
    const short C7 =  799;

	int s0a = data_in_0 * C4; //same as s1a
	int s3a = data_in_2 * C2;
	int s2a = data_in_2 * C6;
	int s7a = data_in_1 * C1;
	int s4a = data_in_1 * C7;
	int s6a = data_in_5 * C5;
	int s5a = data_in_5 * C3;

    idct_stage();

	int s0b = data_in_4 * C4; //same as s1b
	int s3b = data_in_6 * C6;
	int s2b = data_in_6 * C2;
	int s7b = data_in_7 * C7;
	int s4b = data_in_7 * C1;
	int s6b = data_in_3 * C3;
	int s5b = data_in_3 * C5;

    idct_stage();

	int s0 = s0a + s0b;
	int s1 = s0a - s0b;
	int s2 = s2a - s2b;
	int s3 = s3a + s3b;
	int s4 = s4a - s4b;
	int s5 = s5a - s5b;
	int s6 = s6a + s6b;
	int s7 = s7a + s7b;

	int t0 = s0 + s3;
	int t3 = s0 - s3;
	int t1 = s1 + s2;
	int t2 = s1 - s2;
	int t4 = s4 + s5;
	int t5 = s4 - s5;
	int t7 = s7 + s6;
	int t6 = s7 - s6;

    idct_stage();

	int r5 = ((t6 - t5) * 181) >> 8; // *sqrt(.5)
	int r6 = ((t5 + t6) * 181) >> 8; // *sqrt(.5)

    idct_stage();

	int o0 = (t0 + t7) >> 11;
	int o1 = (t1 + r6) >> 11;
	int o2 = (t2 + r5) >> 11;
	int o3 = (t3 + t4) >> 11;
	int o4 = (t3 - t4) >> 11;
	int o5 = (t2 - r5) >> 11;
	int o6 = (t1 - r6) >> 11;
	int o7 = (t0 - t7) >> 11;

	data_out_0 = is_y == 0 ? o0 : o0 >> 4;
	data_out_1 = is_y == 0 ? o1 : o1 >> 4;
	data_out_2 = is_y == 0 ? o2 : o2 >> 4;
	data_out_3 = is_y == 0 ? o3 : o3 >> 4;
	data_out_4 = is_y == 0 ? o4 : o4 >> 4;
	data_out_5 = is_y == 0 ? o5 : o5 >> 4;
	data_out_6 = is_y == 0 ? o6 : o6 >> 4;
	data_out_7 = is_y == 0 ? o7 : o7 >> 4;
}

//AA&N IDCT (5 multipliers)
//based on JPEGDEC's jpeg.inl: (C) BitBank Software LICENSE Apache 2.0
void idct_kernel_aan(
	short data_in_0,
	short data_in_1,
	short data_in_2,
	short data_in_3,
	short data_in_4,
	short data_in_5,
	short data_in_6,
	short data_in_7,
	short& data_out_0,
	short& data_out_1,
	short& data_out_2,
	short& data_out_3,
	short& data_out_4,
	short& data_out_5,
	short& data_out_6,
	short& data_out_7,
	short is_y
)
{
    const short K_1_41	= 362;	// 362>>8 = 1.414213562
    const short K_1_84	= 473;	// 473>>8 = 1.8477
    const short K_2_61M	= -669;	//-669>>8 = -2.6131259
    const short K_1_08	= 277; 	// 277>>8 = 1.08239

    //TODO: use exact precision to reduce multipliers' operands width
    short tmp0,tmp1,tmp2,tmp3,tmp4,tmp5,tmp6,tmp7;
    short x0,x1,x2,x3,x4,x5,x6,x7;
    short z10,z11,z12,z13; 
    //short not enough
    int tmp10;
    int tmp11;
    int tmp12;
    int tmp13;
    int z5; 

    tmp0 = data_in_0;
    tmp4 = data_in_1;
    tmp1 = data_in_2;
    tmp5 = data_in_3;
    tmp2 = data_in_4;
    tmp6 = data_in_5;
    tmp3 = data_in_6;
    tmp7 = data_in_7;

    tmp10 = tmp0 + tmp2;
    tmp11 = tmp0 - tmp2;
    tmp13 = tmp1 + tmp3;
    tmp12 = (((tmp1 - tmp3) * K_1_41) >> 8) - tmp13;  //not wide input operands to multiplier

    idct_stage();

    x0 = tmp10 + tmp13;
    x3 = tmp10 - tmp13;
    x1 = tmp11 + tmp12;
    x2 = tmp11 - tmp12;

    z13 = tmp6 + tmp5;
    z10 = tmp6 - tmp5;
    z11 = tmp4 + tmp7;
    z12 = tmp4 - tmp7;
    x7 = z11 + z13;

    idct_stage();

    tmp11 = ((z11 - z13) * K_1_41) >> 8;
    z5 = ((z10 + z12) * K_1_84) >> 8;

    idct_stage();

    tmp12 = ((z10 * K_2_61M) >> 8) + z5;
    tmp10 = ((z12 * K_1_08) >> 8) - z5;

    idct_stage();

    x6 = tmp12 - x7;
    x5 = tmp11 - x6;
    x4 = tmp10 + x5;

    short o0 = x0 + x7;
    short o1 = x1 + x6;
    short o2 = x2 + x5;
    short o3 = x3 - x4;
    short o4 = x3 + x4;
    short o5 = x2 - x5;
    short o6 = x1 - x6;
    short o7 = x0 - x7;

	if(is_y)
	{
		data_out_0 = o0;
		data_out_1 = o1;
		data_out_2 = o2;
		data_out_3 = o3;
		data_out_4 = o4;
		data_out_5 = o5;
		data_out_6 = o6;
		data_out_7 = o7;
	}
	else
	{
		data_out_0 = o0 < -(128<<5) ? 0 : (o0 > (127<<5) ? 255 : (o0>>5)+128);
		data_out_1 = o1 < -(128<<5) ? 0 : (o1 > (127<<5) ? 255 : (o1>>5)+128);
		data_out_2 = o2 < -(128<<5) ? 0 : (o2 > (127<<5) ? 255 : (o2>>5)+128);
		data_out_3 = o3 < -(128<<5) ? 0 : (o3 > (127<<5) ? 255 : (o3>>5)+128);
		data_out_4 = o4 < -(128<<5) ? 0 : (o4 > (127<<5) ? 255 : (o4>>5)+128);
		data_out_5 = o5 < -(128<<5) ? 0 : (o5 > (127<<5) ? 255 : (o5>>5)+128);
		data_out_6 = o6 < -(128<<5) ? 0 : (o6 > (127<<5) ? 255 : (o6>>5)+128);
		data_out_7 = o7 < -(128<<5) ? 0 : (o7 > (127<<5) ? 255 : (o7>>5)+128);
	}
}
