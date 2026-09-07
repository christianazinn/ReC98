// Included only by keycfg_o.cpp, in its patch-owned code segment.
#include "th01/formats/cfg.hpp"
#include "th01/hardware/egc.h"
#include "th01/hardware/grppsafx.h"
#include "th01/language.hpp"
#include "th01/snd/audio.hpp"
#include "th01/replay_op.hpp"

extern cfg_options_t opts;
extern int8_t menu_sel;
extern int8_t menu_id;
extern uint8_t option_choice_max;
extern int8_t input_left;
extern int8_t input_right;
extern bool input_ok;
extern bool input_shot;
extern bool input_cancel;

enum keyconfig_option_row_t {
	KCO_RANK, KCO_MUSIC, KCO_SFX, KCO_PLAYER, KCO_LANGUAGE,
	KCO_KEYS, KCO_MUSIC_TEST, KCO_QUIT, KCO_COUNT
};
static bool keyconfig_option_initialized;
static bool keyconfig_option_return_from_music;
static bool keyconfig_option_horizontal_locked;
static bool keyconfig_option_confirm_locked;
static int8_t keyconfig_option_previous;

static void keyconfig_option_word(char __ss *out, int id)
{
	switch(id) {
	case 0:
		out[0] = 'R'; out[1] = 'A'; out[2] = 'N'; out[3] = 'K'; out[4] = 0;
		break;
	case 1:
		out[0] = 'M'; out[1] = 'U'; out[2] = 'S'; out[3] = 'I'; out[4] = 'C'; out[5] = 0;
		break;
	case 2:
		out[0] = 'S'; out[1] = 'F'; out[2] = 'X'; out[3] = 0;
		break;
	case 3:
		out[0] = 'P'; out[1] = 'L'; out[2] = 'A'; out[3] = 'Y'; out[4] = 'E'; out[5] = 'R'; out[6] = 0;
		break;
	case 4:
		out[0] = 'L'; out[1] = 'A'; out[2] = 'N'; out[3] = 'G'; out[4] = 0;
		break;
	case 5:
		out[0] = 'K'; out[1] = 'E'; out[2] = 'Y'; out[3] = 'C'; out[4] = 'O'; out[5] = 'N'; out[6] = 'F'; out[7] = 'I'; out[8] = 'G'; out[9] = 0;
		break;
	case 6:
		out[0] = 'M'; out[1] = '.'; out[2] = 'T'; out[3] = 'E'; out[4] = 'S'; out[5] = 'T'; out[6] = 0;
		break;
	case 7:
		out[0] = 'Q'; out[1] = 'U'; out[2] = 'I'; out[3] = 'T'; out[4] = 0;
		break;
	case 8:
		out[0] = 'E'; out[1] = 'A'; out[2] = 'S'; out[3] = 'Y'; out[4] = 0;
		break;
	case 9:
		out[0] = 'N'; out[1] = 'O'; out[2] = 'R'; out[3] = 'M'; out[4] = 'A'; out[5] = 'L'; out[6] = 0;
		break;
	case 10:
		out[0] = 'H'; out[1] = 'A'; out[2] = 'R'; out[3] = 'D'; out[4] = 0;
		break;
	case 11:
		out[0] = 'L'; out[1] = 'U'; out[2] = 'N'; out[3] = 'A'; out[4] = 'T'; out[5] = 'I'; out[6] = 'C'; out[7] = 0;
		break;
	case 12:
		out[0] = 'O'; out[1] = 'F'; out[2] = 'F'; out[3] = 0;
		break;
	case 13:
		out[0] = 'O'; out[1] = 'N'; out[2] = 0;
		break;
	case 14:
		out[0] = 'F'; out[1] = 'M'; out[2] = 0;
		break;
	}
}

static void keyconfig_option_label_fullwidth(char __ss *out, const char __ss *word)
{
	while(*word) {
		if(*word == '.') {
			*out++ = 0x81; *out++ = 0x44;
		} else {
			*out++ = 0x82; *out++ = (0x60 + (*word - 'A'));
		}
		word++;
	}
	*out = 0;
}

static int keyconfig_option_player_count(void)
{
	// rem_lives starts at credit_lives_extra + 2 and counts spare lives.
	// PLAYER includes the current life, matching the original 3..7 labels.
	return (opts.credit_lives_extra + 3);
}

