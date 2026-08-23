/// Extra Stage midboss - the every-16th-frame ring pattern
/// -------------------------------------------------------
/// (#included from th04/mb_upd.cpp, which is its own object naming the head
/// half of the original ENM_POS_TEXT. See that file.)
///
/// This was the last emitting item of MB_UPD_TEXT's root block, which is the
/// whole reason it is the one that could go first: everything above it is still
/// ASM, and this object grows backwards into the hole (kb/codegen/0099 + 0112).
/// Its one caller, midbossx_update(), is ASM in the TAIL segment and reaches it
/// through a `procdesc near` there -- both segments are in the main_03 group,
/// so the near call's encoding never changed.
///
/// **A naming round is owed.** The function keeps the dump's address-suffixed
/// spelling: it is one of midbossx_update()'s unnamed pattern helpers, and the
/// twelve above it in this segment belong to midboss 1 and 3, so nothing here
/// settles what to call it.

#include "platform.h"
#include "pc98.h"
#include "th02/math/randring.hpp"
#include "th04/main/frames.h"
#include "th04/main/bullet/bullet.hpp"

extern "C" void near midbossx_14828(void)
{
	if(stage_frame_mod16 == 0) {
		bullet_template.group = BG_RING;
		bullet_template.count = 32;
		bullet_template.angle = randring2_next16();
		bullet_template.speed.v = TO_SP(2);
		bullet_template_tune();
		bullets_add_regular();
		bullet_template.group = BG_RING;
		bullet_template.count = 16;
		bullet_template.angle = randring2_next16();
		bullet_template.speed.v = TO_SP(3);
		bullet_template_tune();
		bullets_add_regular();
	}
}
