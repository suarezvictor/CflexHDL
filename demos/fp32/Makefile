SHELL=bash
MODULE_SRC?=mandel_fp32.cc
GENERATOR?=../../src/silice_generator.py
PARSER=../../cflexparser/clangparser.py
BUILD?=build
INCLUDE+=-I../../include -I../../src/ -I`pwd`
CFLAGS+=-O3 -flto $(INCLUDE) #-DCFLEX_NO_COROUTINES
GCCFLAGS=-std=c++20 -fcoroutines
CLANGFLAGS+=-DCFLEX_NO_COROUTINES #clang does not work with coroutines that finish
SIMULATOR_MAIN=simulator.cpp
VERILATOR_OPT_FAST?=-O3

#select FP32 core to test
#FP32?=_fp32_div
FP32?=_mandel

all: run

run: compile
	$(BUILD)/compiled

compile: $(BUILD)/compiled

$(BUILD)/compiled: $(MODULE_SRC) $(SIMULATOR_MAIN)
	mkdir -p $(BUILD)
	#g++ -DCFLEX_SIMULATION $(CFLAGS) $(GCCFLAGS) $(SIMULATOR_MAIN) -o $(BUILD)/compiled 
	clang++-17 -DCFLEX_SIMULATION -D$(FP32)=_fp32 $(CFLAGS) $(CLANGFLAGS) $(SIMULATOR_MAIN) -o $(BUILD)/compiled #clang is faster
	#objdump -D build/compiled > build/compiled.S

test:
	g++ -I../../include -o fp32 -D$(FP32)=_fp32 $(MODULE_SRC)
	./fp32

verilog: $(BUILD)/top_instance.v

$(BUILD)/top_instance.cpp: $(MODULE_SRC)
	mkdir -p $(BUILD)
	cpp $(INCLUDE) -DCFLEX_PARSER -D$(FP32)=_fp32 -E $(MODULE_SRC) -o $(BUILD)/top_instance.cpp
	
$(BUILD)/top_instance.si: $(BUILD)/top_instance.cpp $(PARSER)
	$(GENERATOR) $(BUILD)/top_instance.cpp $(CFLAGS) > $(BUILD)/top_instance.si

$(BUILD)/top_instance.v: $(BUILD)/top_instance.si
	silice -f ../../external/silice/bare.v $(BUILD)/top_instance.si -o $(BUILD)/top_instance.v --export fp32

verilator: $(BUILD)/VM_fp32
	$(BUILD)/VM_fp32

$(BUILD)/VM_fp32: $(BUILD)/top_instance.v $(SIMULATOR_MAIN)
	verilator --Mdir $(BUILD) -Wno-PINMISSING -Wno-WIDTH -O2 -cc $(BUILD)/top_instance.v \
	  --top-module M_fp32 --exe $(SIMULATOR_MAIN) -CFLAGS -DCFLEX_VERILATOR \
	  -CFLAGS -D$(FP32)=_fp32 \
	  -CFLAGS -I`pwd` -CFLAGS -I../../../include -CFLAGS -I../../../src/ \
	  -CFLAGS -DCFLEX_SIMULATION -CFLAGS -DCFLEX_NO_COROUTINES #-CFLAGS -fcoroutines

	  
	cd build && make OPT_FAST=$(VERILATOR_OPT_FAST) -f VM_fp32.mk #CXX=clang++-17 gcc is faster

clean:
	rm -Rf build


