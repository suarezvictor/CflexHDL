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
typedef uint32 uint32_t;

typedef int10 int10_t;
#define uintN_internal(n) uint##n
#define uintN(n) uintN_internal(n)


#if !defined(CFLEX_SIMULATION)

#define always(...) 1
#define wait_clk()
#define add_clk() __sync_synchronize()
#define pipeline_stage() __builtin_huge_vall()
#define MODULE void

#else
#define add_clk() wait_clk()
#define pipeline_stage()

#define always(...) ({wait_clk(); true;})

#ifdef CFLEX_NO_COROUTINES
#define MODULE void
//wait_clk is custom defined
#else

#include "coro.h"
#define wait_clk() {co_yield 0;}
#include "moduledef.h"

#endif

#define CFLEX_INLINE __attribute__((always_inline))

#endif

//#define wait_cond(cond) while(always() && (cond)==0)

#define bitslice(B, E, n) (((n) >> (E)) & ((1 << ((B)-(E)+1))-1))

#ifndef promote_u64
#define promote_u64(x) (x) //just needed if compiled (like 32-bit multiplication with 64-bit output)
#endif

#ifndef promote_u128
#define promote_u128(x) (x) //just needed if compiled (like 32-bit multiplication with 64-bit output)
#endif


#endif //__SILICE_COMPAT_H__
