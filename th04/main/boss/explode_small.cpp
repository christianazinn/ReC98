/// The boss's small explosions
/// ---------------------------
/// Spawns one of the two small explosions -- the first one if it is free, the
/// second one otherwise, with no check that the second one is free either --
/// at the current boss position, in one of five shapes selected by the type.
/// ET_CIRCLE is not a case: it is what the `switch` falls out of, leaving the
/// symmetric defaults set above it.
///
/// This body is shared between the two games and identical in both, but only
/// TH05 compiles it. In TH04 the same code is still assembled from
/// th04/main/boss/explode_small.asm, which is NOT the tail of its segment's
/// root contribution there -- th04/main/boss/explode_big.asm and four more
/// items follow it -- so lifting it out of TH04 would move addresses. The
/// module therefore stays in the tree, and TH04's dump keeps including it.
/// That module also defines the TASM macro EXPLOSION_TYPED, which TH04's big
/// explosion expands and which is the reason its `switch` and this one have the
/// same body; nothing in TH05's dump expands it any more.
///
/// Not compiled on its own: th05/gather.cpp #includes this file at the FRONT of
/// its object, above everything lifted out of the same tail before it, because
/// within one object code is emitted in source order (kb/codegen/0112 + 0114).
///
/// kb/codegen/0104: the `switch` below is dense over 1..4 and compiles to a
/// `jmp word ptr cs:[bx + disp]` against a jump table placed after the
/// epilogue. The `cs:` frame is the GROUP, so the wrapper's `-zPmain_03` is
/// what makes the displacement and the four table entries come out right --
/// th05/gather.cpp already carries it, for its own later `switch`es.
///
/// kb/codegen/0119: this is the FIRST contribution to that object that emits
/// `-a2`-alignable data of its own. The table lands at object offset 0x78,
/// which is even, so no pad byte is at stake either way; and every previous
/// lift into this object was of even length, so the object's own base is where
/// it was.

#include "platform.h"
#include "pc98.h"
#include "th02/snd/snd.h"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/explode.hpp"

void pascal near boss_explode_small(explosion_type_t type)
{
	Explosion near *p = explosions_small;

	if(p->alive) {
		p++;
	}
	p->alive = true;
	p->age = 0;
	p->center.x.v = boss.pos.cur.x.v;
	p->center.y.v = boss.pos.cur.y.v;
	p->radius_cur.x.v = 8;
	p->radius_cur.y.v = 8;
	p->radius_delta.x.v = TO_SP(11);
	p->radius_delta.y.v = TO_SP(11);
	p->angle_offset = 0x00;
	switch(type) {
	case ET_NW_SE:
		p->angle_offset = 32;
		break;
	case ET_SW_NE:
		p->angle_offset = -32;
		break;
	case ET_HORIZONTAL:
		p->radius_delta.x.v = TO_SP(13);
		p->radius_delta.y.v = TO_SP(7);
		break;
	case ET_VERTICAL:
		p->radius_delta.x.v = TO_SP(7);
		p->radius_delta.y.v = TO_SP(13);
		break;
	}
	snd_se_play(15);
}
