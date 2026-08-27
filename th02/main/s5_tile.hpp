#ifndef TH02_MAIN_S5_TILE_HPP
#define TH02_MAIN_S5_TILE_HPP

#include "platform.h"
#include "th02/replay_format.hpp"

// Pointer-free, fieldwise Stage 5 tile-logic capture vocabulary. A later
// coordinator owns the all-group apply transaction and the two-page redraw.
#define TH02_S5_TILE_LOGIC_WIRE_SIZE T2REPLAY_EXACT_S5_TILE_LOGIC_SIZE

bool16 far th02_s5_tile_logic_wire_capture(
	uint8_t far *wire, uint16_t wire_size
);
bool16 far th02_s5_tile_logic_wire_valid(
	const uint8_t far *wire, uint16_t wire_size
);

#endif /* TH02_MAIN_S5_TILE_HPP */
