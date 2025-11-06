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
MODULE _float_mul(const uint32& ua, const uint32& ub, uint32& result)
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
    	r.exp = ua == 0 || ub  == 0 ? 0 : exp; //handles zero case
    	r.frac = mant >> 23;
    }

    result = r_u;
}

// ------------------------
// Addition / Substraction
// ------------------------
MODULE _float_add(const uint32& ua, const uint32& ub, uint32& result)
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

    uint8 exp_a = a.exp;
    uint8 exp_b = b.exp;
    uint32 mant_a;
    uint32 mant_b;
    
    uint8 exp;
    
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

MODULE _float_sub(const uint32& ua, const uint32& ub, uint32& result)
{
  _float_add(ua, ub^0x80000000, result); //just change sign of b
}

// ------------------------
// Division
// ------------------------
MODULE _float_div(const uint32& ua, const uint32& ub, uint32& result)
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
// Comparison
// ------------------------
//FIXME: this requires syntax support in parser/generator
int32 float_monotonic(int32 a) { return (a < 0) ? (-a) ^ 0x80000000 : a; }

MODULE _float_lt(const int32& ua, const int32& ub, uint32& result)
{
#if 1
  int32 ai = (ua < 0) ? (-ua) ^ 0x80000000 : ua;
  int32 bi = (ub < 0) ? (-ub) ^ 0x80000000 : ub;
#else
  int32 ai = float_monotonic(ua);
  int32 bi = float_monotonic(ub);
#endif
  result = ai < bi;
}


// ------------------------
// Float/Integer conversion
// ------------------------

MODULE _float_to_int(const uint32& a, int32& result)
{
	union {
		uint32 i_u;
		struct { uint32 frac:23; uint32 exp:8; uint32 sign:1; } i;
	};
	
	i_u = a;

    int8 shift = i.exp - 127 - 23;

    if (shift < -23)
    {
        result = 0;
    }
    else
    {
       uint32 mant = i.frac | (1 << 23);
       int32 value;

       value = (shift >= 0) ? mant << shift : mant >> -shift; //FIXME: this is too slow in logic

       if(i.sign)
         result = 0-value; //FIXME: support unary operator
       else
         result = value;
    }
}

MODULE _float_int(const int32& i, uint32& result)
{
	union {
		uint32 r_u;
		struct { uint32 frac:23; uint32 exp:8; uint32 sign:1; } r;
	};

	uint32 a = i < 0 ? -i : i;
#if 1
	//TODO: this has duplicated code to count zeros
	uint32 mant0 = a;
	uint32 mant1, mant2, mant3, mant4, mant5;
	int8 exp0 = 31;
	int8 exp1, exp2, exp3, exp4, exp5;
	if ((mant0 & 0xFFFF0000u) == 0) { exp1 = exp0 - 16; mant1 = mant0 << 16; } else { mant1 = mant0; exp1 = exp0; }
	if ((mant1 & 0xFF000000u) == 0) { exp2 = exp1 -  8; mant2 = mant1 <<  8; } else { mant2 = mant1; exp2 = exp1; }
	if ((mant2 & 0xF0000000u) == 0) { exp3 = exp2 -  4; mant3 = mant2 <<  4; } else { mant3 = mant2; exp3 = exp2; }
	if ((mant3 & 0xC0000000u) == 0) { exp4 = exp3 -  2; mant4 = mant3 <<  2; } else { mant4 = mant3; exp4 = exp3; }
	if ((mant4 & 0x80000000u) == 0) { exp5 = exp4 -  1; mant5 = mant4 <<  1; } else { mant5 = mant4; exp5 = exp4; }
#else
    int8 z = __builtin_clz(a);
    int8 exp5 = 31 - z;
    uint32 mant5 = (a << z);
#endif
	r.sign = i < 0;
	r.exp = i == 0 ? 0 : exp5 + 127;
	r.frac = mant5 >> 8;
	
	result = r_u;
}


// ------------------------
// Misc functions
// ------------------------
MODULE _float_mul_int(const uint32& ua, const int32& b, uint32& result)
{
   uint32 ub;
   _float_int(b, ub);
   _float_mul(ua, ub, result);
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
        int32 a_int = i < 2 ? 0 : int32(a) >> 11;

        uint32 r_mul, r_add, r_div;
        uint32 r_int;

        _float_mul(a, b, r_mul);
        _float_add(a, b, r_add);
        _float_div(a, b, r_div);
        _float_int(a_int, r_int);

        float f_mul = *reinterpret_cast<float*>(&r_mul);
        float f_add = *reinterpret_cast<float*>(&r_add);
        float f_div = *reinterpret_cast<float*>(&r_div);
        float f_int = *reinterpret_cast<float*>(&r_int);

        if (fabs(f_mul - (a_f*b_f)) > eps)
            std::cout << "ERROR mul: " << a_f << " * " << b_f << " = " << f_mul
                      << " (expected " << a_f*b_f << ")\n";

        if (fabs(f_add - (a_f+b_f)) > eps)
            std::cout << "ERROR add: " << a_f << " + " << b_f << " = " << f_add
                      << " (expected " << a_f+b_f << ")\n";

        if (fabs(f_div - (a_f/b_f)) > eps)
            std::cout << "ERROR div: " << a_f << " / " << b_f << " = " << f_div
                      << " (expected " << a_f/b_f << ")\n";

        if (a_int != f_int)
            std::cout << "ERROR a " << a_int << " f_int " << f_int << ")\n";

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
