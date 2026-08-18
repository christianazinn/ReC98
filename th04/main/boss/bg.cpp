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
#include "th04/main/null.hpp"
#include "th04/main/stage/stage.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/bosses.hpp"
#include "th04/main/boss/backdrop.hpp"
#include "th04/main/boss/impl.hpp"
#include "th04/main/tile/tile.hpp"
#include "th04/main/tile/bb.hpp"
#include "th04/sprites/main_cdg.h"

/// Extra Stage backdrop
/// --------------------
// Mugetsu and Gengetsu share one background, so they also share this one
// renderer; [boss_bg_render] is pointed at it by both stagex_setup() and the
// Gengetsu transition. Position of ST06BK.CDG within the playfield.
static const screen_x_t MUGETSU_GENGETSU_BACKDROP_LEFT = 32;
static const vram_y_t MUGETSU_GENGETSU_BACKDROP_TOP = 16;

// Same value as TH05's ENTRANCE_BB_FRAMES_PER_CEL
// (th05/main/boss/bosses.hpp), which TH04 has no header constant for because
// this is the only TH04 function that has needed it so far.
static const int ENTRANCE_BB_FRAMES_PER_CEL = 4;
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
