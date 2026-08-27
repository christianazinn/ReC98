// TH02 Stage 5 Mima checkpoint and clean-Practice ownership. This patch-only
// segment is inert until the exact codec and Practice dispatcher consume it.
#pragma option -zCT2S5ACT_TEXT -G-

#include "platform.h"
#include "pc98.h"
#include "th01/rank.h"
#include "th02/core/globals.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/s5_actor.hpp"

extern "C" int patnum_2064E;
extern "C" int boss_damage;
extern "C" uint8_t boss_phase;
extern "C" int boss_phase_frame;
extern "C" bool boss_hit_flash;
extern "C" uint8_t boss_rank_param[5];

extern "C" screen_x_t left_26C56;
extern "C" screen_x_t mima_muzzle_left;
extern "C" screen_x_t left_26C5A;
extern "C" screen_x_t x_26C5C;
extern "C" screen_y_t top_26C5E;
extern "C" screen_y_t mima_muzzle_top;
extern "C" screen_y_t top_26C62;
extern "C" screen_y_t y_26C64;
extern "C" int mima_velocity_y;
extern "C" int mima_damage_multiplier;
extern "C" int mima_phase;
extern "C" int mima_pattern;
extern "C" int mima_patterns_this_phase;
extern "C" bool mima_all_patterns;
extern "C" int mima_phase_damage_max;
extern "C" int mima_patterns_max;
extern "C" int mima_pattern_count;
extern "C" int mima_patterns_until_vulnerable;

extern "C" screen_x_t mima_orb_left_on_page[PAGE_COUNT][TH02_S5_MIMA_ORB_SLOTS];
extern "C" screen_y_t mima_orb_top_on_page[PAGE_COUNT][TH02_S5_MIMA_ORB_SLOTS];
extern "C" int16_t mima_orb_flag[TH02_S5_MIMA_ORB_SLOTS];
extern "C" int mima_orbs_gone_unused;
extern "C" bool mima_orb_variant;
extern "C" int mima_orb_flight_center_x;
extern "C" int mima_orb_flight_center_y;
extern "C" int mima_orb_flight_radius;
extern "C" signed char mima_orb_flight_radius_step;
extern "C" unsigned char mima_orb_flight_angle;
extern "C" bool mima_orb_flight_detonated;
extern "C" screen_x_t mima_orb_center_x;
extern "C" screen_y_t mima_orb_center_y;
extern "C" int16_t mima_orb_radius;
extern "C" unsigned char mima_orb_angle;

extern "C" uint8_t mima_bg_ring_radius;
extern "C" uint8_t mima_bg_circle_radius;
extern "C" uint8_t mima_bg_ring_col_head;
extern "C" uint8_t mima_bg_ring_col_tail;
extern "C" uint8_t mima_bg_circle_col;
extern "C" uint8_t mima_bg_ring_phase;
extern "C" uint8_t mima_ring_radius;
extern "C" uint8_t mima_bg_circle_radius_base;
extern "C" uint8_t mima_bg_circle_pulse_frame;
extern "C" uint8_t mima_flash_frame;
extern "C" unsigned char mima_ray_unused;
extern "C" unsigned char mima_ray_angle;
extern "C" uint8_t mima_ray_tone;
extern "C" unsigned char mima_spiral_angle;
extern "C" unsigned char mima_aim_angle_unused;
extern "C" unsigned char mima_charge_ring_radius;
extern "C" unsigned char mima_stream_angle;
extern "C" uint8_t mima_stream_speed;
extern "C" unsigned char mima_pair_angle;
extern "C" unsigned char mima_star_angle;
extern "C" signed char mima_star_direction;
extern "C" unsigned char mima_fan_angle;
extern "C" screen_point_t point_26CD6;
extern "C" screen_point_t point_26CDE;
extern "C" vram_y_t stage4_tile_top;

