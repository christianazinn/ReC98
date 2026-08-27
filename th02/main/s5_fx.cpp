// TH02 Stage 5 Mima Stage-FX exact-checkpoint ownership. This trailing patch
// segment captures only the semantic background-particle quiescence contract.
// It deliberately has no apply entry point or pointer-bearing carrier.
#pragma option -zCT2S5FX_TEXT -G-

#include "platform.h"
#include "th02/v_colors.hpp"
#include "th02/main/bg_particle.hpp"
#include "th02/main/s5_fx.hpp"

#define T2S5FX_LIVE_COUNT_OFFSET 0
#define T2S5FX_SPEED_INITIAL_OFFSET 1
#define T2S5FX_SPEED_DELTA_OFFSET 3
#define T2S5FX_ANGLE_DELTA_OFFSET 5
#define T2S5FX_EDGE_STEP_OFFSET 7
#define T2S5FX_COL_OFFSET 9
#define T2S5FX_UNPUT_COL_OFFSET 10

static const int T2S5FX_SPEED_INITIAL_DEFAULT = 32;
static const int T2S5FX_SPEED_DELTA_DEFAULT = 1;
static const int T2S5FX_ANGLE_DELTA_DEFAULT = 0;
static const int T2S5FX_EDGE_STEP_DEFAULT = 32;

typedef char th02_s5_mima_stage_fx_wire_size_check[
	((T2S5FX_UNPUT_COL_OFFSET + 1) == TH02_S5_MIMA_STAGE_FX_WIRE_SIZE)
		? 1 : -1
];

struct t2s5fx_state_t {
	uint8_t bg_particle_live_count;
	int16_t bg_particle_speed_initial;
	int16_t bg_particle_speed_delta;
	int16_t bg_particle_angle_delta;
	int16_t bg_particle_edge_step;
	uint8_t bg_particle_col;
	uint8_t bg_particle_unput_col;
};

static void near t2s5fx_wire_put_u16(
	uint8_t far *wire, unsigned offset, uint16_t value
)
{
	wire[offset] = static_cast<uint8_t>(value & 0xFF);
	wire[offset + 1] = static_cast<uint8_t>(value >> 8);
}

static uint16_t near t2s5fx_wire_get_u16(
	const uint8_t far *wire, unsigned offset
)
{
	return static_cast<uint16_t>(
		wire[offset] | (static_cast<uint16_t>(wire[offset + 1]) << 8)
	);
}

static bool16 near t2s5fx_state_valid(const t2s5fx_state_t *state)
{
	return (
		(state != 0) &&
		(state->bg_particle_live_count == 0) &&
		(state->bg_particle_speed_initial == T2S5FX_SPEED_INITIAL_DEFAULT) &&
		(state->bg_particle_speed_delta == T2S5FX_SPEED_DELTA_DEFAULT) &&
		(state->bg_particle_angle_delta == T2S5FX_ANGLE_DELTA_DEFAULT) &&
		(state->bg_particle_edge_step == T2S5FX_EDGE_STEP_DEFAULT) &&
		(state->bg_particle_col == V_WHITE) &&
		(state->bg_particle_unput_col == 0)
	);
}

static bool16 near t2s5fx_state_capture(t2s5fx_state_t *state)
{
	t2s5fx_state_t captured;
	int i;

	if(state == 0) {
		return false;
	}
	captured.bg_particle_live_count = 0;
	for(i = 0; i < BG_PARTICLE_COUNT; i++) {
		// Any non-free slot carries page-local motion that Stage 5 neither
		// updates nor renders. Reject it rather than preserving stale storage.
		if(bg_particles[i].flag != F_FREE) {
			return false;
		}
	}
	captured.bg_particle_speed_initial = bg_particle_speed_initial;
	captured.bg_particle_speed_delta = bg_particle_speed_delta;
	captured.bg_particle_angle_delta = bg_particle_angle_delta;
	captured.bg_particle_edge_step = bg_particle_edge_step;
	captured.bg_particle_col = bg_particle_col;
	captured.bg_particle_unput_col = bg_particle_unput_col;
	if(!t2s5fx_state_valid(&captured)) {
		return false;
	}
	*state = captured;
	return true;
}

