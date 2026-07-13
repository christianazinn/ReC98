#pragma option -zCPLAYER_M_TEXT -zPmain_01 -G-

#include "libs/master.lib/pc98_gfx.hpp"
#include "th03/main/replay.hpp"
#include "th03/resident.hpp"

extern "C" uint8_t byte_23B00;

extern "C" void near sub_C7A5(void)
{
	uint8_t pause_action;

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
	pause_action = replay_pause_menu();
	if(pause_action == REPLAY_PAUSE_SAVE_EXIT) {
		goto quit;
	}
	if(pause_action == REPLAY_PAUSE_DISCARD_EXIT) {
		replay_user_record_discard_on_exit();
		goto quit;
	}
	if(pause_action == REPLAY_PAUSE_RESTART) {
		replay_restart_request();
		goto quit;
	}
}

// Replay-mod layout pin: Keep later PLAYER_M_TEXT code at the replay-v3
// offsets while the pause UI itself lives out-of-line in REPLAY_TEXT.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90"
