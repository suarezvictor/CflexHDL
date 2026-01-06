// SPDX-License-Identifier: Apache-2.0
//(C) 2025 Victor Suarez Rovere <suarezvictor@gmail.com>
//(C) 2020 Ultraembedded https://github.com/ultraembedded/core_jpeg

#include "cflexhdl.h"
#define int int32
#define short int16

void _idct_kernel(
	const int& data_in_0,
	const int& data_in_1,
	const int& data_in_2,
	const int& data_in_3,
	const int& data_in_4,
	const int& data_in_5,
	const int& data_in_6,
	const int& data_in_7,
	int& data_out_0,
	int& data_out_1,
	int& data_out_2,
	int& data_out_3,
	int& data_out_4,
	int& data_out_5,
	int& data_out_6,
	int& data_out_7
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

    pipeline_stage();

	int s0b = data_in_4 * C4; //same as s1b
	int s3b = data_in_6 * C6;
	int s2b = data_in_6 * C2;
	int s7b = data_in_7 * C7;
	int s4b = data_in_7 * C1;
	int s6b = data_in_3 * C3;
	int s5b = data_in_3 * C5;

    pipeline_stage();

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

    pipeline_stage();

	int r5 = ((t6 - t5) * 181) >> 8; // 1/sqrt(2)
	int r6 = ((t5 + t6) * 181) >> 8; // 1/sqrt(2)

    pipeline_stage();

	data_out_0 = (t0 + t7) >> 11;
	data_out_1 = (t1 + r6) >> 11;
	data_out_2 = (t2 + r5) >> 11;
	data_out_3 = (t3 + t4) >> 11;
	data_out_4 = (t3 - t4) >> 11;
	data_out_5 = (t2 - r5) >> 11;
	data_out_6 = (t1 - r6) >> 11;
	data_out_7 = (t0 - t7) >> 11;
}
