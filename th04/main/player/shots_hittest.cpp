/// Shot collision
/// --------------
/// (#included from th04/main_.cpp, ahead of th04/main/enemy/render.cpp and
/// th04/main/player/invalidate.cpp, which is the address order the three
/// bodies had in main__TEXT. This proc was the tail of th04_main.asm's own
/// contribution to that segment, so the object that already hosts the other
/// two grows backwards into the hole it left and every byte above it keeps
/// its address (kb/codegen 0099 + 0112 + 0114).
///
/// The only header this file shares with either of the other two is
/// th04/main/player/player.hpp, which MATCH-TH05-MAIN-TAILS-1 guarded for the
/// same collision in th05/shot_inv.cpp, so nobody has to decline it
/// (kb/codegen/0129).)
///
/// Walks the [shots_alive] cache built by shots_update(), decays every shot
/// inside the hitbox that boss_hittest_shots_damage() or its midboss
/// equivalent has just set, and returns the total damage. Each further shot in
/// the same frame is worth less: the damage of the n-th one is divided by n.
/// The option laser is checked separately at the end, once per two frames, and
/// is worth a flat 3 per side.

#include "th02/main/player/bomb.hpp"
#include "th04/main/frames.h"
#include "th04/main/score.hpp"
#include "th04/main/spark.hpp"
#include "th04/main/player/shot.hpp"
#include "th04/sprites/main_pat.h"

// Alternates the spark spawns of both the shot and the laser branches, so that
// only every second (shots) or fourth (laser) hit gets one. Still unnamed; see
// state/notes/th04-main-carve-tails-1.md.
extern "C" unsigned char byte_25980;

// Radius of the spark burst spawned at a hit.
#define HIT_SPARK_RADIUS TO_SP(8)
#define HIT_SPARK_COUNT 1

int shots_hittest(void)
{
	// Declaration order is the frame layout: [bp-2] down to [bp-0E] for the
	// words, then the byte at [bp-0F]. (kb/codegen/0010)
	unsigned int i;
	shot_alive_t near *sa;
	subpixel_t left;
	subpixel_t top;
	subpixel_t w;
	subpixel_t h;
	Subpixel laser_x;
	unsigned char hits;

	register Shot near *shot;
	register unsigned int damage_total;

	left = (shot_hitbox_center.x.v - shot_hitbox_radius.x.v);
	top = (shot_hitbox_center.y.v - shot_hitbox_radius.y.v);
	w = (shot_hitbox_radius.x.v * 2);
	h = (shot_hitbox_radius.y.v * 2);

	damage_total = 0;
	hits = 0;

	sa = shots_alive;
	for(i = 0; i < shots_alive_count; i++, sa++) {
		// An `if` block, not a `continue` guard: the latter is a barrier to
		// Turbo C++'s -Z register tracking and made it reload the iterator
		// into BX for the third field access (kb/codegen 0140 + 0093).
		if(
			((unsigned int)(sa->pos.x.v - left) <= (unsigned int)w) &&
			((unsigned int)(sa->pos.y.v - top) <= (unsigned int)h)
		) {
			shot = sa->shot;
			shot->flag = SF_HIT;
			shot->pos.velocity.x.v = (shot->pos.velocity.x.v / 6);
			shot->pos.velocity.y.v = (shot->pos.velocity.y.v / 6);
			shot->patnum_base = PAT_HITSHOT;
			hits++;
			damage_total += (static_cast<unsigned char>(shot->damage) / hits);
			byte_25980++;
			if(byte_25980 & 1) {
				sparks_add_random(
					shot->pos.cur.x, shot->pos.cur.y,
					HIT_SPARK_RADIUS, HIT_SPARK_COUNT
				);
			}
		}
	}
	if(bombing) {
		if(stage_frame_mod4 == 0) {
			damage_total += 5;
		}
		if(shots_hittest_against_boss) {
			damage_total /= 4;
		}
	}
	if(
		(stage_frame_mod2 != 0) &&
		(shot_laser_time > SHOT_LASER_COOLDOWN_FRAMES) &&
		((unsigned int)top <= (unsigned int)shot_laser_bottomcenter.cur.y.v)
	) {
		laser_x.v = (
			shot_laser_bottomcenter.cur.x.v - TO_SP(PLAYER_OPTION_DISTANCE)
		);
		if((unsigned int)(laser_x.v - left) <= (unsigned int)w) {
			damage_total += 3;
			byte_25980++;
			if((byte_25980 & 3) == 0) {
				sparks_add_random(
					laser_x, shot_hitbox_center.y,
					HIT_SPARK_RADIUS, HIT_SPARK_COUNT
				);
			}
		}
		laser_x.v += TO_SP(PLAYER_OPTION_TO_OPTION_DISTANCE);
		if((unsigned int)(laser_x.v - left) <= (unsigned int)w) {
			damage_total += 3;
			byte_25980++;
			if((byte_25980 & 3) == 0) {
				sparks_add_random(
					laser_x, shot_hitbox_center.y,
					HIT_SPARK_RADIUS, HIT_SPARK_COUNT
				);
			}
		}
	}
	score_delta += damage_total;
	return damage_total;
}
