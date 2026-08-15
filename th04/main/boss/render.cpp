/// Foreground rendering code for TH04's bosses
/// -------------------------------------------
/// (#included from th04/mb_dfr.cpp, ahead of
/// th04/main/midboss/defeat_render.cpp. ZUN's object for this code segment
/// held the Stage 1 and Stage 2 boss foreground renderers and the midboss
/// defeat animation, in exactly that address order — that an original object
/// held several unrelated sources is kb/codegen/0112.)
///
/// Both functions have the same three-way shape, which is the [boss_fg_render]
/// contract for TH04:
///
/// 1) during the two entrance phases, blit the animated boss sprite, plus —
///    only while the HP bar is still filling — three concentric circles that
///    shrink onto the boss,
/// 2) during the fight, blit the animated boss sprite, flashed white on any
///    frame the boss took damage,
/// 3) during the big explosion, blit [boss.sprite] (which boss_defeat_update()
///    has by then set to PAT_ENEMY_KILL) through super_large_put(),
///
/// and then run the explosion animations regardless of phase.
///
/// Neither function resets [boss.damage_this_frame] after the white flash,
/// unlike midboss_put_generic() (th04/main/midboss/midboss.hpp) — which is why
/// that macro is not reused here.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/hardware/grcg.hpp"
#include "th02/v_colors.hpp"
#include "th04/hardware/grcg.hpp"
#include "th04/main/frames.h"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/bosses.hpp"
#include "th04/sprites/main_pat.h"
#include "th04/main/boss/b2.cpp"

/// Shared
/// ------

// Both bosses render their entrance animation for the whole of the HP fill
// and .BB entrance phases, i.e. everything before the first attack pattern.
static const unsigned char PHASE_AFTER_ENTRANCE = (PHASE_BOSS_ENTRANCE_BB + 1);

// The two entrance circles both grow their radius by 2 pixels per frame the
// entrance is *closer* to its end, so they visually shrink onto the boss.
static const int BOSS_ENTRANCE_CIRCLE_RADIUS_PER_FRAME = 2;

// Distance between the three concentric entrance circles.
static const int BOSS_ENTRANCE_CIRCLE_SPACING = 6;
/// ------

/// Stage 2 Boss - Kurumi
/// ---------------------

// Kurumi's cels are addressed relative to the start of the per-stage sprite
// range, unlike Orange's — stage2_setup() seeds [boss.sprite] with 0, while
// stage1_setup() seeds it with PAT_STAGE.
static const int KURUMI_FRAMES_PER_CEL = 4;

// The entrance circles are only shown during the second half of the HP fill
// phase, and have shrunk to nothing by KURUMI_ENTRANCE_CIRCLE_LAST_FRAME.
static const int KURUMI_ENTRANCE_CIRCLE_FIRST_FRAME = 128;
static const int KURUMI_ENTRANCE_CIRCLE_LAST_FRAME = 320;

// Center of the entrance circles, relative to the top-left corner of the
// BOSS_W × BOSS_H sprite box. Horizontally centered, 8 pixels above the
// vertical center.
static const pixel_t KURUMI_ENTRANCE_CIRCLE_CENTER_X = 32;
static const pixel_t KURUMI_ENTRANCE_CIRCLE_CENTER_Y = 24;

