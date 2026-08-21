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
#include "th02/sprites/main_pat.h"

// th02/snd/snd.h has no include guard, and th02/main/dialog/dialog.cpp
// includes it further down this translation unit. Declaring the one function
// needed here avoids the duplicate. `DEFCONV` is __cdecl for TH02.
extern "C" void __cdecl snd_se_play(int new_se);

// The two pattern numbers this file uses, PAT_LASER_CHARGE and PAT_LASER_HEAD,
// live in th02/sprites/main_pat.h with every other stage-independent patnum.

// Size of the charge animation's sprite, and the distance from the laser's
// origin to that sprite's top-left corner. One constant for both axes, so it
// takes no axis suffix, unlike MISSILE_OFFSET_LEFT / _TOP
// (th01/main/bullet/missile.cpp).
static const pixel_t CHARGE_W = 32;
static const pixel_t CHARGE_H = 32;
static const pixel_t CHARGE_OFFSET = 8;

// The 16×16 cels the beam is stacked out of, and the distance from the cap
// sprite's top edge to the first of them.
static const pixel_t BEAM_CEL_W = 16;
static const pixel_t BEAM_CEL_H = 16;
static const pixel_t BEAM_CEL_FIRST_OFFSET = 8;

// The two per-frame functions disagree about when the charge animation ends.
// lasers_invalidate() unblits the 32×32 charge box while [charge_cel] is
// ≤ LASER_CHARGE_CELS, but lasers_update_and_render() already draws the beam
// once [charge_cel] merely *reaches* that count — and [charge_cel] only
// advances every 4th frame. So for one 4-frame window the beam is drawn while
// only a 32×32 box around its origin is invalidated, leaving the scrolling
// tiles under the rest of the beam unmarked for redrawing.
// ZUN landmine: the window is measurably not observable. A per-frame hash of all
// four VRAM bitplanes of both pages, over ZUN's DEMO1.REC, is identical between
// this build and one whose bound is harmonised to `<` — on all 231 rows a laser
// is live, both before the frame's sprites are drawn and on the composed page —
// while [charge_cel] itself differs on 85 of them, so the two really do run the
// two branches. Neither rect is load-bearing at the handover.
//
// `landmine` rather than `bloat`, and the distinction is the whole point: this
// is not redundant work that could be simplified away, it is two bounds that
// must agree and do not, mitigated only because the midboss the laser sits on
// invalidates the same neighbourhood every frame anyway. Move the origin off
// the midboss, lengthen the beam or retime the charge, and the unmarked tiles
// become real — which is exactly what "breaks as soon as the code is modded"
// means. The measurement also reaches only one of the four spawning entities;
// the other three are boss-side and no zero-input corpus gets there.
// (state/port/FIX_LAYER_CANDIDATES.md J5, state/notes/th02-effect-slots.md §14)

// Origin of the laser currently being rendered. Scratch, only used to pass the
// position from lasers_update_and_render() to laser_render(), so it stays out
// of the header — the same treatment TH02 already gives [item_p_left] /
// [item_p_top] at th02/main/item/item.cpp:74-79.
// ZUN bloat: laser_render() already receives the laser itself.
extern screen_point_t laser_origin;

// Offsets from the beam's origin, not screen coordinates — hence
// `HITBOX_OFFSET_*` rather than TH01's absolute `HITBOX_LEFT` / `HITBOX_RIGHT`
// (th01/main/boss/b15j.cpp:44-45); this is the shape of
// HITBOX_OFFSET_LEFT / _RIGHT at th01/main/bullet/missile.cpp:184-185.
// The range is open at both ends (`<` on the left bound, `>` on the right),
// i.e. th01/math/overlap.hpp's `_lt_gt` shape.
static const pixel_t HITBOX_OFFSET_LEFT = -24;
static const pixel_t HITBOX_OFFSET_RIGHT = 8;

// The value lasers_reset() puts back into [laser_wait_frames] at the start of
// every stage. Three of that variable's twelve ASM writes happen to write this
// same 16; the other nine spread over seven other values, and one spawner never
// writes it at all. Census in th02/main/laser.hpp.
static const int LASER_WAIT_FRAMES_DEFAULT = 16;

// th02/main/stage/callback.hpp declares these two slots, but it needs
// th02/main/stage/stage.hpp, which has no include guard and which
// th02/main/dialog/dialog.cpp includes further down this translation unit.
extern void (far *lasers_invalidate_func)(void);
extern void (far *lasers_update_and_render_func)(void);

// Frees every slot at the beginning of a stage.
void far lasers_reset(void)
{
	register laser_t near *laser = lasers;
	int i;

	for(i = 0; i < LASER_COUNT; i++, laser++) {
		laser->flag = F_FREE;
	}
	laser_wait_frames = LASER_WAIT_FRAMES_DEFAULT;
}

// Turns the subsystem on for the rest of the stage.
void far lasers_callbacks_set(void)
{
	lasers_invalidate_func = lasers_invalidate;
	lasers_update_and_render_func = lasers_update_and_render;
}

// Spawns a laser at ([left], [top]), unless [left] is outside the playfield or
// every slot is taken.
void pascal near lasers_add(
	screen_x_t left, screen_y_t top, int active_frames, int patnum_base
)
{
	register laser_t near *laser = lasers;
	register int i;

	if((left < PLAYFIELD_LEFT) || (left >= PLAYFIELD_RIGHT)) {
		return;
	}
	for(i = 0; i < LASER_COUNT; i++, laser++) {
		if(laser->flag != F_FREE) {
			continue;
		}
		laser->flag = F_ALIVE;
		laser->phase = LASER_PHASE_WAIT;
		laser->origin.x = left;
		laser->origin.y = top;
		laser->wait_frames = laser_wait_frames;
		laser->active_frames = active_frames;
		laser->charge_cel = 0;
		laser->patnum_base = patnum_base;
		snd_se_play(6);
		return;
	}
}

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
		if(laser->charge_cel <= LASER_CHARGE_CELS) {
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
				BEAM_CEL_W,
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
			laser_origin.x, vram_top, (PAT_LASER_HEAD + laser->phase)
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
			((laser_origin.x + HITBOX_OFFSET_LEFT) < player_topleft.x) &&
			((laser_origin.x + HITBOX_OFFSET_RIGHT) > player_topleft.x) &&
			(player_topleft.y >= laser_origin.y) &&
			(player_is_hit == PLAYER_NOT_HIT)
		) {
			player_is_hit = PLAYER_HIT;
		}
	}
}

// Advances every live laser's phase and renders it. stage_loop() calls this
// through the [lasers_update_and_render_func] slot.
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
		if(laser->charge_cel < LASER_CHARGE_CELS) {
			vram_top = (laser_origin.y - CHARGE_OFFSET);
			vram_top += scroll_line;
			if(vram_top >= RES_Y) {
				vram_top -= RES_Y;
			}
			x = (laser_origin.x - CHARGE_OFFSET);
			patnum = (PAT_LASER_CHARGE + laser->charge_cel);
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
