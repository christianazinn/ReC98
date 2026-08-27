#pragma codeseg REPLAY_OP_TEXT

// Runtime language preference, intentionally limited to patch-owned UI. This
// parcel neither reads translated game data nor affects replay determinism.

#include "th04/op/language.hpp"
#include "th04/language_overlay.hpp"

// This file is included by rpyop.cpp after the established replay-tail
// umbrella. Several of the PC-98 headers have no include guards, so it must
// deliberately rely on that umbrella instead of re-including them here.

#define LANGUAGE_CONFIG_SIZE 8
#define LANGUAGE_CONFIG_VERSION 1
#define LANGUAGE_OPTION_STOCK_TOP ((GAME == 5) ? 250 : 224)
#define LANGUAGE_OPTION_TOP (LANGUAGE_OPTION_STOCK_TOP - LANGUAGE_OPTION_LABEL_H)
#define LANGUAGE_OPTION_LEFT 224
#define LANGUAGE_OPTION_W 192
#define LANGUAGE_OPTION_VALUE_LEFT 320
#define LANGUAGE_OPTION_CURSOR_RIGHT 384
#define LANGUAGE_OPTION_DESC_TOP (RES_Y - GLYPH_H)
#define LANGUAGE_OPTION_MENU_W 224
#define LANGUAGE_OPTION_MENU_H 164
#define LANGUAGE_OPTION_COMMAND_LEFT 272
#define LANGUAGE_OPTION_COMMAND_CURSOR_LEFT 256
#define LANGUAGE_OPTION_COMMAND_CURSOR_RIGHT 336
#define LANGUAGE_OPTION_LABEL_H 16
#define LANGUAGE_OPTION_COMMAND_H 20
#define LANGUAGE_OPTION_COL_INACTIVE ((GAME == 5) ? 8 : 1)
#define LANGUAGE_OPTION_COL_ACTIVE ((GAME == 5) ? 14 : 8)

static bool language_loaded;
static language_preference_t language_current;
static char language_config_fn[11];
static char language_menu_bgm_fn[3];
static char language_se_fn[5];
static bool language_option_initialized;
static bool language_option_input_allowed;
static int8_t language_option_sel;
static language_preference_t language_option_entry_preference;

enum language_option_choice_t {
	LOC_LANGUAGE,
	LOC_RANK,
	LOC_LIVES,
	LOC_BOMBS,
	LOC_BGM,
	LOC_SE,
	LOC_TURBO_OR_SLOW,
	LOC_RESET,
	LOC_QUIT,
	LOC_COUNT,
};

// English executable-resident text lives in this patch-owned code segment.
// Keeping it out of initialized _DATA preserves every original data address.
enum language_op_text_t {
	LOT_MAIN_START,
	LOT_MAIN_EXTRA,
	LOT_MAIN_PRACTICE,
	LOT_MAIN_HISCORE,
	LOT_MAIN_MUSIC,
	LOT_MAIN_REPLAY,
	LOT_MAIN_OPTIONS,
	LOT_MAIN_QUIT,
	LOT_DESC_1,
	LOT_DESC_2,
	LOT_DESC_3,
	LOT_DESC_4,
	LOT_DESC_5,
	LOT_DESC_6,
	LOT_DESC_7,
	LOT_DESC_8,
	LOT_DESC_9,
	LOT_DESC_10,
	LOT_DESC_11,
	LOT_DESC_12,
	LOT_DESC_13,
	LOT_DESC_14,
	LOT_DESC_15,
	LOT_DESC_16,
	LOT_DESC_17,
	LOT_DESC_18,
	LOT_DESC_19,
	LOT_DESC_20,
	LOT_DESC_21,
	LOT_DESC_22,
	LOT_DESC_23,
	LOT_DESC_24,
	LOT_DESC_25,
	LOT_LABEL_RANK,
	LOT_LABEL_LIVES,
	LOT_LABEL_BOMBS,
	LOT_LABEL_MUSIC,
	LOT_LABEL_SFX,
	LOT_LABEL_TURBO,
	LOT_LABEL_SLOW,
	LOT_LABEL_RESET,
	LOT_LABEL_QUIT,
	LOT_RANK_EASY,
	LOT_RANK_NORMAL,
	LOT_RANK_HARD,
	LOT_RANK_LUNATIC,
	LOT_NUMBER_0,
	LOT_NUMBER_1,
	LOT_NUMBER_2,
	LOT_NUMBER_3,
	LOT_NUMBER_4,
	LOT_NUMBER_5,
	LOT_NUMBER_6,
	LOT_BGM_OFF,
	LOT_BGM_26,
	LOT_BGM_86,
	LOT_SE_OFF,
	LOT_SE_FM,
	LOT_SE_BEEP,
	LOT_REIMU,
	LOT_REIMU_TYPE,
	LOT_MARISA,
	LOT_MARISA_TYPE,
	LOT_SEARCH_SHOT,
	LOT_WIDE_SHOT,
	LOT_IMAGE_LASER,
	LOT_RAPID_SHOT,
	LOT_CHOOSE_SHOT,
	LOT_CLEARED,
	LOT_PRACTICE_DESC,
	LOT_REPLAY_DESC,
	LOT_TH04_MUSIC_1,
	LOT_TH04_MUSIC_2,
	LOT_TH04_MUSIC_3,
	LOT_TH04_MUSIC_4,
	LOT_TH04_MUSIC_5,
	LOT_TH04_MUSIC_6,
	LOT_TH04_MUSIC_7,
	LOT_TH04_MUSIC_8,
	LOT_TH04_MUSIC_9,
	LOT_TH04_MUSIC_10,
	LOT_TH04_MUSIC_11,
	LOT_TH04_MUSIC_12,
	LOT_TH04_MUSIC_13,
	LOT_TH04_MUSIC_14,
	LOT_TH04_MUSIC_15,
	LOT_TH04_MUSIC_16,
	LOT_TH04_MUSIC_17,
	LOT_TH04_MUSIC_18,
	LOT_TH04_MUSIC_19,
	LOT_TH04_MUSIC_20,
	LOT_TH04_MUSIC_21,
	LOT_TH04_MUSIC_22,
	LOT_TH05_MUSIC_1,
};

