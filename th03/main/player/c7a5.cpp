#pragma option -zCPLAYER_M_TEXT -zPmain_01 -G-

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/frmdelay.h"
#include "th03/hardware/input.h"
#include "th03/main/replay.hpp"
#include "th03/resident.hpp"
#include "th03/snd/snd.h"

extern "C" uint8_t byte_23B00;

extern "C" void near sub_C7A5(void)
{
	if(replay_prompt_skip()) {
		byte_23B00 = 1;
		return;
	}
	if((resident->is_cpu[0] != 0) && (resident->is_cpu[1] != 0)) {
		palette_black_out(1);

quit:
		byte_23B00 = 1;
		return;
	}
	snd_se_reset();
	snd_se_play(21);
	snd_se_update();
	goto release_test;

release_wait:
	replay_input_sense_held();
	frame_delay(1);

release_test:
	if(input_sp != INPUT_NONE) {
		goto release_wait;
	}

input_wait:
	replay_input_sense_held();
	if(input_sp & INPUT_Q) {
		goto quit;
	}
	if(input_sp & INPUT_CANCEL) {
		goto cancel_release_test;
	}
	frame_delay(1);
	goto input_wait;

cancel_release_wait:
	replay_input_sense_held();
	frame_delay(1);

cancel_release_test:
	if(input_sp != INPUT_NONE) {
		goto cancel_release_wait;
	}
	snd_se_reset();
	snd_se_play(21);
	snd_se_update();
}
