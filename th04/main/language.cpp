#pragma codeseg REPLAY_TEXT

#include "platform.h"
#include "x86real.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
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

extern "C" {
#if (GAME == 4)
	extern const char *STAGE_CLEAR_BONUS_DESC[];
	extern const char ALL_CLEAR[];
	extern const char POWERX50_2[];
	extern const char BONUS_DREAM_2[];
	extern const char GRAZEX50_2[];
	extern const char PLAYER_REM_10000[];
	extern const char PLAYER_REM_30000[];
	extern const char BONUS_POINT_2[];
	extern const char BONUS_TOTAL_2[];
	extern const char BONUS_STAGE[];
	extern const char POWERX50[];
	extern const char BONUS_DREAM[];
	extern const char GRAZEX50[];
	extern const char BONUS_POINT[];
	extern const char BONUS_TOTAL[];
#else
	extern const char near *STAGE_CLEAR_BONUS_DESC[];
	extern const char near *ALL_CLEAR;
	extern const char near *BONUS_STAGE;
	extern const char near *BONUS_DREAM;
	extern const char near *GRAZEX50;
	extern const char near *PLAYER_REM;
	extern const char near *POINT_ITEMS;
	extern const char near *BONUS_NOMISS;
	extern const char near *BONUS_NOBOMB;
	extern const char near *POINT_TOTAL;
	extern const char near *BONUS_TOTAL;
#endif
}

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
	LMT_CLEAR_DESC_TIMEOUT,
	LMT_CLEAR_DESC_LIVES_6,
	LMT_CLEAR_DESC_LIVES_5,
	LMT_CLEAR_DESC_LIVES_4,
	LMT_CLEAR_DESC_CONTINUE_1,
	LMT_CLEAR_DESC_CONTINUE_2,
	LMT_CLEAR_DESC_CONTINUE_3,
	LMT_CLEAR_DESC_EASY,
	LMT_CLEAR_DESC_NORMAL,
	LMT_CLEAR_DESC_HARD,
	LMT_CLEAR_DESC_LUNATIC,
	LMT_CLEAR_STAGE,
#if (GAME == 4)
	LMT_CLEAR_POWER,
#endif
	LMT_CLEAR_DREAM,
	LMT_CLEAR_GRAZE,
	LMT_CLEAR_POINT_ITEMS,
#if (GAME == 5)
	LMT_CLEAR_NO_MISS,
	LMT_CLEAR_NO_BOMB,
#endif
	LMT_CLEAR_TOTAL,
	LMT_CLEAR_ALL,
#if (GAME == 4)
	LMT_CLEAR_PLAYER_10000,
	LMT_CLEAR_PLAYER_30000,
#else
	LMT_CLEAR_PLAYER,
	LMT_CLEAR_POINT_ITEMS_TOTAL,
#endif
};

enum language_main_clear_bonus_text_t {
	LMCB_DESC_TIMEOUT,
	LMCB_DESC_LIVES_6,
	LMCB_DESC_LIVES_5,
	LMCB_DESC_LIVES_4,
	LMCB_DESC_CONTINUE_1,
	LMCB_DESC_CONTINUE_2,
	LMCB_DESC_CONTINUE_3,
	LMCB_DESC_EASY,
	LMCB_DESC_NORMAL,
	LMCB_DESC_HARD,
	LMCB_DESC_LUNATIC,
	LMCB_STAGE,
	LMCB_DREAM,
	LMCB_GRAZE,
	LMCB_POINT_ITEMS,
	LMCB_TOTAL,
	LMCB_ALL_CLEAR,
#if (GAME == 4)
	LMCB_POWER,
	LMCB_PLAYER_10000,
	LMCB_PLAYER_30000,
#else
	LMCB_PLAYER,
	LMCB_POINT_ITEMS_TOTAL,
	LMCB_NO_MISS,
	LMCB_NO_BOMB,
#endif
};

