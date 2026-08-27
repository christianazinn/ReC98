#include "libs/master.lib/master.hpp"
#include "th02/formats/pf.hpp"
#include "th02/formats/pi.h"

#pragma codeseg T2LANGMAIN_TEXT

static bool t2_language_main_loaded;
static bool t2_language_main_file_switched;

static uint8_t t2_language_main_ascii_upper(uint8_t c)
{
	if((c >= 'a') && (c <= 'z')) {
		return static_cast<uint8_t>(c - ('a' - 'A'));
	}
	return c;
}

static bool t2_language_main_member(const char *fn)
{
	int length = 0;

	while((length <= 10) && fn[length]) {
		length++;
	}
	#define C(i, c) (t2_language_main_ascii_upper(fn[i]) == c)
	if(
		(length == 6) && C(0, 'E') && C(1, 'Y') && C(2, 'E') &&
		C(3, '.') && C(4, 'P') && C(5, 'I') && (fn[6] == '\0')
	) {
		return true;
	}
	if(
		(length == 10) && C(0, 'S') && C(1, 'T') && C(2, 'A') &&
		C(3, 'G') && C(4, 'E') && (fn[5] >= '0') && (fn[5] <= '5') &&
		C(6, '.') && C(7, 'T') && C(8, 'X') && C(9, 'T') &&
		(fn[10] == '\0')
	) {
		return true;
	}
	#undef C
	return false;
}

static void t2_language_main_load(void)
{
	if(!t2_language_main_loaded) {
		t2_language_load();
		t2_language_main_loaded = true;
	}
}

static void t2_language_main_stock_restore(void)
{
	pfend();
	game_pfopen();
}

static bool t2_language_main_begin(const char *fn)
{
	char overlay_fn[9];

	t2_language_main_load();
	if(!t2_language_english_ready() || !t2_language_main_member(fn)) {
		return false;
	}
	t2_language_overlay_fn_set(overlay_fn);
	pfend();
	pfstart(reinterpret_cast<const unsigned char *>(overlay_fn));
	if(file_ropen(fn)) {
		file_close();
		return true;
	}
	t2_language_main_stock_restore();
	return false;
}

int far pascal t2_language_main_pi_load(int slot, const char *fn)
{
	bool switched = t2_language_main_begin(fn);
	int ret = pi_load(slot, fn);

	if(switched) {
		t2_language_main_stock_restore();
		if(ret != 0) {
			ret = pi_load(slot, fn);
		}
	}
	return ret;
}

int far pascal t2_language_main_file_ropen(const char *fn)
{
	if(t2_language_main_file_switched) {
		return 0;
	}
	if(t2_language_main_begin(fn)) {
		if(file_ropen(fn)) {
			t2_language_main_file_switched = true;
			return 1;
		}
		t2_language_main_stock_restore();
	}
	return file_ropen(fn);
}

void far pascal t2_language_main_file_close(void)
{
	file_close();
	if(t2_language_main_file_switched) {
		t2_language_main_file_switched = false;
		t2_language_main_stock_restore();
	}
}

#pragma codeseg
