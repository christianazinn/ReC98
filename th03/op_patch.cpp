#pragma option -zCOPPATCH_TEXT -zPOP_PATCH_GROUP

#include "x86real.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/snd/snd.h"
#include "th03/hardware/input.h"
#include "th03/op_patch.hpp"
#include "th03/scorefile.hpp"

static uint8_t far title_extra_unlock_step = 0;
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