// mima_180AC() is a near BOSS_5_TEXT routine. This patch tail cannot call it
// without overflowing TLINK's near relocation, so the clean-only constructor
// spells its audited two-rank result directly. Exact capture/apply continues
// to treat this generic block as actor-core state.
static void near th02_s5_mima_clean_rank_parameters_set(void)
{
	if(rank != RANK_EASY) {
		boss_rank_param[0] = BG_16_RING;
		boss_rank_param[1] = 0x17;
		boss_rank_param[2] = BG_32_RING;
		boss_rank_param[3] = 0x21;
		boss_rank_param[4] = 6;
		return;
	}
	boss_rank_param[0] = BG_16_RING;
	boss_rank_param[1] = 0x19;
	boss_rank_param[2] = BG_2_SPREAD_MEDIUM_AIMED;
	boss_rank_param[3] = 0x22;
	boss_rank_param[4] = 8;
}

static bool16 near th02_s5_mima_phase_valid(
	int16_t phase, int16_t pattern, uint8_t all_patterns
)
{
	if((phase < 0) || (phase > 9) || (phase == 8)) {
		return false;
	}
	if(phase == 7) {
		return (all_patterns && (pattern >= 0) && (pattern <= 8));
	}
	return (!all_patterns && (pattern >= 0) && (pattern <= 2));
}

static bool16 near th02_s5_mima_phase_parameters_valid(
	const th02_s5_mima_state_t *state
)
{
	if(state->phase == 0) {
		return (
			(state->phase_damage_max == 0) &&
			(state->patterns_max == 0) &&
			(state->pattern_count == 0) &&
			(state->patterns_until_vulnerable == 0)
		);
	}
	if((state->phase == 1) || (state->phase == 2)) {
		return (
			(state->phase_damage_max == 600) &&
			(state->patterns_max == 10) &&
			(state->pattern_count == 3) &&
			(state->patterns_until_vulnerable == 2)
		);
	}
	if((state->phase == 3) || (state->phase == 4)) {
		return (
			(state->phase_damage_max == 700) &&
			(state->patterns_max == 12) &&
			(state->pattern_count == 3) &&
			(state->patterns_until_vulnerable == 2)
		);
	}
	if((state->phase == 5) || (state->phase == 6)) {
		return (
			(state->phase_damage_max == 800) &&
			(state->patterns_max == 12) &&
			(state->pattern_count == 3) &&
			(state->patterns_until_vulnerable == 2)
		);
	}
	if(state->phase == 7) {
		return (
			(state->phase_damage_max == 1500) &&
			(state->patterns_max == 200) &&
			(state->pattern_count == 9) &&
			(state->patterns_until_vulnerable == 3)
		);
	}
	return (
		(state->phase_damage_max == 1100) &&
		(state->patterns_max == 200) &&
		(state->pattern_count == 3) &&
		(state->patterns_until_vulnerable == 2)
	);
}

static bool16 near th02_s5_mima_state_validate(
	const th02_s5_mima_state_t *state
)
{
	int i;
	int page;

	if(
		(state == 0) ||
		!th02_s5_mima_phase_valid(
			state->phase, state->pattern, state->all_patterns
		) ||
		!th02_s5_mima_phase_parameters_valid(state) ||
		(state->damage_multiplier > 1) ||
		(state->damage_multiplier < 0) ||
		(state->velocity_y < -1) ||
		(state->velocity_y > 1) ||
		(state->patterns_this_phase < 0) ||
		(state->patterns_this_phase > 200) ||
		(state->orbs_gone_unused < 0) ||
		(state->orbs_gone_unused > TH02_S5_MIMA_ORB_SLOTS) ||
		(state->orb_variant > 1) ||
		(state->orb_flight_center_x < -8192) ||
		(state->orb_flight_center_x > 16384) ||
		(state->orb_flight_center_y < -8192) ||
		(state->orb_flight_center_y > 16384) ||
		(state->orb_flight_radius < -16) ||
		(state->orb_flight_radius > 512) ||
		(state->orb_flight_radius_step < -5) ||
		(state->orb_flight_radius_step > 5) ||
		(state->orb_flight_detonated > 1) ||
		(state->orb_radius < 0) ||
		(state->orb_radius > 512) ||
		(state->star_direction < -1) ||
		(state->star_direction > 1) ||
		(state->ray_tone > 150)
	) {
		return false;
	}
	for(page = 0; page < PAGE_COUNT; page++) {
		for(i = 0; i < TH02_S5_MIMA_ORB_SLOTS; i++) {
			if(
				(state->orb_left_on_page[page][i] < -512) ||
				(state->orb_left_on_page[page][i] > (RES_X + 512)) ||
				(state->orb_top_on_page[page][i] < -512) ||
				(state->orb_top_on_page[page][i] > (RES_Y + 512)) ||
				(state->orb_flag[i] < 0) ||
				(state->orb_flag[i] > 2)
			) {
				return false;
			}
		}
	}
	return true;
}

