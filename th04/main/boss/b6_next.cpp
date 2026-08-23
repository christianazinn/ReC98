/// Extra Stage Boss - Yuuka: her animations, her seventeen patterns, and
/// ending a phase
/// ---------------------------------------------------------------------
/// (#included from th04/b6_next.cpp, which is its own object for the reason
/// that file gives.)
///
/// Everything here sat in main_034_TEXT, in exactly this order, and this file
/// is now that segment's ENTIRE contents: the root dump contributes nothing to
/// it at all. The two functions at the bottom came first, in the parcels that
/// opened this object; the seventeen above them are the rest of the dump's
/// contribution below the animation module; the eight at the top are that
/// module, which was the root's last remaining block.
///
/// The five `static` ones are Yuuka's shared gather animations, called only
/// from the twelve patterns below them. The twelve are called from
/// yuuka6_update()'s `switch(boss.mode)` chains in th04/main/boss/b6_upd.cpp,
/// another object, so they keep C linkage and the dump's twelve zero-byte
/// `label near` aliases (kb/codegen/0123) are gone with the bodies.
/// **A naming round is owed** for all seventeen address-suffixed spellings.

#include "platform.h"
#include "pc98.h"
// iatan2(), which the shield pattern aims its opening ring with.
#include "libs/master.lib/master.hpp"
#include "th02/v_colors.hpp"
#include "th02/main/player/player.hpp"
#include "th04/snd/snd.h"
#include "th04/sprites/main_pat.h"
#include "th04/math/randring.hpp"
#include "th04/main/rank.hpp"
#include "th04/main/frames.h"
#include "th04/main/circle.hpp"
#include "th04/main/gather.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/bullet/laser_t.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/explode.hpp"
// vector2_at(), which one pattern spawns its mirrored pellet pair with, and
// shots_hittest(). Safe to include HERE, unlike in th04/main/boss/b6_upd.cpp:
// that file shares a translation unit with th04/main/boss/b3_upd.cpp, which
// includes this header LAST on purpose because every function below it was
// matched against the declarations visible in that order. This object shares a
// TU with nothing.
#include "th04/math/vector.hpp"
#include "th04/main/player/shot.hpp"
// MATCH-TH04-MAIN-034-CHAIN's five bodies, at the end of this file, need these
// five as well. Appended rather than sorted in, for the reason the comment
// above gives about the two before them: every function already in this object
// was matched against the declarations visible in the original order, and all
// five of these are guarded or declaration-only.
#include "th03/math/polar.hpp"
#include "th04/main/score.hpp"
#include "th04/main/spark.hpp"
#include "th04/main/item/item.hpp"
// Yuuka's two [custom_entities] overlays, split out of th04/main/boss/b6.cpp
// (which this object still cannot include -- it declares yuuka6_phase_next()
// with C++ linkage and this object DEFINES it `extern "C"`).
#include "th04/main/boss/b6ent.hpp"

/// Still ASM
/// ---------
extern "C" {
	// th04_main.asm `.data?`, shared with th04/main/boss/b6_upd.cpp.
	extern int yuuka6_anim_frame;
	extern unsigned char yuuka6_sprite_flag;

	// Also th04_main.asm `.data?`, and both still WRITTEN from ASM in
	// another segment, so this parcel published them rather than retiring
	// a publication (kb/codegen/0123). A naming round is owed for both;
	// they keep the dump's address-suffixed spellings.
	extern PlayfieldPoint yuuka6_25A0C;
	extern unsigned char yuuka6_25A1E;
	extern unsigned char yuuka6_25A1B;
	extern unsigned char yuuka6_25A08;

	// Which of PHASE2_FLY_PATHS Yuuka takes this fight; th04_main.asm still
	// picks it, and yuuka6_phase2_fly() at the end of this file is its only
	// reader now.
	extern unsigned char yuuka6_phase2_fly_path;

	// The safety circle Yuuka opens around the player on the one frame her
	// forward-parasol phase starts. It sat ABOVE the animation `include` that
	// used to end the root's contribution, so it could not follow the seventeen
	// out and the parcel that lifted them gave it a zero-byte `label near`
	// alias to keep it linkable (kb/codegen/0123). It is C++ now, in
	// th04/main/boss/b6_spawn.cpp, and the naming debt that alias recorded is
	// discharged: the address-suffixed placeholder it carried is gone with the
	// body it named. The `codeseg` pair is not
	// decoration: MATCH-TH04-MAIN-034-HEAD carved it into B6_SPAWN_TEXT, and
	// two declarations of one function under two different code segments is a
	// defect that links, runs and shows up only in the map (kb/codegen/0155).
#pragma codeseg B6_SPAWN_TEXT main_03
	void near safetycircle_open(void);
#pragma codeseg

	// Copies [thicklaser_template] into the first free slot and plays the
	// spawn sound effect. It is still ASM, and in B4M_UPDATE_TEXT rather
	// than this segment -- but the same main_03 group, so the call stays
	// near. th04/main/boss/bx2_upd.cpp gave it its alias and records the
	// naming debt for it.
	void near thicklaser_add(void);

	// th04/main/bullet/update[bss].asm, where it is one of three `label byte`
	// overlays on the single `_bullet_special` byte. ZUN published the byte
	// but not this name for it, so this parcel added the `public`; it emits
	// no bytes.
	extern unsigned char bullet_special_speed_delta;
}

// The one that is not `extern "C"`: the dump published it under the C++
// mangled name that a `pascal` function gets, which is the whole evidence for
// its calling convention. th04/main/boss/b6.cpp declares it identically and is
// not included here for the reason th04/main/boss/b6_upd.cpp gives, so this
// restatement has to carry b6.cpp's `codeseg` binding as well -- it was at the
// head of THIS segment until MATCH-TH04-MAIN-034-HEAD carved that head off
// into B6_SPAWN_TEXT (kb/codegen 0080 + 0155).
#pragma codeseg B6_SPAWN_TEXT main_03
void pascal near chasecrosses_add(
	unsigned char angle, subpixel_length_8_t speed
);
#pragma codeseg

// th04/main/boss/b6.cpp's yuuka6_sprite_flag_t, restated as the four
// enumerators this file needs rather than included: that file declares
// Yuuka's whole fight and is expanded by th04/main/boss/bg.cpp, another
// object, in the same way th04/main/boss/b6_upd.cpp restates it.
static const int Y6SF_VANISHED = 0;
static const int Y6SF_PARASOL_BACK_OPEN = 1;
static const int Y6SF_PARASOL_BACK_CLOSED = 2;
static const int Y6SF_PARASOL_FORWARD = 3;
static const int Y6SF_PARASOL_LEFT = 4;
static const int Y6SF_PARASOL_SHIELD = 8;

// The three `boss_statebyte` slots Yuuka's Extra fight uses, spelled the way
// th04_main.asm's own `boss_statebyte_t` overlay already names them.
#define yuuka6_thicklaser_radius   boss_statebyte[0]
#define yuuka6_spin_ring_points    boss_statebyte[1]
#define yuuka6_spread_angle_range  boss_statebyte[15]
/// ---------

