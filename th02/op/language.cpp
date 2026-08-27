// Patch-owned TH02 Option controller. This file is textually included into
// lang_op.cpp's trailing T2LANGOP_TEXT segment.

#include "th01/rank.h"
#include "th01/math/clamp.hpp"
#include "th01/hardware/grppsafx.h"
#include "th02/common.h"
#include "th02/resident.hpp"
#include "th02/hardware/grp_rect.h"
#include "th02/hardware/input.hpp"
#include "th02/core/globals.hpp"
#include "th02/formats/cfg.hpp"
#include "th02/snd/snd.h"
#include "th02/gaiji/gaiji.h"
#include "th02/op/replay.hpp"

extern char menu_sel;
extern bool in_option;
extern unsigned char snd_bgm_mode;
extern resident_t __seg *resident_seg;

#define T2_LANGUAGE_OPTION_COUNT 8
#define T2_LANGUAGE_OPTION_LABEL_LEFT 192
#define T2_LANGUAGE_OPTION_VALUE_LEFT 368
#define T2_LANGUAGE_OPTION_TOP 336
#define T2_LANGUAGE_OPTION_COLUMN_W 128

static bool t2_language_option_initialized;
static bool t2_language_option_input_allowed;
static int8_t t2_language_option_sel;

static void t2_language_option_gaiji_reload(void)
{
	char fn[11];

	fn[0] = 'M'; fn[1] = 'I'; fn[2] = 'K'; fn[3] = 'O'; fn[4] = 'F';
	fn[5] = 'T'; fn[6] = '.'; fn[7] = 'B'; fn[8] = 'F'; fn[9] = 'T';
	fn[10] = '\0';
	gaiji_restore();
	t2_language_gaiji_entry_bfnt(fn);
}

static void t2_language_option_language_put(tram_atrb2 atrb)
{
	char label[9];
	char value[8];

	t2_language_option_text(label, value);
	text_putsa(
		(T2_LANGUAGE_OPTION_LABEL_LEFT / GLYPH_HALF_W),
		(T2_LANGUAGE_OPTION_TOP / GLYPH_H), label, atrb
	);
	text_putsa(
		(T2_LANGUAGE_OPTION_VALUE_LEFT / GLYPH_HALF_W),
		(T2_LANGUAGE_OPTION_TOP / GLYPH_H), value, atrb
	);
	graph_copy_rect_1_to_0_16(
		T2_LANGUAGE_OPTION_LABEL_LEFT,
		(T2_LANGUAGE_OPTION_TOP + 4),
		T2_LANGUAGE_OPTION_COLUMN_W,
		GLYPH_H
	);
	graph_copy_rect_1_to_0_16(
		T2_LANGUAGE_OPTION_VALUE_LEFT,
		(T2_LANGUAGE_OPTION_TOP + 4),
		80,
		GLYPH_H
	);
	graph_putsa_fx(
		(T2_LANGUAGE_OPTION_LABEL_LEFT + 4),
		(T2_LANGUAGE_OPTION_TOP + 4), 0, label
	);
	graph_putsa_fx(
		(T2_LANGUAGE_OPTION_VALUE_LEFT + 4),
		(T2_LANGUAGE_OPTION_TOP + 4), 0, value
	);
}

static void t2_language_option_put(int sel, tram_atrb2 atrb)
{
	if(sel == 5) {
		t2_language_option_language_put(atrb);
	} else {
		t2_language_op_bridge(T2LOB_OPTION_PUT, sel, atrb);
	}
}

