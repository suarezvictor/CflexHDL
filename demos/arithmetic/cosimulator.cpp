//simulator for arithmetic modules
//Copyright (C) 2023 Victor Suarez Rovere <suarezvictor@gmail.com>

#include <stdio.h>
#include <time.h>

#include "cflexhdl.h"

//#define _DEBUG

#ifdef _DEBUG
#define ITERATIONS 30
#define DISPLAY printf
#else
#define ITERATIONS 5*1000*1000
#define DISPLAY(...)
#endif

#include "arithmetic.cc" //algorithm implementation
#include "VM_arithmetic.h"

VM_arithmetic *top = new VM_arithmetic;

#ifdef CFLEX_NO_COROUTINES
#error this requires coroutines, CFLEX_NO_COROUTINES should NOT be defined
#endif

int main()
{

  clock_t start_time = clock();
  int32_t clk_count = 0;
  uint16_t a = 0, b = 0, result = 0xFFFF, v_result = 0xFFFF;

  MODULE_TYPE *m = new MODULE_TYPE(prod16x16_16(a, b, result));
  for(int i = 0; i < ITERATIONS; ++i)
  {
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

    while(m->clock()); //execute all steps


    DISPLAY("A=0x%04X, B=0x%04X, C_RESULT=0x%04X, V_RESULT=0x%04X\n", a, b, result, v_result);

    if(result != v_result)
    {
      fprintf(stderr, "ERROR: cosimulation do not match\n");
      break;
    }

    delete m;
    m = new MODULE_TYPE(prod16x16_16(a, b, result));
  
    a = rand();
    b = rand();
  }

  clock_t dt = clock()-start_time;
  delete top;

  printf("SIMULATION RESULTS: clock frequency %0.f MHz\n", clk_count*1e-6*CLOCKS_PER_SEC/dt);
  return 0;
}

