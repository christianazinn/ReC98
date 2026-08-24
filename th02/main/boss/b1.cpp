/// Stage 1 boss - Rika
/// -------------------
/// The shared boss-song loader followed by Rika's complete callback set.

#pragma option -zCBOSS_5_TEXT -zPmain_03 -G

#include "platform.h"
#include "pc98.h"
#include "th01/rank.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/core/globals.hpp"
#include "th02/hardware/input.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/frames.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/score.hpp"
#include "th02/main/spark.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/dialog/dialog.hpp"
#include "th02/main/explode.hpp"
#include "th02/main/hud/overlay.hpp"
#include "th02/main/laser.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/shot.hpp"
#include "th02/main/stage/bonus.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/tile/tile.hpp"
#include "th02/snd/snd.h"
#include "th02/sprites/bullet16.h"
#include "th02/sprites/main_pat.h"
#include "th02/v_colors.hpp"

void near dialog_pre(void);
void near dialog_post(void);

extern "C" int patnum_2064E;
extern "C" int boss_pos_x;
extern "C" int boss_pos_y;
extern "C" uint8_t boss_phase;
extern "C" uint8_t boss_rank_param[5];

extern "C" char rika_bgm_fn[];
extern "C" screen_point_t rika_topleft;
extern "C" int16_t rika_move_state;
extern "C" int16_t rika_defeat_frame;
extern "C" uint8_t rika_pattern_angle;
extern "C" uint16_t rika_pattern_frame;
extern "C" int16_t bg_flash_frame;

extern "C" void far boss_bgm_load(char *fn)
{
	snd_kaja_func(KAJA_SONG_STOP, 0);
	snd_load(fn, SND_LOAD_SONG);
	snd_kaja_func(KAJA_SONG_PLAY, 0);
}

extern "C" void far rika_init(void)
{
	boss_bgm_load(rika_bgm_fn);
	dialog_pre();
	dialog_script_generic_part_animate(DS_PREBOSS);
	dialog_post();
	boss_explode_angle_offset = 0x20;
	boss_left_on_page[0] = (PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 32);
	boss_left_on_page[1] = (PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 32);
	boss_damage = 0;
	boss_phase = 0;
	patnum_2064E = 150;
	stage_frame = 0;
	rika_move_state = 0;

	__emit__(0x90); // nop
	__emit__(0x0E); // push cs
	_asm { call near ptr lasers_callbacks_set; }

	patnum_2064E = 150;
	rika_topleft.y = 48;
	rika_topleft.y += scroll_line;
	if(rika_topleft.y >= RES_Y) {
		rika_topleft.y -= RES_Y;
	}
	if(rank != RANK_EASY) {
		boss_rank_param[0] = BG_3_SPREAD_MEDIUM_AIMED;
		boss_rank_param[1] = BG_5_SPREAD_WIDE;
		boss_rank_param[2] = BG_1;
		boss_rank_param[3] = BG_2_SPREAD_MEDIUM;
		boss_rank_param[4] = 3;
	} else {
		boss_rank_param[0] = BG_1_AIMED;
		boss_rank_param[1] = BG_5_SPREAD_WIDE;
		boss_rank_param[2] = BG_1;
		boss_rank_param[3] = BG_1;
		boss_rank_param[4] = 7;
	}
}

extern "C" void far rika_end(void)
{
	dialog_pre();
	dialog_script_generic_part_animate(DS_POSTBOSS);
	stage_clear_bonus_animate();
	key_delay();
	overlay_stage_leave_animate();
	stage_id++;
	spark_accel_x.v = 0;
	bg_flash_frame = 0;
}

extern "C" void far rika_bg_render(void)
{
	boss_left_on_back_page = &boss_left_on_page[page_back];
	tiles_invalidate_rect(*boss_left_on_back_page, 48, 64, 96);
	*boss_left_on_back_page = boss_left_on_page[page_front];
	if((rika_move_state == 0) && (boss_phase_frame < 3)) {
		tiles_invalidate_rect(208, 16, 32, 32);
	}
}

