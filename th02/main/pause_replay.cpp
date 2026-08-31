/* TH02 Replay Patch -- Pause menu terminal actions.
 *
 * This tail replaces only stage_loop()'s existing near Pause call. It owns a
 * separate code segment, emits no initialized data, and keeps the native
 * pause_menu() contribution's RC8 size and downstream offsets exact.
 */

#pragma option -zCT2PAUSE_TEXT -G-

#include "platform.h"
#include "pc98.h"
#include "platform/x86real/pc98/keyboard.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/frmdelay.h"
#include "th02/hardware/input.hpp"
#include "th02/main/execl.hpp"
#include "th02/main/language_presentation.hpp"
#include "th02/main/frames.hpp"
#include "th02/main/pause_replay.hpp"
#include "th02/main/replay.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/stage/callback.hpp"
#include "th02/resident.hpp"

#define T2PAUSE_TITLE_LEFT 18
#define T2PAUSE_TITLE_Y 12
#define T2PAUSE_CHOICE_CENTER 29
#define T2PAUSE_CHOICE_LEFT 16
#define T2PAUSE_CHOICE_Y 14
#define T2PAUSE_CHOICE_COUNT 4
#define T2PAUSE_LEGACY_CHOICE_COUNT 3
#define T2PAUSE_CLEAR_RIGHT 44

#define T2PAUSE_RESUME 0
#define T2PAUSE_RESTART 1
#define T2PAUSE_SAVE_EXIT 2
#define T2PAUSE_DISCARD_EXIT 3

#define T2PAUSE_ATRB_SELECTED (TX_WHITE | TX_UNDERLINE)
#define T2PAUSE_ATRB_UNSELECTED TX_MAGENTA

// The original native title and its matching blank gaiji string stay in the
// protected MAIN data contribution; the patch only references them.
extern char gPAUSE_MENU[];
extern char g11SPACES[];
extern const char arg0[];
extern "C" void far stage_title_unput(void);
extern "C" const shiftjis_t aEMPTY[];
extern "C" uint8_t stage1_gaiji_halflen;
extern "C" const char gStage1[];
extern "C" const char gEXTRA_STAGE[];
extern "C" shiftjis_t near *stage_title;
extern "C" uint8_t stage_title_halflen;

static bool t2pause_stage_title_hide(void)
{
	if(
		resident->demo_num ||
		(stage_title_unput_func != stage_title_unput) ||
		(stage_frame >= 160)
	) {
		return false;
	}
	text_putsa(16, 12, aEMPTY, TX_WHITE);
	text_putsa(16, 13, aEMPTY, TX_WHITE);
	return true;
}

static void t2pause_stage_title_restore(void)
{
	if(stage_title_unput_func != stage_title_unput) {
		return;
	}
	if(stage_id == 5) {
		gaiji_putsa(16, 12, gEXTRA_STAGE, TX_YELLOW);
	} else {
		gaiji_putsa(
			static_cast<tram_x_t>(28 - stage1_gaiji_halflen),
			12, gStage1, TX_YELLOW
		);
	}
	text_putsa(
		static_cast<tram_x_t>(28 - stage_title_halflen),
		13, stage_title, TX_WHITE
	);
}

static void t2pause_label_put(uint8_t option, tram_y_t y, unsigned atrb)
{
	const char far *english_label = t2_language_main_pause_label(option);
	char label[13];
	char *p = label;
	unsigned length = 0;
	tram_x_t left;

	if(english_label) {
		while(english_label[length] != '\0') {
			length++;
		}
		left = static_cast<tram_x_t>(T2PAUSE_CHOICE_CENTER - (length / 2));
		text_putsa(left, y, english_label, atrb);
		return;
	}

	#define P(c) *p++ = c
	switch(option) {
	case T2PAUSE_RESUME:
		P(0x8D); P(0xC4); P(0x8A); P(0x4A);
		break;
	case T2PAUSE_RESTART:
		P(0x8D); P(0xC5); P(0x8F); P(0x89);
		P(0x82); P(0xA9); P(0x82); P(0xE7);
		break;
	case T2PAUSE_SAVE_EXIT:
		P(0x95); P(0xDB); P(0x91); P(0xB6); P(0x82); P(0xB5);
		P(0x82); P(0xC4); P(0x8F); P(0x49); P(0x97); P(0xB9);
		break;
	default:
		P(0x95); P(0xDB); P(0x91); P(0xB6); P(0x82); P(0xB9);
		P(0x82); P(0xB8); P(0x8F); P(0x49); P(0x97); P(0xB9);
		break;
	}
	#undef P
	*p = '\0';
	length = static_cast<unsigned>(p - label);
	left = static_cast<tram_x_t>(T2PAUSE_CHOICE_CENTER - (length / 2));
	text_putsa(left, y, label, atrb);
}