static const char far *language_op_text(unsigned index)
{
	unsigned blob_offset;
	const char far *p;
	_asm {
		// A raw near CALL skips the 1836-byte blob and leaves its
		// first byte on the stack. TCC inline-assembly labels cannot be used as
		// address operands, so the measured displacement is encoded directly.
		db 0xE8, 0x2C, 0x07
		db 'S', 't', 'a', 'r', 't', 0, 'E', 'x', 't', 'r', 'a', ' ', 'S', 't', 'a', 'r'
		db 't', 0, 'P', 'r', 'a', 'c', 't', 'i', 'c', 'e', 0, 'H', 'i', 'S', 'c', 'o'
		db 'r', 'e', 0, 'M', 'u', 's', 'i', 'c', ' ', 'R', 'o', 'o', 'm', 0, 'R', 'e'
		db 'p', 'l', 'a', 'y', 0, 'O', 'p', 't', 'i', 'o', 'n', 's', 0, 'Q', 'u', 'i'
		db 't', 0, 'S', 't', 'a', 'r', 't', ' ', 't', 'h', 'e', ' ', 'E', 'x', 't', 'r'
		db 'a', ' ', 's', 't', 'a', 'g', 'e', 0, 'S', 'h', 'o', 'w', ' ', 't', 'h', 'e'
		db ' ', 'c', 'u', 'r', 'r', 'e', 'n', 't', ' ', 'h', 'i', 'g', 'h', ' ', 's', 'c'
		db 'o', 'r', 'e', 's', 0, 'E', 'n', 't', 'e', 'r', ' ', 't', 'h', 'e', ' ', 'M'
		db 'u', 's', 'i', 'c', ' ', 'R', 'o', 'o', 'm', 0, 'C', 'h', 'a', 'n', 'g', 'e'
		db ' ', 'g', 'a', 'm', 'e', ' ', 'o', 'p', 't', 'i', 'o', 'n', 's', 0, 'R', 'e'
		db 't', 'u', 'r', 'n', ' ', 't', 'o', ' ', 'D', 'O', 'S', 0, 'S', 'e', 't', ' '
		db 'r', 'a', 'n', 'k', ' ', 't', 'o', ' ', 'E', 'a', 's', 'y', 0, 'S', 'e', 't'
		db ' ', 'r', 'a', 'n', 'k', ' ', 't', 'o', ' ', 'N', 'o', 'r', 'm', 'a', 'l', 0
		db 'S', 'e', 't', ' ', 'r', 'a', 'n', 'k', ' ', 't', 'o', ' ', 'H', 'a', 'r', 'd'
		db 0, 'S', 'e', 't', ' ', 'r', 'a', 'n', 'k', ' ', 't', 'o', ' ', 'L', 'u', 'n'
		db 'a', 't', 'i', 'c', 0, 'S', 'e', 't', ' ', 's', 't', 'a', 'r', 't', 'i', 'n'
		db 'g', ' ', 'l', 'i', 'v', 'e', 's', 0, 'S', 'e', 't', ' ', 's', 't', 'a', 'r'
		db 't', 'i', 'n', 'g', ' ', 'b', 'o', 'm', 'b', 's', 0, 'D', 'i', 's', 'a', 'b'
		db 'l', 'e', ' ', 'm', 'u', 's', 'i', 'c', 0, 'U', 's', 'e', ' ', 'F', 'M', '-'
		db '2', '6', ' ', 'm', 'u', 's', 'i', 'c', 0, 'U', 's', 'e', ' ', 'F', 'M', '-'
		db '8', '6', ' ', 'm', 'u', 's', 'i', 'c', 0, 'D', 'i', 's', 'a', 'b', 'l', 'e'
		db ' ', 's', 'o', 'u', 'n', 'd', ' ', 'e', 'f', 'f', 'e', 'c', 't', 's', 0, 'U'
		db 's', 'e', ' ', 'F', 'M', ' ', 's', 'o', 'u', 'n', 'd', ' ', 'e', 'f', 'f', 'e'
		db 'c', 't', 's', 0, 'U', 's', 'e', ' ', 'P', 'C', '-', '9', '8', ' ', 'b', 'e'
		db 'e', 'p', 's', 0, 'S', 'l', 'o', 'w', ' ', 'd', 'o', 'w', 'n', ' ', 'i', 'n'
		db ' ', 'd', 'e', 'n', 's', 'e', ' ', 'p', 'a', 't', 't', 'e', 'r', 'n', 's', 0
		db 'R', 'u', 'n', ' ', 'a', 't', ' ', 'n', 'o', 'r', 'm', 'a', 'l', ' ', 's', 'p'
		db 'e', 'e', 'd', 0, 'R', 'e', 's', 't', 'o', 'r', 'e', ' ', 'd', 'e', 'f', 'a'
		db 'u', 'l', 't', ' ', 'o', 'p', 't', 'i', 'o', 'n', 's', 0, 'L', 'e', 'a', 'v'
		db 'e', ' ', 't', 'h', 'i', 's', ' ', 'm', 'e', 'n', 'u', 0, 'S', 't', 'a', 'r'
		db 't', ' ', 't', 'h', 'e', ' ', 'g', 'a', 'm', 'e', ' ', 'o', 'n', ' ', 'E', 'a'
		db 's', 'y', 0, 'S', 't', 'a', 'r', 't', ' ', 't', 'h', 'e', ' ', 'g', 'a', 'm'
		db 'e', ' ', 'o', 'n', ' ', 'N', 'o', 'r', 'm', 'a', 'l', 0, 'S', 't', 'a', 'r'
		db 't', ' ', 't', 'h', 'e', ' ', 'g', 'a', 'm', 'e', ' ', 'o', 'n', ' ', 'H', 'a'
		db 'r', 'd', 0, 'S', 't', 'a', 'r', 't', ' ', 't', 'h', 'e', ' ', 'g', 'a', 'm'
		db 'e', ' ', 'o', 'n', ' ', 'L', 'u', 'n', 'a', 't', 'i', 'c', 0, 'R', 'a', 'n'
		db 'k', 0, 'L', 'i', 'v', 'e', 's', 0, 'B', 'o', 'm', 'b', 's', 0, 'M', 'u'
		db 's', 'i', 'c', 0, 'S', 'F', 'X', 0, 'T', 'u', 'r', 'b', 'o', 0, 'S', 'l'
		db 'o', 'w', 0, 'R', 'e', 's', 'e', 't', 0, 'Q', 'u', 'i', 't', 0, 'E', 'a'
		db 's', 'y', 0, 'N', 'o', 'r', 'm', 'a', 'l', 0, 'H', 'a', 'r', 'd', 0, 'L'
		db 'u', 'n', 'a', 't', 'i', 'c', 0, '0', 0, '1', 0, '2', 0, '3', 0, '4'
		db 0, '5', 0, '6', 0, 'O', 'f', 'f', 0, 'F', 'M', '-', '2', '6', 0, 'F'
		db 'M', '-', '8', '6', 0, 'O', 'f', 'f', 0, 'F', 'M', 0, 'B', 'e', 'e', 'p'
		db 0, 'R', 'e', 'i', 'm', 'u', ' ', 'H', 'a', 'k', 'u', 'r', 'e', 'i', 0, 'W'
		db 'i', 'd', 'e', ' ', 'A', 't', 't', 'a', 'c', 'k', ' ', 'T', 'y', 'p', 'e', 0
		db 'M', 'a', 'r', 'i', 's', 'a', ' ', 'K', 'i', 'r', 'i', 's', 'a', 'm', 'e', 0
		db 'P', 'o', 'w', 'e', 'r', ' ', 'A', 't', 't', 'a', 'c', 'k', ' ', 'T', 'y', 'p'
		db 'e', 0, 'S', 'e', 'a', 'r', 'c', 'h', ' ', 'S', 'h', 'o', 't', 0, 'W', 'i'
		db 'd', 'e', ' ', 'S', 'h', 'o', 't', 0, 'I', 'm', 'a', 'g', 'e', ' ', 'L', 'a'
		db 's', 'e', 'r', 0, 'R', 'a', 'p', 'i', 'd', ' ', 'S', 'h', 'o', 't', 0, 'C'
		db 'h', 'o', 'o', 's', 'e', ' ', 'Y', 'o', 'u', 'r', ' ', 'S', 'h', 'o', 't', 0
		db 'C', 'l', 'e', 'a', 'r', 'e', 'd', 0, 'C', 'o', 'n', 'f', 'i', 'g', 'u', 'r'
		db 'e', ' ', 'a', ' ', 'p', 'r', 'a', 'c', 't', 'i', 'c', 'e', ' ', 'r', 'u', 'n'
		db 0, 'W', 'a', 't', 'c', 'h', ' ', 'a', ' ', 'r', 'e', 'c', 'o', 'r', 'd', 'e'
		db 'd', ' ', 'r', 'u', 'n', 0, 'N', 'o', '.', '1', ' ', 'G', 'e', 'n', 's', 'o'
		db 'k', 'y', 'o', 0, 'N', 'o', '.', '2', ' ', 'W', 'i', 't', 'c', 'h', 'i', 'n'
		db 'g', ' ', 'D', 'r', 'e', 'a', 'm', 0, 'N', 'o', '.', '3', ' ', 'S', 'e', 'l'
		db 'e', 'n', 'e', 0x27, 's', ' ', 'L', 'i', 'g', 'h', 't', 0, 'N', 'o', '.', '4'
		db ' ', 'D', 'e', 'c', 'o', 'r', 'a', 't', 'i', 'o', 'n', ' ', 'B', 'a', 't', 't'
		db 'l', 'e', 0, 'N', 'o', '.', '5', ' ', 'B', 'r', 'e', 'a', 'k', ' ', 't', 'h'
		db 'e', ' ', 'S', 'a', 'b', 'b', 'a', 't', 'h', 0, 'N', 'o', '.', '6', ' ', 'S'
		db 'c', 'a', 'r', 'l', 'e', 't', ' ', 'S', 'y', 'm', 'p', 'h', 'o', 'n', 'y', 0
		db 'N', 'o', '.', '7', ' ', 'B', 'a', 'd', ' ', 'A', 'p', 'p', 'l', 'e', '!', '!'
		db 0, 'N', 'o', '.', '8', ' ', 'S', 'p', 'i', 'r', 'i', 't', ' ', 'B', 'a', 't'
		db 't', 'l', 'e', 0, 'N', 'o', '.', '9', ' ', 'A', 'l', 'i', 'c', 'e', ' ', 'M'
		db 'a', 'e', 's', 't', 'r', 'a', 0, 'N', 'o', '.', '1', '0', ' ', 'M', 'a', 'i'
		db 'd', 'e', 'n', 0x27, 's', ' ', 'C', 'a', 'p', 'r', 'i', 'c', 'c', 'i', 'o', 0
		db 'N', 'o', '.', '1', '1', ' ', 'V', 'e', 's', 's', 'e', 'l', ' ', 'o', 'f', ' '
		db 'S', 't', 'a', 'r', 's', 0, 'N', 'o', '.', '1', '2', ' ', 'L', 'o', 't', 'u'
		db 's', ' ', 'L', 'o', 'v', 'e', 0, 'N', 'o', '.', '1', '3', ' ', 'S', 'l', 'e'
		db 'e', 'p', 'i', 'n', 'g', ' ', 'T', 'e', 'r', 'r', 'o', 'r', 0, 'N', 'o', '.'
		db '1', '4', ' ', 'D', 'r', 'e', 'a', 'm', ' ', 'L', 'a', 'n', 'd', 0, 'N', 'o'
		db '.', '1', '5', ' ', 'F', 'a', 'i', 'n', 't', ' ', 'D', 'r', 'e', 'a', 'm', 0
		db 'N', 'o', '.', '1', '6', ' ', 'I', 'n', 'e', 'v', 'i', 't', 'a', 'b', 'l', 'y'
		db ' ', 'F', 'o', 'r', 'b', 'i', 'd', 'd', 'e', 'n', ' ', 'G', 'a', 'm', 'e', 0
		db 'N', 'o', '.', '1', '7', ' ', 'I', 'l', 'l', 'u', 's', 'i', 'o', 'n', ' ', 'o'
		db 'f', ' ', 'M', 'a', 'i', 'd', 0, 'N', 'o', '.', '1', '8', ' ', 'C', 'u', 't'
		db 'e', ' ', 'D', 'e', 'v', 'i', 'l', 0, 'N', 'o', '.', '1', '9', ' ', 'D', 'a'
		db 'y', 's', 0, 'N', 'o', '.', '2', '0', ' ', 'P', 'e', 'a', 'c', 'e', 'f', 'u'
		db 'l', 0, 'N', 'o', '.', '2', '1', ' ', 'A', 'r', 'c', 'a', 'd', 'i', 'a', 'n'
		db ' ', 'D', 'r', 'e', 'a', 'm', 0, 'N', 'o', '.', '2', '2', ' ', 'T', 'h', 'o'
		db 's', 'e', ' ', 'W', 'h', 'o', ' ', 'L', 'i', 'v', 'e', ' ', 'i', 'n', ' ', 'I'
		db 'l', 'l', 'u', 's', 'i', 'o', 'n', 's', 0, 'N', 'o', '.', '1', ' ', 'G', 'h'
		db 'o', 's', 't', 'l', 'y', ' ', 'a', 'n', 'd', ' ', 'B', 'e', 'a', 'u', 't', 'i'
		db 'f', 'u', 'l', ' ', 'T', 'a', 'l', 'e', 's', '.', '.', '.', 0, 'N', 'o', '.'
		db '2', ' ', 'D', 'r', 'e', 'a', 'm', ' ', 'E', 'x', 'p', 'r', 'e', 's', 's', 0
		db 'N', 'o', '.', '3', ' ', 'M', 'a', 'g', 'i', 'c', ' ', 'S', 'q', 'u', 'a', 'r'
		db 'e', 0, 'N', 'o', '.', '4', ' ', 'D', 'i', 'm', 'e', 'n', 's', 'i', 'o', 'n'
		db ' ', 'o', 'f', ' ', 'R', 'e', 'v', 'e', 'r', 'i', 'e', 0, 'N', 'o', '.', '5'
		db ' ', 'S', 'p', 'i', 'r', 'i', 't', 'u', 'a', 'l', ' ', 'H', 'e', 'a', 'v', 'e'
		db 'n', 0, 'N', 'o', '.', '6', ' ', 'R', 'o', 'm', 'a', 'n', 't', 'i', 'c', ' '
		db 'C', 'h', 'i', 'l', 'd', 'r', 'e', 'n', 0, 'N', 'o', '.', '7', ' ', 'P', 'l'
		db 'a', 's', 't', 'i', 'c', ' ', 'M', 'i', 'n', 'd', 0, 'N', 'o', '.', '8', ' '
		db 'M', 'a', 'p', 'l', 'e', ' ', 'W', 'i', 's', 'e', 0, 'N', 'o', '.', '9', ' '
		db 'F', 'o', 'r', 'b', 'i', 'd', 'd', 'e', 'n', ' ', 'M', 'a', 'g', 'i', 'c', 0
		db 'N', 'o', '.', '1', '0', ' ', 'C', 'r', 'i', 'm', 's', 'o', 'n', ' ', 'M', 'a'
		db 'i', 'd', 'e', 'n', 0, 'N', 'o', '.', '1', '1', ' ', 'T', 'r', 'e', 'a', 'c'
		db 'h', 'e', 'r', 'o', 'u', 's', ' ', 'M', 'a', 'i', 'd', 'e', 'n', 0, 'N', 'o'
		db '.', '1', '2', ' ', 'T', 'h', 'e', ' ', 'L', 'a', 's', 't', ' ', 'J', 'u', 'd'
		db 'g', 'e', 'm', 'e', 'n', 't', 0, 'N', 'o', '.', '1', '3', ' ', 'D', 'o', 'l'
		db 'l', ' ', 'o', 'f', ' ', 'M', 'i', 's', 'e', 'r', 'y', 0, 'N', 'o', '.', '1'
		db '4', ' ', 'W', 'o', 'r', 'l', 'd', 0x27, 's', ' ', 'E', 'n', 'd', 0, 'N', 'o'
		db '.', '1', '5', ' ', 'L', 'e', 'g', 'e', 'n', 'd', 'a', 'r', 'y', ' ', 'I', 'l'
		db 'l', 'u', 's', 'i', 'o', 'n', 0, 'N', 'o', '.', '1', '6', ' ', 'A', 'l', 'i'
		db 'c', 'e', ' ', 'i', 'n', ' ', 'W', 'o', 'n', 'd', 'e', 'r', 'l', 'a', 'n', 'd'
		db 0, 'N', 'o', '.', '1', '7', ' ', 'T', 'h', 'e', ' ', 'G', 'r', 'i', 'm', 'o'
		db 'i', 'r', 'e', ' ', 'o', 'f', ' ', 'A', 'l', 'i', 'c', 'e', 0, 'N', 'o', '.'
		db '1', '8', ' ', 'S', 'h', 'i', 'n', 't', 'o', ' ', 'S', 'h', 'r', 'i', 'n', 'e'
		db 0, 'N', 'o', '.', '1', '9', ' ', 'E', 'n', 'd', 'l', 'e', 's', 's', 0, 'N'
		db 'o', '.', '2', '0', ' ', 'E', 't', 'e', 'r', 'n', 'a', 'l', ' ', 'P', 'a', 'r'
		db 'a', 'd', 'i', 's', 'e', 0, 'N', 'o', '.', '2', '1', ' ', 'M', 'y', 's', 't'
		db 'i', 'c', ' ', 'D', 'r', 'e', 'a', 'm', 0, 'N', 'o', '.', '2', '2', ' ', 'P'
		db 'e', 'a', 'c', 'e', 'f', 'u', 'l', ' ', 'R', 'o', 'm', 'a', 'n', 'c', 'e', 'r'
		db 0, 'N', 'o', '.', '2', '3', ' ', 'S', 'o', 'u', 'l', 0x27, 's', ' ', 'R', 'e'
		db 's', 't', 'i', 'n', 'g', ' ', 'P', 'l', 'a', 'c', 'e', 0
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

bool16 language_op_english_selected(void)
{
	return (language_preference_get() == LANGUAGE_ENGLISH);
}

const char *language_op_main_label(int choice)
{
	if((choice < 0) || (choice >= 8)) {
		return 0;
	}
	return language_op_text(LOT_MAIN_START + choice);
}

const char *language_op_main_desc(int desc_id)
{
	if((desc_id < 1) || (desc_id > 25)) {
		return 0;
	}
	return language_op_text(LOT_DESC_1 + desc_id - 1);
}

const char *language_op_custom_desc(bool practice)
{
	return language_op_text(practice ? LOT_PRACTICE_DESC : LOT_REPLAY_DESC);
}

const char *language_op_music_choice(
	uint8_t game, uint8_t track, const char *stock_choice
)
{
	if(!language_op_english_selected()) {
		return stock_choice;
	}
	if((game == 3) && (track < 22)) {
		return language_op_text(LOT_TH04_MUSIC_1 + track);
	}
	if((game == 4) && (track < 23)) {
		return language_op_text(LOT_TH05_MUSIC_1 + track);
	}
	return stock_choice;
}

const char *language_op_option_label(int choice)
{
	switch(choice) {
	case LOC_RANK:          return language_op_text(LOT_LABEL_RANK);
	case LOC_LIVES:         return language_op_text(LOT_LABEL_LIVES);
	case LOC_BOMBS:         return language_op_text(LOT_LABEL_BOMBS);
	case LOC_BGM:           return language_op_text(LOT_LABEL_MUSIC);
	case LOC_SE:            return language_op_text(LOT_LABEL_SFX);
	case LOC_TURBO_OR_SLOW:
		return language_op_text(
			(GAME == 4) ? LOT_LABEL_TURBO : LOT_LABEL_SLOW
		);
	case LOC_RESET:         return language_op_text(LOT_LABEL_RESET);
	case LOC_QUIT:          return language_op_text(LOT_LABEL_QUIT);
	}
	return 0;
}

const char *language_op_option_value(int choice, int value)
{
	if(choice == LOC_RANK) {
		if((value >= 0) && (value < 4)) {
			return language_op_text(LOT_RANK_EASY + value);
		}
	} else if((choice == LOC_LIVES) || (choice == LOC_BOMBS)) {
		if((value >= 0) && (value < 7)) {
			return language_op_text(LOT_NUMBER_0 + value);
		}
	} else if(choice == LOC_BGM) {
		if((value >= 0) && (value < 3)) {
			return language_op_text(LOT_BGM_OFF + value);
		}
	} else if(choice == LOC_SE) {
		if((value >= 0) && (value < 3)) {
			return language_op_text(LOT_SE_OFF + value);
		}
	}
	return 0;
}

#if (GAME == 4)
extern const shiftjis_t *PLAYCHAR_TITLE[2][2];
extern const shiftjis_t *SHOTTYPE_TITLE[2][2];
static const shiftjis_t *language_playchar_title_stock[2][2];
static const shiftjis_t *language_shottype_title_stock[2][2];
static bool language_character_titles_captured;

void language_op_character_prepare(void)
{
	uint8_t playchar;
	uint8_t line;

	if(!language_character_titles_captured) {
		for(playchar = 0; playchar < 2; playchar++) {
			for(line = 0; line < 2; line++) {
				language_playchar_title_stock[playchar][line] =
					PLAYCHAR_TITLE[playchar][line];
				language_shottype_title_stock[playchar][line] =
					SHOTTYPE_TITLE[playchar][line];
			}
		}
		language_character_titles_captured = true;
	}
	for(playchar = 0; playchar < 2; playchar++) {
		for(line = 0; line < 2; line++) {
			if(language_op_english_selected()) {
				PLAYCHAR_TITLE[playchar][line] =
					reinterpret_cast<const shiftjis_t *>(language_op_text(
						LOT_REIMU + (playchar * 2) + line
					));
				SHOTTYPE_TITLE[playchar][line] =
					reinterpret_cast<const shiftjis_t *>(language_op_text(
						LOT_SEARCH_SHOT + (playchar * 2) + line
					));
			} else {
				PLAYCHAR_TITLE[playchar][line] =
					language_playchar_title_stock[playchar][line];
				SHOTTYPE_TITLE[playchar][line] =
					language_shottype_title_stock[playchar][line];
			}
		}
	}
}

#endif

static void language_config_name_set(void)
{
	language_config_fn[0] = 'T';
	language_config_fn[1] = ('0' + GAME);
	language_config_fn[2] = 'L';
	language_config_fn[3] = 'A';
	language_config_fn[4] = 'N';
	language_config_fn[5] = 'G';
	language_config_fn[6] = '.';
	language_config_fn[7] = 'C';
	language_config_fn[8] = 'F';
	language_config_fn[9] = 'G';
	language_config_fn[10] = '\0';
}

static void language_text_set(void)
{
	language_menu_bgm_fn[0] = 'o';
	language_menu_bgm_fn[1] = 'p';
	language_menu_bgm_fn[2] = '\0';
	language_se_fn[0] = 'm';
	language_se_fn[1] = 'i';
	language_se_fn[2] = 'k';
	language_se_fn[3] = 'o';
	language_se_fn[4] = '\0';
}

static uint8_t language_config_checksum(const uint8_t *data)
{
	uint8_t sum = 0;
	int i;

	for(i = 0; i < 6; i++) {
		sum += data[i];
	}
	return sum;
}

static void language_config_load(void)
{
	uint8_t data[LANGUAGE_CONFIG_SIZE];
	uint8_t extra;
	uint8_t sum;
	int fh;

	if(language_loaded) {
		return;
	}
	language_loaded = true;
	language_current = LANGUAGE_JAPANESE;
	language_config_name_set();
	fh = replay_op_dos_open(language_config_fn);
	if(fh < 0) {
		return;
	}
	if(
		(replay_op_dos_read(fh, data, LANGUAGE_CONFIG_SIZE) != LANGUAGE_CONFIG_SIZE) ||
		(replay_op_dos_read(fh, &extra, 1) != 0)
	) {
		replay_op_dos_close(fh);
		return;
	}
	replay_op_dos_close(fh);
	sum = language_config_checksum(data);
	if(
		(data[0] != 'T') || (data[1] != ('0' + GAME)) ||
		(data[2] != 'L') || (data[3] != 'G') ||
		(data[4] != LANGUAGE_CONFIG_VERSION) ||
		(data[5] > LANGUAGE_ENGLISH) ||
		(data[6] != sum) || (data[7] != static_cast<uint8_t>(~sum))
	) {
		return;
	}
	language_current = static_cast<language_preference_t>(data[5]);
}

language_preference_t language_preference_get(void)
{
	language_config_load();
	return language_current;
}

bool16 language_preference_set(language_preference_t preference)
{
	uint8_t data[LANGUAGE_CONFIG_SIZE];
	uint8_t sum;
	language_preference_t previous;
	int fh;

	language_config_load();
	if(preference > LANGUAGE_ENGLISH) {
		return false;
	}
	if(preference == language_current) {
		return true;
	}
	previous = language_current;
	language_current = preference;
	language_config_name_set();
	data[0] = 'T';
	data[1] = ('0' + GAME);
	data[2] = 'L';
	data[3] = 'G';
	data[4] = LANGUAGE_CONFIG_VERSION;
	data[5] = language_current;
	sum = language_config_checksum(data);
	data[6] = sum;
	data[7] = static_cast<uint8_t>(~sum);
	fh = replay_op_dos_create(language_config_fn);
	if(fh < 0) {
		language_current = previous;
		return false;
	}
	if(replay_op_dos_write(fh, data, LANGUAGE_CONFIG_SIZE) != LANGUAGE_CONFIG_SIZE) {
		replay_op_dos_close(fh);
		language_current = previous;
		return false;
	}
	replay_op_dos_close(fh);
	replay_op_dos_flush();
	return true;
}

static screen_y_t language_option_choice_top(language_option_choice_t sel)
{
	if(sel == LOC_LANGUAGE) {
		return LANGUAGE_OPTION_TOP;
	}
	sel = static_cast<language_option_choice_t>(sel - 1);
	return ((sel >= 7)
		? (LANGUAGE_OPTION_STOCK_TOP + (6 * LANGUAGE_OPTION_LABEL_H) +
			((sel - 6) * LANGUAGE_OPTION_COMMAND_H))
		: (LANGUAGE_OPTION_STOCK_TOP + (sel * LANGUAGE_OPTION_LABEL_H))
	);
}

static void language_option_desc_put(int desc_id)
{
	const char *desc = language_op_main_desc(desc_id);

	egc_copy_rect_1_to_0_16(0, LANGUAGE_OPTION_DESC_TOP, RES_X, GLYPH_H);
	if(language_op_english_selected() && desc) {
		replay_op_font_put_right(
			(RES_X - GLYPH_HALF_W), LANGUAGE_OPTION_DESC_TOP,
			desc, ((GAME == 5) ? 9 : V_WHITE)
		);
		return;
	}
	graph_putsa_fx_func = FX_WEIGHT_BOLD;
	graph_putsa_fx(
		(RES_X - GLYPH_FULL_W - (strlen(MENU_DESC[desc_id]) * GLYPH_HALF_W)),
		LANGUAGE_OPTION_DESC_TOP, ((GAME == 5) ? 9 : V_WHITE),
		MENU_DESC[desc_id]
	);
}

static void language_option_language_desc_put(void)
{
	char desc[24];
	char *p = desc;

	#define P(c) *p++ = c
	if(language_preference_get() == LANGUAGE_ENGLISH) {
		P('C'); P('h'); P('a'); P('n'); P('g'); P('e'); P(' ');
		P('d'); P('i'); P('s'); P('p'); P('l'); P('a'); P('y'); P(' ');
		P('l'); P('a'); P('n'); P('g'); P('u'); P('a'); P('g'); P('e');
	} else {
		// 表示言語を変更します
		P(0x95); P(0x5C); P(0x8E); P(0xA6); P(0x8C); P(0xBE);
		P(0x8C); P(0xEA); P(0x82); P(0xF0); P(0x95); P(0xCF);
		P(0x8D); P(0x58); P(0x82); P(0xB5); P(0x82); P(0xDC);
		P(0x82); P(0xB7);
	}
	#undef P
	*p = '\0';
	graph_putsa_fx_func = FX_WEIGHT_BOLD;
	graph_putsa_fx(
		(RES_X - GLYPH_FULL_W - (strlen(desc) * GLYPH_HALF_W)),
		LANGUAGE_OPTION_DESC_TOP, ((GAME == 5) ? 9 : V_WHITE), desc
	);
}

static void language_option_language_put(vc2 color)
{
	op_cdg_slot_t value;

	value = ((language_preference_get() == LANGUAGE_ENGLISH)
		? CDG_OPTION_VALUE_ENGLISH : CDG_OPTION_VALUE_JAPANESE
	);
	egc_copy_rect_1_to_0_16(
		LANGUAGE_OPTION_LEFT, LANGUAGE_OPTION_TOP, LANGUAGE_OPTION_W, 16
	);
	grcg_setcolor(GC_RMW, color);
	cdg_put_nocolors_8(
		LANGUAGE_OPTION_LEFT, LANGUAGE_OPTION_TOP, CDG_OPTION_LABEL_LANGUAGE
	);
	cdg_put_nocolors_8(LANGUAGE_OPTION_VALUE_LEFT, LANGUAGE_OPTION_TOP, value);
	grcg_off();
	if(color == LANGUAGE_OPTION_COL_ACTIVE) {
		cdg_put_8(LANGUAGE_OPTION_LEFT, LANGUAGE_OPTION_TOP, CDG_CURSOR_LEFT);
		cdg_put_8(
			LANGUAGE_OPTION_CURSOR_RIGHT, LANGUAGE_OPTION_TOP, CDG_CURSOR_RIGHT
		);
		egc_copy_rect_1_to_0_16(
			0, LANGUAGE_OPTION_DESC_TOP, RES_X, GLYPH_H
		);
		language_option_language_desc_put();
	}
}

static void language_option_stock_put(language_option_choice_t sel, vc2 color)
{
	screen_y_t top = language_option_choice_top(sel);
	screen_x_t cursor_left = LANGUAGE_OPTION_LEFT;
	int cdg_value;
	int desc_id;

	egc_copy_rect_1_to_0_16(
		LANGUAGE_OPTION_LEFT, top, LANGUAGE_OPTION_W, LANGUAGE_OPTION_LABEL_H
	);
	grcg_setcolor(GC_RMW, color);
	switch(sel) {
	case LOC_RANK:
		cdg_put_nocolors_8(LANGUAGE_OPTION_LEFT, top, CDG_OPTION_LABEL_RANK);
		cdg_put_nocolors_8(
			LANGUAGE_OPTION_VALUE_LEFT, top,
			(CDG_OPTION_VALUE_RANK + resident->rank)
		);
		desc_id = (6 + resident->rank);
		break;
	case LOC_LIVES:
		cdg_put_nocolors_8(LANGUAGE_OPTION_LEFT, top, CDG_OPTION_LABEL_LIVES);
		cdg_put_nocolors_8(
			LANGUAGE_OPTION_VALUE_LEFT, top,
			(CDG_NUMERAL + resident->cfg_lives)
		);
		desc_id = 10;
		break;
	case LOC_BOMBS:
		cdg_put_nocolors_8(LANGUAGE_OPTION_LEFT, top, CDG_OPTION_LABEL_BOMBS);
		cdg_put_nocolors_8(
			LANGUAGE_OPTION_VALUE_LEFT, top,
			(CDG_NUMERAL + resident->cfg_bombs)
		);
		desc_id = 11;
		break;
	case LOC_BGM:
		cdg_put_nocolors_8(LANGUAGE_OPTION_LEFT, top, CDG_OPTION_LABEL_BGM);
		cdg_value = ((resident->bgm_mode == SND_BGM_OFF)
			? CDG_OPTION_VALUE_OFF
			: (CDG_OPTION_VALUE_BGM - SND_BGM_FM26 + resident->bgm_mode)
		);
		cdg_put_nocolors_8(LANGUAGE_OPTION_VALUE_LEFT, top, cdg_value);
		desc_id = (12 + resident->bgm_mode);
		break;
	case LOC_SE:
		cdg_put_nocolors_8(LANGUAGE_OPTION_LEFT, top, CDG_OPTION_LABEL_SE);
		cdg_value = ((resident->se_mode == SND_SE_OFF)
			? CDG_OPTION_VALUE_OFF
			: (CDG_OPTION_VALUE_SE_FM + SND_SE_FM - resident->se_mode)
		);
		cdg_put_nocolors_8(LANGUAGE_OPTION_VALUE_LEFT, top, cdg_value);
		desc_id = (15 + resident->se_mode);
		break;
	case LOC_TURBO_OR_SLOW:
		cdg_put_nocolors_8(
			LANGUAGE_OPTION_COMMAND_LEFT, top,
			(CDG_OPTION_SLOW - resident->turbo_mode)
		);
		cursor_left = LANGUAGE_OPTION_COMMAND_CURSOR_LEFT;
		desc_id = (18 + resident->turbo_mode);
		break;
	case LOC_RESET:
		cdg_put_nocolors_8(LANGUAGE_OPTION_COMMAND_LEFT, top, CDG_OPTION_RESET);
		cursor_left = LANGUAGE_OPTION_COMMAND_CURSOR_LEFT;
		desc_id = 20;
		break;
	default:
		cdg_put_nocolors_8(LANGUAGE_OPTION_COMMAND_LEFT, top, CDG_QUIT);
		cursor_left = LANGUAGE_OPTION_COMMAND_CURSOR_LEFT;
		desc_id = 21;
		break;
	}
	grcg_off();
	if(color == LANGUAGE_OPTION_COL_ACTIVE) {
		cdg_put_8(cursor_left, top, CDG_CURSOR_LEFT);
		cdg_put_8(
			((cursor_left == LANGUAGE_OPTION_COMMAND_CURSOR_LEFT)
				? LANGUAGE_OPTION_COMMAND_CURSOR_RIGHT
				: LANGUAGE_OPTION_CURSOR_RIGHT),
			top, CDG_CURSOR_RIGHT
		);
		language_option_desc_put(desc_id);
	}
}

static void language_option_put(language_option_choice_t sel, vc2 color)
{
	if(sel == LOC_LANGUAGE) {
		language_option_language_put(color);
	} else {
		language_option_stock_put(sel, color);
	}
}

static void language_option_audio_restart(bool also_reload_se)
{
	snd_kaja_func(KAJA_SONG_STOP, 0);
#if (GAME == 5)
	if(also_reload_se) {
		snd_determine_modes(resident->bgm_mode, resident->se_mode);
		snd_load(language_se_fn, SND_LOAD_SE);
	} else {
		snd_determine_modes(resident->bgm_mode, resident->se_mode);
	}
#else
	snd_determine_modes(resident->bgm_mode, resident->se_mode);
#endif
	snd_load(language_menu_bgm_fn, SND_LOAD_SONG);
	snd_kaja_func(KAJA_SONG_PLAY, 0);
}

static void language_option_change(bool increment)
{
	if(language_option_sel == LOC_LANGUAGE) {
		language_preference_set(
			(language_preference_get() == LANGUAGE_JAPANESE)
				? LANGUAGE_ENGLISH : LANGUAGE_JAPANESE
		);
		return;
	}
	switch(language_option_sel) {
	case LOC_RANK:
		if(increment) {
			if(resident->rank == RANK_LUNATIC) {
				resident->rank = RANK_EASY;
			} else {
				resident->rank++;
			}
		} else if(resident->rank == RANK_EASY) {
			resident->rank = RANK_LUNATIC;
		} else {
			resident->rank--;
		}
		break;
	case LOC_LIVES:
		if(increment) {
			if(resident->cfg_lives == CFG_LIVES_MAX) {
				resident->cfg_lives = 1;
			} else {
				resident->cfg_lives++;
			}
		} else if(resident->cfg_lives == 1) {
			resident->cfg_lives = CFG_LIVES_MAX;
		} else {
			resident->cfg_lives--;
		}
		break;
	case LOC_BOMBS:
		if(increment) {
			if(resident->cfg_bombs == CFG_BOMBS_MAX) {
				resident->cfg_bombs = 0;
			} else {
				resident->cfg_bombs++;
			}
		} else if(resident->cfg_bombs == 0) {
			resident->cfg_bombs = CFG_BOMBS_MAX;
		} else {
			resident->cfg_bombs--;
		}
		break;
	case LOC_BGM:
		if(increment) {
			if(resident->bgm_mode == SND_BGM_FM86) {
				resident->bgm_mode = SND_BGM_OFF;
			} else {
				resident->bgm_mode++;
			}
		} else if(resident->bgm_mode == SND_BGM_OFF) {
			resident->bgm_mode = SND_BGM_FM86;
		} else {
			resident->bgm_mode--;
		}
		language_option_audio_restart(false);
		break;
	case LOC_SE:
		if(increment) {
			if(resident->se_mode == SND_SE_OFF) {
				resident->se_mode = SND_SE_BEEP;
			} else {
				resident->se_mode--;
			}
		} else if(resident->se_mode == SND_SE_BEEP) {
			resident->se_mode = SND_SE_OFF;
		} else {
			resident->se_mode++;
		}
#if (GAME == 5)
		snd_determine_modes(resident->bgm_mode, resident->se_mode);
		snd_load(language_se_fn, SND_LOAD_SE);
#endif
		break;
	case LOC_TURBO_OR_SLOW:
		resident->turbo_mode = (1 - resident->turbo_mode);
		break;
	}
}

static void language_option_selection_move(int8_t direction)
{
	language_option_put(
		static_cast<language_option_choice_t>(language_option_sel),
		LANGUAGE_OPTION_COL_INACTIVE
	);
	language_option_sel += direction;
	if(language_option_sel < 0) {
		language_option_sel = (LOC_COUNT - 1);
	}
	if(language_option_sel >= LOC_COUNT) {
		language_option_sel = 0;
	}
	language_option_put(
		static_cast<language_option_choice_t>(language_option_sel),
		LANGUAGE_OPTION_COL_ACTIVE
	);
	snd_se_play_force(1);
}

static void language_option_changed_put(void)
{
	language_option_put(
		static_cast<language_option_choice_t>(language_option_sel),
		LANGUAGE_OPTION_COL_ACTIVE
	);
}

static void language_option_return_to_main(void)
{
	if(language_preference_get() != language_option_entry_preference) {
		language_asset_music_prepare();
		#if (GAME == 5)
			replay_op_bridge(ROBF_MAIN_CDG_LOAD);
			replay_main_title_labels_load();
		#else
			language_op_character_prepare();
		#endif
		replay_main_language_assets_reload();
	}
	language_option_initialized = false;
	menu_sel = 6; // RMC_OPTION in replay.cpp
	in_option = false;
}

void far language_option_update_and_render(void)
{
	int i;

	if(!language_option_initialized) {
		language_option_sel = LOC_LANGUAGE;
		language_option_entry_preference = language_preference_get();
		language_option_input_allowed = false;
		egc_copy_rect_1_to_0_16(
			LANGUAGE_OPTION_LEFT, LANGUAGE_OPTION_TOP,
			LANGUAGE_OPTION_MENU_W, LANGUAGE_OPTION_MENU_H
		);
		for(i = 0; i < LOC_COUNT; i++) {
			language_option_put(
				static_cast<language_option_choice_t>(i),
				((i == language_option_sel)
					? LANGUAGE_OPTION_COL_ACTIVE : LANGUAGE_OPTION_COL_INACTIVE)
			);
		}
		language_option_initialized = true;
		return;
	}
	if(!key_det) {
		language_option_input_allowed = true;
	}
	if(!language_option_input_allowed) {
		return;
	}
	if(key_det & INPUT_UP) {
		language_option_selection_move(-1);
	}
	if(key_det & INPUT_DOWN) {
		language_option_selection_move(+1);
	}
	if((key_det & INPUT_OK) || (key_det & INPUT_SHOT)) {
		if(language_option_sel == LOC_RESET) {
			resident->rank = RANK_NORMAL;
			resident->cfg_lives = CFG_LIVES_DEFAULT;
			resident->cfg_bombs = CFG_BOMBS_DEFAULT;
			resident->bgm_mode = SND_BGM_FM86;
			resident->se_mode = SND_SE_FM;
			resident->turbo_mode = true;
			language_option_audio_restart(true);
			language_option_initialized = false;
		} else if(language_option_sel == LOC_QUIT) {
			snd_se_play_force(11);
			language_option_return_to_main();
		} else {
			language_option_change(true);
			language_option_changed_put();
		}
	}
	if((key_det & INPUT_RIGHT) && (language_option_sel <= LOC_TURBO_OR_SLOW)) {
		language_option_change(true);
		language_option_changed_put();
	}
	if((key_det & INPUT_LEFT) && (language_option_sel <= LOC_TURBO_OR_SLOW)) {
		language_option_change(false);
		language_option_changed_put();
	}
	if(key_det & INPUT_CANCEL) {
		language_option_return_to_main();
	}
	if(key_det) {
		language_option_input_allowed = false;
	}
}

// Keep each game's following CRT segment on its stock paragraph phase.
#if (GAME == 4)
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#else
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#endif
