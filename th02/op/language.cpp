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
#include "th02/practice_diag.hpp"

extern char menu_sel;
extern bool in_option;
extern unsigned char snd_bgm_mode;
extern resident_t __seg *resident_seg;

#define T2_LANGUAGE_OPTION_COUNT 8
#define T2_LANGUAGE_OPTION_LABEL_CENTER 256
#define T2_LANGUAGE_OPTION_VALUE_CENTER 400
#define T2_PERF_OPTION_LABEL_CENTER 224
#define T2_PERF_OPTION_VALUE_LEFT 336
#define T2_PERF_OPTION_VALUE_CELLS 8
#define T2_PERF_OPTION_TOP 320
#define T2_LANGUAGE_OPTION_TOP 336
#define T2_LANGUAGE_OPTION_VALUE_LEFT 344
#define T2_LANGUAGE_OPTION_VALUE_CELLS 7

static bool t2_language_option_initialized;
static bool t2_language_option_input_allowed;
static bool t2_language_option_reload_pending;
static int8_t t2_language_option_sel;

static uint8_t t2_language_option_gaiji(char c)
{
	if((c >= 'a') && (c <= 'z')) {
		c = static_cast<char>(c - ('a' - 'A'));
	}
	if((c >= 'A') && (c <= 'Z')) {
		if(c == 'M') {
			return gb_M;
		}
		if(c == 'N') {
			return gb_N;
		}
		return static_cast<uint8_t>(gb_A + (c - 'A'));
	}
	return gs_SPACE;
}

static void t2_language_option_native_center_put_at(
	screen_x_t center, screen_y_t top, tram_atrb2 atrb, const char *text
)
{
	uint8_t gaiji[9];
	unsigned length = 0;
	screen_x_t left;

	while((text[length] != '\0') && (length < (sizeof(gaiji) - 1))) {
		gaiji[length] = t2_language_option_gaiji(text[length]);
		length++;
	}
	gaiji[length] = gs_NULL;
	left = static_cast<screen_x_t>(center - ((length * GAIJI_W) / 2));
	graph_gaiji_puts(
		(left + 4), (top + 4), GAIJI_W,
		reinterpret_cast<const char *>(gaiji), 0
	);
	gaiji_putsa(
		(left / GLYPH_HALF_W), (top / GLYPH_H),
		reinterpret_cast<const char *>(gaiji), atrb
	);
}

static void t2_language_option_native_center_put(
	screen_x_t center, tram_atrb2 atrb, const char *text
)
{
	t2_language_option_native_center_put_at(
		center, T2_LANGUAGE_OPTION_TOP, atrb, text
	);
}

static void t2_language_option_japanese_center_put(tram_atrb2 atrb)
{
	// MIKOFT has no Japanese glyph cells. Keep this one native Japanese value
	// in TH02's Shift-JIS font rather than introducing a patch font table.
	static const shiftjis_t japanese[] = "\x93\xFA\x96\x7B\x8C\xEA";
	screen_x_t left = static_cast<screen_x_t>(
		T2_LANGUAGE_OPTION_VALUE_CENTER - ((3 * GAIJI_W) / 2)
	);

	graph_putsa_fx(left + 4, T2_LANGUAGE_OPTION_TOP + 4, 0, japanese);
	text_putsa(
		(left / GLYPH_HALF_W), (T2_LANGUAGE_OPTION_TOP / GLYPH_H), japanese,
		atrb
	);
}

static void t2_language_option_language_put(tram_atrb2 atrb)
{
	char clear[(T2_LANGUAGE_OPTION_VALUE_CELLS * GAIJI_TRAM_W) + 1];
	int i;

	// Patch-owned labels are intentionally English in both locales. The native
	// MIKOFT gaiji route owns their geometry on both VRAM and TRAM.
	t2_language_option_native_center_put(
		T2_LANGUAGE_OPTION_LABEL_CENTER, atrb, "Language"
	);
	// English is wider than the three full-width Japanese glyphs. Restore the
	// complete value field on both layers before drawing either form so the
	// outer EN...SH cells cannot survive a switch back to Japanese.
	graph_copy_rect_1_to_0_16(
		T2_LANGUAGE_OPTION_VALUE_LEFT, T2_LANGUAGE_OPTION_TOP,
		(T2_LANGUAGE_OPTION_VALUE_CELLS * GAIJI_W), (GLYPH_H + 4)
	);
	for(i = 0; i < (T2_LANGUAGE_OPTION_VALUE_CELLS * GAIJI_TRAM_W); i++) {
		clear[i] = ' ';
	}
	clear[T2_LANGUAGE_OPTION_VALUE_CELLS * GAIJI_TRAM_W] = '\0';
	text_putsa(
		(T2_LANGUAGE_OPTION_VALUE_LEFT / GLYPH_HALF_W),
		(T2_LANGUAGE_OPTION_TOP / GLYPH_H), clear, TX_BLACK
	);
	if(t2_language_get() == T2LANG_ENGLISH) {
		t2_language_option_native_center_put(
			T2_LANGUAGE_OPTION_VALUE_CENTER, atrb, "English"
		);
	} else {
		t2_language_option_japanese_center_put(atrb);
	}
}

