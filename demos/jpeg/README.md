#Hardware accelerated 2D Inverse Discrete Cosine Transform

In this example a hardware accelerated 2D Inverse DCT was implemented as a C function. It can be run on the CPU or the unmodified code be translated unmodified to Verilog, for faster operation.


## Performance

Computing the 2D 8x8 IDCT takes **4618 cycles** if run as software, and just **393 cycles** with the hardware accelerator core (a tenfold increase).  
When the CPU controls the core, it is barely involved in calculations and data movements, so the accelerator can save resources by allowing a less complex CPU in the design: in the case of using a VexRiscV "standard" core vs. a "lite" core, the performance of the software-only implementation drops by 5X, while the accelerator performance just drops 4% with the less capable CPU.


## Implementation details

See the main accelerator module: [idct_kernel.cc](idct_kernel.cc)

## Building

    git switch jpeg
    make clean prerequisistes
    make