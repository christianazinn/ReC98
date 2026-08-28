#ifndef TH01_MAIN_BOSS_B15M_HPP
#define TH01_MAIN_BOSS_B15M_HPP

#include "platform.h"

enum {
	T1BOSS_ELIS_CHECKPOINT_OWNER = 5,
	T1BOSS_ELIS_CHECKPOINT_SCHEMA = 1,
	T1BOSS_ELIS_CHECKPOINT_ENTITY_COUNT = 3,
	T1BOSS_ELIS_CHECKPOINT_SIZE = 60,
};

// Pointer-free dynamic state for one normally loaded Elis .BOS entity.
struct t1boss_elis_entity_checkpoint_t {
	int16_t left;
	int16_t top;
	int16_t prev_left;
	int16_t prev_top;
	int16_t prev_delta_x;
	int16_t prev_delta_y;
	uint8_t image;
	uint8_t hitbox_inactive;
	int16_t lock_frame;
};

// The current private checkpoint is captured only at Elis' canonical
// pre-entrance gameplay-loop seam. Mid-fight and transition state remains
// unsupported and must fail closed.
struct t1boss_elis_checkpoint_t {
	uint8_t owner;
	uint8_t schema;
	int8_t phase;
	uint8_t reserved_0;
	int16_t phase_frame;
	int16_t hp;
	int16_t pattern_state;
	t1boss_elis_entity_checkpoint_t entity[
		T1BOSS_ELIS_CHECKPOINT_ENTITY_COUNT
	];
	uint8_t reserved[2];
};

typedef char t1boss_elis_entity_checkpoint_size_check[
	(sizeof(t1boss_elis_entity_checkpoint_t) == 16) ? 1 : -1
];
typedef char t1boss_elis_checkpoint_size_check[
	(sizeof(t1boss_elis_checkpoint_t) == T1BOSS_ELIS_CHECKPOINT_SIZE) ? 1 : -1
];

bool16 t1boss_elis_checkpoint_validate(
	const t1boss_elis_checkpoint_t *checkpoint
);
bool16 t1boss_elis_checkpoint_capture(
	t1boss_elis_checkpoint_t *checkpoint
);
bool16 t1boss_elis_checkpoint_apply(
	const t1boss_elis_checkpoint_t *checkpoint
);
bool16 t1boss_elis_ckpt_apply_loaded(
	const t1boss_elis_checkpoint_t *checkpoint
);

// Private fresh-process owner seam after Elis' native phase-0 entrance. This
// accepts only the still-loaded Makai Stage 15 pre-entrance state and never
// reads a serialized checkpoint.
bool16 t1boss_elis_practice_first_combat_apply(void);

#endif /* TH01_MAIN_BOSS_B15M_HPP */
