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

// ZUN quirk: Hardcoded, unlike the top and left edges below, which do come out
// of the regular macros for a MIDBOSSX_W × MIDBOSSX_H sprite. It clips 8
// pixels earlier than playfield_clip_center_right_small() would, i.e. once the
// sprite's right edge reaches (PLAYFIELD_CLIP_RIGHT - GLYPH_HALF_W) rather
// than PLAYFIELD_CLIP_RIGHT.
static const subpixel_t MIDBOSSX_CLIP_RIGHT = TO_SP(392);
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
	// ZUN quirk: No bottom clip, unlike every other *_render() function that
	// uses these macros. The Extra Stage midboss never reaches the bottom of
	// the playfield, so the missing check is unobservable.
	if(
		playfield_clip_center_top_small_roll(midboss.pos.cur.y, MIDBOSSX_H) ||
		playfield_clip_center_left_small(midboss.pos.cur.x, MIDBOSSX_W) ||
		(midboss.pos.cur.x >= MIDBOSSX_CLIP_RIGHT)
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
// exactly like the original's. (kb/codegen/0080, 0069)
#pragma codestring "\x00"
#endif
// ---------
