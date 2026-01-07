// (C) 2020 Ultraembedded
// (C) 2025 Victor Suarez Rovere <suarezvictor@gmail.com>
// SPDX-License-Identifier: Apache-2.0
// https://github.com/ultraembedded/core_jpeg

#ifndef JPEG_IDCT_H
#define JPEG_IDCT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

typedef void (*idct_kernel_t)(
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
);

extern idct_kernel_t idct_kernel;

//-----------------------------------------------------------------------------
// jpeg_idct:
//-----------------------------------------------------------------------------
class jpeg_idct
{
public:
    jpeg_idct() { reset(); }
    void reset(void) { }

    //-----------------------------------------------------------------------------
    // process: Perform inverse DCT on already dequantized data.
    // [Not quite sure who to attribute this implementation to...]
    //-----------------------------------------------------------------------------
    void process(int *data_in, int *data_out)
    {
        int s0,s1,s2,s3,s4,s5,s6,s7;
        int t0,t1,t2,t3,t4,t5,t6,t7;

        int working_buf[64];
        int *temp_buf = working_buf;

        // X - Rows
        for(int i=0;i<8;i++)
        {
			//idct_kernel((int*)data_in, (int*)temp_buf);
			idct_kernel(
			  data_in[0],
			  data_in[1],
			  data_in[2],
			  data_in[3],
			  data_in[4],
			  data_in[5],
			  data_in[6],
			  data_in[7],
			  temp_buf[0],
			  temp_buf[1],
			  temp_buf[2],
			  temp_buf[3],
			  temp_buf[4],
			  temp_buf[5],
			  temp_buf[6],
			  temp_buf[7]
			  );
			
            // Next row
            data_in += 8;
            temp_buf += 8;
        }

        // Y - Columns
        temp_buf = working_buf;
        for(int i=0;i<8;i++)
        {
            int col[8] = {temp_buf[0], temp_buf[8], temp_buf[16], temp_buf[24], temp_buf[32], temp_buf[40], temp_buf[48], temp_buf[56]};
            int out[8];
			//idct_kernel(col, out);
			idct_kernel(
			  col[0],
			  col[1],
			  col[2],
			  col[3],
			  col[4],
			  col[5],
			  col[6],
			  col[7],
			  out[0],
			  out[1],
			  out[2],
			  out[3],
			  out[4],
			  out[5],
			  out[6],
			  out[7]
			  );

            data_out[0]  = out[0] >> 4;
            data_out[8]  = out[1] >> 4;
            data_out[16] = out[2] >> 4;
            data_out[24] = out[3] >> 4;
            data_out[32] = out[4] >> 4;
            data_out[40] = out[5] >> 4;
            data_out[48] = out[6] >> 4;
            data_out[56] = out[7] >> 4;
            
            temp_buf++;
            data_out++;
        }
        data_out -= 8;
    }

};

#endif
