#TODO: see $(LITEX_ROOT)/litex/soc/software/demo/Makefile

BOARD?=sim
BUILD_DIR=./build/$(BOARD)
LITEX_ROOT=../../../litex
MAIN_SRC=idct_kernel
GENERATOR?=../../src/silice_generator.py

ifeq ($(filter prerequisites clean,$(MAKECMDGOALS)),)
include $(BUILD_DIR)/software/include/generated/variables.mak
endif

include $(LITEX_ROOT)/litex/soc/software/common.mak

INCLUDE = -I../../include -I../../ultraembedded_jpeg/
CXXFLAGS += $(INCLUDE) -O2 -fno-threadsafe-statics

all: $(BOARD)

%.o: %.S
	$(assemble)

%.o: %.c
	$(compile)

%.o: %.cpp
	$(compilexx)

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	chmod -x $@

firmware: $(BUILD_DIR)/main.bin

upload: $(BUILD_DIR)/main.bin
	$(LITEX_ROOT)/litex/tools/litex_term.py /dev/ttyUSB1 --kernel $(BUILD_DIR)/main.bin --speed 115200

$(BUILD_DIR)/main.elf: main.o crt0.o sample640x480.o
	$(CC) -T linker.ld -o $(BUILD_DIR)/main.elf crt0.o main.o sample640x480.o $(PACKAGES:%=-L$(BUILD_DIR)/software/%) -L$(BUILDINC_DIRECTORY) $(CPUFLAGS) -nostartfiles $(LIBS:lib%=-l%)
	
prerequisites: $(BOARD).py
	python $(BOARD).py --no-compile-gateware

sim: $(BUILD_DIR)/main.bin verilog
	VIDEO=1 $(PYTHON) sim.py --sdram-init $(BUILD_DIR)/main.bin

bitstream: $(BOARD)

verilog: $(BUILD_DIR)/../top_instance.v

$(BUILD_DIR)/top_instance.si: $(MAIN_SRC).cc
	cpp $(INCLUDE) -DCFLEX_PARSER $(MAIN_SRC).cc -E -o $(BUILD_DIR)/top_instance.cpp
	$(GENERATOR) $(BUILD_DIR)/top_instance.cpp > $(BUILD_DIR)/top_instance.si

$(BUILD_DIR)/../top_instance.v: $(BUILD_DIR)/top_instance.si
	silice -f ../../external/silice/bare.v $(BUILD_DIR)/top_instance.si -o $(BUILD_DIR)/../top_instance.v --export $(MAIN_SRC)

sample640x480.c: sample640x480.jpeg
	echo "//SOURCE IMAGE: Kirk Burn by Jim Barton https://www.geograph.org.uk/photo/903277" > $@
	echo "//License: CC BY-SA 2.0" >> $@
	xxd -i $< >> $@

	
clean:
	rm -Rf build *.elf *.o *.bin