static bool16 near rika_defeat_update_and_render(void)
{
	register int zoom;

	sparks_add(
		(rika_topleft.x + 32),
		(PLAYFIELD_LEFT + 96),
		(PLAYFIELD_TOP + 34),
		2,
		false
	);
	zoom = 10;
	zoom += ((rika_defeat_frame - 32) / 8);
	rika_defeat_frame++;
	if(rika_defeat_frame >= 80) {
		rika_defeat_frame = 0;
		return true;
	}
	if(boss_phase_frame < 32) {
		super_roll_put(rika_topleft.x, rika_topleft.y, patnum_2064E);
	} else {
		super_zoom(
			rika_topleft.x, (rika_topleft.y + 16), zoom, 2
		);
	}
	boss_explode_render(
		(rika_topleft.x + 24), (PLAYFIELD_TOP + 96), rika_defeat_frame
	);
	return false;
}

static void near rika_pattern(void)
{
	uint8_t group;
	register screen_x_t left;

	left = (rika_topleft.x + 28);
	if(boss_damage < 700) {
		rika_move_state = 0;
		if((boss_phase_frame & 0x3F) == 0) {
			snd_se_play(3);
			bullets_add_pellet(
				left,
				(PLAYFIELD_TOP + 96),
				0x00,
				boss_rank_param[0],
				to_sp(3.125f)
			);
			bullets_add_pellet(
				left,
				(PLAYFIELD_TOP + 96),
				0x40,
				boss_rank_param[1],
				to_sp(1.875f)
			);
		}
	} else if(boss_damage < 1400) {
		if((stage_frame & 0x1F) == 0) {
			rika_pattern_angle += 0x18;
			rika_pattern_angle %= 0x80;
			left = (rika_topleft.x + 8);
			snd_se_play(3);
			bullets_add_pellet(
				left,
				(PLAYFIELD_TOP + 96),
				rika_pattern_angle,
				boss_rank_param[2],
				to_sp(3.375f)
			);
			bullets_add_pellet(
				left,
				(PLAYFIELD_TOP + 96),
				rika_pattern_angle,
				boss_rank_param[3],
				to_sp(2.75f)
			);
			left += 44;
			bullets_add_pellet(
				left,
				(PLAYFIELD_TOP + 96),
				(0x80 - rika_pattern_angle),
				boss_rank_param[2],
				to_sp(3.375f)
			);
			bullets_add_pellet(
				left,
				(PLAYFIELD_TOP + 96),
				(0x80 - rika_pattern_angle),
				boss_rank_param[3],
				to_sp(2.75f)
			);
		}
	} else {
		if(rika_move_state == 0) {
			rika_move_state = 1;
			rika_pattern_frame = 0;
			rika_pattern_angle = 0;
		}
		rika_pattern_frame++;
		if(rika_pattern_frame < 100) {
			if((rika_pattern_frame & 7) < 4) {
				patnum_2064E = 150;
			} else {
				patnum_2064E = 151;
			}
			if((rika_pattern_frame & 0x1F) == 0) {
				snd_se_play(9);
			}
		} else if(rika_pattern_frame == 100) {
			if((rika_pattern_frame & 0x1F) == 0) {
				snd_se_play(9);
			}
			laser_wait_frames = 0x40;
			lasers_add(
				(rika_topleft.x + 2), 112, 160, 0x6F
			);
			lasers_add(
				(rika_topleft.x + 46), 112, 160, 0x6F
			);
		} else if(
			(rika_pattern_frame > 164) && (rika_pattern_frame < 280)
		) {
			if(rika_pattern_frame < 250) {
				if((rika_pattern_frame & 7) < 4) {
					patnum_2064E = 150;
				} else {
					patnum_2064E = 151;
				}
			} else if(rika_pattern_frame == 260) {
				patnum_2064E = 152;
			} else if(rika_pattern_frame == 270) {
				patnum_2064E = 153;
			}
			if((rika_pattern_frame & 0x0F) == 0) {
				snd_se_play(6);
			}
			if((rika_pattern_frame & 0x0F) == 0) {
				if((rika_pattern_frame & 0x3F) == 0) {
					group = BG_4_SPREAD_WIDE;
				} else if((rika_pattern_frame & 0x3F) == 0x10) {
					group = BG_4_SPREAD_MEDIUM;
				} else if((rika_pattern_frame & 0x3F) == 0x20) {
					group = BG_4_SPREAD_WIDE;
				} else if((rika_pattern_frame & 0x3F) == 0x30) {
					group = BG_4_SPREAD_MEDIUM;
				}
				bullets_add_pellet(
					left,
					(PLAYFIELD_TOP + 96),
					0x40,
					group,
					to_sp(3.625f)
				);
			}
		} else if(
			(rika_pattern_frame >= 280) &&
			((rika_pattern_frame & boss_rank_param[4]) == 0)
		) {
			bullets_add_16x16(
				left,
				(PLAYFIELD_TOP + 56),
				0x01,
				BG_RANDOM_ANGLE_AND_SPEED,
				PAT_BULLET16_OUTLINED_BALL_BEIGE,
				to_sp(2.75f)
			);
		}
	}
}

