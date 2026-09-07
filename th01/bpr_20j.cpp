#pragma option -zCT1B20JPRA_TEXT -G-
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
#include "th01/main/boss/b20j.hpp"
enum { CHOOSE_NEW=99 };
#pragma codeseg T1B20JPRA_TEXT

bool16 t1boss_konngara_practice_construct(uint8_t target)
{
	t1boss_konngara_checkpoint_t loaded;
	if((target < T1RPBPT_KONNGARA_PHASE_1) ||
	   (target > T1RPBPT_KONNGARA_PHASE_7) ||
	   (frame_since_start_of_binary != 0) ||
	   !t1boss_konngara_checkpoint_capture(&loaded) ||
	   !t1boss_konngara_ckpt_apply_loaded(&loaded)) {
		return false;
	}
	boss_phase = (1 + ((target - T1RPBPT_KONNGARA_PHASE_1) * 2));
	boss_hp = (18 - ((target - T1RPBPT_KONNGARA_PHASE_1) * 3));
	boss_phase_frame = 0;
	t1bp_kon_hit.invincible = false;
	t1bp_kon_hit.frame = 0;
	t1bp_kon_prev = CHOOSE_NEW;
	t1bp_kon_phase.pattern = CHOOSE_NEW;
	t1bp_kon_phase.done = 0;
	t1bp_kon_hp_done = true;
	t1bp_kon_pattern = 0;
	// Native load_and_entrance already painted the neutral head on both pages.
	boss_palette_snap();
	hud_hp_rerender(boss_hp);
	graph_accesspage_func(0);
	return true;
}

#pragma codeseg
