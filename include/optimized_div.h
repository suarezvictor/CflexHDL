//optimized fixed point division
// Copyright (C) 2022 Victor Suarez Rovere <suarezvictor@gmail.com>
// inspired on // https://en.wikipedia.org/wiki/Division_algorithm

///////////////////////////////////////////////////////
//typedef uint32_t div_newton_numerator_t;
#define div_newton_numerator_t int32_t

inline uint16_t fixed16_newton_initial_estimate(uint16_t x)
{
  return (uint16_t)(48*32768/17) - (x-(x>>4)); //can be a factor of 16/17 instead of this 15/16
}

inline uint16_t fixed16_div_newton(uint16_t D, uint16_t X)
{
  int16_t a = -((D*X)>>15)+1;
  int16_t c = (X * a)>>16;
  return X + c;
}

inline uint16_t fixed16_approx_reciprocal_bounded_half(uint16_t D) //range 0x8000=0.5 to 0xFFFF=~1.0, returns value/2
{
#if 1
  uint16_t X0 = fixed16_newton_initial_estimate(D);
  uint16_t X1 = fixed16_div_newton(D, X0);
  uint16_t X2 = fixed16_div_newton(D, X1);
  return X2;
#else
  return 0x80000000u/D;
#endif
}

inline div_newton_numerator_t fixed16_div_aprox_32(div_newton_numerator_t N, uint16_t D)
{
  uint8_t justify = 0;
#ifdef __CFLEXHDL_SYNTH__
  if((D & 0xFF00) == 0) { justify = justify | 8; D = D << 8; }
  if((D & 0xF000) == 0) { justify = justify | 4; D = D << 4; }
  if((D & 0xC000) == 0) { justify = justify | 2; D = D << 2; }
  if((D & 0x8000) == 0) { justify = justify | 1; D = D << 1; }
#else
  justify = __builtin_clz(uint32_t(D))-16; D <<= justify;
#endif
  div_newton_numerator_t R = N*(div_newton_numerator_t)fixed16_approx_reciprocal_bounded_half(D);
#ifndef __CFLEXHDL_SYNTH__
  R = R >> (15-justify);
#else
  if((justify & 8) == 0) R = R >> 8;
  if((justify & 4) == 0) R = R >> 4;
  if((justify & 2) == 0) R = R >> 2;
  if((justify & 1) == 0) R = R >> 1;
#endif
  return R;
}


