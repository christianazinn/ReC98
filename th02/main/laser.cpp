/* ReC98
 * -----
 * TH02's vertical boss lasers. ZUN's object placed these in the same code
 * segment as the dialog code, which is why they are compiled into the same
 * translation unit here — see th02/dialog.cpp.
 */

// The original's prologs are `push bp; mov bp, sp; sub sp, N`, which is -G.
// The dialog code that follows in this object was built without it and emits
// ENTER, so the option has to be turned back off at the end of this file.
// (kb/codegen/0011)
#pragma option -G

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/main/frames.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/entity.hpp"
#include "th02/main/laser.hpp"
#include "th02/main/tile/tile.hpp"
#include "th02/main/player/player.hpp"

// th02/snd/snd.h has no include guard, and th02/main/dialog/dialog.cpp
// includes it further down this translation unit. Declaring the one function
// needed here avoids the duplicate. `DEFCONV` is __cdecl for TH02.
extern "C" void __cdecl snd_se_play(int new_se);

// Pattern numbers. All below 128, i.e. inside the stage-independent set that
// stage_init() loads once; th02/sprites/main_pat.h leaves the 10…33 and
// 88…121 ranges unnamed, so these are held here until a naming review decides
// whether they belong in that enum.
// ----------------------------------------------------------------------

// 32×32, four cels of the charge animation, indexed by [charge_cel].
static const int PATNUM_CHARGE = 30;

// 16×16, the cap at the top of the beam, indexed by [phase].
static const int PATNUM_HEAD = 91;
// ----------------------------------------------------------------------

// Size of the charge animation's sprite, and the distance from the laser's
// origin to that sprite's top-left corner.
static const pixel_t CHARGE_W = 32;
static const pixel_t CHARGE_H = 32;
static const pixel_t CHARGE_OFFSET = 8;

// The cels the beam is stacked out of, and the distance from the cap sprite's
// top edge to the first of them.
static const pixel_t BEAM_W = 16;
static const pixel_t BEAM_CEL_H = 16;
static const pixel_t BEAM_CEL_FIRST_OFFSET = 8;

// [charge_cel] stops here. lasers_invalidate() unblits the 32×32 charge sprite
// while [charge_cel] is ≤ this, but lasers_update_and_render() already draws
// the beam once it reaches it, so the two disagree for one 4-frame window.
static const uint8_t CHARGE_CEL_LAST = 4;

// Half-open horizontal range, relative to the beam's origin, in which the
// player's top-left corner counts as hit.
static const pixel_t HITBOX_LEFT = -24;
static const pixel_t HITBOX_RIGHT = 8;

// Marks the tiles under every live laser for redrawing, and doubles as the
// driver of the charge animation. Retires lasers that were marked F_REMOVE
// during the previous frame, after this final unblit.
void far lasers_invalidate(void)
{
	register laser_t near *laser = lasers;
	register int i;

	for(i = 0; i < LASER_COUNT; i++, laser++) {
		if(laser->flag == F_FREE) {
			continue;
		}
		if(laser->charge_cel <= CHARGE_CEL_LAST) {
			tiles_invalidate_rect(
				(laser->origin.x - CHARGE_OFFSET),
				(laser->origin.y - CHARGE_OFFSET),
				CHARGE_W,
				CHARGE_H
			);
			if((stage_frame & 3) == 0) {
				laser->charge_cel++;
			}
		} else {
			tiles_invalidate_rect(
				laser->origin.x,
				laser->origin.y,
				BEAM_W,
				(RES_Y - laser->origin.y)
			);
		}
		if(laser->flag == F_REMOVE) {
			laser->flag = F_FREE;
		}
	}
}

// Blits the beam at [laser_origin]: a cap sprite, then a column of 16×16 cels
// down to the bottom of the playfield. Also runs the player collision, which
// only applies during LASER_PHASE_ACTIVE.
void pascal near laser_render(laser_t near *laser)
{
	screen_y_t y;
	int patnum;
	register vram_y_t vram_top;

	vram_top = scroll_line;
	y = laser_origin.y;
	vram_top += laser_origin.y;
	if(vram_top >= RES_Y) {
		vram_top -= RES_Y;
	}
	if(laser->phase < LASER_PHASE_SHRINK) {
		super_roll_put_tiny(
			laser_origin.x, vram_top, (PATNUM_HEAD + laser->phase)
		);
	}
	vram_top += BEAM_CEL_FIRST_OFFSET;
	if(vram_top >= RES_Y) {
		vram_top -= RES_Y;
	}
	y += BEAM_CEL_FIRST_OFFSET;
	patnum = (laser->patnum_base + laser->phase);
	while(y < PLAYFIELD_BOTTOM) {
		super_roll_put_tiny(laser_origin.x, vram_top, patnum);
		vram_top += BEAM_CEL_H;
		if(vram_top >= RES_Y) {
			vram_top -= RES_Y;
		}
		y += BEAM_CEL_H;
	}
	if(laser->phase == LASER_PHASE_ACTIVE) {
		if(
			((laser_origin.x + HITBOX_LEFT) < player_topleft.x) &&
			((laser_origin.x + HITBOX_RIGHT) > player_topleft.x) &&
			(player_topleft.y >= laser_origin.y) &&
			(player_is_hit == PLAYER_NOT_HIT)
		) {
			player_is_hit = PLAYER_HIT;
		}
	}
}

// Advances every live laser's phase and renders it. stage_loop() calls this
// through the [farfp_23A76] callback.
void far lasers_update_and_render(void)
{
	screen_x_t x;
	int i;
	int patnum;
	register laser_t near *laser = lasers;
	register vram_y_t vram_top;

	for(i = 0; i < LASER_COUNT; i++, laser++) {
		if(laser->flag != F_ALIVE) {
			continue;
		}
		laser_origin.x = laser->origin.x;
		laser_origin.y = laser->origin.y;
		if(laser->charge_cel < CHARGE_CEL_LAST) {
			vram_top = (laser_origin.y - CHARGE_OFFSET);
			vram_top += scroll_line;
			if(vram_top >= RES_Y) {
				vram_top -= RES_Y;
			}
			x = (laser_origin.x - CHARGE_OFFSET);
			patnum = (PATNUM_CHARGE + laser->charge_cel);
			super_roll_put(x, vram_top, patnum);
			laser->wait_frames--;
			continue;
		}
		if(laser->phase == LASER_PHASE_WAIT) {
			laser->wait_frames--;
			if(laser->wait_frames <= 0) {
				laser->phase++;
			}
		} else if(laser->phase < LASER_PHASE_ACTIVE) {
			if((stage_frame & 7) == 0) {
				laser->phase++;
			}
			if(laser->phase == LASER_PHASE_ACTIVE) {
				snd_se_play(7);
			}
		} else if(laser->phase == LASER_PHASE_ACTIVE) {
			laser->active_frames--;
			if(laser->active_frames == 0) {
				laser->phase++;
			}
		} else {
			if((stage_frame & 3) == 0) {
				laser->phase++;
			}
			if(laser->phase == LASER_PHASE_DONE) {
				laser->flag = F_REMOVE;
				continue;
			}
		}
		laser_render(laser);
	}
}

#pragma option -G-
