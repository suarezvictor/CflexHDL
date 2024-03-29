SHELL=bash
MODULE_SRC?=uart.cc
GENERATOR?=../../src/silice_generator.py
PARSER=../../cflexparser/clangparser.so
BUILD?=build
INCLUDE+=-I../../include -I../../src/ -I`pwd` 
CFLAGS+=-O3 -flto $(INCLUDE)
GCCFLAGS=-std=c++20 -fcoroutines
#GCCFLAGS=-DCFLEX_NO_COROUTINES
#CLANGFLAGS=-Wno-parentheses-equality -Wno-deprecated -Wno-deprecated-experimental-coroutine -std=c++20 -stdlib=libc++ -fcoroutines-ts
#CLANGFLAGS+=-I/media/vsuarez/elocaldata/SCRATCH/llvm-project/libcxx/include/
CLANGFLAGS+=-DCFLEX_NO_COROUTINES #clang does not work with coroutines that finish
VERILATOR_OPT_FAST?=-O3
#VERILATOR_OPT_FAST?=-ggdb
SIMULATOR_MAIN?=simulator_main.cpp

all: run

run: compile
	$(BUILD)/compiled

compile: $(BUILD)/compiled

$(BUILD)/compiled: $(MODULE_SRC) $(SIMULATOR_MAIN)
	mkdir -p $(BUILD)
	g++ -DCFLEX_SIMULATION $(CFLAGS) $(GCCFLAGS) $(SIMULATOR_MAIN) -o $(BUILD)/compiled 
	#clang++-14 -DCFLEX_SIMULATION $(CFLAGS) $(CLANGFLAGS) $(SIMULATOR_MAIN) -o $(BUILD)/compiled #clang is faster

verilog: $(BUILD)/top_instance.v

$(BUILD)/top_instance.cpp: $(MODULE_SRC)
	mkdir -p $(BUILD)
	cpp $(INCLUDE) -E $(MODULE_SRC) -o $(BUILD)/top_instance.cpp
	
$(BUILD)/top_instance.ice: $(BUILD)/top_instance.cpp $(PARSER)
	$(GENERATOR) $(BUILD)/top_instance.cpp $(CFLAGS) > $(BUILD)/top_instance.ice

$(BUILD)/top_instance.v: $(BUILD)/top_instance.ice 
	silice -f ../../external/silice/bare.v $(BUILD)/top_instance.ice -o $(BUILD)/top_instance.v --export uart_tx

verilator: $(BUILD)/VM_uart_tx
	$(BUILD)/VM_uart_tx

$(BUILD)/VM_uart_tx: $(BUILD)/top_instance.v $(SIMULATOR_MAIN)
	verilator --Mdir $(BUILD) -Wno-PINMISSING -Wno-WIDTH -O2 -cc $(BUILD)/top_instance.v \
	  --top-module M_uart_tx --exe $(SIMULATOR_MAIN) -CFLAGS -DCFLEX_VERILATOR -CFLAGS -DCFLEX_SIMULATION \
	  -CFLAGS -fcoroutines \
	  -CFLAGS -I`pwd` -CFLAGS -I../../../include -CFLAGS -I../../../src/ 
	cd build && make OPT_FAST=$(VERILATOR_OPT_FAST) -f VM_uart_tx.mk

$(PARSER):
	cd ../../cflexparser && make

clean:
	rm -Rf build


