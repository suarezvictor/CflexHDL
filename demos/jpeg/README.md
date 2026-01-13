# Hardware accelerated 2D Inverse Discrete Cosine Transform

In this example a hardware accelerated 2D Inverse DCT was implemented as a C function. It can be run on the CPU or the code be translated unmodified to Verilog, for faster operation.


## Performance

Computing the 2D 8x8 IDCT takes **4618 cycles** if run as software, and just **393 cycles** with the hardware accelerator core (a tenfold increase).  
When the CPU controls the core, it is barely involved in calculations and data movements, so the accelerator can save resources by allowing a less complex CPU in the design: in the case of using a VexRiscV "standard" core vs. a "lite" core, the performance of the software-only implementation drops by 5X, while the accelerator performance just drops 4% with the less capable CPU.


## Implementation details

See the main accelerator module: [idct_kernel.cc](idct_kernel.cc)

It implements two altenatives, a [Loeffler IDCT](https://www.semanticscholar.org/paper/Practical-fast-1-D-DCT-algorithms-with-11-Loeffler-Ligtenberg/6134d65dc1d01db1e3c4be6f675763a469a973f6) (11 multipliers, provably minimal) and [AA&N IDCT](https://www.semanticscholar.org/paper/A-Fast-DCT-SQ-Scheme-for-Images-Arai-Agui/cedcee19124c6d29238f781023e617aae1e8fe80) (5 multipliers) which assumes that the inputs are premultiplied by the required coefficients (the factors are absorbed in the JPEG quantization coefficients).

A 2D IDCT implementation usually exploits its separability property: it runs eight 1D IDCT over columns, then eight 1D IDCT over the rows. In this implementation, eight instances of the 1D core are run in parallel, first with the input data, then again with the resulting data of the previous step. With this bare architecture 2X speed improvement was achieved, but the factor is not large considering that the math computations takes just a few cycles.  
  
It was then discovered that the bottleneck was the data movement: it's too slow for the CPU to read 64 16-bit words from RAM just to feed the accelerator's inputs. So the core was improved to access memory by DMA, which was optimized to get 32 bits per cycle using the burst mode of the Wishbone bus. This required to access a SRAM memory (mapped in the stack of the SOC), and data arrays very well aligned.  
  
Another improvement was to avoid involving the the CPU to send the results of the first stage of IDCT to the second: in this case, all the required 1024-bits are copied in a single cycle by the accelerator.
Another optimization was to tweak the size of the intermediate computation registers in the C source, using 16-bit values whenever possible. This allowed to implement the accelerator with 8 instances of the IDCT plus the CPU using 60 multipliers, which was able to fit the testing FPGA device (an Lattice ECP5 45F which has 72 multipliers).

A further improvement was to reduce amount of data in the 2D pass of the IDCT: since the resulting data is byte-sized, it was packed in just two 32-bit values.

See [videocodecs.py](videocodecs.py) for the implementation details.

Benchmark code was also added, to know the effect of each change in the optimization strategies, see [main.cpp](main.cpp)

## Integration

It was integrated and tested with two JPEG decoding libraries: Ultraembedded [core_jpeg](https://github.com/ultraembedded/core_jpeg/tree/main/c_model) and [JPEGDEC](https://github.com/bitbank2/JPEGDEC/), the latter being selected faster and funded by Nlnet. The Ultraembedded's was selected since it shows how this transpiler tool can really speed up development workflows: the original project required a "C model" of the algorithms to test correctness, then they were manually translated to Verilog, being the task **tedious**, **error-prone** and **hard to mantain**, all typical problems that this project solves.  

The 
mMain sources that required changes to adequately integrate this accelerator core are: [jpeg.inl](../../jpegdec/src/jpeg.inl) for the JPEGDEC case, and [c_model_jpeg_test.cpp](../../ultraembedded_jpeg/c_model/c_model_jpeg_test.cpp) for the Ultraembedded case.

## Building

    git switch jpeg
    make clean prerequisistes
    make

The BOARD environment variable can be set to select the hardware target instead of the simulator.  
  
