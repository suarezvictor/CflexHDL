//UART simulator
//Copyright (C) 2023 Victor Suarez Rovere <suarezvictor@gmail.com>

#include <stdio.h>
#include <time.h>
#include <iostream> //just for placement new definition

#include "cflexhdl.h"

//#define _DEBUG

#ifdef _DEBUG
#define ITERATIONS 30
#define DISPLAY printf
#else
#define ITERATIONS 1000*1000*1000
#define DISPLAY(...)
#endif

#define UART_CLKS_PER_BIT 1
#include "uart.cc" //algorithm implementation
#ifdef CFLEX_VERILATOR
#include "VM_uart_tx.h"
#include "VM_uart_tx__Syms.h"
VM_uart_tx *top = new VM_uart_tx;
#endif

#ifdef CFLEX_NO_COROUTINES
#error this requires coroutines, CFLEX_NO_COROUTINES should be defined
#endif

int main()
{
  uint1_t tx_pin, tx_pin2, out_valid2;
  int32 clk_count = 0;

  clock_t start_time = clock();
  uint8_t data = 'a';

#ifdef CFLEX_VERILATOR
  top->in_run = 0;
  top->clock = 0; top->eval();
  top->clock = 1; top->eval();
  top->in_run = 1;
  top->in_data = data;
#endif

  MODULE_TYPE m = uart_tx(data, tx_pin2, out_valid2, clk_count);
  for(int i = 0; i < ITERATIONS; ++i)
  {
#ifdef CFLEX_VERILATOR
    top->in_clock_counter = clk_count;
	while(!top->out_done)
	{
      //DISPLAY("waiting...\n");
      top->clock = 0; top->eval();
      top->clock = 1; top->eval();
      tx_pin = top->out_tx_pin;
      if(top->out_out_valid)
        break;
    }
#endif

    bool cdone = !m.clock();

#ifdef CFLEX_VERILATOR
    top->clock = 0; top->eval();
    top->clock = 1; top->eval();

    if(top->out_done)
#else
    if(cdone)
#endif
    {
      DISPLAY("Simulated data sent 0x%02X\n", data);
      data = data == 'z' ? 'a' : data+1;

      module_update(m, uart_tx(data, tx_pin2, out_valid2, clk_count));

#ifdef CFLEX_VERILATOR
      top->in_run = 0;
      top->clock = 0; top->eval();
      top->clock = 1; top->eval();
      top->in_run = 1;
      top->in_data = data;
#endif
    }
    else
    {
#ifdef CFLEX_VERILATOR
      DISPLAY("Sequence %06d, TX pin (Verilator): %d, TX pin (CflexHDL): %d\n", clk_count, tx_pin, tx_pin2);
      if(tx_pin != tx_pin2)
      {
        fprintf(stderr, "ERROR: cosimulation do not match\n");
        break;
      }
#else
      DISPLAY("Sequence %06d, TX pin (CflexHDL): %d\n", clk_count, tx_pin2);
#endif
      ++clk_count;
    }
  }

  clock_t dt = clock()-start_time;
#ifdef CFLEX_VERILATOR
  delete top;
#endif

  printf("SIMULATION RESULTS: time %.2fs, clock frequency %0.f MHz\n", float(dt)/CLOCKS_PER_SEC, clk_count*1e-6*CLOCKS_PER_SEC/dt);
  return 0;
}