bool16 far th02_s5_mima_state_capture(th02_s5_mima_state_t *state)
{
	int i;
	int page;
	th02_s5_mima_state_t captured;

	if(state == 0) {
		return false;
	}
	captured.left_26C56 = left_26C56;
	captured.muzzle_left = mima_muzzle_left;
	captured.left_26C5A = left_26C5A;
	captured.x_26C5C = x_26C5C;
	captured.top_26C5E = top_26C5E;
	captured.muzzle_top = mima_muzzle_top;
	captured.top_26C62 = top_26C62;
	captured.y_26C64 = y_26C64;
	captured.velocity_y = mima_velocity_y;
	captured.damage_multiplier = mima_damage_multiplier;
	captured.phase = mima_phase;
	captured.pattern = mima_pattern;
	captured.patterns_this_phase = mima_patterns_this_phase;
	captured.all_patterns = mima_all_patterns;
	captured.phase_damage_max = mima_phase_damage_max;
	captured.patterns_max = mima_patterns_max;
	captured.pattern_count = mima_pattern_count;
	captured.patterns_until_vulnerable = mima_patterns_until_vulnerable;
	for(page = 0; page < PAGE_COUNT; page++) {
		for(i = 0; i < TH02_S5_MIMA_ORB_SLOTS; i++) {
			captured.orb_left_on_page[page][i] = mima_orb_left_on_page[page][i];
			captured.orb_top_on_page[page][i] = mima_orb_top_on_page[page][i];
		}
	}
	for(i = 0; i < TH02_S5_MIMA_ORB_SLOTS; i++) {
		captured.orb_flag[i] = mima_orb_flag[i];
	}
	captured.orbs_gone_unused = mima_orbs_gone_unused;
	captured.orb_variant = mima_orb_variant;
	captured.orb_flight_center_x = mima_orb_flight_center_x;
	captured.orb_flight_center_y = mima_orb_flight_center_y;
	captured.orb_flight_radius = mima_orb_flight_radius;
	captured.orb_flight_radius_step = mima_orb_flight_radius_step;
	captured.orb_flight_angle = mima_orb_flight_angle;
	captured.orb_flight_detonated = mima_orb_flight_detonated;
	captured.orb_center_x = mima_orb_center_x;
	captured.orb_center_y = mima_orb_center_y;
	captured.orb_radius = mima_orb_radius;
	captured.orb_angle = mima_orb_angle;
	captured.bg_ring_radius = mima_bg_ring_radius;
	captured.bg_circle_radius = mima_bg_circle_radius;
	captured.bg_ring_col_head = mima_bg_ring_col_head;
	captured.bg_ring_col_tail = mima_bg_ring_col_tail;
	captured.bg_circle_col = mima_bg_circle_col;
	captured.bg_ring_phase = mima_bg_ring_phase;
	captured.ring_radius = mima_ring_radius;
	captured.bg_circle_radius_base = mima_bg_circle_radius_base;
	captured.bg_circle_pulse_frame = mima_bg_circle_pulse_frame;
	captured.flash_frame = mima_flash_frame;
	captured.ray_unused = mima_ray_unused;
	captured.ray_angle = mima_ray_angle;
	captured.ray_tone = mima_ray_tone;
	captured.spiral_angle = mima_spiral_angle;
	captured.aim_angle_unused = mima_aim_angle_unused;
	captured.charge_ring_radius = mima_charge_ring_radius;
	captured.stream_angle = mima_stream_angle;
	captured.stream_speed = mima_stream_speed;
	captured.pair_angle = mima_pair_angle;
	captured.star_angle = mima_star_angle;
	captured.star_direction = mima_star_direction;
	captured.fan_angle = mima_fan_angle;
	captured.point_26CD6 = point_26CD6;
	captured.point_26CDE = point_26CDE;
	if(!th02_s5_mima_state_validate(&captured)) {
		return false;
	}
	*state = captured;
	return true;
}

