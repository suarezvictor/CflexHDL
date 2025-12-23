// Human Shader, float version
// Copyright (C) 2023-2024 Inigo Quilez
// Copyright (C) 2025 Victor Suarez Rovere
// SPDX-License-Identifier: MIT
// adapted from: https://www.shadertoy.com/view/4ft3Wn

#ifndef DISABLE_HARDWARE_ACCEL
#include "cflexhdl.h"
#endif

#include "../../src/fp32.cc"

//FIXME do not use macros for this
#ifndef min
#define min(a, b) ( a < b ? a : b)
#define max(a, b) ( a > b ? a : b)
#endif

//tweak as required to fit FPGA device (i.e. all except SUNSHADOW for the ECP5 25F)
#define INCLUDE_FLOOR 0
#define INCLUDE_CHECKERS 1
#define INCLUDE_SPHERE 2
#define INCLUDE_SKYLIGHT 2
#define INCLUDE_ANTIALIAS 2
#define INCLUDE_AMBIENTSHADOW 3
#define INCLUDE_SUNSHADOW 4 //requires ECP 45F
#define INCLUDE_BOUNCELIGHT 5
#define SHADER_MAXFRAMES 7

#define AA_CHECKERS //enable antialiasing for checkerboard

void _triang(float x, float& r)
{
  int16 xi = 32768.f * x;
  int16 sy = xi + 16384;
  int16 abssy = sy < 0 ? -sy : sy;
  int32 y = abssy - 16384;
  r = 0.00003f * y; // 1.f/32768 = .0000305175f, but smaller value adds a bit more AA
}

void _clamp(float x, float& r)
{
  r = x;
  if(x < -1.f) r = 0.f-1.f; //FIXME: float negative constant unsupported
  if(1.f < x) r = 1.f;
}


#define _shader _mandel //FIXME: this is only for compatibility with mandelbrot project
MODULE _shader( const int32& ua, const int32& ub, const int32& frame, uint32& result)
{
    float x = 0.125f*ua;
    float y = 0.125f*ub;
    
    int16 R, G, B;

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
#ifdef INCLUDE_FLOOR
	if(frame > INCLUDE_FLOOR && v2int < 0)
    {
        //-------------------
        // Section C, Ground 
        //-------------------
#ifdef INCLUDE_CHECKERS
		if(frame > INCLUDE_CHECKERS)
		{
#ifdef AA_CHECKERS
			//see prototype at https://www.shadertoy.com/view/tfdBWl
			
			//int32 W = 640;
			//int32 H = 480;
			
            float sx = .033f*x;
			float sy = .025f*v;
			float sysq = sy * sy;

			float dt_dsy = 1.f / sysq;
			float t = dt_dsy * sy;
			float pz = -2.f * t;

			float dzw = dt_dsy * 0.00417f; // (2.f / (H-1)) ; //constant is pixel footprint in world space, increase for more AA
			
            float px = sx * t; //perspective
            float abssx = sx;
            if (sx < 0.f)
              abssx = 0.f-sx; //FIXME: CflexHDL unary operator is interpreted as pointer!!

			//FIXME: make functions calls easier to call
			float dxw = 0.003125f + .5f * dzw * abssx ; //adds a minimum AA (2/W) near x center
			float tri_fx = 0.f;
            float sx_filtered = 0.f;
			_triang(px, tri_fx);
            _clamp(tri_fx / dxw, sx_filtered);

			float tri_fz = 0.f;
            float sz_filtered = 0.f;
			_triang(pz, tri_fz);
            _clamp(tri_fz / dzw, sz_filtered); 

            float fog = v;
            int32 c = sx_filtered * sz_filtered * v; //product results in checkerboard pattern

            B = 40 + 32 - c;
#else
		    int16 l = 5000.f/v;
		    int16 cx = x*l;
		    int8 cz = l;
		    int16 check = (((cx>>6)^cz) & 64) ? v2int : 0;
		    B = 40 - check;
#endif
        }
        else
#endif
		{
	        B = 40;
	    }


#ifdef INCLUDE_SUNSHADOW
		if(frame > INCLUDE_SUNSHADOW)
	        R = 130 - v2int;
        else
#endif
		{
    	    R = 210 - v2int;
	    }

#ifdef INCLUDE_AMBIENTSHADOW
		if(frame > INCLUDE_AMBIENTSHADOW)
		{
		    int16 p = h + v2*8;
		    int16 cp = v * -240;
		    int16 c = cp - p;

		    // sky light / ambient occlusion
		    if(c > 1320)        
		    {
		        int16 o0 = (25*c)>>3;
		        int16 o = (c*(7840-o0)>>9) - 8560;
		        int32 Ro = R*o;
		        int32 Bo = B*o;
		        R = Ro>>10;
		        B = Bo>>10;
		    }
		}
#endif
#ifdef INCLUDE_SUNSHADOW
		if(frame > INCLUDE_SUNSHADOW)
		{
		    // sun/key light with soft shadow
		    //NOTE: this is expensive in terms of FPGA resources
		    float w = 50.f + v*4.f;
		    float r = u - w;
		    float wx = w + 24.f;
		    int16 d = r*r + u*wx;
		    if(d > 90)
		    	R = R + d - 90;
		}
#endif
    }
    else
    {
        //----------------
        // Section D, Sky 
        //----------------
        int8 c = x + y;
        R = 140 + c;
        B = 240 + c;
    }
    
    R = max(0, min(R, 255));
    B = max(0, min(B, 255));

#ifdef INCLUDE_SPHERE
    if(frame > INCLUDE_SPHERE && h < 200.f) 
    {
        //-------------------
        // Section B, Sphere 
        //-------------------
        int16 R2 = 420; //ambient light
        int16 B2 = 520;

        float t = 40.625f + h*0.0625f;
        float p = t*u;
        float q = t*z;
		int16 wsqint;
#ifdef INCLUDE_BOUNCELIGHT
        // bounce light
        if(frame > INCLUDE_BOUNCELIGHT)
        {
		    int32 qf = q*208;
		    int32 pk = p*80;
		    int32 pq = ((208+80)<<9) + pk - qf;
		    int16 w = pq>>8;
		   	wsqint = pq < 0 ? 0 : (w*w)>>10;
		}
		else
#endif
		{
			wsqint = 0;
		}
		
		int16 pqint = 0;
		int16 o;
#ifdef INCLUDE_SKYLIGHT
        // sky light / ambient occlusion
        if(frame > INCLUDE_SKYLIGHT)
        {
		    int16 pint = p;
		    int16 qint = q;
		    o = qint + 900;
		    pqint = pint + qint;
        }
        else
#endif
        {
	        o = 512;
        }
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

#ifdef INCLUDE_ANTIALIAS
         __sync_synchronize();
        if(frame <= INCLUDE_ANTIALIAS || h < 196.f)
        {
          R = R3;
          B = B3;
        }
        else
        {
          //antialias
          //NOTE: mismatch at input coordinates -27, -36
          int16 hint = h*64;
          int16 m = 200*64 - hint;
          int32 Rd = (R3-R)*m;
          int32 Bd = (B3-B)*m;
          R = R+(Rd>>8);
          B = B+(Bd>>8);
        }
#else
        R = R3;
        B = B3;
#endif
	}
#endif
#endif
    //-------------------------  

    G = (R+B)>>1;
    result = (B<<16) + (G<<8) + R;
}

