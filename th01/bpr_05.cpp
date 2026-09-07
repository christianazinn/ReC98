#pragma option -zCT1B05PRA_TEXT -G-
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
#include "th01/main/boss/b05.hpp"
#pragma codeseg T1B05PRA_TEXT

bool16 t1boss_singyoku_practice_construct(uint8_t target)
{
	t1boss_singyoku_checkpoint_t start;

	if((frame_since_start_of_binary != 0) ||
	   (boss_entity_0.bos_slot != 0) || (boss_entity_0.bos_image_count < 8) ||
	   (boss_entity_1.bos_slot != 1) || (boss_entity_1.bos_image_count < 3) ||
	   (boss_entity_2.bos_slot != 2) || (boss_entity_2.bos_image_count < 5) ||
	   boss_entity_0.loading || boss_entity_1.loading || boss_entity_2.loading) {
		return false;
	}
	if(target == T1RPBPT_SINGYOKU_FIRST_COMBAT) {
		return t1boss_singyoku_practice_boss_phase_apply(target);
	}
	if((target != T1RPBPT_SINGYOKU_PHASE_2) ||
	   !t1boss_singyoku_practice_boss_phase_apply(T1RPBPT_SINGYOKU_FIRST_COMBAT)) {
		return false;
	}

	start.owner = T1BOSS_SINGYOKU_CHECKPOINT_OWNER;
	start.schema = T1BOSS_SINGYOKU_CHECKPOINT_SCHEMA;
	start.phase = 2;
	start.reserved_0 = 0;
	start.phase_frame = 0;
	start.hp = 6;
	start.invincibility_frame = 0;
	start.pattern_value = (
		(rank == RANK_EASY) ? 4 : (rank == RANK_NORMAL) ? 4 :
		(rank == RANK_HARD) ? 5 : (rank == RANK_LUNATIC) ? 6 : 4
	);
	start.pattern_cur = 0;
	start.hit_invincible = false;
	start.initial_hp_rendered = true;
	start.slam_velocity_x = 0;
	start.slam_velocity_y = 0;
	start.sphere_left = (PLAYFIELD_CENTER_X - 48);
	start.sphere_top = (PLAYFIELD_TOP + (PLAYFIELD_H / 21) * 5 - 48);
	start.halfcircle_angle = 0;
	start.halfcircle_direction = 0;
	start.sphere_image = 0;
	start.person_image = 0;
	start.reserved[0] = 0;
	start.reserved[1] = 0;
	if(!t1boss_singyoku_ckpt_apply_loaded(&start)) {
		return false;
	}
	boss_palette_show();
	stage_palette_set(z_Palettes);
	boss_palette_snap();
	return t1boss_singyoku_presentation_reconstruct(&start);
}

#pragma codeseg
