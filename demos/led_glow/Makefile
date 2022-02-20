GENERATOR?=../../src/silice_generator.py
PARSER=../../cflexparser/clangparser.so
BUILD?=build
BOARD?=de0nano
INCLUDE+=-I../../include
CFLAGS+=-O2 $(INCLUDE)

all: run

run: compile
	@$(BUILD)/compiled

compile: $(BUILD)/compiled

$(BUILD)/compiled: led_glow.cc simulator_main.cpp
	@mkdir -p $(BUILD)
	@$(CXX) $(CFLAGS) simulator_main.cpp -o $(BUILD)/compiled

bitstream: $(BUILD)/top.rbf

$(BUILD)/top_instance.v: led_glow.cc $(PARSER)
	mkdir -p $(BUILD)
	cpp $(CFLAGS) -E led_glow.cc > $(BUILD)/top_instance.cpp
	$(GENERATOR) $(BUILD)/top_instance.cpp > $(BUILD)/top_instance.ice
	silice -f ../../external/silice/bare.v $(BUILD)/top_instance.ice -o $(BUILD)/top_instance.v --export led_glow

$(BUILD)/top.rbf: $(BUILD)/top_instance.v soc.py
	python3 soc.py

load: bitstream
	openFPGALoader -b $(BOARD) $(BUILD)/top.rbf

$(PARSER):
	cd ../../cflexparser && make

clean:
	rm -Rf build


