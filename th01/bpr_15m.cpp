#pragma option -zCT1B15MPRA_TEXT -G-
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
#include "th01/main/bullet/missile.hpp"
#include "th01/main/boss/b15m.hpp"
enum { F_GIRL=0, F_BAT=1, CHOOSE_NEW=0, C_BAT=0,
	HP_PHASE_1_END=10, HP_PHASE_3_END=6 };
#define ent_still_or_wave boss_entity_0
#define ent_bat boss_entity_2
void girl_bg_put(int unnecessary);
#pragma codeseg T1B15MPRA_TEXT

bool16 t1boss_elis_practice_construct(uint8_t target)
{
	if(target == T1RPBPT_ELIS_PHASE_1) {
		return t1boss_elis_practice_first_combat_apply();
	}
	if(((target != T1RPBPT_ELIS_PHASE_3) &&
	    (target != T1RPBPT_ELIS_PHASE_5)) ||
	   !t1boss_elis_practice_first_combat_apply()) {
		return false;
	}
	t1bp_el_form = (target == T1RPBPT_ELIS_PHASE_5) ? F_BAT : F_GIRL;
	t1bp_el_hit.frame = 0;
	t1bp_el_hit.invincible = false;
	t1bp_el_phase.pattern = (t1bp_el_form == F_GIRL) ? 1 : CHOOSE_NEW;
	t1bp_el_phase.teleport_done = false;
	t1bp_el_vx = 0;
	t1bp_el_vy = 0;
	t1bp_el_hp_done = true;
	boss_phase = (t1bp_el_form == F_GIRL) ? 3 : 5;
	boss_phase_frame = 0;
	boss_hp = (t1bp_el_form == F_GIRL) ? HP_PHASE_1_END : HP_PHASE_3_END;
	t1bp_el_pattern = 0;
	Missiles.reset();
	// The fresh-process guard also guarantees fresh pattern-local statics.
	boss_palette_snap();
	graph_accesspage_func(1); girl_bg_put(1);
	graph_accesspage_func(0); girl_bg_put(1);
	if(t1bp_el_form == F_BAT) {
		ent_bat.pos_cur_set(ent_still_or_wave.cur_left + 40,
			ent_still_or_wave.cur_top + 32);
		ent_bat.prev_left = ent_bat.cur_left;
		ent_bat.prev_top = ent_bat.cur_top;
		ent_bat.prev_delta_x = 0;
		ent_bat.prev_delta_y = 0;
		ent_bat.lock_frame = 0;
		ent_bat.set_image(C_BAT);
		graph_accesspage_func(0);
		ent_bat.put_8(ent_bat.cur_left, ent_bat.cur_top, C_BAT);
	} else {
		graph_accesspage_func(1);
		ent_still_or_wave.unlock_put_lock_8();
		graph_accesspage_func(0);
		ent_still_or_wave.unlock_put_lock_8();
	}
	hud_hp_rerender(boss_hp);
	return true;
}

#pragma codeseg
