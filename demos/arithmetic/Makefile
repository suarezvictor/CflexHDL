SHELL=bash
MODULE_SRC?=arithmetic.cc
GENERATOR?=../../src/silice_generator.py
PARSER=../../cflexparser/clangparser.so
BUILD?=build
INCLUDE+=-I../../include -I../../src/ -I`pwd`
CFLAGS+=-O3 -flto $(INCLUDE) #-DCFLEX_NO_COROUTINES
GCCFLAGS=-std=c++20 -fcoroutines
CLANGFLAGS+=-DCFLEX_NO_COROUTINES #clang does not work with coroutines that finish
SIMULATOR_MAIN=simulator.cpp
VERILATOR_OPT_FAST?=-O3

all: run

run: compile
	$(BUILD)/compiled

compile: $(BUILD)/compiled

$(BUILD)/compiled: $(MODULE_SRC) $(SIMULATOR_MAIN)
	mkdir -p $(BUILD)
	#g++ -DCFLEX_SIMULATION $(CFLAGS) $(GCCFLAGS) $(SIMULATOR_MAIN) -o $(BUILD)/compiled 
	clang++-14 -DCFLEX_SIMULATION $(CFLAGS) $(CLANGFLAGS) $(SIMULATOR_MAIN) -o $(BUILD)/compiled #clang is faster
	#objdump -D build/compiled > build/compiled.S

verilog: $(BUILD)/top_instance.v

$(BUILD)/top_instance.cpp: $(MODULE_SRC)
	mkdir -p $(BUILD)
	cpp $(INCLUDE) -E $(MODULE_SRC) -o $(BUILD)/top_instance.cpp
	
$(BUILD)/top_instance.ice: $(BUILD)/top_instance.cpp $(PARSER)
	$(GENERATOR) $(BUILD)/top_instance.cpp $(CFLAGS) > $(BUILD)/top_instance.ice

$(BUILD)/top_instance.v: $(BUILD)/top_instance.ice 
	silice -f ../../external/silice/bare.v $(BUILD)/top_instance.ice -o $(BUILD)/top_instance.v --export arithmetic

verilator: $(BUILD)/VM_arithmetic
	$(BUILD)/VM_arithmetic

$(BUILD)/VM_arithmetic: $(BUILD)/top_instance.v $(SIMULATOR_MAIN)
	verilator --Mdir $(BUILD) -Wno-PINMISSING -Wno-WIDTH -O2 -cc $(BUILD)/top_instance.v \
	  --top-module M_arithmetic --exe $(SIMULATOR_MAIN) -CFLAGS -DCFLEX_VERILATOR \
	  -CFLAGS -I`pwd` -CFLAGS -I../../../include -CFLAGS -I../../../src/ \
	  -CFLAGS -DCFLEX_SIMULATION -CFLAGS -DCFLEX_NO_COROUTINES #-CFLAGS -fcoroutines

	  
	cd build && make OPT_FAST=$(VERILATOR_OPT_FAST) -f VM_arithmetic.mk #CXX=clang++-14 gcc is faster

$(PARSER):
	cd ../../cflexparser && make

clean:
	rm -Rf build


