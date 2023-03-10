#include "cflexhdl.h"

#define prod16x16_16 _arithmetic

#define PREC 16 //try 16 bits of precision or less

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

