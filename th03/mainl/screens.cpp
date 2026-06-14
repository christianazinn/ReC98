#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/hardware/grppsafx.h"
#include "th02/v_colors.hpp"
#include "th03/common.h"
#include "th03/hardware/input.h"
#include "th03/resident.hpp"
#include "th03/formats/cdg.h"
#include "th03/formats/pi.hpp"
#include "th03/formats/win.hpp"
#include "th03/sprites/playchar.hpp"
#include "th02/hardware/frmdelay.h"
#include "th03/snd/snd.h"

extern const char near* PIC_FN[PLAYCHAR_COUNT];
shiftjis_t win_text[WIN_LINES][WIN_LINE_SIZE + 1];
PlaycharPaletted playchar[PLAYER_COUNT];

// Win screen
// ----------

static const int LOGO_FADE_CELS = 5;

enum win_cdg_slot_t {
	CDG_LOGO_FADE = 0,
	CDG_LOGO = (CDG_LOGO_FADE + LOGO_FADE_CELS),
	CDG_PIC_WINNER,
};

static const screen_y_t WIN_PIC_TOP = 64;
static const screen_y_t WIN_PIC_BOTTOM = (64 + PLAYCHAR_PIC_H);

inline playchar_t loser_char_id(void) {
	return static_cast<playchar_t>((resident->pid_winner == 0)
		? playchar[1].char_id()
		: playchar[0].char_id()
	);
}

void near win_load(void)
{
	extern const char logo0_rgb[];
	extern const char logo_cd2[];
	extern const char logo5_cdg[];
	extern const char* WIN_MESSAGE_FN[PLAYCHAR_COUNT];

	playchar_paletted_t playchar_winner;
	uint8_t message_id;

	// MODDERS: [playchar_winner] has to be declared as a scalar type to cause
	// Turbo C++ 4.0J to place it at its original stack position, and I'd like
	// to still use the `PlaycharPaletted` methods. Just turn [playchar_winner]
	// into a `PlaycharPaletted` and remove this macro.
	#define playchar_winner ( \
		reinterpret_cast<PlaycharPaletted &>(playchar_winner) \
	)

	palette_entry_rgb_show(logo0_rgb);
	cdg_load_all_noalpha(CDG_LOGO_FADE, logo_cd2);
	cdg_load_single(CDG_LOGO, logo5_cdg, 0);

	// ZUN bloat: playchar[resident->pid_winner]
	playchar_winner = ((resident->pid_winner == 0) ? playchar[0] : playchar[1]);

	if(resident->pid_winner == 0) {
		// The usually 0-based [story_stage] has already been incremented when
		// we get here, so these refer to the *next* stage, not the one that
		// has just been won.
		if(resident->story_stage == STAGE_DECISIVE) {
			message_id = (PLAYCHAR_COUNT + 0);
		} else if(resident->story_stage == STAGE_CHIYURI) {
			message_id = (PLAYCHAR_COUNT + 1);
		} else {
			message_id = loser_char_id();
		}
	} else {
		message_id = loser_char_id();
	}

	cdg_load_single_noalpha(
		CDG_PIC_WINNER,
		PIC_FN[playchar_winner.char_id_16()],
		playchar_winner.palette_id()
	);

	static_assert(WIN_LINES == 3);
	file_ropen(WIN_MESSAGE_FN[playchar_winner.char_id_16()]);
	file_seek((message_id * (WIN_LINES * WIN_LINE_SIZE)), SEEK_SET);
	file_read(win_text[0], WIN_LINE_SIZE);	win_text[0][WIN_LINE_SIZE] = '\0';
	file_read(win_text[1], WIN_LINE_SIZE);	win_text[1][WIN_LINE_SIZE] = '\0';
	file_read(win_text[2], WIN_LINE_SIZE);	win_text[2][WIN_LINE_SIZE] = '\0';
	file_close();

	#undef playchar_winner
}

void near win_text_put(void)
{
	enum {
		LEFT = ((RES_X / 2) - ((WIN_LINE_SIZE / 2) * GLYPH_HALF_W)),
		TOP = (WIN_PIC_BOTTOM + GLYPH_H),
		COL_AND_FX = (FX_WEIGHT_BOLD | V_WHITE),
	};

	static_assert(WIN_LINES == 3);
	graph_putsa_fx(LEFT, (TOP + (0 * GLYPH_H)), COL_AND_FX, win_text[0]);
	graph_putsa_fx(LEFT, (TOP + (1 * GLYPH_H)), COL_AND_FX, win_text[1]);
	graph_putsa_fx(LEFT, (TOP + (2 * GLYPH_H)), COL_AND_FX, win_text[2]);
}

