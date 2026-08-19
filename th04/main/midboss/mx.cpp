/// Extra Stage midboss
/// -------------------
/// TWO bodies. TH04's is a plain 4-cel idle animation with no white damage
/// flash and no bottom clip; TH05's is the same function as midboss5_render(),
/// down to the byte.

#pragma option -zPmain_01

#if (GAME != 5)
// TH04's original object for MIDBOSSX_TEXT opened with enemies_invalidate(),
// ahead of the three renderers below. th04_main.asm contributed it as the
// segment's only line, an assembler include of th04/main/enemy/inv.asm, and
// TLINK appends this object after that root contribution -- so the lift has to
// be the FIRST code this translation unit emits.
//
// It goes here rather than into th04/midbossx.cpp, above the `#include` of
// this file, because the `-zP` above cannot follow emitted code
// (kb/codegen/0138) and this file is shared: th05/midbossx.cpp includes it
// too, so honouring 0138 by moving the pragma into the wrapper would mean
// duplicating it into BOTH wrappers. Same kb/codegen/0129 reasoning the
// m1/m3 include below already gives for staying in one translation unit.
//
// TH05 still has the module: th05_main.asm's own `include` of it is not the
// last emitting item of its segment, so lifting that copy needs a carve.
#include "th04/main/enemy/inv.cpp"
#endif

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/main/phase.hpp"
#include "th04/main/scroll.hpp"
#include "th04/main/midboss/midboss.hpp"
#if (GAME != 5)
#include "th04/main/frames.h"
#include "th04/main/hud/hud.hpp"
#include "th04/sprites/main_pat.h"
#endif

// Constants
// ---------

#if (GAME == 5)
static const pixel_t MIDBOSSX_W = 64;
static const pixel_t MIDBOSSX_H = 64;
#else
static const pixel_t MIDBOSSX_W = 32;
static const pixel_t MIDBOSSX_H = 32;

// Number of frames each of the MIDBOSSX_CELS idle cels is shown for. Together
// they exactly fill the 16-frame [stage_frame_mod16] cycle.
static const int MIDBOSSX_FRAMES_PER_CEL = 4;

// ZUN bug: Clips 8 pixels too early at the right edge, removing the sprite
// while a slice of it is still inside the playfield.
//
// Hardcoded, unlike the top and left edges below, which do come out of the
// regular macros — playfield_clip_center_right_small() for a
// MIDBOSSX_W × MIDBOSSX_H sprite. That macro would give TO_SP(400), which is
// exactly the coordinate at which the sprite's left edge reaches
// PLAYFIELD_RIGHT — and it is also, to the subpixel, the right-hand despawn
// bound that midbossx_update() applies to the very same coordinate.
// So the sprite stops being drawn 8 pixels before it stops existing.
//
// [verified-by-emulator] Measured over a full Extra Stage encounter, from
// activation to midboss_reset(): pos.cur.x reaches this bound on exactly 8
// consecutive frames at 1 pixel per frame, never reaches TO_SP(400) on a frame
// that is still rendered, and midbossx_update() then despawns the midboss at
// TO_SP(400.94). Across those 8 frames the suppressed blit would have covered
// screen columns 408…415 down to 415…415, i.e. 8 down to 1 columns of a 32-row
// sprite inside PLAYFIELD_RIGHT (416).
//
// Measured twice and independently, agreeing on all 2593 frames: a DOSBox-X
// probe running ZUN's own ASM (ReC98 harness/PROBE-TH04-MIDBOSSX 459eff0b, two
// byte-identical runs reporting through oracle_diag()), and a from-the-dump
// host model. Parcel FIXLAYER-MIDBOSSX-CLIP-TAXONOMY; the run, its in-band
// control and the full table are in state/notes/_midbossx_render_qv.md, section
// "SETTLED", and in state/port/TH04_ORACLE_DELTA_INDEX.md F6.
//
// `ZUN bug` rather than `ZUN quirk` because the fix cannot desync a replay:
// th02/main/playfld.hpp says these macros are for rendering only, with
// gameplay-relevant clipping going through playfield_encloses*(), and
// midbossx_update()'s despawn test reads pos.cur.x directly rather than
// through this constant. Same defect and same label as the Stage 4 Marisa bits
// ("ZUN bug: Clipped at the right and bottom edges 16 pixels too early").
static const subpixel_t MIDBOSSX_CLIP_CENTER_RIGHT = TO_SP(392);
#endif
// ---------

// Rendering
// ---------