static uint8_t t2pause_action(uint8_t selected, bool restart_semantics)
{
	if(restart_semantics || (selected == T2PAUSE_RESUME)) {
		return selected;
	}
	return static_cast<uint8_t>(selected + 1);
}

static bool t2pause_action_enabled(
	uint8_t action, bool restart_available, bool save_available
)
{
	if(action == T2PAUSE_RESTART) {
		return restart_available;
	}
	if(action == T2PAUSE_SAVE_EXIT) {
		return save_available;
	}
	return true;
}

static bool t2pause_action_selectable(
	uint8_t action, bool restart_available, bool save_available
)
{
	// Playback remains read-only, but recorded cursor movement traversed all
	// four rows. Preserve that topology so a later confirmation reaches the
	// same action that was selected while recording.
	return (
		replay_playback_active() ||
		t2pause_action_enabled(action, restart_available, save_available)
	);
}

static void t2pause_render(
	uint8_t selected, uint8_t choice_count,
	bool restart_semantics, bool restart_available, bool save_available
)
{
	uint8_t choice;
	uint8_t action;
	unsigned atrb;

	for(choice = 0; choice < choice_count; choice++) {
		action = t2pause_action(choice, restart_semantics);
		if(!t2pause_action_enabled(
			action, restart_available, save_available
		)) {
			atrb = TX_BLUE;
		} else if(choice == selected) {
			atrb = T2PAUSE_ATRB_SELECTED;
		} else {
			atrb = T2PAUSE_ATRB_UNSELECTED;
		}
		t2pause_label_put(action, (T2PAUSE_CHOICE_Y + choice), atrb);
	}
}

static void t2pause_clear(void)
{
	tram_y_t y;
	tram_x_t x;

	gaiji_putsa(T2PAUSE_TITLE_LEFT, T2PAUSE_TITLE_Y, g11SPACES, TX_WHITE);
	for(y = T2PAUSE_CHOICE_Y; y < (T2PAUSE_CHOICE_Y + T2PAUSE_CHOICE_COUNT); y++) {
		for(x = T2PAUSE_CHOICE_LEFT; x < T2PAUSE_CLEAR_RIGHT; x++) {
			text_putca(x, y, ' ', TX_WHITE);
		}
	}
}

static bool t2pause_input_sample(void)
{
	input_reset_sense();
	replay_input_sample(T2REPLAY_PHASE_PAUSE);
	return replay_playback_exit_requested();
}

static bool16 t2pause_playback_exit(void)
{
	t2pause_clear();
	return true;
}

static bool t2pause_restart_pressed(void)
{
	return (
		replay_pause_restart_available() &&
		!replay_playback_active() &&
		((peekb(0, KEYGROUP_2) & K2_R) != 0)
	);
}

static bool t2pause_save_refresh(
	uint8_t far &selected, uint8_t choice_count,
	bool restart_semantics, bool restart_available, bool far &save_available
)
{
	uint8_t action;

	if(!save_available || replay_pause_save_refresh()) {
		return false;
	}
	save_available = false;
	action = t2pause_action(selected, restart_semantics);
	if(action == T2PAUSE_SAVE_EXIT) {
		do {
			selected = static_cast<uint8_t>(
				(selected == (choice_count - 1))
				? T2PAUSE_RESUME : (selected + 1)
			);
			action = t2pause_action(selected, restart_semantics);
		} while(!t2pause_action_enabled(
			action, restart_available, save_available
		));
	}
	t2pause_render(
		selected, choice_count, restart_semantics,
		restart_available, save_available
	);
	return true;
}