/// Yuuka's sprite animations
/// -------------------------
/// The eight functions the animation module under th04/main/boss/ used to be
/// -- named here without a code span because this parcel deleted it -- in the
/// order they had there. They were the whole of main_034_TEXT's remaining root
/// contribution, so lifting them drains that block to nothing.
///
/// Each one advances [yuuka6_anim_frame] and picks the cel for the frame it
/// just reached, returning `true` on the last one and resetting the counter.
/// The `switch` is on the global rather than on a copy of it: seven of the
/// eight compile to Borland's sparse-switch scan, which needs the selector in
/// memory while the loop walks the value table through AX, so `enter 2, 0`
/// allocates the compiler's own temporary and there is no local here to
/// declare (kb/codegen/0132). yuuka6_anim_parasol_left_spin_back() is the one
/// dense switch, and it takes a plain `push bp` / `mov bp, sp` frame for
/// exactly that reason -- a jump table indexed straight off the selector never
/// spills it.
///
/// [yuuka6_sprite_flag] is set to the state the animation is LEAVING on the
/// two that have a state to leave, and to the state it reached on all eight.
/// Nothing here reads it; th04/main/boss/b6_upd.cpp does.

#pragma option -a2
extern "C" bool near yuuka6_anim_parasol_back_close(void)
{
	yuuka6_sprite_flag = Y6SF_PARASOL_BACK_OPEN;
	yuuka6_anim_frame++;
	switch(yuuka6_anim_frame) {
	case 1:
		boss.sprite = PAT_YUUKA6_PARASOL_BACK_OPEN;
		break;
	case 7:
		boss.sprite = PAT_YUUKA6_PARASOL_BACK_HALFOPEN;
		break;
	case 13:
		boss.sprite = PAT_YUUKA6_PARASOL_BACK_HALFCLOSED;
		break;
	case 19:
		boss.sprite = PAT_YUUKA6_PARASOL_BACK_CLOSED;
		break;
	case 25:
		yuuka6_anim_frame = 0;
		yuuka6_sprite_flag = Y6SF_PARASOL_BACK_CLOSED;
		return true;
	}
	return false;
}

extern "C" bool near yuuka6_anim_parasol_back_open(void)
{
	yuuka6_sprite_flag = Y6SF_PARASOL_BACK_CLOSED;
	yuuka6_anim_frame++;
	switch(yuuka6_anim_frame) {
	case 1:
		boss.sprite = PAT_YUUKA6_PARASOL_BACK_CLOSED;
		break;
	case 7:
		boss.sprite = PAT_YUUKA6_PARASOL_BACK_HALFCLOSED;
		break;
	case 13:
		boss.sprite = PAT_YUUKA6_PARASOL_BACK_HALFOPEN;
		break;
	case 19:
		boss.sprite = PAT_YUUKA6_PARASOL_BACK_OPEN;
		break;
	case 25:
		yuuka6_anim_frame = 0;
		yuuka6_sprite_flag = Y6SF_PARASOL_BACK_OPEN;
		return true;
	}
	return false;
}

// The only one that leaves [yuuka6_sprite_flag] alone on the way in: the state
// it comes from is always the closed-back one the caller just finished.
extern "C" bool near yuuka6_anim_parasol_back_pull_forward(void)
{
	yuuka6_anim_frame++;
	switch(yuuka6_anim_frame) {
	case 1:
		boss.sprite = PAT_YUUKA6_PARASOL_BACK_CLOSED;
		break;
	case 7:
		boss.sprite = PAT_YUUKA6_PARASOL_LEFT_PULL;
		break;
	case 13:
		boss.sprite = PAT_YUUKA6_PARASOL_LEFT_FORWARD_PULL;
		break;
	case 19:
		boss.sprite = PAT_YUUKA6_PARASOL_FORWARD_CLOSED;
		break;
	case 25:
		yuuka6_anim_frame = 0;
		yuuka6_sprite_flag = Y6SF_PARASOL_FORWARD;
		return true;
	}
	return false;
}

extern "C" bool near yuuka6_anim_parasol_back_pull_left(void)
{
	yuuka6_anim_frame++;
	switch(yuuka6_anim_frame) {
	case 1:
		boss.sprite = PAT_YUUKA6_PARASOL_BACK_CLOSED;
		break;
	case 7:
		boss.sprite = PAT_YUUKA6_PARASOL_LEFT_PULL;
		break;
	case 13:
		boss.sprite = PAT_YUUKA6_PARASOL_LEFT;
		break;
	case 19:
		yuuka6_anim_frame = 0;
		yuuka6_sprite_flag = Y6SF_PARASOL_LEFT;
		return true;
	}
	return false;
}

// The long one, and the only dense switch: 40 frames, a cel every third one,
// so 27 of the jump table's 40 entries are the `default` label.
extern "C" bool near yuuka6_anim_parasol_left_spin_back(void)
{
	yuuka6_anim_frame++;
	switch(yuuka6_anim_frame) {
	case 1:
		boss.sprite = PAT_YUUKA6_PARASOL_LEFT;
		break;
	case 4:
		boss.sprite = PAT_YUUKA6_PARASOL_LEFT_FORWARD_PULL;
		break;
	case 7:
		boss.sprite = PAT_YUUKA6_PARASOL_SPIN_BACK_0;
		break;
	case 10:
		boss.sprite = PAT_YUUKA6_PARASOL_SPIN_BACK_1;
		break;
	case 13:
		boss.sprite = PAT_YUUKA6_PARASOL_SPIN_BACK_2;
		break;
	case 16:
		boss.sprite = PAT_YUUKA6_PARASOL_SPIN_BACK_3;
		break;
	case 19:
		boss.sprite = PAT_YUUKA6_PARASOL_SPIN_BACK_4;
		break;
	case 22:
		boss.sprite = PAT_YUUKA6_PARASOL_SPIN_BACK_5;
		break;
	case 25:
		boss.sprite = PAT_YUUKA6_PARASOL_SPIN_BACK_6;
		break;
	case 28:
		boss.sprite = PAT_YUUKA6_PARASOL_SPIN_BACK_7;
		break;
	case 31:
		boss.sprite = PAT_YUUKA6_PARASOL_SPIN_BACK_8;
		break;
	case 34:
		boss.sprite = PAT_YUUKA6_PARASOL_SPIN_BACK_9;
		break;
	case 37:
		boss.sprite = PAT_YUUKA6_PARASOL_BACK_CLOSED;
		break;
	case 40:
		yuuka6_anim_frame = 0;
		yuuka6_sprite_flag = Y6SF_PARASOL_BACK_CLOSED;
		return true;
	}
	return false;
}

extern "C" bool near yuuka6_anim_vanish(void)
{
	yuuka6_anim_frame++;
	switch(yuuka6_anim_frame) {
	case 1:
		boss.sprite = PAT_YUUKA6_VANISH_0;
		break;
	case 8:
		boss.sprite = PAT_YUUKA6_VANISH_1;
		break;
	case 15:
		boss.sprite = PAT_YUUKA6_VANISH_2;
		break;
	case 22:
		boss.sprite = PAT_YUUKA6_VANISH_3;
		break;
	case 29:
		boss.sprite = 0;
		break;
	case 36:
		yuuka6_anim_frame = 0;
		yuuka6_sprite_flag = Y6SF_VANISHED;
		return true;
	}
	return false;
}

// vanish() backwards, ending on the open parasol rather than on nothing.
extern "C" bool near yuuka6_anim_appear(void)
{
	yuuka6_anim_frame++;
	switch(yuuka6_anim_frame) {
	case 1:
		boss.sprite = PAT_YUUKA6_VANISH_3;
		break;
	case 8:
		boss.sprite = PAT_YUUKA6_VANISH_2;
		break;
	case 15:
		boss.sprite = PAT_YUUKA6_VANISH_1;
		break;
	case 22:
		boss.sprite = PAT_YUUKA6_VANISH_0;
		break;
	case 29:
		boss.sprite = PAT_YUUKA6_PARASOL_BACK_OPEN;
		break;
	case 36:
		yuuka6_anim_frame = 0;
		yuuka6_sprite_flag = Y6SF_PARASOL_BACK_OPEN;
		return true;
	}
	return false;
}

