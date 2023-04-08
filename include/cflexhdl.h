// Copyright (c) 2021 Victor Suarez Rovere <suarezvictor@gmail.com>

#ifndef __CFLEXHDL_H__
#define __CFLEXHDL_H__

#include "silice_compat.h"
#include "moduledef.h"

//TODO: add missing widths
typedef uint1 uint1_t;
typedef uint4 uint4_t;
typedef uint6 uint6_t;
typedef uint7 uint7_t;
typedef uint8 uint8_t;
typedef uint9 uint9_t;
typedef uint10 uint10_t;
typedef uint15 uint15_t;
typedef uint16 uint16_t;
typedef uint25 uint25_t;
typedef uint32 uint32_t;

typedef int10 int10_t;
#define uintN_internal(n) uint##n
#define uintN(n) uintN_internal(n)


#define CFLEX_INLINE __attribute__((always_inline))

//#define wait_cond(cond) while(always() && (cond)==0)

#define bitslice(B, E, n) (((n) >> (E)) & ((1 << ((B)-(E)+1))-1))

#ifndef promote_u64
#define promote_u64(x) (x) //just needed if compiled (like 32-bit multiplication with 64-bit output)
#endif

#ifndef promote_u128
#define promote_u128(x) (x) //just needed if compiled (like 32-bit multiplication with 64-bit output)
#endif


template<class T>
class reg //delayed assigment
{
  T& d;
  T q;
  reg() = delete;
  reg(reg&) = delete;
public:
  reg(reg&&) = default;
  reg(T& _d) : d(_d), q(_d) {}
  ~reg() { d = q; } //copy data on destruction
  operator T() const { return d; }
  void operator = (T next) { q = next; }
};

template<class T> reg<T> sync(T& r) { return r; }


#endif //__SILICE_COMPAT_H__
