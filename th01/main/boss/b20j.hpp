#ifndef TH01_MAIN_BOSS_B20J_HPP
#define TH01_MAIN_BOSS_B20J_HPP

#include "platform.h"

enum {
	T1BOSS_KONNGARA_CHECKPOINT_OWNER = 7,
	T1BOSS_KONNGARA_CHECKPOINT_SCHEMA = 1,
	T1BOSS_KONNGARA_CHECKPOINT_SIZE = 4,
};

// Konngara is currently restorable only at the canonical phase-0 seam. All
// live state represented by this tag is fixed and revalidated before capture.
struct t1boss_konngara_checkpoint_t {
	uint8_t owner;
	uint8_t schema;
	int8_t phase;
	uint8_t reserved;
};

typedef char t1boss_konngara_checkpoint_size_check[
	(sizeof(t1boss_konngara_checkpoint_t) == T1BOSS_KONNGARA_CHECKPOINT_SIZE)
		? 1 : -1
];

bool16 t1boss_konngara_checkpoint_validate(
	const t1boss_konngara_checkpoint_t *checkpoint
);
bool16 t1boss_konngara_checkpoint_capture(
	t1boss_konngara_checkpoint_t *checkpoint
);
bool16 t1boss_konngara_ckpt_apply_loaded(
	const t1boss_konngara_checkpoint_t *checkpoint
);

#endif /* TH01_MAIN_BOSS_B20J_HPP */
