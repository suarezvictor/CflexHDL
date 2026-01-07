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
    void process(short *data_in, short *data_out)
    {

        short working_buf[64];
        short *temp_buf = working_buf;

        // X - Rows
        for(int i=0;i<8;i++)
        {
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
			  temp_buf[7],
			  0
			  );
			
            // Next row
            data_in += 8;
            temp_buf += 8;
        }

        // Y - Columns
        temp_buf = working_buf;
        for(int i=0;i<8;i++)
        {
			idct_kernel(
			  temp_buf[0],
			  temp_buf[8],
			  temp_buf[16],
			  temp_buf[24],
			  temp_buf[32],
			  temp_buf[40],
			  temp_buf[48],
			  temp_buf[56],
			  data_out[0],
			  data_out[8],
			  data_out[16],
			  data_out[24],
			  data_out[32],
			  data_out[40],
			  data_out[48],
			  data_out[56],
			  1
			  );
            
            temp_buf++;
            data_out++;
        }
        data_out -= 8;
    }

};

#endif
