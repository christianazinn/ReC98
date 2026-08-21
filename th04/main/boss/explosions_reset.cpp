/// Freeing the small boss explosions
/// ---------------------------------
/// Called from boss_reset(). Two byte stores and a standard frame; the `far`
/// convention is the large model's default, and TH04 reaches the very same
/// function through hand-written `nop` / `push cs` / near call instead
/// (kb/codegen/0083, spelled out in th04/main/boss/reset.cpp).
///
/// Like th04/main/boss/explode_small.cpp, this body is shared and identical in
/// both games, and both now compile it. In TH04 the same code was assembled
/// from a module of its own until the lift below it promoted that module to
/// the tail of its segment's root contribution.
///
/// Not compiled on its own: th05/gather.cpp and th04/expl_sm.cpp both
/// #include this file at the FRONT of their objects, above everything lifted
/// out of the same tail before it (kb/codegen/0112 + 0114).

#include "platform.h"
#include "pc98.h"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/explode.hpp"

void explosions_small_reset(void)
{
	explosions_small[0].alive = false;
	explosions_small[1].alive = false;
}
