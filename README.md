# CflexHDL
Design digital circuits in C. Simulate really fast with a regular compiler! <br><br>
![image](https://user-images.githubusercontent.com/8551129/154831058-58d46e66-95ee-456f-86af-d6b71917de36.png)

So can it run algorithms without a CPU? Yes, the algorithm gets implemented as hardware, with gates interconnected to match the C code logic. Complex algorithms are possible like rendering graphics as demoed.
# TL;DR
See the [CflexHDL slides](https://speakerdeck.com/suarezvictor/cflexhdl)

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

`$ make load` synths and loads the bitstream on the Arty board with a VGA PMOD on JB-JC. You should be able to see your PC and FPGA both running at 60 FPS*, side to side!!!<br><br>
For the DE0-Nano, use `$ make BOARD=de0nano bitstream load`, the board outputs DVI signals at LVDS levels (use a simple capacitor coupling)
<br><br>
*_To limit FPS, set vsync to true when calling fb_init on simulator_main.cpp_
