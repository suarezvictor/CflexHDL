// SPDX-License-Identifier: AGPL-3.0-only
//(C) 2026 Victor Suarez Rovere <suarezvictor@gmail.com>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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


void JPEGIDCT_internal_block(short src[64], uint8_t out[64], uint8_t cols, bool soft);

#ifdef JPEGDEC_OTHER_PLATFORM
#include "../../jpegdec/src/JPEGDEC.cpp"
#endif

//TODO; use function pointer to select software or hardware implementation
void JPEGIDCT_internal_block(short src[64], uint8_t out[64], uint8_t cols, bool soft)
{
    cols |= 1; // column 0 must always be calculated
    if(soft)
    {
		short tmp[64];
		const short *psrc = src;
		short *ptmp = tmp;
		for (int iCol = 0; iCol < 8; iCol++)
		{
		    _idct_kernel(
		    	psrc[0*8],psrc[1*8],psrc[2*8],psrc[3*8],psrc[4*8],psrc[5*8],psrc[6*8],psrc[7*8],
				ptmp[0*8],ptmp[1*8],ptmp[2*8],ptmp[3*8],ptmp[4*8],ptmp[5*8],ptmp[6*8],ptmp[7*8],
				1
			);
			++psrc;
			++ptmp;
	    }
		for (int iRow=0; iRow<64; iRow+=8)
		{
			const short *psrc = &tmp[iRow];
			uint8_t *pout = &out[iRow];
			short o[8];
			_idct_kernel(psrc[0],psrc[1], psrc[2], psrc[3], psrc[4], psrc[5], psrc[6], psrc[7],
				o[0], o[1],	o[2], o[3], o[4], o[5], o[6], o[7], 0
			);
			pout[0] = o[0]; pout[1] = o[1]; pout[2] = o[2]; pout[3] = o[3];
			pout[4] = o[4]; pout[5] = o[5]; pout[6] = o[6]; pout[7] = o[7];
		}
		return;
	}

#ifdef CSR_WBREAD1024_BASE
	{
#ifndef JPEGDEC_MCU_ALIGN
#error JPEGDEC_MCU_ALIGN shold be defined
#endif
		uintptr_t adr = uintptr_t(&src[0]); //already aligned
		assert(!(adr & (JPEGDEC_MCU_ALIGN-1)));

		//this takes 146 cycles with bus bursting enabled and source in SRAM
		wbread1024_base_addr_write(adr);
		wbread1024_start_write(1);
		while(!wbread1024_done_read());
		wbread1024_start_write(0);

		idct_kernel_remap_ctrl_write(2); //set DMA read data as input
	}
#else
#error this is deprecated
    volatile uint32_t *basein = (volatile uint32_t *) CSR_IDCT_KERNEL_REMAP_MAP_DIN0_0_ADDR;
    //this takes 362 cycles is source is in SRAM

    for (int iCol = 0; iCol < 8; iCol++) //memcpy-like
    {
		basein[iCol*4+0] = uint16_t(src[iCol*8+0]) | (src[iCol*8+1] << 16);
		basein[iCol*4+1] = uint16_t(src[iCol*8+2]) | (src[iCol*8+3] << 16);
		basein[iCol*4+2] = uint16_t(src[iCol*8+4]) | (src[iCol*8+5] << 16);
		basein[iCol*4+3] = uint16_t(src[iCol*8+6]) | (src[iCol*8+7] << 16);
	}
#endif


    //this takes 47 cycles
    {
		idct_kernel_remap_is_y_write(1); //set is_y
		idct_kernel_remap_run_write(0xFF); //start all
		while(idct_kernel_remap_done_read() != 0xFF); //wait all
		idct_kernel_remap_run_write(0x00); //stops all

		//super fast copy of outputs to inputs in hardware
		idct_kernel_remap_ctrl_write(1);

		idct_kernel_remap_is_y_write(0); //unset is_y
		idct_kernel_remap_run_write(0xFF); //start all
		while(idct_kernel_remap_done_read() != 0xFF); //wait all
		idct_kernel_remap_run_write(0x00); //stops all
	}

    //this takes 286 cycles! (about half the rest)
    volatile uint32_t *baseout = (volatile uint32_t *) CSR_IDCT_KERNEL_REMAP_MAP_DOUT0_0_ADDR;

    for (int iRow=0, iCol=0; iRow<64; iRow+=8, ++iCol)
    {
		uint32_t o0123 = baseout[iCol*4+0];
		uint32_t o4567 = baseout[iCol*4+1];

		out[iRow+0] = o0123 >> 0;
		out[iRow+1] = o0123 >> 8;
		out[iRow+2] = o0123 >> 16;
		out[iRow+3] = o0123 >> 24;
		out[iRow+4] = o4567 >> 0;
		out[iRow+5] = o4567 >> 8;
		out[iRow+6] = o4567 >> 16;
		out[iRow+7] = o4567 >> 24;
    }
}

#ifdef JPEGDEC_OTHER_PLATFORM
void jpegdec_test(uint8_t *buf, size_t size, uint8_t *videobuf, unsigned stride)
{
	static JPEGDEC jpg;
#ifdef JPEGDEC_MCU_ALIGN	
	int16_t m[JPEGDEC_MCU_ALIGN+MCU6]; //keep in SRAM so DMA burst optimization works
	jpg._jpeg.sUnalignedMCUs = &m[0];
#endif
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

#define BENCHMARK_REPEATS 8 //try not to exceed CPU cache nor SRAM


#include <math.h>
bool idct_benchmark()
{
	uint64_t t;
    printf("Running IDCT benchmark...\n");

	//keep in SRAM for burst optimization to work (saves 64 cycles)
	short s[BENCHMARK_REPEATS][JPEGDEC_MCU_ALIGN]; 
	uint8_t r[BENCHMARK_REPEATS][JPEGDEC_MCU_ALIGN];

	for(int i = 0; i < BENCHMARK_REPEATS; ++i)
      for(int j = 0; j < 64; ++j)
          s[i][j] = uint16_t(-(i << 8)) | uint16_t(-j);

	int	soft_acc = 0;
	t = highres_ticks();
	typedef short (*ps_t)[sizeof(s[0])/sizeof(s[0][0])];
	ps_t ps = ps_t((uintptr_t(&s[0]) + JPEGDEC_MCU_ALIGN -1) &~ (JPEGDEC_MCU_ALIGN-1));
	for(int i = 0; i < BENCHMARK_REPEATS; ++i)
	{
	    JPEGIDCT_internal_block(ps[i], r[i], 0xFF, true);
	    asm volatile("" ::: "memory"); //barrier to avoid profiling distortions as cause of optimizations
    	soft_acc += r[i][1];
	}
    printf("Software time %lu clocks/block\n", long((highres_ticks() - t)/BENCHMARK_REPEATS));
    
	int	hard_acc = 0;
	t = highres_ticks();
	for(int i = 0; i < BENCHMARK_REPEATS; ++i)
	{
	    JPEGIDCT_internal_block(ps[i], r[i], 0xFF, false);
	    asm volatile("" ::: "memory"); //barrier to avoid profiling distortions as cause of optimizations
    	hard_acc += r[i][1];
	}
    printf("Hardware time %lu clocks/block, results %d (soft), %d (hard)\n",
      long((highres_ticks() - t)/BENCHMARK_REPEATS), soft_acc, hard_acc);
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

