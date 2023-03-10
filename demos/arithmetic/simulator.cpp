//simulator for arithmetic modules
//Copyright (C) 2023 Victor Suarez Rovere <suarezvictor@gmail.com>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "cflexhdl.h"

#if defined(CFLEX_NO_COROUTINES) && defined(CFLEX_VERILATOR)
#error cosimulation needs coroutines, CFLEX_NO_COROUTINES should NOT be defined
#endif

//#define _DEBUG

#ifdef _DEBUG
#define ITERATIONS 30
#define DISPLAY printf
#else
#define ITERATIONS 100*1000*1000
#define DISPLAY(...)
#endif

static long clk_count = 0;
#ifdef CFLEX_NO_COROUTINES
inline void wait_clk() {++clk_count;}
#endif

#include "arithmetic.cc" //algorithm implementation

#ifdef CFLEX_VERILATOR  
#include "VM_arithmetic.h"
VM_arithmetic *top = new VM_arithmetic;
#endif

int main()
{

  clock_t start_time = clock();
  uint16_t sum = 0;
  uint16_t a = 0, b = 0, result = 0xFFFF, v_result = 0xFFFF;

  for(int i = 0; i < ITERATIONS; ++i)
  {
#ifdef CFLEX_SIMULATION
#ifdef CFLEX_NO_COROUTINES
    prod16x16_16(a, b, result);
#else
    MODULE_TYPE m = prod16x16_16(a, b, result);
#endif
#endif

#ifdef CFLEX_VERILATOR  
    top->in_run = 0;
    top->clock = 0; top->eval();
    top->clock = 1; top->eval();
    top->in_run = 1;
    top->in_a = a;
    top->in_b = b;

	while(!top->out_done)
	{
      //DISPLAY("waiting...\n");
      top->clock = 0; top->eval();
      top->clock = 1; top->eval();

      v_result = top->out_result;
      ++clk_count;
    }
#endif

#ifdef CFLEX_SIMULATION
#ifndef CFLEX_NO_COROUTINES
    while(m.clock()) //execute all steps
    {
#ifndef CFLEX_VERILATOR
      ++clk_count;
#endif
    }
#endif
#else
    result = v_result;
#endif

    DISPLAY("A=0x%04X, B=0x%04X, C_RESULT=0x%04X, V_RESULT=0x%04X\n", a, b, result, v_result);
    sum += result;

#ifdef CFLEX_VERILATOR  
    if(result != v_result)
    {
      fprintf(stderr, "ERROR: cosimulation do not match\n");
      break;
    }
#endif
  
    a = rand();
    b = rand();
  }

  clock_t dt = clock()-start_time;
#ifdef CFLEX_VERILATOR  
  delete top;
#endif

  printf("SIMULATION RESULTS: sum %hu, clock frequency %0.f MHz\n", sum, clk_count*1e-6*CLOCKS_PER_SEC/dt);
  return 0;
}

