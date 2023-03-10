#include "cflexhdl.h"

//#define CFLEX_USE_DIV_NEWTON

#ifdef CFLEX_USE_DIV_NEWTON

void fixed16_newton_initial_estimate(uint16 x, uint16& ret)
{
  ret = x - (x>>4); //substracts 1/16 (so the factor is 15/16)
}

void fixed16_div_newton(uint16 D, uint16 X, uint16& ret)
{
  uint16 a;
  int16 b;
  int16 c;
  a = (D*X) >> 16;
  b = (a^32767)+1;//0x8000u - a;
  c = X*b >> (16-1);
  ret = X + c;
}

void fixed16_approx_reciprocal_bounded_half(uint16 D, uint16& ret) //0x8000=0.5 to 0xFFFF=~1.0, returns value/2
{
  uint16 X0;
  uint16 X1;
  uint16 X2;
  uint16 X = D^0x7FFF; //approximate negative and offset, good enough for an initial guess
  fixed16_newton_initial_estimate(X, X0);
  fixed16_div_newton(D, X0, X1);
  fixed16_div_newton(D, X1, X2);
  ret = X2;
}

void fixed16_div_aprox_32(uint16 N, uint16 D, uint32& ret)
{
  uint8 justify = 0;
  uint32 R;
  if((D & 0xFF00) == 0) { justify = justify | 8; D = D << 8; }
  if((D & 0xF000) == 0) { justify = justify | 4; D = D << 4; }
  if((D & 0xC000) == 0) { justify = justify | 2; D = D << 2; }
  if((D & 0x8000) == 0) { justify = justify | 1; D = D << 1; }
  R = N*fixed16_approx_reciprocal_bounded_half(D);
  if((justify & 8) == 0) R = R >> 8;
  if((justify & 4) == 0) R = R >> 4;
  if((justify & 2) == 0) R = R >> 2;
  if((justify & 1) == 0) R = R >> 1;
  ret = R;
}

MODULE div16(const uint16& num, const uint16& den, uint16& ret) //FIXME: correct parser/generator
{
  uint16 r;
  fixed16_div_aprox_32(num, den, r);
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

