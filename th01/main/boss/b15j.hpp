#ifndef TH01_MAIN_BOSS_B15J_HPP
#define TH01_MAIN_BOSS_B15J_HPP

#include "platform.h"

enum {
	T1BOSS_KIKURI_CHECKPOINT_OWNER = 4,
	T1BOSS_KIKURI_CHECKPOINT_SCHEMA = 1,
	T1BOSS_KIKURI_CHECKPOINT_SOUL_COUNT = 2,
	T1BOSS_KIKURI_CHECKPOINT_TEAR_COUNT = 10,
	T1BOSS_KIKURI_CHECKPOINT_COLOR_COUNT = 16,
	T1BOSS_KIKURI_CHECKPOINT_COMPONENT_COUNT = 3,
	T1BOSS_KIKURI_CHECKPOINT_SIZE = 264,
};

// Pointer-free state owned specifically by b15j.cpp. It covers ordinary
// Kikuri combat only; the blocking entrance, palette transition, and defeat
// continuation do not provide an exact restore seam.
struct t1boss_kikuri_checkpoint_t {
	uint8_t owner;
	uint8_t schema;
	int8_t phase;
	uint8_t reserved_0;
	int16_t phase_frame;
	int16_t hp;
	int16_t invincibility_frame;
	int16_t pattern_state;
	int16_t phase_u1;
	int16_t patterns_done;
	int16_t phase_2_distance;
	int16_t phase_6_random_range_x_half;
	int16_t soul_left[T1BOSS_KIKURI_CHECKPOINT_SOUL_COUNT];
	int16_t soul_top[T1BOSS_KIKURI_CHECKPOINT_SOUL_COUNT];
	int16_t soul_prev_left[T1BOSS_KIKURI_CHECKPOINT_SOUL_COUNT];
	int16_t soul_prev_top[T1BOSS_KIKURI_CHECKPOINT_SOUL_COUNT];
	int16_t soul_prev_delta_x[T1BOSS_KIKURI_CHECKPOINT_SOUL_COUNT];
	int16_t soul_prev_delta_y[T1BOSS_KIKURI_CHECKPOINT_SOUL_COUNT];
	uint8_t soul_image[T1BOSS_KIKURI_CHECKPOINT_SOUL_COUNT];
	int16_t soul_lock_frame[T1BOSS_KIKURI_CHECKPOINT_SOUL_COUNT];
	int8_t tear_anim_frame[T1BOSS_KIKURI_CHECKPOINT_TEAR_COUNT];
	int16_t tear_left[T1BOSS_KIKURI_CHECKPOINT_TEAR_COUNT];
	int16_t tear_top[T1BOSS_KIKURI_CHECKPOINT_TEAR_COUNT];
	int16_t tear_prev_left[T1BOSS_KIKURI_CHECKPOINT_TEAR_COUNT];
	int16_t tear_prev_top[T1BOSS_KIKURI_CHECKPOINT_TEAR_COUNT];
	int16_t tear_prev_delta_x[T1BOSS_KIKURI_CHECKPOINT_TEAR_COUNT];
	int16_t tear_prev_delta_y[T1BOSS_KIKURI_CHECKPOINT_TEAR_COUNT];
	uint8_t tear_image[T1BOSS_KIKURI_CHECKPOINT_TEAR_COUNT];
	int16_t tear_lock_frame[T1BOSS_KIKURI_CHECKPOINT_TEAR_COUNT];
	uint8_t hit_invincible;
	uint8_t initial_hp_rendered;
	uint8_t phase_2_angle;
	uint8_t phase_2_drift;
	uint8_t phase_6_spiral_angle;
	uint8_t boss_palette[
		T1BOSS_KIKURI_CHECKPOINT_COLOR_COUNT
	][T1BOSS_KIKURI_CHECKPOINT_COMPONENT_COUNT];
	uint8_t reserved[1];
};

typedef char t1boss_kikuri_checkpoint_size_check[
	(sizeof(t1boss_kikuri_checkpoint_t) == T1BOSS_KIKURI_CHECKPOINT_SIZE) ? 1 : -1
];

bool16 t1boss_kikuri_checkpoint_validate(
	const t1boss_kikuri_checkpoint_t *checkpoint
);
bool16 t1boss_kikuri_checkpoint_capture(
	t1boss_kikuri_checkpoint_t *checkpoint
);
bool16 t1boss_kikuri_checkpoint_apply(
	const t1boss_kikuri_checkpoint_t *checkpoint
);

#endif /* TH01_MAIN_BOSS_B15J_HPP */
