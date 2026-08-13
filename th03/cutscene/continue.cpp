#pragma codeseg CUTSCENE_TEXT group_01

#include "libs/master.lib/master.hpp"
#include "th02/v_colors.hpp"
#include "th02/hardware/frmdelay.h"
#include "th01/hardware/grppsafx.h"
#include "th03/formats/cdg.h"
#include "th03/formats/pi.hpp"
#include "th03/hardware/input.h"
#include "th03/mainl/replay.hpp"
#include "th03/pixel_capture.hpp"
#include "th03/resident.hpp"
#include "th03/snd/snd.h"

extern char continue_count_str[];
extern const char continue_gameover_bg_pi_fn[];

int near continue_menu(void)
{
	t3pix_scene_set(T3PIX_SCENE_CONTINUE);
	int input_locked;
	char far *credit_str;
	register int ret;
	register int i;

	ret = 1;
	input_locked = 0;
	credit_str = continue_count_str;
	i = 0;
	while(i < (PLAYER_COUNT * SCORE_DIGITS)) {
		reinterpret_cast<unsigned char far *>(resident->score_last)[i] = 0;
		i++;
	}

	if(resident->rem_credits == 0) {
		return 0;
	}

	cdg_put_noalpha_8(192, 272, 0);
	cdg_put_noalpha_8(352, 272, 3);
	credit_str[0] += resident->rem_credits;
	graph_putsa_fx(576, 371, (V_WHITE | FX_WEIGHT_BOLD), credit_str);
	palette_black_in(1);

	while(1) {
		mainl_replay_input_mode_interface();

		if((input_sp & INPUT_LEFT) || (input_sp & INPUT_RIGHT)) {
			if(input_locked == 0) {
				ret = (1 - ret);
				cdg_put_noalpha_8(192, 272, (2 - (ret * 2)));
				cdg_put_noalpha_8(352, 272, ((ret * 2) + 1));
				input_locked = 1;
			}
		} else {
			input_locked = 0;
		}

		if((input_sp & INPUT_OK) || (input_sp & INPUT_SHOT)) {
			if(ret == 1) {
				grcg_setcolor(GC_RMW, 0);
				grcg_boxfill(576, 371, 592, 387);
				grcg_off();
				resident->rem_credits--;
				credit_str[0]--;
				graph_putsa_fx(576, 371, (V_WHITE | FX_WEIGHT_BOLD), credit_str);
			}
			break;
		} else if(input_sp & INPUT_CANCEL) {
			ret = 0;
			break;
		}
		frame_delay(1);
	}

	snd_kaja_func(KAJA_SONG_FADE, 3);
	palette_black_out(1);
	graph_accesspage(0);
	graph_showpage(0);
	PaletteTone = 0;
	palette_show();
	pi_load(0, continue_gameover_bg_pi_fn);
	pi_palette_apply(0);
	pi_put_8(0, 0, 0);
	pi_free(0);
	snd_kaja_func(KAJA_SONG_STOP, 0);
	resident->story_stage--;
	resident->story_lives = CREDIT_LIVES;
	return ret;
}

#pragma codeseg