extern "C" bool near yuuka6_anim_parasol_shield(void)
{
	yuuka6_anim_frame++;
	switch(yuuka6_anim_frame) {
	case 1:
		boss.sprite = PAT_YUUKA6_PARASOL_BACK_OPEN;
		break;
	case 7:
		boss.sprite = PAT_YUUKA6_PARASOL_SHIELD_0;
		break;
	case 13:
		boss.sprite = PAT_YUUKA6_PARASOL_SHIELD_1;
		break;
	case 19:
		yuuka6_anim_frame = 0;
		yuuka6_sprite_flag = Y6SF_PARASOL_SHIELD;
		return true;
	}
	return false;
}
#pragma option -a1
/// ---------


/// Yuuka's shared gather animations
/// --------------------------------
/// Four wind-up animations, one per pattern group, each keyed on
/// [boss.phase_frame] with a sparse switch statement -- which is what the
/// value/jump table pair behind each of them is, and what the one padding byte
/// in front of three of those four pairs is (kb/codegen/0160). The first pair
/// needs no pad, because it lands on an even offset in this object; ZUN's
/// object started at the same place, so the parities carry over unchanged.

#pragma option -a2
// Two gathers a parasol's width apart, spinning against each other, for the
// phases Yuuka fires from both her own position and her mirror's.
static void near yuuka6_1A907(void)
{
	switch(boss.phase_frame) {
	case 16:
		snd_se_play(8);
		gather_template.radius.v = TO_SP(320);
		gather_template.center.y.v = (boss.pos.cur.y.v + TO_SP(-4));
		gather_template.center.x.v = (boss.pos.cur.x.v + TO_SP(24));
		gather_template.ring_points = 16;
		gather_template.col = 9;
		gather_template.angle_delta = -2;
		// fall through
	case 20:
		gather_add_only();
		gather_template.center.x.v -= TO_SP(44);
		gather_template.angle_delta = 2;
mirror:
		gather_add_only();
		break;

	case 18:
		gather_template.col = 8;
		gather_add_only();
		gather_template.center.x.v += TO_SP(44);
		gather_template.angle_delta = -2;
		goto mirror;

	case 32:
		circles_add_shrinking(
			gather_template.center.x.v, gather_template.center.y.v
		);
		circles_add_shrinking(
			(gather_template.center.x.v + TO_SP(44)),
			gather_template.center.y.v
		);
		circles_color = V_WHITE;
		break;
	}
}

// One ring each way, so the two cross. The same shape Elly's patterns use.
static void near yuuka6_1A9B5(void)
{
	gather_template.angle_delta = -2;
	gather_add_only();
	gather_template.angle_delta = 2;
	gather_add_only();
}

static void near yuuka6_1A9CA(void)
{
	switch(boss.phase_frame) {
	case 48:
		snd_se_play(8);
		gather_template.radius.v = TO_SP(320);
		gather_template.center.y.v = (boss.pos.cur.y.v + TO_SP(32));
		gather_template.center.x.v = boss.pos.cur.x.v;
		gather_template.ring_points = 8;
		gather_template.col = 9;
		// fall through
	case 52:
crossed:
		yuuka6_1A9B5();
		break;

	case 50:
		gather_template.col = 8;
		goto crossed;

	case 64:
		circles_add_shrinking(
			gather_template.center.x.v, gather_template.center.y.v
		);
		circles_color = V_WHITE;
		break;
	}
}

// The two-position variant: everything is done once at Yuuka's own position
// and once at the mirror's.
static void near yuuka6_1AA45(void)
{
	switch(boss.phase_frame) {
	case 32:
		snd_se_play(8);
		gather_template.radius.v = TO_SP(320);
		gather_template.ring_points = 8;
		gather_template.col = 9;
		// fall through
	case 36:
crossed:
		gather_template.center.y.v = (boss.pos.cur.y.v + TO_SP(32));
		gather_template.center.x.v = boss.pos.cur.x.v;
		yuuka6_1A9B5();
		gather_template.center.y.v = (yuuka6_25A0C.y.v + TO_SP(32));
		gather_template.center.x.v = yuuka6_25A0C.x.v;
		yuuka6_1A9B5();
		break;

	case 34:
		gather_template.col = 8;
		goto crossed;

	case 48:
		circles_add_shrinking(
			boss.pos.cur.x.v, (boss.pos.cur.y.v + TO_SP(32))
		);
		circles_add_shrinking(
			yuuka6_25A0C.x.v, (yuuka6_25A0C.y.v + TO_SP(32))
		);
		circles_color = V_WHITE;
		break;
	}
}

static void near yuuka6_1AAE5(void)
{
	switch(boss.phase_frame) {
	case 16:
		snd_se_play(8);
		gather_template.radius.v = TO_SP(320);
		gather_template.center.y.v = boss.pos.cur.y.v;
		gather_template.center.x.v = boss.pos.cur.x.v;
		gather_template.ring_points = 16;
		gather_template.col = 7;
		// fall through
	case 20:
crossed:
		yuuka6_1A9B5();
		break;

	case 18:
		gather_template.col = 6;
		goto crossed;

	case 32:
		circles_add_shrinking(
			gather_template.center.x.v, gather_template.center.y.v
		);
		circles_color = V_WHITE;
		break;
	}
}
#pragma option -a1
/// --------------------------------

/// Phase 2's three patterns
/// ------------------------

// Two 20-bullet rings of yellow crosses that decelerate and then turn a
// quarter each way, one per position, getting faster with every repetition.
extern "C" void near yuuka6_1AB5D(void)
{
	if(yuuka6_sprite_flag == Y6SF_PARASOL_BACK_OPEN) {
		yuuka6_anim_parasol_back_close();
	}
	yuuka6_1A907();
	switch(boss.phase_frame) {
	case 48:
		bullet_template.speed.v = (TO_SP(2) + 8);
		// fall through
	case 64:
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.patnum = PAT_BULLET16_N_CROSS_YELLOW;
		bullet_template.origin.x.v = boss.pos.cur.x.v;
		bullet_template.origin.y.v = (boss.pos.cur.y.v + TO_SP(-4));
		bullet_template.angle = 0;
		bullet_template.group = BG_RING;
		bullet_template.special_motion = BSM_DECELERATE_THEN_TURN;
		bullet_template.count = 20;
		bullet_template_special_angle.turn_by = -0x40;
		bullet_template_tune();
		bullets_add_special_fixedspeed();
		bullet_template_special_angle.turn_by = 0x40;
		bullets_add_special_fixedspeed();
		snd_se_play(9);
		_AL = bullet_template.speed.v;
		_AL += TO_SP(1);
		bullet_template.speed.v = _AL;
		break;

	case 80:
		boss.phase_frame = 0;
		boss.mode = 255;
		break;
	}
}

