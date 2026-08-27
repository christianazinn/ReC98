#ifndef TH01_MAIN_BOSS_B20M_HPP
#define TH01_MAIN_BOSS_B20M_HPP

#include "platform.h"

enum {
	T1BOSS_SARIEL_CHECKPOINT_OWNER = 6,
	T1BOSS_SARIEL_CHECKPOINT_SCHEMA = 1,
	T1BOSS_SARIEL_CHECKPOINT_SIZE = 44,
};

struct t1boss_sariel_entity_ckpt_t {
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

struct t1boss_sariel_anim_ckpt_t {
	int16_t left;
	int16_t top;
	uint8_t image;
	uint8_t reserved;
};

// Sariel is currently restorable only at the canonical phase-0 seam after
// the native entrance. Later phases retain function-local simulation state.
struct t1boss_sariel_checkpoint_t {
	uint8_t owner;
	uint8_t schema;
	int8_t phase;
	uint8_t reserved_0;
	int16_t phase_frame;
	int16_t hp;
	int16_t hud_hp_first_white;
	int16_t hud_hp_first_redwhite;
	int16_t pattern_state;
	t1boss_sariel_entity_ckpt_t shield;
	t1boss_sariel_anim_ckpt_t dress;
	t1boss_sariel_anim_ckpt_t wand;
	uint8_t reserved[2];
};

typedef char t1boss_sariel_entity_size_check[
	(sizeof(t1boss_sariel_entity_ckpt_t) == 16) ? 1 : -1
];
typedef char t1boss_sariel_anim_size_check[
	(sizeof(t1boss_sariel_anim_ckpt_t) == 6) ? 1 : -1
];
typedef char t1boss_sariel_checkpoint_size_check[
	(sizeof(t1boss_sariel_checkpoint_t) == T1BOSS_SARIEL_CHECKPOINT_SIZE)
		? 1 : -1
];

bool16 t1boss_sariel_checkpoint_validate(
	const t1boss_sariel_checkpoint_t *checkpoint
);
bool16 t1boss_sariel_checkpoint_capture(
	t1boss_sariel_checkpoint_t *checkpoint
);
bool16 t1boss_sariel_ckpt_apply_loaded(
	const t1boss_sariel_checkpoint_t *checkpoint
);

#endif /* TH01_MAIN_BOSS_B20M_HPP */
