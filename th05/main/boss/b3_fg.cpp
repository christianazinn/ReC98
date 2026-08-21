/// Stage 3 Boss - Alice, foreground rendering
/// ------------------------------------------
/// Keeps the three-way [boss_fg_render] contract that th04/main/boss/fg.cpp
/// documents for TH04's bosses, and is the only TH05 boss renderer that draws
/// something *other* than the boss as part of it: Alice's two puppets, whose
/// own renderer sits directly above this function in th05_main.asm and is
/// deliberately NOT called during the big explosion.
///
/// Like Mai and Yuki, Alice does not reset [boss.damage_this_frame] after the
/// white flash; yumeko_fg_render() and exalice_fg_render() both do.
///
/// (#included from th05/b34fg.cpp, ahead of
/// th05/main/boss/b4_pair_fg.cpp. Both functions were the last `proc`s of
/// th05_main.asm's MIDBOSSX_TEXT block once mai_yuki_fg_render() was lifted
/// out from under this one, and that object is the segment's next
/// contribution, so the lift lands exactly where the root's block ended.
/// kb/codegen 0112 + 0114.)

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/main/frames.h"
#include "th04/main/boss/boss.hpp"
// th05/main/boss/b3puppet.hpp, which declares puppets_render(), is NOT
// named here. It has no include guard and th05/main/boss/b3puppet_render.cpp
// already pulls it into this object ahead of us, so naming it again is a
// compile error rather than a no-op.
#include "th05/main/boss/bosses.hpp"

// [inferred] Kept file-local for the same reason th05/main/boss/b4_solo_fg.cpp
// keeps its own: what these cels depict has not been decided, only that the
// renderer animates them and blits every other cel as-is. 180 is the value of
// th05/sprites/main_pat.h's [PAT_STAGE], i.e. the first cel of *whichever*
// stage's boss sheet is loaded, so a name taken from it would claim more than
// this function shows.
static const int PAT_ALICE_ANIMATED_SLOW = 180;
static const int PAT_ALICE_ANIMATED_FAST = 184;

// Both animated cels step every 4th frame. The [PAT_ALICE_ANIMATED_FAST] cel
// reaches [stage_frame_mod2] undivided instead, so it alternates every frame.
static const int ALICE_FRAMES_PER_CEL = 4;

void pascal near alice_fg_render(void)
{
	// Same frame as b4_solo_fg_render(): `ENTER 2, 0`, three 16-bit locals,
	// two enregistered and one on the stack. (kb/codegen 0010 + 0146)
	screen_x_t left;
	int patnum;
	screen_y_t top;

	left = boss.pos.cur.to_screen_left(BOSS_W);
	top = boss.pos.cur.to_screen_top(BOSS_H);

	if(boss.phase == PHASE_BOSS_EXPLODE_BIG) {
		super_large_put(left, top, boss.sprite);
	} else {
		patnum = boss.sprite;
		if(patnum == PAT_ALICE_ANIMATED_SLOW) {
			patnum += (stage_frame_mod16 / ALICE_FRAMES_PER_CEL);
		} else if(patnum == PAT_ALICE_ANIMATED_FAST) {
			patnum += stage_frame_mod2;
		} else {
			patnum += (stage_frame_mod8 / ALICE_FRAMES_PER_CEL);
		}
		if(boss.damage_this_frame == 0) {
			super_put(left, top, patnum);
		} else {
			super_put_1plane(left, top, patnum, 0, super_plane(V_WHITE));
		}
		puppets_render();
	}
	explosions_small_update_and_render();
	explosions_big_update_and_render();
}
