#pragma codeseg REPLAY_TEXT

#include "platform.h"
#include "libs/master.lib/master.hpp"
#include "th04/main/language.hpp"

#define LANGUAGE_MAIN_CONFIG_SIZE 8
#define LANGUAGE_MAIN_CONFIG_VERSION 1

static bool language_main_loaded;
static bool language_main_english;
static bool language_main_dialog_overlay_active;

#if (GAME == 5)
	extern "C" const unsigned char aKAIKIDAN2_DAT[];
#else
	extern "C" const unsigned char aUmx[];
#endif

static uint8_t language_main_ascii_upper(uint8_t c)
{
	if((c >= 'a') && (c <= 'z')) {
		return (c - ('a' - 'A'));
	}
	return c;
}

static void language_main_config_fn_set(char *fn)
{
	fn[0] = 'T';
	fn[1] = ('0' + GAME);
	fn[2] = 'L';
	fn[3] = 'A';
	fn[4] = 'N';
	fn[5] = 'G';
	fn[6] = '.';
	fn[7] = 'C';
	fn[8] = 'F';
	fn[9] = 'G';
	fn[10] = '\0';
}

static uint8_t language_main_config_checksum(const uint8_t *data)
{
	uint8_t sum = 0;
	int i;

	for(i = 0; i < 6; i++) {
		sum += data[i];
	}
	return sum;
}

static bool language_main_preference_is_english(void)
{
	uint8_t data[LANGUAGE_MAIN_CONFIG_SIZE];
	uint8_t extra;
	uint8_t checksum;
	char fn[11];

	if(language_main_loaded) {
		return language_main_english;
	}
	language_main_loaded = true;
	language_main_english = false;
	language_main_config_fn_set(fn);
	if(!file_ropen(fn)) {
		return false;
	}
	if(
		(file_read(data, LANGUAGE_MAIN_CONFIG_SIZE) !=
		 LANGUAGE_MAIN_CONFIG_SIZE) ||
		(file_read(&extra, 1) != 0)
	) {
		file_close();
		return false;
	}
	file_close();
	checksum = language_main_config_checksum(data);
	if(
		(data[0] != 'T') || (data[1] != ('0' + GAME)) ||
		(data[2] != 'L') || (data[3] != 'G') ||
		(data[4] != LANGUAGE_MAIN_CONFIG_VERSION) ||
		(data[5] != 1) ||
		(data[6] != checksum) ||
		(data[7] != static_cast<uint8_t>(~checksum))
	) {
		return false;
	}
	language_main_english = true;
	return true;
}

static bool language_main_dialog_member(const char *fn)
{
	uint8_t i;

	if(
		(fn[0] != '_') ||
		(language_main_ascii_upper(fn[1]) != 'D') ||
		(language_main_ascii_upper(fn[2]) != 'M')
	) {
		return false;
	}
	for(i = 3; (i < 9) && (fn[i] != '.'); i++) {
		if(fn[i] == '\0') {
			return false;
		}
	}
	if((i == 9) || (fn[i] != '.')) {
		return false;
	}
#if (GAME == 5)
	return (
		(language_main_ascii_upper(fn[i + 1]) == 'T') &&
		(language_main_ascii_upper(fn[i + 2]) == 'X') &&
		(fn[i + 3] == '2') && (fn[i + 4] == '\0')
	);
#else
	return (
		(language_main_ascii_upper(fn[i + 1]) == 'T') &&
		(language_main_ascii_upper(fn[i + 2]) == 'X') &&
		(language_main_ascii_upper(fn[i + 3]) == 'T') &&
		(fn[i + 4] == '\0')
	);
#endif
}

static void language_main_overlay_fn_set(char *fn)
{
	fn[0] = 'T';
	fn[1] = ('0' + GAME);
	fn[2] = 'E';
	fn[3] = 'N';
	fn[4] = 'M';
	fn[5] = 'A';
	fn[6] = 'I';
	fn[7] = 'N';
	fn[8] = '.';
	fn[9] = 'D';
	fn[10] = 'A';
	fn[11] = 'T';
	fn[12] = '\0';
}

static bool language_main_overlay_available(const char *fn)
{
	if(!file_ropen(fn)) {
		return false;
	}
	file_close();
	return true;
}

static void language_main_stock_archive_restore(void)
{
#if (GAME == 5)
	pfstart(aKAIKIDAN2_DAT);
#else
	pfstart(aUmx);
#endif
}

int far pascal language_main_dialog_ropen(const char *fn)
{
	char overlay_fn[13];

	language_main_dialog_overlay_active = false;
	if(
		!language_main_preference_is_english() ||
		!language_main_dialog_member(fn)
	) {
		return file_ropen(fn);
	}
	language_main_overlay_fn_set(overlay_fn);
	if(!language_main_overlay_available(overlay_fn)) {
		return file_ropen(fn);
	}
	pfend();
	pfstart(reinterpret_cast<const unsigned char *>(overlay_fn));
	if(file_ropen(fn)) {
		language_main_dialog_overlay_active = true;
		return true;
	}
	pfend();
	language_main_stock_archive_restore();
	return file_ropen(fn);
}

void far pascal language_main_dialog_close(void)
{
	file_close();
	if(language_main_dialog_overlay_active) {
		language_main_dialog_overlay_active = false;
		pfend();
		language_main_stock_archive_restore();
	}
}

#if (GAME == 4)
static bool language_main_gaiji_member(const char *fn)
{
	return (
		(language_main_ascii_upper(fn[0]) == 'G') &&
		(language_main_ascii_upper(fn[1]) == 'A') &&
		(language_main_ascii_upper(fn[2]) == 'M') &&
		(language_main_ascii_upper(fn[3]) == 'E') &&
		(language_main_ascii_upper(fn[4]) == 'F') &&
		(language_main_ascii_upper(fn[5]) == 'T') &&
		(fn[6] == '.') &&
		(language_main_ascii_upper(fn[7]) == 'B') &&
		(language_main_ascii_upper(fn[8]) == 'F') &&
		(language_main_ascii_upper(fn[9]) == 'T') &&
		(fn[10] == '\0')
	);
}

int far pascal language_main_gaiji_entry_bfnt(const char *fn)
{
	char overlay_fn[13];

	if(
		!language_main_preference_is_english() ||
		!language_main_gaiji_member(fn)
	) {
		return gaiji_entry_bfnt(fn);
	}
	language_main_overlay_fn_set(overlay_fn);
	if(!language_main_overlay_available(overlay_fn)) {
		return gaiji_entry_bfnt(fn);
	}
	pfend();
	pfstart(reinterpret_cast<const unsigned char *>(overlay_fn));
	if(gaiji_entry_bfnt(fn)) {
		pfend();
		language_main_stock_archive_restore();
		return true;
	}
	pfend();
	language_main_stock_archive_restore();
	return gaiji_entry_bfnt(fn);
}
#endif

// The body is never called. It is an explicit, measurable tail contribution
// used only to retain the original CRT paragraph phase after this module.
void far pascal language_main_layout_pad(void)
{
	_asm { nop; }
}

#if (GAME == 4)
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#else
	#pragma codestring "\x90\x90\x90"
#endif
