#ifndef TH02_MAIN_S5_CBRED_HPP
#define TH02_MAIN_S5_CBRED_HPP

#include "platform.h"
#include "th02/replay_format.hpp"

#define TH02_S5_MIMA_CALLBACK_WIRE_SIZE T2REPLAY_EXACT_S5_CALLBACK_SIZE
#define TH02_S5_MIMA_REDRAW_WIRE_SIZE T2REPLAY_EXACT_S5_REDRAW_SIZE

// Captures one semantic profile only after every live callback slot agrees
// with the canonical Stage 5 Mima table and the common callback groups.
bool16 far th02_s5_mima_callback_wire_capture(
	uint8_t far *wire, uint16_t wire_size, uint8_t header_profile,
	uint8_t shottype, const uint8_t far *laser_wire,
	const uint8_t far *enemy_wire
);
bool16 far th02_s5_mima_callback_wire_valid(
	const uint8_t far *wire, uint16_t wire_size
);
bool16 far th02_s5_mima_callback_wire_agree(
	uint8_t header_profile, const uint8_t far *callback_wire,
	const uint8_t far *laser_wire, const uint8_t far *enemy_wire
);

// REDRAW stores a recipe ID rather than graphics or hardware state.
bool16 far th02_s5_mima_redraw_wire_capture(
	uint8_t far *wire, uint16_t wire_size
);
bool16 far th02_s5_mima_redraw_wire_valid(
	const uint8_t far *wire, uint16_t wire_size
);

#endif /* TH02_MAIN_S5_CBRED_HPP */