static bool16 far rika_hittest_and_put(void)
{
	int damage;

	if((damage = shots_hittest(rika_topleft.x, 48, 64, 96)) != 0) {
		boss_damage += damage;
		if(boss_damage <= 2260) {
			snd_se_play(4);
			super_roll_put_1plane(
				rika_topleft.x,
				rika_topleft.y,
				patnum_2064E,
				0,
				super_plane(V_WHITE)
			);
		} else {
			boss_phase = 1;
			score_delta += 20000;
			player_invincibility_time = BOSS_DEFEAT_INVINCIBILITY_FRAMES;
		}
		return true;
	}
	return false;
}

static void far rika_move(void)
{
	uint8_t angle;

	if(rika_move_state == 0) {
		if(boss_phase_frame < 16) {
			if((boss_phase_frame & 3) != 0) {
				return;
			}
			if(rank >= RANK_HARD) {
				angle = boss_phase_frame;
			} else {
				angle = 0;
			}
			bullets_add_pellet(
				228,
				112,
				angle,
				BG_16_RING,
				((boss_phase_frame << 2) + to_sp(2.5f))
			);
		} else if(boss_phase_frame < 116) {
			(*boss_left_on_back_page)--;
		} else if(boss_phase_frame < 316) {
			(*boss_left_on_back_page)++;
		} else {
			if(boss_phase_frame >= 416) {
				boss_phase_frame = 16;
			}
			(*boss_left_on_back_page)--;
		}
	} else if(rika_move_state == 1) {
		if(*boss_left_on_back_page <
			(PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 32)
		) {
			(*boss_left_on_back_page)++;
		} else if(boss_phase_frame > 192) {
			(*boss_left_on_back_page)--;
		} else {
			rika_move_state = 2;
		}
	} else if(rika_move_state == 2) {
		boss_phase_frame = 0;
	}
}

extern "C" int far rika_update(void)
{
	boss_pos_x = (rika_topleft.x + 24);
	boss_pos_y = 48;
	boss_phase_frame++;
	if(boss_phase == 0) {
		rika_move();
		rika_topleft.x = *boss_left_on_back_page;
		if(!rika_hittest_and_put()) {
			super_roll_put(
				rika_topleft.x, rika_topleft.y, patnum_2064E
			);
		}
		rika_pattern();
	} else if(rika_defeat_update_and_render()) {
		return SP_CLEAR;
	}
	return SP_BOSS;
}
