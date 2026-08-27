/* TH01 replay-aware Pause surface.
 *
 * This tail owns only added presentation and action selection. Ordinary
 * input_sense(false) remains the sole replay seam, so Pause navigation is
 * recorded and played back as part of the canonical input stream.
 */

#pragma option -zCT1REPLAY_TEXT -G-

#include "th01/hardware/egc.h"
#include "th01/hardware/frmdelay.h"
#include "th01/hardware/graph.h"
#include "th01/hardware/grppsafx.h"
#include "th01/hardware/grp_text.hpp"
#include "th01/hardware/input.hpp"
#include "th01/hardware/palette.h"
#include "th01/main/playfld.hpp"
#include "th01/main/stage/palette.hpp"
#include "th01/replay.hpp"
#include "th01/v_colors.hpp"
#include "platform/x86real/pc98/keyboard.hpp"

extern void z_palette_settone_but_keep_white(int tone);

static screen_y_t t1replay_pause_top(uint8_t line)
{
	return static_cast<screen_y_t>(
		PLAYFIELD_TOP + ((PLAYFIELD_H / 21) * 4) + (line * GLYPH_H)
	);
}

static int t1replay_pause_text(
	shiftjis_t *out, t1replay_pause_action_t action
)
{
	switch(action) {
	case T1RPA_RESUME:
		out[0] = 0x8D; out[1] = 0xC4; out[2] = 0x8A; out[3] = 0x4A;
		out[4] = '\0';
		return 4;
	case T1RPA_RESTART:
		out[0] = 0x82; out[1] = 0xE2; out[2] = 0x82; out[3] = 0xE8;
		out[4] = 0x92; out[5] = 0xBC; out[6] = 0x82; out[7] = 0xB5;
		out[8] = '\0';
		return 8;
	case T1RPA_SAVE_EXIT:
		out[0] = 0x95; out[1] = 0xDB; out[2] = 0x91; out[3] = 0xB6;
		out[4] = 0x82; out[5] = 0xB5; out[6] = 0x82; out[7] = 0xC4;
		out[8] = 0x8F; out[9] = 0x49; out[10] = 0x97; out[11] = 0xB9;
		out[12] = '\0';
		return 12;
	case T1RPA_DISCARD_EXIT:
		out[0] = 0x95; out[1] = 0xDB; out[2] = 0x91; out[3] = 0xB6;
		out[4] = 0x82; out[5] = 0xB9; out[6] = 0x82; out[7] = 0xB8;
		out[8] = 0x8F; out[9] = 0x49; out[10] = 0x97; out[11] = 0xB9;
		out[12] = '\0';
		return 12;
	}
	out[0] = '\0';
	return 0;
}

static void t1replay_pause_title_text(shiftjis_t *out)
{
	out[0] = 0x82; out[1] = 0x6F; out[2] = 0x82; out[3] = 0x60;
	out[4] = 0x82; out[5] = 0x74; out[6] = 0x82; out[7] = 0x72;
	out[8] = 0x82; out[9] = 0x64; out[10] = '\0';
}

static bool t1replay_pause_action_enabled(t1replay_pause_action_t action)
{
	if(action == T1RPA_RESTART) {
		return (t1replay_pause_restart_available() != false);
	}
	if(action == T1RPA_SAVE_EXIT) {
		return (t1replay_pause_save_available() != false);
	}
	return true;
}

// Group 2 is not part of REIIDEN's canonical replay input contract.
// Sampling it only inside Pause gives live runs an Esc+R shortcut without
// making R a replay input. Playback cannot restart because its transaction is
// unavailable.
static bool t1replay_pause_restart_pressed(void)
{
	return (
		(t1replay_pause_restart_available() != false) &&
		((peekb(0, KEYGROUP_2) & K2_R) != 0)
	);
}

// Keep the physical shortcut from surviving the EXE handoff. Any
// canonical held state is sampled through the existing Pause seam, while R
// remains outside the replay format and this recording is discarded on restart.
static void t1replay_pause_restart_release(void)
{
	while(peekb(0, KEYGROUP_2) & K2_R) {
		input_sense(false);
		frame_delay(1);
	}
}

