#ifndef TH01_MAIN_BOSS_B10M_HPP
#define TH01_MAIN_BOSS_B10M_HPP

#include "platform.h"

enum {
	T1BOSS_YUUGENMAGAN_CHECKPOINT_OWNER = 2,
	T1BOSS_YUUGENMAGAN_CHECKPOINT_SCHEMA = 1,
	T1BOSS_YUUGENMAGAN_CHECKPOINT_SIZE = 77,
};

// Pointer-free state owned specifically by b10m.cpp. The owner accepts only
// ordinary combat phases; entrance, palette transitions, and defeat are not
// exact restore boundaries.
struct t1boss_yuugenmagan_checkpoint_t {
	uint8_t owner;
	uint8_t schema;
	int8_t phase;
	uint8_t reserved_0;
	int16_t phase_frame;
	int16_t hp;
	int16_t invincibility_frame;
	int16_t pattern_interval;
	int16_t u1;
	int16_t u2;
	int16_t target_left;
	int16_t unused_distance;
	int16_t after_hit_frame;
	int8_t u3;
	uint8_t initial_hp_rendered;
	uint8_t angle;
	uint8_t angle_missile_southeast;
	uint8_t hit_invincible;
	int16_t pentagram_phase;
	int16_t pentagram_angle;
	int16_t line_x[5];
	int16_t line_y[5];
	int16_t line_radius;
	int16_t line_center_x;
	int16_t line_center_y;
	int16_t line_velocity_x;
	int16_t line_velocity_y;
	uint8_t eye_image[5];
	uint8_t eye_hitbox_inactive[5];
	int8_t eye_lock_frame[5];
	uint8_t reserved[1];
};

typedef char t1boss_yuugenmagan_checkpoint_size_check[
	(sizeof(t1boss_yuugenmagan_checkpoint_t) ==
	 T1BOSS_YUUGENMAGAN_CHECKPOINT_SIZE) ? 1 : -1
];

bool16 t1boss_yuugenmagan_checkpoint_validate(
	const t1boss_yuugenmagan_checkpoint_t *checkpoint
);
bool16 t1boss_yuugenmagan_checkpoint_capture(
	t1boss_yuugenmagan_checkpoint_t *checkpoint
);
bool16 t1boss_yuugenmagan_checkpoint_apply(
	const t1boss_yuugenmagan_checkpoint_t *checkpoint
);
bool16 t1boss_yuugenmagan_ckpt_apply_loaded(
	const t1boss_yuugenmagan_checkpoint_t *checkpoint
);

#endif /* TH01_MAIN_BOSS_B10M_HPP */