static void t2_language_option_change(int direction)
{
	t2_language_option_put(t2_language_option_sel, TX_YELLOW);
	switch(t2_language_option_sel) {
	case 0:
		if(direction > 0) {
			ring_inc(rank, RANK_LUNATIC);
		} else {
			ring_dec(rank, RANK_LUNATIC);
		}
		break;
	case 1:
		if(direction > 0) {
			ring_inc((char)snd_bgm_mode, SND_BGM_MIDI);
		} else {
			ring_dec((char)snd_bgm_mode, SND_BGM_MIDI);
		}
		t2_language_op_bridge(T2LOB_BGM_RESTART, 0, 0);
		break;
	case 2:
		if(direction > 0) {
			ring_inc(lives, CFG_LIVES_MAX);
		} else {
			ring_dec(lives, CFG_LIVES_MAX);
		}
		break;
	case 3:
		if(direction > 0) {
			ring_inc(bombs, CFG_BOMBS_MAX);
		} else {
			ring_dec(bombs, CFG_BOMBS_MAX);
		}
		break;
	case 4:
		resident->reduce_effects = (true - resident->reduce_effects);
		break;
	case 5: {
		t2_language_preference_t target = (
			(t2_language_get() == T2LANG_JAPANESE) ?
				T2LANG_ENGLISH : T2LANG_JAPANESE
		);

		if(
			((target == T2LANG_JAPANESE) || t2_language_english_ready()) &&
			t2_language_set(target)
		) {
			t2_language_op_tables_apply();
			t2_language_option_gaiji_reload();
			replay_title_background_restore();
			// Options still owns the foreground. Force the normal title rebuild
			// after it returns rather than exposing stale Option text.
			replay_title_restore_needed = true;
			t2_language_option_initialized = false;
			return;
		}
		break;
	}
	}
	t2_language_option_put(t2_language_option_sel, TX_WHITE);
}

static void t2_language_option_selection_move(int direction)
{
	t2_language_option_put(t2_language_option_sel, TX_YELLOW);
	t2_language_option_sel += direction;
	if(t2_language_option_sel < 0) {
		t2_language_option_sel = (T2_LANGUAGE_OPTION_COUNT - 1);
	} else if(t2_language_option_sel >= T2_LANGUAGE_OPTION_COUNT) {
		t2_language_option_sel = 0;
	}
	t2_language_option_put(t2_language_option_sel, TX_WHITE);
}

static void t2_language_option_return_to_main(void)
{
	menu_sel = 3;
	in_option = false;
	t2_language_option_initialized = false;
}

void far pascal t2_language_option_update_and_render(void)
{
	int i;

	if(!t2_language_option_initialized) {
		t2_language_option_sel = 5;
		t2_language_option_input_allowed = false;
		text_clear();
		graph_showpage(1);
		graph_copy_page(0);
		t2_language_op_bridge(T2LOB_OPTION_SHADOW, 0, 0);
		t2_language_option_language_put(TX_BLACK);
		graph_showpage(0);
		for(i = 0; i < T2_LANGUAGE_OPTION_COUNT; i++) {
			t2_language_option_put(
				i, ((i == t2_language_option_sel) ? TX_WHITE : TX_YELLOW)
			);
		}
		t2_language_option_initialized = true;
		return;
	}
	if(!key_det) {
		t2_language_option_input_allowed = true;
	}
	if(!t2_language_option_input_allowed) {
		return;
	}
	if(key_det & INPUT_UP) {
		t2_language_option_selection_move(-1);
	}
	if(key_det & INPUT_DOWN) {
		t2_language_option_selection_move(+1);
	}
	if((key_det & INPUT_RIGHT) && (t2_language_option_sel <= 5)) {
		t2_language_option_change(+1);
	}
	if((key_det & INPUT_LEFT) && (t2_language_option_sel <= 5)) {
		t2_language_option_change(-1);
	}
	if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
		if(t2_language_option_sel == 6) {
			t2_language_op_bridge(T2LOB_OPTION_RESET, 0, 0);
			for(i = 0; i < 5; i++) {
				t2_language_option_put(i, TX_YELLOW);
			}
		} else if(t2_language_option_sel == 7) {
			t2_language_option_return_to_main();
		} else {
			t2_language_option_change(+1);
		}
	}
	if(key_det & INPUT_CANCEL) {
		t2_language_option_return_to_main();
	}
	if(key_det) {
		t2_language_option_input_allowed = false;
	}
}
