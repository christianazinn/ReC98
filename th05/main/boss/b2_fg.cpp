/// Stage 2 Boss - Louise, foreground rendering
/// -------------------------------------------
/// The simplest of TH05's boss renderers, and the baseline the other four in
/// this segment are variations on: one unconditional cel animation, the
/// three-way [boss_fg_render] contract that th04/main/boss/fg.cpp documents,
/// and nothing else. Like Alice's and the Stage 4 pair's, it does not reset
/// [boss.damage_this_frame] after the white flash.
///
/// Where alice_fg_render() picks between three animation rates and
/// b4_solo_fg_render() animates only two specific cels, this one adds the
/// frame offset to whatever [boss.sprite] happens to be.
///
/// (#included from th05/b34fg.cpp, at the front of that object. This function
/// was the last `proc` of th05_main.asm's MIDBOSSX_TEXT block once
/// midboss3_render() was lifted out from under it, and that object is the
/// segment's next contribution, so the lift lands exactly where the root's
/// block ended. kb/codegen 0112 + 0114.)

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/main/frames.h"
#include "th04/main/boss/boss.hpp"
// th05/main/boss/bosses.hpp, which declares louise_fg_render(), is NOT
// named here: it has no include guard and th05/main/boss/b1_fg.cpp is now
// the earliest file in this object that needs it.

static const int LOUISE_FRAMES_PER_CEL = 4;

void pascal near louise_fg_render(void)
{
	// NOT the frame alice_fg_render() has, even though the two are adjacent
	// in the original and take the same three locals: here [left] and [top]
	// are the enregistered pair and [patnum] is on the stack, where there it
	// is [left] and [patnum] in registers and [top] on the stack. The
	// declaration order below is what produces this one.
	// (kb/codegen 0010 + 0146)
	screen_x_t left;
	screen_y_t top;
	int patnum;

	left = boss.pos.cur.to_screen_left(BOSS_W);
	top = boss.pos.cur.to_screen_top(BOSS_H);

	if(boss.phase == PHASE_BOSS_EXPLODE_BIG) {
		super_large_put(left, top, boss.sprite);
	} else {
		patnum = (boss.sprite + (stage_frame_mod16 / LOUISE_FRAMES_PER_CEL));
		if(boss.damage_this_frame == 0) {
			super_put(left, top, patnum);
		} else {
			super_put_1plane(left, top, patnum, 0, super_plane(V_WHITE));
		}
	}
	explosions_small_update_and_render();
	explosions_big_update_and_render();
}
