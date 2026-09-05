/// Foreground rendering code for TH04's Stage 5 boss
/// -------------------------------------------------
/// (#included from th04/boss_5r.cpp. ZUN's object for this code segment held
/// yuuka5_fg_render() followed by kurumi_backdrop_colorfill(). That an original
/// object held several unrelated sources is kb/codegen/0112.
///
/// This used to add "the latter is hand-written ASM and stays in the dump".
/// It is not and it did not: it is two grcg_fill_playfield_rows_at() calls and
/// it lives in th04/main/boss/colorfill.cpp as of 2026-08-19.)
///
/// This is the third member of the [boss_fg_render] family, next to
/// kurumi_fg_render() and orange_fg_render() (th04/main/boss/render.cpp), and
/// keeps their three-way phase contract:
///
/// 1) during the two entrance phases, blit the boss sprite,
/// 2) during the fight, blit the animated boss sprite, flashed white on any
///    frame the boss took damage,
/// 3) during the big explosion, blit [boss.sprite] (which boss_defeat_update()
///    has by then set to PAT_ENEMY_KILL) at a fixed zoom level,
///
/// and then run the explosion animations regardless of phase. Five things
/// differ from the other two, all of them visible in the original code:
///
/// • The phases are tested in a different order, with PHASE_EXPLODE_BIG first.
///   The coordinates that branch uses are therefore the ones computed at the
///   top of the function – which are the *fight* coordinates, since ZUN wrote
///   the fight branch to reuse them as well. Every other branch recomputes
///   them from scratch.
/// • The explosion blit goes through super_zoom() at YUUKA5_EXPLODE_ZOOM,
///   rather than the fixed 2× of super_large_put().
/// • Both white-flash branches *do* reset [boss.damage_this_frame], the way
///   midboss_put_generic() (th04/main/midboss/midboss.hpp) does and the other
///   two boss renderers don't.
/// • There are no entrance circles. Instead, the fight is split across the
///   four states of Yuuka's warp animation, and two of those draw a circle.
/// • The function ends by rendering the thick lasers, which only one other
///   [boss_fg_render] does – yuuka6_fg_render(), C++ as of
///   MATCH-TH04-MAIN-012-HEAD-CARVE (th04/main/boss/b6_fg.cpp).
///   gengetsu_fg_render() in th04/main/boss/fg.cpp is the third.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/hardware/grcg.hpp"
#include "th02/v_colors.hpp"
#include "th04/hardware/grcg.hpp"
#include "th04/main/frames.h"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/bosses.hpp"
#include "th04/main/bullet/laser_t.hpp"
#include "th04/sprites/main_pat.h"

/// Stage 5 Boss - Yuuka
/// --------------------

// Yuuka's warp animation, cycled 0 → 1 → 2 → 3 → 0 by her update function.
// [verified] Every write to this variable lives in B4M_UPDATE_TEXT, in
// yuuka5_15ECE() and yuuka5_update() in th04/main/boss/b4m.cpp; the frame counts below are the
// [boss.phase_frame] thresholds those writes compare against, and each
// transition resets [boss.phase_frame] to 0.
//
// 	YUUKA5_WARP_NONE      the regular fight; no circle
// 	YUUKA5_WARP_OUT       32 frames; sprite plus a shrinking circle
// 	YUUKA5_WARP_TRAVELING 64 frames; only the circle, and the only state in
// 	                      which PlayfieldMotion actually moves Yuuka
// 	YUUKA5_WARP_IN         8 frames; sprite plus a growing circle
//
// Declared here rather than in a header because this is its only reader.
extern "C" unsigned char yuuka5_warp_phase;

static const unsigned char YUUKA5_WARP_NONE = 0;
static const unsigned char YUUKA5_WARP_OUT = 1;
static const unsigned char YUUKA5_WARP_TRAVELING = 2;
static const unsigned char YUUKA5_WARP_IN = 3;

// Yuuka's cels are absolute patnums, like Orange's. Unlike both other bosses
// though, yuuka5_fg_render() never animates through [boss.sprite] – it only
// reads that field for the PHASE_EXPLODE_BIG blit, and hardcodes every other
// patnum.
//
// [PAT_YUUKA5_STILL] is a single cel, shown during the entrance and during
// both halves of the warp. The fight instead cycles YUUKA5_FIGHT_CELS cels
// that are twice as wide, each blitted from two consecutive patnums.
static const int PAT_YUUKA5_STILL = PAT_STAGE;
static const int PAT_YUUKA5_FIGHT = (PAT_STAGE + 1);