#if (GAME == 5)
void pascal near midbossx_render(void)
{
	if(midboss.phase < PHASE_EXPLODE_BIG) {
		if(playfield_clip_center_top_large_roll(
			midboss.pos.cur.y, MIDBOSSX_H
		)) {
			return;
		}
		screen_x_t left = midboss.pos.cur.to_screen_left(MIDBOSSX_W);
		vram_y_t top = midboss.pos.cur.to_vram_top_scrolled_seg1(MIDBOSSX_H);
		midboss_put_generic(left, top, midboss.sprite);
	} else if(midboss.phase == PHASE_BOSS_EXPLODE_BIG) {
		midboss_defeat_render();
	}
}
#else
// [measured] ZUN's object for this code segment also held the Stage 1 and the
// Stage 3 midboss renderer, immediately ahead of this one and in this order:
// midboss1_render at 0AAF:1C88+0x10D, midboss3_render at 0AAF:1D95+0xC5, this
// one at 0AAF:1E5A — each starting exactly where its predecessor ends. That an
// original object held several unrelated sources is kb/codegen/0112.
//
// They keep their own files, but not their own translation unit. The route is
// kb/codegen/0129, not 0112: every header they need is already included above,
// and 11 of the 13 in the closure have no include guard, so giving them a
// second TU means adding guards to 11 shared headers that other lanes want.
// (The two that already have one are libs/master.lib/pc98_gfx.hpp and
// th01/math/subpixel.hpp; 0129 said thirteen and none, and its ruling is
// unchanged by the correction — 11 is still not worth paying for one lift.)
#include "th04/main/midboss/m1.cpp"
#include "th04/main/midboss/m3.cpp"

void pascal near midbossx_render(void)
{
	// No bottom clip, unlike midboss4_render()'s
	// playfield_clip_point_yx_small_roll(); and the top check that *is* present
	// never fires either. No label is assigned, and that is the finding: the
	// whole vertical axis is inert for this sprite, so the omission is not an
	// asymmetry ZUN got wrong. `ZUN landmine` is excluded on measured evidence
	// — a landmine is unobservable because of a layout or TSR assumption that a
	// recompilation can break, and this is unreachable because of code, which
	// no rebuild can move. A mod that changed the trajectory would need the
	// clip; that is what this comment is for, not a taxonomy label. Settled by
	// parcel FIXLAYER-MIDBOSSX-CLIP-TAXONOMY; see
	// state/port/TH04_ORACLE_DELTA_INDEX.md F7.
	//
	// [measured, static simulation of th04_main.asm] [pos.cur.y] is derived as
	// polar(TO_SP(96), midboss.hp, Sin8(midboss.angle)) — not in
	// midbossx_update() itself, but in the two procs it calls, sub_146AF and
	// sub_14700. So it is confined to TO_SP(96) ± [hp] — and [hp] is 4096 only
	// for the ~125 frames the script takes to drain it to 128, and is capped at
	// 896 and 768 afterwards.
	//
	// [verified-by-emulator] Over a full encounter [pos.cur.y] stays inside
	// [TO_SP(-7.69), TO_SP(323.44)], against a missing bottom bound of
	// TO_SP(384) and the present top bound of TO_SP(-16). Neither is approached
	// within 60 and 8 pixels respectively — same run as the note above the
	// MIDBOSSX_CLIP_CENTER_RIGHT definition. The two markers are split on
	// purpose: the mechanism is read from the dump, the bounds are measured.
	if(
		playfield_clip_center_top_small_roll(midboss.pos.cur.y, MIDBOSSX_H) ||
		playfield_clip_center_left_small(midboss.pos.cur.x, MIDBOSSX_W) ||
		(midboss.pos.cur.x >= MIDBOSSX_CLIP_CENTER_RIGHT)
	) {
		return;
	}
	screen_x_t left = midboss.pos.cur.to_screen_left(MIDBOSSX_W);
	vram_y_t top = midboss.pos.cur.to_vram_top_scrolled_seg1(MIDBOSSX_H);
	int patnum = (PAT_MIDBOSSX + (stage_frame_mod16 / MIDBOSSX_FRAMES_PER_CEL));
	super_roll_put(left, top, patnum);
}

// The original's inter-object pad between this function and
// th04/main/bullet/pellet_r.asm. It has to be emitted here rather than left at
// the head of the reopened TILE_TEXT, because TASM's `even` directives further
// down that segment align against the *module-local* offset, which a reopened
// segment restarts at 0. Keeping this byte on this side keeps that offset even,
// exactly like the original's. (kb/codegen/0111; carve mechanics in 0080 and
// 0069)
#pragma codestring "\x00"
#endif
// ---------
