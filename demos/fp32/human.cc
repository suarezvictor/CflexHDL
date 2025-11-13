// Human Shader, float version
// Copyright (C) 2023-2024 Inigo Quilez
// Copyright (C) 2025 Victor Suarez Rovere
// SPDX-License-Identifier: MIT
// adapted from: https://www.shadertoy.com/view/4ft3Wn

#include "../src/fp32.cc"

//FIXME do not use macros for this
#ifndef min
#define min(a, b) ( a < b ? a : b)
#define max(a, b) ( a > b ? a : b)
#endif

//tweak as required to fit FPGA device (just the sphere for the ECP5 25F)
#define INCLUDE_SPHERE
#define INCLUDE_SKYLIGHT
#ifndef CFLEX_PARSER
#define INCLUDE_SKYSHADOW
#define INCLUDE_SUNSHADOW
#define INCLUDE_BOUNCELIGHT
#endif

#define _shader _mandel //FIXME: this is only for compatibility with mandelbrot project
MODULE _shader( const int32& ua, const int32& ub, uint32& result)
{
	float xk = /*2.f/640.f*/40.f/320.f;
	float yk = /*2.f/480.f*/40.f/320.f;
    float x = xk*ua;
    float y = yk*ub;
    
    int32 R, G, B;

    //-----------
    // Section A 
    //-----------
    float u = x - 8.f;
    float v = -4.f - y;
    float z = v;
    float u2 = u*u;
    float v2 = z*z;
    float h = u2 + v2;

    int32 v2int = v+v;
	if(v2int < 0)
    {
        //-------------------
        // Section C, Ground 
        //-------------------
        R = 150 + v2int;
        B = 50;

#ifdef INCLUDE_SKYSHADOW
        int16 p = h + v2*8;
        int16 cp = v * -240;
        int16 c = cp - p;

        // sky light / ambient occlusion
        if(c > 1200)
        {
            int16 o0 = (25*c)>>3;
            int16 o = (c*(7840-o0)>>9) - 8560;
            R = (R*o)>>10;
            B = (B*o)>>10;
        }
#endif
#ifdef INCLUDE_SUNSHADOW
        // sun/key light with soft shadow
        float w = 50.f + v*4.f;
        float r = u - w;
        float wx = w + 24.f;
        int16 d = r*r + u*wx;
        if(d > 90)
        	R = R + d-90;
#endif
    }
    else
    {
        //----------------
        // Section D, Sky 
        //----------------
        int32 c = x + y + y;
        R = 160 + c;
        B = 250 + c;
    }
    
    R = max(0, min(R, 255));
    B = max(0, min(B, 255));

#ifdef INCLUDE_SPHERE
    if( h < 200.f) 
    {
        //-------------------
        // Section B, Sphere 
        //-------------------
        int16 R2 = 420; //ambient light
        int16 B2 = 520;

        float t = 40.625f + h*0.0625f;
        float p = t*u;
        float q = t*z;
#ifdef INCLUDE_BOUNCELIGHT
        // bounce light
        int32 qf = q*208;
        int32 pk = p*80;
        int32 pq = ((208+80)<<9) + pk - qf;
        int16 w = pq>>8;
       	int16 wsqint = pq < 0 ? 0 : (w*w)>>10;
#else
		int16 wsqint = 0;
#endif
#ifdef INCLUDE_SKYLIGHT
        // sky light / ambient occlusion
        int16 pint = p;
        int16 qint = q;
        int16 o = qint + 900;
        int16 pqint = pint + qint;
#else
        int16 pqint = 0;
        int16 o = 512;
#endif
        uint32 R2o = (R2 + wsqint)*o;
        uint32 B2o = B2*o; 
        R2 = R2o>>12;
        B2 = B2o>>12;

        // sun/key light
        int16 R3;
        int16 B3;

        if(pqint >= 0)
        {
            R3 = min(R2 + (pqint>>3), 255);
            B3 = min(B2 + (pqint>>3), 255);
        }
        else
        {
        	R3 = min(R2, 255);
        	B3 = min(B2, 255);
        }

        if(h < 196.f)
        {
          R = R3;
          B = B3;
        }
        else
        {
          //antialias
          uint16 hint = h*64;
          uint9 m = 200*64 - hint;
          R = R+((R3-R)*m>>8);
          B = B+((B3-B)*m>>8);
        }
	}
#endif
    //-------------------------  

    G = (R+B)>>1;
    result = (B<<16) + (G<<8) + R;
}

