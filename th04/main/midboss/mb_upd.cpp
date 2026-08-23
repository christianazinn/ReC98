/// MB_UPD_TEXT - the Stage 3 and Extra Stage midboss update code
/// -------------------------------------------------------------
/// (#included from th04/mb_upd.cpp, MB_UPD_TEXT's SECOND C++ object. The
/// Stage 1 midboss's half of the segment is in th04/mb_upd1.cpp ahead of it,
/// and that file records why the two are separate objects.)
///
/// Together the two are the WHOLE of the segment: the root dump contributes
/// nothing to it any more. ZUN's object held midboss1_update() and its pellet
/// helper, then the Stage 3 midboss's four patterns and its update function,
/// then the two flight helpers and the four bullet patterns of the Extra Stage
/// midboss -- that one original object held several unrelated sources is
/// kb/codegen/0112, and th04/main/midboss/m3_updt.cpp keeps the per-midboss
/// split inside this object without paying for a third translation unit
/// (kb/codegen/0129). Most of this file's header closure has no include guard,
/// which is why that file has no #includes of its own; every file-scope name
/// in it is therefore prefixed.
///
/// Code is emitted in source order within an object, so the order below IS the
/// original address order.
///
/// midboss1_render(), midboss3_render() and midbossx_render() are all in
/// MIDBOSSX_TEXT and therefore in another object; see
/// th04/main/midboss/mx.cpp.

#include "platform.h"
#include "pc98.h"
// iatan2(), which the Stage 3 midboss's first pattern aims with.
#include "libs/master.lib/master.hpp"
#include "th02/v_colors.hpp"
// randring2_next16() is th02's; randring2_next16_and() is th03's, and that
// header pulls th02's, so this one include reaches both.
#include "th03/math/randring.hpp"
// polar_x() / polar_y(), which the two Extra Stage flight helpers orbit with.
#include "th03/math/polar.hpp"
#include "th04/snd/snd.h"
#include "th04/sprites/main_pat.h"
#include "th04/math/vector.hpp"
#include "th04/main/frames.h"
#include "th04/main/homing.hpp"
#include "th04/main/gather.hpp"
#include "th04/main/spark.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/item/item.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/player/player.hpp"
#include "th04/main/phase.hpp"
// Last, and that is load-bearing: th04/main/playfld.hpp behind it pulls
// th04/main/scroll.hpp, whose `#pragma codeseg mai_TEXT main_01` / bare
// `#pragma codeseg` pair has to run before this object emits any code.
// It also makes th04/main/scroll.hpp reachable without a second expansion of
// that unguarded header.
#include "th04/main/midboss/midboss.hpp"

#include "th04/main/midboss/m3_updt.cpp"

/// The Extra Stage midboss's two flight helpers
/// --------------------------------------------
/// Both orbit [midboss.pos] around a fixed center, and both pass
/// **[midboss.hp] as the orbit RADIUS** -- so the circle shrinks as the
/// midboss takes damage. That is a real quirk of ZUN's and not a misreading of
/// the argument order: polar()'s `pascal` arguments go left to right, so the
/// second push is the radius. No label is assigned; the taxonomy lane owns
/// that call for this family (th04/main/midboss/mx.cpp).
///
/// midbossx_update(), still ASM in the TAIL segment, is the only caller of
/// either, and reaches both through a `procdesc near` there -- both segments
/// are in the main_03 group, so those near calls' encodings never changed.
/// **A naming round is owed** for both address-suffixed spellings.

// The full circle, counter-clockwise.
extern "C" void near midbossx_146AF(void)
{
	midboss.pos.prev.x.v = midboss.pos.cur.x.v;
	midboss.pos.prev.y.v = midboss.pos.cur.y.v;
	midboss.pos.cur.x.v = polar_x(TO_SP(192), midboss.hp, midboss.angle);
	midboss.pos.cur.y.v = polar_y(TO_SP(96), midboss.hp, midboss.angle);

	// kb/codegen/0032: 0xFE rather than a subtraction of 2. On a wrapping
	// 8-bit angle the two are the same value, but the subtraction promotes and
	// comes out as a same-length SUB where the original has an ADD.
	_AL = midboss.angle;
	_AL += 0xFE;
	midboss.angle = _AL;
}

// Sine on the vertical axis only, with a horizontal sweep that bounces off
// both playfield edges -- and stops bouncing from phase 6 on, which is how the
// midboss leaves through the side.
extern "C" void near midbossx_14700(void)
{
	midboss.pos.prev.x.v = midboss.pos.cur.x.v;
	midboss.pos.prev.y.v = midboss.pos.cur.y.v;
	if(midboss.phase_frame == 1) {
		midboss.pos.velocity.x.v = TO_SP(1);
		midboss.angle = 0;
	}
	midboss.pos.cur.x.v += midboss.pos.velocity.x.v;
	if(midboss.phase <= 5) {
		if(
			(midboss.pos.cur.x.v <= TO_SP(16)) ||
			(midboss.pos.cur.x.v >= TO_SP(368))
		) {
			midboss.pos.velocity.x.v *= -1;
		}
	}
	midboss.pos.cur.y.v = polar_y(TO_SP(96), midboss.hp, midboss.angle);
	_AL = midboss.angle;
	_AL += 2;
	midboss.angle = _AL;
}
/// --------------------------------------------

/// The Extra Stage midboss's every-16th-frame bullet patterns
/// ----------------------------------------------------------
/// All four run one frame of a pattern on every sixteenth stage frame and do
/// nothing on the other fifteen; that one guard is the whole of each function.
/// Reached from midbossx_update() the same way the two helpers above are.
///
/// **A naming round is owed** for all four address-suffixed spellings.

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
/// ----------------------------------------------------------