static bool16 near t2s5fx_state_wire_encode(
	uint8_t far *wire, uint16_t wire_size, const t2s5fx_state_t *state
)
{
	if(
		(wire == 0) ||
		(wire_size != TH02_S5_MIMA_STAGE_FX_WIRE_SIZE) ||
		!t2s5fx_state_valid(state)
	) {
		return false;
	}
	wire[T2S5FX_LIVE_COUNT_OFFSET] = state->bg_particle_live_count;
	t2s5fx_wire_put_u16(
		wire, T2S5FX_SPEED_INITIAL_OFFSET,
		static_cast<uint16_t>(state->bg_particle_speed_initial)
	);
	t2s5fx_wire_put_u16(
		wire, T2S5FX_SPEED_DELTA_OFFSET,
		static_cast<uint16_t>(state->bg_particle_speed_delta)
	);
	t2s5fx_wire_put_u16(
		wire, T2S5FX_ANGLE_DELTA_OFFSET,
		static_cast<uint16_t>(state->bg_particle_angle_delta)
	);
	t2s5fx_wire_put_u16(
		wire, T2S5FX_EDGE_STEP_OFFSET,
		static_cast<uint16_t>(state->bg_particle_edge_step)
	);
	wire[T2S5FX_COL_OFFSET] = state->bg_particle_col;
	wire[T2S5FX_UNPUT_COL_OFFSET] = state->bg_particle_unput_col;
	return true;
}

static bool16 near t2s5fx_state_wire_decode(
	t2s5fx_state_t *state, const uint8_t far *wire, uint16_t wire_size
)
{
	t2s5fx_state_t decoded;

	if(
		(state == 0) || (wire == 0) ||
		(wire_size != TH02_S5_MIMA_STAGE_FX_WIRE_SIZE)
	) {
		return false;
	}
	decoded.bg_particle_live_count = wire[T2S5FX_LIVE_COUNT_OFFSET];
	decoded.bg_particle_speed_initial = static_cast<int16_t>(
		t2s5fx_wire_get_u16(wire, T2S5FX_SPEED_INITIAL_OFFSET)
	);
	decoded.bg_particle_speed_delta = static_cast<int16_t>(
		t2s5fx_wire_get_u16(wire, T2S5FX_SPEED_DELTA_OFFSET)
	);
	decoded.bg_particle_angle_delta = static_cast<int16_t>(
		t2s5fx_wire_get_u16(wire, T2S5FX_ANGLE_DELTA_OFFSET)
	);
	decoded.bg_particle_edge_step = static_cast<int16_t>(
		t2s5fx_wire_get_u16(wire, T2S5FX_EDGE_STEP_OFFSET)
	);
	decoded.bg_particle_col = wire[T2S5FX_COL_OFFSET];
	decoded.bg_particle_unput_col = wire[T2S5FX_UNPUT_COL_OFFSET];
	if(!t2s5fx_state_valid(&decoded)) {
		return false;
	}
	*state = decoded;
	return true;
}

bool16 far th02_s5_mima_stage_fx_wire_capture(
	uint8_t far *wire, uint16_t wire_size
)
{
	t2s5fx_state_t captured;

	return (
		t2s5fx_state_capture(&captured) &&
		t2s5fx_state_wire_encode(wire, wire_size, &captured) &&
		th02_s5_mima_stage_fx_wire_valid(wire, wire_size)
	);
}

bool16 far th02_s5_mima_stage_fx_wire_valid(
	const uint8_t far *wire, uint16_t wire_size
)
{
	t2s5fx_state_t decoded;

	return t2s5fx_state_wire_decode(&decoded, wire, wire_size);
}
