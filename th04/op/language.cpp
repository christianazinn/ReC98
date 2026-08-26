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
#define LANGUAGE_OPTION_TOP ((GAME == 5) ? 230 : 204)
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
static char language_label[9];
static char language_japanese[9];
static char language_english[8];
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
	language_label[0] = 'L'; language_label[1] = 'a';
	language_label[2] = 'n'; language_label[3] = 'g';
	language_label[4] = 'u'; language_label[5] = 'a';
	language_label[6] = 'g'; language_label[7] = 'e';
	language_label[8] = '\0';
	language_japanese[0] = 'J'; language_japanese[1] = 'a';
	language_japanese[2] = 'p'; language_japanese[3] = 'a';
	language_japanese[4] = 'n'; language_japanese[5] = 'e';
	language_japanese[6] = 's'; language_japanese[7] = 'e';
	language_japanese[8] = '\0';
	language_english[0] = 'E'; language_english[1] = 'n';
	language_english[2] = 'g'; language_english[3] = 'l';
	language_english[4] = 'i'; language_english[5] = 's';
	language_english[6] = 'h'; language_english[7] = '\0';
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

	if(language_loaded) {
		return;
	}
	language_loaded = true;
	language_current = LANGUAGE_JAPANESE;
	language_config_name_set();
	if(!file_ropen(language_config_fn)) {
		return;
	}
	if(
		(file_read(data, LANGUAGE_CONFIG_SIZE) != LANGUAGE_CONFIG_SIZE) ||
		(file_read(&extra, 1) != 0)
	) {
		file_close();
		return;
	}
	file_close();
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
	if(!file_create(language_config_fn)) {
		language_current = previous;
		return false;
	}
	if(file_write(data, LANGUAGE_CONFIG_SIZE) != LANGUAGE_CONFIG_SIZE) {
		file_close();
		language_current = previous;
		return false;
	}
	file_close();
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
	egc_copy_rect_1_to_0_16(0, LANGUAGE_OPTION_DESC_TOP, RES_X, GLYPH_H);
	graph_putsa_fx_func = FX_WEIGHT_BOLD;
	graph_putsa_fx(
		(RES_X - GLYPH_FULL_W - (strlen(MENU_DESC[desc_id]) * GLYPH_HALF_W)),
		LANGUAGE_OPTION_DESC_TOP, ((GAME == 5) ? 9 : V_WHITE),
		MENU_DESC[desc_id]
	);
}

static void language_option_language_put(vc2 color)
{
	const char *value;

	language_text_set();
	value = ((language_preference_get() == LANGUAGE_ENGLISH)
		? language_english : language_japanese
	);
	egc_copy_rect_1_to_0_16(
		LANGUAGE_OPTION_LEFT, LANGUAGE_OPTION_TOP, LANGUAGE_OPTION_W, 16
	);
	replay_op_font_put(
		LANGUAGE_OPTION_LEFT, LANGUAGE_OPTION_TOP, language_label, color
	);
		replay_op_font_put(
			LANGUAGE_OPTION_VALUE_LEFT, LANGUAGE_OPTION_TOP, value, color
		);
	if(color == LANGUAGE_OPTION_COL_ACTIVE) {
		cdg_put_8(LANGUAGE_OPTION_LEFT, LANGUAGE_OPTION_TOP, CDG_CURSOR_LEFT);
		cdg_put_8(
			LANGUAGE_OPTION_CURSOR_RIGHT, LANGUAGE_OPTION_TOP, CDG_CURSOR_RIGHT
		);
		egc_copy_rect_1_to_0_16(
			0, LANGUAGE_OPTION_DESC_TOP, RES_X, GLYPH_H
		);
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

static void language_option_return_to_main(void)
{
	if(language_preference_get() != language_option_entry_preference) {
		graph_accesspage(1);
		language_asset_pi_load(0, replay_op_main_bg_fn);
		pi_palette_apply(0);
		pi_put_8(0, 0, 0);
		pi_free(0);
		graph_copy_page(0);
		palette_100();
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
			language_option_put(
				static_cast<language_option_choice_t>(language_option_sel),
				LANGUAGE_OPTION_COL_ACTIVE
			);
		}
	}
	if((key_det & INPUT_RIGHT) && (language_option_sel <= LOC_TURBO_OR_SLOW)) {
		language_option_change(true);
		language_option_put(
			static_cast<language_option_choice_t>(language_option_sel),
			LANGUAGE_OPTION_COL_ACTIVE
		);
	}
	if((key_det & INPUT_LEFT) && (language_option_sel <= LOC_TURBO_OR_SLOW)) {
		language_option_change(false);
		language_option_put(
			static_cast<language_option_choice_t>(language_option_sel),
			LANGUAGE_OPTION_COL_ACTIVE
		);
	}
	if(key_det & INPUT_CANCEL) {
		language_option_return_to_main();
	}
	if(key_det) {
		language_option_input_allowed = false;
	}
}

// Keep TH05's following CRT segment on its stock paragraph phase.
#if (GAME == 5)
	#pragma codestring "\x90\x90"
#endif