// A pellet ring fired from a walking angle and its mirror image about the
// vertical, both spawned a parasol's length out from Yuuka herself.
extern "C" void near yuuka6_1ABE5(void)
{
	unsigned char angle;

	if(boss.phase_frame < 48) {
		if(yuuka6_sprite_flag != Y6SF_PARASOL_BACK_CLOSED) {
			goto ending;
		}
		yuuka6_anim_parasol_back_pull_left();
		goto ending;
	}
	if(yuuka6_sprite_flag != Y6SF_PARASOL_LEFT) {
		goto ending;
	}
	yuuka6_anim_parasol_left_spin_back();
	if(boss.phase_frame > 80) {
		goto ending;
	}
	bullet_template.spawn_type = BST_PELLET;
	bullet_template.group = BG_RING;
	bullet_template.count = yuuka6_spin_ring_points;
	_AL = boss.phase_frame;
	_AL <<= 3;
	_DL = 0;
	_DL -= _AL;
	angle = _DL;
	bullet_template.speed.v = (TO_SP(2) + 8);
	bullet_template.angle = angle;
	vector2_at(
		bullet_template.origin,
		boss.pos.cur.x.v,
		boss.pos.cur.y.v,
		TO_SP(34),
		angle
	);
	bullets_add_regular();
	_AL = 0x80;
	_AL -= angle;
	angle = _AL;
	// `_AL`, not `angle`: the original never reloads the byte it just stored,
	// and passing the variable makes Turbo C++ do exactly that -- three bytes
	// that the identical statement one call up does NOT cost, because there
	// the last write to AL was a LOAD of [angle] rather than a store to it.
	vector2_at(
		bullet_template.origin,
		boss.pos.cur.x.v,
		boss.pos.cur.y.v,
		TO_SP(34),
		_AL
	);
	// Order matters and is not the obvious one: the division is emitted first
	// and only then the constant, so the subtraction has to be its own
	// statement against a second pseudo-register.
	_AL = (boss.phase_frame / 2);
	_DL = (TO_SP(3) + 12);
	_DL -= _AL;
	bullet_template.speed.v = _DL;
	bullets_add_regular();
	if(stage_frame_mod4 == 0) {
		snd_se_play(9);
	}
ending:
	switch(boss.phase_frame) {
	case 32:
		circles_add_shrinking(
			(boss.pos.cur.x.v + TO_SP(-40)), (boss.pos.cur.y.v + TO_SP(40))
		);
		circles_color = V_WHITE;
		break;

	case 96:
		boss.phase_frame = 0;
		boss.mode = 255;
		break;
	}
}

// Falling red balls from both positions, in one of the four bullet types that
// the current frame picks.
extern "C" void near yuuka6_1ACCC(void)
{
	if(yuuka6_sprite_flag == Y6SF_PARASOL_BACK_CLOSED) {
		yuuka6_anim_parasol_back_open();
	}
	if((boss.phase_frame >= 48) && (boss.phase_frame <= 80)) {
		if(stage_frame_mod4 != 0) {
			goto gather;
		}
		bullet_template.spawn_type = ((stage_frame_mod8 / 4) + 1);
		bullet_template.patnum = PAT_BULLET16_N_SMALL_BALL_RED;
		bullet_template.origin.y.v = (boss.pos.cur.y.v + TO_SP(-4));
		bullet_template.group = BG_RANDOM_ANGLE_AND_SPEED;
		bullet_template.count = 4;
		bullet_template.special_motion = BSM_GRAVITY;
		_AL = randring2_next16_mod(TO_SP(1) + 8);
		_AL += 8;
		bullet_template.speed.v = _AL;
		bullet_template.origin.x.v = (boss.pos.cur.x.v + TO_SP(-20));
		bullet_template.angle = randring2_next16();
		bullet_special_speed_delta = 1;
		bullet_template_tune();
		bullets_add_special_fixedspeed();
		bullet_template.angle = randring2_next16();
		bullet_template.origin.x.v += TO_SP(44);
		bullets_add_special_fixedspeed();
		snd_se_play(9);
		goto gather;
	}
	if(boss.phase_frame > 80) {
		boss.phase_frame = 0;
		boss.mode = 255;
	}
gather:
	yuuka6_1A907();
}
/// ------------------------

// Phase 4: nothing of its own. The parasol turns forward, and the safety
// circle opens around the player on the frame that starts it.
extern "C" void near yuuka6_1AD6F(void)
{
	if(boss.phase_frame < 64) {
		if(yuuka6_sprite_flag == Y6SF_PARASOL_BACK_OPEN) {
			yuuka6_anim_parasol_back_close();
			goto gather;
		}
		if(yuuka6_sprite_flag != Y6SF_PARASOL_BACK_CLOSED) {
			goto gather;
		}
		yuuka6_anim_parasol_back_pull_forward();
		goto gather;
	}
	if((boss.phase_frame >= 64) && (boss.phase_frame <= 112)) {
		boss.sprite = PAT_YUUKA6_PARASOL_FORWARD_OPEN;
		goto gather;
	}
	if(boss.phase_frame < 288) {
		goto gather;
	}
	if(yuuka6_sprite_flag != Y6SF_PARASOL_BACK_CLOSED) {
		yuuka6_anim_parasol_left_spin_back();
		goto gather;
	}
	if(yuuka6_anim_parasol_back_open()) {
		boss.phase_frame = 0;
		boss.mode = 255;
	}
gather:
	yuuka6_1A9CA();
	if(boss.phase_frame == 64) {
		safetycircle_open();
	}
}

// The background pattern that runs underneath the two thick-laser phases: an
// aimed 7-stack every 16 frames, or a 16-bullet ring plus a growing forward
// cloud once the fight reaches phase 6.
extern "C" void near yuuka6_1ADDB(void)
{
	if(stage_frame_mod16 != 0) {
		return;
	}
	bullet_template.origin.x.v = boss.pos.cur.x.v;
	bullet_template.origin.y.v = boss.pos.cur.y.v;
	if(boss.phase == 6) {
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.patnum = PAT_BULLET16_N_SMALL_BALL_RED;
		bullet_template.angle = randring2_next16();
		bullet_template.group = BG_RING;
		bullet_template.count = 16;
		bullet_template.speed.v = (TO_SP(1) + 14);
		bullet_template_tune();
		bullets_add_regular();
		bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.count = ((boss.phase_frame / 16) + 2);
		bullet_template.speed.v = (TO_SP(2) + 2);
	} else {
		bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
		bullet_template.patnum = PAT_BULLET16_D_BLUE;
		bullet_template.angle = 0;
		bullet_template.group = BG_STACK_AIMED;
		bullet_template.count = 7;
		bullet_template.speed.v = TO_SP(2);
		bullet_template.delta.spread_angle = 0x0A;
		bullet_template_tune();
		bullets_add_regular_fixedspeed();
		snd_se_play(15);
		bullet_template.spawn_type = BST_PELLET;
		bullet_template.group = BG_RANDOM_ANGLE_AND_SPEED;
		bullet_template.count = 4;
		bullet_template.speed.v = (TO_SP(1) + 8);
	}
	bullet_template_tune();
	bullets_add_regular();
}

/// The two thick-laser patterns
/// ----------------------------

