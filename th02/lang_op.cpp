// Link this patch-owned translation unit last in OP.EXE.
#include "th02/language.cpp"

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "shiftjis.hpp"
#include "th02/common.h"
#include "th02/formats/pf.hpp"
#include "th02/formats/pi.h"
#include "th02/shiftjis/hiscore.hpp"

extern const shiftjis_t far t2op_en_reduce[];
extern const shiftjis_t far t2op_en_hiscore[];
extern const shiftjis_t far t2op_en_character[];
extern const shiftjis_t far t2op_en_music[];

extern const shiftjis_t *REDUCE_LABEL;
extern const shiftjis_t *REDUCE_VALUES[2];
extern const shiftjis_t *SHOTTYPES[SHOTTYPE_COUNT];
extern const shiftjis_t *DESC[SHOTTYPE_COUNT][3];
extern const shiftjis_t *CHOOSE;
extern const shiftjis_t *EXTRA_NOTE[2];
extern const shiftjis_t *CLEARED;
extern const shiftjis_t *MUSIC_CHOICES[17];
extern char need_op_h_bft;

#pragma codeseg T2LANGOP_TEXT

static bool t2_language_op_file_switched;

static bool t2_op_tables_ready;
static const shiftjis_t *t2_language_jp_reduce_label;
static const shiftjis_t *t2_language_jp_reduce_values[2];
static const shiftjis_t *t2_language_jp_shottypes[SHOTTYPE_COUNT];
static const shiftjis_t *t2_language_jp_desc[SHOTTYPE_COUNT][3];
static const shiftjis_t *t2_language_jp_choose;
static const shiftjis_t *t2_language_jp_extra_note[2];
static const shiftjis_t *t2_language_jp_cleared;
static const shiftjis_t *t2_language_jp_music_choices[17];

#define T2_OP_HISCORE_HEADER_SIZE 54
static shiftjis_t t2_language_jp_hiscore_header[T2_OP_HISCORE_HEADER_SIZE];
static shiftjis_t far *t2_language_hiscore_header_target;

static void t2_op_hiscore_header_copy(
	shiftjis_t far *destination, const shiftjis_t far *source
)
{
	int i;

	for(i = 0; i < T2_OP_HISCORE_HEADER_SIZE; i++) {
		destination[i] = source[i];
	}
}

static void t2_op_tables_init(void)
{
	int i;

	if(t2_op_tables_ready) {
		return;
	}
	t2_language_jp_reduce_label = REDUCE_LABEL;
	for(i = 0; i < 2; i++) {
		t2_language_jp_reduce_values[i] = REDUCE_VALUES[i];
		t2_language_jp_extra_note[i] = EXTRA_NOTE[i];
	}
	for(i = 0; i < SHOTTYPE_COUNT; i++) {
		int line;

		t2_language_jp_shottypes[i] = SHOTTYPES[i];
		for(line = 0; line < 3; line++) {
			t2_language_jp_desc[i][line] = DESC[i][line];
		}
	}
	t2_language_jp_choose = CHOOSE;
	t2_language_jp_cleared = CLEARED;
	for(i = 0; i < 17; i++) {
		t2_language_jp_music_choices[i] = MUSIC_CHOICES[i];
	}
	// The stock header is an anonymous OP_04 literal. RC24 pins it 0x24 bytes
	// after need_op_h_bft; the P5 layout gate rejects any future movement.
	t2_language_hiscore_header_target = (
		reinterpret_cast<shiftjis_t far *>(&need_op_h_bft) + 0x24
	);
	t2_op_hiscore_header_copy(
		t2_language_jp_hiscore_header,
		t2_language_hiscore_header_target
	);
	t2_op_tables_ready = true;
}

void far t2_language_op_tables_apply(void)
{
	int i;

	t2_op_tables_init();
	if(t2_language_english_ready()) {
		REDUCE_LABEL = &t2op_en_reduce[0];
		REDUCE_VALUES[0] = &t2op_en_reduce[5];
		REDUCE_VALUES[1] = &t2op_en_reduce[14];
		for(i = 0; i < SHOTTYPE_COUNT; i++) {
			int line;

			SHOTTYPES[i] = &t2op_en_hiscore[(i == 0) ? 0 : ((i == 1) ? 7 : 12)];
			for(line = 0; line < 3; line++) {
				DESC[i][line] = &t2op_en_character[((i * 69) + (line * 23))];
			}
		}
		t2_op_hiscore_header_copy(
			t2_language_hiscore_header_target, &t2op_en_hiscore[17]
		);
		CHOOSE = &t2op_en_character[207];
		EXTRA_NOTE[0] = &t2op_en_character[252];
		EXTRA_NOTE[1] = &t2op_en_character[325];
		CLEARED = &t2op_en_character[398];
		for(i = 0; i < 17; i++) {
			MUSIC_CHOICES[i] = &t2op_en_music[(i * 33)];
		}
		return;
	}
	REDUCE_LABEL = t2_language_jp_reduce_label;
	for(i = 0; i < 2; i++) {
		REDUCE_VALUES[i] = t2_language_jp_reduce_values[i];
		EXTRA_NOTE[i] = t2_language_jp_extra_note[i];
	}
	for(i = 0; i < SHOTTYPE_COUNT; i++) {
		int line;

		SHOTTYPES[i] = t2_language_jp_shottypes[i];
		for(line = 0; line < 3; line++) {
			DESC[i][line] = t2_language_jp_desc[i][line];
		}
	}
	t2_op_hiscore_header_copy(
		t2_language_hiscore_header_target,
		t2_language_jp_hiscore_header
	);
	CHOOSE = t2_language_jp_choose;
	CLEARED = t2_language_jp_cleared;
	for(i = 0; i < 17; i++) {
		MUSIC_CHOICES[i] = t2_language_jp_music_choices[i];
	}
}

