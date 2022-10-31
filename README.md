# CflexHDL
Design digital circuits in C. Simulate really fast with a regular compiler! <br><br>
![image](https://user-images.githubusercontent.com/8551129/154831058-58d46e66-95ee-456f-86af-d6b71917de36.png)

**Q**: So can it run algorithms without a CPU?<br>
**A**: Yes, the algorithm gets implemented as hardware, with gates interconnected to match the C code logic. Complex algorithms are possible like rendering graphics as demoed.<br>
**Q**: Is the simulation that fast?<br>
**A**: Well, the fastest logic simulator is Verilator, and after some tests converting existing logic cores writen in verilog or migen to CflexHDL (unsing a provided automatic tool), speed gains were 2.5X to 5X, and up to 10X in some cases, compared with the same cores simulated with Verilator (a few tests, but in all cases so far). See [DEMOS.md](demos/DEMOS.md) or this [video](https://youtu.be/QS_XVe824Ck).

# TL;DR
See the [CflexHDL slides](https://suarezvictor.github.io/cflexhdl/slides.html)

# Quickstart
Install the minimal dependencies including GCC, Python's libclang, Sylefeb's Silice and Verilator. SDL library for graphical simulations. For synthesis on actual hardware, Vivado or Yosys+NextPNR or Quartus, and OpenFPGALoader (currently supports the Arty A7 board and the Terasic DE0-Nano)

# Led glow demo
`$ cd demos/led_glow && make` should print simulation results <br>
`$ make load` synths with default toolchain and loads the bitstream. First time of running the parser needs to be compiled and takes some extra time, just once! <br><br>
Synth options: <br>
`$ make BOARD=de0nano load` overrides the default board (the Arty)<br>
`$ make XILINXTOOLCHAIN=yosys+nextpnr load` overrides the default toolchain for the Arty (Vivado)

# Graphical demo

`$ cd demos/vga && make` should bring a window that renders graphics at high FPS (on your PC), and print the FPS on closing.

![image](https://user-images.githubusercontent.com/8551129/154829656-1e1e916e-e1dd-460c-805a-50c46dd325b7.png)

`$ make verilator` should bring the same window but at slower FPS

`$ make load` synths and loads the bitstream on the Arty board with a VGA PMOD on JB-JC. You should be able to see your PC and FPGA both running at 60 FPS*, side to side (the blurring is proof that the image was moving when taking the picture):<br>

![image](doc/laptop%2BFPGA%203d%20demo.jpeg)

See it in action!
https://youtu.be/TqV9wUDEG2o

<br><br>
For using yosys+nextpnr toolchain on the Digilent Arty board, use `XILINXTOOLCHAIN=yosys+nextpnr` in make<br>
For the DE0-Nano board, use `$ make BOARD=de0nano bitstream load`, the board outputs DVI signals at LVDS levels (use a simple capacitor coupling)
<br><br>
*_To limit FPS, set vsync to true when calling fb_init on simulator_main.cpp_

# Benchmarks
See [DEMOS](demos/DEMOS.md) page