// Four 3-bullet spreads on two fixed angles from both positions, and one
// thick laser out of each position on the frame the pattern starts.
extern "C" void near yuuka6_1AE8F(void)
{
	if(boss.phase_frame < 64) {
		if(yuuka6_sprite_flag == Y6SF_PARASOL_BACK_OPEN) {
			yuuka6_anim_parasol_back_close();
		} else if(yuuka6_sprite_flag == Y6SF_PARASOL_BACK_CLOSED) {
			yuuka6_anim_parasol_back_pull_forward();
		}
		bullet_template.speed.v = TO_SP(1);
		goto gather;
	}
	if((boss.phase_frame >= 64) && (boss.phase_frame <= 128)) {
		boss.sprite = PAT_YUUKA6_PARASOL_FORWARD_OPEN;
		if(stage_frame_mod8 != 0) {
			goto gather;
		}
		bullet_template.origin.x.v = boss.pos.cur.x.v;
		bullet_template.origin.y.v = boss.pos.cur.y.v;
		bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
		bullet_template.patnum = PAT_BULLET16_D_BLUE;
		bullet_template.group = BG_SPREAD;
		bullet_template.delta.spread_angle = 8;
		bullet_template.count = 3;
		bullet_template_tune();
		bullet_template.angle = 0x60;
		bullets_add_regular();
		bullet_template.angle = 0x20;
		bullets_add_regular();
		bullet_template.origin.x.v = yuuka6_25A0C.x.v;
		bullets_add_regular();
		bullet_template.angle = 0x60;
		bullets_add_regular();
		_AL = bullet_template.speed.v;
		_AL += 12;
		bullet_template.speed.v = _AL;
		snd_se_play(3);
		goto gather;
	}
	if(boss.phase_frame < 128) {
		goto gather;
	}
	if(yuuka6_sprite_flag != Y6SF_PARASOL_BACK_CLOSED) {
		yuuka6_anim_parasol_left_spin_back();
		goto gather;
	}
	if(yuuka6_anim_parasol_back_open()) {
		boss.phase_frame = 0;
		boss.mode = 255;
	}
gather:
	yuuka6_1AA45();
	if(boss.phase_frame == 64) {
		thicklaser_template.radius_max = yuuka6_thicklaser_radius;
		thicklaser_template.radius_speed = 4;
		thicklaser_template.line_frames = 36;
		thicklaser_template.static_frames = 40;
		thicklaser_template.col_outline = 8;
		thicklaser_template.origin.x.v = boss.pos.cur.x.v;
		thicklaser_template.origin.y.v = (boss.pos.cur.y.v + TO_SP(32));
		thicklaser_add();
		thicklaser_template.origin.x.v = yuuka6_25A0C.x.v;
		thicklaser_template.origin.y.v = (yuuka6_25A0C.y.v + TO_SP(40));
		thicklaser_add();
	}
}

// A 5-bullet spread from both positions, randomly angled within a window that
// widens by 6 with every shot.
extern "C" void near yuuka6_1AFA8(void)
{
	if(boss.phase_frame < 64) {
		if(yuuka6_sprite_flag == Y6SF_PARASOL_BACK_OPEN) {
			yuuka6_anim_parasol_back_close();
		} else if(yuuka6_sprite_flag == Y6SF_PARASOL_BACK_CLOSED) {
			yuuka6_anim_parasol_back_pull_forward();
		}
		yuuka6_spread_angle_range = 2;
		goto gather;
	}
	if((boss.phase_frame >= 64) && (boss.phase_frame <= 128)) {
		boss.sprite = PAT_YUUKA6_PARASOL_FORWARD_OPEN;
		if(stage_frame_mod4 != 0) {
			goto gather;
		}
		bullet_template.origin.y.v = (boss.pos.cur.y.v + TO_SP(32));
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.patnum = PAT_BULLET16_D_YELLOW;
		bullet_template.group = BG_SPREAD;
		bullet_template.count = 5;
		_AL = randring2_next16_and(0x1F);
		_AL += 12;
		bullet_template.speed.v = _AL;
		bullet_template.delta.spread_angle = 0x10;
		_AL = randring2_next16_mod(yuuka6_spread_angle_range * 2);
		_DL = 0x40;
		_DL -= yuuka6_spread_angle_range;
		_AL += _DL;
		bullet_template.angle = _AL;
		bullet_template.origin.x.v = boss.pos.cur.x.v;
		bullets_add_regular();
		_AL = randring2_next16_mod(yuuka6_spread_angle_range * 2);
		_DL = 0x40;
		_DL -= yuuka6_spread_angle_range;
		_AL += _DL;
		bullet_template.angle = _AL;
		bullet_template.origin.x.v = yuuka6_25A0C.x.v;
		bullets_add_regular();
		snd_se_play(9);
		_AL = yuuka6_spread_angle_range;
		_AL += 6;
		yuuka6_spread_angle_range = _AL;
		goto gather;
	}
	if(boss.phase_frame < 128) {
		goto gather;
	}
	if(yuuka6_sprite_flag != Y6SF_PARASOL_BACK_CLOSED) {
		yuuka6_anim_parasol_left_spin_back();
		goto gather;
	}
	if(yuuka6_anim_parasol_back_open()) {
		boss.phase_frame = 0;
		boss.mode = 255;
	}
gather:
	yuuka6_1AA45();
}
/// ----------------------------

/// The parasol-shield phase's three patterns
/// -----------------------------------------

// The spinning ring: one aimed 8-bullet ring set up on frame 48, then walked
// one way for as long as the pattern runs and back again on Hard and above.
extern "C" void near yuuka6_1B099(void)
{
	if(boss.phase_frame <= 48) {
		if(yuuka6_sprite_flag != Y6SF_PARASOL_SHIELD) {
			yuuka6_anim_parasol_shield();
		}
		goto gather;
	}
	if(boss.phase_frame < 136) {
		boss.sprite = (
			((stage_frame_mod4 / 2) * 2) + PAT_YUUKA6_PARASOL_SHIELD_2
		);
		if(stage_frame_mod2 != 0) {
			bullets_add_regular_fixedspeed();
		}
		if(stage_frame_mod4 == 0) {
			snd_se_play(3);
		}
		if(boss.phase_frame < 112) {
			goto gather;
		}
		bullet_template.angle += boss.angle;
		goto gather;
	}
	if((rank >= RANK_HARD) && (boss.phase_frame < 150)) {
		bullet_template.angle -= boss.angle;
		if(stage_frame_mod2 != 0) {
			bullets_add_regular_fixedspeed();
		}
		if(stage_frame_mod4 != 0) {
			goto gather;
		}
		snd_se_play(3);
		goto gather;
	}
	boss.sprite = PAT_YUUKA6_PARASOL_SHIELD_2;
gather:
	yuuka6_1AAE5();
	switch(boss.phase_frame) {
	case 48:
		_AL = iatan2(
			(boss.pos.cur.y.v - player_pos.cur.y.v),
			(boss.pos.cur.x.v - player_pos.cur.x.v)
		);
		_AL += 0x10;
		bullet_template.angle = _AL;
		bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
		bullet_template.patnum = PAT_BULLET16_N_BALL_RED;
		bullet_template.origin.x.v = boss.pos.cur.x.v;
		bullet_template.origin.y.v = boss.pos.cur.y.v;
		bullet_template.group = BG_RING;
		bullet_template.count = 8;
		bullet_template.speed.v = TO_SP(9);
		// The Easy arm is the FALL-THROUGH one and the other is the tested
		// one,
		// which is the order the original emits them in: its `jz` on
		// RANK_EASY is a forward jump to the `boss.angle = 0` store, so
		// that store is the LAST thing in the function.
		if(rank != RANK_EASY) {
			if(randring2_next16_and(1) != 0) {
				_AL = 1;
			} else {
				_AL = -1;
			}
			boss.angle = _AL;
			break;
		}
		boss.angle = 0;
		break;

	case 144:
		if(rank >= RANK_HARD) {
			break;
		}
		// fall through
	case 156:
		boss.phase_frame = 0;
		boss.mode = 255;
		break;
	}
}

