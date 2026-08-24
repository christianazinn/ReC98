/// The secondary boss's small explosion
/// ------------------------------------
/// The twin of th05/main/boss/2_explode_big.cpp: boss_explode_small() spawns
/// its explosion at whatever [boss.pos.cur] happens to be and takes no
/// position, so [boss2]'s copy is a swap around the call. The only difference
/// is the type parameter this one forwards -- and that it is `pascal`, which
/// the original publishes in upper case, while the big one is not and
/// publishes in lower (kb/codegen/0102).
///
/// The parameter is an `unsigned int` rather than an explosion_type_t: the
/// original's mangled name ends in `$qui`, and th05/main/boss/boss.hpp has said
/// so since long before this lift. The cast at the call is what C++ costs for
/// ZUN's implicit int-to-enum conversion; it is not a widening or a narrowing,
/// and both sides push one word.
///
/// The restore is two word stores where the copy was one 32-bit move, exactly
/// as in the big version, and for the same reason: ZUN saved X and Y into two
/// separate locals rather than into one Point, so there was no 4-byte value
/// left to put back. The two locals are memory-homed rather than register
/// variables because they have to survive the call (kb/codegen/0117); their
/// frame slots give their declaration order, X nearest BP and therefore first.
///
/// Not compiled on its own: th05/gather.cpp #includes this file at the FRONT of
/// its object, above the two functions lifted before it, because within one
/// object code is emitted in source order and this one sat above both of them
/// in the original (kb/codegen/0112 + 0114).
///
/// kb/codegen/0129: like the big version, this file deliberately does NOT
/// include th05/main/boss/boss.hpp, where its own declaration lives. That
/// header carries no include guard and th05/main/stage/bonus.cpp -- later in
/// this same translation unit -- already includes it, so a second copy would
/// re-process its `y_direction_t` enum and its namespace-scope constants.
/// th04/main/boss/boss.hpp IS guarded, so boss_stuff_t, explosion_type_t and
/// boss_explode_small() come from there, and the two declarations below are the
/// rest of what the TH05 header would have provided. They are plain `extern`s,
/// which may legally repeat.

#include "platform.h"
#include "pc98.h"
#include "th04/main/boss/boss.hpp"

// Both also declared by th05/main/boss/boss.hpp, which this translation unit
// gets from th05/main/stage/bonus.cpp; see the note above.
extern boss_stuff_t boss2;
void pascal near boss2_explode_small(unsigned int type);

void pascal near boss2_explode_small(unsigned int type)
{
	subpixel_t x = boss.pos.cur.x.v;
	subpixel_t y = boss.pos.cur.y.v;

	boss.pos.cur = boss2.pos.cur;
	boss_explode_small(static_cast<explosion_type_t>(type));
	boss.pos.cur.x.v = x;
	boss.pos.cur.y.v = y;
}
