/// Background rendering code for TH04's bosses
/// -------------------------------------------
/// ZUN's object for this code segment held every one of TH04's boss
/// background renderers, in stage order, plus the two Stage 6 background
/// shape helpers wedged in between (kb/codegen/0112). Only the LAST of them,
/// the one shared by both Extra Stage bosses, is C++ so far; everything above
/// it is still th04_main.asm's `BOSS_BG_TEXT` contribution, which this object
/// is appended to. Later lifts extend this file upwards, one tail at a time.
///
/// TH05's counterpart is th05/main/boss/render.cpp, which holds that game's
/// five macro-shaped renderers and its two hand-rolled ones.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th03/formats/cdg.h"
#include "th04/hardware/grcg.hpp"
#include "th04/formats/bb.h"
#include "th04/math/randring.hpp"
#include "th04/main/checkerb.hpp"
#include "th04/main/null.hpp"
#include "th04/main/stage/stage.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/bosses.hpp"
#include "th04/main/boss/backdrop.hpp"
#include "th04/main/boss/impl.hpp"
#include "th04/main/boss/b6.cpp"
#include "th04/main/tile/tile.hpp"
#include "th04/main/tile/bb.hpp"
#include "th04/sprites/main_cdg.h"

/// Still ASM
/// ---------
// Fills the entire playfield with the current GRCG tile register, assuming
// TDW mode. th04_main.asm's main_013_TEXT, a GRCG_FILL_PLAYFIELD_ROWS pair
// with an ES:DI __usercall callee — the same hand-written shape as its
// neighbor playfield_fillm_0_40_384_274() (th04/main/player/bombchar.cpp).
// Unlike TH05's boss_bg_fill_col_0(), it neither enables nor disables the
// GRCG; both are the caller's job.
extern "C" void near playfield_fill(void);

// Yuuka's Phase 6 background: an 18-state machine that ramps palette color 0,
// re-seeds and re-aims all [bg_shapes] on every state change, then advances
// and blits each of them. Still th04_main.asm's sub_12461, and the placeholder
// name is deliberate — yuuka6_bg_render() is its only caller, so nothing about
// it has been graded, and naming it belongs to the parcel that lifts it.
extern "C" void near sub_12461(void);

// sub_12461()'s two state variables, reset here. [byte_2CDD0] is the state
// index (0…0x11, bounding two `cs:` jump tables), [byte_2CDD1] its per-state
// frame counter, which doubles as a triangle-wave fade ramp. Left under their
// dump names for the same reason as sub_12461() itself; both needed a
// zero-byte `label` alias in th04_main.asm to become linkable at all
// (kb/codegen/0123).
extern "C" unsigned char byte_2CDD0;
extern "C" unsigned char byte_2CDD1;
/// ---------

// Same value as TH05's ENTRANCE_BB_FRAMES_PER_CEL
// (th05/main/boss/bosses.hpp), which TH04 has no header constant for because
// these are the only TH04 functions that have needed it so far.
static const int ENTRANCE_BB_FRAMES_PER_CEL = 4;

/// Stage 6 — Yuuka
/// ---------------
// Nothing in Phase 6's background is made of tiles or of a backdrop image, so
// this one shares none of the impl.hpp macros' body — only their phase chain.
// Three structural differences are worth naming, because each is why a macro
// could not have been used:
//
// 1) grcg_setmode_tdw() is hoisted ABOVE the phase test and runs once for
//    every arm. The macros only ever set TDW inside their entrance branch.
// 2) There are FOUR arms, not five: the `>= PHASE_EXPLODE_BIG` arm covers
//    both PHASE_EXPLODE_BIG and PHASE_NONE, and there is no
//    tiles_render_after_custom() at all.
// 3) The last two arms share a trailing sub_12461() call, and the HP-fill arm
//    carries the one-shot shape re-seed. Neither has a macro slot.
void pascal near yuuka6_bg_render(void)
{
	grcg_setmode_tdw();
	if(boss.phase == PHASE_HP_FILL) {
		grcg_setcolor_direct(1);
		playfield_fill();
		grcg_off();

		// Frame 2 rather than frame 0: boss_reset() runs on frame 0, and the
		// HP fill is the only phase long enough for a one-shot to be safe
		// here. [inferred] — the binary only shows the comparison.
		if(boss.phase_frame == 2) {
			bg_shape_t near *shape = bg_shapes;
			int i;

			// kb/codegen/0003's near-pointer iterator, with one addition: the
			// two increments are emitted in SOURCE order, so `shape++` has to
			// stay in the for-increment expression after `i++`. Written as a
			// trailing statement in the body instead, it becomes `ADD SI, 6` /
			// `INC DI` — the same two instructions, swapped, and the only
			// thing that separated a 2/70 diff from IDENTICAL.
			for(i = 0; i < BG_SHAPE_COUNT; i++, shape++) {
				shape->pos.x.v = randring1_next16_mod(TO_SP(PLAYFIELD_W));
				shape->pos.y.v = randring1_next16_mod(TO_SP(PLAYFIELD_H));
				shape->angle = 0x60;
				shape->speed.v = TO_SP(1);
			}
			bg_shape_flyout_speed.v = TO_SP(1);

			// Not one of the PAT_* constants: 120 is below PAT_STAGE, in the
			// range th04/sprites/main_pat.h leaves unnamed.
			bg_shape_patnum = static_cast<main_patnum_t>(120);

			byte_2CDD0 = 0;
			byte_2CDD1 = 0;
		}
	} else if(boss.phase == PHASE_BOSS_ENTRANCE_BB) {
		unsigned char entrance_cel = (
			boss.phase_frame / ENTRANCE_BB_FRAMES_PER_CEL
		);
		grcg_setcolor_direct(1);
		if(entrance_cel < (TILES_BB_CELS / 2)) {
			playfield_fill();
		} else {
			playfield_checkerboard_grcg_tdw_update_and_render();
		}
		tiles_bb_put(bb_boss_seg, entrance_cel);
	} else {
		if(boss.phase < PHASE_EXPLODE_BIG) {
			playfield_checkerboard_grcg_tdw_update_and_render();
		} else {
			grcg_setcolor_direct(1);
			playfield_fill();
			grcg_off();
		}
		sub_12461();
	}
}
/// ---------------

