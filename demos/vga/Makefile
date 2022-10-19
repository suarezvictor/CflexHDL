VGA_SRC?=vga_flyover3d.cc
MIGEN_VGA?=pergola_vga.py
GENERATOR?=../../src/silice_generator.py
PARSER=../../cflexparser/clangparser.so
MIGEN2C=../../migen2cflexhdl/migen2cflexhdl.py
BUILD?=build
BOARD?=arty
INCLUDE+=-I../../include -I../../src/ -I`pwd`
CFLAGS+=-O3 $(INCLUDE) -DCFLEX_NO_COROUTINES
#CFLAGS+=-O3 $(INCLUDE) -fcoroutines

CLANGFLAGS+=-Wno-deprecated -Wno-deprecated-experimental-coroutine -stdlib=libc++ -std=c++20 
SDL_CFLAGS=$(shell sdl2-config --cflags)
SDL_LIBS=$(shell sdl2-config --libs)
XILINXTOOLCHAIN?=vivado #for yosys+nextpnr make sure fasm2frames.py is on PATH (and on front of PATH...)

all: run

run: compile
	$(BUILD)/compiled

compile: $(BUILD)/compiled

$(BUILD)/compiled_simulator.cpp: $(VGA_SRC)
	mkdir -p $(BUILD)
	cat $(VGA_SRC) > $(BUILD)/compiled_simulator.cpp

$(BUILD)/compiled: $(VGA_SRC) simulator_main.cpp $(BUILD)/compiled_simulator.cpp
	g++ -DCFLEX_SIMULATION $(CFLAGS) $(SDL_CFLAGS) simulator_main.cpp ../../src/sim_fb.c $(SDL_LIBS) -o $(BUILD)/compiled
	#clang++ -c -DCFLEX_SIMULATION $(CLANGFLAGS) $(CFLAGS) $(SDL_CFLAGS) simulator_main.cpp -o $(BUILD)/compiled.o
	#clang++ -c -DCFLEX_SIMULATION $(CLANGFLAGS) $(CFLAGS) $(SDL_CFLAGS) ../../src/sim_fb.c -o $(BUILD)/sim_fb.o
	
	#clang++ $(BUILD)/sim_fb.o $(BUILD)/compiled.o -o $(BUILD)/compiled $(SDL_LIBS)

verilog: $(BUILD)/top_instance.v

$(BUILD)/migen_vga.cc: migen/*.py $(MIGEN2C)
	mkdir -p $(BUILD)
	PYTHONPATH=../../migen2cflexhdl MIGEN2C_TARGET= python3 migen/$(MIGEN_VGA) > $(BUILD)/migen_vga.cc
	#cp $(BUILD)/migen_vga.cc migen/$(MIGEN_VGA).cc
	echo "#include \"vga_config.h\"" >> $(BUILD)/migen_vga.cc

$(BUILD)/top_instance.ice: $(VGA_SRC) ../../src/vga.cc $(BUILD)/div16.ice $(PARSER)
	cpp $(INCLUDE) -E -include $(VGA_SRC) ../../src/vga.cc -o $(BUILD)/top_instance.cpp
	cat $(BUILD)/div16.ice > $(BUILD)/top_instance.ice #FIXME: forced include
	$(GENERATOR) $(BUILD)/top_instance.cpp $(CFLAGS)>> $(BUILD)/top_instance.ice
	cat top_glue.ice >> $(BUILD)/top_instance.ice
	
$(BUILD)/top_instance.v: $(BUILD)/top_instance.ice 
	silice -f ../../external/silice/bare.v $(BUILD)/top_instance.ice -o $(BUILD)/top_instance.v --export vga_demo

$(BUILD)/div16.ice: div16.cc
	mkdir -p $(BUILD)
	cpp $(INCLUDE) -E div16.cc > $(BUILD)/div16.cc
	$(GENERATOR) $(BUILD)/div16.cc $(INCLUDE)> $(BUILD)/div16.ice

bitstream: $(BOARD)

de0nano: $(BUILD)/terasic_de0nano/gateware/terasic_de0nano.rbf

arty: $(BUILD)/digilent_arty/gateware/digilent_arty.bit

$(BUILD)/terasic_de0nano/gateware/terasic_de0nano.rbf: $(BUILD)/top_instance.v litex_soc.py
	python3 litex_soc.py $(BOARD) --cpu-type=None
	@echo openFPGALoader -b $(BOARD) $@ > $(BUILD)/load.sh

$(BUILD)/digilent_arty/gateware/digilent_arty.bit: $(BUILD)/top_instance.v litex_soc.py
	python3 litex_soc.py $(BOARD) --cpu-type=None $(XILINXTOOLCHAIN)
	@echo openFPGALoader -b $(BOARD) $@ > $(BUILD)/load.sh

load: bitstream
	sh $(BUILD)/load.sh

verilator: $(BUILD)/VM_vga_demo
	$(BUILD)/VM_vga_demo

$(BUILD)/VM_vga_demo: $(BUILD)/top_instance.v simulator_main.cpp $(BUILD)/compiled_simulator.cpp
	verilator --Mdir $(BUILD) -Wno-PINMISSING -Wno-WIDTH -O2 -cc $(BUILD)/top_instance.v --top-module M_vga_demo --exe simulator_main.cpp ../../src/sim_fb.c -CFLAGS -DCFLEX_VERILATOR -CFLAGS -I`pwd` -CFLAGS -I../../../include -CFLAGS -I../../../src/ -CFLAGS $(shell sdl2-config --cflags) -LDFLAGS $(shell sdl2-config --libs)
	cd build && make OPT_FAST="-O3" -f VM_vga_demo.mk

$(PARSER):
	cd ../../cflexparser && make

clean:
	rm -Rf build


