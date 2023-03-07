//(C) 2023 Victor Suarez Rovere <suarezvictor@gmail.com>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef uint32_t uint18_t, uint19_t, uint27_t;
typedef uint16_t uint9_t;
typedef uint8_t uint3_t;

#define PREC 16 //try 16 bits of precision or less

uint16_t prod_ref(uint16_t a, uint16_t b)
{
  return (a*(uint32_t)b) >> PREC;
}


uint16_t prod0(uint16_t _a, uint16_t _b) //1 LSB error @11-16 bits
{
  uint18_t a = _a << (18-PREC);
  uint18_t b = _b << (18-PREC);

  uint9_t a0 = a & 0x1FF;
  uint9_t a1 = (a >> 9) & 0x1FF;

  uint9_t b0 = b & 0x1FF;
  uint9_t b1 = (b >> 9) & 0x1FF;


  uint18_t c2 = a1 * b1;
  uint19_t c1 = a1 * b0 + a0 * b1;

  return (c1  + (c2 << 9)) >> (PREC-9+(18-PREC)+(18-PREC));
}



uint16_t prod1(uint16_t _a, uint16_t _b) //no error @ 14 bits
{
  //divide and conquer algorithm
  //based on https://codecrucks.com/large-integer-multiplication-using-divide-and-conquer/
  uint16_t a = _a;
  uint16_t b = _b;

  uint8_t a0 = a & 0xFF;
  uint8_t a1 = a >> 8;

  uint8_t b0 = b & 0xFF;
  uint8_t b1 = b >> 8;


  uint16_t c2 = a1 * b1;
  uint16_t c0 = a0 * b0;
  uint9_t a10 = (a1 + a0) & 0x1FF;
  uint9_t b10 = (b1 + b0) & 0x1FF;
  uint18_t c1 = a10 * b10;
  c1 -= c2;
  c1 -= c0;

  return (c0 + (c1 << 8) + (c2 << 16)) >> PREC;
}

uint16_t prod2(uint16_t _a, uint16_t _b) //1 LSB error @11-16 bits
{
  uint18_t a = _a << (18-PREC);
  uint18_t b = _b << (18-PREC);

  uint9_t a0 = a & 0x1FF;
  uint9_t a1 = (a >> 9) & 0x1FF;

  uint9_t b0 = b & 0x1FF;
  uint9_t b1 = (b >> 9) & 0x1FF;

  uint27_t c = 0;
  uint3_t i = 4; //one hot
  while(i)
  {
    uint9_t pa = i & 4 ? a0 : a1;
    uint9_t pb = i & 2 ? b0 : b1;
    uint18_t p = pa * pb;
    c += i & 1 ? p << 9 : p;
    i = i >> 1;
  }

  return c >> (PREC-9+(18-PREC)+(18-PREC));
}

uint16_t prod3(uint16_t a, uint16_t b) //1 LSB error @11-16 bits
{
  uint27_t c = 0;
  uint3_t i = 1; //one hot
  while(i != 8)
  {
    uint9_t pa = (i & 1 ? a << 2 : a >> 7) & 0x1FF;
    uint9_t pb = (i & 2 ? b << 2 : b >> 7) & 0x1FF;
    uint18_t p = pa * pb;
    c += (i == 4) ? p << 9 : p;
    i = i << 1;
  }

  return c >> 11;
}

int main()
{
  srand(0);
  int maxerr = 0;
  for(int i = 0; i < 100*1000*1000; ++i)
  {
    uint16_t a = rand();
    uint16_t b = rand();
    a >>= (16-PREC);
    b >>= (16-PREC);
    uint16_t c0 = prod2(a, b);
    uint16_t c1 = prod_ref(a, b);
    int err = abs(c0 - c1);
    const float K = 1./(1 << PREC);
    /*
    printf("iter %d, a %d (%f), b %d (%f), prod %d (%f), ref %d (%f), error %d\n", i,
      a, a*K,
      b, b*K,
      c0, c0*K,
      c1, c1*K,
      err);*/
      
    if(maxerr < err)
      maxerr = err;
  }
  fprintf(stderr, "Max error %d\n", maxerr);
  return 0;
}
