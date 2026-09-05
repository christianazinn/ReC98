/// Stage 2 midboss - rendering
/// ---------------------------
/// th05/main/midboss/m3.cpp with a two-way cel selection instead of a
/// one-way one, and nothing else changed: the same phase dispatch, the same
/// single bottom-edge test, the same coordinate pair and the same damage
/// branch.
///
/// The two arms disagree on cel base *and* on rate, which is what makes this
/// a selection rather than an offset: one animates over four frames per cel
/// on [stage_frame_mod16], the other over two on [stage_frame_mod8].
///
/// (#included from th05/b34fg.cpp, behind th05/main/boss/b1_fg.cpp. This
/// function was the last `proc` of th05_main.asm's MIDBOSSX_TEXT block once
/// louise_fg_render() was lifted out from under it. kb/codegen 0112 + 0114.)

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/main/frames.h"
#include "th04/main/phase.hpp"
// th04/main/scroll.hpp and th04/main/midboss/midboss.hpp are NOT named
// here. Neither has an include guard and th05/main/midboss/m1_render.cpp,
// which compiles first in this object, already pulls both in; naming
// either again is a compile error rather than a no-op.

static const pixel_t MIDBOSS2_H = 64;

// [inferred] Two cel bases, one per arm. th05/sprites/main_pat.h has no
// Stage 2 section at all, so neither number has a name there and what they
// depict has not been decided -- only which [midboss.sprite] reaches which.
// Note that 208 means something else again in th05/main/midboss/m3.cpp and in
// th05/main/midboss/m1_render.cpp: these are per-stage sheets, so the same
// patnum is different art in each, which is exactly why none of them is
// spelled through a shared header.
static const int PAT_MIDBOSS2_ANIMATED = 202;
static const int PAT_MIDBOSS2_OTHER = 206;

static const int MIDBOSS2_ANIMATED_FRAMES_PER_CEL = 4;
static const int MIDBOSS2_OTHER_FRAMES_PER_CEL = 2;

void pascal near midboss2_render(void)
{
	screen_x_t left;
	vram_y_t top;
	int patnum;

	if(midboss.phase < PHASE_EXPLODE_BIG) {
		if(midboss.pos.cur.y < 0) {
			return;
		}
		left = midboss.pos.cur.x.to_pixel();
		top = midboss.pos.cur.to_vram_top_scrolled_seg1(MIDBOSS2_H);
		if(midboss.sprite == PAT_MIDBOSS2_ANIMATED) {
			patnum = (
				PAT_MIDBOSS2_ANIMATED +
				(stage_frame_mod16 / MIDBOSS2_ANIMATED_FRAMES_PER_CEL)
			);
		} else {
			patnum = (
				PAT_MIDBOSS2_OTHER +
				(stage_frame_mod8 / MIDBOSS2_OTHER_FRAMES_PER_CEL)
			);
		}
		if(midboss.damage_this_frame == 0) {
			super_roll_put(left, top, patnum);
		} else {
			super_roll_put_1plane(left, top, patnum, 0, super_plane(V_WHITE));
			midboss.damage_this_frame = 0;
		}
	} else if(midboss.phase == PHASE_EXPLODE_BIG) {
		midboss_defeat_render();
	}
}
