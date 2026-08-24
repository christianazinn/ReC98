/// Boss explosions
/// ---------------
/// The state of the two small explosions and the single big one that every
/// TH04 and TH05 boss plays back on defeat, together with the per-frame update
/// every renderer applies to one of them.
///
/// This was declared inside th04/main/boss/explode.cpp for as long as that file
/// was the only code that touched it. It stopped being so once the small and
/// big explosion *spawners* started coming out of TH05 MAIN's `include` tails
/// into their own files, which are compiled into a different segment and
/// therefore cannot be part of that translation unit. Two copies of a struct
/// whose layout is fixed by the original binary would not diverge silently --
/// the oracle would catch it -- but they would still be two copies.
///
/// Assumes platform.h (for `bool` and `int8_t`); every current includer has it
/// well before this point, which is also true of th04/main/boss/boss.hpp.

#ifndef TH04_MAIN_BOSS_EXPLODE_HPP
#define TH04_MAIN_BOSS_EXPLODE_HPP

#include "th01/math/subpixel.hpp"

#define EXPLOSION_SMALL_COUNT 2

struct Explosion {
	bool alive;
	unsigned char age;
	SPPoint center;
	SPPoint radius_cur;
	SPPoint radius_delta;
	int8_t unused; // ZUN bloat

	// Offset to add to the angle for the Y coordinate, turning the circle
	// into a slanted ellipse. See https://www.desmos.com/calculator/faeefi6w1u
	// for a plot of the effect.
	unsigned char angle_offset;

	// A method is the most idiomatic way for this to compile into ZUN's
	// original three instructions, believe it or not.
	unsigned char angle_y(const unsigned char& angle_base) {
		return (angle_offset + angle_base);
	}
};

#define explosion_update(p) { \
	(p).radius_cur.x.v += (p).radius_delta.x.v; \
	(p).radius_cur.y.v += (p).radius_delta.y.v; \
	(p).age++; \
	if((p).age >= 32) { \
		(p).alive = false; \
	} \
}

extern Explosion explosions_small[EXPLOSION_SMALL_COUNT];
extern Explosion explosions_big;

#endif /* TH04_MAIN_BOSS_EXPLODE_HPP */
