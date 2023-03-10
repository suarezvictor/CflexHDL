#include "cflexhdl.h"

//#define prod16x16_16 _arithmetic
//#define div16 _arithmetic
#define div16_newton _arithmetic

// SERIAL MULTIPLIER -------------------------------------------------------------------------------

#define PREC 16 //try 16 bits of precision or less

/*
//Clang assembly (unrolled loop)
    1180:	c1 ea 07             	shr    $0x7,%edx
    1183:	c1 ee 07             	shr    $0x7,%esi
    1186:	81 e6 ff 01 00 00    	and    $0x1ff,%esi
    118c:	81 e2 ff 01 00 00    	and    $0x1ff,%edx
    1192:	0f af d6             	imul   %esi,%edx
    1195:	c1 ea 02             	shr    $0x2,%edx
    1198:	01 ea                	add    %ebp,%edx
    119a:	69 ff 8f bc 00 00    	imul   $0xbc8f,%edi,%edi
    11a0:	83 c7 01             	add    $0x1,%edi
    11a3:	69 f7 8f bc 00 00    	imul   $0xbc8f,%edi,%esi
    11a9:	83 c6 01             	add    $0x1,%esi
    11ac:	c1 ef 07             	shr    $0x7,%edi
    11af:	69 de 8f bc 00 00    	imul   $0xbc8f,%esi,%ebx
    11b5:	c1 ee 07             	shr    $0x7,%esi
    11b8:	81 e6 ff 01 00 00    	and    $0x1ff,%esi
    11be:	81 e7 ff 01 00 00    	and    $0x1ff,%edi
    11c4:	0f af fe             	imul   %esi,%edi
    11c7:	c1 ef 02             	shr    $0x2,%edi
    11ca:	01 d7                	add    %edx,%edi
    11cc:	83 c3 01             	add    $0x1,%ebx
    11cf:	69 f3 8f bc 00 00    	imul   $0xbc8f,%ebx,%esi
    11d5:	83 c6 01             	add    $0x1,%esi
    11d8:	0f b7 ef             	movzwl %di,%ebp
    11db:	89 da                	mov    %ebx,%edx
    11dd:	89 f7                	mov    %esi,%edi
    11df:	83 c1 fe             	add    $0xfffffffe,%ecx
    11e2:	75 9c                	jne    1180 <main+0x30>
*/

MODULE prod16x16_16(const uint16& a, const uint16& b, uint16& result) //1 LSB error @11-16 bits
{
  uint18 ax;
  uint18 bx;
  ax = a << (18-PREC);
  bx = b << (18-PREC);

  uint9 a0 = ax & 511;
  uint9 a1 = (ax >> 9) & 511;

  uint9 b0 = bx & 511;
  uint9 b1 = (bx >> 9) & 511;

  uint27 c = 0;
  uint3 i = 4; //one hot
  while(i)
  {
    uint9 pa = i & 4 ? a0 : a1;
    uint9 pb = i & 2 ? b0 : b1;
    uint18 p = pa * pb;
    c = c + i & 1 ? p << 9 : p;
    i = i >> 1;

    wait_clk();
  }

  result = c >> (PREC-9+(18-PREC)+(18-PREC));
}

// SERIAL DIVISOR ----------------------------------------------------------------------------------

MODULE div16(const uint16& a, const uint16& b, uint16& result)
{
  uint16 RD = 0;
  uint16 R = 0;
  uint16 mask;
  uint16 R1;
  uint16 n;
  n = a;
  result = 0;
  for(mask = 32768; mask != 0; mask = mask >> 1)
  {
    R = (R << 1) | (n >> 15);
    RD = R - b;
    n = n << 1;

    add_clk();
    if (!(RD & 32768))
    {
      R = RD;
      result = result | mask;
    }
  }
}

// NEWTON DIVISOR ----------------------------------------------------------------------------------


uint16 fixed16_newton_initial_estimate(uint16 e)
{
  return e - (e>>4); //substracts 1/16 (so the factor is 15/16)
}

uint32 unsiged16_times_signed16(uint16 X, int16 y)
{
#if 0
    return X*y; //this makes cosimulation not to match
#else
    uint16 abs_y = y < 0 ? -y : y;
    uint32 m = X*abs_y;
    return y < 0 ? -m : m;
#endif
}

uint16 fixed16_div_newton(uint16 D, uint16 X)
{
  uint16 ua;
  int16 sb;
  int16 sc;
  uint32 xb;
  ua = D*X >> 16;
  sb = (ua^32767)+1;//0x8000u - a;
  xb = unsiged16_times_signed16(X, sb); //X*b; //unsigned by signed bring issues in synthesys
  sc = xb >> (16-1); //TODO: use a function more suited to fixed point to avoid 32-bit results
  return X + sc;
}

uint16 fixed16_approx_reciprocal_bounded_half(uint16 D) //0x8000=0.5 to 0xFFFF=~1.0, returns value/2
{
  uint16 X0;
  uint16 X1;
  uint16 X2;
  uint16 X = D^/*0x7FFF*/32767; //approximate negative and offset, good enough for an initial guess
  X0 = fixed16_newton_initial_estimate(X);
  X1 = fixed16_div_newton(D, X0);
  X2 = fixed16_div_newton(D, X1); //this further step may not be needed
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

MODULE div16_newton(const uint16& a, const uint16& b, uint16& result) //FIXME: correct parser/generator
{
  uint32 r;
  r = fixed16_div_aprox_32(a, b);
  result = r >> 16;
}

