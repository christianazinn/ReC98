/// Extra Stage midboss - her every-16th-frame bullet patterns
/// ----------------------------------------------------------
/// (#included from th04/mb_upd.cpp, which is its own object naming the head
/// half of the original ENM_POS_TEXT. See that file.)
///
/// These were the bottom of MB_UPD_TEXT's root block, in this order, which is
/// the whole reason they could go first: everything above them is still ASM,
/// and this object grows backwards into the hole (kb/codegen/0099 + 0112).
/// midbossx_update() is the ONLY caller of any of them, it is ASM in the TAIL
/// segment, and it reaches each through a `procdesc near` there -- both
/// segments are in the main_03 group, so the near calls' encodings never
/// changed.
///
/// All four run one frame of a pattern on every sixteenth stage frame and do
/// nothing on the other fifteen; the `if` is the whole of each function.
///
/// **A naming round is owed** for all four address-suffixed spellings. They are
/// midbossx_update()'s unnamed pattern helpers, and the eleven procs still
/// above them in this segment belong to midbosses 1 and 3, so nothing here
/// settles what to call them.

#include "platform.h"
#include "pc98.h"
// randring2_next16() is th02's; randring2_next16_and() is th03's, and that
// header pulls th02's, so this one include reaches both.
#include "th03/math/randring.hpp"
#include "th04/snd/snd.h"
#include "th04/sprites/main_pat.h"
#include "th04/main/frames.h"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/midboss/midboss.hpp"

extern "C" void near midbossx_1476F(void)
{
	if(stage_frame_mod16 == 0) {
		bullet_template.group = BG_RING;
		bullet_template.count = 32;
		bullet_template.angle = randring2_next16();
		bullet_template.speed.v = TO_SP(3);
		bullet_template_tune();
		bullets_add_regular();
	}
}

// The one whose speed ramps: the longer the phase has been running, the faster
// the ring comes out.
extern "C" void near midbossx_14798(void)
{
	if(stage_frame_mod16 == 0) {
		bullet_template.spawn_type = BST_BULLET16_CLOUD_BACKWARDS;
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.group = BG_RING;
		bullet_template.count = 32;
		bullet_template.angle = randring2_next16();
		bullet_template.speed.v = ((midboss.phase_frame / 4) + 10);
		bullet_template_tune();
		bullets_add_regular();
		snd_se_play(3);
	}
}

extern "C" void near midbossx_147DB(void)
{
	if(stage_frame_mod16 == 0) {
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.special_motion = BSM_BOUNCE_LEFT_RIGHT_TOP;
		bullet_template.group = BG_SPREAD;
		bullet_template.count = 5;
		bullet_template.delta.spread_angle = 0x0C;
		bullet_template.patnum = PAT_BULLET16_N_CROSS_YELLOW;
		bullet_template.speed.v = TO_SP(2);

		// kb/codegen/0032: the original adds the minimum to the returned byte
		// in AL and then stores it. The immediate has to be spelled as the
		// ADDITION 0xB8, even though IDA renders the original's operand as a
		// negative byte: on a wrapping 8-bit angle the two are the same value,
		// but subtracting 0x48 promotes and comes out as a same-length
		// subtract instruction instead of an add. 0xB8 is 8 short of straight
		// down, so the 16-value spread is centred there.
		_AL = randring2_next16_and(0x0F);
		_AL += 0xB8;
		bullet_template.angle = _AL;

		bullet_template_tune();
		bullet_special.turns_max = 2;
		bullets_add_special();
		snd_se_play(9);
	}
}

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
