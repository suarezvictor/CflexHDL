//simulator for arithmetic modules
//Copyright (C) 2023 Victor Suarez Rovere <suarezvictor@gmail.com>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "cflexhdl.h"

//#define _DEBUG

#ifdef _DEBUG
#define ITERATIONS 30
#define DISPLAY printf
#else
#ifndef CFLEX_VERILATOR
#define ITERATIONS 100*1000*1000
#else
#define ITERATIONS 1*1000*1000
#endif
#define DISPLAY(...)
#endif

#include "arithmetic.cc" //algorithm implementation

#ifdef CFLEX_VERILATOR  
#include "VM_arithmetic.h"
VM_arithmetic *top = new VM_arithmetic;
#endif

#ifdef CFLEX_NO_COROUTINES
#error this requires coroutines, CFLEX_NO_COROUTINES should NOT be defined
#endif

int main()
{

  clock_t start_time = clock();
  long clk_count = 0;
  uint16_t a = 0, b = 0, result = 0xFFFF, v_result = 0xFFFF;

  for(int i = 0; i < ITERATIONS; ++i)
  {
    MODULE_TYPE m = prod16x16_16(a, b, result);
    
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
    while(m.clock()) //execute all steps
    {
#ifndef CFLEX_VERILATOR
      ++clk_count;
#endif
    }

    DISPLAY("A=0x%04X, B=0x%04X, C_RESULT=0x%04X, V_RESULT=0x%04X\n", a, b, result, v_result);

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

  printf("SIMULATION RESULTS: clock frequency %0.f MHz\n", clk_count*1e-6*CLOCKS_PER_SEC/dt);
  return 0;
}

