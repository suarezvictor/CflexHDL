#include "cflexhdl.h"

#ifndef CFLEX_PARSER
union fp32_t {
    uint32 u;
    struct { uint32 frac:23; uint32 exp:8; uint32 sign:1; };
    float f;
};
#endif

// ------------------------
// Multiplication
// ------------------------
MODULE _fp32_mul(const uint32& ua, const uint32& ub, uint32& result)
{
	union {
		uint32 a_u;
		struct { uint32 frac:23; uint32 exp:8; uint32 sign:1; } a;
	};

	union {
		uint32 b_u;
		struct { uint32 frac:23; uint32 exp:8; uint32 sign:1; } b;
	};

	union {
		uint32 r_u;
		struct { uint32 frac:23; uint32 exp:8; uint32 sign:1; } r;
	};

	a_u = ua;
	b_u = ub;

    int8 exp = a.exp + b.exp - 127;
    uint25 mant_a = a.frac | 0x800000;
    uint25 mant_b = b.frac | 0x800000;
    uint64 mant = promote_u64(mant_a) * promote_u64(mant_b);

    r.sign = a.sign ^ b.sign;
    if ((mant >> 47) & 1)
    {
    	r.exp = exp + 1;
    	r.frac = mant >> 24;
    }
    else
    {
    	r.exp = exp;
    	r.frac = mant >> 23;
    }

    result = r_u;
}

// ------------------------
// Addition
// ------------------------
MODULE _fp32_add(const uint32& ua, const uint32& ub, uint32& result)
{
	union {
		uint32 a_u;
		struct { uint32 frac:23; uint32 exp:8; uint32 sign:1; } a;
	};

	union {
		uint32 b_u;
		struct { uint32 frac:23; uint32 exp:8; uint32 sign:1; } b;
	};

	union {
		uint32 r_u;
		struct { uint32 frac:23; uint32 exp:8; uint32 sign:1; } r;
	};

	a_u = ua;
	b_u = ub;

    int8 exp_a = a.exp;
    int8 exp_b = b.exp;
    uint32 mant_a;
    uint32 mant_b;
    
    int8 exp;
    
    // Align exponents
    if (exp_a >= exp_b)
    {
      exp = exp_a;
      mant_a = a.frac | 0x800000;
      mant_b = (b.frac | 0x800000) >> (exp_a - exp_b);
    }
    else if (exp_b > exp_a)
    {
      exp = exp_b;
      mant_b = b.frac | 0x800000;
      mant_a = (a.frac | 0x800000) >> (exp_b - exp_a);
    }


    uint32 mant;
    uint1 sign;

    // Addition or subtraction depending on signs
    if (a.sign == b.sign)
    {
        mant = mant_a + mant_b;
        sign = a.sign;
    } else {
        if (mant_a >= mant_b)
        {
        	mant = mant_a - mant_b;
        	sign = a.sign;
        }
        else
        {
        	mant = mant_b - mant_a;
        	sign = b.sign;
        }
    }

    // Normalize mantissa
    if (mant == 0)
    {
        result = 0;
    }
    else
    {
		if (mant & (1 << 24))
		{
		    // overflow: shift right once
		    r.frac = mant >> 1;
		    r.exp = exp + 1;
		}
		else if (!(mant & 0xFF800000))
		{
		    // underflow: shift left until top bit set
		    uint32 mant0 = mant;
		    uint32 mant1, mant2, mant3, mant4, mant5;
		    uint8 exp0 = exp;
		    uint8 exp1, exp2, exp3, exp4, exp5;
			if ((mant0 & 0xFFFF0000u) == 0) { exp1 = exp0 - 16; mant1 = mant0 << 16; } else { mant1 = mant0; exp1 = exp0; }
			if ((mant1 & 0xFF000000u) == 0) { exp2 = exp1 -  8; mant2 = mant1 <<  8; } else { mant2 = mant1; exp2 = exp1; }
			if ((mant2 & 0xF0000000u) == 0) { exp3 = exp2 -  4; mant3 = mant2 <<  4; } else { mant3 = mant2; exp3 = exp2; }
			if ((mant3 & 0xC0000000u) == 0) { exp4 = exp3 -  2; mant4 = mant3 <<  2; } else { mant4 = mant3; exp4 = exp3; }
			if ((mant4 & 0x80000000u) == 0) { exp5 = exp4 -  1; mant5 = mant4 <<  1; } else { mant5 = mant4; exp5 = exp4; }
			
		    r.frac = mant5 >> 8;
		    r.exp = exp5 + 8;
		}
		else
		{
			r.frac = mant;
			r.exp  = exp;
		}

		r.sign = sign;

		result = r_u;
    }
}

