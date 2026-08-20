/// The secondary boss's big circle explosion
/// -----------------------------------------
/// boss_explode_big_circle() spawns its explosion at whatever [boss.pos.cur]
/// happens to be and takes no position, so [boss2]'s copy is a swap: save the
/// primary boss's position, move the secondary one in, call, put the original
/// back. th05/main/boss/2_explode_small.cpp is the same body with a parameter;
/// it was still ASM when this file was written, because it was not yet the tail
/// of this segment's root contribution, and it is the file directly above this
/// one in th05/gather.cpp now that it is. TH04 has neither: it has no secondary
/// boss.
///
/// The restore is two word stores where the copy was one 32-bit move, and that
/// asymmetry is in the original: ZUN saved X and Y into two separate locals
/// rather than into one Point, so there was no 4-byte value left to put back.
/// The two locals are memory-homed rather than register variables because they
/// have to survive the call (kb/codegen/0117); their frame slots give their
/// declaration order, X nearest BP and therefore first.
///
/// Not compiled on its own: th05/gather.cpp #includes this file ABOVE
/// th05/main/stage/bonus.cpp, because within one object code is emitted in
/// source order and this function sat immediately before that object's
/// contribution in the original (kb/codegen/0112 + 0114).
///
/// kb/codegen/0129: this file deliberately does NOT include
/// th05/main/boss/boss.hpp, even though that is where its own declaration
/// lives. That header carries no include guard and th05/main/stage/bonus.cpp --
/// the next file in this same translation unit -- already includes it, so a
/// second copy would re-process its `y_direction_t` enum and its
/// namespace-scope constants. th04/main/boss/boss.hpp IS guarded, so taking
/// boss_stuff_t and boss_explode_big_circle() from there is safe from either
/// side, and the two declarations below are the rest of what that header would
/// have provided. They are plain `extern`s, which may legally repeat.

#include "platform.h"
#include "pc98.h"
#include "th04/main/boss/boss.hpp"

// Both also declared by th05/main/boss/boss.hpp, which this translation unit
// gets from th05/main/stage/bonus.cpp; see the note above.
extern boss_stuff_t boss2;
void near boss2_explode_big_circle(void);

void near boss2_explode_big_circle(void)
{
	subpixel_t x = boss.pos.cur.x.v;
	subpixel_t y = boss.pos.cur.y.v;

	boss.pos.cur = boss2.pos.cur;
	boss_explode_big_circle();
	boss.pos.cur.x.v = x;
	boss.pos.cur.y.v = y;
}
