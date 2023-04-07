//basic simulator for py4hw
//Copyright (C) 2023 Victor Suarez Rovere <suarezvictor@gmail.com>

#include <stdio.h>
#include <time.h>
#include <stdint.h>

#include "cflexhdl.h"

//#define _DEBUG

#ifdef _DEBUG
#define USE_VCD
#define ITERATIONS 20
#define DISPLAY printf
#else
#define ITERATIONS 10ull*1000*1000*1000
#define DISPLAY(...)
#endif

#ifdef CFLEX_NO_COROUTINES
#define wait_clk() clock_stats()
#endif

#ifdef USE_VCD
#include "cflexvcd.h"
CflexVCD vcd("dump.vcd");
#endif

static uint8 a, r;
static uint64_t clk_count = 0;
static uint64_t acc = 0; //generate some result to avoid solving everything at compile-time

static inline void clock_stats()
{
#ifndef USE_VCD
  DISPLAY("clk %lld, a %d, r: %d\n", clk_count, a, r);
#else
  vcd.clock(clk_count);
#endif
  ++clk_count;
  if(clk_count >= ITERATIONS)
    throw false;
  acc += r; //
}

#include "basic_py4hw.cc" //algorithm implementation

void do_test()
{
    a = 1;
    r = 0;
#ifdef CFLEX_NO_COROUTINES
    for(;;)
      TestCircuit(a, r);
#else
    auto m = TestCircuit(a, r);
    for(;;)
    {
      if(!m.clock())
        break;
      clock_stats();
    }
#endif
}

int main()
{
#ifdef USE_VCD
  vcd.add_signal("a", a, 8);
  vcd.add_signal("r", r, 8);
  vcd.start();
#endif

  clock_t start_time = clock();
  try {
    do_test();
  }
  catch(...) {}
  clock_t dt = clock()-start_time;

  printf("SIMULATION RESULTS: time %.1fs, cycles %llu, clock frequency %0.f MHz, result %llu\n",
      dt/float(CLOCKS_PER_SEC), clk_count, clk_count*1.e-6*CLOCKS_PER_SEC/dt, acc);
  return 0;
}

