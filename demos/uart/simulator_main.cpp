//UART simulator
//Copyright (C) 2023 Victor Suarez Rovere <suarezvictor@gmail.com>

#include <stdio.h>
#include <time.h>

#include "cflexhdl.h"

//#define _DEBUG

#ifdef _DEBUG
#define ITERATIONS 1
#else
#define ITERATIONS 1e8L
#endif

#ifndef CFLEX_NO_COROUTINES
#error CFLEX_NO_COROUTINES not supported in this demo
#endif

uint1_t tx_pin;
uint32_t clk_count = 0;
long on_count = 0;

inline void wait_clk()
{
   ++clk_count;
   on_count += tx_pin;
#ifdef _DEBUG
   printf("clk %d, TX pin: %d\n", clk_count, tx_pin);
#endif

}

#define UART_CLKS_PER_BIT 1
#include "uart.cc" //algorithm implementation

void do_test()
{
  for(long i = 0; i < ITERATIONS; ++i)
  {
    uart_tx((uint8_t) i, tx_pin, clk_count);
  }
}

int main()
{
  timespec start_time, end_time;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);

  do_test();
  
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
  long nanos = (end_time.tv_sec - start_time.tv_sec) * 1e9L + (end_time.tv_nsec - start_time.tv_nsec);

  //expected value ~1350MHz
  printf("SIMULATION RESULTS: Average bit value %.2f, clock frequency %d MHz\n", float(on_count)/clk_count, clk_count/(nanos/1000));

  return 0;
}

