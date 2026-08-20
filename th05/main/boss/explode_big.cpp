/// The boss's big circle explosion
/// -------------------------------
/// Spawns the single large explosion at whatever [boss.pos.cur] happens to be.
/// It takes no position and no type: TH05 hardcoded the circle here, which is
/// why the secondary boss needs the save/restore wrapper in
/// th05/main/boss/2_explode_big.cpp, and why TH04's function of nearly the same
/// name -- which takes an explosion_type_t and shares TASM's EXPLOSION_TYPED
/// macro with boss_explode_small() -- has no body in common with this one.
/// th04/main/boss/boss.hpp papers over the signature difference with an inline
/// wrapper for the shared TH04/TH05 call sites.
///
/// Not compiled on its own: th05/gather.cpp #includes this file at the FRONT of
/// its object, above th05/main/boss/2_explode_big.cpp. Within one object, code
/// is emitted in source order, and in the original this function sat
/// immediately before that one, as the last thing th05_main.asm contributed to
/// main_032_TEXT (kb/codegen/0112 + 0114).
///
/// kb/codegen/0117: [p] is ONE source local. It earns SI by being dereferenced
/// nine times, and the original's frame -- `push bp` / `mov bp, sp` /
/// `push si`, with nothing reserved -- confirms that no second, stack-homed
/// local exists.

#include "platform.h"
#include "pc98.h"
#include "th02/snd/snd.h"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/explode.hpp"

void near boss_explode_big_circle(void)
{
	Explosion near *p = &explosions_big;

	p->alive = true;
	p->age = 0;
	p->center.x.v = boss.pos.cur.x.v;
	p->center.y.v = boss.pos.cur.y.v;
	p->radius_cur.x.v = 8;
	p->radius_cur.y.v = 8;
	p->radius_delta.x.v = TO_SP(12);
	p->radius_delta.y.v = TO_SP(12);
	p->angle_offset = 0x00;
	snd_se_play(12);
}