// A blue ring that grows by one bullet every four frames and walks two units
// per shot.
extern "C" void near yuuka6_1B1B1(void)
{
	// The idle-sprite arm is written as the FALL-THROUGH one, not as the
	// leading test: the original's `jle` is a forward jump to it, so it is
	// emitted BELOW the pattern rather than above it. Same in the next
	// function.
	if(boss.phase_frame > 48) {
		boss.sprite = (
			((stage_frame_mod4 / 2) * 2) + PAT_YUUKA6_PARASOL_SHIELD_2
		);
		if(stage_frame_mod8 != 0) {
			goto gather;
		}
		snd_se_play(3);
		_AL = bullet_template.angle;
		_AL += 2;
		bullet_template.angle = _AL;
		bullet_template.group = BG_RING;
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.patnum = PAT_BULLET16_D_BLUE;
		bullet_template.speed.v = TO_SP(3);
		bullet_template.count = ((boss.phase_frame / 4) + 4);
		bullet_template_tune();
		bullets_add_regular();
		goto gather;
	}
	boss.sprite = PAT_YUUKA6_PARASOL_SHIELD_2;
gather:
	yuuka6_1AAE5();
	if(boss.phase_frame == 144) {
		boss.phase_frame = 0;
		boss.mode = 255;
	}
}

// Two randomly angled chasing crosses every eight frames. The only pattern of
// the seventeen that runs no gather animation of its own.
extern "C" void near yuuka6_1B22B(void)
{
	if(boss.phase_frame > 48) {
		boss.sprite = (
			((stage_frame_mod4 / 2) * 2) + PAT_YUUKA6_PARASOL_SHIELD_2
		);
		if(stage_frame_mod8 != 0) {
			goto ending;
		}
		chasecrosses_add(randring2_next16(), TO_SP(2));
		chasecrosses_add(randring2_next16(), TO_SP(2));
		snd_se_play(3);
		goto ending;
	}
	boss.sprite = PAT_YUUKA6_PARASOL_SHIELD_2;
ending:
	if(boss.phase_frame == 144) {
		boss.phase_frame = 0;
		boss.mode = 255;
	}
}
/// -----------------------------------------

// The defeat pattern: a mirrored pair of 8-bullet rings every four frames,
// alternating between pellets and gather pellets, with one gather circle on
// every 32nd frame.
extern "C" void near yuuka6_1B282(void)
{
	unsigned char subframe;

	_AL = boss.phase_frame;
	_AL &= 0x1F;
	subframe = _AL;
	boss.sprite = (((stage_frame_mod4 / 2) * 2) + PAT_YUUKA6_PARASOL_SHIELD_2);
	// `& 3`, not `% 4`: [subframe] promotes to a SIGNED int, so the modulo
	// cannot become a mask and Turbo C++ emits a full `idiv` -- nine bytes
	// where the original has one `test byte ptr [bp-1], 3`.
	if((subframe & 3) == 0) {
		bullet_template.origin.x.v = boss.pos.cur.x.v;
		bullet_template.origin.y.v = boss.pos.cur.y.v;
		_AL = BST_GATHER_PELLET;
		_AL -= bullet_template.spawn_type;
		bullet_template.spawn_type = _AL;
		bullet_template.patnum = PAT_BULLET16_N_SMALL_BALL_RED;
		bullet_template.group = BG_RING;
		bullet_template.count = 8;
		_AL = subframe;
		_AL += TO_SP(2);
		bullet_template.speed.v = _AL;
		bullet_template_tune();
		_AL = -0x7E;
		_AL -= bullet_template.angle;
		bullet_template.angle = _AL;
		bullets_add_regular_fixedspeed();
		_AL = 0x80;
		_AL -= bullet_template.angle;
		bullet_template.angle = _AL;
		bullets_add_regular_fixedspeed();
	}
	if(subframe != 0) {
		return;
	}
	_AL = bullet_template.angle;
	_AL += 8;
	bullet_template.angle = _AL;
	gather_template.ring_points = 8;
	gather_template.col = 9;
	_AL = gather_template.angle_delta;
	_AL = -_AL;
	gather_template.angle_delta = _AL;
	gather_add_only();
}

// The last of the seventeen: two spreads of blue bullets from both positions
// every 16 frames, unaimed on every 32nd frame and aimed otherwise.
extern "C" void near yuuka6_1B313(void)
{
	if(boss.phase_frame < 64) {
		if(yuuka6_sprite_flag == Y6SF_PARASOL_BACK_OPEN) {
			yuuka6_anim_parasol_back_close();
			goto gather;
		}
		if(yuuka6_sprite_flag != Y6SF_PARASOL_BACK_CLOSED) {
			goto gather;
		}
		yuuka6_anim_parasol_back_pull_forward();
		goto gather;
	}
	if((boss.phase_frame >= 64) && (boss.phase_frame <= 192)) {
		boss.sprite = PAT_YUUKA6_PARASOL_FORWARD_OPEN;
		if(stage_frame_mod16 != 0) {
			goto gather;
		}
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.patnum = PAT_BULLET16_D_BLUE;
		bullet_template.speed.v = (TO_SP(2) + 8);
		bullet_template.delta.spread_angle = 0x0E;
		if((stage_frame % 32) == 0) {
			bullet_template.count = 10;
			bullet_template.group = BG_SPREAD;
			bullet_template.angle = 0x40;
		} else {
			bullet_template.count = 7;
			bullet_template.group = BG_SPREAD_AIMED;
			bullet_template.angle = 0;
		}
		bullet_template.origin.y.v = (boss.pos.cur.y.v + TO_SP(32));
		bullet_template.origin.x.v = boss.pos.cur.x.v;
		bullets_add_regular();
		bullet_template.origin.x.v = yuuka6_25A0C.x.v;
		bullets_add_regular();
		snd_se_play(3);
		goto gather;
	}
	if(boss.phase_frame < 192) {
		goto gather;
	}
	if(yuuka6_sprite_flag != Y6SF_PARASOL_BACK_CLOSED) {
		yuuka6_anim_parasol_left_spin_back();
		goto gather;
	}
	if(yuuka6_anim_parasol_back_open()) {
		boss.phase_frame = 0;
		boss.mode = 255;
	}
gather:
	yuuka6_1AA45();
}

// Runs one frame of shot collision against the parasol shield, at the
// shield's own position rather than the boss sprite's, and subtracts whatever
// damage it took from Yuuka's HP.
//
// Returns whether that brought her HP below zero -- which is ZUN bloat here,
// because the single caller in th04/main/boss/b6_upd.cpp discards it.
extern "C" bool near yuuka6_1B3E2(void)
{
	if(yuuka6_25A1B != 2) {
		return false;
	}

	// The hitbox is set field by field rather than through
	// th04/main/player/shot.hpp's 3-argument shots_hittest() overload, and
	// that is `[measured]`: that overload takes its radii by `const&`, so
	// naming the two constants -- or even passing them as literals -- forces
	// Turbo C++ to MATERIALISE them, and this object then contributes 4 bytes
	// to _DATA that ZUN's never did. The symptom is not local: every DS
	// offset after 0x248E shifts by 4, so every memory operand in this
	// function reads as if it resolved to the wrong symbol. The original
	// stores both radii as immediates, which is what this does.
	shot_hitbox_radius.x.v = TO_SP(24);
	shot_hitbox_radius.y.v = TO_SP(48);
	shot_hitbox_center.x.v = yuuka6_25A0C.x.v;
	shot_hitbox_center.y.v = yuuka6_25A0C.y.v;

	// One expression, deliberately: written as a statement followed by a
	// separate test, Turbo C++ reloads the byte it just stored, where the
	// original tests the value still in AL.
	if(yuuka6_25A1E = shots_hittest()) {
		snd_se_play(4);
	}
	boss.hp -= yuuka6_25A1E;
	if(boss.hp < 0) {
		return true;
	}
	return false;
}

