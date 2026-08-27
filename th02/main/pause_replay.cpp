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
#include "th02/main/pause_replay.hpp"
#include "th02/main/replay.hpp"

#define T2PAUSE_TITLE_LEFT 18
#define T2PAUSE_TITLE_Y 12
#define T2PAUSE_CHOICE_LEFT 23
#define T2PAUSE_CHOICE_Y 14
#define T2PAUSE_CHOICE_COUNT 4
#define T2PAUSE_LEGACY_CHOICE_COUNT 3
#define T2PAUSE_CLEAR_RIGHT 40

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

static void t2pause_label_put(uint8_t option, tram_y_t y, unsigned atrb)
{
	char label[13];
	char *p = label;

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
	text_putsa(T2PAUSE_CHOICE_LEFT, y, label, atrb);
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

static void t2pause_input_sample(void)
{
	input_reset_sense();
	replay_input_sample(T2REPLAY_PHASE_PAUSE);
}

static bool t2pause_restart_pressed(void)
{
	return (
		replay_pause_restart_available() &&
		!replay_playback_active() &&
		((peekb(0, KEYGROUP_2) & K2_R) != 0)
	);
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

	while((key_det != INPUT_NONE) || t2pause_restart_pressed()) {
		t2pause_input_sample();
		frame_delay(1);
	}
	palette_settone(70);
	gaiji_putsa(T2PAUSE_TITLE_LEFT, T2PAUSE_TITLE_Y, gPAUSE_MENU, TX_WHITE);
	t2pause_render(
		selected, choice_count, restart_semantics,
		restart_available, save_available
	);
	while(1) {
		t2pause_input_sample();
		if(restart_semantics && t2pause_restart_pressed()) {
			selected = T2PAUSE_RESTART;
			break;
		} else if((key_det & INPUT_UP) || (key_det & INPUT_DOWN)) {
			do {
				selected = static_cast<uint8_t>(
					(key_det & INPUT_UP)
					? ((selected == T2PAUSE_RESUME) ? (choice_count - 1) : (selected - 1))
					: ((selected == (choice_count - 1)) ? T2PAUSE_RESUME : (selected + 1))
				);
				action = t2pause_action(selected, restart_semantics);
			} while(!t2pause_action_enabled(
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
			if(t2pause_action_enabled(
				action, restart_available, save_available
			)) {
				break;
			}
		}
		if(key_det != INPUT_NONE) {
			while(key_det != INPUT_NONE) {
				t2pause_input_sample();
				frame_delay(1);
			}
		}
		frame_delay(1);
	}
	while((key_det != INPUT_NONE) || t2pause_restart_pressed()) {
		t2pause_input_sample();
		frame_delay(1);
	}
	t2pause_clear();
	action = t2pause_action(selected, restart_semantics);
	if(action == T2PAUSE_RESUME) {
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
