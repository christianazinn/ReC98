/// Freeing the small boss explosions
/// ---------------------------------
/// Called from boss_reset(). Two byte stores and a standard frame; the `far`
/// convention is the large model's default, and TH04 reaches the very same
/// function through hand-written `nop` / `push cs` / near call instead
/// (kb/codegen/0083, spelled out in th04/main/boss/reset.cpp).
///
/// Like th04/main/boss/explode_small.cpp, this body is shared and identical in
/// both games but only TH05 compiles it: in TH04 the same code is still
/// assembled from th04/main/boss/explosions_reset.asm, which is not the tail of
/// its segment's root contribution there, so the module stays in the tree and
/// TH04's dump keeps including it.
///
/// Not compiled on its own: th05/gather.cpp #includes this file at the FRONT of
/// its object, above everything lifted out of the same tail before it
/// (kb/codegen/0112 + 0114).

#include "platform.h"
#include "pc98.h"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/explode.hpp"

void explosions_small_reset(void)
{
	explosions_small[0].alive = false;
	explosions_small[1].alive = false;
}
