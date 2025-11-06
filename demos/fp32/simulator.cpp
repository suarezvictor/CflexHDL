//simulator for floating point
//Copyright (C) 2025 Victor Suarez Rovere <suarezvictor@gmail.com>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "cflexhdl.h"

#define _DEBUG

#ifdef _DEBUG
#define ITERATIONS 1000	
#define DISPLAY printf
#else
#define ITERATIONS 1*1000*1000
#define DISPLAY(...)
#endif

static long clk_count = 0;
#if defined(CFLEX_SIMULATION) && defined(CFLEX_NO_COROUTINES)
inline void wait_clk() {++clk_count;}
#endif

#include "mandel_fp32.cc"

#ifdef CFLEX_VERILATOR  
#include "VM_fp32.h"
VM_fp32 *top = new VM_fp32;
#endif

uint32_t random_fp32()
{
	fp32_t x;
    //x.f =  ((float)rand() / RAND_MAX) * 4. - 2.;
    x.u = (rand() / (RAND_MAX/480)) - 240; //for mandel
    return x.u;
}

int main()
{

  clock_t start_time = clock();
  uint32_t sum = 0;
  uint32_t a = 0, b = 0, result = 0xFFFF, v_result = 0xFFFF;
  int i;
  for(i = 0; i < ITERATIONS; ++i)
  {
    DISPLAY("iteration %d\n", i);

    a = random_fp32();
    b = random_fp32();

#ifdef CFLEX_SIMULATION
#ifdef CFLEX_NO_COROUTINES
    _fp32(a, b, result);
#else

#error COROUTINES not tested!
    MODULE_TYPE m = _fp32(a, b, result);
#endif
#endif

#ifdef CFLEX_VERILATOR  
    top->in_run = 0;
    top->clock = 0; top->eval();
    top->clock = 1; top->eval();
    top->in_run = 1;
    top->in_ua = a;
    top->in_ub = b;

	int multicycle = 0;
	while(!top->out_done)
	{
      top->clock = 0; top->eval();
      top->clock = 1; top->eval();

      v_result = top->out_result;
      ++clk_count;
      ++multicycle;
    }
    if(multicycle)
      DISPLAY("simulation intermediate steps: %ld\n", multicycle);
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

    fp32_t f_a, f_b, f_result, f_v_result;
    f_a.u = a;
    f_b.u = b;
    f_result.u = result;
    f_v_result.u = v_result;
    DISPLAY("step %ld, A=%f(0x%08X) B=%f(0x%08X), C_RESULT=%f(0x%08X), V_RESULT=%f(0x%08X)\n",
      clk_count, f_a.f, f_a.u, f_b.f, f_b.u, f_result.f, f_result.u, f_v_result.f, f_v_result.u);
    sum += result;

#ifdef CFLEX_VERILATOR  
    if(result != v_result)
    //if(fabs(double(result) - double(v_result)) > 1)
    //if(fabs(double(result) - double(v_result)) > 8)
    {
      fprintf(stderr, "FAIL: cosimulation does NOT match\n");
      exit(1);
    }
#endif
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

