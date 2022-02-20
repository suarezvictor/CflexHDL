// Copyright (c) 2021 Victor Suarez Rovere <suarezvictor@gmail.com>

#ifndef __SILICE_COMPAT_H__
#define __SILICE_COMPAT_H__

//TODO: add types
typedef unsigned uint26, uint25;
typedef unsigned short uint9, uint10, uint15, uint16;
typedef unsigned char uint4, uint6, uint8, uint7, uint1;
typedef signed short int16;

#if defined(CFLEX_SIMULATION) || defined(CFLEX_VERILATOR)
#define always() ({wait_clk(); true;})
#else
#define always() 1
#define wait_clk() {}
#endif

struct silice_module {};

#endif //__SILICE_COMPAT_H__
