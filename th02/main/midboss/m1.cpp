/// Stage 1 midboss
/// ---------------
/// Invalidation, defeat animation, firing pattern, and the public per-frame
/// update. These four procedures follow the Stage 1 scenery object in
/// BOSS_5_TEXT.

#pragma option -zCBOSS_5_TEXT -zPmain_03 -G

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/math/subpixel.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/score.hpp"
#include "th02/main/spark.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/midboss/midboss.hpp"
#include "th02/main/player/shot.hpp"
#include "th02/main/tile/tile.hpp"
#include "th02/sprites/main_pat.h"
#include "th02/v_colors.hpp"

extern "C" void __cdecl snd_se_play(int new_se);

extern "C" int boss_pos_x;
extern "C" int boss_pos_y;
extern "C" uint8_t boss_phase;

extern "C" int16_t stage1_scenery_frame;
extern "C" int16_t midboss1_defeat_frame;
extern "C" bool16 midboss1_active;
extern "C" int16_t midboss1_patnum;

static const int MIDBOSS1_W = 64;
static const int MIDBOSS1_H = 96;
static const int MIDBOSS1_DEFEAT_W = 96;
static const int MIDBOSS1_DEFEAT_LEFT_OFFSET = -16;
static const int MIDBOSS1_PATNUM = 148;
static const int MIDBOSS1_PATNUM_ALT = 149;

bool16 midboss1_invalidate(void)
{
	boss_left_on_back_page = &boss_left_on_page[page_back];
	boss_top_on_back_page = &boss_top_on_page[page_back];
	if(boss_phase != 0) {
		tiles_invalidate_rect(
			(*boss_left_on_back_page + MIDBOSS1_DEFEAT_LEFT_OFFSET),
			*boss_top_on_back_page,
			MIDBOSS1_DEFEAT_W,
			MIDBOSS1_H
		);
	} else {
		tiles_invalidate_rect(
			*boss_left_on_back_page,
			*boss_top_on_back_page,
			MIDBOSS1_W,
			MIDBOSS1_H
		);
	}
	*boss_left_on_back_page = boss_left_on_page[page_front];
	*boss_top_on_back_page = boss_top_on_page[page_front];
	return midboss1_active;
}

static void near midboss1_defeat_update_and_render(void)
{
	register screen_y_t y;
	register int zoom;

	zoom = 10;
	zoom += (midboss1_defeat_frame >> 3);
	midboss1_defeat_frame++;
	sparks_add(
		(*boss_left_on_back_page + 32),
		(*boss_top_on_back_page + 48),
		to_sp(3.75f),
		1,
		false
	);
	if(midboss1_defeat_frame >= 64) {
		sparks_add(
			(*boss_left_on_back_page + 32),
			(*boss_top_on_back_page + 48),
			to_sp(8.0f),
			48,
			true
		);
		midboss1_defeat_frame = 0;
		boss_phase_frame = 0;
		boss_pos_x = -1;
		boss_pos_y = -1;
		midboss1_active = false;
	} else {
		if((midboss1_defeat_frame & 3) == 0) {
			(*boss_top_on_back_page)++;
		}
		y = *boss_top_on_back_page;
		y += scroll_line;
		if(y >= RES_Y) {
			y -= RES_Y;
		}
		super_zoom(
			(*boss_left_on_back_page + MIDBOSS1_DEFEAT_LEFT_OFFSET),
			y,
			zoom,
			3
		);
	}
}

static void near midboss1_pattern(void)
{
	register screen_y_t top;

	top = (*boss_top_on_back_page + 88);
	if((boss_phase_frame & 0x7F) == 0) {
		bullets_add_pellet(
			(*boss_left_on_back_page + 28),
			top,
			0x00,
			BG_4_SPREAD_WIDE_AIMED,
			to_sp(3.125f)
		);
	} else if((boss_phase_frame & 0x7F) == 48) {
		bullets_add_pellet(
			(*boss_left_on_back_page + 28),
			top,
			0x00,
			BG_3_SPREAD_MEDIUM_AIMED,
			to_sp(1.875f)
		);
	} else if((boss_phase_frame & 0x7F) == 96) {
		bullets_add_pellet(
			(*boss_left_on_back_page + 28),
			top,
			0x00,
			BG_4_SPREAD_WIDE_AIMED,
			to_sp(3.125f)
		);
	}
}

void midboss1_update_and_render(void)
{
	int damage;
	register screen_y_t y;

	boss_pos_x = 216;
	boss_pos_y = 64;
	boss_phase_frame++;
	if(boss_phase_frame == 1) {
		boss_left_on_page[0] = (PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 32);
		boss_left_on_page[1] = (PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 32);
		boss_top_on_page[0] = (PLAYFIELD_TOP - 32);
		boss_top_on_page[1] = (PLAYFIELD_TOP - 32);
		boss_damage = 0;
		midboss1_patnum = MIDBOSS1_PATNUM;
		midboss1_active = true;
		stage1_scenery_frame = 1;
		return;
	}

	if(boss_phase_frame < 18) {
		*boss_top_on_back_page += 2;
		y = *boss_top_on_back_page;
		y += scroll_line;
		if(y < 0) {
			y += RES_Y;
		} else if(y >= RES_Y) {
			y -= RES_Y;
		}
	} else {
		if(scroll_line & 1) {
			midboss1_patnum = MIDBOSS1_PATNUM_ALT;
		} else {
			midboss1_patnum = MIDBOSS1_PATNUM;
		}
		if(boss_phase != 0) {
			midboss1_defeat_update_and_render();
			return;
		}

		midboss1_pattern();
		if((scroll_line & 0x0F) == 0) {
			y = 48;
			y += scroll_line;
			if(y >= RES_Y) {
				y -= RES_Y;
			}
			tile_ring_set_and_put_both_8(
				*boss_left_on_back_page, y, 60
			);
			tile_ring_set_and_put_both_8(
				(*boss_left_on_back_page + 16), y, 61
			);
			tile_ring_set_and_put_both_8(
				(*boss_left_on_back_page + 32), y, 62
			);
			tile_ring_set_and_put_both_8(
				(*boss_left_on_back_page + 48), y, 63
			);
		}

		y = *boss_top_on_back_page;
		y += scroll_line;
		if(y >= RES_Y) {
			y -= RES_Y;
		}
		if((damage = shots_hittest(
			*boss_left_on_back_page,
			*boss_top_on_back_page,
			MIDBOSS1_W,
			MIDBOSS1_H
		)) != 0) {
			boss_damage += damage;
			if((boss_damage <= 300) || (y >= 304)) {
				snd_se_play(4);
				super_roll_put_1plane(
					*boss_left_on_back_page,
					y,
					MIDBOSS1_PATNUM,
					0,
					super_plane(V_WHITE)
				);
			} else {
				snd_se_play(2);
				boss_phase = 1;
				score_delta += 10000;
			}
			return;
		}
		if((scroll_step >= 184) && (*boss_top_on_back_page <= 304)) {
			snd_se_play(2);
			boss_phase = 1;
			return;
		}
	}

	super_roll_put(*boss_left_on_back_page, y, midboss1_patnum);
}