static void t2_language_option_perf_put(tram_atrb2 atrb)
{
	char clear[(T2_PERF_OPTION_VALUE_CELLS * GAIJI_TRAM_W) + 1];
	int i;

	// Patch-owned labels remain English in both locales and use the same native
	// menu gaiji as every other Option row.
	t2_language_option_native_center_put_at(
		T2_PERF_OPTION_LABEL_CENTER, T2_PERF_OPTION_TOP, atrb, "Perf"
	);
	// Truncate is wider than Normal. Clear the entire value field in both VRAM
	// and TRAM before redrawing so its edge cells cannot survive as TNORMALE.
	graph_copy_rect_1_to_0_16(
		T2_PERF_OPTION_VALUE_LEFT, T2_PERF_OPTION_TOP,
		(T2_PERF_OPTION_VALUE_CELLS * GAIJI_W), (GLYPH_H + 4)
	);
	for(i = 0; i < (T2_PERF_OPTION_VALUE_CELLS * GAIJI_TRAM_W); i++) {
		clear[i] = ' ';
	}
	clear[(T2_PERF_OPTION_VALUE_CELLS * GAIJI_TRAM_W)] = '\0';
	text_putsa(
		(T2_PERF_OPTION_VALUE_LEFT / GLYPH_HALF_W),
		(T2_PERF_OPTION_TOP / GLYPH_H),
		clear, TX_BLACK
	);
	t2_language_option_native_center_put_at(
		T2_LANGUAGE_OPTION_VALUE_CENTER, T2_PERF_OPTION_TOP, atrb,
		resident->reduce_effects ? "Truncate" : "Normal"
	);
}

static void t2_language_option_put(int sel, tram_atrb2 atrb)
{
	if(sel == 4) {
		t2_language_option_perf_put(atrb);
	} else if(sel == 5) {
		t2_language_option_language_put(atrb);
	} else {
		t2_language_op_bridge(T2LOB_OPTION_PUT, sel, atrb);
	}
}

static bool t2_language_option_change(int direction)
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
			((target == T2LANG_JAPANESE) || t2_language_overlay_valid()) &&
			t2_language_set(target)
		) {
			// Keep the active Option surface on its current gaiji table. The
			// complete localized font/background transaction runs only after
			// this menu returns to the title and releases its drawing state.
			t2_language_option_reload_pending = true;
		}
		break;
	}
	}
	t2_language_option_put(t2_language_option_sel, TX_WHITE);
	return false;
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
	if(t2_language_option_reload_pending) {
		t2_language_option_reload_pending = false;
		t2practice_diag_lifecycle(T2PDLM_LANGUAGE_RETURN, 0, 0, 0);
	}
	// option_put_shadow() draws into the hidden page that stock OP uses as its
	// menu backing. Copying that page back here preserves the Option shadows as
	// black text over the title. Rebuild the clean localized title backing for
	// both language-changing and ordinary returns.
	replay_title_restore_request();
}

void far pascal t2_language_option_update_and_render(void)
{
	int i;

	if(!t2_language_option_initialized) {
		t2_language_option_sel = 5;
		t2_language_option_input_allowed = false;
		text_clear();
		graph_accesspage(1);
		graph_copy_page(0);
		graph_showpage(0);
		graph_accesspage(0);
		t2_language_op_bridge(T2LOB_OPTION_SHADOW, 0, 0);
		t2_language_option_perf_put(TX_BLACK);
		t2_language_option_language_put(TX_BLACK);
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
		if(t2_language_option_change(+1)) {
			return;
		}
	}
	if((key_det & INPUT_LEFT) && (t2_language_option_sel <= 5)) {
		if(t2_language_option_change(-1)) {
			return;
		}
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
			if(t2_language_option_change(+1)) {
				return;
			}
		}
	}
	if(key_det & INPUT_CANCEL) {
		t2_language_option_return_to_main();
	}
	if(key_det) {
		t2_language_option_input_allowed = false;
	}
}
