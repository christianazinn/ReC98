/// Stage 3 midboss - rendering
/// ---------------------------
/// TH05's Stage 3 midboss, and NOT a sibling of TH04's midboss3_render()
/// despite the shared name and despite th04/main/midboss/m3.cpp existing:
/// that one clips exclusively on all four edges, ignores the sprite extents
/// while doing it, and blits through midboss_put_generic(). This one has the
/// same single bottom-edge test that th05/main/midboss/m4.cpp has, picks its
/// cel from [midboss.sprite] rather than from a fixed base, and reaches
/// super_roll_put_1plane() directly.
///
/// The one it IS a sibling of is th05/main/midboss/m4.cpp, which shares its
/// phase dispatch, its clip, its coordinate pair and its damage branch
/// exactly. The two differences are here in the cel selection and in the
/// missing [boss_statebyte] gate on the ordinary blit.
///
/// (#included from th05/b34fg.cpp, ahead of
/// th05/main/boss/b3puppet_render.cpp. This function was the last `proc` of
/// th05_main.asm's MIDBOSSX_TEXT block once puppets_render() was lifted out
/// from under it, and that object is the segment's next contribution, so the
/// lift lands exactly where the root's block ended. kb/codegen 0112 + 0114.)

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/main/frames.h"
#include "th04/main/phase.hpp"
// th04/main/scroll.hpp and th04/main/midboss/midboss.hpp are NOT named
// here. Neither has an include guard and th05/main/midboss/m1_render.cpp,
// which compiles first in this object, already pulls both in; naming
// either again is a compile error rather than a no-op.

static const pixel_t MIDBOSS3_H = 64;

// [inferred] The one cel of this midboss's block that animates. Every other
// value of [midboss.sprite] is blitted as-is, which is the opposite of
// th05/main/midboss/m4.cpp, where the frame offset is added unconditionally.
// Spelled as the literal for the reason th05/main/boss/b4_solo_fg.cpp gives:
// where the cel is, is measured; what it depicts is not.
static const int PAT_MIDBOSS3_ANIMATED = 208;

static const int MIDBOSS3_FRAMES_PER_CEL = 8;

void pascal near midboss3_render(void)
{
	// Same frame and same declaration order as midboss4_render():
	// `ENTER 2, 0`, with [left] and [patnum] enregistered and [top] on the
	// stack. (kb/codegen 0010 + 0146)
	screen_x_t left;
	vram_y_t top;
	int patnum;

	if(midboss.phase < PHASE_EXPLODE_BIG) {
		// The only bound this renderer tests, exactly as in m4.cpp: no left,
		// right or top clip at all.
		if(midboss.pos.cur.y < 0) {
			return;
		}
		left = midboss.pos.cur.x.to_pixel();
		top = midboss.pos.cur.to_vram_top_scrolled_seg1(MIDBOSS3_H);
		if(midboss.sprite == PAT_MIDBOSS3_ANIMATED) {
			patnum = (
				PAT_MIDBOSS3_ANIMATED +
				(stage_frame_mod16 / MIDBOSS3_FRAMES_PER_CEL)
			);
		} else {
			patnum = midboss.sprite;
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
