//UART simulator
//Copyright (C) 2023 Victor Suarez Rovere <suarezvictor@gmail.com>

#include <stdio.h>
#include <time.h>

#include "cflexhdl.h"

//#define _DEBUG

#ifdef _DEBUG
#define UART_CLKS_PER_BIT 1
#define ITERATIONS 1
#define DISPLAY printf
#else
#define ITERATIONS 1e8L
#define DISPLAY(...)
#endif

#ifdef CFLEX_NO_COROUTINES
#define wait_clk() clock_stats()
#endif


uint1_t tx_pin;
uint32_t clk_count = 0;
volatile long on_count = 0; //volatile ensures not extra optimizations

static inline void clock_stats()
{
  ++clk_count;
  on_count = on_count + tx_pin;
  DISPLAY("clk %d, TX pin: %d\n", clk_count, tx_pin);
}

#include "uart.cc" //algorithm implementation

void do_test(uint8_t data)
{
#ifdef CFLEX_NO_COROUTINES
    uart_tx(data, tx_pin, clk_count);
#else
    MODULE m = uart_tx(data, tx_pin, clk_count);
    while(m.clock())
      clock_stats();
#endif
}

int main()
{
  clock_t start_time = clock();
  uint8_t data = '0';
  for(long i = 0; i < ITERATIONS; ++i)
  {
    do_test(data);
    ++data;
  }
  clock_t dt = clock()-start_time;

  printf("SIMULATION RESULTS: Average bit value %.2f, clock frequency %0.f MHz\n", float(on_count)/clk_count, clk_count*1e-6*CLOCKS_PER_SEC/dt);
  return 0;
}