static const char far *language_main_text(unsigned index)
{
	unsigned blob_offset;
	const char far *p;
	_asm {
#if (GAME == 4)
		db 0xE8, 0x5E, 0x04
#else
		db 0xE8, 0x5B, 0x04
#endif
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

		// These strings are copied byte-for-byte from the executable-resident
		// clear-bonus labels in the English v1.00 donors. The final full-width
		// multiplier fields are Shift-JIS bytes.
		db 0x42,0x6F,0x73,0x73,0x27,0x73,0x20,0x66,0x69,0x6E,0x61,0x6C,0x20,0x70,0x68,0x61
		db 0x73,0x65,0x20,0x77,0x61,0x73,0x20,0x74,0x69,0x6D,0x65,0x64,0x20,0x6F,0x75,0x74
		db 0x3A,0x20,0x81,0x7E,0x81,0x40,0x82,0x4F,0x81,0x44,0x82,0x4F,0
		db 0x50,0x6C,0x61,0x79,0x65,0x72,0x20,0x50,0x65,0x6E,0x61,0x6C,0x74,0x79,0x20,0x28
		db 0x36,0x20,0x49,0x6E,0x69,0x74,0x69,0x61,0x6C,0x20,0x4C,0x69,0x76,0x65,0x73,0x29
		db 0x3A,0x20,0x81,0x7E,0x81,0x40,0x82,0x4F,0x81,0x44,0x82,0x52,0
		db 0x50,0x6C,0x61,0x79,0x65,0x72,0x20,0x50,0x65,0x6E,0x61,0x6C,0x74,0x79,0x20,0x28
		db 0x35,0x20,0x49,0x6E,0x69,0x74,0x69,0x61,0x6C,0x20,0x4C,0x69,0x76,0x65,0x73,0x29
		db 0x3A,0x20,0x81,0x7E,0x81,0x40,0x82,0x4F,0x81,0x44,0x82,0x54,0
		db 0x50,0x6C,0x61,0x79,0x65,0x72,0x20,0x50,0x65,0x6E,0x61,0x6C,0x74,0x79,0x20,0x28
		db 0x34,0x20,0x49,0x6E,0x69,0x74,0x69,0x61,0x6C,0x20,0x4C,0x69,0x76,0x65,0x73,0x29
		db 0x3A,0x20,0x81,0x7E,0x81,0x40,0x82,0x4F,0x81,0x44,0x82,0x56,0
		db 0x31,0x20,0x43,0x6F,0x6E,0x74,0x69,0x6E,0x75,0x65,0x20,0x55,0x73,0x65,0x64
		db 0x3A,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20
		db 0x20,0x81,0x40,0x81,0x7E,0x81,0x40,0x82,0x4F,0x81,0x44,0x82,0x57,0
		db 0x32,0x20,0x43,0x6F,0x6E,0x74,0x69,0x6E,0x75,0x65,0x73,0x20,0x55,0x73,0x65
		db 0x64,0x3A,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20
		db 0x20,0x81,0x40,0x81,0x7E,0x81,0x40,0x82,0x4F,0x81,0x44,0x82,0x55,0
		db 0x33,0x20,0x43,0x6F,0x6E,0x74,0x69,0x6E,0x75,0x65,0x73,0x20,0x55,0x73,0x65
		db 0x64,0x3A,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20
		db 0x20,0x81,0x40,0x81,0x7E,0x81,0x40,0x82,0x4F,0x81,0x44,0x82,0x53,0
		db 0x45,0x61,0x73,0x79,0x20,0x52,0x61,0x6E,0x6B,0x3A,0x20,0x20,0x20,0x20,0x20,0x20
		db 0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x81,0x40,0x81,0x40,0x81,0x40
		db 0x81,0x40,0x81,0x7E,0x81,0x40,0x82,0x4F,0x81,0x44,0x82,0x54,0
		db 0x4E,0x6F,0x72,0x6D,0x61,0x6C,0x20,0x52,0x61,0x6E,0x6B,0x3A,0x20,0x20,0x20,0x20
		db 0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x81,0x40
		db 0x81,0x40,0x81,0x7E,0x81,0x40,0x82,0x50,0x81,0x44,0x82,0x4F,0
		db 0x48,0x61,0x72,0x64,0x20,0x52,0x61,0x6E,0x6B,0x3A,0x20,0x20,0x20,0x20,0x20,0x20
		db 0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x81,0x40,0x81,0x40,0x81,0x40
		db 0x81,0x40,0x81,0x7E,0x81,0x40,0x82,0x50,0x81,0x44,0x82,0x51,0
		db 0x4C,0x75,0x6E,0x61,0x74,0x69,0x63,0x20,0x52,0x61,0x6E,0x6B,0x3A,0x20,0x20
		db 0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20
		db 0x20,0x81
		db 0x40,0x81,0x7E,0x81,0x40,0x82,0x50,0x81,0x44,0x82,0x53,0
		db 0x53,0x74,0x61,0x67,0x65,0x20,0x42,0x6F,0x6E,0x75,0x73,0x3A,0x20,0x20,0x20,0x20
		db 0x20,0x20,0x20,0x20,0x20,0x20,0
#if (GAME == 4)
		db 0x50,0x6F,0x77,0x65,0x72,0x20,0x4C,0x65,0x76,0x65,0x6C,0x20,0x81,0x7E,0x20,0x20
		db 0x82,0x54,0x82,0x4F,0x3A,0x20,0
#endif
		db 0x44,0x72,0x65,0x61,0x6D,0x20,0x42,0x6F,0x6E,0x75,0x73,0x20,0x81,0x69,0x96,0xB2
		db 0x81,0x6A,0x3A,0x20,0x20,0x20,0
		db 0x47,0x72,0x61,0x7A,0x65,0x20,0x43,0x6F,0x75,0x6E,0x74,0x20,0x81,0x7E,0x20,0x20
		db 0x82,0x54,0x82,0x4F,0x3A,0x20,0
#if (GAME == 4)
		db 0x50,0x6F,0x69,0x6E,0x74,0x20,0x49,0x74,0x65,0x6D,0x73,0x20,0x43,0x6F,0x6C,0x6C
		db 0x65,0x63,0x74,0x65,0x64,0x3A,0x81,0x40,0x81,0x40,0x81,0x40,0x81,0x40,0x81,0x40
		db 0x81,0x40,0x81,0x7E,0
#else
		db 0x50,0x6F,0x69,0x6E,0x74,0x20,0x49,0x74,0x65,0x6D,0x73,0x20,0x43,0x6F,0x6C,0x6C
		db 0x65,0x63,0x74,0x65,0x64,0x3A,0
		db 0x4E,0x6F,0x20,0x4D,0x69,0x73,0x73,0x20,0x42,0x6F,0x6E,0x75,0x73,0x3A,0x20,0x20,0
		db 0x4E,0x6F,0x20,0x42,0x6F,0x6D,0x62,0x20,0x42,0x6F,0x6E,0x75,0x73,0x3A,0x20,0x20,0
#endif
		db 0x42,0x6F,0x6E,0x75,0x73,0x20,0x54,0x6F,0x74,0x61,0x6C,0x3A,0x20,0x20,0x20,0x20,0
		db 0x41,0x6C,0x6C,0x20,0x43,0x6C,0x65,0x61,0x72,0x3A,0x20,0x20,0x20,0x20,0x20,0x20
		db 0x20,0x20,0x81,0x40,0x81,0x40,0
#if (GAME == 4)
		db 0x50,0x6C,0x61,0x79,0x65,0x72,0x20,0x20,0x81,0x40,0x81,0x7E,0x82,0x50,0x82,0x4F
		db 0x82,0x4F,0x82,0x4F,0x82,0x4F,0
		db 0x50,0x6C,0x61,0x79,0x65,0x72,0x20,0x20,0x81,0x40,0x81,0x7E,0x82,0x52,0x82,0x4F
		db 0x82,0x4F,0x82,0x4F,0x82,0x4F,0
#else
		db 0x50,0x6C,0x61,0x79,0x65,0x72,0x20,0x20,0x81,0x7E,0x20,0x20,0x82,0x50,0x82,0x4F
		db 0x82,0x4F,0x82,0x4F,0x82,0x4F,0
		db 0x50,0x6F,0x69,0x6E,0x74,0x20,0x49,0x74,0x65,0x6D,0x73,0x20,0x54,0x6F,0x74,0x61,0x6C
		db 0x3A,0x20,0x20,0x20,0x20,0
#endif
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

static const char *language_main_clear_bonus_text(
	unsigned id, const char *stock
)
{
	if(!language_main_english_selected()) {
		return stock;
	}
	if(id <= LMCB_DESC_LUNATIC) {
		return language_main_text(LMT_CLEAR_DESC_TIMEOUT + id);
	}
	switch(id) {
	case LMCB_STAGE: return language_main_text(LMT_CLEAR_STAGE);
	case LMCB_DREAM: return language_main_text(LMT_CLEAR_DREAM);
	case LMCB_GRAZE: return language_main_text(LMT_CLEAR_GRAZE);
	case LMCB_POINT_ITEMS: return language_main_text(LMT_CLEAR_POINT_ITEMS);
	case LMCB_TOTAL: return language_main_text(LMT_CLEAR_TOTAL);
	case LMCB_ALL_CLEAR: return language_main_text(LMT_CLEAR_ALL);
#if (GAME == 4)
	case LMCB_POWER: return language_main_text(LMT_CLEAR_POWER);
	case LMCB_PLAYER_10000:
		return language_main_text(LMT_CLEAR_PLAYER_10000);
	case LMCB_PLAYER_30000:
		return language_main_text(LMT_CLEAR_PLAYER_30000);
#else
	case LMCB_PLAYER: return language_main_text(LMT_CLEAR_PLAYER);
	case LMCB_POINT_ITEMS_TOTAL:
		return language_main_text(LMT_CLEAR_POINT_ITEMS_TOTAL);
	case LMCB_NO_MISS: return language_main_text(LMT_CLEAR_NO_MISS);
	case LMCB_NO_BOMB: return language_main_text(LMT_CLEAR_NO_BOMB);
#endif
	}
	return stock;
}

static unsigned language_main_clear_bonus_id(const char far *stock)
{
	int i;

	for(i = 0; i <= LMCB_DESC_LUNATIC; i++) {
		if(stock == STAGE_CLEAR_BONUS_DESC[i]) {
			return (LMCB_DESC_TIMEOUT + i);
		}
	}
#if (GAME == 4)
	if(stock == BONUS_STAGE) { return LMCB_STAGE; }
	if((stock == POWERX50) || (stock == POWERX50_2)) { return LMCB_POWER; }
	if((stock == BONUS_DREAM) || (stock == BONUS_DREAM_2)) { return LMCB_DREAM; }
	if((stock == GRAZEX50) || (stock == GRAZEX50_2)) { return LMCB_GRAZE; }
	if((stock == BONUS_POINT) || (stock == BONUS_POINT_2)) {
		return LMCB_POINT_ITEMS;
	}
	if((stock == BONUS_TOTAL) || (stock == BONUS_TOTAL_2)) { return LMCB_TOTAL; }
	if(stock == ALL_CLEAR) { return LMCB_ALL_CLEAR; }
	if(stock == PLAYER_REM_10000) { return LMCB_PLAYER_10000; }
	if(stock == PLAYER_REM_30000) { return LMCB_PLAYER_30000; }
#else
	if(stock == BONUS_STAGE) { return LMCB_STAGE; }
	if(stock == BONUS_DREAM) { return LMCB_DREAM; }
	if(stock == GRAZEX50) { return LMCB_GRAZE; }
	if(stock == POINT_ITEMS) { return LMCB_POINT_ITEMS; }
	if(stock == BONUS_TOTAL) { return LMCB_TOTAL; }
	if(stock == ALL_CLEAR) { return LMCB_ALL_CLEAR; }
	if(stock == PLAYER_REM) { return LMCB_PLAYER; }
	if(stock == POINT_TOTAL) { return LMCB_POINT_ITEMS_TOTAL; }
	if(stock == BONUS_NOMISS) { return LMCB_NO_MISS; }
	if(stock == BONUS_NOBOMB) { return LMCB_NO_BOMB; }
#endif
	return 0xFFFF;
}

void far pascal language_main_clear_bonus_putsa(
	unsigned x, unsigned y, const char far *stock, unsigned atrb
)
{
	unsigned id = language_main_clear_bonus_id(stock);

	text_putsa(x, y, language_main_clear_bonus_text(id, stock), atrb);
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

#if (GAME == 4)
#define LANGUAGE_MAIN_TH04_STAGE_TITLE_COUNT 8
#define LANGUAGE_MAIN_TH04_BGM_TITLE_COUNT 17

extern const shiftjis_t* BGM_TITLES[];
extern const shiftjis_t* STAGE_TITLES[];

static const shiftjis_t far *language_main_th04_title(unsigned index);
#endif

void language_main_titles_apply(void)
{
#if (GAME == 4)
	int i;

	if(!language_main_english_selected()) {
		return;
	}
	for(i = 0; i < LANGUAGE_MAIN_TH04_STAGE_TITLE_COUNT; i++) {
		STAGE_TITLES[i] = language_main_th04_title(i);
	}
	for(i = 0; i < LANGUAGE_MAIN_TH04_BGM_TITLE_COUNT; i++) {
		BGM_TITLES[i] = language_main_th04_title(
			LANGUAGE_MAIN_TH04_STAGE_TITLE_COUNT + i
		);
	}
#elif (GAME == 5)
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

#if (GAME == 4)
// The English patch keeps these HUD titles in MAIN.EXE rather than a data
// archive. Rebind the existing overlay pointer tables only for English;
// Japanese continues to use the stock tables unchanged.

static const shiftjis_t far *language_main_th04_title(unsigned index)
{
	unsigned blob_offset;
	const char far *p;
	_asm {
		db 0xE8, 0x21, 0x02
		db 'P', 'h', 'a', 'n', 't', 'o', 'm', ' ', 'L', 'a', 'n', 'd', 0
		db 'P', 'h', 'a', 'n', 't', 'o', 'm', ' ', 'N', 'i', 'g', 'h', 't', 0
		db 'F', 'a', 'm', 'i', 'n', 'e', ' ', 0x81, 0x60, ' ', 'L', 'a', 'k'
		db 'e', ' ', 'o', 'f', ' ', 'B', 'l', 'o', 'o', 'd', 0
		db 'D', 'a', 'r', 'k', 'n', 'e', 's', 's', 0
		db 'D', 'r', 'e', 'a', 'm', ' ', 'o', 'f', ' ', 'F', 'r', 'a', 'i'
		db 'l', ' ', 'G', 'i', 'r', 'l', 0
		db 'P', 'h', 'a', 'n', 't', 'a', 's', 'm', 'a', 'g', 'o', 'r', 'i'
		db 'a', 0
		db 'P', 'u', 'r', 's', 'u', 'i', 't', ' ', 0x81, 0x60, ' ', 'R', 'a'
		db 's', 'p', 'b', 'e', 'r', 'r', 'y', ' ', 'T', 'r', 'a', 'p', 0
		db 'Q', 'u', 'i', 'e', 't', ' ', 'C', 'l', 'o', 's', 'i', 'n', 'g'
		db ' ', 'f', 'o', 'r', ' ', 'W', 'o', 'n', 'd', 'e', 'r', 'f', 'u'
		db 'l', ' ', 'Y', 'o', 'u', ' ', 0x81, 0x60, ' ', 'P', 'u', 'c', 'k'
		db 'i', 's', 'h', ' ', 'A', 'n', 'g', 'e', 'l', 0
		db 'W', 'i', 't', 'c', 'h', 'i', 'n', 'g', ' ', 'D', 'r', 'e', 'a'
		db 'm', 0, 'S', 'e', 'l', 'e', 'n', 'e', 0x27, 's', ' ', 'L', 'i'
		db 'g', 'h', 't', 0, 'D', 'e', 'c', 'o', 'r', 'a', 't', 'i', 'o'
		db 'n', ' ', 'B', 'a', 't', 't', 'l', 'e', 0
		db 'B', 'r', 'e', 'a', 'k', ' ', 't', 'h', 'e', ' ', 'S', 'a', 'b'
		db 'b', 'a', 't', 'h', 0
		db 'S', 'c', 'a', 'r', 'l', 'e', 't', ' ', 'S', 'y', 'm', 'p', 'h'
		db 'o', 'n', 'y', ' ', 0x81, 0x60, ' ', 'S', 'c', 'a', 'r', 'l', 'e'
		db 't', ' ', 'P', 'h', 'o', 'n', 'e', 'm', 'e', 0
		db 'B', 'a', 'd', ' ', 'A', 'p', 'p', 'l', 'e', '!', '!', 0
		db 'S', 'p', 'i', 'r', 'i', 't', ' ', 'B', 'a', 't', 't', 'l', 'e'
		db ' ', 0x81, 0x60, ' ', 'P', 'e', 'r', 'd', 'i', 't', 'i', 'o', 'n'
		db ' ', 'C', 'r', 'i', 's', 'i', 's', 0
		db 'A', 'l', 'i', 'c', 'e', ' ', 'M', 'a', 'e', 's', 't', 'r', 'a', 0
		db 'V', 'e', 's', 's', 'e', 'l', ' ', 'o', 'f', ' ', 'S', 't', 'a'
		db 'r', 's', ' ', 0x81, 0x60, ' ', 'C', 'a', 's', 'k', 'e', 't', ' '
		db 'o', 'f', ' ', 'S', 't', 'a', 'r', 0
		db 'L', 'o', 't', 'u', 's', ' ', 'L', 'o', 'v', 'e', 0
		db 'S', 'l', 'e', 'e', 'p', 'i', 'n', 'g', ' ', 'T', 'e', 'r', 'r'
		db 'o', 'r', 0, 'D', 'r', 'e', 'a', 'm', ' ', 'L', 'a', 'n', 'd', 0
		db 'F', 'a', 'i', 'n', 't', ' ', 'D', 'r', 'e', 'a', 'm', ' ', 0x81
		db 0x60, ' ', 'I', 'n', 'a', 'n', 'i', 'm', 'a', 't', 'e', ' ', 'D'
		db 'r', 'e', 'a', 'm', 0
		db 'T', 'h', 'e', ' ', 'I', 'n', 'e', 'v', 'i', 't', 'a', 'b', 'l'
		db 'y', ' ', 'F', 'o', 'r', 'b', 'i', 'd', 'd', 'e', 'n', ' ', 'G'
		db 'a', 'm', 'e', 0
		db 'I', 'l', 'l', 'u', 's', 'i', 'o', 'n', ' ', 'o', 'f', ' ', 'a'
		db ' ', 'M', 'a', 'i', 'd', ' ', 0x81, 0x60, ' ', 'I', 'c', 'e', 'm'
		db 'i', 'l', 'k', ' ', 'M', 'a', 'g', 'i', 'c', ' ', 0
		db 'C', 'u', 't', 'e', ' ', 'D', 'e', 'v', 'i', 'l', ' ', 0x81, 0x60
		db ' ', 'I', 'n', 'n', 'o', 'c', 'e', 'n', 'c', 'e', 0
		db 'M', 'a', 'i', 'd', 'e', 'n', 0x27, 's', ' ', 'C', 'a', 'p', 'r'
		db 'i', 'c', 'c', 'i', 'o', 0
		pop ax
		mov blob_offset, ax
	}
	p = reinterpret_cast<const char far *>(MK_FP(_CS, blob_offset));
	while(index--) {
		while(*p++) {
		}
	}
	return reinterpret_cast<const shiftjis_t far *>(p);
}

#endif

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

#if (GAME == 5)
#define LANGUAGE_MAIN_HUD_GAIJI_COUNT 26
#define LANGUAGE_MAIN_HUD_GAIJI_CELL_SIZE 32
#define LANGUAGE_MAIN_HUD_GAIJI_HEADER_SIZE 8

static uint8_t language_main_hud_gaiji_id(unsigned index)
{
	if(index < 9) {
		return (0x5C + index);
	}
	if(index < 15) {
		return (0xDA + (index - 9));
	}
	switch(index) {
	case 15: return 0xE6;
	case 16: return 0xE8;
	case 17: return 0xEA;
	case 18: return 0xED;
	case 19: return 0xEE;
	default: return (0xF0 + (index - 20));
	}
}

static bool language_main_hud_gaiji_header_valid(const uint8_t *header)
{
	return (
		(header[0] == 'T') && (header[1] == '5') &&
		(header[2] == 'H') && (header[3] == 'G') &&
		(header[4] == 1) &&
		(header[5] == LANGUAGE_MAIN_HUD_GAIJI_COUNT) &&
		(header[6] == LANGUAGE_MAIN_HUD_GAIJI_CELL_SIZE) &&
		(header[7] == 0)
	);
}

static const char far *language_main_hud_gaiji_fn(void)
{
	unsigned blob_offset;
	_asm {
		db 0xE8, 0x0A, 0x00
		db 'T', '5', 'H', 'U', 'D', '.', 'G', 'F', 'T', 0
		pop ax
		mov blob_offset, ax
	}
	return reinterpret_cast<const char far *>(MK_FP(_CS, blob_offset));
}

static bool language_main_hud_gaiji_validate(void)
{
	uint8_t header[LANGUAGE_MAIN_HUD_GAIJI_HEADER_SIZE];
	uint8_t pattern[LANGUAGE_MAIN_HUD_GAIJI_CELL_SIZE];
	uint8_t id;
	uint8_t extra;
	unsigned i;
	const char far *fn;

	fn = language_main_hud_gaiji_fn();
	if(!file_ropen(fn)) {
		return false;
	}
	if(
		(file_read(header, sizeof(header)) != sizeof(header)) ||
		!language_main_hud_gaiji_header_valid(header)
	) {
		file_close();
		return false;
	}
	for(i = 0; i < LANGUAGE_MAIN_HUD_GAIJI_COUNT; i++) {
		if(
			(file_read(&id, sizeof(id)) != sizeof(id)) ||
			(id != language_main_hud_gaiji_id(i)) ||
			(file_read(pattern, sizeof(pattern)) != sizeof(pattern)) ||
			(file_read(pattern, sizeof(pattern)) != sizeof(pattern))
		) {
			file_close();
			return false;
		}
	}
	if(file_read(&extra, sizeof(extra)) != 0) {
		file_close();
		return false;
	}
	file_close();
	return true;
}

static void language_main_hud_gaiji_write_cell(
	uint8_t id, const uint8_t *pattern
)
{
	uint16_t jis = (0x5680 + id);
	uint8_t row;

	jis &= 0xFF7F;
	outportb(0x68, 0x0B);
	outportb(0xA1, static_cast<uint8_t>(jis));
	outportb(0xA3, static_cast<uint8_t>(jis >> 8));
	for(row = 0; row < 16; row++) {
		outportb(0xA5, (row | 0x20));
		outportb(0xA9, *pattern++);
		outportb(0xA5, row);
		outportb(0xA9, *pattern++);
	}
	outportb(0x68, 0x0A);
}

static bool language_main_hud_gaiji_write(bool english)
{
	uint8_t header[LANGUAGE_MAIN_HUD_GAIJI_HEADER_SIZE];
	uint8_t pattern[LANGUAGE_MAIN_HUD_GAIJI_CELL_SIZE];
	uint8_t id;
	unsigned i;
	const char far *fn;

	fn = language_main_hud_gaiji_fn();
	if(!file_ropen(fn)) {
		return false;
	}
	if(file_read(header, sizeof(header)) != sizeof(header)) {
		file_close();
		return false;
	}
	for(i = 0; i < LANGUAGE_MAIN_HUD_GAIJI_COUNT; i++) {
		if(file_read(&id, sizeof(id)) != sizeof(id)) {
			file_close();
			return false;
		}
		if(english) {
			file_seek(LANGUAGE_MAIN_HUD_GAIJI_CELL_SIZE, SEEK_CUR);
		}
		if(
			file_read(pattern, sizeof(pattern)) != sizeof(pattern)
		) {
			file_close();
			return false;
		}
		if(!english) {
			file_seek(LANGUAGE_MAIN_HUD_GAIJI_CELL_SIZE, SEEK_CUR);
		}
		language_main_hud_gaiji_write_cell(id, pattern);
	}
	file_close();
	return true;
}

void language_main_hud_gaiji_apply(void)
{
	char overlay_fn[13];

	language_main_overlay_fn_set(overlay_fn);
	if(!language_main_overlay_available(overlay_fn)) {
		return;
	}
	pfend();
	pfstart(reinterpret_cast<const unsigned char *>(overlay_fn));
	if(language_main_hud_gaiji_validate()) {
		language_main_hud_gaiji_write(language_main_preference_is_english());
	}
	pfend();
	language_main_stock_archive_restore();
}
#endif

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
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#else
	#pragma codestring "\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90"
	// Keep the stock TH05 `_TEXTC` segment on paragraph phase 0xD after the
	// dual-language HUD gaiji loader above.
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	// The replay-owned activation call contributes five bytes in this segment;
	// retain the following stock segment's frozen paragraph phase.
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#endif

// v0.1.1-rc4 grows only patch-owned replay tails. Preserve the frozen CRT
// paragraph phase without moving any native code inside its own segment.
#if (GAME == 4)
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90"
#else
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#endif
