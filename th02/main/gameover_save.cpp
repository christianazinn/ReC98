// Patch-owned post-registration replay-save decision. This file is included
// after POINTNUM_TEXT so the five-byte far call in continue_prompt() is its
// only original-code change.

#pragma codeseg T2GOSAVE_TEXT

#include "th02/main/replay.hpp"

static void t2gosave_prompt_put(bool save)
{
	text_putca(34, 12, 'S', TX_WHITE);
	text_putca(35, 12, 'a', TX_WHITE);
	text_putca(36, 12, 'v', TX_WHITE);
	text_putca(37, 12, 'e', TX_WHITE);
	text_putca(38, 12, ' ', TX_WHITE);
	text_putca(39, 12, 'R', TX_WHITE);
	text_putca(40, 12, 'e', TX_WHITE);
	text_putca(41, 12, 'p', TX_WHITE);
	text_putca(42, 12, 'l', TX_WHITE);
	text_putca(43, 12, 'a', TX_WHITE);
	text_putca(44, 12, 'y', TX_WHITE);
	text_putca(45, 12, '?', TX_WHITE);
	gaiji_putsa(30, 14, gYES, save ? (TX_WHITE | TX_REVERSE) : TX_WHITE);
	gaiji_putsa(30, 15, gNO, save ? TX_WHITE : (TX_WHITE | TX_REVERSE));
}

static bool t2gosave_prompt(void)
{
	bool save = true;
	int confirmable = 0;

	t2gosave_prompt_put(save);
	while(1) {
		input_reset_sense();
		if((confirmable == 0) && (key_det == INPUT_NONE)) {
			confirmable = 1;
		} else if((confirmable == 1) && (key_det != INPUT_NONE)) {
			confirmable = 2;
		}
		if(key_det & INPUT_UP) {
			save = true;
			t2gosave_prompt_put(save);
		}
		if(key_det & INPUT_DOWN) {
			save = false;
			t2gosave_prompt_put(save);
		}
		if(
			(confirmable == 2) && (
				(key_det & INPUT_SHOT) || (key_det & INPUT_OK) ||
				(key_det & INPUT_CANCEL)
			)
		) {
			break;
		}
		frame_delay(1);
	}
	return (save && !(key_det & INPUT_CANCEL));
}

void far t2gosave_post_regist(void)
{
	overlay_wipe();
	if(replay_save_request_prompt_needed() && !t2gosave_prompt()) {
		replay_save_request_discard();
	}
	overlay_wipe();
	key_det = INPUT_NONE;
}

#pragma codeseg