// ------------------------
// Division
// ------------------------
MODULE _fp32_div(const uint32& ua, const uint32& ub, uint32& result)
{
	union {
		uint32 a_u;
		struct { uint32 frac:23; uint32 exp:8; uint32 sign:1; } a;
	};

	union {
		uint32 b_u;
		struct { uint32 frac:23; uint32 exp:8; uint32 sign:1; } b;
	};

	union {
		uint32 r_u;
		struct { uint32 frac:23; uint32 exp:8; uint32 sign:1; } r;
	};

	a_u = ua;
	b_u = ub;

    int8 exp = a.exp - b.exp;
    uint32 mant_a = a.frac | 0x800000;
    uint32 mant_b = b.frac | 0x800000;
    
    uint32 mant;
    if(ub == 0)
    {
      result = 0;
    }
    else
    {

	//division algorithm
	uint32 dividend = mant_a;
	uint32 divisor = mant_b;
#if 1
	uint32 quotient = 0;
    uint32 remainder = 0;

    uint64 bit = promote_u64(1) << (24+23);

    while(bit)
    {
        uint32 r = (remainder << 1) | ((dividend & (bit >> 23)) ? 1 : 0);

        if(r >= divisor)
        {
	        remainder = r - divisor;
    	    quotient = quotient | bit;
    	}
    	else
    	  remainder = r;
    	
        bit = bit >> 1;
    }

    mant =  quotient;
#else
	mant = dividend / divisor;
#endif

    if (mant & (1 << 23))
    {
      r.exp  = exp + 127;
      r.frac = mant;
    }
    else
    {
      r.exp  = exp - 1 + 127;
      r.frac = mant << 1;
    }
    

    r.sign = a.sign ^ b.sign;

    result = r_u;
    }
}

// ------------------------
// Test helpers
// ------------------------
#ifndef CFLEX_SIMULATION
#ifndef CFLEX_PARSER

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <iostream>
#include <iomanip>

float random_float()
{
    return ((float)rand() / RAND_MAX) * 4. - 2.;
}

void test_random_operations(int count, float eps) {
    for (int i = 0; i < count; ++i) {
        float a_f = random_float();
        float b_f = random_float();

        uint32 a = *reinterpret_cast<uint32*>(&a_f);
        uint32 b = *reinterpret_cast<uint32*>(&b_f);

        uint32 r_mul, r_add, r_div;

        _fp32_mul(a, b, r_mul);
        _fp32_add(a, b, r_add);
        _fp32_div(a, b, r_div);

        float f_mul = *reinterpret_cast<float*>(&r_mul);
        float f_add = *reinterpret_cast<float*>(&r_add);
        float f_div = *reinterpret_cast<float*>(&r_div);

        if (fabs(f_mul - (a_f*b_f)) > eps)
            std::cout << "ERROR mul: " << a_f << " * " << b_f << " = " << f_mul
                      << " (expected " << a_f*b_f << ")\n";

        if (fabs(f_add - (a_f+b_f)) > eps)
            std::cout << "ERROR add: " << a_f << " + " << b_f << " = " << f_add
                      << " (expected " << a_f+b_f << ")\n";

        if (fabs(f_div - (a_f/b_f)) > eps)
            std::cout << "ERROR div: " << a_f << " / " << b_f << " = " << f_div
                      << " (expected " << a_f/b_f << ")\n";
    }
}


int main()
{
    srand((unsigned)time(NULL));
    
#if 0    
    uint32 a = 1234, b = 53, r;
    r = div24(a, b);
    printf("a %d, b %d, r %d, expected %d\n", a, b, r, a/b);
    exit(1);
#endif    
    test_random_operations(1000, 1e-5);
    return 0;
}

#endif
#endif
