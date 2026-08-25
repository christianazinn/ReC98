#ifndef TH02_MAIN_S5_ACTOR_HPP
#define TH02_MAIN_S5_ACTOR_HPP

#include "platform.h"
#include "pc98.h"

static const int TH02_S5_MIMA_ORB_SLOTS = 8;

// Pointer-free Stage 5 Mima state. Generic boss fields, the shared background
// particle pool, palette state, and stage callbacks belong to their own exact
// checkpoint groups.
struct th02_s5_mima_state_t {
	screen_x_t left_26C56;
	screen_x_t muzzle_left;
	screen_x_t left_26C5A;
	screen_x_t x_26C5C;
	screen_y_t top_26C5E;
	screen_y_t muzzle_top;
	screen_y_t top_26C62;
	screen_y_t y_26C64;
	int16_t velocity_y;
	int16_t damage_multiplier;
	int16_t phase;
	int16_t pattern;
	int16_t patterns_this_phase;
	uint8_t all_patterns;
	int16_t phase_damage_max;
	int16_t patterns_max;
	int16_t pattern_count;
	int16_t patterns_until_vulnerable;
	screen_x_t orb_left_on_page[PAGE_COUNT][TH02_S5_MIMA_ORB_SLOTS];
	screen_y_t orb_top_on_page[PAGE_COUNT][TH02_S5_MIMA_ORB_SLOTS];
	int16_t orb_flag[TH02_S5_MIMA_ORB_SLOTS];
	int16_t orbs_gone_unused;
	uint8_t orb_variant;
	int16_t orb_flight_center_x;
	int16_t orb_flight_center_y;
	int16_t orb_flight_radius;
	int8_t orb_flight_radius_step;
	uint8_t orb_flight_angle;
	uint8_t orb_flight_detonated;
	screen_x_t orb_center_x;
	screen_y_t orb_center_y;
	int16_t orb_radius;
	uint8_t orb_angle;
	uint8_t bg_ring_radius;
	uint8_t bg_circle_radius;
	uint8_t bg_ring_col_head;
	uint8_t bg_ring_col_tail;
	uint8_t bg_circle_col;
	uint8_t bg_ring_phase;
	uint8_t ring_radius;
	uint8_t bg_circle_radius_base;
	uint8_t bg_circle_pulse_frame;
	uint8_t flash_frame;
	uint8_t ray_unused;
	uint8_t ray_angle;
	uint8_t ray_tone;
	uint8_t spiral_angle;
	uint8_t aim_angle_unused;
	uint8_t charge_ring_radius;
	uint8_t stream_angle;
	uint8_t stream_speed;
	uint8_t pair_angle;
	uint8_t star_angle;
	int8_t star_direction;
	uint8_t fan_angle;
	screen_point_t point_26CD6;
	screen_point_t point_26CDE;
};

// The Stage 5 boss background uses the Stage 4 tile-copy helper. Its logical
// top is mutable field state and is kept separate from Mima's actor payload.
struct th02_s5_field_state_t {
	vram_y_t tile_top;
};

enum th02_s5_mima_clean_target_t {
	T2S5_MIMA_BOSS_START,
};

bool16 far th02_s5_mima_state_capture(th02_s5_mima_state_t *state);
bool16 far th02_s5_mima_state_apply(const th02_s5_mima_state_t *state);
bool16 far th02_s5_field_state_capture(th02_s5_field_state_t *state);
bool16 far th02_s5_field_state_apply(const th02_s5_field_state_t *state);

// This owns only the post-presentation Mima actor state. A later dispatcher
// must provide the common terminal field, stage assets, callback promotion,
// BGM, and first redraw before this target is exposed to players.
bool16 far th02_s5_mima_clean_init(th02_s5_mima_clean_target_t target);

#endif /* TH02_MAIN_S5_ACTOR_HPP */
