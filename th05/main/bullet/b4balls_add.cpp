/// Mai & Yuki's ball bullets: the reset and the spawn function
/// ----------------------------------------------------------
/// The last two of the four functions th05/main/bullet/b4ball.hpp declares,
/// and with them that header has no assembly left behind any of it: the
/// subsystem is entirely C++. They replace the two-`proc` assembly module
/// that th05_main.asm included at the very end of its main_035_TEXT
/// contribution and that this commit deletes, so this file goes at the front
/// of th05/main/boss/b4_mai.cpp -- ahead of that file's include of
/// b4balls_update.cpp -- and the object grows backwards into the hole the
/// module leaves. No carve, no new segment, no Tupfile.lua line (kb/codegen
/// 0099 + 0112 + 0114).
///
/// PORTED from swords_add() in th05/main/bullet/swords_add_update.cpp, which
/// was itself ported from b6balls_add() in th05/main/bullet/b6ball.cpp: the
/// same playperf_speedtune() opening, the same scan for a free slot, the same
/// vector2_near() call and the same early `return` out of the loop. What this
/// one adds is the per-ball HP and revenge fields; what it leaves out is the
/// spawn circle and the twirl.
///
/// `[measured]` Both bodies were graded by `tcc -S` (kb/codegen/0152) before
/// the first build and matched the module they replace instruction for
/// instruction on the first probe, including the two things that would
/// otherwise have cost a build cycle each: b4balls_reset()'s loop counter
/// lives in AX rather than in DI, with only SI pushed, even though `ball` and
/// `i` are two ordinary locals; and the `mov ah, 0` after the
/// playperf_speedtune() call is the byte return widened into the word local,
/// not a spelling of the assignment.
///
/// 0x1B + 0x69 = 0x84 bytes, EVEN, which is what the `-a2` parity ledger in
/// state/notes/th05-main-mai-update.md requires of every run prepended to this
/// object: it holds two generated jump tables that take OPPOSITE pads, and an
/// odd total moves both (kb/codegen 0119 + 0154 + 0159 + 0160).

// The two headers this object reaches nowhere else. BOTH are unguarded, and
// this file is now the earliest one in the object, so it owns them and no
// later file may include either (kb/codegen/0129). Neither was in the
// translation unit before this parcel -- measured, not assumed -- so nothing
// below is a hoist.
#include "th04/math/vector.hpp"
#include "th05/main/playperf.hpp"

// `extern "C"` + `pascal` on both, for the reason th05/main/bullet/b4ball.hpp
// gives at all four of its declarations: the module this replaces PUBLISHed
// the undecorated upper-case B4BALLS_RESET and B4BALLS_ADD (kb/codegen 0081 +
// 0102).
extern "C" void pascal near b4balls_reset(void)
{
	b4ball_t near *ball;
	int i;

	for((ball = b4balls, i = 1); i < (1 + B4BALL_COUNT); (i++, ball++)) {
		ball->flag = 0;
	}
}

extern "C" void pascal near b4balls_add(void)
{
	subpixel_t speed = playperf_speedtune(b4ball_template.speed);
	b4ball_t near *ball;
	int i;

	for((ball = b4balls, i = 1); i < (1 + B4BALL_COUNT); (i++, ball++)) {
		if(ball->flag != 0) {
			continue;
		}
		ball->flag = 1;
		ball->pos.cur = b4ball_template.origin;
		vector2_near(ball->pos.velocity, b4ball_template.angle, speed);
		ball->angle = b4ball_template.angle;

		// `[measured]` The template's UNTUNED speed, not the tuned [speed]
		// the velocity was built from: the module this replaces reads the
		// template field a second time rather than [bp-2]. swords_add()
		// stores the tuned value at the same point, so the two functions
		// really do differ here. Nothing in the C++ reads this field back;
		// what it is for is not decided.
		ball->speed = b4ball_template.speed;

		ball->patnum_tiny_base = b4ball_template.patnum_tiny_base;
		ball->hp = b4ball_template.hp;
		ball->damaged_this_frame = 0;
		ball->revenge = b4ball_template.revenge;
		return;
	}
}
