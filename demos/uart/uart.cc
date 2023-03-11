#include "cflexhdl.h"

#ifndef UART_CLKS_PER_BIT
#define UART_CLKS_PER_BIT 1//0000
#endif

#define uart_tx _uart_tx

MODULE uart_tx(const uint8& data, uint1& tx_pin, uint1& out_valid, const int32& clock_counter)
{
#if 0
while(always()) { //clang 14 needs this to avoid internal compiler error
#endif

  int32 t1;
  t1 = clock_counter + UART_CLKS_PER_BIT;

  //---- start bit --------------------------------
  tx_pin = 0; out_valid = 1; //start bit

  while(always() && clock_counter - t1 < 0)
    out_valid = 0;

  t1 = t1 + UART_CLKS_PER_BIT;

  //---- data bits -------------------------------
  uint8 mask;
  for(mask = 1; mask != 0;  mask = mask << 1)
  {
    tx_pin = (data & mask) != 0; out_valid = 1;

    while(always() && clock_counter - t1 < 0)
      out_valid = 0;
    t1 = t1 + UART_CLKS_PER_BIT;
  }

  //---- stop bit -------------------------------
  tx_pin = 1; out_valid = 1;
  while(always() && clock_counter - t1 < 0)
    out_valid = 0;

#if 0
}
#endif
}

#if 0
uint8_t receive_byte()
{
  // Wait for start bit transition
  // First wait for UART_IDLE
  uint1_t the_bit = !UART_IDLE;
  while(the_bit != UART_IDLE)
  {
    the_bit = get_uart_input();
  }
  // Then wait for the start bit start
  while(the_bit != UART_START)
  {
    the_bit = get_uart_input();
  }

  // Wait for 1.5 bit periods to align to center of first data bit
  wait_clks(UART_CLKS_PER_BIT+UART_CLKS_PER_BIT_DIV2);
  
  // Loop sampling each data bit into 8b shift register
  uint8_t the_byte = 0;
  uint16_t i;
  for(i=0;i<8;i+=1)
  {
    // Read the wire
    the_bit = get_uart_input();
    // Shift buffer down to make room for next bit
    the_byte = the_byte >> 1;
    // Save sampled bit at top of shift reg [7]
    the_byte |= ( (uint8_t)the_bit << 7 );
    // And wait a full bit period so next bit is ready to receive next
    wait_clks(UART_CLKS_PER_BIT);
  }

  // Dont need to wait for stop bit
  return the_byte;
}
#endif
