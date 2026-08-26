#ifndef TH01_MAIN_BOSS_B10J_HPP
#define TH01_MAIN_BOSS_B10J_HPP

#include "platform.h"

enum {
	T1BOSS_MIMA_CHECKPOINT_OWNER = 3,
	T1BOSS_MIMA_CHECKPOINT_SCHEMA = 1,
	T1BOSS_MIMA_CHECKPOINT_SIZE = 108,
};

// Pointer-free state owned specifically by b10j.cpp. The owner accepts only
// ordinary combat phases; entrance, phase transition, and defeat are not
// exact restore boundaries.
struct t1boss_mima_checkpoint_t {
	uint8_t owner;
	uint8_t schema;
	int8_t phase;
	uint8_t pattern;
	int16_t phase_frame;
	int16_t hp;
	int16_t invincibility_frame;
	int16_t pattern_state;
	int16_t entity_left;
	int16_t entity_top;
	int16_t target_left;
	int16_t pillar_time[8];
	int16_t pillar_center_x[8];
	int16_t pillar_bottom[8];
	int16_t laser_corner_x[4];
	int16_t laser_corner_y[4];
	uint8_t meteor_active;
	uint8_t spreadin_interval;
	uint8_t spreadin_speed;
	uint8_t initial_hp_rendered;
	uint8_t hit_invincible;
	uint8_t hop;
	uint8_t hop_direction;
	uint8_t entity_image;
	uint8_t animation_image;
	uint8_t entity_hitbox_inactive;
	uint8_t square_aimed_pellets_angle;
	uint8_t square_aimed_pellets_radius;
	uint8_t square_aimed_missiles_angle;
	uint8_t square_aimed_missiles_radius;
	uint8_t square_two_pellets_angle;
	uint8_t square_two_pellets_radius;
	uint8_t square_halfcircle_missiles_angle;
	uint8_t square_halfcircle_missiles_radius;
	uint8_t square_slow_spray_angle;
	uint8_t square_slow_spray_radius;
	uint8_t square_lasers_angle;
	uint8_t square_lasers_radius;
	uint8_t missile_angle;
	uint8_t pellet_angle;
	uint8_t reserved[2];
};

typedef char t1boss_mima_checkpoint_size_check[
	(sizeof(t1boss_mima_checkpoint_t) == T1BOSS_MIMA_CHECKPOINT_SIZE) ? 1 : -1
];

bool16 t1boss_mima_checkpoint_validate(
	const t1boss_mima_checkpoint_t *checkpoint
);
bool16 t1boss_mima_checkpoint_capture(
	t1boss_mima_checkpoint_t *checkpoint
);
bool16 t1boss_mima_checkpoint_apply(
	const t1boss_mima_checkpoint_t *checkpoint
);

#endif /* TH01_MAIN_BOSS_B10J_HPP */
