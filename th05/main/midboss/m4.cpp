/// Stage 4 midboss - rendering
/// ---------------------------
/// TH05's Stage 4 midboss, and NOT a sibling of TH04's midboss4_render()
/// despite the shared name: that one clips through
/// playfield_clip_point_yx_small_roll() and picks its cel from which half of
/// the playfield the midboss is in, where this one has a single bottom-edge
/// test and animates on [stage_frame_mod16]. Nothing about the two bodies is
/// shared, so they stay separate files.
///
/// (#included from th05/b6cbull.cpp, ahead of
/// th05/main/bullet/b4balls_render.cpp. This function was the last `proc` of
/// th05_main.asm's MIDBOSSX_TEXT block once that module was lifted out from
/// under it, and this object is the segment's next contribution, so the lift
/// lands exactly where the root's block ended. kb/codegen 0112 + 0114.)

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/main/frames.h"
#include "th04/main/scroll.hpp"
#include "th04/main/phase.hpp"
#include "th04/main/midboss/midboss.hpp"
#include "th04/main/boss/boss.hpp"

static const pixel_t MIDBOSS4_H = 64;

static const int MIDBOSS4_FRAMES_PER_CEL = 4;

// Gates the ordinary blit: a zero here draws the midboss in solid white on
// every frame, the same single-plane blit the damage flash uses, and resets
// [damage_this_frame] with it. th05/main/midboss/m4_updt.cpp is the state
// machine that writes it, and names the same slot `midboss4_solid` for what it
// gates there -- the midboss takes no damage while this is zero either, so the
// white silhouette and the invulnerability are one state, not two.
#define midboss4_blit_normally boss_statebyte[15]

void pascal near midboss4_render(void)
{
	screen_x_t left;
	vram_y_t top;
	int patnum;

	if(midboss.phase < PHASE_EXPLODE_BIG) {
		// The only bound this renderer tests. Unlike TH04's Stage 4 midboss
		// there is no left, right or top clip at all.
		if(midboss.pos.cur.y < 0) {
			return;
		}
		left = midboss.pos.cur.x.to_pixel();
		top = midboss.pos.cur.to_vram_top_scrolled_seg1(MIDBOSS4_H);
		patnum = (
			midboss.sprite + (stage_frame_mod16 / MIDBOSS4_FRAMES_PER_CEL)
		);
		if((midboss.damage_this_frame == 0) && midboss4_blit_normally) {
			super_roll_put(left, top, patnum);
		} else {
			super_roll_put_1plane(left, top, patnum, 0, super_plane(V_WHITE));
			midboss.damage_this_frame = 0;
		}
	} else if(midboss.phase == PHASE_EXPLODE_BIG) {
		midboss_defeat_render();
	}
}

#undef midboss4_blit_normally
