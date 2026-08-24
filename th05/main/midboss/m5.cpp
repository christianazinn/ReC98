/// Stage 5 midboss
/// ---------------

// The `-zCmain_0_TEXT` pragma this file used to carry now lives in
// th05/midboss5.cpp, which compiles this file together with EX-Alice's
// foreground renderer -- and a pragma only takes effect before any code is
// generated, so it has to precede both. (kb/codegen/0112, trap 0)

#include "libs/master.lib/pc98_gfx.hpp"
// th02/v_colors.hpp is likewise unguarded and likewise already supplied by
// th05/main/boss/bx_fg.cpp above.
// th04/main/phase.hpp is NOT included here: it has no include guard, and
// th04/main/boss/boss.hpp already supplies it to this object through
// th05/main/boss/bx_fg.cpp above. Naming it again is a compile error.
#include "th04/main/scroll.hpp"
#include "th04/main/midboss/midboss.hpp"

// Constants
// ---------

static const pixel_t MIDBOSS5_W = 64;
static const pixel_t MIDBOSS5_H = 64;
// ---------

// Rendering
// ---------

void pascal near midboss5_render(void)
{
	if(midboss.phase < PHASE_EXPLODE_BIG) {
		if(playfield_clip_center_top_large_roll(
			midboss.pos.cur.y, MIDBOSS5_H
		)) {
			return;
		}
		screen_x_t left = midboss.pos.cur.to_screen_left(MIDBOSS5_W);
		vram_y_t top = midboss.pos.cur.to_vram_top_scrolled_seg1(MIDBOSS5_H);
		midboss_put_generic(left, top, midboss.sprite);
	} else if(midboss.phase == PHASE_BOSS_EXPLODE_BIG) {
		midboss_defeat_render();
	}
}
// ---------
