/// Yumeko's swords: spawning, and the per-frame state update
/// ---------------------------------------------------------
/// The other two of the three functions th05/main/bullet/sword.hpp declares.
/// With these, that header has no assembly left behind any of them, and the
/// subsystem is entirely C++.
///
/// swords_update() is PORTED from b6balls_update() in
/// th05/main/bullet/b6ball.cpp rather than derived: Shinki's ball bullets run
/// the same loop, the same clip test, the same in-place hit test and the same
/// two-state decay machine, down to the `optimization_barrier()` calls that
/// keep -O from merging the arms' jumps (kb/codegen 0097 + 0144). What the
/// swords add is the twirl animation ahead of the motion update, and what
/// they leave out is the cloud state.
///
/// Its OWN object (th05/swords.cpp) rather than an `#include` at the front of
/// th05/main035.cpp. Length is not the reason -- 0x194 is even, so
/// kb/codegen/0119 would have been satisfied either way. The reason is
/// th05/main/bullet/sword.hpp, which is unguarded and which
/// th05/main/boss/b5.cpp already includes: reaching it from the front of that
/// object would either collide or force the header up the include order of a
/// translation unit that is already matched.

#include "decomp.hpp"
#include "th01/math/overlap.hpp"
#include "th02/snd/snd.h"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/circle.hpp"
#include "th04/main/custom.hpp"
#include "th04/main/player/player.hpp"
#include "th04/math/vector.hpp"
#include "th05/main/playperf.hpp"
#include "th05/main/bullet/sword.hpp"
#include "th05/sprites/main_pat.h"

// [inferred] The sword's collision box, spelled inline at the one place that
// uses it because a `static const` initialized through to_sp() is not a
// constant expression and would earn this object a static constructor. It is
// a good deal tighter than the 32×32 sprite: the blade is drawn diagonally
// across the cel, so only its middle can hit.
#define SWORD_HITBOX_W 14.0f
#define SWORD_HITBOX_H 14.0f

// `extern "C"` + `pascal`, for the reason th05/main/bullet/sword.hpp gives at
// both declarations: the module this replaces published the undecorated
// upper-case SWORDS_ADD and SWORDS_UPDATE (kb/codegen 0081 + 0102).
extern "C" void pascal near swords_add(void)
{
	subpixel_t speed = playperf_speedtune(sword_template.speed);
	sword_t near *sword;
	int i;

	circles_color = 9;
	for((sword = swords, i = 1); i < (1 + SWORD_COUNT); (i++, sword++)) {
		if(sword->flag != F_FREE) {
			continue;
		}
		sword->flag = F_ALIVE;
		sword->pos.cur = sword_template.origin;
		circles_add_shrinking(
			sword_template.origin.x, sword_template.origin.y
		);
		vector2_near(sword->pos.velocity, sword_template.angle, speed);
		sword->angle = sword_template.angle;
		sword->patnum_tiny = bullet_patnum_for_angle(
			PAT_SWORD, sword_template.angle
		);
		sword->speed.v = speed;
		sword->twirl_time = sword_template.twirl_time;
		return;
	}
}

extern "C" void pascal near swords_update(void)
{
	sword_t near *sword;
	int i;

	for((sword = swords, i = 1); i < (1 + SWORD_COUNT); (i++, sword++)) {
		if(sword->flag == F_FREE) {
			continue;
		}
		if(bullet_clear_time && (sword->flag == F_ALIVE)) {
			sword->flag = F_REMOVE;
			sword->twirl_time = 0;
		}
		if(sword->twirl_time > 0) {
			sword->twirl_time--;
			if(sword->twirl_time == 0) {
				// Done twirling: settle on the cel that shows the direction
				// the sword is actually flying in, and clang.
				sword->patnum_tiny = bullet_patnum_for_angle(
					PAT_SWORD, sword->angle
				);
				snd_se_play(3);
			} else if(i & 1) {
				// ZUN quirk: The rotation direction comes from the sword's
				// slot number rather than from anything about the sword, so
				// which way a thrown sword spins is decided by whichever
				// slots happened to be free.
				sword->patnum_tiny += 2;
				if(sword->patnum_tiny >= (PAT_SWORD_last + 1)) {
					sword->patnum_tiny -= BULLET_V_CELS;
				}
				continue;
			} else {
				sword->patnum_tiny -= 2;
				if(sword->patnum_tiny < PAT_SWORD) {
					sword->patnum_tiny += BULLET_V_CELS;
				}
				continue;
			}
		}

		/* DX:AX = */ sword->pos.update_seg3();
		if(!playfield_encloses(_AX, _DX, SWORD_W, SWORD_H)) {
			optimization_barrier();
			sword->flag = F_FREE;
		} else if(sword->flag != F_REMOVE) {
			_AX -= player_pos.cur.x.v;
			_DX -= player_pos.cur.y.v;
			if(overlap_wh_inplace_fast(
				_AX, _DX, to_sp(SWORD_HITBOX_W), to_sp(SWORD_HITBOX_H)
			)) {
				player_is_hit = true;
				sword->flag = F_REMOVE;
				continue;
			}
			optimization_barrier();
		} else if(sword->patnum_tiny < PAT_DECAY_SWORD) {
			sword->patnum_tiny = PAT_DECAY_SWORD;
			sword->pos.velocity.x.v /= 2;
			sword->pos.velocity.y.v /= 2;
			sword->decay_frame = 0;
		} else {
			sword->decay_frame++;
			if((sword->decay_frame % 4) == 0) {
				sword->patnum_tiny++;
				if(sword->patnum_tiny >= (PAT_DECAY_SWORD_last + 1)) {
					sword->flag = F_FREE;
				}
			}
		}
	}
}