bool16 far th02_s5_mima_state_apply(const th02_s5_mima_state_t *state)
{
	int i;
	int page;

	if(!th02_s5_mima_state_validate(state)) {
		return false;
	}
	left_26C56 = state->left_26C56;
	mima_muzzle_left = state->muzzle_left;
	left_26C5A = state->left_26C5A;
	x_26C5C = state->x_26C5C;
	top_26C5E = state->top_26C5E;
	mima_muzzle_top = state->muzzle_top;
	top_26C62 = state->top_26C62;
	y_26C64 = state->y_26C64;
	mima_velocity_y = state->velocity_y;
	mima_damage_multiplier = state->damage_multiplier;
	mima_phase = state->phase;
	mima_pattern = state->pattern;
	mima_patterns_this_phase = state->patterns_this_phase;
	mima_all_patterns = state->all_patterns;
	mima_phase_damage_max = state->phase_damage_max;
	mima_patterns_max = state->patterns_max;
	mima_pattern_count = state->pattern_count;
	mima_patterns_until_vulnerable = state->patterns_until_vulnerable;
	for(page = 0; page < PAGE_COUNT; page++) {
		for(i = 0; i < TH02_S5_MIMA_ORB_SLOTS; i++) {
			mima_orb_left_on_page[page][i] = state->orb_left_on_page[page][i];
			mima_orb_top_on_page[page][i] = state->orb_top_on_page[page][i];
		}
	}
	for(i = 0; i < TH02_S5_MIMA_ORB_SLOTS; i++) {
		mima_orb_flag[i] = state->orb_flag[i];
	}
	mima_orbs_gone_unused = state->orbs_gone_unused;
	mima_orb_variant = state->orb_variant;
	mima_orb_flight_center_x = state->orb_flight_center_x;
	mima_orb_flight_center_y = state->orb_flight_center_y;
	mima_orb_flight_radius = state->orb_flight_radius;
	mima_orb_flight_radius_step = state->orb_flight_radius_step;
	mima_orb_flight_angle = state->orb_flight_angle;
	mima_orb_flight_detonated = state->orb_flight_detonated;
	mima_orb_center_x = state->orb_center_x;
	mima_orb_center_y = state->orb_center_y;
	mima_orb_radius = state->orb_radius;
	mima_orb_angle = state->orb_angle;
	mima_bg_ring_radius = state->bg_ring_radius;
	mima_bg_circle_radius = state->bg_circle_radius;
	mima_bg_ring_col_head = state->bg_ring_col_head;
	mima_bg_ring_col_tail = state->bg_ring_col_tail;
	mima_bg_circle_col = state->bg_circle_col;
	mima_bg_ring_phase = state->bg_ring_phase;
	mima_ring_radius = state->ring_radius;
	mima_bg_circle_radius_base = state->bg_circle_radius_base;
	mima_bg_circle_pulse_frame = state->bg_circle_pulse_frame;
	mima_flash_frame = state->flash_frame;
	mima_ray_unused = state->ray_unused;
	mima_ray_angle = state->ray_angle;
	mima_ray_tone = state->ray_tone;
	mima_spiral_angle = state->spiral_angle;
	mima_aim_angle_unused = state->aim_angle_unused;
	mima_charge_ring_radius = state->charge_ring_radius;
	mima_stream_angle = state->stream_angle;
	mima_stream_speed = state->stream_speed;
	mima_pair_angle = state->pair_angle;
	mima_star_angle = state->star_angle;
	mima_star_direction = state->star_direction;
	mima_fan_angle = state->fan_angle;
	point_26CD6 = state->point_26CD6;
	point_26CDE = state->point_26CDE;
	boss_left_on_back_page = &boss_left_on_page[page_back];
	boss_top_on_back_page = &boss_top_on_page[page_back];
	return true;
}

