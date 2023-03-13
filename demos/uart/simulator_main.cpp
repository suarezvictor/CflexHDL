//UART simulator
//Copyright (C) 2023 Victor Suarez Rovere <suarezvictor@gmail.com>

#include <stdio.h>
#include <time.h>

#include "cflexhdl.h"

#define _DEBUG

#ifdef _DEBUG
#define USE_VCD
#define ITERATIONS 20
#define DISPLAY printf
#else
#define ITERATIONS 10*1000*1000
#define DISPLAY(...)
#endif

#ifdef CFLEX_NO_COROUTINES
#define wait_clk() clock_stats()
#endif

#ifdef USE_VCD
#include "cflexvcd.h"
CflexVCD vcd("dump.vcd");
#endif

#ifndef CFLEX_VERILATOR
#ifndef USE_VCD
#define UART_CLKS_PER_BIT 1
#endif
#endif

uint1_t tx_pin;
uint1_t out_valid, done_irq;
int32 clk_count = 0;
/*volatile*/ long on_count = 0; //volatile ensures no extra optimizations


static inline void clock_stats()
{
#ifndef USE_VCD
  DISPLAY("clk %d, TX pin: %d, out_valid %d\n", clk_count, tx_pin, out_valid);
#endif
  clk_count = clk_count + 1;
  on_count = on_count + (tx_pin & out_valid);
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
void do_test(uint8_t in_data)
{
#ifdef CFLEX_NO_COROUTINES
    uart_tx(in_data, tx_pin, out_valid, clk_count);
#else
    auto m = uart_tx(in_data, tx_pin, out_valid, clk_count);
    for(;;)
    {
      done_irq = !m.clock();
      if(done_irq)
        break;
      clock_stats();
#ifdef USE_VCD
      vcd.clock(clk_count);
#endif
    }
#ifdef USE_VCD
    vcd.clock(clk_count);
#endif
#endif
}
#endif

int main()
{
  clock_t start_time = clock();
  uint8_t in_data = 'a';
#ifdef USE_VCD
  vcd.add_signal("out_valid", out_valid);
  vcd.add_signal("tx_pin", tx_pin);
  vcd.add_signal("done_irq", done_irq);
  vcd.add_signal("in_data", in_data, 8);
  vcd.start();
#endif

  for(int i = 0; i < ITERATIONS; ++i)
  {
#ifndef USE_VCD
    DISPLAY("clk %d, in_data 0x%02X\n", clk_count, in_data);
#endif
#ifdef CFLEX_VERILATOR
    top->in_run = 0;
    top->clock = 0; top->eval();
    top->clock = 1; top->eval();
    top->in_run = 1;
	top->in_data = in_data;
	for(;;)
	{
      out_valid = top->out_out_valid;
      tx_pin = top->out_tx_pin;
      done_irq = top->out_done;
#ifdef USE_VCD
      vcd.clock(top->in_clock_counter);
#endif
      ++top->in_clock_counter;
	  if(out_valid)
        clock_stats();


#ifndef USE_VCD
      DISPLAY("clk %d, internal clk %d, out_valid %d\n", clk_count, top->in_clock_counter, out_valid);
#endif
      top->clock = 0; top->eval();
      top->clock = 1; top->eval();

      if(done_irq)
        break;
    }
    done_irq = top->out_done;
#else
    do_test(in_data);
#endif
    in_data = in_data == 'z' ? 'a' : in_data+1;
  }
  clock_t dt = clock()-start_time;

  printf("SIMULATION RESULTS: Average bit value %f, clock frequency %0.f MHz\n", float(on_count)/clk_count, clk_count*1e-6*CLOCKS_PER_SEC/dt);
  return 0;
}

