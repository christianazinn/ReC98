#pragma option -zCT1B15JPRA_TEXT -G-
#include <string.h>
#include "th01/boss_practice.hpp"
#include "th01/replay_milestone.hpp"
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
#include "th01/formats/ptn_data.hpp"
#include "th01/main/boss/b15j.hpp"
extern int8_t kikuri_phase;
extern CBossEntity souls_raw[5];
extern CBossEntity tears[10];

bool16 t1boss_kikuri_practice_construct(uint8_t target)
{
	t1boss_kikuri_checkpoint_t start;
	int i;
	int col;
	int comp;
	if((target < T1RPBPT_KIKURI_PHASE_2) ||
	   (target > T1RPBPT_KIKURI_PHASE_6) ||
	   (frame_since_start_of_binary != 0) ||
	   (kikuri_phase != 0) || (boss_phase_frame != 0) || (boss_hp != 14) ||
	   !ptn_images[PTN_SLOT_BOSS_1] || (ptn_image_count[PTN_SLOT_BOSS_1] != 1)) {
#if T1REPLAY_PROCESS_MILESTONES
		t1replay_process_milestone(T1RPM_KIKURI_INITIAL_STATE_REJECTED);
		if(kikuri_phase != 0) {
			t1replay_process_milestone(T1RPM_KIKURI_PHASE_NOT_ZERO);
		}
		if(boss_phase_frame != 0) {
			t1replay_process_milestone(T1RPM_KIKURI_FRAME_NOT_ZERO);
		}
		if(boss_hp != 14) {
			t1replay_process_milestone(T1RPM_KIKURI_HP_NOT_FULL);
		}
		if(!ptn_images[PTN_SLOT_BOSS_1]) {
			t1replay_process_milestone(T1RPM_KIKURI_PTN_MISSING);
		}
		if(ptn_image_count[PTN_SLOT_BOSS_1] != 1) {
			t1replay_process_milestone(T1RPM_KIKURI_PTN_COUNT_WRONG);
		}
#endif
		return false;
	}
	for(i = 0; i < 2; i++) {
		if((souls_raw[i].bos_slot != 0) || (souls_raw[i].bos_image_count != 3) ||
		   souls_raw[i].loading) {
#if T1REPLAY_PROCESS_MILESTONES
			t1replay_process_milestone(T1RPM_KIKURI_SOUL_RESOURCE_REJECTED);
#endif
			return false;
		}
	}
	for(i = 0; i < 10; i++) {
		if((tears[i].bos_slot != 1) || (tears[i].bos_image_count < 1) ||
		   tears[i].loading) {
#if T1REPLAY_PROCESS_MILESTONES
			t1replay_process_milestone(T1RPM_KIKURI_TEAR_RESOURCE_REJECTED);
#endif
			return false;
		}
	}
	memset(&start, 0, sizeof(start));
	start.owner = T1BOSS_KIKURI_CHECKPOINT_OWNER;
	start.schema = T1BOSS_KIKURI_CHECKPOINT_SCHEMA;
	start.phase = t1boss_practice_phase(target);
	start.hp = (start.phase == 2) ? 14 : ((start.phase == 6) ? 6 : 10);
	start.initial_hp_rendered = true;
	start.phase_2_distance = (rank == RANK_LUNATIC) ? 50 : 90;
	start.pattern_state = (start.phase == 5) ?
		((rank == RANK_EASY) ? 200 : ((rank == RANK_NORMAL) ? 160 :
		((rank == RANK_HARD) ? 140 : 120))) : (4 - rank);
	// The exact-checkpoint owner validates a mature clock. Apply its complete
	// state without executing that frame, then install the direct-entry clock.
	start.phase_frame = 100;
	for(i = 0; i < 2; i++) {
		start.soul_left[i] = start.soul_prev_left[i] = (start.phase < 5) ? 0 :
			(i ? (PLAYFIELD_RIGHT - PLAYFIELD_W / 20 - 32) :
			(PLAYFIELD_LEFT + PLAYFIELD_W / 20));
		start.soul_top[i] = start.soul_prev_top[i] =
			(start.phase < 5) ? 0 : (PLAYFIELD_TOP + 32);
	}
	boss_palette_show();
	if(start.phase >= 4) z_palette_set_show(5, 15, 11, 10);
	for(col = 0; col < 16; col++) {
		for(comp = 0; comp < 3; comp++) {
			start.boss_palette[col][comp] = z_Palettes[col].v[comp];
		}
	}
	if(!t1boss_kikuri_ckpt_apply_loaded(&start)) {
#if T1REPLAY_PROCESS_MILESTONES
		t1replay_process_milestone(T1RPM_KIKURI_SNAPSHOT_REJECTED);
#endif
		return false;
	}
	boss_phase_frame = 0;
	stage_palette_set(z_Palettes);
	boss_palette_snap();
	graph_accesspage_func(1);
	graph_copy_accessed_page_to_other();
	graph_accesspage_func(0);
	if(start.phase >= 5) {
		for(i = 0; i < 2; i++) {
			souls_raw[i].put_8(souls_raw[i].cur_left, souls_raw[i].cur_top, 0);
		}
	}
	hud_hp_rerender(boss_hp);
	return true;
}