static void keyconfig_option_put(int row, int color)
{
	char label[24];
	char native_label[32];
	char value[24];
	int top = (236 + (row * 20));
	keyconfig_option_word(label, row);
	keyconfig_option_label_fullwidth(native_label, label);
	egc_copy_rect_1_to_0_16(208, top, 224, 16);
	if(row == KCO_LANGUAGE) {
		t1replay_op_language_choice_put(228, top, color, FX_WEIGHT_BLACK);
		return;
	}
	if(row >= KCO_KEYS) {
		graph_putsa_fx(316 - (text_extent_fx(FX_WEIGHT_BLACK, native_label) / 2), top,
			(color | FX_WEIGHT_BLACK), native_label);
		return;
	}
	if(row == KCO_RANK) keyconfig_option_word(value, 8 + opts.rank);
	else if(row == KCO_MUSIC) keyconfig_option_word(value, opts.bgm_mode ? 14 : 12);
	else if(row == KCO_SFX) keyconfig_option_word(value, t1_sfx_enabled() ? 13 : 12);
	else {
		value[0] = ('0' + keyconfig_option_player_count()); value[1] = 0;
	}
	graph_putsa_fx(272 - (text_extent_fx(FX_WEIGHT_BLACK, native_label) / 2), top,
		(color | FX_WEIGHT_BLACK), native_label);
	graph_putsa_fx(360 - (text_extent_fx(FX_WEIGHT_BLACK, value) / 2), top,
		(color | FX_WEIGHT_BLACK), value);
}

static void keyconfig_option_render(void)
{
	for(int i = 0; i < KCO_COUNT; i++) {
		keyconfig_option_put(i, (i == menu_sel) ? 15 : 5);
	}
}

void far keyconfig_option_update(void)
{
	if(!keyconfig_option_initialized) {
		menu_sel = keyconfig_option_return_from_music ? KCO_MUSIC_TEST : KCO_RANK;
		keyconfig_option_return_from_music = false;
		option_choice_max = (KCO_COUNT - 1);
		keyconfig_option_horizontal_locked = true;
		keyconfig_option_confirm_locked = true;
		keyconfig_option_previous = menu_sel;
		keyconfig_option_initialized = true;
		egc_copy_rect_1_to_0_16(208, 216, 224, 176);
		keyconfig_option_render();
		return;
	}
	if(keyconfig_option_previous != menu_sel) {
		keyconfig_option_put(keyconfig_option_previous, 5);
		keyconfig_option_put(menu_sel, 15);
		keyconfig_option_previous = menu_sel;
	}
	if(!input_ok && !input_shot) keyconfig_option_confirm_locked = false;
	if(!input_left && !input_right) keyconfig_option_horizontal_locked = false;
	if((input_left || input_right) && !keyconfig_option_horizontal_locked) {
		int direction = input_right ? 1 : -1;
		if(menu_sel == KCO_RANK) {
			opts.rank = ((opts.rank + direction + 4) % 4);
		} else if(menu_sel == KCO_MUSIC) {
			opts.bgm_mode = static_cast<bgm_mode_t>(1 - opts.bgm_mode);
		} else if(menu_sel == KCO_SFX) {
			t1_sfx_set(!t1_sfx_enabled());
		} else if(menu_sel == KCO_PLAYER) {
			opts.credit_lives_extra = ((opts.credit_lives_extra + direction + CFG_CREDIT_LIVES_EXTRA_MAX) %
				CFG_CREDIT_LIVES_EXTRA_MAX);
		} else if(menu_sel == KCO_LANGUAGE) {
			t1replay_op_language_toggle();
		}
		keyconfig_option_horizontal_locked = true;
		keyconfig_option_render();
	}
	if(input_cancel || ((input_ok || input_shot) && (menu_sel == KCO_QUIT))) {
		egc_copy_rect_1_to_0_16(208, 216, 224, 176);
		t1_audio_configure(opts.bgm_mode);
		menu_id = 5; // MID_UPDATE_BGM_MODE__DELAY__SWITCH_TO_MAIN
		menu_sel = 2;
		keyconfig_option_initialized = false;
		return;
	}
	if((input_ok || input_shot) && !keyconfig_option_confirm_locked) {
		keyconfig_option_confirm_locked = true;
		if(menu_sel == KCO_KEYS) {
			keyconfig_menu();
			t1replay_op_restore();
			menu_sel = KCO_KEYS;
			keyconfig_option_horizontal_locked = true;
			keyconfig_option_render();
		} else if(menu_sel == KCO_SFX) {
			t1_sfx_set(!t1_sfx_enabled());
			keyconfig_option_render();
		} else if(menu_sel == KCO_MUSIC_TEST) {
			egc_copy_rect_1_to_0_16(208, 216, 224, 176);
			keyconfig_option_return_from_music = true;
			menu_id = 2; // MID_MUSIC
			menu_sel = 0;
			keyconfig_option_initialized = false;
		}
	}
}
