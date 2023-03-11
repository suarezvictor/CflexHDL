#ifndef __MODULEDEF_H__
#define __MODULEDEF_H__

#ifndef CFLEX_SIMULATION
#define always(...) 1
#define wait_clk()
#define add_clk() __sync_synchronize() //special marker to paser to emit code
#define pipeline_stage() __builtin_huge_vall() //special marker to paser to emit code
#define MODULE void
#else
#ifdef CFLEX_NO_COROUTINES
//wait_clk is custom defined
#define MODULE void
#else

#include "coro.h"
#define wait_clk() {co_yield 0;}
#define MODULE_TYPE Coroutine
#define MODULE inline MODULE_TYPE

#endif

#define always(...) ({wait_clk(); true;})
//wait_clk is custom defined
#define add_clk() wait_clk()
#endif //CFLEX_SIMULATION

#endif //__MODULEDEF_H__
