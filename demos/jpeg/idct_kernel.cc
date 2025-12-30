// SPDX-License-Identifier: Apache-2.0
//(C) 2025 Victor Suarez Rovere <suarezvictor@gmail.com>
//(C) 2020 Ultraembedded https://github.com/ultraembedded/core_jpeg

#include "silice_compat.h"
#define int int32
#define short int16

void idct_kernel(
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
    short C1 = 4017; // cos( pi/16)*4096
    short C2 = 3784; // cos(2pi/16)*4096
    short C3 = 3406; // cos(3pi/16)*4096
    short C4 = 2896; // cos(4pi/16)*4096
    short C5 = 2276; // cos(5pi/16)*4096
    short C6 = 1567; // cos(6pi/16)*4096
    short C7 = 799;  // cos(7pi/16)*4096

	int s0 = (data_in_0 + data_in_4)       * C4;
	int s1 = (data_in_0 - data_in_4)       * C4;
	int s3 = (data_in_2 * C2) + (data_in_6 * C6);
	int s2 = (data_in_2 * C6) - (data_in_6 * C2);
	int s7 = (data_in_1 * C1) + (data_in_7 * C7);
	int s4 = (data_in_1 * C7) - (data_in_7 * C1);
	int s6 = (data_in_5 * C5) + (data_in_3 * C3);
	int s5 = (data_in_5 * C3) - (data_in_3 * C5);

	int t0 = s0 + s3;
	int t3 = s0 - s3;
	int t1 = s1 + s2;
	int t2 = s1 - s2;
	int t4 = s4 + s5;
	int t5 = s4 - s5;
	int t7 = s7 + s6;
	int t6 = s7 - s6;

	int r5 = ((t6 - t5) * 181) >> 8; // 1/sqrt(2)
	int r6 = ((t5 + t6) * 181) >> 8; // 1/sqrt(2)

	data_out_0 = (t0 + t7) >> 11;
	data_out_1 = (t1 + r6) >> 11;
	data_out_2 = (t2 + r5) >> 11;
	data_out_3 = (t3 + t4) >> 11;
	data_out_4 = (t3 - t4) >> 11;
	data_out_5 = (t2 - r5) >> 11;
	data_out_6 = (t1 - r6) >> 11;
	data_out_7 = (t0 - t7) >> 11;
}
