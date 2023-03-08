//UART simulator
//Copyright (C) 2023 Victor Suarez Rovere <suarezvictor@gmail.com>

#include <stdio.h>
#include <time.h>

#include "cflexhdl.h"

#define _DEBUG

#ifdef _DEBUG
#define ITERATIONS 30
#define DISPLAY printf
#else
#define ITERATIONS 100*1000*1000
#define DISPLAY(...)
#endif

#define UART_CLKS_PER_BIT 1
#include "uart.cc" //algorithm implementation
#include "VM_uart_tx.h"
#include "VM_uart_tx__Syms.h"

VM_uart_tx *top = new VM_uart_tx;

#ifdef CFLEX_NO_COROUTINES
#error this requires coroutines, CFLEX_NO_COROUTINES should be defined
#endif

int main()
{
  uint1_t tx_pin, tx_pin2, out_valid2;
  int32 clk_count = 0;

  clock_t start_time = clock();
  uint8_t data = 'a';

  top->in_run = 0;
  top->clock = 0; top->eval();
  top->clock = 1; top->eval();
  top->in_run = 1;
  top->in_data = data;


  MODULE_TYPE *m = new MODULE_TYPE(uart_tx(data, tx_pin2, out_valid2, clk_count));
  for(int i = 0; i < ITERATIONS; ++i)
  {
  
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

    m->clock();
    top->clock = 0; top->eval();
    top->clock = 1; top->eval();

    if(top->out_done)
    {
      DISPLAY("Simulated data sent 0x%02X\n", data);
      data = data == 'z' ? 'a' : data+1;

      delete m;
      m = new MODULE_TYPE(uart_tx(data, tx_pin2, out_valid2, clk_count));
      
      top->in_run = 0;
      top->clock = 0; top->eval();
      top->clock = 1; top->eval();
      top->in_run = 1;
      top->in_data = data;
    }
    else
    {
      DISPLAY("Sequence %06d, TX pin (Verilator): %d, TX pin (CflexHDL): %d\n", clk_count, tx_pin, tx_pin2);
      if(tx_pin != tx_pin2)
      {
        fprintf(stderr, "ERROR: cosimulation do not match\n");
        break;
      }
      ++clk_count;
    }
  }

  clock_t dt = clock()-start_time;
  delete top;

  printf("SIMULATION RESULTS: clock frequency %0.f MHz\n", clk_count*1e-6*CLOCKS_PER_SEC/dt);
  return 0;
}

