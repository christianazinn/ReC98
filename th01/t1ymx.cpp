// Private YuugenMagan phase-1 measurement constructor. This file stays a
// separate tail object so it cannot perturb b10m.cpp's original codegen.

#pragma option -zCT1YMX_TEXT -G-

#include "th01/t1ymx.hpp"
#include "th01/rank.h"

#if T1REPLAY_YUUGENMAGAN_FIRST_COMBAT_TRACE

extern int8_t rank;

// b10m.cpp source facts, kept local rather than widening the boss owner's API:
// HP_TOTAL=16; phase-0 ends after frame 330; C_HIDDEN/C_DOWN/C_LEFT=0/3/4.
enum {
	T1YMX_PHASE_FIRST_COMBAT = 1,
	T1YMX_HP_TOTAL = 16,
	T1YMX_PENTAGRAM_PREPARE_1 = 0,
	T1YMX_EYE_COUNT = 5,
	T1YMX_EYE_CEL_HIDDEN = 0,
	T1YMX_EYE_CEL_DOWN = 3,
	T1YMX_EYE_CEL_LEFT = 4,
	T1YMX_LOCK_FRAME = ((330 + 1) % 4),
};

bool16 t1boss_yuugenmagan_first_combat_construct(
	t1boss_yuugenmagan_checkpoint_t *checkpoint
)
{
	int i;

	// Hard and Lunatic phase 0 emit pellets before this seam. Their live pool
	// is not part of this owner-local constructor, so the witness rejects them.
	if(
		!checkpoint ||
		((rank != RANK_EASY) && (rank != RANK_NORMAL))
	) {
		return false;
	}
	checkpoint->owner = T1BOSS_YUUGENMAGAN_CHECKPOINT_OWNER;
	checkpoint->schema = T1BOSS_YUUGENMAGAN_CHECKPOINT_SCHEMA;
	checkpoint->phase = T1YMX_PHASE_FIRST_COMBAT;
	checkpoint->reserved_0 = 0;
	checkpoint->phase_frame = 0;
	checkpoint->hp = T1YMX_HP_TOTAL;
	checkpoint->invincibility_frame = 0;
	checkpoint->pattern_interval = (
		(rank == RANK_EASY) ? 350 : 300
	);
	checkpoint->u1 = 0;
	checkpoint->u2 = 0;
	checkpoint->target_left = 0;
	checkpoint->unused_distance = 0;
	checkpoint->after_hit_frame = 0;
	checkpoint->u3 = 0;
	checkpoint->initial_hp_rendered = false;
	checkpoint->angle = 0;
	checkpoint->angle_missile_southeast = 0;
	checkpoint->hit_invincible = false;
	checkpoint->pentagram_phase = T1YMX_PENTAGRAM_PREPARE_1;
	checkpoint->pentagram_angle = 0;
	for(i = 0; i < T1YMX_EYE_COUNT; i++) {
		checkpoint->line_x[i] = 0;
		checkpoint->line_y[i] = 0;
	}
	checkpoint->line_radius = 0;
	checkpoint->line_center_x = 0;
	checkpoint->line_center_y = 0;
	checkpoint->line_velocity_x = 0;
	checkpoint->line_velocity_y = 0;
	checkpoint->eye_image[0] = T1YMX_EYE_CEL_LEFT;
	checkpoint->eye_image[1] = T1YMX_EYE_CEL_DOWN;
	checkpoint->eye_image[2] = T1YMX_EYE_CEL_HIDDEN;
	checkpoint->eye_image[3] = T1YMX_EYE_CEL_HIDDEN;
	checkpoint->eye_image[4] = T1YMX_EYE_CEL_HIDDEN;
	checkpoint->eye_hitbox_inactive[0] = false;
	checkpoint->eye_hitbox_inactive[1] = false;
	checkpoint->eye_hitbox_inactive[2] = true;
	checkpoint->eye_hitbox_inactive[3] = true;
	checkpoint->eye_hitbox_inactive[4] = true;
	for(i = 0; i < T1YMX_EYE_COUNT; i++) {
		checkpoint->eye_lock_frame[i] = T1YMX_LOCK_FRAME;
	}
	checkpoint->reserved[0] = 0;
	return t1boss_yuugenmagan_checkpoint_validate(checkpoint);
}

#endif
