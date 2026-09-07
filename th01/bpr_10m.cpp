#pragma option -zCT1B10MPRA_TEXT -G-
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
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/v_colors.hpp"
#include "th01/math/polar.hpp"
#include "th01/main/bullet/missile.hpp"
#include "th01/formats/ptn_data.hpp"
#include "th01/main/boss/b10m.hpp"

enum { EYE_COUNT=5, C_HIDDEN=0, C_AHEAD=6,
	EF_WEST=1, EF_EAST=2, EF_SOUTHWEST=4, EF_SOUTHEAST=8, EF_NORTH=16,
	HP_TOTAL=16, HP_PHASE_1_END=15, HP_PHASE_3_END=12,
	HP_PHASE_5_END=10, HP_PHASE_7_END=8, HP_PHASE_9_END=2,
	PENTAGRAM_POINTS=5, PENTAGRAM_RADIUS_FINAL=64,
	PENTAGRAM_ANGLE_INITIAL=0xC0, COL_YOKOSHIMA=15 };
#define PTN_SLOT_MISSILE PTN_SLOT_BOSS_1

static CBossEntity& t1bp_ym_eye(int i)
{
	if(i == 0) return boss_entity_0;
	if(i == 1) return boss_entity_1;
	if(i == 2) return boss_entity_2;
	if(i == 3) return boss_entity_3;
	return boss_entity_4;
}
static int select_for_rank(int easy, int normal, int hard, int lunatic)
{
	return (rank == RANK_EASY) ? easy : ((rank == RANK_NORMAL) ? normal :
		((rank == RANK_HARD) ? hard : lunatic));
}
#pragma codeseg T1B10MPRA_TEXT

bool16 t1boss_yuugenmagan_practice_construct(uint8_t target)
{
	t1boss_yuugenmagan_checkpoint_t start;
	int i;
	int mask;
	if((target < T1RPBPT_YUUGENMAGAN_PHASE_1) ||
	   (target > T1RPBPT_YUUGENMAGAN_PHASE_13) ||
	   (frame_since_start_of_binary != 0) ||
	   (boss_phase != 0) || (boss_phase_frame != 0) ||
	   (boss_hp != HP_TOTAL) || !ptn_images[PTN_SLOT_MISSILE]) {
		return false;
	}
	for(i = 0; i < EYE_COUNT; i++) {
		CBossEntity& eye = t1bp_ym_eye(i);
		if((eye.bos_slot != 0) || (eye.bos_image_count < 7) || eye.loading) {
			return false;
		}
	}
	memset(&start, 0, sizeof(start));
	start.owner = T1BOSS_YUUGENMAGAN_CHECKPOINT_OWNER;
	start.schema = T1BOSS_YUUGENMAGAN_CHECKPOINT_SCHEMA;
	start.phase = (1 + (target - T1RPBPT_YUUGENMAGAN_PHASE_1) * 2);
	start.initial_hp_rendered = true;
	start.hp = (start.phase == 1) ? HP_TOTAL :
		((start.phase == 3) ? HP_PHASE_1_END :
		((start.phase == 5) ? HP_PHASE_3_END :
		((start.phase == 7) ? HP_PHASE_5_END :
		((start.phase == 9) ? HP_PHASE_7_END : HP_PHASE_9_END))));
	start.pattern_interval = (start.phase == 1) ? select_for_rank(350, 300, 200, 130) :
		((start.phase == 3) ? select_for_rank(8, 12, 16, 20) :
		((start.phase == 5) ? select_for_rank(12, 8, 4, 2) :
		((start.phase == 13) ? select_for_rank(24, 14, 10, 8) :
		select_for_rank(10, 16, 20, 24))));
	mask = ((start.phase == 1) || (start.phase == 5)) ? (EF_WEST | EF_EAST) :
		(((start.phase == 3) || (start.phase == 7)) ? (EF_SOUTHWEST | EF_SOUTHEAST) :
		((start.phase == 9) ? EF_NORTH : ((start.phase == 11) ? 31 : EF_WEST)));
	for(i = 0; i < EYE_COUNT; i++) {
		start.eye_hitbox_inactive[i] = !(mask & (1 << i));
		start.eye_image[i] = start.eye_hitbox_inactive[i] ? C_HIDDEN : C_AHEAD;
	}
	if(start.phase == 13) {
		start.u3 = EF_WEST;
		start.line_radius = PENTAGRAM_RADIUS_FINAL;
		start.line_center_x = PLAYFIELD_CENTER_X;
		start.line_center_y = 189;
		for(i = 0; i < PENTAGRAM_POINTS; i++) {
			start.line_x[i] = polar_x(start.line_center_x, start.line_radius,
				PENTAGRAM_ANGLE_INITIAL + i * (0x100 / PENTAGRAM_POINTS));
			start.line_y[i] = polar_y(start.line_center_y, start.line_radius,
				PENTAGRAM_ANGLE_INITIAL + i * (0x100 / PENTAGRAM_POINTS));
		}
	}
	if(!t1boss_yuugenmagan_ckpt_apply_loaded(&start)) {
		return false;
	}
	Missiles.reset();
	// Deliberately bounded RGB4 endpoints, independent of prior transition overflow.
	z_palette_set_show(COL_YOKOSHIMA,
		(start.phase == 3 || start.phase == 7 || start.phase == 9) ? 0 : 13,
		(start.phase == 1 || start.phase == 3 || start.phase == 9) ? 13 : 0,
		(start.phase == 1 || start.phase >= 9) ? 5 : 15);
	stage_palette_set(z_Palettes);
	boss_palette_snap();
	graph_accesspage_func(1);
	graph_copy_accessed_page_to_other();
	graph_accesspage_func(0);
	for(i = 0; i < EYE_COUNT; i++) {
		CBossEntity& eye = t1bp_ym_eye(i);
		eye.put_8(eye.cur_left, eye.cur_top, eye.image());
	}
	if(start.phase == 13) {
		for(i = 0; i < PENTAGRAM_POINTS; i++) {
			int next = ((i + 2) % PENTAGRAM_POINTS);
			graph_r_line(start.line_x[i], start.line_y[i],
				start.line_x[next], start.line_y[next], V_WHITE);
		}
	}
	hud_hp_rerender(boss_hp);
	return true;
}

#pragma codeseg
