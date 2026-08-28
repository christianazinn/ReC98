#include "libs/master.lib/master.hpp"
#include "th02/formats/pf.hpp"
#include "th02/formats/pi.h"
#include "th02/gaiji/gaiji.h"
#include "th01/hardware/grppsafx.h"
#include "th02/language.hpp"

// This file is textually included after th02/language.cpp by lange.cpp, so it
// can share that TU's direct-DOS helpers and overlay-name constructor without
// adding another linked object or moving any retained MAINE contribution.
#pragma codeseg T2LANGMAINE_TEXT

static bool t2_language_maine_loaded;
static bool t2_language_maine_file_switched;

static uint8_t t2_language_maine_ascii_upper(uint8_t c)
{
	if((c >= 'a') && (c <= 'z')) {
		return static_cast<uint8_t>(c - ('a' - 'A'));
	}
	return c;
}

static bool t2_language_maine_member(const char *fn)
{
	int length = 0;

	while((length <= 10) && fn[length]) {
		length++;
	}
	#define C(i, c) (t2_language_maine_ascii_upper(fn[i]) == c)
	if(
		(length == 10) && C(0, 'M') && C(1, 'I') && C(2, 'K') &&
		C(3, 'O') && C(4, 'F') && C(5, 'T') && C(6, '.') &&
		C(7, 'B') && C(8, 'F') && C(9, 'T') && (fn[10] == '\0')
	) {
		return true;
	}
	if(
		(length == 8) && C(0, 'E') && C(1, 'N') && C(2, 'D') &&
		(fn[3] >= '1') && (fn[3] <= '3') && C(4, '.') && C(5, 'T') &&
		C(6, 'X') && C(7, 'T') && (fn[8] == '\0')
	) {
		return true;
	}
	if(
		(length == 7) && C(0, 'E') && C(1, 'D') && C(2, '0') &&
		((fn[3] == '2') || (fn[3] == '3') || (fn[3] == '5')) &&
		C(4, '.') && C(5, 'P') && C(6, 'I') && (fn[7] == '\0')
	) {
		return true;
	}
	#undef C
	return false;
}

static void t2_language_maine_load(void)
{
	if(!t2_language_maine_loaded) {
		t2_language_load();
		t2_language_maine_loaded = true;
	}
}

static void t2_language_maine_stock_restore(void)
{
	pfend();
	game_pfopen();
}

static bool t2_language_maine_begin(const char *fn)
{
	char overlay_fn[9];

	t2_language_maine_load();
	if(!t2_language_english_ready() || !t2_language_maine_member(fn)) {
		return false;
	}
	t2_language_overlay_fn_set(overlay_fn);
	pfend();
	pfstart(reinterpret_cast<const unsigned char *>(overlay_fn));
	if(file_ropen(fn)) {
		file_close();
		return true;
	}
	t2_language_maine_stock_restore();
	return false;
}

int far __cdecl t2_language_maine_pi_load(int slot, const char *fn)
{
	bool switched = t2_language_maine_begin(fn);
	int ret = pi_load(slot, fn);

	if(switched) {
		t2_language_maine_stock_restore();
		if(ret != 0) {
			ret = pi_load(slot, fn);
		}
	}
	return ret;
}

int far pascal t2_language_maine_gaiji_entry_bfnt(const char *fn)
{
	bool switched = t2_language_maine_begin(fn);
	int ret = gaiji_entry_bfnt(fn);

	if(switched) {
		t2_language_maine_stock_restore();
		if(ret == 0) {
			ret = gaiji_entry_bfnt(fn);
		}
	}
	return ret;
}

int far pascal t2_language_maine_file_ropen(const char *fn)
{
	if(t2_language_maine_file_switched) {
		return 0;
	}
	if(t2_language_maine_begin(fn)) {
		if(file_ropen(fn)) {
			t2_language_maine_file_switched = true;
			return 1;
		}
		t2_language_maine_stock_restore();
	}
	return file_ropen(fn);
}

void far pascal t2_language_maine_file_close(void)
{
	file_close();
	if(t2_language_maine_file_switched) {
		t2_language_maine_file_switched = false;
		t2_language_maine_stock_restore();
	}
}

