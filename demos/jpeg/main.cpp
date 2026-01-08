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
#define dprintf(...)
//#define dprintf printf
#include "c_model/c_model_jpeg_test.cpp"
#endif

void accel_idct_kernel(
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
)
{
#ifdef IDCT_MERGE_IN_FIELDS
  idct_kernel_din01_write(uint16_t(data_in_0) | (data_in_1 << 16));
  idct_kernel_din23_write(uint16_t(data_in_2) | (data_in_3 << 16));
  idct_kernel_din45_write(uint16_t(data_in_4) | (data_in_5 << 16));
  idct_kernel_din67_write(uint16_t(data_in_6) | (data_in_7 << 16));
#else
  idct_kernel_din0_write((int)data_in_0);
  idct_kernel_din1_write((int)data_in_1);
  idct_kernel_din2_write((int)data_in_2);
  idct_kernel_din3_write((int)data_in_3);
  idct_kernel_din4_write((int)data_in_4);
  idct_kernel_din5_write((int)data_in_5);
  idct_kernel_din6_write((int)data_in_6);
  idct_kernel_din7_write((int)data_in_7);
#endif
  idct_kernel_is_y_write(is_y);

  idct_kernel_run_write(1);
  while(!idct_kernel_done_read());

#ifdef IDCT_MERGE_OUT_FIELDS
  uint32_t o01 = idct_kernel_dout01_read();
  uint32_t o23 = idct_kernel_dout23_read();
  uint32_t o45 = idct_kernel_dout45_read();
  uint32_t o67 = idct_kernel_dout67_read();

  data_out_0 = o01;
  data_out_1 = o01 >> 16;
  data_out_2 = o23;
  data_out_3 = o23 >> 16;
  data_out_4 = o45;
  data_out_5 = o45 >> 16;
  data_out_6 = o67;
  data_out_7 = o67 >> 16;
#else
  data_out_0 = idct_kernel_dout0_read();
  data_out_1 = idct_kernel_dout1_read();
  data_out_2 = idct_kernel_dout2_read();
  data_out_3 = idct_kernel_dout3_read();
  data_out_4 = idct_kernel_dout4_read();
  data_out_5 = idct_kernel_dout5_read();
  data_out_6 = idct_kernel_dout6_read();
  data_out_7 = idct_kernel_dout7_read();
#endif
  idct_kernel_run_write(0);
}

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
    jpg.decode(0, 0, 0);
    jpg.close();
}
#endif

bool idct_benchmark();

idct_kernel_t idct_kernel = nullptr;

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
		bool hard = !(frame & 1);
		printf("JPEG %s IDCT decoding...\n", hard ? "hardware": "software");
		idct_kernel = hard ? accel_idct_kernel : _idct_kernel;
#ifndef JPEGDEC_OTHER_PLATFORM		
		ultraembedded_jpeg_decompress((uint8_t*) sample640x480_jpeg, sample640x480_jpeg_len, video_buf, stride);
#else
	    jpegdec_test((uint8_t*) sample640x480_jpeg, sample640x480_jpeg_len, video_buf, stride);
#endif
	
		++frame;
	}
}

short s[8][8], r[8][8]; //keep global memory so the compiler cannot guess contents
#include <math.h>
bool idct_benchmark()
{
#ifndef LITEX_SIMULATION
	const int REPEATS = 100*1000;
#else
	const int REPEATS = 1000;
#endif
	uint64_t t;
    printf("Running IDCT benchmark...\n");

	int	soft_acc = 0;
	for(int i = 0; i < 8; ++i)
	  s[0][i] = 256*(i == 0);


#define ARGS(i)	s[i][0], s[i][1], s[i][2], s[i][3], s[i][4], s[i][5], s[i][6], s[i][7], \
    		r[i][0], r[i][1], r[i][2], r[i][3], r[i][4], r[i][5], r[i][6], r[i][7], 0

	t = highres_ticks();
	for(int i = 0; i < REPEATS; ++i)
	{
    	_idct_kernel(ARGS(0));
    	_idct_kernel(ARGS(1));
    	_idct_kernel(ARGS(2));
    	_idct_kernel(ARGS(3));
    	_idct_kernel(ARGS(4));
    	_idct_kernel(ARGS(5));
    	_idct_kernel(ARGS(6));
    	_idct_kernel(ARGS(7));
    	soft_acc += r[0][7];
	}
    printf("Software time %lu clocks/block\n", long((highres_ticks() - t)/REPEATS));

	int	hard_acc = 0;
	t = highres_ticks();
	for(int i = 0; i < REPEATS; ++i)
	{
    	accel_idct_kernel(ARGS(0));
    	accel_idct_kernel(ARGS(1));
    	accel_idct_kernel(ARGS(2));
    	accel_idct_kernel(ARGS(3));
    	accel_idct_kernel(ARGS(4));
    	accel_idct_kernel(ARGS(5));
    	accel_idct_kernel(ARGS(6));
    	accel_idct_kernel(ARGS(7));
    	hard_acc += r[0][7];
	}
    printf("Hardware time %lu clocks/block\n", long((highres_ticks() - t)/REPEATS));

    return soft_acc == hard_acc;
#undef ARGS    
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