static const int YUUKA5_FRAMES_PER_CEL = 4;
static const int YUUKA5_FIGHT_CELS = 4;
static const int YUUKA5_FIGHT_PATNUMS_PER_CEL = 2;

// [inferred] Yuuka's fight animation is 96×96, blitted as two 48-pixel-wide
// halves. As with Orange, the original only preserves the folded sum of
// PLAYFIELD_LEFT / PLAYFIELD_TOP and half the sprite extent, so the split
// between "sprite extent" and "extra offset" is not recoverable from the
// binary – but expressing it this way reuses the extents boss.hpp already
// documents, and correctly centers the sprite on the BOSS_W × BOSS_H box the
// hitbox and the still cel use.
static const pixel_t YUUKA5_FIGHT_W = 96;
static const pixel_t YUUKA5_FIGHT_H = 96;
static const pixel_t YUUKA5_FIGHT_HALF_W = (YUUKA5_FIGHT_W / 2);
static const pixel_t YUUKA5_FIGHT_OFFSET_X = ((BOSS_W - YUUKA5_FIGHT_W) / 2);
static const pixel_t YUUKA5_FIGHT_OFFSET_Y = ((BOSS_H - YUUKA5_FIGHT_H) / 2);

// Center of the warp circle, relative to the top-left corner of the
// BOSS_W × BOSS_H sprite box – i.e. exactly its center.
static const pixel_t YUUKA5_WARP_CIRCLE_CENTER_X = (BOSS_W / 2);
static const pixel_t YUUKA5_WARP_CIRCLE_CENTER_Y = (BOSS_H / 2);

// The warp-out circle starts out large enough to cover the entire sprite and
// shrinks onto it; the warp-in circle grows back out of the traveling one,
// but 4× as fast, since YUUKA5_WARP_IN only lasts 8 frames.
static const int YUUKA5_WARP_OUT_RADIUS_INITIAL = 80;
static const int YUUKA5_WARP_OUT_RADIUS_PER_FRAME = 2;
static const int YUUKA5_WARP_TRAVELING_RADIUS = 16;
static const int YUUKA5_WARP_IN_RADIUS_INITIAL = 16;
static const int YUUKA5_WARP_IN_RADIUS_PER_FRAME = 8;

// Magnification factor for the defeat explosion, in whole pixels per source
// pixel. super_large_put(), used by the other two bosses, is hardcoded to 2.
static const int YUUKA5_EXPLODE_ZOOM = 3;