#pragma codeseg CUTSCENE_TEXT group_01

int near sub_9887(void);
void near stage_splash_load(void);

void near win_animate_and_wait(void)
{
	register int cel;

	graph_accesspage(1);
	graph_clear();
	graph_accesspage(0);
	graph_clear();
	graph_showpage(0);
	PaletteTone = 0;
	palette_show();
	graph_show();
	cdg_put_noalpha_8(352, 300, CDG_LOGO_FADE);
	snd_kaja_func(KAJA_SONG_PLAY, 0);
	palette_black_in(2);
	snd_delay_until_measure(6, 16);

	for(cel = 1; cel < LOGO_FADE_CELS; cel++) {
		cdg_put_noalpha_8(352, 300, cel);
		frame_delay(6);
	}

	snd_delay_until_measure(10, 64);
	PaletteTone = 200;
	palette_show();
	cdg_put_noalpha_8(224, 64, CDG_PIC_WINNER);
	cdg_put_noalpha_8(352, 300, CDG_LOGO);

	extern const char logo1_rgb[];
	palette_entry_rgb(logo1_rgb);
	palette_show();
	cdg_free_all();
	snd_delay_until_measure(11, 4);
	palette_white_in(1);
	frame_delay(8);
	// Turbo C++ mis-relocates this near call from CUTSCENE_TEXT.
	_asm { db	0E8h, 0E0h, 0FEh; }
	if(sub_9887() == 0) {
		stage_splash_load();
	}
	while(1) {
		input_mode_interface();
		if(input_sp != INPUT_NONE) {
			break;
		}
		frame_delay(1);
	}

	palette_black_out(1);
}

int near sub_9887(void)
{
	int opponent;

	if(resident->game_mode != GM_STORY) {
		return 1;
	}
	if(resident->pid_winner != 0) {
		return 1;
	}

	resident->playchar_paletted[1] = resident->story_opponents[
		resident->story_stage
	];
	if(resident->story_stage == STAGE_CHIYURI) {
		return 3;
	}
	if(resident->story_stage == STAGE_YUMEMI) {
		return 4;
	}
	if(resident->story_stage == STAGE_COUNT) {
		return 5;
	}

	opponent = resident->playchar_paletted[1].char_id_16();
	if(opponent >= PLAYCHAR_CHIYURI) {
		resident->playchar_paletted[1].set(PLAYCHAR_REIMU);
	}
	return 0;
}

#pragma codeseg
// ----------

// Stage splash screen
// -------------------

bool do_not_show_stage_number;

extern const char near* stage_number_cdg_fn;
extern char near* stage_splash_bg_fn;
extern const char stage_splash_base_pi_fn[];

void near stage_splash_load(void)
{
	playchar_paletted_t paletted;

	graph_showpage(0);
	graph_accesspage(1);

	paletted = playchar[0].v;
	cdg_load_single(0, PIC_FN[paletted / 2], (paletted & 1));

	paletted = (resident->playchar_paletted[1].v - 1);
	cdg_load_single(1, PIC_FN[paletted / 2], (paletted & 1));

	paletted = (paletted / 2);
	do_not_show_stage_number = true;
	if(resident->game_mode != GM_STORY) {
		stage_splash_bg_fn[4] = (stage_splash_bg_fn[4] + 4);
	} else if(paletted == PLAYCHAR_CHIYURI) {
		stage_splash_bg_fn[4] = (stage_splash_bg_fn[4] + 2);
	} else if(paletted == PLAYCHAR_YUMEMI) {
		stage_splash_bg_fn[4]++;
	} else if(resident->story_stage == STAGE_DECISIVE) {
		stage_splash_bg_fn[4] = (stage_splash_bg_fn[4] + 3);
	} else {
		cdg_load_single(2, stage_number_cdg_fn, (resident->story_stage + 1));
		do_not_show_stage_number = false;
	}

	pi_load(0, stage_splash_base_pi_fn);
	pi_put_8(0, 0, 0);
	pi_free(0);
	pi_load(0, stage_splash_bg_fn);
	pi_put_8(0, 0, 0);
}
// -------------------
