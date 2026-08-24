/// Stage 5 Boss - Yumeko, foreground rendering
/// -------------------------------------------
/// (#included from th05/b6cbull.cpp, ahead of
/// th05/main/bullet/b6_custombullets_render.cpp. This function was the last
/// `proc` of th05_main.asm's MIDBOSSX_TEXT block once that module was lifted
/// out from under it, and this object is the segment's next contribution, so
/// the lift lands exactly where the root's block ended and every byte above
/// it keeps its address. kb/codegen 0112 + 0114.)
///
/// Yumeko keeps the three-way [boss_fg_render] contract that
/// th04/main/boss/fg.cpp documents for TH04's bosses, with two differences:
///
/// • She animates by adding a cel index derived from [stage_frame_mod16] to
///   [boss.sprite] on every frame of the fight, where EX-Alice and Shinki
///   both blit [boss.sprite] as-is and animate through the phase code.
/// • While the HP bar fills she blits the SECONDARY on-screen boss as well —
///   th05/main/boss/vars2[bss].asm records that [boss2] carries Shinki's
///   leave animation at the start of this fight. It is a super_put_rect()
///   blit rather than a super_put() one, so it is clipped to the playfield.
///
/// She also resets [boss.damage_this_frame] on every frame rather than only
/// after the white flash, which is TH04's Reimu's behaviour rather than
/// Orange's and Kurumi's.

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/main/frames.h"
#include "th04/main/boss/boss.hpp"
// th05/main/boss/bosses.hpp has no include guard and th05/b6cbull.cpp
// compiles th05/main/boss/b4_solo_fg.cpp ahead of this file, so that file owns
// the include for the whole object (kb/codegen/0129).

// Variables for the secondary on-screen boss, still ZUN's assembly
// (th05/main/boss/vars2[bss].asm). Declared the same way in
// th05/main/boss/2_explode_big.cpp and 2_explode_small.cpp.
extern boss_stuff_t boss2;

// [inferred] The cel blitted for [boss2] while the HP bar fills. It comes out
// of Stage 5's own sprite file, not Stage 6's — PAT_STAGE is 180 here, so
// this is the last patnum below PAT_SWORD — and [boss2] carrying Shinki's
// leave animation makes it her departing pose. The cel layout of that file
// has not otherwise been decided, so this is named for the slot it occupies
// rather than for a sprite anyone has looked at.
static const int PAT_YUMEKO_BOSS2 = 192;

static const int YUMEKO_FRAMES_PER_CEL = 4;

void pascal near yumeko_fg_render(void)
{
	screen_x_t left;
	screen_y_t top;
	int patnum;
	screen_x_t boss2_left;
	screen_y_t boss2_top;

	left = boss.pos.cur.to_screen_left(BOSS_W);
	top = boss.pos.cur.to_screen_top(BOSS_H);

	if(boss.phase == PHASE_BOSS_EXPLODE_BIG) {
		super_large_put(left, top, boss.sprite);
	} else {
		if(boss.phase == PHASE_HP_FILL) {
			boss2_left = boss2.pos.cur.to_screen_left(BOSS_W);
			boss2_top = boss2.pos.cur.to_screen_top(BOSS_H);
			super_put_rect(boss2_left, boss2_top, PAT_YUMEKO_BOSS2);
		}
		patnum = (
			boss.sprite + (stage_frame_mod16 / YUMEKO_FRAMES_PER_CEL)
		);
		if(boss.damage_this_frame == 0) {
			super_put(left, top, patnum);
		} else {
			super_put_1plane(left, top, patnum, 0, super_plane(V_WHITE));
		}
		boss.damage_this_frame = 0;
	}
	explosions_small_update_and_render();
	explosions_big_update_and_render();
}
