// Machine-generated using Migen2CflexHDL - do not edit!
// check source license to know the license of this file

#include "cflexhdl.h"

MODULE frame_display(
	const uint12& pix_x,
	const uint12& pix_y,
	const uint1& pix_active,
	const uint1& pix_vblank,
	uint8& pix_r,
	uint8& pix_g,
	uint8& pix_b
) {
	uint1 sys_clk = 0;
	uint1 sys_rst = 0;
uint1 enable0 = 1;
uint1 vtg_sink_valid;
uint1 vtg_sink_ready;
uint1 vtg_sink_first;
uint1 vtg_sink_last = 0;
uint1 vtg_sink_payload_hsync = 0;
uint1 vtg_sink_payload_vsync = 0;
uint1 vtg_sink_payload_de;
uint12 vtg_sink_payload_hres;
uint12 vtg_sink_payload_vres;
uint12 vtg_sink_payload_hcount = 0;
uint12 vtg_sink_payload_vcount = 0;
uint1 source_valid;
uint1 source_ready;
uint1 source_last;
uint1 source_payload_hsync;
uint1 source_payload_vsync;
uint1 source_payload_de;
uint8 source_payload_r;
uint8 source_payload_g;
uint8 source_payload_b;
uint1 enable1;
uint12 pix = 0;
uint3 bar = 0;
uint1 reset_1;
uint1 state = 0;
uint1 next_state;
uint12 pix_next_value0;
uint1 pix_next_value_ce0;
uint3 bar_next_value1;
uint1 bar_next_value_ce1;
/* no_retiming = "true" */ uint1 regs0 = 0;
/* no_retiming = "true" */ uint1 regs1 = 0;

while(always()) {
	 source_ready = 1;
	 pix_r = source_payload_r;
	 pix_g = source_payload_g;
	 pix_b = source_payload_b;
	 vtg_sink_valid = 1;
	 vtg_sink_first = 1;
	 vtg_sink_payload_de = pix_active;
	 vtg_sink_payload_hres = 640;
	 vtg_sink_payload_vres = 480;
	 reset_1 = (!enable0);
	source_payload_r = 0;
	source_payload_g = 0;
	source_payload_b = 0;
	switch(bar) {
	case 0: {
			source_payload_r = 255;
			source_payload_g = 255;
			source_payload_b = 255;
		} break;
	case 1: {
			source_payload_r = 255;
			source_payload_g = 255;
			source_payload_b = 0;
		} break;
	case 2: {
			source_payload_r = 0;
			source_payload_g = 255;
			source_payload_b = 255;
		} break;
	case 3: {
			source_payload_r = 0;
			source_payload_g = 255;
			source_payload_b = 0;
		} break;
	case 4: {
			source_payload_r = 255;
			source_payload_g = 0;
			source_payload_b = 255;
		} break;
	case 5: {
			source_payload_r = 255;
			source_payload_g = 0;
			source_payload_b = 0;
		} break;
	case 6: {
			source_payload_r = 0;
			source_payload_g = 0;
			source_payload_b = 255;
		} break;
	case 7: {
			source_payload_r = 0;
			source_payload_g = 0;
			source_payload_b = 0;
		} break;
	}
	vtg_sink_ready = 0;
	source_valid = 0;
	source_last = 0;
	source_payload_hsync = 0;
	source_payload_vsync = 0;
	source_payload_de = 0;
	next_state = 0;
	pix_next_value0 = 0;
	pix_next_value_ce0 = 0;
	bar_next_value1 = 0;
	bar_next_value_ce1 = 0;
	vtg_sink_ready = 1;
	next_state = state;
	switch(state) {
	case 1: {
			source_valid = vtg_sink_valid;
			vtg_sink_ready = source_ready;
			source_last = vtg_sink_last;
			source_payload_hsync = vtg_sink_payload_hsync;
			source_payload_vsync = vtg_sink_payload_vsync;
			source_payload_de = vtg_sink_payload_de;
			if (((source_valid & source_ready) & source_payload_de)) {
				pix_next_value0 = (pix + 1);
				pix_next_value_ce0 = 1;
				if ((pix == (bitslice(11, 3, vtg_sink_payload_hres) - 1))) {
					pix_next_value0 = 0;
					pix_next_value_ce0 = 1;
					bar_next_value1 = (bar + 1);
					bar_next_value_ce1 = 1;
				}
			} else {
				pix_next_value0 = 0;
				pix_next_value_ce0 = 1;
				bar_next_value1 = 0;
				bar_next_value_ce1 = 1;
			}
		} break;
		default: {
			pix_next_value0 = 0;
			pix_next_value_ce0 = 1;
			bar_next_value1 = 0;
			bar_next_value_ce1 = 1;
			vtg_sink_ready = 1;
			if ((((vtg_sink_valid & vtg_sink_first) & (vtg_sink_payload_hcount == 0)) & (vtg_sink_payload_vcount == 0))) {
				vtg_sink_ready = 0;
				next_state = 1;
			}
		} break;
	}
	 enable1 = regs1;

	state = next_state;
	if (pix_next_value_ce0) {
		pix = pix_next_value0;
	}
	if (bar_next_value_ce1) {
		bar = bar_next_value1;
	}
	if (reset_1) {
		pix = 0;
		bar = 0;
		state = 0;
	}
	if (sys_rst) {
		pix = 0;
		bar = 0;
		state = 0;
	}
	regs0 = enable0;
	regs1 = regs0;
}

}


#include "vga_config.h"
