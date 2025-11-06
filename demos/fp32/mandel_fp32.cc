// Simple mandelbrot demo
// (C) 2025 Victor Suarez Rovere <suarezvictor@gmail.com>

#include "cflexhdl.h"

#include "../../src/fp32.cc"


MODULE _mandel(const int32& ua, const int32& ub, uint32& result)
{
#if 1
	//this makes cosimulation pass
	float zr = 0.f;
	float zi = 0.f;
	float x = /*4.f/FRAME_WIDTH*/0.00625f * ua;
	float y = /*4.f/FRAME_HEIGHT*/0.00833f* ub;

	uint32 i = 0;

	union {
		float sum;
		struct { uint32 frac:23; uint32 exp:8; uint32 sign:1; } s;
		struct { uint32 :20; uint32 color:12; } u;
	};

	uint8 c;
	for (c = 0; c < 16; c = c + 1)
	{
		float zrsq = zr * zr;
		float zisq = zi * zi;
		float sum = zrsq + zisq;

		if(sum < 4.f)
		{
			float zr2 = zr + zr;
			float zi_t = zr2 * zi;
			zr = zrsq - zisq + x;
			zi = zi_t + y;
			i = i + 1;
		}
	} 
	
	//aproximates log2(sum), for colorization
	int32 f = u.color - (127<<3);
	result = ((i+1)<<4) - f;
#else
	//this makes cosimulation pass
	float x = 0.00625f * ua;
	float y = 0.00833f * ub;
	float z = x + y;
	int32 i = 2.f*z;
	result = i;
#endif
}

