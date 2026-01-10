// SPDX-License-Identifier: AGPL-3.0-only
//(C) 2026 Victor Suarez Rovere <suarezvictor@gmail.com>

#include <stdio.h>
#include <stdlib.h>

#include <irq.h>
#include <uart.h>
#include <generated/csr.h>

#define JPEGDEC_OTHER_PLATFORM //use JPEGDEC library, note that accelerator should match this

extern "C" void isr_handler(void)
{
  __attribute__((unused)) unsigned int irqs;
  irqs = irq_pending() & irq_getmask();

#ifndef UART_POLLING
  if(irqs & (1 << UART_INTERRUPT))
    uart_isr();
#endif

#if defined(TIMER0_INTERRUPT) && !defined(TIMER0_POLLING)
	if(irqs & (1 << TIMER0_INTERRUPT))
	{
	}
#endif

}

#ifndef JPEGDEC_OTHER_PLATFORM
#error Ultraembedded's decoder only tested in past versions
#define dprintf(...)
//#define dprintf printf
#include "c_model/c_model_jpeg_test.cpp"
#endif

#ifndef IDCT_INSTANCE_COUNT
#error requires IDCT_INSTANCE_COUNT defined by Block IDCT accelerator
#endif

void _idct_kernel(
	short data_in_0,
	short data_in_1,
	short data_in_2,
	short data_in_3,
	short data_in_4,
	short data_in_5,
	short data_in_6,
	short data_in_7,
	short& data_out_0,
	short& data_out_1,
	short& data_out_2,
	short& data_out_3,
	short& data_out_4,
	short& data_out_5,
	short& data_out_6,
	short& data_out_7,
	short is_y
);

#ifdef CSR_TIMER0_UPTIME_CYCLES_ADDR
static inline uint64_t highres_ticks(void) { timer0_uptime_latch_write(1);  return timer0_uptime_cycles_read(); }
static inline uint64_t highres_ticks_freq(void) { return CONFIG_CLOCK_FREQUENCY; }
#endif

extern "C" char sample640x480_jpeg[];
extern "C" unsigned sample640x480_jpeg_len;


#ifdef JPEGDEC_OTHER_PLATFORM
#include "../../jpegdec/src/JPEGDEC.cpp"

void jpegdec_test(uint8_t *buf, size_t size, uint8_t *videobuf, unsigned stride)
{
    static JPEGDEC jpg;
    jpg.openRAM(buf, size, nullptr);
    jpg.setPixelType(RGB8888);
	jpg.setFramebuffer(videobuf);
	printf("jpegdec_test...\n");
    jpg.decode(0, 0, 0);
    jpg.close();
}
#endif

bool idct_benchmark();

void graphics_app()
{
    uint8_t *const video_buf = (uint8_t *) VIDEO_FRAMEBUFFER_BASE;
    const unsigned stride = VIDEO_FRAMEBUFFER_HRES*(VIDEO_FRAMEBUFFER_DEPTH/8);
	unsigned frame = 0;
	
	if(idct_benchmark())
      printf("Software and hardware results MATCH!\n");
    else
      printf("Software and hardware results DOES NOT match!\n");
	
	for(;;)
	{
#ifndef LITEX_SIMULATION
		memset(video_buf, 0x40, VIDEO_FRAMEBUFFER_VRES*stride);
#endif

	    jpegdec_test((uint8_t*) sample640x480_jpeg, sample640x480_jpeg_len, video_buf, stride);
		++frame;
	}
}

short s[64]; 
uint8_t r[64];

#include <math.h>
bool idct_benchmark()
{
#ifndef LITEX_SIMULATION
	const int REPEATS = 10*1000;
#else
	const int REPEATS = 100;
#endif
	uint64_t t;
    printf("Running IDCT benchmark...\n");

	int	soft_acc = 0;
	t = highres_ticks();
	for(int i = 0; i < REPEATS; ++i)
	{
	    JPEGIDCT_internal_block(s, r, 0xFF, true);
    	soft_acc += r[63];
	}
    printf("Software time %lu clocks/block\n", long((highres_ticks() - t)/REPEATS));

	int	hard_acc = 0;
	t = highres_ticks();
	for(int i = 0; i < REPEATS; ++i)
	{
	    JPEGIDCT_internal_block(s, r, 0xFF, false);
    	hard_acc += r[63];
	}
    printf("Hardware time %lu clocks/block\n", long((highres_ticks() - t)/REPEATS));

    return soft_acc == hard_acc;
}


typedef void (*func_ptr) (void);
extern func_ptr __init_array_start, __init_array_end;
extern func_ptr __fini_array_start, __fini_array_end;

static void _init_array(void)
{
  for (func_ptr *f = &__init_array_start; f != &__init_array_end; ++f)
    (*f)();
}
 
static void _fini_array(void)
{
  for (func_ptr *f = &__fini_array_start; f != &__fini_array_end; ++f)
    (*f)();
}

void graphics_app(void);

int main(int argc, char **argv)
{
  irq_setmask(0);
  irq_setie(1);

  uart_init();
  _init_array(); //call constructors and initializers
  void graphics_app(void);
  graphics_app();
  _fini_array();

  irq_setie(0);
  irq_setmask(~0);
    
  return 0;
}

// helpers -----------------------------------------------------------------------------------------
void _putchar(char c) { uart_write(c); } //this is to make printf work


//inlcude sofware implementation of the accelerator, to test matching with hardware
#define CFLEXHDL_SKIP_STDINT_DEFS
#include "idct_kernel.cc"

