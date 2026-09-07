#pragma option -zCT1B20MPRA_TEXT -G-
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
#include "th01/bprstate.hpp"
#include "th01/main/boss/b20m.hpp"
#include "th01/formats/grp.h"
#include "th01/formats/ptn.hpp"
#include "th01/main/hud/hud.hpp"
#include "th01/main/player/player.hpp"
#include "th01/snd/mdrv2.h"
enum { SHIELD_LEFT=304, SHIELD_TOP=144, WAND_W=128, WAND_H=96,
	COL_FORM2_PULSE=6, PHASE_FORM2=100 };
#define ent_shield boss_entity_0
#define anm_dress boss_anims[0]
#define anm_wand boss_anims[1]
#define PTN_SLOT_WAND_LOWERED PTN_SLOT_BOSS_1
#pragma codeseg T1B20MPRA_TEXT

bool16 t1boss_sariel_practice_construct(uint8_t target)
{
	t1boss_sariel_checkpoint_t loaded;
	int ptn_x;
	int ptn_y;
	int image = 0;
	char bg[13];
	char music[11];
	bool16 form2 = (target == T1RPBPT_SARIEL_FORM_2);

	if((target < T1RPBPT_SARIEL_PHASE_1) ||
	   (target > T1RPBPT_SARIEL_FORM_2) ||
	   (frame_since_start_of_binary != 0) ||
	   (boss_phase != 0) || (boss_phase_frame != 0) || (boss_hp != 18) ||
	   !t1boss_sariel_checkpoint_capture(&loaded) ||
	   !t1boss_sariel_ckpt_apply_loaded(&loaded)) {
		return false;
	}
	// Stack-built names add no initialized DATA ahead of stock BSS.
	bg[0]='b'; bg[1]='o'; bg[2]='s'; bg[3]='s'; bg[4]='6'; bg[5]='_';
	bg[6]='a'; bg[7]='6'; bg[8]='.'; bg[9]='g'; bg[10]='r';
	bg[11]='p'; bg[12]=0;
	if(!form2) bg[7] = ('1' + target - T1RPBPT_SARIEL_PHASE_1);
	graph_accesspage_func(1);
	if(grp_put_palette_show(bg) != 0) {
		graph_accesspage_func(0);
		return false;
	}
	if(form2) {
		z_palette_set_show(COL_FORM2_PULSE, 0, 0, 0);
	}
	graph_copy_accessed_page_to_other();
	hud_rerender();
	graph_accesspage_func(0);
	boss_phase = form2 ? PHASE_FORM2 :
		(1 + ((target - T1RPBPT_SARIEL_PHASE_1) * 2));
	boss_phase_frame = 0;
	boss_hp = form2 ? 6 : 18;
	hud_hp_first_white = form2 ? 10 : 8;
	hud_hp_first_redwhite = form2 ? 3 : 2;
	t1bp_sar_invincible = false;
	t1bp_sar_hit_frame = 0;
	t1bp_sar_ring = 0;
	t1bp_sar_hp_done = true;
	t1bp_sar_phase.pattern = 0;
	t1bp_sar_phase.done = 0;
	t1bp_sar_phase.until_next = 3;
	t1bp_sar_pattern = 0;
	ent_shield.pos_cur_set(SHIELD_LEFT, SHIELD_TOP);
	ent_shield.prev_left = SHIELD_LEFT;
	ent_shield.prev_top = SHIELD_TOP;
	ent_shield.prev_delta_x = 0;
	ent_shield.prev_delta_y = 0;
	ent_shield.lock_frame = 0;
	ent_shield.set_image(0);
	ent_shield.hitbox_orb_inactive = false;
	anm_dress.bos_image = 0;
	anm_wand.bos_image = 0;
	// Equivalent to wand_lowered_snap(), whose near ABI cannot cross segments.
	ptn_snap_rect_from_1_8(
		anm_wand.left, anm_wand.top, WAND_W, WAND_H,
		PTN_SLOT_WAND_LOWERED, image, ptn_x, ptn_y
	);
	// Bird, leaf, particle, and wand-animation statics are fresh at this seam.
	boss_palette_snap();
	stage_palette_set(z_Palettes);
	hud_hp_rerender(boss_hp);
	if(form2) {
		music[0]='s'; music[1]='y'; music[2]='u'; music[3]='g'; music[4]='e';
		music[5]='n'; music[6]='.'; music[7]='M'; music[8]='D';
		music[9]='T'; music[10]=0;
		mdrv2_bgm_load(music);
		mdrv2_bgm_play();
		player_invincibility_time = 0;
		player_invincible = false;
	}
	return true;
}

#pragma codeseg