void pascal near yuuka5_fg_render(void)
{
	// The function's single stack slot, [bp-2]. Doubles as the warp circle
	// radius, exactly as in kurumi_fg_render() and orange_fg_render(); ZUN
	// reuses the one slot, so splitting this into two locals would grow the
	// frame. (kb/codegen/0010)
	int patnum;

	// The two register variables.
	screen_x_t left;
	vram_y_t top;

	// The fight coordinates, which the PHASE_EXPLODE_BIG branch also uses.
	left = (boss.pos.cur.to_screen_left(BOSS_W) + YUUKA5_FIGHT_OFFSET_X);
	top = (boss.pos.cur.to_screen_top(BOSS_H) + YUUKA5_FIGHT_OFFSET_Y);

	if(boss.phase == PHASE_EXPLODE_BIG) {
		super_zoom(left, top, boss.sprite, YUUKA5_EXPLODE_ZOOM);

	// Yuuka renders her entrance animation for the whole of the HP fill and
	// .BB entrance phases, i.e. everything before the first attack pattern.
	// The `<=` spelling is load-bearing: the original compares against
	// PHASE_BOSS_ENTRANCE_BB and branches on `JA`, whereas the
	// `< (PHASE_BOSS_ENTRANCE_BB + 1)` that render.cpp uses for the other two
	// bosses would compare against PHASE_BOSS_ENTRANCE_BB + 1 and branch on
	// `JNB`. Turbo C++ 4.0J does not canonicalize between the two.
	// (kb/codegen/0095)
	} else if(boss.phase <= PHASE_BOSS_ENTRANCE_BB) {
		left = boss.pos.cur.to_screen_left(BOSS_W);
		top = boss.pos.cur.to_screen_top(BOSS_H);
		if(boss.damage_this_frame == 0) {
			super_put(left, top, PAT_YUUKA5_STILL);
		} else {
			super_put_1plane(
				left, top, PAT_YUUKA5_STILL, 0, super_plane(V_WHITE)
			);
			boss.damage_this_frame = 0;
		}
	} else if(boss.phase < PHASE_EXPLODE_BIG) {
		if(yuuka5_warp_phase == YUUKA5_WARP_NONE) {
			patnum = (
				((stage_frame_mod16 / YUUKA5_FRAMES_PER_CEL) *
					YUUKA5_FIGHT_PATNUMS_PER_CEL) +
				PAT_YUUKA5_FIGHT
			);
			if(boss.damage_this_frame == 0) {
				super_put(left, top, patnum);
				super_put(
					(left + YUUKA5_FIGHT_HALF_W), top, (patnum + 1)
				);
			} else {
				super_put_1plane(
					left, top, patnum, 0, super_plane(V_WHITE)
				);
				super_put_1plane(
					(left + YUUKA5_FIGHT_HALF_W), top, (patnum + 1), 0,
					super_plane(V_WHITE)
				);
				boss.damage_this_frame = 0;
			}
		} else if(yuuka5_warp_phase == YUUKA5_WARP_OUT) {
			left = boss.pos.cur.to_screen_left(BOSS_W);
			top = boss.pos.cur.to_screen_top(BOSS_H);
			grcg_setmode_rmw();
			grcg_setcolor_direct(V_WHITE);
			patnum = (
				YUUKA5_WARP_OUT_RADIUS_INITIAL -
				(boss.phase_frame * YUUKA5_WARP_OUT_RADIUS_PER_FRAME)
			);
			grcg_circlefill(
				(left + YUUKA5_WARP_CIRCLE_CENTER_X),
				(top + YUUKA5_WARP_CIRCLE_CENTER_Y),
				patnum
			);

			// ZUN inconsistency: The two branches that blit both a circle and
			// the sprite disagree on whether the sprite goes inside or outside
			// the GRCG block. It makes no difference – super_put() sets its
			// own GRCG state – but it does mean that only YUUKA5_WARP_IN can
			// share its grcg_off() with YUUKA5_WARP_TRAVELING's.
			grcg_off();
			super_put(left, top, PAT_YUUKA5_STILL);
		} else if(yuuka5_warp_phase == YUUKA5_WARP_TRAVELING) {
			// No sprite to blit here, so the circle's center is folded
			// straight into the coordinates.
			left = (
				boss.pos.cur.to_screen_left(BOSS_W) +
				YUUKA5_WARP_CIRCLE_CENTER_X
			);
			top = (
				boss.pos.cur.to_screen_top(BOSS_H) +
				YUUKA5_WARP_CIRCLE_CENTER_Y
			);
			grcg_setmode_rmw();
			grcg_setcolor_direct(V_WHITE);
			grcg_circlefill(left, top, YUUKA5_WARP_TRAVELING_RADIUS);
			grcg_off();
		} else if(yuuka5_warp_phase == YUUKA5_WARP_IN) {
			left = boss.pos.cur.to_screen_left(BOSS_W);
			top = boss.pos.cur.to_screen_top(BOSS_H);
			grcg_setmode_rmw();
			grcg_setcolor_direct(V_WHITE);
			patnum = (
				(boss.phase_frame * YUUKA5_WARP_IN_RADIUS_PER_FRAME) +
				YUUKA5_WARP_IN_RADIUS_INITIAL
			);
			grcg_circlefill(
				(left + YUUKA5_WARP_CIRCLE_CENTER_X),
				(top + YUUKA5_WARP_CIRCLE_CENTER_Y),
				patnum
			);
			super_put(left, top, PAT_YUUKA5_STILL);
			grcg_off();
		}
	}
	explosions_small_update_and_render();
	explosions_big_update_and_render();
	if(boss.phase < PHASE_NONE) {
		thicklasers_render();
	}
}
/// --------------------

// Turbo C++ pads this object out to an even length; the `db 0` that followed
// yuuka5_fg_render() in the dump is that pad, not data. (kb/codegen/0111;
// carve mechanics in 0080 and 0069)
#pragma codestring "\x00"
