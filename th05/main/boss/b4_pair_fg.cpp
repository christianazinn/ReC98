/// Stage 4 Bosses - Mai & Yuki, PAIR foreground rendering
/// ------------------------------------------------------
/// THE Stage 4 [boss_fg_render_func], installed by th05/main/stage/setup.cpp
/// and the one that draws *both* characters. th05/main/boss/b4_solo_fg.cpp
/// holds the second-phase counterpart that draws whichever of the two is
/// left; read the two together. b4_solo_fg.cpp's own banner explains how the
/// survivor ends up in [boss].
///
/// [boss2_phase_state] is what decides who is who here. th05/main/boss/
/// b4_both.cpp derives it from mai_yuki_hittest_shots(): 0 means Mai ended
/// her phase this frame, non-zero means Yuki did. So the character named by
/// it is the one that explodes, and the other one keeps being blitted
/// normally -- which is why every test in this function comes in a pair with
/// the sense of [boss2_phase_state] inverted between them.
///
/// Note that both characters' explosion phases are read off [boss.phase]
/// alone. [boss2] has a [phase] of its own and this function never looks at
/// it.
///
/// (#included from th05/b34fg.cpp, behind th05/main/boss/b3_fg.cpp.
/// This function was the last `proc` of th05_main.asm's MIDBOSSX_TEXT block
/// once midboss4_render() was lifted out from under it, and that object is
/// the segment's next contribution, so the lift lands exactly where the
/// root's block ended. kb/codegen 0112 + 0114.)

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/main/frames.h"
#include "th04/main/boss/boss.hpp"
#include "th05/sprites/main_pat.h"
// th05/main/boss/bosses.hpp -- which declares [yuki] and, through
// th05/main/boss/boss.hpp, [boss2] -- is NOT named here. It has no include
// guard and th05/main/boss/b3_fg.cpp already pulls it into this object ahead
// of us, so naming it again is a compile error rather than a no-op. Same
// arrangement as th05/main/boss/b4_both.cpp's note about main_pat.h.

// th05/main/boss/b4_both.cpp spells the same pairing this way, and [yuki] is
// declared in th05/main/boss/bosses.hpp as its own alias of [boss2].
#define mai boss

// [inferred] The cel that animates in THIS renderer sits 4 cels into each
// character's B4_CELS-wide block. th05/main/boss/b4_solo_fg.cpp animates a
// different one, 12 cels in, so the two are not interchangeable and neither
// pair of numbers is a good name for the other. Kept file-local for the
// reason that file gives: where the cels are is measured, what they depict is
// not.
static const int PAT_MAI_PAIR_ANIMATED = 184;
static const int PAT_YUKI_PAIR_ANIMATED = 200;

static const int B4_ANIM_FRAMES_PER_CEL = 2;

// The ordinary (= non-explosion) blit for one of the two, factored out because
// the four places below that need it differ only in which character's
// coordinates, cel and damage flag they pass. The dump left it unnamed and
// published no symbol for it, so it stays `static`: nothing outside this file
// ever called it.
static void pascal near b4_cel_put(
	screen_x_t left, screen_y_t top, int patnum, int damage_this_frame
)
{
	if(
		(patnum == PAT_MAI_PAIR_ANIMATED) ||
		(patnum == PAT_YUKI_PAIR_ANIMATED)
	) {
		patnum += (stage_frame_mod8 / B4_ANIM_FRAMES_PER_CEL);
	}
	if(damage_this_frame == 0) {
		super_put(left, top, patnum);
	} else {
		super_put_1plane(left, top, patnum, 0, super_plane(V_WHITE));
	}
}

void pascal near mai_yuki_fg_render(void)
{
	// `ENTER 4, 0`: four 16-bit locals, Mai's two enregistered and Yuki's two
	// on the stack, with Yuki's X in the slot nearer to BP. So the four have
	// to be declared in exactly this order. (kb/codegen 0010 + 0146)
	screen_x_t mai_left;
	screen_y_t mai_top;
	screen_x_t yuki_left;
	screen_y_t yuki_top;

	mai_left = mai.pos.cur.to_screen_left(BOSS_W);
	mai_top = mai.pos.cur.to_screen_top(BOSS_H);
	yuki_left = yuki.pos.cur.to_screen_left(BOSS_W);
	yuki_top = yuki.pos.cur.to_screen_top(BOSS_H);

	if(mai.phase == PHASE_BOSS_EXPLODE_BIG) {
		// Whoever is exploding gets the large blit, and the other one is still
		// drawn normally. Note that Yuki's large cel is NOT offset by B4_CELS
		// the way her ordinary one below is.
		if(boss2.phase_state.patterns_seen == 0) {
			super_large_put(mai_left, mai_top, mai.sprite);
			b4_cel_put(
				yuki_left, yuki_top, (yuki.sprite + B4_CELS),
				yuki.damage_this_frame
			);
		} else {
			super_large_put(yuki_left, yuki_top, yuki.sprite);
			b4_cel_put(
				mai_left, mai_top, mai.sprite, mai.damage_this_frame
			);
		}
	} else {
		// Before either explosion phase, both are drawn. Once one of them has
		// started exploding, [boss2_phase_state] names it and it is dropped
		// here, because boss_explode_*() is drawing it instead.
		if(
			(mai.phase <= PHASE_BOSS_EXPLODE_SMALL) ||
			(boss2.phase_state.patterns_seen != 0)
		) {
			b4_cel_put(
				mai_left, mai_top, mai.sprite, mai.damage_this_frame
			);
		}
		if(
			(mai.phase <= PHASE_BOSS_EXPLODE_SMALL) ||
			(boss2.phase_state.patterns_seen == 0)
		) {
			b4_cel_put(
				yuki_left, yuki_top, (yuki.sprite + B4_CELS),
				yuki.damage_this_frame
			);
		}
	}
	explosions_small_update_and_render();
	explosions_big_update_and_render();

	// Unlike b4_solo_fg_render(), the pair renderer DOES clear both flags.
	mai.damage_this_frame = 0;
	yuki.damage_this_frame = 0;
}
