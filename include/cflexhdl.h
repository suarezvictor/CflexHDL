// Copyright (c) 2021 Victor Suarez Rovere <suarezvictor@gmail.com>

#ifndef __CFLEXHDL_H__
#define __CFLEXHDL_H__

#include "silice_compat.h"

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


#if !defined(CFLEX_SIMULATION)

#define always(...) 1
#define wait_clk() {}
#define MODULE void

#else

#define always(...) ({wait_clk(); true;})

#ifdef CFLEX_NO_COROUTINES
#define MODULE void
//wait_clk is custom defined
#else

#include "coro.h"
#define wait_clk() co_yield 0
#define MODULE Coroutine

#endif

#define CFLEX_INLINE __attribute__((always_inline))

#endif

//#define wait_cond(cond) while(always() && (cond)==0)

#define bitslice(B, E, n) (((n) >> (E)) & ((1 << ((B)-(E)+1))-1))

#endif //__SILICE_COMPAT_H__
