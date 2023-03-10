#include "cflexhdl.h"

#define prod16x16_16 _arithmetic
//#define div16 _arithmetic

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
    R1 = R << 1;

    add_clk();
    R = R1 | (n >> 15);
    n = n << 1;

    add_clk();
    RD = R - b;
    if (!(RD & 32768))
    {
      R = RD;
      result = result | mask;
    }
  }
}
