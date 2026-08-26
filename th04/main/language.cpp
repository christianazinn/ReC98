#pragma codeseg REPLAY_TEXT

#include "platform.h"
#include "x86real.h"
#include "libs/master.lib/master.hpp"
#include "th04/main/language.hpp"
#if (GAME == 5)
	#include "th04/main/hud/overlay.hpp"
	#include "th04/main/stage/stage.hpp"
	extern "C" shiftjis_t aTH05_10[];
	extern "C" shiftjis_t aTH05_11[];
#endif

#define LANGUAGE_MAIN_CONFIG_SIZE 8
#define LANGUAGE_MAIN_CONFIG_VERSION 1

static bool language_main_loaded;
static bool language_main_english;
static bool language_main_dialog_overlay_active;

enum language_main_text_t {
	LMT_STAGE_1,
	LMT_STAGE_2,
	LMT_STAGE_3,
	LMT_STAGE_4,
	LMT_STAGE_5,
	LMT_STAGE_6,
	LMT_STAGE_EXTRA,
	LMT_STAGE_BGM_1,
	LMT_STAGE_BGM_2,
	LMT_STAGE_BGM_3,
	LMT_STAGE_BGM_4,
	LMT_STAGE_BGM_5,
	LMT_STAGE_BGM_6,
	LMT_STAGE_BGM_EXTRA,
	LMT_BOSS_BGM_1,
	LMT_BOSS_BGM_2,
	LMT_BOSS_BGM_3,
	LMT_BOSS_BGM_4,
	LMT_BOSS_BGM_5,
	LMT_BOSS_BGM_6,
	LMT_BOSS_BGM_EXTRA,
	LMT_PAIR_YUKI,
	LMT_PAIR_MAI,
	LMT_PAUSE_RESUME,
	LMT_PAUSE_RESTART,
	LMT_PAUSE_SAVE,
	LMT_PAUSE_EXIT,
	LMT_PAUSED,
};

static const char far *language_main_text(unsigned index)
{
	unsigned blob_offset;
	const char far *p;
	_asm {
		db 0xE8, 0x98, 0x01
		db 'E', 'l', 'e', 'm', 'e', 'n', 't', 's', ' ', 'o', 'f', ' ', 'C', 'r', 'e', 'a'
		db 't', 'i', 'o', 'n', 0, 'M', 'a', 'g', 'i', 'c', 'a', 'l', ' ', 'S', 'p', 'a'
		db 'c', 'e', 0, 'M', 'a', 'k', 'a', 'i', 0, 'F', 'r', 'o', 'z', 'e', 'n', ' '
		db 'W', 'o', 'r', 'l', 'd', 0, 'L', 'a', 's', 't', ' ', 'J', 'u', 'd', 'g', 'e'
		db 'm', 'e', 'n', 't', 0, 'H', 'o', 'l', 'y', ' ', 'W', 'a', 'r', 0, 'O', 'p'
		db 'e', 'n', ' ', 'S', 'e', 's', 'a', 'm', 'e', 0, 'D', 'r', 'e', 'a', 'm', ' '
		db 'E', 'x', 'p', 'r', 'e', 's', 's', 0, 'D', 'i', 'm', 'e', 'n', 's', 'i', 'o'
		db 'n', ' ', 'o', 'f', ' ', 'R', 'e', 'v', 'e', 'r', 'i', 'e', 0, 'R', 'o', 'm'
		db 'a', 'n', 't', 'i', 'c', ' ', 'C', 'h', 'i', 'l', 'd', 'r', 'e', 'n', 0, 'M'
		db 'a', 'p', 'l', 'e', ' ', 'W', 'i', 's', 'e', 0, 'T', 'h', 'e', ' ', 'L', 'a'
		db 's', 't', ' ', 'J', 'u', 'd', 'g', 'e', 'm', 'e', 'n', 't', 0, 'W', 'o', 'r'
		db 'l', 'd', 0x27, 's', ' ', 'E', 'n', 'd', 0, 'A', 'l', 'i', 'c', 'e', ' ', 'i'
		db 'n', ' ', 'W', 'o', 'n', 'd', 'e', 'r', 'l', 'a', 'n', 'd', 0, 'M', 'a', 'g'
		db 'i', 'c', ' ', 'S', 'q', 'u', 'a', 'r', 'e', 0, 'S', 'p', 'i', 'r', 'i', 't'
		db 'u', 'a', 'l', ' ', 'H', 'e', 'a', 'v', 'e', 'n', 0, 'P', 'l', 'a', 's', 't'
		db 'i', 'c', ' ', 'M', 'i', 'n', 'd', 0, 'F', 'o', 'r', 'b', 'i', 'd', 'd', 'e'
		db 'n', ' ', 'M', 'a', 'g', 'i', 'c', 0, 'D', 'o', 'l', 'l', ' ', 'o', 'f', ' '
		db 'M', 'i', 's', 'e', 'r', 'y', 0, 'L', 'e', 'g', 'e', 'n', 'd', 'a', 'r', 'y'
		db ' ', 'I', 'l', 'l', 'u', 's', 'i', 'o', 'n', 0, 'T', 'h', 'e', ' ', 'G', 'r'
		db 'i', 'm', 'o', 'i', 'r', 'e', ' ', 'o', 'f', ' ', 'A', 'l', 'i', 'c', 'e', 0
		db 'C', 'r', 'i', 'm', 's', 'o', 'n', ' ', 'M', 'a', 'i', 'd', 'e', 'n', 0, 'T'
		db 'r', 'e', 'a', 'c', 'h', 'e', 'r', 'o', 'u', 's', ' ', 'M', 'a', 'i', 'd', 'e'
		db 'n', 0, 'R', 'e', 's', 'u', 'm', 'e', 0, 'R', 'e', 's', 't', 'a', 'r', 't'
		db 0, 'S', 'a', 'v', 'e', ' ', 'a', 'n', 'd', ' ', 'E', 'x', 'i', 't', 0, 'E'
		db 'x', 'i', 't', ' ', 'W', 'i', 't', 'h', 'o', 'u', 't', ' ', 'S', 'a', 'v', 'e'
		db 0, 'P', 'A', 'U', 'S', 'E', 'D', 0
		pop ax
		mov blob_offset, ax
	}
	p = reinterpret_cast<const char far *>(MK_FP(_CS, blob_offset));
	while(index--) {
		while(*p++) {
		}
	}
	return p;
}

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

