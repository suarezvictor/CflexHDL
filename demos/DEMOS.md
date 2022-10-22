#Led glow<br>
cd led_glow && make<br>
SIMULATION RESULTS: Average duty cycle: 49%, clock frequency 437 MHz<br>

#Graphics demos<br>
cd demos/vga<br>

**Pergola's ROTOZOOM:**<br>
![image](https://user-images.githubusercontent.com/8551129/196615024-e890c3fa-6fb1-49be-b37e-ab7f29763734.png)<br>

make VGA_SRC=build/migen_vga.cc clean all<br>
FPS 503.3, pixel clock 154.6 MHz (simulation)<br>
FPS 181.5, pixel clock 55.7 MHz (verilator)<br>

**Silice's FLYOVER 3D:**<br>
![image](https://user-images.githubusercontent.com/8551129/196614548-fd2ee539-0427-47b6-b9b3-d4dd9885be80.png)<br>

make clean all<br>
FPS 642.3, pixel clock 197.3 MHz (simulation)<br>
FPS 132.4, pixel clock 40.7 MHz (verilator)<br>

**LiteX's COLORBARS:**<br>
![image](https://user-images.githubusercontent.com/8551129/196614509-781472c2-bea5-4dcf-8512-76ab8a9d444d.png)<br>

make VGA_SRC=build/migen_vga.cc MIGEN_VGA=litex_vga.py clean all<br>
FPS 764.4, pixel clock 234.8 MHz (simulation)<br>
FPS 136.8, pixel clock 42.0 MHz (verilator)<br>

**Pergola's TestImageGenerator**<br>
![image](https://user-images.githubusercontent.com/8551129/197364164-a1b41146-107c-4a42-a2b3-8173451213f2.png)<br>
FPS 580.9, pixel clock 178.5 MHz (simulation)<br>
FPS 553.2, pixel clock 169.9 MHz (thru RTLIL)<br>
FPS 157.6, pixel clock 48.4 MHz (verilator)<br>

**Pergola's StaticTestImageGenerator**<br>
![image](https://user-images.githubusercontent.com/8551129/197365006-9b0ba714-ce9f-4c03-9dbe-d912abcf64de.png)<br>
FPS 1039.5, pixel clock 319.3 MHz (simulation)<br>
FPS 1033.1, pixel clock 317.4 MHz (thru RTLIL)<br>
FPS 200.4, pixel clock 61.6 MHz (verilator)<br>
