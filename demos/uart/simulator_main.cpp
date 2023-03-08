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
#define ITERATIONS 10*1000*1000
#define DISPLAY(...)
#endif

#ifdef CFLEX_NO_COROUTINES
#define wait_clk() clock_stats()
#endif
#define UART_CLKS_PER_BIT 1

uint1_t tx_pin;
uint1_t out_valid;
int32 clk_count = 0;
/*volatile*/ long on_count = 0; //volatile ensures no extra optimizations


static inline void clock_stats()
{
  clk_count = clk_count + 1;
  on_count = on_count + (tx_pin & out_valid);
  DISPLAY("clk %d, TX pin: %d, out_valid %d\n", clk_count, tx_pin, out_valid);
}

#include "uart.cc" //algorithm implementation

/*
benchmark (MHz) - volatile
internal loop clang 324? coro/537? no coro
internal loop gcc 317? coro/490? no coro
no internal loop clang nocoro 1212
no internal loop gcc nocoro 807
no internal loop gcc coro 189
verilator 11
*/

#ifdef CFLEX_VERILATOR
#include "VM_uart_tx.h"
#include "VM_uart_tx__Syms.h"
VM_uart_tx *top = new VM_uart_tx;
#else
void do_test(uint8_t data)
{
#ifdef CFLEX_NO_COROUTINES
    uart_tx(data, tx_pin, out_valid, clk_count);
#else
    auto m = uart_tx(data, tx_pin, out_valid, clk_count);
    while(m.clock())
      clock_stats();
#endif
}
#endif

int main()
{
  clock_t start_time = clock();
  uint8_t data = 'a';

  for(int i = 0; i < ITERATIONS; ++i)
  {
    DISPLAY("clk %d, data 0x%02X\n", clk_count, data);
#ifdef CFLEX_VERILATOR
    top->in_run = 0;
    top->clock = 0; top->eval();
    top->clock = 1; top->eval();
    top->in_run = 1;
	top->in_data = data;
	while(!top->out_done)
	{
      top->in_clock_counter = clk_count;
      out_valid = top->out_out_valid;
      DISPLAY("out_valid %d\n", out_valid);
	  if(out_valid)
	  {
        tx_pin = top->out_tx_pin;
        clock_stats();
      }
      
      top->clock = 0; top->eval();
      top->clock = 1; top->eval();
    }
#else
    do_test(data);
#endif
    data = data == 'z' ? 'a' : data+1;
  }
  clock_t dt = clock()-start_time;

  printf("SIMULATION RESULTS: Average bit value %f, clock frequency %0.f MHz\n", float(on_count)/clk_count, clk_count*1e-6*CLOCKS_PER_SEC/dt);
  return 0;
}