// Ends the current phase: starts a bullet clear if one is not already running,
// plays the given explosion, and resets every piece of per-phase state. The
// phase this moves on to will itself end at [next_end_hp].
//
// `extern "C"`, because that is the only spelling that reproduces the plain,
// undecorated public the dump used to export for this function -- which is
// what th04/main/boss/b6_upd.cpp declares and links against. `pascal` alone
// does NOT suffice: measured from this object's own PUBDEF, C++ linkage
// upper-cases the name by the calling convention and then STILL appends the
// mangled argument suffix. TLINK reports the demangled form, so the failure
// reads as if the body never compiled. kb/codegen/0027 carries this as a
// counter-shape; state/notes/yuuka6_phase_next.md has the exact two
// spellings. (th04/main/boss/b6.cpp declares this function without
// `extern "C"`, so that declaration has never resolved against anything. It
// is harmless only because th04/main/boss/bg.cpp, which expands that file,
// does not call it.)
extern "C" void pascal near yuuka6_phase_next(
	explosion_type_t explosion_type, int next_end_hp
)
{
	if(bullet_clear_time < 20) {
		bullet_clear_time = 20;
	}
	boss_explode_small(explosion_type);
	boss.phase++;
	boss.phase_frame = 0;
	boss.phase_state.patterns_seen = 0;
	boss.mode = 0;
	boss.hp = boss.phase_end_hp;
	boss.phase_end_hp = next_end_hp;
	yuuka6_anim_frame = 0;
	boss.sprite = PAT_YUUKA6_PARASOL_BACK_OPEN;
	yuuka6_anim_frame = 0; // ZUN bloat
	yuuka6_sprite_flag = Y6SF_PARASOL_BACK_OPEN;
}


/// The head of B6_SPAWN_TEXT's C++ half
/// -----------------------------------
/// Everything below is in a DIFFERENT code segment from everything above:
/// B6_SPAWN_TEXT, the kb/codegen/0080 anchor MATCH-TH04-MAIN-034-HEAD carved
/// off main_034_TEXT's head. One object contributing to two code segments is
/// kb/codegen/0155, and it is what lets this whole chain land without a single
/// new translation unit or Tupfile.lua line.
///
/// This object rather than th04/boss_bg.cpp's, which hosts the first two procs
/// of that segment: yuuka6_customs_update() makes near calls into main_03 --
/// items_add(), vector2_near(), vector2_at(), shots_hittest() -- and an object
/// whose `-zP` names main_01 frames every one of them on the wrong group.
/// `[measured]`: the items_add() fixup OVERFLOWED and the rest were silently
/// within 16 bits, i.e. would have linked to garbage (kb/codegen/0104). This
/// object is `-zPmain_03`, and TLINK already places its B6_SPAWN_TEXT block
/// immediately after th04/boss_bg.cpp's -- which is also these five bodies'
/// original address order.
#pragma codeseg B6_SPAWN_TEXT main_03

// Defined below, and called by yuuka6_phase2_fly() above it.
extern "C" bool pascal near yuuka6_teleport_to(subpixel_t x, subpixel_t y);

// One frame of everything Yuuka's Extra fight overlays on [custom_entities]:
// every chasing cross, and then the safety circle. Called unconditionally out
// of yuuka6_update()'s tail, after the phase dispatch.
//
// The two halves share ONE pointer. ZUN walks [si] across the chasing crosses
// and then keeps using it for the safety circle, which works because the circle
// occupies the slot immediately past the last cross -- so the loop leaves the
// pointer aimed at it. [circle] is a cast of that same pointer rather than a
// second variable, because a second one would not fit: SI and DI are already
// the pointer and the counter, and the frame this function declares has no room
// for a third.
extern "C" void near yuuka6_customs_update(void)
{
	chasecross_t near *p;
	int i;
	unsigned int left;
	unsigned int top;
	unsigned char angle;
	unsigned char angle_offset;

	#define circle (reinterpret_cast<safetycircle_t near *>(p))

	shot_hitbox_radius.x.v = TO_SP(12);
	shot_hitbox_radius.y.v = TO_SP(12);
	p = chasecrosses;
	for(i = 0; i < CHASECROSS_COUNT; i++, p++) {
		if(p->flag != CCF_ALIVE) {
			continue;
		}
		vector2_near(p->velocity, p->angle, p->speed);
		p->center.x.v += p->velocity.x.v;
		p->center.y.v += p->velocity.y.v;

		// ZUN bug, and the mirror of the one th04/main/boss/b4m_upd.cpp
		// records for Marisa's bits: clipped 16 pixels too early at the right
		// and bottom edges, because the box is biased into its top-left corner
		// on one axis and not the other.
		if(
			(p->center.x.v <= TO_SP(-(CHASECROSS_W / 2))) ||
			(p->center.x.v >= TO_SP(PLAYFIELD_W + (CHASECROSS_W / 2))) ||
			(p->center.y.v >= TO_SP(PLAYFIELD_H + (CHASECROSS_H / 2))) ||
			(p->center.y.v <= TO_SP(-(CHASECROSS_H / 2)))
		) {
			p->flag = CCF_FREE;
		}

		// The player hittest, biased into the top-left corner of the cross's
		// box so that one UNSIGNED comparison per axis covers both directions.
		// `jnb`, not `jge`, is what says these are unsigned.
		left = (p->center.x.v + TO_SP(-12));
		top = (p->center.y.v + TO_SP(-12));
		if(
			((player_pos.cur.x.v - left) < TO_SP(24)) &&
			((player_pos.cur.y.v - top) < TO_SP(24))
		) {
			player_is_hit = true;
		}

		// Turns one angle unit per frame towards the player, for the first 56
		// frames of its life only.
		if(p->age < 56) {
			angle = iatan2(
				(player_pos.cur.y.v - p->center.y.v),
				(player_pos.cur.x.v - p->center.x.v)
			);
			angle -= p->angle;
			// ZUN bloat: the second test is the negation of the first.
			if(angle < 0x80) {
				p->angle++;
			} else if(angle >= 0x80) {
				p->angle--;
			}
		}

		shot_hitbox_center.x.v = p->center.x.v;
		shot_hitbox_center.y.v = p->center.y.v;
		if(p->damage_this_frame = shots_hittest()) {
			snd_se_play(4);
		}
		p->hp -= p->damage_this_frame;
		if(p->hp <= 0) {
			snd_se_play(3);
			score_delta += 3000;
			p->flag = CCF_KILL_ANIM;
			p->age = 0;

			// kb/codegen 0083 + 0122: the original reaches sparks_add_random()
			// through a nopcall, which a C++ far call never becomes, and the
			// two SI-relative pushes have to be `db`s as well -- naming SI to
			// the inline assembler would move [p] out of it. Byte-for-byte the
			// same island, and the same callee, as
			// th04/main/boss/b4m_upd.cpp's; the two `db`ed pushes are even the
			// same encodings, because [center] sits at offset 2 in both
			// structures.
			/* TODO: Replace with the decompiled call
			 * 	sparks_add_random(p->center.x, p->center.y, TO_SP(4), 8);
			 * once that function is part of the same segment */
			_asm {
				db  	0xFF, 0x74, 0x02;
				db  	0xFF, 0x74, 0x04;
				db  	0x66, 0x68, 8, 0x00, (4 * 16), 0x00;
				nop;
				push	cs;
				call	near ptr sparks_add_random;
			}
			items_add(p->center.x.v, p->center.y.v, IT_BIGPOWER);
		}
		p->age++;
	}

	switch(circle->flag) {
	case SCF_GROW:
		if(circle->radius_filled <= 128) {
			circle->radius_filled += 8;
		} else {
			circle->flag = SCF_SHRINK;
		}
		break;

	case SCF_SHRINK:
		if(circle->shrink_frame < 8) {
			circle->radius_ring_distance -= 8;
		} else if(circle->shrink_frame == 8) {
			circle->col_ring = 9;
		} else if(circle->shrink_frame < 16) {
			circle->radius_ring_distance -= 2;
		} else if(circle->shrink_frame < 160) {
			if((circle->shrink_frame & 0x1F) < 16) {
				circle->radius_ring_distance++;
			} else {
				circle->radius_ring_distance--;
			}
			if(stage_frame_mod2 != 0) {
				circle->col_ring = 15;
			} else {
				circle->col_ring = 9;
			}
			if(circle->shrink_frame <= 104) {
				circle->radius_filled--;
			}
			if((circle->shrink_frame & 0x0F) == 0) {
				angle_offset = -0x40;
				if((circle->shrink_frame & 0x1F) == 0) {
					bullet_template.origin.x.v = boss.pos.cur.x.v;
					bullet_template.origin.y.v = (
						boss.pos.cur.y.v + TO_SP(32)
					);
					bullet_template.spawn_type = BST_BULLET16;
					bullet_template.patnum = PAT_BULLET16_N_SMALL_BALL_RED;
					bullet_template.group = BG_RING_AIMED;
					bullet_template.count = 32;
					bullet_template.speed.v = TO_SP(1);
					bullet_template.angle = 0;
					bullet_template_tune();
					bullets_add_regular();
					angle_offset = 0x40;
				}
				bullet_template.spawn_type = BST_PELLET;
				bullet_template.delta.spread_angle = 0x0C;
				angle = (stage_frame / 5);
				left = (circle->radius_filled + 4);
				for(i = 0; i < 8; i++, angle += 0x20) {
					bullet_template.angle = (angle + angle_offset);
					vector2_at(
						bullet_template.origin,
						circle->center.x,
						circle->center.y,
						left,
						angle
					);
					bullet_template.origin.x.v -= TO_SP(2);
					bullet_template.origin.y.v -= TO_SP(1);
					bullet_template.origin.x.v *= TO_SP(1);
					bullet_template.origin.y.v *= TO_SP(1);
					bullet_template.group = BG_STACK;
					bullet_template.count = 8;
					bullet_template.speed.v = TO_SP(1);
					bullets_add_regular();
					bullet_template.group = BG_SPREAD;
					bullet_template.count = 4;
					bullet_template.speed.v = TO_SP(3);
					bullets_add_regular();
				}
				snd_se_play(3);
			}
		} else if(circle->shrink_frame < 176) {
			circle->radius_ring_distance += 16;
			circle->radius_filled -= 2;
		} else {
			circle->flag = SCF_FREE;
		}
		circle->shrink_frame++;
		break;
	}

	#undef circle
}

