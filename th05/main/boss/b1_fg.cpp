/// Stage 1 Boss - Sara, foreground rendering
/// -----------------------------------------
/// The only TH05 boss renderer whose animation is driven by *which* cel is
/// showing rather than by a single rate: Sara's two wind-move cels step on
/// [stage_frame_mod8] and her stay cel on [stage_frame_mod16], both over
/// WIND_STAY_CELS cels, and every other cel is blitted as-is.
///
/// th05/sprites/main_pat.h already names all three, so unlike the other four
/// renderers in this segment this one needs no file-local patnum constants.
/// Its game-logic half is th05/main/boss/b1.cpp.
///
/// Like Louise's and Alice's, it does not reset [boss.damage_this_frame]
/// after the white flash.
///
/// (#included from th05/b34fg.cpp, behind th05/main/midboss/m1_render.cpp.
/// This function was the last `proc` of th05_main.asm's MIDBOSSX_TEXT block
/// once midboss2_render() was lifted out from under it. kb/codegen
/// 0112 + 0114.)

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/main/frames.h"
#include "th04/main/boss/boss.hpp"
#include "th05/sprites/main_pat.h"
#include "th05/main/boss/bosses.hpp"

void pascal near sara_fg_render(void)
{
	// alice_fg_render()'s frame, not louise_fg_render()'s: [left] and
	// [patnum] enregistered, [top] on the stack. (kb/codegen 0010 + 0146)
	screen_x_t left;
	int patnum;
	screen_y_t top;

	left = boss.pos.cur.to_screen_left(BOSS_W);
	top = boss.pos.cur.to_screen_top(BOSS_H);

	if(boss.phase == PHASE_BOSS_EXPLODE_BIG) {
		super_large_put(left, top, boss.sprite);
	} else {
		if(
			(boss.sprite == PAT_SARA_RIGHT) || (boss.sprite == PAT_SARA_LEFT)
		) {
			patnum = (boss.sprite + (stage_frame_mod8 / WIND_STAY_CELS));
		} else if(boss.sprite == PAT_SARA_STAY) {
			patnum = (boss.sprite + (stage_frame_mod16 / WIND_STAY_CELS));
		} else {
			patnum = boss.sprite;
		}
		if(boss.damage_this_frame == 0) {
			super_put(left, top, patnum);
		} else {
			super_put_1plane(left, top, patnum, 0, super_plane(V_WHITE));
		}
	}
	explosions_small_update_and_render();
	explosions_big_update_and_render();
}
