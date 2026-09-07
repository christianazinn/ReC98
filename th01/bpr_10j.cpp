#pragma option -zCT1B10JPRA_TEXT -G-
#include <string.h>
#include "th01/boss_practice.hpp"
#include "th01/rank.h"
#include "th01/resident.hpp"
#include "th01/hardware/graph.h"
#include "th01/hardware/palette.h"
#include "th01/main/boss/entity_a.hpp"
#include "th01/main/boss/palette.hpp"
#include "th01/main/playfld.hpp"
#include "th01/main/stage/palette.hpp"
#include "th01/main/stage/stages.hpp"
#include "th01/main/hud/hp.hpp"
#include "th01/main/debug.hpp"
#include "th01/main/boss/b10j.hpp"
#include "th01/formats/ptn_data.hpp"
#include "th01/formats/ptn.hpp"
void mima_put_still_both(void);
#pragma codeseg T1B10JPRA_TEXT

bool16 t1boss_mima_practice_construct(uint8_t target)
{
	t1boss_mima_checkpoint_t start;
	int i;

	if((frame_since_start_of_binary != 0) ||
	   (boss_entity_0.bos_slot != 0) || (boss_entity_0.bos_image_count < 1) ||
	   (boss_entity_1.bos_slot != 1) || (boss_entity_1.bos_image_count < 5) ||
	   boss_entity_0.loading || boss_entity_1.loading ||
	   !ptn_images[PTN_SLOT_BOSS_1] || (ptn_image_count[PTN_SLOT_BOSS_1] != 24) ||
	   !ptn_images[PTN_SLOT_BOSS_2]) {
		return false;
	}
	if(target == T1RPBPT_MIMA_FIRST_COMBAT) {
		return t1boss_mima_practice_first_combat_construct();
	}
	if((target != T1RPBPT_MIMA_PHASE_3) ||
	   (boss_phase != 0) || (boss_phase_frame != 0) || (boss_hp != 12) ||
	   (boss_entity_0.cur_left != (PLAYFIELD_CENTER_X - 64)) ||
	   (boss_entity_0.cur_top != PLAYFIELD_TOP) ||
	   (boss_entity_0.image() != 0) || (boss_entity_1.image() != 1) ||
	   boss_entity_0.hitbox_orb_inactive) {
		return false;
	}

	start.owner = T1BOSS_MIMA_CHECKPOINT_OWNER;
	start.schema = T1BOSS_MIMA_CHECKPOINT_SCHEMA;
	start.phase = 3;
	start.pattern = 0;
	start.phase_frame = 0;
	start.hp = 6;
	start.invincibility_frame = 0;
	start.pattern_state = 0;
	start.entity_left = (PLAYFIELD_CENTER_X - 64);
	start.entity_top = (PLAYFIELD_TOP + (PLAYFIELD_H / 42) * 17 - 80);
	start.target_left = 0;
	for(i = 0; i < 8; i++) {
		start.pillar_time[i] = 0;
		start.pillar_center_x[i] = 0;
		start.pillar_bottom[i] = 0;
	}
	for(i = 0; i < 4; i++) {
		start.laser_corner_x[i] = 0;
		start.laser_corner_y[i] = 0;
	}
	start.meteor_active = true;
	start.spreadin_interval = 4;
	start.spreadin_speed = 8;
	start.initial_hp_rendered = true;
	start.hit_invincible = false;
	start.hop = static_cast<uint8_t>(-1);
	start.hop_direction = 0;
	start.entity_image = 0;
	start.animation_image = 1;
	start.entity_hitbox_inactive = false;
	start.square_aimed_pellets_angle = 0;
	start.square_aimed_pellets_radius = 0;
	start.square_aimed_missiles_angle = 0;
	start.square_aimed_missiles_radius = 0;
	start.square_two_pellets_angle = 0;
	start.square_two_pellets_radius = 0;
	start.square_halfcircle_missiles_angle = 0;
	start.square_halfcircle_missiles_radius = 0;
	start.square_slow_spray_angle = 0;
	start.square_slow_spray_radius = 0;
	start.square_lasers_angle = 0;
	start.square_lasers_radius = 0;
	start.missile_angle = 0;
	start.pellet_angle = 0;
	start.reserved[0] = 0;
	start.reserved[1] = 0;
	if(!t1boss_mima_ckpt_apply_loaded(&start)) {
		return false;
	}
	mima_put_still_both();
	stage_palette_set(z_Palettes);
	boss_palette_snap();
	hud_hp_rerender(6);
	return true;
}

#pragma codeseg
