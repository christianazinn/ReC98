// Link this patch-owned translation unit last in OP.EXE.
#include "th02/language.cpp"

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/formats/pf.hpp"
#include "th02/formats/pi.h"

#pragma codeseg T2LANGOP_TEXT

static bool t2_language_op_file_switched;

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
