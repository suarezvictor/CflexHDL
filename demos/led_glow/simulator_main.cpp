//Counts cycles with LED in "on" state
//Copyright (C) 2022 Victor Suarez Rovere <suarezvictor@gmail.com>

#include <stdio.h>
#include <time.h>

#define ITERATIONS 1e8L
#define CFLEX_SIMULATION
#include "cflexhdl.h"

uint1_t led_state;
long on_count = 0;
long clk_count = 0;

inline void wait_clk()
{
    if(++clk_count >= ITERATIONS)
        throw false;
    if(led_state)
        ++on_count;
}

#define clk() ({wait_clk(); true;})
#include "led_glow.cc" //algorithm implementation

int main()
{
  timespec start_time, end_time;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);

  try { led_glow(led_state); } catch(...) {} //exits by exeption

  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
  long nanos = (end_time.tv_sec - start_time.tv_sec) * 1e9L + (end_time.tv_nsec - start_time.tv_nsec);
  printf("SIMULATION RESULTS: Average duty cycle: %d%%, clock frequency %d MHz\n", on_count/(clk_count/100), clk_count/(nanos/1000));

  return 0;
}

