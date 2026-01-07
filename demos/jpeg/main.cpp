#include <stdio.h>
#include <stdlib.h>

#include <irq.h>
#include <uart.h>
#include <generated/csr.h>


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

#define dprintf(...)
//#define dprintf printf
#include "c_model/c_model_jpeg_test.cpp"

static inline __attribute__((always_inline))
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
  idct_kernel_din0_write((int)data_in_0);
  idct_kernel_din1_write((int)data_in_1);
  idct_kernel_din2_write((int)data_in_2);
  idct_kernel_din3_write((int)data_in_3);
  idct_kernel_din4_write((int)data_in_4);
  idct_kernel_din5_write((int)data_in_5);
  idct_kernel_din6_write((int)data_in_6);
  idct_kernel_din7_write((int)data_in_7);
  idct_kernel_is_y_write(is_y);

  idct_kernel_run_write(1);
  while(!idct_kernel_done_read());

  data_out_0 = idct_kernel_dout0_read();
  data_out_1 = idct_kernel_dout1_read();
  data_out_2 = idct_kernel_dout2_read();
  data_out_3 = idct_kernel_dout3_read();
  data_out_4 = idct_kernel_dout4_read();
  data_out_5 = idct_kernel_dout5_read();
  data_out_6 = idct_kernel_dout6_read();
  data_out_7 = idct_kernel_dout7_read();

  idct_kernel_run_write(0);
}


void __attribute__ ((noinline)) //atribute is needed so compiler does not optimize constant inputs
_idct_kernel(
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

idct_kernel_t idct_kernel = nullptr;


#ifdef CSR_TIMER0_UPTIME_CYCLES_ADDR
static inline uint64_t highres_ticks(void) { timer0_uptime_latch_write(1);  return timer0_uptime_cycles_read(); }
static inline uint64_t highres_ticks_freq(void) { return CONFIG_CLOCK_FREQUENCY; }
#endif

extern "C" char sample640x480_jpeg[];
extern "C" unsigned sample640x480_jpeg_len;


bool idct_benchmark();

void graphics_app()
{
	uint8_t *const video_buf = (uint8_t *) VIDEO_FRAMEBUFFER_BASE;
	const unsigned stride = VIDEO_FRAMEBUFFER_HRES*(VIDEO_FRAMEBUFFER_DEPTH/8);

	unsigned frame = 0;
	
	if(idct_benchmark())
      printf("Results MATCH!\n");
	
	for(;;)
	{

		memset(video_buf, 0x40, VIDEO_FRAMEBUFFER_VRES*stride);
		bool hard = !(frame & 1);
		printf("JPEG %s IDCT decoding...\n", hard ? "hardware": "software");
		idct_kernel = hard ? accel_idct_kernel : _idct_kernel;
		ultraembedded_jpeg_decompress((uint8_t*) sample640x480_jpeg, sample640x480_jpeg_len, video_buf, stride);
	
		++frame;
	}
}


bool idct_benchmark()
{
#ifndef LITEX_SIMULATION
	const int REPEATS = 1000*1000;
#else
	const int REPEATS = 10*1000;
#endif
	uint64_t t;

    printf("Running IDCT benchmark...\n");
    short s[8]={1, 2, 3, 4, 5, 6, 7, 8}, r[8];

	int	soft_acc = 0;
	t = highres_ticks();
	for(int i = 0; i < REPEATS; ++i)
	{
    	_idct_kernel(
    	    s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7],
    		r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 0);
    	soft_acc += r[7];
	}
    printf("Software result: %d, time %lu clocks/pixel\n", soft_acc, long((highres_ticks() - t)/(REPEATS*8)));

	int	hard_acc = 0;
	t = highres_ticks();
	for(int i = 0; i < REPEATS; ++i)
	{
    	accel_idct_kernel(
    	    s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7],
    		r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 0);
    	hard_acc += r[7];
	}
    printf("Hardware result: %d, time %lu clocks/pixel\n", hard_acc, long((highres_ticks() - t)/(REPEATS*8)));

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

#define CFLEXHDL_SKIP_STDINT_DEFS
#include "idct_kernel.cc"

// helpers -----------------------------------------------------------------------------------------
void _putchar(char c) { uart_write(c); } //this is to make printf work