bool16 far th02_s5_field_state_capture(th02_s5_field_state_t *state)
{
	if((state == 0) || (stage4_tile_top >= RES_Y)) {
		return false;
	}
	state->tile_top = stage4_tile_top;
	return true;
}

bool16 far th02_s5_field_state_apply(const th02_s5_field_state_t *state)
{
	if((state == 0) || (state->tile_top >= RES_Y)) {
		return false;
	}
	stage4_tile_top = state->tile_top;
	return true;
}

bool16 far th02_s5_mima_clean_init(th02_s5_mima_clean_target_t target)
{
	int i;
	int page;
	const screen_x_t initial_left = (PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 80);
	const screen_y_t initial_top = (PLAYFIELD_TOP + 48);

	if(target != T2S5_MIMA_BOSS_START) {
		return false;
	}
	boss_left_on_page[0] = initial_left;
	boss_left_on_page[1] = initial_left;
	boss_top_on_page[0] = initial_top;
	boss_top_on_page[1] = initial_top;
	boss_left_on_back_page = &boss_left_on_page[page_back];
	boss_top_on_back_page = &boss_top_on_page[page_back];
	boss_damage = 0;
	boss_phase = 0;
	boss_phase_frame = 0;
	boss_hit_flash = false;
	patnum_2064E = 128;
	left_26C56 = (initial_left + 32);
	mima_muzzle_left = (initial_left + 40);
	left_26C5A = (initial_left + 64);
	x_26C5C = (initial_left + 64);
	top_26C5E = (initial_top + 96);
	mima_muzzle_top = (initial_top + 16);
	top_26C62 = (initial_top + 114);
	y_26C64 = (initial_top + 44);
	mima_velocity_y = 0;
	mima_damage_multiplier = 0;
	mima_phase = 0;
	mima_pattern = 0;
	mima_patterns_this_phase = 0;
	mima_all_patterns = false;
	mima_phase_damage_max = 0;
	mima_patterns_max = 0;
	mima_pattern_count = 0;
	mima_patterns_until_vulnerable = 0;
	for(page = 0; page < PAGE_COUNT; page++) {
		for(i = 0; i < TH02_S5_MIMA_ORB_SLOTS; i++) {
			mima_orb_left_on_page[page][i] = 0;
			mima_orb_top_on_page[page][i] = 0;
		}
	}
	for(i = 0; i < TH02_S5_MIMA_ORB_SLOTS; i++) {
		mima_orb_flag[i] = 0;
	}
	mima_orbs_gone_unused = TH02_S5_MIMA_ORB_SLOTS;
	mima_orb_variant = false;
	mima_orb_flight_center_x = 0;
	mima_orb_flight_center_y = 0;
	mima_orb_flight_radius = 0;
	mima_orb_flight_radius_step = 0;
	mima_orb_flight_angle = 0;
	mima_orb_flight_detonated = false;
	mima_orb_center_x = 0;
	mima_orb_center_y = 0;
	mima_orb_radius = 0;
	mima_orb_angle = 0;
	mima_bg_ring_radius = 0;
	mima_bg_circle_radius = 0;
	mima_bg_ring_col_head = 13;
	mima_bg_ring_col_tail = 12;
	mima_bg_circle_col = 3;
	mima_bg_ring_phase = 0;
	mima_ring_radius = 0;
	mima_bg_circle_radius_base = 0;
	mima_bg_circle_pulse_frame = 0;
	mima_flash_frame = 0;
	mima_ray_unused = 0;
	mima_ray_angle = 0;
	mima_ray_tone = 0;
	mima_spiral_angle = 0;
	mima_aim_angle_unused = 0;
	mima_charge_ring_radius = 0;
	mima_stream_angle = 0;
	mima_stream_speed = 0;
	mima_pair_angle = 0;
	mima_star_angle = 0;
	mima_star_direction = 0;
	mima_fan_angle = 0;
	point_26CD6.x = point_26CD6.y = 0;
	point_26CDE.x = point_26CDE.y = 0;
	th02_s5_mima_clean_rank_parameters_set();
	return true;
}

