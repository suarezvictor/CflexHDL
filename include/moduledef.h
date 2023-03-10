#ifndef __MODULEDEF_H__
#define __MODULEDEF_H__

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
#define add_clk() wait_clk()


#endif //__MODULEDEF_H__
