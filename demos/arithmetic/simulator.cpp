//simulator for arithmetic modules
//Copyright (C) 2023 Victor Suarez Rovere <suarezvictor@gmail.com>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "cflexhdl.h"

#define _DEBUG

#ifdef _DEBUG
#define ITERATIONS 30
#define DISPLAY printf
#else
#define ITERATIONS 10*1000*1000
#define DISPLAY(...)
#endif

static long clk_count = 0;
#if defined(CFLEX_SIMULATION) && defined(CFLEX_NO_COROUTINES)
inline void wait_clk() {++clk_count;}
#endif

#include "arithmetic.cc" //algorithm implementation

#ifdef CFLEX_VERILATOR  
#include "VM_arithmetic.h"
VM_arithmetic *top = new VM_arithmetic;
#endif

uint16_t lc_random_16()
{
    static unsigned seed = 1;
    seed = 48271 * seed + 1;
    return seed;
}

int main()
{

  clock_t start_time = clock();
  uint16_t sum = 0;
  uint16_t a = 0, b = 0, result = 0xFFFF, v_result = 0xFFFF;
  int i;
  for(i = 0; i < ITERATIONS; ++i)
  {
    //DISPLAY("iteration %d\n", i);
#ifdef CFLEX_SIMULATION
#ifdef CFLEX_NO_COROUTINES
    _arithmetic(a, b, result);
#else
    MODULE_TYPE m = _arithmetic(a, b, result);
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
      //DISPLAY("verilator: step %ld, waiting...\n", clk_count);
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
#else
#ifndef CFLEX_VERILATOR
      ++clk_count;
#endif
#endif
#else
    result = v_result;
#endif

    DISPLAY("step %ld, A=0x%04X, B=0x%04X, C_RESULT=0x%04X, V_RESULT=0x%04X\n", clk_count, a, b, result, v_result);
    sum += result;

#ifdef CFLEX_VERILATOR  
    if(result != v_result)
    {
      fprintf(stderr, "FAIL: cosimulation does NOT match\n");
      exit(1);
    }
#endif
  
    
    a = lc_random_16();
    b = lc_random_16();
  }

  clock_t dt = clock()-start_time;
#ifdef CFLEX_VERILATOR  
  delete top;
#endif

  printf("SIMULATION RESULTS: time %.2fs, iterations %d, sum %hu, clock frequency %0.f MHz\n", float(dt)/CLOCKS_PER_SEC, i, sum, clk_count*1e-6*CLOCKS_PER_SEC/dt);

#if defined(CFLEX_SIMULATION) && defined(CFLEX_VERILATOR)
  printf("PASS\n");
#endif

  return 0;
}

