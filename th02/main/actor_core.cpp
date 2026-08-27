// TH02 generic boss and midboss checkpoint ownership. This patch-only segment
// follows the stage-specific actor tails and does not move native code.
#pragma option -zCT2ACTCORE_TEXT -G-

#include "platform.h"
#include "pc98.h"
#include "th02/main/actor_core.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/explode.hpp"
#include "th02/hardware/pages.hpp"

extern "C" int patnum_2064E;
extern "C" uint8_t boss_phase;
extern "C" bool boss_hit_flash;
extern "C" uint8_t boss_rank_param[5];
extern "C" uint8_t bomb_damage_frame_mask;

static bool16 near th02_actor_core_coord_valid(int value, int extent)
{
	return ((value >= -512) && (value < (extent + 512)));
}

static bool16 near th02_actor_core_state_validate(
	const th02_actor_core_state_t *state
)
{
	int page;

	if(
		(state == 0) ||
		(state->patnum < 0) ||
		(state->patnum >= 512) ||
		(state->phase_frame < 0) ||
		(state->damage < 0) ||
		(state->phase > 31) ||
		!((state->hit_flash == false) || (state->hit_flash == true)) ||
		!((state->bomb_damage_frame_mask == 1) ||
		  (state->bomb_damage_frame_mask == 3))
	) {
		return false;
	}
	for(page = 0; page < PAGE_COUNT; page++) {
		if(
			!th02_actor_core_coord_valid(state->left_on_page[page], RES_X) ||
			!th02_actor_core_coord_valid(state->top_on_page[page], RES_Y)
		) {
			return false;
		}
	}
	return true;
}

bool16 far th02_actor_core_state_capture(th02_actor_core_state_t *state)
{
	th02_actor_core_state_t captured;
	int i;

	captured.patnum = patnum_2064E;
	captured.phase_frame = boss_phase_frame;
	for(i = 0; i < PAGE_COUNT; i++) {
		captured.left_on_page[i] = boss_left_on_page[i];
		captured.top_on_page[i] = boss_top_on_page[i];
	}
	captured.damage = boss_damage;
	captured.phase = boss_phase;
	captured.hit_flash = boss_hit_flash;
	for(i = 0; i < 5; i++) {
		captured.rank_param[i] = boss_rank_param[i];
	}
	captured.explode_angle_offset = boss_explode_angle_offset;
	captured.bomb_damage_frame_mask = bomb_damage_frame_mask;
	if(!th02_actor_core_state_validate(&captured) || (state == 0)) {
		return false;
	}
	*state = captured;
	return true;
}

static void near th02_actor_core_state_commit(
	const th02_actor_core_state_t *state
)
{
	int i;

	patnum_2064E = state->patnum;
	boss_phase_frame = state->phase_frame;
	for(i = 0; i < PAGE_COUNT; i++) {
		boss_left_on_page[i] = state->left_on_page[i];
		boss_top_on_page[i] = state->top_on_page[i];
	}
	boss_left_on_back_page = &boss_left_on_page[page_back];
	boss_top_on_back_page = &boss_top_on_page[page_back];
	boss_damage = state->damage;
	boss_phase = state->phase;
	boss_hit_flash = state->hit_flash;
	for(i = 0; i < 5; i++) {
		boss_rank_param[i] = state->rank_param[i];
	}
	boss_explode_angle_offset = state->explode_angle_offset;
	bomb_damage_frame_mask = state->bomb_damage_frame_mask;

}

bool16 far th02_actor_core_state_apply(
	const th02_actor_core_state_t *state
)
{
	if(!th02_actor_core_state_validate(state)) {
		return false;
	}
	th02_actor_core_state_commit(state);
	return true;
}

// Keep the existing state owner exact and append the private byte codec in its
// own tail. The replay envelope consumes bytes, never compiler struct layout.
#pragma codeseg T2ACTCORECKPT_TEXT

static void near th02_actor_core_wire_put_u16(
	uint8_t far *wire, uint16_t& offset, uint16_t value
)
{
	wire[offset++] = static_cast<uint8_t>(value);
	wire[offset++] = static_cast<uint8_t>(value >> 8);
}

static uint16_t near th02_actor_core_wire_take_u16(
	const uint8_t far *wire, uint16_t& offset
)
{
	uint16_t value = static_cast<uint16_t>(
		static_cast<uint16_t>(wire[offset]) |
		(static_cast<uint16_t>(wire[offset + 1]) << 8)
	);

	offset += 2;
	return value;
}