// These are the immutable DS offsets of MAINE.EXE's retained Japanese
// literals. END_TEXT and MAINE_01_TEXT are pinned by the companion layout
// gate, so this dispatch adds no mutable presentation state or source-owner
// data table to DGROUP.
enum t2_language_maine_text_off_t {
	T2LMT_STAFF_PROGRAM = 0x1DB,
	T2LMT_STAFF_GRAPHIC_1 = 0x20E,
	T2LMT_STAFF_GRAPHIC_2 = 0x223,
	T2LMT_STAFF_GRAPHIC_3 = 0x23A,
	T2LMT_STAFF_TESTER_5 = 0x2D7,
	T2LMT_VERDICT_SCORE = 0x334,
	T2LMT_VERDICT_CONTINUES = 0x33F,
	T2LMT_VERDICT_RANK = 0x34E,
	T2LMT_VERDICT_START_LIVES = 0x358,
	T2LMT_VERDICT_START_BOMBS = 0x367,
	T2LMT_VERDICT_SKILL = 0x373,
};

extern const shiftjis_t far t2maine_en_staff_program[];
extern const shiftjis_t far t2maine_en_staff_graphic_1[];
extern const shiftjis_t far t2maine_en_staff_graphic_2[];
extern const shiftjis_t far t2maine_en_staff_graphic_3[];
extern const shiftjis_t far t2maine_en_staff_tester_5[];
extern const shiftjis_t far t2maine_en_verdict_score[];
extern const shiftjis_t far t2maine_en_verdict_continues[];
extern const shiftjis_t far t2maine_en_verdict_rank[];
extern const shiftjis_t far t2maine_en_verdict_start_lives[];
extern const shiftjis_t far t2maine_en_verdict_start_bombs[];
extern const shiftjis_t far t2maine_en_verdict_skill[];

static const shiftjis_t far *t2_language_maine_english_source(
	const shiftjis_t *str
)
{
	if(T2LANG_FP_SEG(str) != T2LANG_FP_SEG(&t2_language_runtime)) {
		return 0;
	}

	switch(T2LANG_FP_OFF(str)) {
		case T2LMT_STAFF_PROGRAM:
			return t2maine_en_staff_program;
		case T2LMT_STAFF_GRAPHIC_1:
			return t2maine_en_staff_graphic_1;
		case T2LMT_STAFF_GRAPHIC_2:
			return t2maine_en_staff_graphic_2;
		case T2LMT_STAFF_GRAPHIC_3:
			return t2maine_en_staff_graphic_3;
		case T2LMT_STAFF_TESTER_5:
			return t2maine_en_staff_tester_5;
		case T2LMT_VERDICT_SCORE:
			return t2maine_en_verdict_score;
		case T2LMT_VERDICT_CONTINUES:
			return t2maine_en_verdict_continues;
		case T2LMT_VERDICT_RANK:
			return t2maine_en_verdict_rank;
		case T2LMT_VERDICT_START_LIVES:
			return t2maine_en_verdict_start_lives;
		case T2LMT_VERDICT_START_BOMBS:
			return t2maine_en_verdict_start_bombs;
		case T2LMT_VERDICT_SKILL:
			return t2maine_en_verdict_skill;
	}
	return 0;
}

#define T2LMT_ENGLISH_TEXT_CAPACITY 25

extern "C" void DEFCONV t2_language_maine_graph_putsa_fx(
	screen_x_t left, vram_y_t top, int16_t col_and_fx, shiftjis_t *str
)
{
	shiftjis_t english[T2LMT_ENGLISH_TEXT_CAPACITY];
	const shiftjis_t *output = str;
	const shiftjis_t far *english_source;
	int i;

	t2_language_maine_load();
	if(t2_language_english_ready()) {
		english_source = t2_language_maine_english_source(str);
		if(english_source) {
			i = 0;
			do {
				english[i] = english_source[i];
			} while(english[i++]);
			output = english;
		}
	}
	graph_putsa_fx(left, top, col_and_fx, output);
}

#undef T2LMT_ENGLISH_TEXT_CAPACITY

#pragma codeseg
