#pragma option -zCmainl_03_TEXT -zPgroup_01

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/frmdelay.h"
#include "th03/fast_forward.hpp"
#include "th03/formats/cdg.h"
#include "th03/formats/scoredat.hpp"
#include "th03/hardware/input.h"
#include "th03/mainl/replay.hpp"
#include "th03/language.hpp"
#include "th03/resident.hpp"
#include "th03/snd/snd.h"
#include "x86real.h"

extern const char staffroll_bgm_fn[];
extern const char staffroll_bg_palette_fn[];
extern const char staffroll_cdg_fn_0[];
extern const char staffroll_cdg_fn_1[];
extern const char staffroll_cdg_fn_2[];
extern const char staffroll_cdg_fn_3[];
extern const char staffroll_cdg_fn_4[];
extern const char staffroll_cdg_fn_5[];
extern const char staffroll_cdg_fn_6[];
extern const char staffroll_cdg_fn_7[];
extern const char staffroll_cdg_fn_8[];
extern const char staffroll_cdg_fn_9[];
extern const char staffroll_cdg_fn_10[];
extern const char staffroll_cdg_fn_11[];

extern unsigned char continues_used;
extern page_t page_back;
extern unsigned char rank;
extern unsigned char score[];
extern unsigned char skill;
extern unsigned char staffroll_flake_count;
extern int staffroll_frame;
extern bool staffroll_cdg_put_alpha;
extern bool staffroll_cdg_alpha;
extern int staffroll_cdg_speed;
extern int staffroll_cdg_frames;
extern screen_y_t staffroll_cdg_y_stop;
extern screen_y_t staffroll_cdg_y_start_off;
extern screen_y_t staffroll_cdg_overlay_y;
extern unsigned char staffroll_cdg_aux_slot;
extern bool staffroll_flake_reset_pending;
extern unsigned char staffroll_verdict_playchar;

void near staffroll_blue_plane_clear(void);
void near flakes_reset(void);
void near staffroll_flakes_tick(void);
bool16 pascal near staffroll_phase_done(
	uint16_t measure_threshold, int frame_threshold
);
void pascal near staffroll_cdg_dissolve_in(
	int slot, screen_y_t center_y, screen_x_t center_x
);
void pascal near staffroll_cdg_slide_cycle(
	int slot_arg, int frame_threshold_in, int frame_threshold_out
);
void pascal near staffroll_cdg_overlay(
	int slot_arg, int frame_threshold_in, int frame_threshold_out
);
void pascal near staffroll_cdg_setup_y(
	int slot_arg, int frame_threshold_in, int frame_threshold_out
);
void near staffroll_verdict_overlay_put(void);

// TCC bundles the first two source arguments of these 3-int pascal calls.
#define staffroll_cdg_dissolve_in_call(slot, center_y, center_x) \
	staffroll_cdg_dissolve_in(center_x, center_y, slot)

#define staffroll_cdg_slide_cycle_call(slot_arg, frame_threshold_in, frame_threshold_out) \
	staffroll_cdg_slide_cycle(frame_threshold_out, frame_threshold_in, slot_arg)

#define staffroll_cdg_overlay_call(slot_arg, frame_threshold_in, frame_threshold_out) \
	staffroll_cdg_overlay(frame_threshold_out, frame_threshold_in, slot_arg)

#define staffroll_cdg_setup_y_call(slot_arg, frame_threshold_in, frame_threshold_out) \
	staffroll_cdg_setup_y(frame_threshold_out, frame_threshold_in, slot_arg)

static bool near staffroll_fast_forward_unlocked_load(void)
{
	for(int i = RANK_EASY; i < (RANK_LUNATIC + 1); i++) {
		scoredat_load_and_decode(static_cast<rank_t>(i));
		if(hi.score.cleared == SCOREDAT_CLEARED) {
			return true;
		}
	}
	return false;
}