static bool16 near th02_actor_core_state_wire_encode(
	uint8_t far *wire, uint16_t wire_size,
	const th02_actor_core_state_t *state
)
{
	uint16_t offset = 0;
	int page;

	if(
		(wire == 0) ||
		(wire_size != TH02_ACTOR_CORE_WIRE_SIZE) ||
		!th02_actor_core_state_validate(state)
	) {
		return false;
	}
	th02_actor_core_wire_put_u16(
		wire, offset, static_cast<uint16_t>(state->patnum)
	);
	th02_actor_core_wire_put_u16(
		wire, offset, static_cast<uint16_t>(state->phase_frame)
	);
	for(page = 0; page < PAGE_COUNT; page++) {
		th02_actor_core_wire_put_u16(
			wire, offset, static_cast<uint16_t>(state->left_on_page[page])
		);
	}
	for(page = 0; page < PAGE_COUNT; page++) {
		th02_actor_core_wire_put_u16(
			wire, offset, static_cast<uint16_t>(state->top_on_page[page])
		);
	}
	th02_actor_core_wire_put_u16(
		wire, offset, static_cast<uint16_t>(state->damage)
	);
	wire[offset++] = state->phase;
	wire[offset++] = static_cast<uint8_t>(state->hit_flash);
	for(page = 0; page < 5; page++) {
		wire[offset++] = state->rank_param[page];
	}
	wire[offset++] = state->explode_angle_offset;
	wire[offset++] = state->bomb_damage_frame_mask;
	return (offset == TH02_ACTOR_CORE_WIRE_SIZE);
}

static bool16 near th02_actor_core_state_wire_decode(
	th02_actor_core_state_t *state,
	const uint8_t far *wire, uint16_t wire_size
)
{
	th02_actor_core_state_t decoded;
	uint16_t offset = 0;
	int page;

	if((state == 0) || (wire == 0) ||
		(wire_size != TH02_ACTOR_CORE_WIRE_SIZE)) {
		return false;
	}
	decoded.patnum = static_cast<int16_t>(
		th02_actor_core_wire_take_u16(wire, offset)
	);
	decoded.phase_frame = static_cast<int16_t>(
		th02_actor_core_wire_take_u16(wire, offset)
	);
	for(page = 0; page < PAGE_COUNT; page++) {
		decoded.left_on_page[page] = static_cast<screen_x_t>(
			th02_actor_core_wire_take_u16(wire, offset)
		);
	}
	for(page = 0; page < PAGE_COUNT; page++) {
		decoded.top_on_page[page] = static_cast<screen_y_t>(
			th02_actor_core_wire_take_u16(wire, offset)
		);
	}
	decoded.damage = static_cast<int16_t>(
		th02_actor_core_wire_take_u16(wire, offset)
	);
	decoded.phase = wire[offset++];
	decoded.hit_flash = static_cast<bool16>(wire[offset++]);
	for(page = 0; page < 5; page++) {
		decoded.rank_param[page] = wire[offset++];
	}
	decoded.explode_angle_offset = wire[offset++];
	decoded.bomb_damage_frame_mask = wire[offset++];
	if((offset != TH02_ACTOR_CORE_WIRE_SIZE) ||
		!th02_actor_core_state_validate(&decoded)) {
		return false;
	}
	*state = decoded;
	return true;
}

bool16 far th02_actor_core_state_wire_capture(
	uint8_t far *wire, uint16_t wire_size
)
{
	th02_actor_core_state_t captured;

	return (
		th02_actor_core_state_capture(&captured) &&
		th02_actor_core_state_wire_encode(wire, wire_size, &captured)
	);
}

bool16 far th02_actor_core_state_wire_valid(
	const uint8_t far *wire, uint16_t wire_size
)
{
	th02_actor_core_state_t decoded;

	return th02_actor_core_state_wire_decode(&decoded, wire, wire_size);
}

#if T2REPLAY_EXACT_APPLY
bool16 far th02_actor_core_state_wire_prepare(
	th02_actor_core_state_t *state,
	const uint8_t far *wire, uint16_t wire_size
)
{
	return th02_actor_core_state_wire_decode(state, wire, wire_size);
}

void far th02_actor_core_state_commit_prepared(
	const th02_actor_core_state_t *state
)
{
	th02_actor_core_state_commit(state);
}
#endif
