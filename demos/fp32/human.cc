// Human Shader, float version (C) Victor Suarez Rovere 2025
//
// adapted from: https://www.shadertoy.com/view/4ft3Wn
// Copyright © 2023-2024 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include "../src/fp32.cc"

//FIXME do not use macros for this
#define min(a, b) ( a < b ? a : b)
#define max(a, b) ( a > b ? a : b)

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
    float v = 0.f - y;
    float z = v;
    float u2 = u*u;
    float v2 = z*z;
    float h = u2 + v2;

	if(v <0.f)
    {
        //-------------------
        // Section C, Ground 
        //-------------------
        int32 v2int = v + v;
        R = 150 + v2int;
        B = 50;

        int32 p = h + 8.f*v2;
        int32 cp = v * -240.f;
        int32 c = cp - p;

        // sky light / ambient occlusion
        if( c>1200 )
        {
            int32 o0 = (25*c)>>3;
            int32 o = (c*(7840-o0)>>9) - 8560;
            R = (R*o)>>10;
            B = (B*o)>>10;
        }

        // sun/key light with soft shadow
        float w0 = v*4.f;
        float w = w0 + 50.f;
        float r = u - w;
        float wx = w + 24.f;
        int32 d = r*r + u*wx;
        if( d>90 )
        	R = R + d-90;
    }
    else
    {
        //----------------
        // Section D, Sky 
        //----------------
        int32 c = x +4.f*y;
        R = 132 + c;
        B = 192 + c;
    }
    
    R = max(0, min(R, 255));
    B = max(0, min(B, 255));
        
    if( h < 200.f) 
    {
        //-------------------
        // Section B, Sphere 
        //-------------------
        int32 R2 = 420;
        int32 B2 = 520;

        float t = 5200.f + h*8.f;
        float K1 = 1.f/128.f;
        float p = t*u*K1;
        float q = t*z*K1;
        
        // bounce light
        float K2 = 1.f/512.f;
        float qf = q*13.f;
        float pq = p*5.f - qf;
        float w = 18.f + pq*K2;
        if(0.f < w)
        {
        	int32 wsqint = w*w;
        	R2 = R2 + wsqint;
        }
        
        // sky light / ambient occlusion
        int32 qint = q;
        int32 o = qint + 900;
        R2 = (R2*o)>>12;
        B2 = (B2*o)>>12;

        // sun/key light
        if( 0.f < p + q )
        {
            int32 pqint = p + q;
            int32 w = pqint>>3;
            R2 = R2 + w;
            B2 = B2 + w;
        }

        R2 = min(R2, 255);
        B2 = min(B2, 255);
        
        if(h < 196.f)
        {
          R = R2;
          B = B2;
        }
        else
        {
          //antialias
          uint32 hint = h*64;
          uint32 m = 200*64 - hint;
          R = R+(((R2-R)*m)>>8);
          B = B+(((B2-B)*m)>>8);
        }
	}
    //-------------------------  

    G = (R*11 + 5*B)>>4;
    result = (B<<16) + (G<<8) + R;
}

