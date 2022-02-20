//Led glowing demo, can be run on a FPGA board or simulated by C compilation
//
//(C) 2022 Victor Suarez Rovere <suarezvictor@gmail.com>
//inspired in https://www.fpga4fun.com/Opto0.html

#include "cflexhdl.h"

void led_glow(uint1_t& led)
{
  union {
    uint25_t counter; //counter cam be accesed as raw or by bitfields
    struct { uint25_t :16; uint25_t mid:8; uint25_t msb:1; } counter_bits;
  };

  union {
    uint9_t PWM;
    struct { uint8_t lsb:8; uint8_t msb:1; } PWM_bits;
  };

  uint8_t intensity;

  counter = 0;
  while(always())
  {
    intensity = counter_bits.msb ? counter_bits.mid : ~counter_bits.mid;
    PWM = PWM_bits.lsb + intensity;

    led = PWM_bits.msb;
    counter = counter + 1;
  }
}