void near staffroll_and_verdict_animate(void)
{
	#define i   	_SI
	#define tone	_DI

	snd_kaja_func(KAJA_SONG_FADE, 16);
	palette_black_out(4);
	snd_delay_until_volume(255);
	snd_kaja_func(KAJA_SONG_STOP, 0);
	resident->unused_3[T3_RES_FAST_FORWARD_STAFF_UNLOCKED_INDEX] =
		staffroll_fast_forward_unlocked_load();
	resident->unused_3[T3_RES_FAST_FORWARD_STAFF_PHASE_INDEX] = 0;

	staffroll_flake_count = 0x50;
	i = 1;
	while(static_cast<int16_t>(i) < 9) {
		score[i] = reinterpret_cast<unsigned char far *>(&resident->pid_winner)[i];
		i++;
	}

	continues_used = (3 - resident->rem_credits);
	staffroll_verdict_playchar = ((resident->playchar_paletted[0].v - 1) / 2);
	rank = resident->rank;
	skill = resident->skill;

	switch(score[7]) {
	case 3:
		skill += ((score[6] / 2) + 2);
	case 4:
		skill += ((score[6] / 2) + 7);
		break;
	}
	if(score[7] >= 5) {
		skill += 15;
	}
	if(score[8] != 0) {
		skill = 100;
	}
	if(skill > 100) {
		skill = 100;
	}

	snd_load(staffroll_bgm_fn, SND_LOAD_SONG);
	PaletteTone = 0;
	palette_show();
	palette_entry_rgb(staffroll_bg_palette_fn);
	palette_show();

	grcg_setcolor(GC_RMW, 8);
	graph_accesspage(1);
	grcg_byteboxfill_x(0, 0, ((RES_X - 1) / 8), (RES_Y - 1));
	graph_accesspage(0);
	grcg_byteboxfill_x(0, 0, ((RES_X - 1) / 8), (RES_Y - 1));
	grcg_setcolor(GC_RMW, 0);
	graph_accesspage(1);
	grcg_byteboxfill_x((8 / 8), 8, ((RES_X - 1 - 8) / 8), (RES_Y - 1 - 8));
	graph_accesspage(0);
	grcg_byteboxfill_x((8 / 8), 8, ((RES_X - 1 - 8) / 8), (RES_Y - 1 - 8));
	grcg_off();
	graph_showpage(1);

	cdg_load_single_noalpha(0, staffroll_cdg_fn_0, 0);
	cdg_load_single_noalpha(1, staffroll_cdg_fn_1, 0);
	cdg_load_single(2, staffroll_cdg_fn_2, 0);
	cdg_load_single(3, staffroll_cdg_fn_3, 0);
	cdg_load_single_noalpha(4, staffroll_cdg_fn_4, 0);
	cdg_load_single_noalpha(5, staffroll_cdg_fn_5, 0);
	cdg_load_single_noalpha(6, staffroll_cdg_fn_6, 0);
	cdg_load_single_noalpha(7, staffroll_cdg_fn_7, 0);
	{
		bool16 language_switched = language_archive_begin_if_translated(
			staffroll_cdg_fn_8
		);
		cdg_load_single_noalpha(8, staffroll_cdg_fn_8, 0);
		cdg_load_single_noalpha(9, staffroll_cdg_fn_9, 0);
		language_archive_end(language_switched);
	}
	cdg_load_single_noalpha(10, staffroll_cdg_fn_10, 0);
	cdg_load_single_noalpha(11, staffroll_cdg_fn_11, 0);

	flakes_reset();
	staffroll_frame = 0;
	random_seed = resident->rand;
	page_back = 0;
	PaletteTone = 100;
	palette_show();
	snd_kaja_func(KAJA_SONG_PLAY, 0);
	staffroll_flake_reset_pending = true;
	staffroll_cdg_put_alpha = true;
	frame_delay(1);
	vsync_Count1 = 0;

	do {
		staffroll_blue_plane_clear();
		staffroll_flakes_tick();
		staffroll_frame++;
	} while(!staffroll_phase_done(4, 0x100));

	staffroll_cdg_alpha = false;
	staffroll_cdg_y_start_off = 0;
	staffroll_cdg_y_stop = 200;
	staffroll_cdg_speed = 2;
	staffroll_cdg_frames = 0x41;
	staffroll_cdg_aux_slot = 0;
	staffroll_cdg_slide_cycle_call(10, 8, 0);

	staffroll_cdg_speed = 1;
	staffroll_cdg_frames = 0xA1;
	staffroll_flake_reset_pending = false;
	staffroll_cdg_slide_cycle_call(0x14, 0x10, 1);

	staffroll_cdg_y_start_off = 0x20;
	staffroll_cdg_y_stop = 0xA8;
	staffroll_cdg_setup_y_call(0x18, 0x16, 2);

	staffroll_cdg_aux_slot = 7;
	staffroll_cdg_y_stop = 0xD8;
	staffroll_cdg_y_start_off = -0x10;
	staffroll_cdg_overlay_call(0x22, 0x20, 3);

	staffroll_cdg_aux_slot = 0;
	staffroll_cdg_y_stop = 0xC8;
	staffroll_cdg_y_start_off = 0;
	staffroll_cdg_slide_cycle_call(0x26, 0x24, 4);
	staffroll_cdg_slide_cycle_call(0x2C, 0x2A, 0x0B);
	staffroll_cdg_slide_cycle_call(0x32, 0x30, 5);
	staffroll_cdg_slide_cycle_call(0x38, 0x36, 6);
	staffroll_cdg_slide_cycle_call(0x3E, 0x3C, 0x0A);

	staffroll_frame = 0;
	do {
		staffroll_blue_plane_clear();
		staffroll_cdg_dissolve_in_call(8, 0x80, 0x140);
		staffroll_cdg_dissolve_in_call(9, 0xF0, 0x0C0);
		staffroll_flakes_tick();
		staffroll_frame++;
	} while(!staffroll_phase_done(0x42, 0x100));

	graph_accesspage(1 - page_back);
	staffroll_verdict_overlay_put();
	graph_accesspage(page_back);
	staffroll_verdict_overlay_put();

	staffroll_frame = 0;
	tone = 0;
	do {
		mainl_replay_input_mode_interface();
		staffroll_blue_plane_clear();
		staffroll_flakes_tick();
		staffroll_frame++;
		if(tone != 0) {
			PaletteTone = tone;
			palette_show();
			if(staffroll_frame & 1) {
				tone--;
				if(tone == 0) {
					break;
				}
			}
		} else {
			if(input_sp != INPUT_NONE) {
				if(staffroll_frame > 0x100) {
					snd_kaja_func(KAJA_SONG_FADE, 8);
					tone = 100;
					staffroll_frame = 0;
				}
			}
		}
	} while(1);

	i = 0;
	while(static_cast<int16_t>(i) < CDG_SLOT_COUNT) {
		cdg_free(i);
		i++;
	}
	resident->unused_3[T3_RES_FAST_FORWARD_STAFF_UNLOCKED_INDEX] = false;
	resident->unused_3[T3_RES_FAST_FORWARD_STAFF_PHASE_INDEX] = 0;

	#undef tone
	#undef i
}

#undef staffroll_cdg_setup_y_call
#undef staffroll_cdg_overlay_call
#undef staffroll_cdg_slide_cycle_call
#undef staffroll_cdg_dissolve_in_call
