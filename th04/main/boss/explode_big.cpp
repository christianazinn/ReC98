/// The boss's big explosion
/// ------------------------
/// Spawns the single large explosion at the current boss position, in the same
/// five shapes boss_explode_small() offers -- ET_CIRCLE again being what the
/// `switch` falls out of rather than a case of its own. Until this parcel it
/// was assembled from a module of its own that th04_main.asm `include`d, and
/// that module's whole body was one expansion of the TASM macro
/// EXPLOSION_TYPED which th04/main/boss/explode_small.asm defines and still
/// expands for its own function. So this file
/// and th04/main/boss/explode_small.cpp are the same body twice, minus that
/// one's "use the second slot if the first is taken" test -- which is
/// precisely what a macro shared between two functions means.
///
/// TH05 has no counterpart: its boss_explode_big_circle()
/// (th05/main/boss/explode_big.cpp) takes no type and hardcodes the circle, so
/// th04/main/boss/boss.hpp declares the two differently and papers over the
/// difference with an inline wrapper for the shared call sites.
///
/// Not compiled on its own: th04/boss_4m.cpp #includes this file AHEAD of
/// th04/main/boss/b4m.cpp, because within one object code is emitted in source
/// order and in the original this function was the last thing th04_main.asm
/// contributed to B4M_UPDATE_TEXT, immediately above that object
/// (kb/codegen/0112 + 0114). No carve, no new segment, no group-list edit and
/// no Tupfile.lua line.
///
/// kb/codegen/0104: the `switch` is dense over 1..4 and compiles to a
/// `jmp word ptr cs:[bx + disp]` against a table placed after the epilogue.
/// The `cs:` frame is the GROUP, which is why th04/boss_4m.cpp now carries
/// `-zPmain_03` itself rather than leaving it to the file below.
///
/// kb/codegen/0119: this body is `0x78` bytes, EVEN, so prepending it moves
/// every object-local offset in th04/main/boss/b4m.cpp by an even amount and
/// neither of that file's two `#pragma option -a2` regions can change its
/// padding. That is the whole of the parity question here, and it is a
/// property of the length rather than of the route. Its own table lands at
/// object offset `0x70` under the default `-a1`, which needs no pad at any
/// parity.

#include "platform.h"
#include "pc98.h"
#include "th04/snd/snd.h"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/explode.hpp"

void pascal near boss_explode_big(unsigned int type)
{
	Explosion near *p = &explosions_big;

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
