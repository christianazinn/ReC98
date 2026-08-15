/// Extra Stage midboss
/// -------------------
/// TWO bodies. TH04's is a plain 4-cel idle animation with no white damage
/// flash and no bottom clip; TH05's is the same function as midboss5_render(),
/// down to the byte.

#pragma option -zPmain_01

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
// [measured, static simulation of th04_main.asm] Derived over a full Extra
// Stage encounter from stagex_setup()'s constants: pos.cur.x reaches this bound
// on exactly 8 consecutive frames at 1 pixel per frame, never reaches TO_SP(400)
// on a frame that is still rendered, and midbossx_update() then despawns the
// midboss at TO_SP(400.94). Across those 8 frames the suppressed blit would have
// covered screen columns 408…415 down to 415…415, i.e. 8 down to 1 columns of a
// 32-row sprite inside PLAYFIELD_RIGHT (416).
//
// The numbers were reproduced independently by naming review round 5, but no
// emulator run exists for them: nothing on disk records one, and every quantity
// above is derivable from th04_main.asm alone. Marked accordingly rather than
// `[verified-by-emulator]`, which is what this comment claimed before.
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
void pascal near midbossx_render(void)
{
	// No bottom clip, unlike midboss4_render()'s
	// playfield_clip_point_yx_small_roll(); and the top check that *is* present
	// never fires either. Left unlabelled here, but the disposition is open —
	// see state/port/FIX_LAYER_CANDIDATES.md J2. "Dead condition rather than a
	// defect" was the previous wording and is a false dichotomy: the missing
	// bottom check is a candidate `ZUN landmine` (harmless only because of
	// stagex_setup()'s specific hp/angle constants), and the never-firing top
	// check is a candidate `ZUN bloat` (CONTRIBUTING.md lists "code without any
	// effect" first among its examples). The taxonomy lane owns that call.
	//
	// [measured, static simulation of th04_main.asm] [pos.cur.y] is derived as
	// polar(TO_SP(96), midboss.hp, Sin8(midboss.angle)) — not in
	// midbossx_update() itself, but in its callees sub_146AF
	// (th04_main.asm:10461) and sub_14700 (:10498). So it is confined to
	// TO_SP(96) ± [hp] — and [hp] is 4096 only for the ~125 frames the script
	// takes to drain it to 128, and is capped at 896 and 768 afterwards. Over
	// a full encounter [pos.cur.y] stays inside [TO_SP(-7.69), TO_SP(323.44)],
	// against a missing bottom bound of TO_SP(384) and the present top bound
	// of TO_SP(-16). Neither is approached within 60 and 8 pixels
	// respectively. No emulator run backs this; see the note above the
	// MIDBOSSX_CLIP_CENTER_RIGHT definition.
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
