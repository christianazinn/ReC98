// Patch-owned post-registration replay-save decision. This file is included
// after POINTNUM_TEXT so the five-byte far call in continue_prompt() is its
// only original-code change.

#pragma codeseg T2GOSAVE_TEXT

#include "planar.h"
#include "th01/hardware/grppsafx.h"
#include "th01/hardware/grppsafx.cpp"
#include "th02/language.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/replay.hpp"
#include "th02/v_colors.hpp"

enum {
	T2GOSAVE_SELECTED_COLOR = 6,
	T2GOSAVE_QUESTION_TOP = 192,
	T2GOSAVE_YES_TOP = 224,
	T2GOSAVE_NO_TOP = 248,
};

static void t2gosave_graph_putja_fx(
	screen_x_t left, vram_y_t top, int16_t col_and_fx,
	const jis_t *text, int length
)
{
	jis_t codepoint;
	dots_t(GLYPH_FULL_W) glyph_row;
	dots8_t far *vram;
	int fullwidth;
	int first_bit;
	int weight = (col_and_fx >> 4) & 3;
	pixel_t spacing = (col_and_fx >> 6) & 7;
	pixel_t line;
	dot_rect_t(GLYPH_FULL_W, GLYPH_H) glyph;
	register dots_t(GLYPH_FULL_W) glyph_row_tmp;

	grcg_setcolor(GC_RMW, col_and_fx);
	outportb(0x68, 0xB);
	while(length--) {
		codepoint = *text++;
		fullwidth = (codepoint > 0xFF);
		if(!fullwidth) {
			codepoint = (
				(codepoint >= 0x21 && codepoint <= 0x7E)
				? (codepoint + 0x2900)
				: 0x2B21
			);
		}
		set_vram_ptr(vram, first_bit, left, top);
		outportb(0xA1, codepoint & 0xFF);
		outportb(0xA3, (codepoint >> 8) - 0x20);
		if(fullwidth) {
			for(line = 0; line < GLYPH_H; line++) {
				outportb(0xA5, line | 0x20);
				glyph[line] = inportb(0xA9) << 8;
				outportb(0xA5, line);
				glyph[line] += inportb(0xA9);
			}
		} else {
			for(line = 0; line < GLYPH_H; line++) {
				outportb(0xA5, line | 0x20);
				glyph[line] = (inportb(0xA9) << 8);
			}
		}
		for(line = 0; line < GLYPH_H; line++) {
			apply_weight(glyph_row, glyph[line], glyph_row_tmp, weight);
			put_row_and_advance(vram, glyph_row, first_bit);
		}
		advance_left(left, fullwidth, spacing);
	}
	outportb(0x68, 0xA);
	grcg_off();
}

static void t2gosave_palette_black_in(void)
{
	int tone;

	for(tone = 0; tone < 100; tone += 6) {
		palette_settone(tone);
		frame_delay(1);
	}
	palette_settone(100);
}

static void t2gosave_text_put_both(
	screen_x_t left, vram_y_t top, int color, const jis_t *text, int length
)
{
	graph_accesspage(0);
	t2gosave_graph_putja_fx(left, top, (color | FX_WEIGHT_BOLD), text, length);
	graph_accesspage(1);
	t2gosave_graph_putja_fx(left, top, (color | FX_WEIGHT_BOLD), text, length);
	graph_accesspage(0);
}

static void t2gosave_prompt_put(bool save)
{
	const bool english = (t2_language_get() == T2LANG_ENGLISH);
	jis_t question[12];
	jis_t yes[3];
	jis_t no[3];
	const pixel_t question_w = (english ? (12 * 8) : (12 * 16));
	const pixel_t yes_w = (english ? (3 * 8) : (2 * 16));
	const pixel_t no_w = (english ? (2 * 8) : (3 * 16));
	const screen_x_t center = (PLAYFIELD_LEFT + (PLAYFIELD_W / 2));

	if(english) {
		question[0] = 'S'; question[1] = 'a'; question[2] = 'v';
		question[3] = 'e'; question[4] = ' '; question[5] = 'R';
		question[6] = 'e'; question[7] = 'p'; question[8] = 'l';
		question[9] = 'a'; question[10] = 'y'; question[11] = '?';
		yes[0] = 'Y'; yes[1] = 'e'; yes[2] = 's';
		no[0] = 'N'; no[1] = 'o';
	} else {
		question[0] = 0x256A; question[1] = 0x2557;
		question[2] = 0x256C; question[3] = 0x2524;
		question[4] = 0x2472; question[5] = 0x4A5D;
		question[6] = 0x4238; question[7] = 0x2437;
		question[8] = 0x245E; question[9] = 0x2439;
		question[10] = 0x242B; question[11] = 0x2129;
		yes[0] = 0x244F; yes[1] = 0x2424;
		no[0] = 0x2424; no[1] = 0x2424; no[2] = 0x2428;
	}

	t2gosave_text_put_both(
		static_cast<screen_x_t>(center - (question_w / 2)),
		T2GOSAVE_QUESTION_TOP, V_WHITE, question, 12
	);
	t2gosave_text_put_both(
		static_cast<screen_x_t>(center - (yes_w / 2)), T2GOSAVE_YES_TOP,
		save ? T2GOSAVE_SELECTED_COLOR : V_WHITE, yes, english ? 3 : 2
	);
	t2gosave_text_put_both(
		static_cast<screen_x_t>(center - (no_w / 2)), T2GOSAVE_NO_TOP,
		save ? V_WHITE : T2GOSAVE_SELECTED_COLOR, no, english ? 2 : 3
	);
}

static bool t2gosave_prompt(void)
{
	bool save = true;
	int confirmable = 0;

	t2gosave_prompt_put(save);
	t2gosave_palette_black_in();
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
