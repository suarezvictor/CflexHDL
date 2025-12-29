// Simple mandelbrot demo
// (C) 2025 Victor Suarez Rovere <suarezvictor@gmail.com>

#ifndef DISABLE_HARDWARE_ACCEL
#include "cflexhdl.h"
#endif

#include "../../src/fp32.cc"

MODULE _shader(float ua, float ub, const uint32& frame, uint32& result)
{

#if 1
	//this makes cosimulation pass
	float zr = 0.f;
	float zi = 0.f;
	float x = 2.f*ua;
	float y = 2.f*ub;

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
		sum = zrsq + zisq;

		if(sum < 4.f)
		{
			float zr2 = zr + zr;
			zr = zrsq - zisq + x;
			zi = zr2 * zi + y;
			i = i + 1;
		}
	} 
	
	//aproximates log2(sum), for colorization
	int32 f = u.color - (127<<3);
	uint16 shade = ((i+1)<<4) - f;
	result = (shade<<8) | (shade > 255 ? 0 : 0xC0);
#endif
#if 0
	//this makes cosimulation fail if denormalized numbers are not correctly handled
	float y = 1.f * ub;
	float u = 0.f + y; //checks adding zero
	float zisq = u * u;
	
	int32 r = 10000.f*zisq;
	if(r<0)
		result = 0-r;
	else
		result = r;
#endif
#if 0
	//this makes cosimulation fail if multiplication rounding is not correctly handled
	int32 f = 100000.f*0.36916f;
	result = f;
#endif
#if 0
	//this makes cosimulation fail if addition rounding is not correctly handled
	float zr = 3.975039f - 1.989062f;

	union
	{
		float m;
		struct {uint32 h:32; } l;
	};
	m = zr;

	int32 f = l.h;
	result = f;
#endif

}