// The one that is not `extern "C"`: the dump publishes it under the lower-case
// C++ mangled name a non-`pascal` C++ function gets, which is the evidence for
// its linkage. Defined ahead of yuuka6_teleport_to() because that is the
// original address order, so it needs the forward declaration
// th04/main/boss/b6.cpp already carries.
bool near yuuka6_phase2_fly(void)
{
	if(
		(boss.phase_state.patterns_seen % (PHASE2_FLY_NODES + 1)) <
		PHASE2_FLY_NODES
	) {
		switch(boss.phase_frame) {
		case 1:
			vector2_near(
				boss.pos.velocity,
				YUUKA6_PHASE2_FLY_ANGLES[yuuka6_phase2_fly_path][
					boss.phase_state.patterns_seen % (PHASE2_FLY_NODES + 1)
				],
				8
			);
			break;
		case 112:
			boss.phase_frame = 0;
			boss.phase_state.patterns_seen++;
			return true;
		}
		boss.pos.cur.x.v += boss.pos.velocity.x.v;
		boss.pos.cur.y.v += boss.pos.velocity.y.v;
		return false;
	}
	// The leg after the last one: back to the top centre, and on to the next
	// phase from there.
	return yuuka6_teleport_to(TO_SP(PLAYFIELD_W / 2), TO_SP(80));
}

// `pascal`, so the dump's `public` is the bare UPPERCASE spelling and this one
// needed no zero-byte alias to become linkable -- the same device
// yuuka6_phase_next() uses.
//
// [yuuka6_25A1B] gates the mirror position [yuuka6_25A0C], which the patterns
// in th04/main/boss/b6_next.cpp spawn their second half from. Setting it to 2
// here is what tells them the mirror exists.
bool pascal near yuuka6_teleport_to(subpixel_t x, subpixel_t y)
{
	if(boss.phase_frame < 64) {
		if(yuuka6_sprite_flag != Y6SF_VANISHED) {
			yuuka6_anim_vanish();
		}
	} else {
		if(yuuka6_sprite_flag == Y6SF_VANISHED) {
			yuuka6_anim_appear();
		}
	}
	switch(boss.phase_frame) {
	case 64:
		boss.pos.cur.x.v = x;
		boss.pos.cur.y.v = y;
		if(yuuka6_25A1B != 0) {
			yuuka6_25A1B = 2;
			yuuka6_25A0C.x.v = (TO_SP(PLAYFIELD_W) - x);
			yuuka6_25A0C.y.v = y;
		}
		break;
	case 128:
		boss.phase_frame = 0;
		boss.phase_state.patterns_seen++;
		return true;
	}
	return false;
}

extern "C" void near yuuka6_fly_sine(void)
{
	if(boss.phase_frame == 1) {
		boss.pos.velocity.x.v = TO_SP(2);
		boss.angle = 0;
	}
	boss.pos.cur.x.v += boss.pos.velocity.x.v;
	if((boss.pos.cur.x.v <= TO_SP(48)) || (boss.pos.cur.x.v >= TO_SP(336))) {
		// ZUN bloat: a negation spelled as a multiplication, which is why it
		// comes out as an `imul` rather than a `neg`.
		boss.pos.velocity.x.v *= -1;
	}
	boss.pos.cur.y.v = polar_y(TO_SP(80), TO_SP(48), boss.angle);
	boss.angle += 2;
}

extern "C" bool near yuuka6_return_to_center(void)
{
	if(yuuka6_sprite_flag == Y6SF_VANISHED) {
		yuuka6_anim_appear();
	} else {
		yuuka6_25A08 = 0;
		if(boss.pos.cur.x.v < TO_SP(PLAYFIELD_W / 2)) {
			boss.pos.cur.x.v += TO_SP(1);
		} else if(boss.pos.cur.x.v >= TO_SP((PLAYFIELD_W / 2) + 1)) {
			boss.pos.cur.x.v -= TO_SP(1);
		} else {
			return true;
		}
	}
	return false;
}


#pragma codeseg
