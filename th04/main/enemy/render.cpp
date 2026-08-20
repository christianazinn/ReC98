/// Enemy rendering
/// ---------------
/// (#included from th04/main_.cpp, ahead of th04/main/player/invalidate.cpp
/// and therefore in its original address order. Both were `include`s at the
/// tail of th04_main.asm's main__TEXT block, so the new object lands exactly
/// where they ended and every byte above them keeps its address
/// (kb/codegen 0112 + 0114).
///
/// th04/main/enemy/render.asm is NOT deleted: th05_main.asm still includes
/// it, as the tail of its own MAIN_TEXT block. Lifting the TH05 half is a
/// separate parcel that only has to delete that `include` and give TH05's
/// segment a host; this body is already shared.)
///
/// Every live enemy is blitted with its animation cel resolved from [age],
/// and flashed white on any frame it took damage. Clipped twice: once on the
/// PREVIOUS Y position before any work is done, and once on the current
/// position in both axes right before the blit.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/main/enemy/enemy.hpp"
#include "th04/main/enemy/size.hpp"

extern "C" void pascal near enemies_render(void)
{
	// Declaration order is the frame layout: [bp-2], [bp-4], [bp-5].
	// (kb/codegen/0010)
	int i;
	vram_y_t top;
	unsigned char patnum;

	// The two register variables, SI and DI, in that order: [enemy] is
	// mentioned more often than [left]. (kb/codegen/0146)
	register enemy_t near *enemy;
	register screen_x_t left;

	enemy = enemies;
	for(i = 0; i < ENEMY_COUNT; (i++, enemy++)) {
		if((enemy->flag != EF_ALIVE) && (enemy->flag < EF_KILL_ANIM)) {
			continue;
		}

		// The enemy is skipped entirely if its PREVIOUS position was outside
		// the vertically extended playfield -- which is not the position
		// anything below is blitted at.
		if(
			(enemy->pos.prev.y <= TO_SP(-(ENEMY_H / 2))) ||
			(enemy->pos.prev.y >= TO_SP(PLAYFIELD_H + (ENEMY_H / 2)))
		) {
			continue;
		}

		patnum = enemy->patnum_base;
		if(enemy->anim_cels > 1) {
			if((enemy->age % enemy->anim_frames_per_cel) == 0) {
				enemy->anim_cur_cel++;
				if(enemy->anim_cur_cel >= enemy->anim_cels) {
					enemy->anim_cur_cel = 0;
				}
			}
			patnum += enemy->anim_cur_cel;
		}

		left = enemy->pos.cur.to_screen_left(ENEMY_W);
		top = enemy->pos.cur.to_vram_top_scrolled_seg1(ENEMY_H);

		if(
			(left > 0) &&
			(left < PLAYFIELD_RIGHT) &&
			(enemy->pos.cur.y > TO_SP(-(ENEMY_H / 2))) &&
			(enemy->pos.cur.y < TO_SP(PLAYFIELD_H + (ENEMY_H / 2)))
		) {
			if(enemy->damaged_this_frame == false) {
				// [top] is pushed out of AX, where the assignment above left
				// it, rather than out of the [bp-4] slot the other call below
				// reads -- this is the branch the compiler reaches in a
				// straight line. (kb/codegen/0126)
				super_roll_put(left, top, patnum);
			} else {
				super_roll_put_1plane(
					left, top, patnum, 0, super_plane(V_WHITE)
				);
				enemy->damaged_this_frame = false;
			}
		}
	}
}
