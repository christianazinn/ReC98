#ifndef TH02_MAIN_S5_FX_HPP
#define TH02_MAIN_S5_FX_HPP

#include "platform.h"
#include "th02/replay_format.hpp"

// Stage 5 has no live generic background particles. Its exact STAGE_FX
// payload records that quiescent native contract and the reset template
// values, without carrying stale positions or compiler-layout bytes.
#define TH02_S5_MIMA_STAGE_FX_WIRE_SIZE \
	T2REPLAY_EXACT_S5MFX_SIZE

bool16 far th02_s5_mima_stage_fx_wire_capture(
	uint8_t far *wire, uint16_t wire_size
);
bool16 far th02_s5_mima_stage_fx_wire_valid(
	const uint8_t far *wire, uint16_t wire_size
);

#if T2REPLAY_EXACT_APPLY
struct th02_s5_fx_apply_plan_t {
	const uint8_t far *wire;
};
bool16 far th02_s5_mima_stage_fx_wire_prepare(
	th02_s5_fx_apply_plan_t *plan,
	const uint8_t far *wire, uint16_t wire_size
);
void far th02_s5_mima_stage_fx_commit_prepared(
	const th02_s5_fx_apply_plan_t *plan
);
#endif

#endif /* TH02_MAIN_S5_FX_HPP */