// Keep the pointer-free state owner intact and append its fieldwise replay
// codec separately. The emitted bytes are a private exact-envelope payload,
// not a compiler-layout carrier.
#pragma codeseg T2S5CKPT_TEXT

static void near th02_s5_mima_wire_put_u16(
	uint8_t far *wire, uint16_t& offset, uint16_t value
)
{
	wire[offset++] = static_cast<uint8_t>(value);
	wire[offset++] = static_cast<uint8_t>(value >> 8);
}

static uint16_t near th02_s5_mima_wire_take_u16(
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

static void near th02_s5_mima_wire_put_point(
	uint8_t far *wire, uint16_t& offset, const screen_point_t& point
)
{
	th02_s5_mima_wire_put_u16(
		wire, offset, static_cast<uint16_t>(point.x)
	);
	th02_s5_mima_wire_put_u16(
		wire, offset, static_cast<uint16_t>(point.y)
	);
}

static void near th02_s5_mima_wire_take_point(
	screen_point_t& point, const uint8_t far *wire, uint16_t& offset
)
{
	point.x = static_cast<screen_x_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	point.y = static_cast<screen_y_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
}

static bool16 near th02_s5_mima_state_wire_encode(
	uint8_t far *wire, uint16_t wire_size,
	const th02_s5_mima_state_t *state
)
{
	uint16_t offset = 0;
	int i;
	int page;

	if((wire == 0) || (wire_size != TH02_S5_MIMA_WIRE_SIZE) ||
		!th02_s5_mima_state_validate(state)) {
		return false;
	}
	th02_s5_mima_wire_put_u16(wire, offset, state->left_26C56);
	th02_s5_mima_wire_put_u16(wire, offset, state->muzzle_left);
	th02_s5_mima_wire_put_u16(wire, offset, state->left_26C5A);
	th02_s5_mima_wire_put_u16(wire, offset, state->x_26C5C);
	th02_s5_mima_wire_put_u16(wire, offset, state->top_26C5E);
	th02_s5_mima_wire_put_u16(wire, offset, state->muzzle_top);
	th02_s5_mima_wire_put_u16(wire, offset, state->top_26C62);
	th02_s5_mima_wire_put_u16(wire, offset, state->y_26C64);
	th02_s5_mima_wire_put_u16(
		wire, offset, static_cast<uint16_t>(state->velocity_y)
	);
	th02_s5_mima_wire_put_u16(
		wire, offset, static_cast<uint16_t>(state->damage_multiplier)
	);
	th02_s5_mima_wire_put_u16(wire, offset, static_cast<uint16_t>(state->phase));
	th02_s5_mima_wire_put_u16(wire, offset, static_cast<uint16_t>(state->pattern));
	th02_s5_mima_wire_put_u16(
		wire, offset, static_cast<uint16_t>(state->patterns_this_phase)
	);
	wire[offset++] = state->all_patterns;
	th02_s5_mima_wire_put_u16(
		wire, offset, static_cast<uint16_t>(state->phase_damage_max)
	);
	th02_s5_mima_wire_put_u16(
		wire, offset, static_cast<uint16_t>(state->patterns_max)
	);
	th02_s5_mima_wire_put_u16(
		wire, offset, static_cast<uint16_t>(state->pattern_count)
	);
	th02_s5_mima_wire_put_u16(
		wire, offset, static_cast<uint16_t>(state->patterns_until_vulnerable)
	);
	for(page = 0; page < PAGE_COUNT; page++) {
		for(i = 0; i < TH02_S5_MIMA_ORB_SLOTS; i++) {
			th02_s5_mima_wire_put_u16(
				wire, offset,
				static_cast<uint16_t>(state->orb_left_on_page[page][i])
			);
		}
	}
	for(page = 0; page < PAGE_COUNT; page++) {
		for(i = 0; i < TH02_S5_MIMA_ORB_SLOTS; i++) {
			th02_s5_mima_wire_put_u16(
				wire, offset,
				static_cast<uint16_t>(state->orb_top_on_page[page][i])
			);
		}
	}
	for(i = 0; i < TH02_S5_MIMA_ORB_SLOTS; i++) {
		th02_s5_mima_wire_put_u16(
			wire, offset, static_cast<uint16_t>(state->orb_flag[i])
		);
	}
	th02_s5_mima_wire_put_u16(
		wire, offset, static_cast<uint16_t>(state->orbs_gone_unused)
	);
	wire[offset++] = state->orb_variant;
	th02_s5_mima_wire_put_u16(
		wire, offset, static_cast<uint16_t>(state->orb_flight_center_x)
	);
	th02_s5_mima_wire_put_u16(
		wire, offset, static_cast<uint16_t>(state->orb_flight_center_y)
	);
	th02_s5_mima_wire_put_u16(
		wire, offset, static_cast<uint16_t>(state->orb_flight_radius)
	);
	wire[offset++] = static_cast<uint8_t>(state->orb_flight_radius_step);
	wire[offset++] = state->orb_flight_angle;
	wire[offset++] = state->orb_flight_detonated;
	th02_s5_mima_wire_put_u16(
		wire, offset, static_cast<uint16_t>(state->orb_center_x)
	);
	th02_s5_mima_wire_put_u16(
		wire, offset, static_cast<uint16_t>(state->orb_center_y)
	);
	th02_s5_mima_wire_put_u16(
		wire, offset, static_cast<uint16_t>(state->orb_radius)
	);
	wire[offset++] = state->orb_angle;
	wire[offset++] = state->bg_ring_radius;
	wire[offset++] = state->bg_circle_radius;
	wire[offset++] = state->bg_ring_col_head;
	wire[offset++] = state->bg_ring_col_tail;
	wire[offset++] = state->bg_circle_col;
	wire[offset++] = state->bg_ring_phase;
	wire[offset++] = state->ring_radius;
	wire[offset++] = state->bg_circle_radius_base;
	wire[offset++] = state->bg_circle_pulse_frame;
	wire[offset++] = state->flash_frame;
	wire[offset++] = state->ray_unused;
	wire[offset++] = state->ray_angle;
	wire[offset++] = state->ray_tone;
	wire[offset++] = state->spiral_angle;
	wire[offset++] = state->aim_angle_unused;
	wire[offset++] = state->charge_ring_radius;
	wire[offset++] = state->stream_angle;
	wire[offset++] = state->stream_speed;
	wire[offset++] = state->pair_angle;
	wire[offset++] = state->star_angle;
	wire[offset++] = static_cast<uint8_t>(state->star_direction);
	wire[offset++] = state->fan_angle;
	th02_s5_mima_wire_put_point(wire, offset, state->point_26CD6);
	th02_s5_mima_wire_put_point(wire, offset, state->point_26CDE);
	return (offset == TH02_S5_MIMA_WIRE_SIZE);
}

static bool16 near th02_s5_mima_state_wire_decode(
	th02_s5_mima_state_t *state,
	const uint8_t far *wire, uint16_t wire_size
)
{
	th02_s5_mima_state_t decoded;
	uint16_t offset = 0;
	int i;
	int page;

	if((state == 0) || (wire == 0) ||
		(wire_size != TH02_S5_MIMA_WIRE_SIZE)) {
		return false;
	}
	decoded.left_26C56 = static_cast<screen_x_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.muzzle_left = static_cast<screen_x_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.left_26C5A = static_cast<screen_x_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.x_26C5C = static_cast<screen_x_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.top_26C5E = static_cast<screen_y_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.muzzle_top = static_cast<screen_y_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.top_26C62 = static_cast<screen_y_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.y_26C64 = static_cast<screen_y_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.velocity_y = static_cast<int16_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.damage_multiplier = static_cast<int16_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.phase = static_cast<int16_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.pattern = static_cast<int16_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.patterns_this_phase = static_cast<int16_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.all_patterns = wire[offset++];
	decoded.phase_damage_max = static_cast<int16_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.patterns_max = static_cast<int16_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.pattern_count = static_cast<int16_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.patterns_until_vulnerable = static_cast<int16_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	for(page = 0; page < PAGE_COUNT; page++) {
		for(i = 0; i < TH02_S5_MIMA_ORB_SLOTS; i++) {
			decoded.orb_left_on_page[page][i] = static_cast<screen_x_t>(
				th02_s5_mima_wire_take_u16(wire, offset)
			);
		}
	}
	for(page = 0; page < PAGE_COUNT; page++) {
		for(i = 0; i < TH02_S5_MIMA_ORB_SLOTS; i++) {
			decoded.orb_top_on_page[page][i] = static_cast<screen_y_t>(
				th02_s5_mima_wire_take_u16(wire, offset)
			);
		}
	}
	for(i = 0; i < TH02_S5_MIMA_ORB_SLOTS; i++) {
		decoded.orb_flag[i] = static_cast<int16_t>(
			th02_s5_mima_wire_take_u16(wire, offset)
		);
	}
	decoded.orbs_gone_unused = static_cast<int16_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.orb_variant = wire[offset++];
	decoded.orb_flight_center_x = static_cast<int16_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.orb_flight_center_y = static_cast<int16_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.orb_flight_radius = static_cast<int16_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.orb_flight_radius_step = static_cast<int8_t>(wire[offset++]);
	decoded.orb_flight_angle = wire[offset++];
	decoded.orb_flight_detonated = wire[offset++];
	decoded.orb_center_x = static_cast<screen_x_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.orb_center_y = static_cast<screen_y_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.orb_radius = static_cast<int16_t>(
		th02_s5_mima_wire_take_u16(wire, offset)
	);
	decoded.orb_angle = wire[offset++];
	decoded.bg_ring_radius = wire[offset++];
	decoded.bg_circle_radius = wire[offset++];
	decoded.bg_ring_col_head = wire[offset++];
	decoded.bg_ring_col_tail = wire[offset++];
	decoded.bg_circle_col = wire[offset++];
	decoded.bg_ring_phase = wire[offset++];
	decoded.ring_radius = wire[offset++];
	decoded.bg_circle_radius_base = wire[offset++];
	decoded.bg_circle_pulse_frame = wire[offset++];
	decoded.flash_frame = wire[offset++];
	decoded.ray_unused = wire[offset++];
	decoded.ray_angle = wire[offset++];
	decoded.ray_tone = wire[offset++];
	decoded.spiral_angle = wire[offset++];
	decoded.aim_angle_unused = wire[offset++];
	decoded.charge_ring_radius = wire[offset++];
	decoded.stream_angle = wire[offset++];
	decoded.stream_speed = wire[offset++];
	decoded.pair_angle = wire[offset++];
	decoded.star_angle = wire[offset++];
	decoded.star_direction = static_cast<int8_t>(wire[offset++]);
	decoded.fan_angle = wire[offset++];
	th02_s5_mima_wire_take_point(decoded.point_26CD6, wire, offset);
	th02_s5_mima_wire_take_point(decoded.point_26CDE, wire, offset);
	if((offset != TH02_S5_MIMA_WIRE_SIZE) ||
		!th02_s5_mima_state_validate(&decoded)) {
		return false;
	}
	*state = decoded;
	return true;
}

bool16 far th02_s5_mima_state_wire_capture(
	uint8_t far *wire, uint16_t wire_size
)
{
	th02_s5_mima_state_t captured;

	return (
		th02_s5_mima_state_capture(&captured) &&
		th02_s5_mima_state_wire_encode(wire, wire_size, &captured)
	);
}

bool16 far th02_s5_mima_state_wire_valid(
	const uint8_t far *wire, uint16_t wire_size
)
{
	th02_s5_mima_state_t decoded;

	return th02_s5_mima_state_wire_decode(&decoded, wire, wire_size);
}
