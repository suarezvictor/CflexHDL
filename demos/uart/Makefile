MODULE_SRC?=uart.cc
GENERATOR?=../../src/silice_generator.py
PARSER=../../cflexparser/clangparser.so
BUILD?=build
CFLAGS+=-O3 $(INCLUDE) -DCFLEX_NO_COROUTINES
INCLUDE+=-I../../include -I../../src/ -I`pwd`
#CFLAGS+=-O3 $(INCLUDE) -fcoroutines
#CLANGFLAGS+=-Wno-deprecated -Wno-deprecated-experimental-coroutine -stdlib=libc++ -std=c++20 

all: run

run: compile
	$(BUILD)/compiled

compile: $(BUILD)/compiled

$(BUILD)/compiled: $(MODULE_SRC) simulator_main.cpp
	g++ -DCFLEX_SIMULATION $(CFLAGS) simulator_main.cpp -o $(BUILD)/compiled

verilog: $(BUILD)/top_instance.v

$(BUILD)/top_instance.cpp: $(MODULE_SRC)
	mkdir -p $(BUILD)
	cpp $(INCLUDE) -E $(MODULE_SRC) -o $(BUILD)/top_instance.cpp
	
$(BUILD)/top_instance.ice: $(BUILD)/top_instance.cpp $(PARSER)
	$(GENERATOR) $(BUILD)/top_instance.cpp $(CFLAGS) > $(BUILD)/top_instance.ice
	
$(BUILD)/top_instance.v: $(BUILD)/top_instance.ice 
	silice -f ../../external/silice/bare.v $(BUILD)/top_instance.ice -o $(BUILD)/top_instance.v --export uart_tx

verilator: $(BUILD)/VM_uart
	$(BUILD)/VM_uart_tx

$(BUILD)/VM_uart_tx: $(BUILD)/top_instance.v simulator_main.cpp $(BUILD)/compiled_simulator.cpp
	verilator --Mdir $(BUILD) -Wno-PINMISSING -Wno-WIDTH -O2 -cc $(BUILD)/top_instance.v --top-module M_uart_tx --exe simulator_main.cpp -CFLAGS -DCFLEX_VERILATOR -CFLAGS -I`pwd` -CFLAGS -I../../../include -CFLAGS -I../../../src/ 
	cd build && make OPT_FAST="-O3" -f VM_uart_tx.mk

$(PARSER):
	cd ../../cflexparser && make

clean:
	rm -Rf build