void pascal near kurumi_fg_render(void)
{
	// Declared in this order to reproduce the original's stack slots, which
	// run [bp-2] … [bp-0Ah] in declaration order. (kb/codegen/0010)
	// [patnum] doubles as the entrance circle radius; ZUN reuses the one
	// slot, so this can't be split into two locals without growing the frame.
	int patnum;
	int i;
	screen_x_t ray_origin_left;
	vram_y_t ray_origin_top;
	kurumi_spawnray_t near *ray;

	// The two register variables. [left] and [top] are reassigned inside the
	// spawn ray loop below, which is why they can't be recomputed per call.
	screen_x_t left;
	vram_y_t top;

	if(boss.phase < PHASE_AFTER_ENTRANCE) {
		left = boss.pos.cur.to_screen_left(BOSS_W);
		top = boss.pos.cur.to_screen_top(BOSS_H);
		patnum = (
			boss.sprite + (stage_frame_mod16 / KURUMI_FRAMES_PER_CEL) +
			PAT_KURUMI
		);
		super_put(left, top, patnum);
		if(
			(boss.phase == PHASE_HP_FILL) &&
			(boss.phase_frame > KURUMI_ENTRANCE_CIRCLE_FIRST_FRAME)
		) {
			patnum = (
				(KURUMI_ENTRANCE_CIRCLE_LAST_FRAME - boss.phase_frame) *
				BOSS_ENTRANCE_CIRCLE_RADIUS_PER_FRAME
			);
			left += KURUMI_ENTRANCE_CIRCLE_CENTER_X;
			top += KURUMI_ENTRANCE_CIRCLE_CENTER_Y;
			grcg_setmode_rmw();
			grcg_setcolor_direct(7);
			grcg_circle(left, top, patnum);
			grcg_setcolor_direct(6);
			grcg_circle(left, top, (patnum + BOSS_ENTRANCE_CIRCLE_SPACING));
			grcg_circle(
				left, top, (patnum + (BOSS_ENTRANCE_CIRCLE_SPACING * 2))
			);
			grcg_off();
		}
	} else if(boss.phase < PHASE_EXPLODE_BIG) {
		left = boss.pos.cur.to_screen_left(BOSS_W);
		top = boss.pos.cur.to_screen_top(BOSS_H);
		patnum = (boss.sprite + PAT_KURUMI);

		// Only some of Kurumi's cels carry a 4-cel sub-animation, and the two
		// groups run at different speeds.
		if((boss.sprite == 0) || (boss.sprite == 12)) {
			patnum += (stage_frame_mod16 / KURUMI_FRAMES_PER_CEL);
		}
		if((boss.sprite == 4) || (boss.sprite == 6)) {
			patnum += (stage_frame_mod8 / KURUMI_FRAMES_PER_CEL);
		}

		if(boss.damage_this_frame == 0) {
			super_put(left, top, patnum);
		} else {
			super_put_1plane(left, top, patnum, 0, super_plane(V_WHITE));
		}

		grcg_setmode_rmw();
		grcg_setcolor_direct(9);
		ray = kurumi_spawnrays;
		for(i = 0; i < KURUMI_SPAWNRAY_COUNT; i++, ray++) {
			if(ray->flag == B2SF_FREE) {
				continue;
			}
			// Divisions, not TO_PIXEL() shifts: the original really does
			// emit `MOV BX, SUBPIXEL_FACTOR` / `CWD` / `IDIV BX` at all four
			// sites, which a `>>` cannot produce. The boss position above is
			// the opposite case, and shifts.
			left = ((ray->target.x / SUBPIXEL_FACTOR) + PLAYFIELD_LEFT);
			top = ((ray->target.y / SUBPIXEL_FACTOR) + PLAYFIELD_TOP);
			ray_origin_left = (
				(ray->origin.x / SUBPIXEL_FACTOR) + PLAYFIELD_LEFT
			);
			ray_origin_top = (
				(ray->origin.y / SUBPIXEL_FACTOR) + PLAYFIELD_TOP
			);
			grcg_line(left, top, ray_origin_left, ray_origin_top);
		}
	} else if(boss.phase == PHASE_EXPLODE_BIG) {
		left = boss.pos.cur.to_screen_left(BOSS_W);
		top = boss.pos.cur.to_screen_top(BOSS_H);
		super_large_put(left, top, boss.sprite);
	}
	explosions_small_update_and_render();
	explosions_big_update_and_render();
}
/// ---------------------

/// Stage 1 Boss - Orange
/// ---------------------

// Orange's [boss.sprite] is an absolute patnum: stage1_setup() seeds it with
// PAT_STAGE.
static const int ORANGE_FRAMES_PER_CEL = 4;

