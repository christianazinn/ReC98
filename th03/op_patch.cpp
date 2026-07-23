#pragma option -zCOPPATCH_TEXT -zPOP_PATCH_GROUP

#include "x86real.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/snd/snd.h"
#include "th03/common.h"
#include "th03/hardware/input.h"
#include "th03/op_patch.hpp"
#include "th03/replay_format.hpp"
#include "th03/replay_handoff.hpp"
#include "th03/resident.hpp"
#include "th03/scorefile.hpp"

extern replay_user_header_t replay_user_menu_header;
extern replay_user_summary_ext_t replay_user_menu_summary_ext;

static uint8_t far title_extra_unlock_step = 0;
static uint8_t far replay_checkpoint_target_for_menu = 0;
static const char far TITLE_EXTRA_UNLOCK_SE_FN[] = "YUME.EFC";

void far keyconfig_palette_fade_in(void)
{
	palette_black_in(1);
}

void far keyconfig_palette_fade_out(void)
{
	palette_black_out(1);
}

void far title_extra_unlock_update(void)
{
	if(input_sp == INPUT_NONE) {
		return;
	}
	input_t expected = (
		(title_extra_unlock_step & 1) ? INPUT_RIGHT : INPUT_LEFT
	);

	if(input_sp & expected) {
		title_extra_unlock_step++;
	} else {
		title_extra_unlock_step = ((input_sp & INPUT_LEFT) ? 1 : 0);
	}
	if(title_extra_unlock_step < 4) {
		return;
	}
	title_extra_unlock_step = 0;
	if(!scorefile_extra_unlock()) {
		return;
	}
	if(snd_fm_possible) {
		snd_load(TITLE_EXTRA_UNLOCK_SE_FN, SND_LOAD_SE);
		_AX = ((PMD_SE_PLAY << 8) | 8);
		geninterrupt(PMD);
	}
}

uint8_t far replay_checkpoint_anchor_for_menu(uint8_t selected)
{
	replay_checkpoint_target_for_menu = selected;
	if(!replay_user_version_has_round_state(replay_user_menu_header.version)) {
		return selected;
	}
	if(replay_user_menu_header.game_mode != GM_STORY) {
		return 0;
	}
	while(
		(selected != 0) &&
		(
			(replay_user_menu_summary_ext.checkpoint_stage_round[selected - 1] &
			 0x0F) ==
			(replay_user_menu_summary_ext.checkpoint_stage_round[selected] &
			 0x0F)
		)
	) {
		selected--;
	}
	return selected;
}

void far replay_checkpoint_handoff_set(uint8_t anchor)
{
	resident->unused_3[T3_REPLAY_RES_PLAYBACK_CHECKPOINT_INDEX] = (anchor + 1);
	resident->unused_3[T3_REPLAY_RES_PREROLL_TARGET_INDEX] = (
		(anchor == replay_checkpoint_target_for_menu) ?
			0 : (replay_checkpoint_target_for_menu + 1)
	);
}
