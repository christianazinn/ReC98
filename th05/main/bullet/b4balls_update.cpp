/// Mai & Yuki's ball bullets: the per-frame state update
/// ----------------------------------------------------
/// The third of the four functions th05/main/bullet/b4ball.hpp declares to be
/// decompiled, and the twin of b4balls_render() in the file next to this one:
/// the two halves of the same loop over the same array, in the two segments
/// the boss's update and render code live in.
///
/// PORTED from b6balls_update() in th05/main/bullet/b6ball.cpp rather than
/// derived. Shinki's ball bullets run the same loop, the same clip test, the
/// same in-place player hit test and the same two-state decay machine, and
/// swords_update() in th05/main/bullet/swords_add_update.cpp is the same body
/// a third time. What THIS one adds is the shot hit test with per-ball HP, and
/// what it leaves out is the cloud state and the bullet-clear check.
///
/// (#included from th05/main/boss/b4_mai.cpp, immediately ahead of that file's
/// `#include` of th05/main/boss/b4_yuki.cpp, which makes this the first body
/// the th05/b4mai.cpp object emits -- exactly its position in the original,
/// directly above yuki_1B557(). Nothing in b4_mai.cpp above that point emits
/// code, so the object grows backwards into the hole the two `proc`s leave and
/// every byte above them keeps its address: kb/codegen 0112 + 0114, no carve,
/// no new segment, no Tupfile.lua line.)
///
/// The pair is ONE parcel because of the `-a2` parity ledger in
/// state/notes/th05-main-mai-update.md: 273 + 209 = 482 is EVEN, so both of
/// the object's generated jump tables keep the pad they have (kb/codegen 0119
/// + 0154 + 0157 + 0160). yuki_1B557() alone would have moved both.

// The only header this object reaches that th05/main/boss/b4_mai.cpp does not
// include itself, and it is UNGUARDED -- so it is included here, from the
// earliest file in the object that needs it, and no later file may include it
// again (kb/codegen/0129). Macros only; nothing in it can move a byte of the
// seventeen bodies below.
#include "decomp.hpp"
#include "th01/math/overlap.hpp"

/// From headers this object must not expand
/// ---------------------------------------
// th04/main/player/shot.hpp, which reaches th04/math/randring.hpp and would
// therefore expand those inline bodies a second time in this object. Repeated
// rather than included, which is what th04/main/boss/b4m_upd.cpp does with the
// same three declarations for the same reason.
extern SPPoint shot_hitbox_center;
extern SPPoint shot_hitbox_radius;
int shots_hittest(void);

// th04/main/score.hpp, which has no include guard at all.
extern unsigned long score_delta;
/// ---------------------------------------

// [inferred] The box the shot hit test uses for a ball: a quarter of the box
// the player is tested against below, and a quarter of the 32-pixel sprite.
// Spelled at the one place that sets it, the way th04/main/boss/b3_upd.cpp
// spells the same field's value.
#define B4BALL_SHOT_HITBOX_RADIUS 8.0f

// Score for destroying one snow ball. The fire ones cannot be destroyed.
#define B4BALL_DESTROY_SCORE 550

// [inferred] Below this line, a revenge ball fires its dying shot; above it,
// it dies quietly. 240 is 16 pixels above the bottom of the playfield.
#define B4BALL_REVENGE_BOTTOM 240.0f

// `extern "C"` + `pascal`, for the reason th05/main/bullet/b4ball.hpp gives at
// all four of its declarations: the module this replaces published the
// undecorated upper-case B4BALLS_UPDATE (kb/codegen 0081 + 0102).
extern "C" void pascal near b4balls_update(void)
{
	b4ball_t near *ball;
	int i;
	int damage;

	// Set once for the whole loop, so this function cannot use the
	// shots_hittest() overload that takes the box with the center.
	shot_hitbox_radius.x.v = to_sp(B4BALL_SHOT_HITBOX_RADIUS);
	shot_hitbox_radius.y.v = to_sp(B4BALL_SHOT_HITBOX_RADIUS);

	for((ball = b4balls, i = 1); i < (1 + B4BALL_COUNT); (i++, ball++)) {
		if(ball->flag == 0) {
			continue;
		}
		ball->age++;
		/* DX:AX = */ ball->pos.update_seg3();
		if(!playfield_encloses(_AX, _DX, B4BALL_W, B4BALL_H)) {
			// [measured] The barrier is b6balls_update()'s, for the same
			// reason and with the same effect: without it, Turbo threads all
			// four of the clip test's conditional jumps straight onto the
			// shared `flag = 0` store below, and each has to be a 386 long
			// jump. The original branches SHORT to a local three-byte
			// trampoline instead, which is 5 bytes less in a body that is
			// otherwise instruction-for-instruction identical.
			optimization_barrier();
			ball->flag = 0;
		} else if(ball->flag != 2) {
			_AX -= player_pos.cur.x.v;
			_DX -= player_pos.cur.y.v;
			if(overlap_wh_inplace_fast(
				_AX, _DX, to_sp(B4BALL_W / 2), to_sp(B4BALL_H / 2)
			)) {
				player_is_hit = true;
			}
			shot_hitbox_center = ball->pos.cur;
			if(!(damage = shots_hittest())) {
				continue;
			}
			if(ball->patnum_tiny_base != PAT_B4BALL_SNOW) {
				// Yuki's fire balls take no damage at all, and the sound is
				// the one the game plays for a shot hitting an invincible
				// target.
				snd_se_play(10);

				// [measured] Neither a plain fallthrough out of the branch nor
				// a loop continuation reproduces this. Turbo cross-jumps the two
				// snd_se_play() tails either way, but only an explicit jump out
				// makes it keep the LATER copy of the shared call instruction,
				// which is where the original has it; both other spellings keep
				// the earlier one and move five instructions' worth of bytes
				// without changing the body's length.
				goto next;
			} else {
				ball->damaged_this_frame = 1;
				ball->hp -= damage;
				snd_se_play(4);
				if(ball->hp < 0) {
					ball->flag++;
					ball->age = 0;
					ball->patnum_tiny_base = PAT_DECAY_B4BALL;
					ball->pos.velocity.x.v /= 4;
					ball->pos.velocity.y.v /= 4;
					score_delta += B4BALL_DESTROY_SCORE;
					if(
						ball->revenge &&
						(ball->pos.cur.y.v <= to_sp(B4BALL_REVENGE_BOTTOM))
					) {
						bullet_template.origin = ball->pos.cur;
						bullets_add_regular();
					}
					snd_se_play(3);
				}
			}
		} else if((ball->age % 4) == 0) {
			ball->patnum_tiny_base++;
			if(ball->patnum_tiny_base >= (PAT_DECAY_B4BALL_last + 1)) {
				ball->flag = 0;
			}
		}
next:	;
	}
}
