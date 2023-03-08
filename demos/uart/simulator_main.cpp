//UART simulator
//Copyright (C) 2023 Victor Suarez Rovere <suarezvictor@gmail.com>

#include <stdio.h>
#include <time.h>

#include "cflexhdl.h"

//#define _DEBUG

#ifdef _DEBUG
#define ITERATIONS 20
#define DISPLAY printf
#else
#define ITERATIONS 1000*1000*1000
#define DISPLAY(...)
#endif

#ifdef CFLEX_NO_COROUTINES
#define wait_clk() clock_stats()
#endif
#define UART_CLKS_PER_BIT 1

uint1_t tx_pin;
int32 clk_count = 0;
/*volatile*/ long on_count = 0; //volatile ensures no extra optimizations


static inline void clock_stats()
{
  clk_count = clk_count + 1;
  on_count = on_count + tx_pin;
  DISPLAY("clk %d, TX pin: %d\n", clk_count, tx_pin);
}

#include "uart.cc" //algorithm implementation

/*
benchmark (MHz) - volatile
internal loop clang 324 coro/537 no coro
internal loop gcc 317 coro/490 no coro
no internal loop clang nocoro 537
no internal loop gcc nocoro 233
*/

void do_test(uint8_t data)
{
#ifdef CFLEX_NO_COROUTINES
    uart_tx(data, tx_pin, clk_count);
#else
    auto m = uart_tx(data, tx_pin, clk_count);
    while(m.clock())
      clock_stats();
#endif
}

int main()
{
  clock_t start_time = clock();
  uint8_t data = 'a';
  while(clk_count < ITERATIONS)
  {
    do_test(data);
    data = data == 'z' ? 'a' : data+1;
  }
  clock_t dt = clock()-start_time;

  printf("SIMULATION RESULTS: Average bit value %f, clock frequency %0.f MHz\n", float(on_count)/clk_count, clk_count*1e-6*CLOCKS_PER_SEC/dt);
  return 0;
}