bool16 language_main_english_selected(void)
{
	return language_main_preference_is_english();
}

#if (GAME == 5)
static void language_main_ascii_copy(
	shiftjis_t far *dest, const char far *source
)
{
	do {
		*dest = *source;
		dest++;
	} while(*source++);
}
#endif

void language_main_titles_apply(void)
{
#if (GAME == 5)
	if(!language_main_english_selected() ||
		(stage_id >= 7)) {
		return;
	}
	stage_title = reinterpret_cast<shiftjis_t far *>(
		const_cast<char far *>(language_main_text(LMT_STAGE_1 + stage_id))
	);
	stage_bgm_title = reinterpret_cast<shiftjis_t far *>(
		const_cast<char far *>(language_main_text(LMT_STAGE_BGM_1 + stage_id))
	);
	boss_bgm_title = reinterpret_cast<shiftjis_t far *>(
		const_cast<char far *>(language_main_text(LMT_BOSS_BGM_1 + stage_id))
	);
	if(stage_id == 4) {
		// The pair boss points to these stock buffers before the first dissolve
		// frame. Translate them in place rather than growing its hot update.
		language_main_ascii_copy(aTH05_10, language_main_text(LMT_PAIR_YUKI));
		language_main_ascii_copy(aTH05_11, language_main_text(LMT_PAIR_MAI));
	}
#endif
}

const char *language_main_pause_label(uint8_t option)
{
	if(!language_main_english_selected()) {
		return 0;
	}
	switch(option) {
	case 0: return language_main_text(LMT_PAUSE_RESUME);
	case 1: return language_main_text(LMT_PAUSE_RESTART);
	case 2: return language_main_text(LMT_PAUSE_SAVE);
	default: return language_main_text(LMT_PAUSE_EXIT);
	}
}

const char *language_main_pause_title(void)
{
	return language_main_english_selected()
		? language_main_text(LMT_PAUSED) : 0;
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
	// Keep the frozen TH04 CRT paragraph phase after the English title tables.
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90"
#else
	#pragma codestring "\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#endif
