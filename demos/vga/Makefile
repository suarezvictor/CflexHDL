GENERATOR?=../../src/silice_generator.py
PARSER=../../cflexparser/clangparser.so
BUILD?=build
BOARD?=arty
INCLUDE+=-I../../include -I../../src/
CFLAGS+=-O2 $(INCLUDE)

all: run

run: compile
	$(BUILD)/compiled

compile: $(BUILD)/compiled

$(BUILD)/compiled: vga_flyover3d.cc simulator_main.cpp
	mkdir -p $(BUILD)
	cpp -DCFLEX_SIMULATION $(INCLUDE) -E vga_flyover3d.cc > $(BUILD)/compiled_simulator.cpp
	g++ $(CFLAGS) $(shell sdl2-config --cflags) simulator_main.cpp ../../src/sim_fb.c $(shell sdl2-config --libs) -o $(BUILD)/compiled

verilog: $(BUILD)/top_instance.v

$(BUILD)/top_instance.v: vga_flyover3d.cc $(PARSER)
	mkdir -p $(BUILD)
	cpp $(INCLUDE) -E vga_flyover3d.cc > $(BUILD)/top_instance.cpp
	cp includes.ice $(BUILD)/top_instance.ice #FIXME: forced include
	$(GENERATOR) $(BUILD)/top_instance.cpp $(CFLAGS)>> $(BUILD)/top_instance.ice
	cat top_glue.ice >> $(BUILD)/top_instance.ice
	silice -f ../../external/silice/bare.v $(BUILD)/top_instance.ice -o $(BUILD)/top_instance.v --export vga_demo

bitstream: $(BOARD)

de0nano: $(BUILD)/terasic_de0nano/gateware/terasic_de0nano.rbf

arty: $(BUILD)/digilent_arty/gateware/digilent_arty.bit

$(BUILD)/terasic_de0nano/gateware/terasic_de0nano.rbf: $(BUILD)/top_instance.v litex_soc.py
	python3 litex_soc.py $(BOARD) --cpu-type=None
	@echo openFPGALoader -b $(BOARD) $@ > $(BUILD)/load.sh

$(BUILD)/digilent_arty/gateware/digilent_arty.bit: $(BUILD)/top_instance.v litex_soc.py
	python3 litex_soc.py $(BOARD) --cpu-type=None
	@echo openFPGALoader -b $(BOARD) $@ > $(BUILD)/load.sh

load: bitstream
	sh $(BUILD)/load.sh

verilator: $(BUILD)/VM_vga_demo
	$(BUILD)/VM_vga_demo

$(BUILD)/VM_vga_demo: $(BUILD)/top_instance.v simulator_main.cpp
	verilator --Mdir $(BUILD) -Wno-PINMISSING -Wno-WIDTH -O2 -cc $^ --top-module M_vga_demo --exe simulator_main.cpp ../../src/sim_fb.c -CFLAGS -DCFLEX_VERILATOR -CFLAGS -I../../../include -CFLAGS $(shell sdl2-config --cflags) -LDFLAGS $(shell sdl2-config --libs)
	cd build && make -f VM_vga_demo.mk

$(PARSER):
	cd ../../cflexparser && make

clean:
	rm -Rf build


