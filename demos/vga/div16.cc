#include "cflexhdl.h"

#define CFLEX_USE_DIV_NEWTON //newton algorithm is faster when compiled

#ifdef CFLEX_USE_DIV_NEWTON

uint16 fixed16_newton_initial_estimate(uint16 x)
{
  return x - (x>>4); //substracts 1/16 (so the factor is 15/16)
}

uint16 fixed16_div_newton(uint16 D, uint16 X)
{
  uint16 a;
  int16 b;
  int16 c;
  a = (D*X) >> 16;
  b = (a^32767)+1;//0x8000u - a;
  c = X*b >> (16-1);
  return X + c;
}

uint16 fixed16_approx_reciprocal_bounded_half(uint16 D) //0x8000=0.5 to 0xFFFF=~1.0, returns value/2
{
  uint16 X0;
  uint16 X1;
  uint16 X2;
  uint16 X3;
  uint16 X = D^/*0x7FFF*/32767; //approximate negative and offset, good enough for an initial guess
  X0 = fixed16_newton_initial_estimate(X);
  X1 = fixed16_div_newton(D, X0);
  X2 = fixed16_div_newton(D, X1);
  return X2;
}

uint32 fixed16_div_aprox_32(uint16 N, const uint16& D0)
{
  uint32 R0, R1, R2, R3, R4;
  uint32 D1, D2, D3, D4;
  uint1 C0, C1, C2, C3;
  C0 = (D0 & (255<<8)) == 0;
  D1 = C0 ? D0 << 8 : D0;
  C1 = (D1 & (15<<12)) == 0;
  D2 = C1 ? D1 << 4: D1;
  C2 = (D2 & (3<<14)) == 0;
  D3 = C2 ? D2 << 2: D2;
  C3 = (D3 & (1<<15)) == 0;
  D4 = C3 ? D3 << 1: D3; 
  uint16 f;
  f = fixed16_approx_reciprocal_bounded_half(D4);
  R0 = N*f;
  R1 = C0 ? R0 : R0 >> 8;
  R2 = C1 ? R1 : R1 >> 4;
  R3 = C2 ? R2 : R2 >> 2;
  R4 = C3 ? R3 : R3 >> 1;
  return R4;
}

MODULE _div16(const uint16& num, const uint16& den, uint16& ret) //FIXME: correct parser/generator
{
  uint32 r;
  r = fixed16_div_aprox_32(num, den);
  ret = r >> 16;
}

#else

//typedef uint16 uint_div_width;

static MODULE _div16(const uint16& num, const uint16& den, uint16& ret)
{
  uint16 RD = 0;
  uint16 R = 0;
  uint16 mask;
  uint16 R1;
  uint16 n;
  n = num;
  ret = 0;
  for(mask = 32768; mask != 0; mask = mask >> 1)
  {
    R = (R << 1) | (n >> 15);
    n = n << 1;
    RD = R - den;

    add_clk();
    if (!(RD & 32768))
    {
      R = RD;
      ret = ret | mask;
    }
  }
}

#endif