/// Extra Stage backdrop
/// --------------------
// Mugetsu and Gengetsu share one background, so they also share this one
// renderer; [boss_bg_render] is pointed at it by both stagex_setup() and the
// Gengetsu transition. Position of ST06BK.CDG within the playfield.
static const screen_x_t MUGETSU_GENGETSU_BACKDROP_LEFT = 32;
static const vram_y_t MUGETSU_GENGETSU_BACKDROP_TOP = 16;
/// --------------------

// This is boss_bg_render_entrance_bb_opaque_and_backdrop()
// (th04/main/boss/impl.hpp) with three documented deviations, written out
// because none of the three can be expressed through that macro:
//
// 1) The macro's [bb_col] parameter emits `tiles_bb_col = bb_col;` before the
//    .BB blit. NO TH04 boss background renderer does that — TH04 sets
//    [tiles_bb_col] once at boss setup time instead, and TH05 is the game
//    that moved the write into the renderer. So the assignment is absent
//    here, and the macro cannot express its absence.
// 2) The macro spells the phase constant `PHASE_BOSS_EXPLODE_BIG`, which
//    th04/main/phase.hpp only defines under `#if (GAME == 5)`. TH04's name
//    for the same value is `PHASE_EXPLODE_BIG`.
// 3) The HP-fill arm is not one statement. It disables the stage's own
//    renderer on the same frames it redraws every tile, and shares its
//    tiles_render_all() call with the PHASE_EXPLODE_BIG arm below — a single
//    `boss.phase_frame <= 2` test, not the two that
//    tiles_render_after_custom() plus a separate `if` would produce.
//
// Everything else — the phase chain and its order, the `unsigned char` cel
// counter, the `< (TILES_BB_CELS / 2)` half-animation split, and the inlined
// copy of boss_backdrop_render() with its arguments in the opposite order —
// is the macro, unchanged.
void pascal near mugetsu_gengetsu_bg_render(void)
{
	if(boss.phase == PHASE_HP_FILL) {
		// The Extra Stage has no scrolling background of its own, so the
		// stage renderer is retired for good as soon as the boss appears.
		if(boss.phase_frame <= 2) {
			stage_render = nullfunc_near;
			tiles_render_all();
		} else {
			tiles_render();
		}
	} else if(boss.phase == PHASE_BOSS_ENTRANCE_BB) {
		unsigned char entrance_cel = (
			boss.phase_frame / ENTRANCE_BB_FRAMES_PER_CEL
		);
		if(entrance_cel < (TILES_BB_CELS / 2)) {
			tiles_render_all();
		} else {
			// A copy of boss_backdrop_render()…
			grcg_setmode_tdw();
			grcg_setcolor_direct(1);
			// … that probably predated [boss_backdrop_colorfill]?
			mugetsu_gengetsu_backdrop_colorfill();
			grcg_off();

			cdg_put_noalpha_8(
				MUGETSU_GENGETSU_BACKDROP_LEFT,
				MUGETSU_GENGETSU_BACKDROP_TOP,
				CDG_BG_BOSS
			);
		}
		tiles_bb_put(bb_boss_seg, entrance_cel);
	} else if(boss.phase < PHASE_EXPLODE_BIG) {
		boss_backdrop_render(
			MUGETSU_GENGETSU_BACKDROP_LEFT,
			MUGETSU_GENGETSU_BACKDROP_TOP,
			1
		);
	} else if(boss.phase == PHASE_EXPLODE_BIG) {
		tiles_render_all();
	} else /* if(boss.phase == PHASE_NONE) */ {
		tiles_render_after_custom(boss.phase_frame);
	}
}
