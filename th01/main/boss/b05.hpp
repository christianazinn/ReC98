#ifndef TH01_MAIN_BOSS_B05_HPP
#define TH01_MAIN_BOSS_B05_HPP

#include "platform.h"
#include "th01/replay_format.hpp"

enum {
	T1BOSS_SINGYOKU_CHECKPOINT_OWNER = 1,
	T1BOSS_SINGYOKU_CHECKPOINT_SCHEMA = 1,
	T1BOSS_SINGYOKU_CHECKPOINT_SIZE = 32,
};

// Pointer-free state owned specifically by b05.cpp. Phase 8 is excluded: its
// blocking defeat and route-selection continuation has no ordinary input seam.
struct t1boss_singyoku_checkpoint_t {
	uint8_t owner;
	uint8_t schema;
	int8_t phase;
	uint8_t reserved_0;
	int16_t phase_frame;
	int16_t hp;
	int16_t invincibility_frame;
	int16_t pattern_value;
	int16_t pattern_cur;
	uint16_t hit_invincible;
	uint16_t initial_hp_rendered;
	int16_t slam_velocity_x;
	int16_t slam_velocity_y;
	int16_t sphere_left;
	int16_t sphere_top;
	uint8_t halfcircle_angle;
	int8_t halfcircle_direction;
	int8_t sphere_image;
	int8_t person_image;
	uint8_t reserved[2];
};

typedef char t1boss_singyoku_checkpoint_size_check[
	(sizeof(t1boss_singyoku_checkpoint_t) ==
	 T1BOSS_SINGYOKU_CHECKPOINT_SIZE) ? 1 : -1
];

bool16 t1boss_singyoku_checkpoint_validate(
	const t1boss_singyoku_checkpoint_t *checkpoint
);
bool16 t1boss_singyoku_checkpoint_capture(
	t1boss_singyoku_checkpoint_t *checkpoint
);
bool16 t1boss_singyoku_checkpoint_apply(
	const t1boss_singyoku_checkpoint_t *checkpoint
);
bool16 t1boss_singyoku_ckpt_apply_loaded(
	const t1boss_singyoku_checkpoint_t *checkpoint
);

// Constructs the one public direct-start seam from native post-entrance
// constants. It never accepts a serialized checkpoint payload.
bool16 t1boss_singyoku_practice_boss_phase_apply(uint8_t target);

#if T1REPLAY_CHECKPOINT_PRIVATE_RESTORE
// Private source-owned phase-2 constructor. It deliberately has no public
// Practice or replay target and accepts no serialized state.
bool16 t1boss_singyoku_phase2_owner_construct(void);
#endif

#if T1REPLAY_CHECKPOINT_RESTORE || T1REPLAY_PIXEL_TRACE || T1REPLAY_PRACTICE_BOSS_PHASE
// Exact-restore presentation. These functions consume only the already-loaded
// owner and paint no dynamic object onto page 1.
bool16 t1boss_singyoku_presentation_validate(
	const t1boss_singyoku_checkpoint_t *checkpoint
);
bool16 t1boss_singyoku_presentation_reconstruct(
	const t1boss_singyoku_checkpoint_t *checkpoint
);
#endif

#endif /* TH01_MAIN_BOSS_B05_HPP */