static uint8_t t2_language_ascii_upper(uint8_t c)
{
	if((c >= 'a') && (c <= 'z')) {
		return static_cast<uint8_t>(c - ('a' - 'A'));
	}
	return c;
}

static bool t2_language_op_member(const char *fn)
{
	int length = 0;

	while(fn[length] && (length <= 10)) {
		length++;
	}
	#define C(i, c) (t2_language_ascii_upper(fn[i]) == c)
	if((length == 6) && C(0, 'O') && C(1, 'P') && C(2, '2') && C(3, '.') && C(4, 'P') &&
		C(5, 'I') && (fn[6] == '\0')) {
		return true;
	}
	if((length == 10) && C(0, 'M') && C(1, 'I') && C(2, 'K') && C(3, 'O') && C(4, 'F') &&
		C(5, 'T') && C(6, '.') && C(7, 'B') && C(8, 'F') && C(9, 'T') &&
		(fn[10] == '\0')) {
		return true;
	}
	if((length == 9) && C(0, 'M') && C(1, 'U') && C(2, 'S') && C(3, 'I') && C(4, 'C') &&
		C(5, '.') && C(6, 'T') && C(7, 'X') && C(8, 'T') &&
		(fn[9] == '\0')) {
		return true;
	}
	#undef C
	return false;
}

static void t2_language_stock_restore(void)
{
	pfend();
	game_pfopen();
}

static bool t2_language_op_begin(const char *fn)
{
	char overlay_fn[9];

	if(!t2_language_english_ready() || !t2_language_op_member(fn)) {
		return false;
	}
	t2_language_overlay_fn_set(overlay_fn);
	pfend();
	pfstart(reinterpret_cast<const unsigned char *>(overlay_fn));
	if(file_ropen(fn)) {
		file_close();
		return true;
	}
	t2_language_stock_restore();
	return false;
}

int far pascal t2_language_pi_load(int slot, const char *fn)
{
	bool switched = t2_language_op_begin(fn);
	int ret = pi_load(slot, fn);

	if(switched) {
		t2_language_stock_restore();
		if(ret != 0) {
			ret = pi_load(slot, fn);
		}
	}
	return ret;
}

int far pascal t2_language_gaiji_entry_bfnt(const char *fn)
{
	// OP's first language-sensitive operation is its startup gaiji load. Loading
	// the preference here keeps the original main() contribution byte-stable.
	t2_language_load();
	t2_language_op_tables_apply();
	bool switched = t2_language_op_begin(fn);
	int ret = gaiji_entry_bfnt(fn);

	if(switched) {
		t2_language_stock_restore();
		if(ret == 0) {
			ret = gaiji_entry_bfnt(fn);
		}
	}
	return ret;
}

int far pascal t2_language_file_ropen(const char *fn)
{
	if(t2_language_op_file_switched) {
		return 0;
	}
	if(t2_language_op_begin(fn)) {
		if(file_ropen(fn)) {
			t2_language_op_file_switched = true;
			return 1;
		}
		t2_language_stock_restore();
	}
	return file_ropen(fn);
}

void far pascal t2_language_file_close(void)
{
	file_close();
	if(t2_language_op_file_switched) {
		t2_language_op_file_switched = false;
		t2_language_stock_restore();
	}
}

void far pascal t2_language_option_text(char *label, char *value)
{
	#define P(dst, c) *dst++ = c
	if(t2_language_english_ready()) {
		P(label, 'L'); P(label, 'a'); P(label, 'n'); P(label, 'g');
		P(label, 'u'); P(label, 'a'); P(label, 'g'); P(label, 'e');
		P(value, 'E'); P(value, 'n'); P(value, 'g'); P(value, 'l');
		P(value, 'i'); P(value, 's'); P(value, 'h');
	} else {
		// Japanese label and value.
		P(label, 0x8C); P(label, 0xBE); P(label, 0x8C); P(label, 0xEA);
		P(value, 0x93); P(value, 0xFA); P(value, 0x96); P(value, 0x7B);
		P(value, 0x8C); P(value, 0xEA);
	}
	#undef P
	*label = '\0';
	*value = '\0';
}

#include "th02/op/language.cpp"

#pragma codeseg