// The entrance circles are only shown during the last quarter of the HP fill
// phase, and have shrunk to nothing by ORANGE_ENTRANCE_CIRCLE_LAST_FRAME.
static const int ORANGE_ENTRANCE_CIRCLE_FIRST_FRAME = 192;
static const int ORANGE_ENTRANCE_CIRCLE_LAST_FRAME = 352;

// Center of the entrance circles, relative to the top-left corner of the
// sprite box used during the entrance.
static const pixel_t ORANGE_ENTRANCE_CIRCLE_CENTER_X = 24;
static const pixel_t ORANGE_ENTRANCE_CIRCLE_CENTER_Y = 8;

// [inferred] Orange is blitted from a different corner in each of the three
// phases, and the original only preserves the folded sum of PLAYFIELD_LEFT /
// PLAYFIELD_TOP and half the sprite extent. Expressing that offset relative to
// the BOSS_W × BOSS_H box reuses the extents boss.hpp already documents, but
// the split between "sprite extent" and "extra offset" is not recoverable from
// the binary.
static const pixel_t ORANGE_ENTRANCE_OFFSET_X = 16;
static const pixel_t ORANGE_ENTRANCE_OFFSET_Y = 8;
static const pixel_t ORANGE_FIGHT_OFFSET_Y = -8;

void pascal near orange_fg_render(void)
{
	// Doubles as the entrance circle radius, exactly as in kurumi_fg_render().
	int patnum;

	screen_x_t left;
	vram_y_t top;

	if(boss.phase < PHASE_AFTER_ENTRANCE) {
		left = (
			boss.pos.cur.to_screen_left(BOSS_W) +
			ORANGE_ENTRANCE_OFFSET_X
		);
		top = (
			boss.pos.cur.to_screen_top(BOSS_H) +
			ORANGE_ENTRANCE_OFFSET_Y
		);
		patnum = (boss.sprite + (stage_frame_mod8 / ORANGE_FRAMES_PER_CEL));
		super_put(left, top, patnum);
		if(
			(boss.phase == PHASE_HP_FILL) &&
			(boss.phase_frame >= ORANGE_ENTRANCE_CIRCLE_FIRST_FRAME)
		) {
			patnum = (
				(ORANGE_ENTRANCE_CIRCLE_LAST_FRAME - boss.phase_frame) *
				BOSS_ENTRANCE_CIRCLE_RADIUS_PER_FRAME
			);
			left += ORANGE_ENTRANCE_CIRCLE_CENTER_X;
			top += ORANGE_ENTRANCE_CIRCLE_CENTER_Y;
			grcg_setmode_rmw();
			grcg_setcolor_direct(V_WHITE);
			grcg_circle(left, top, patnum);
			grcg_setcolor_direct(9);
			grcg_circle(left, top, (patnum + BOSS_ENTRANCE_CIRCLE_SPACING));
			grcg_circle(
				left, top, (patnum + (BOSS_ENTRANCE_CIRCLE_SPACING * 2))
			);
			grcg_off();
		}
	} else if(boss.phase < PHASE_EXPLODE_BIG) {
		left = boss.pos.cur.to_screen_left(BOSS_W);
		top = (
			boss.pos.cur.to_screen_top(BOSS_H) +
			ORANGE_FIGHT_OFFSET_Y
		);
		patnum = (boss.sprite + (stage_frame_mod16 / ORANGE_FRAMES_PER_CEL));
		if(boss.damage_this_frame == 0) {
			super_put(left, top, patnum);
		} else {
			super_put_1plane(left, top, patnum, 0, super_plane(V_WHITE));
		}
	} else if(boss.phase == PHASE_EXPLODE_BIG) {
		left = boss.pos.cur.to_screen_left(BOSS_W);
		top = boss.pos.cur.to_screen_top(BOSS_H);
		super_large_put(left, top, boss.sprite);
	}
	explosions_small_update_and_render();
	explosions_big_update_and_render();
}
/// ---------------------
