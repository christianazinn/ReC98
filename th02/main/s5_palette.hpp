#ifndef TH02_MAIN_S5_PALETTE_HPP
#define TH02_MAIN_S5_PALETTE_HPP

#include "platform.h"
#include "th02/replay_format.hpp"

#define TH02_S5_MIMA_PALETTE_WIRE_SIZE T2REPLAY_EXACT_S5_PALETTE_SIZE

// Fieldwise private exact-checkpoint codec. The payload contains only Mima's
// mutable semantic palette state; stage resources and hardware registers are
// intentionally not wire values.
bool16 far th02_s5_mima_palette_wire_capture(
	uint8_t far *wire, uint16_t wire_size
);
bool16 far th02_s5_mima_palette_wire_valid(
	const uint8_t far *wire, uint16_t wire_size
);
bool16 far th02_s5_mima_palette_wire_agree(
	const uint8_t far *bomb_wire,
	const uint8_t far *actor_core_wire,
	const uint8_t far *actor_stage_wire,
	const uint8_t far *palette_wire
);

#if T2REPLAY_EXACT_APPLY
struct th02_s5_palette_apply_plan_t {
	const uint8_t far *wire;
};
bool16 far th02_s5_mima_palette_wire_prepare(
	th02_s5_palette_apply_plan_t *plan,
	const uint8_t far *wire, uint16_t wire_size
);
void far th02_s5_mima_palette_commit_prepared(
	const th02_s5_palette_apply_plan_t *plan
);
#endif

#endif /* TH02_MAIN_S5_PALETTE_HPP */