static void t1replay_pause_row_put(
	t1replay_pause_action_t action, bool selected
)
{
	shiftjis_t text[13];
	shiftjis_t cursor[3];
	t1replay_pause_text(text, action);
	screen_x_t left = static_cast<screen_x_t>(
		(PLAYFIELD_CENTER_X - GLYPH_HALF_W) - (shiftjis_w(text) / 2)
	);
	vc_t col = (
		selected ? V_WHITE :
		(t1replay_pause_action_enabled(action) ? V_GRAY : V_BLACK)
	);

	// The stock Pause renderer restores its text from page 1 before changing a
	// choice. Do the same for each row, including the preceding cursor cell, so
	// moving the selection cannot leave white pixels on the gameplay page.
	egc_copy_rect_1_to_0_16(
		192,
		t1replay_pause_top(static_cast<uint8_t>(action + 1)),
		256,
		GLYPH_H
	);

	graph_putsa_fx(
		left,
		t1replay_pause_top(static_cast<uint8_t>(action + 1)),
		(col | FX_WEIGHT_BLACK),
		text
	);
	if(selected) {
		cursor[0] = 0x81; cursor[1] = 0x9C; cursor[2] = '\0';
		graph_putsa_fx(
			(left - GLYPH_FULL_W),
			t1replay_pause_top(static_cast<uint8_t>(action + 1)),
			(V_WHITE | FX_WEIGHT_BLACK),
			cursor
		);
	}
}

static void t1replay_pause_render(t1replay_pause_action_t selected)
{
	shiftjis_t title[11];
	int action;

	t1replay_pause_title_text(title);
	graph_putsa_fx(
		static_cast<screen_x_t>(
			(PLAYFIELD_CENTER_X - GLYPH_HALF_W) - (shiftjis_w(title) / 2)
		),
		t1replay_pause_top(0),
		(V_WHITE | FX_WEIGHT_BLACK),
		title
	);
	for(action = T1RPA_RESUME; action <= T1RPA_DISCARD_EXIT; action++) {
		t1replay_pause_row_put(
			static_cast<t1replay_pause_action_t>(action), (action == selected)
		);
	}
}

static t1replay_pause_action_t t1replay_pause_next(
	t1replay_pause_action_t selected, int direction
)
{
	int candidate = selected;

	do {
		candidate += direction;
		if(candidate < T1RPA_RESUME) {
			candidate = T1RPA_DISCARD_EXIT;
		} else if(candidate > T1RPA_DISCARD_EXIT) {
			candidate = T1RPA_RESUME;
		}
	} while(!t1replay_pause_action_enabled(
		static_cast<t1replay_pause_action_t>(candidate)
	));
	return static_cast<t1replay_pause_action_t>(candidate);
}

bool16 far t1replay_pause_menu(void)
{
	t1replay_pause_action_t selected = T1RPA_RESUME;
	t1replay_pause_action_t previous;

	t1replay_pause_render(selected);
	z_palette_settone_but_keep_white(40);
	input_reset_menu_related();
	while(paused) {
		input_sense(false);
		if(player_is_hit == true) {
			t1replay_pause_action_set(T1RPA_DISCARD_EXIT);
			return true;
		}
		if(t1replay_pause_restart_pressed()) {
			t1replay_pause_action_set(T1RPA_RESTART);
			t1replay_pause_restart_release();
			return true;
		}
		previous = selected;
		if(input_up) {
			selected = t1replay_pause_next(selected, -1);
		}
		if(input_down) {
			selected = t1replay_pause_next(selected, 1);
		}
		if(previous != selected) {
			t1replay_pause_row_put(previous, false);
			t1replay_pause_row_put(selected, true);
		}
		if(input_shot || input_ok) {
			t1replay_pause_action_set(selected);
			return (selected != T1RPA_RESUME);
		}
		frame_delay(1);
	}

	z_palette_set_all_show(stage_palette);
	input_reset_sense();
	egc_copy_rect_1_to_0_16(
		192,
		t1replay_pause_top(0),
		256,
		static_cast<pixel_t>(t1replay_pause_top(5) - t1replay_pause_top(0))
	);
	return false;
}
