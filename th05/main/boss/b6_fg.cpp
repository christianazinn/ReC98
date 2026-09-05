/// Stage 6 Boss - Shinki, foreground rendering
/// -------------------------------------------
/// (#included from th05/stages.cpp, ahead of th05/main/stage/stages.cpp. This
/// function was the last `proc` of th05_main.asm's MIDBOSSX_TEXT block — a
/// harness carve of the head of the original main_0_TEXT, see the comment
/// above that segment in the dump — and th05/stages.cpp is the segment's next
/// contribution, so the lift lands exactly where the root's block ended and
/// every byte above it keeps its address. kb/codegen 0112 + 0114.)
///
/// Shinki keeps the three-way [boss_fg_render] contract that
/// th04/main/boss/fg.cpp documents for TH04's bosses, and is the one boss in
/// either game whose sprite changes SIZE mid-fight: her winged form is a
/// 256×96 image that master.lib's BFNT limit forced ZUN to split into
/// bfnt_parts_x(SHINKI_WING_W) physical sprites, blitted left to right in the
/// loop below. Which form is on screen is read off [boss.sprite] alone —
/// every patnum from PAT_SHINKI_WINGS_WHITE up is a wing part, everything
/// below it is one of the four 64×64 poses.
///
/// The damage flash is therefore also two different mechanisms: the 64×64
/// poses get the usual white single-plane blit, while the winged form
/// switches to the parallel *_HIT cel set, which is exactly
/// bfnt_parts_x(SHINKI_WING_W) patnums further along.

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/main/boss/boss.hpp"
#include "th05/sprites/main_pat.h"

void pascal near shinki_fg_render(void)
{
	// The original's frame is `ENTER 4, 0`: two enregistered locals, then
	// [bp-2] and [bp-4]. [left] and [patnum] are mentioned equally often, so
	// declaration order alone decides the pair, and it runs SI then DI here —
	// measured by swapping exactly these two declarations, which moved
	// nothing else in the body. (kb/codegen 0010 + 0146)
	screen_x_t left;
	int patnum;
	screen_y_t top;
	int i;

	// Computed once, ahead of the phase branch, the way exalice_fg_render()
	// (th05/main/boss/bx_fg.cpp) does it and TH04's renderers do not. Both
	// offsets fall out to nothing and to -16 respectively, which is why the
	// original has a bare `SAR AX, 4` on the X axis.
	left = boss.pos.cur.to_screen_left(BOSS_W);
	top = boss.pos.cur.to_screen_top(BOSS_H);

	if(boss.phase == PHASE_BOSS_EXPLODE_BIG) {
		super_large_put(left, top, boss.sprite);
	} else {
		patnum = boss.sprite;
		if(patnum < PAT_SHINKI_WINGS_WHITE) {
			if(boss.damage_this_frame == 0) {
				// [patnum] is pushed out of AX, where the assignment above
				// left it, rather than out of DI — this is the branch the
				// compiler reaches in a straight line. (kb/codegen/0126)
				super_put(left, top, patnum);
			} else {
				super_put_1plane(
					left, top, patnum, 0, super_plane(V_WHITE)
				);
			}
		} else {
			// Re-centering the wide image on the position the 64×64 poses
			// were centered on, by moving the top-left corner rather than by
			// recomputing it from [boss.pos].
			left -= ((SHINKI_WING_W / 2) - (BOSS_W / 2));
			top -= ((SHINKI_WING_H / 2) - (BOSS_H / 2));

			if(boss.damage_this_frame != 0) {
				patnum += bfnt_parts_x(SHINKI_WING_W);
			}
			// All three steps in the `for`'s own increment clause, in this
			// order: the original advances [i] before the other two, which is
			// only what TCC emits if none of them is a statement in the body.
			for(i = 0; i < bfnt_parts_x(SHINKI_WING_W); (
				i++,
				left += (SHINKI_WING_W / bfnt_parts_x(SHINKI_WING_W)),
				patnum++
			)) {
				super_put_8(left, top, patnum);
			}
		}
		boss.damage_this_frame = 0;
	}
	explosions_small_update_and_render();
	explosions_big_update_and_render();
}
/// -------------------------------------------