bool16 far t2pause_menu(void)
{
	uint8_t selected = T2PAUSE_RESUME;
	uint8_t action;
	bool restart_semantics = replay_pause_restart_semantics();
	uint8_t choice_count = (
		restart_semantics ? T2PAUSE_CHOICE_COUNT : T2PAUSE_LEGACY_CHOICE_COUNT
	);
	bool restart_available = replay_pause_restart_available();
	bool save_available = replay_pause_save_available();
	bool stage_title_hidden = t2pause_stage_title_hide();

	while((key_det != INPUT_NONE) || t2pause_restart_pressed()) {
		if(t2pause_input_sample()) {
			return t2pause_playback_exit();
		}
		frame_delay(1);
	}
	save_available = replay_pause_save_refresh();
	palette_settone(70);
	gaiji_putsa(T2PAUSE_TITLE_LEFT, T2PAUSE_TITLE_Y, gPAUSE_MENU, TX_WHITE);
	t2pause_render(
		selected, choice_count, restart_semantics,
		restart_available, save_available
	);
	while(1) {
		if(t2pause_input_sample()) {
			return t2pause_playback_exit();
		}
		if(
			(key_det != INPUT_NONE) &&
			t2pause_save_refresh(
				selected, choice_count, restart_semantics,
				restart_available, save_available
			)
		) {
			while(key_det != INPUT_NONE) {
				if(t2pause_input_sample()) {
					return t2pause_playback_exit();
				}
				frame_delay(1);
			}
			continue;
		}
		if(restart_semantics && t2pause_restart_pressed()) {
			selected = T2PAUSE_RESTART;
			break;
		} else if(key_det & INPUT_Q) {
			selected = (choice_count - 1);
			break;
		} else if((key_det & INPUT_UP) || (key_det & INPUT_DOWN)) {
			do {
				selected = static_cast<uint8_t>(
					(key_det & INPUT_UP)
					? ((selected == T2PAUSE_RESUME) ? (choice_count - 1) : (selected - 1))
					: ((selected == (choice_count - 1)) ? T2PAUSE_RESUME : (selected + 1))
				);
				action = t2pause_action(selected, restart_semantics);
			} while(!t2pause_action_selectable(
				action, restart_available, save_available
			));
			t2pause_render(
				selected, choice_count, restart_semantics,
				restart_available, save_available
			);
		} else if(key_det & INPUT_CANCEL) {
			selected = T2PAUSE_RESUME;
			break;
		} else if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
			action = t2pause_action(selected, restart_semantics);
			if(t2pause_action_selectable(
				action, restart_available, save_available
			)) {
				break;
			}
		}
		if(key_det != INPUT_NONE) {
			while(key_det != INPUT_NONE) {
				if(t2pause_input_sample()) {
					return t2pause_playback_exit();
				}
				frame_delay(1);
			}
		}
		frame_delay(1);
	}
	while((key_det != INPUT_NONE) || t2pause_restart_pressed()) {
		if(t2pause_input_sample()) {
			return t2pause_playback_exit();
		}
		frame_delay(1);
	}
	t2pause_clear();
	action = t2pause_action(selected, restart_semantics);
	if(action == T2PAUSE_RESUME) {
		if(stage_title_hidden) {
			t2pause_stage_title_restore();
		}
		palette_settone(100);
		key_det = INPUT_NONE;
		return false;
	}
	if(action == T2PAUSE_RESTART) {
		if(!replay_pause_restart()) {
			palette_settone(100);
			key_det = INPUT_NONE;
			return false;
		}
	} else if(action == T2PAUSE_SAVE_EXIT) {
		replay_pause_save_and_exit();
	} else {
		replay_pause_exit_without_saving();
	}
	GameExecl(arg0);
	return true;
}
