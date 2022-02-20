# CFlexHDL
Design digital circuits in C. Simulate really fast with a regular compiler.

# Quickstart
Install the minimal dependencies including GCC, Silice and Verilator. SDL library for graphical simulations. For synthesis on actual hardware, Vivado or Yosys+NextPNR or Quartus, and OpenFPGALoader (currently supports the Arty A7 board and Terasic DE0-Nano)

# Led glow demo
`$ cd demos/led_glow && make` should print simulation results <br>
`$ make load` synths with default toolchain and loads the bitstream. First time of running the parser needs to be compiled and takes some extra time, just once! <br>
`$ make BOARD=de0nano load` overrides the default board (the Arty)

# Graphical demo
`$ cd demos/vga && make` should bring a window rendering graphics at high FPS, and print the FPS on closing.

![image](https://user-images.githubusercontent.com/8551129/154829656-1e1e916e-e1dd-460c-805a-50c46dd325b7.png)

`$ make verilator` should bring the same window but at slower FPS

`$ make load` synths and loads the bitstream on the Cyclone IV device, with monitors-compatible DVI output using LVDS signals and capacitor coupling
