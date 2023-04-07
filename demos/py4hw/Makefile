BUILD?=build
INCLUDE+=-I../../include -I../../src/ -I`pwd`
CFLAGS+=-O3 $(INCLUDE) -DCFLEX_NO_COROUTINES
SDL_CFLAGS=$(shell sdl2-config --cflags)
SDL_LIBS=$(shell sdl2-config --libs)

all: run

run: compile
	$(BUILD)/compiled

compile: $(BUILD)/compiled

$(BUILD)/compiled: $(VGA_SRC) simulator.cpp vga_py4hw.cc
	mkdir -p $(BUILD)
	g++ -DCFLEX_SIMULATION $(CFLAGS) $(SDL_CFLAGS) simulator.cpp ../../src/sim_fb.c $(SDL_LIBS) -o $(BUILD)/compiled

clean:
	rm -Rf $(BUILD)


