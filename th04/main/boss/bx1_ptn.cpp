/// Stage 5 Boss - Mugetsu: the four patterns, and the fight's helpers
/// -------------------------------------------------------
/// (#included from th04/bx1_ptn.cpp, which names MUGETSU_TEXT --
/// th04_main.asm's kb/codegen/0080 carve off main_033_TEXT's head. That
/// segment's root contribution is enemies_update() and nothing else now, and
/// this fight's four objects append behind it in address order, which is the
/// order these functions already had. See th04/bx1_gath.cpp.)
///
/// mugetsu_fg_render() is elsewhere, in th04/main/boss/bx1_fg.cpp, and so is
/// mugetsu_gengetsu_bg_render(), which phase 0 installs.
///
/// **A naming round is owed** for every address-suffixed function name below.
/// Every one of them is reached from mugetsu_update()'s two `switch`es, or
/// from a pose driver, and from nowhere else, which is why the ones that stay
/// inside one object are `static` -- exactly like Gengetsu's half of the Extra
/// Stage in th04/main/boss/bx2_upd.cpp -- and why the zero-byte `label`
/// aliases th04_main.asm would otherwise have needed (kb/codegen/0123) do not
/// exist.
///
/// THE SPLIT, and why there are four objects rather than one
/// ----------------------------------------------------------
/// `[measured 2026-08-24]` A single object for all fourteen procs is RED at
/// 400 bytes: in any translation unit that reaches th04/main/bullet/bullet.hpp
/// -- which th04/main/gather.hpp pulls in, and which pulls in
/// th04/main/playfld.hpp TOGETHER WITH th04/main/rank.hpp, either alone being
/// harmless -- Turbo C++'s OBJ writer stages the two pose drivers' dense
/// `switch` selector through AX (`mov ax,mem` / `sub ax,10h` / `mov bx,ax`)
/// instead of loading BX directly, one byte longer. That extra byte flips the
/// function's `-a2` table parity and the pad in front of the table disappears
/// with it, so the change is LENGTH-NEUTRAL and neither an object-length probe
/// nor a SEGDEF or PUBDEF check can see it -- and `tcc -S` prints the BX form
/// for the very
/// same source, so kb/codegen/0152's listing screen reports it clean. Six
/// `#pragma option` settings and seven source spellings do not move it; only
/// the header set does.
///
/// So the pose pair gets an object with no bullet.hpp in its closure. The
/// other three boundaries are then forced by the `-a2` parity arithmetic of
/// kb/codegen/0096 (as corrected by 0154: the pad appears when the natural
/// table offset is EVEN *in the compiling object*), measured lengths:
///
///   bx1_gath  mugetsu_1802F 0x15, mugetsu_18044 0x67 + 0x10 sparse pair
///                                                              = 0x8C
///   bx1_pose  mugetsu_180BB 0x6F, mugetsu_1812A 0xB1 + pad + 0x42,
///             mugetsu_1821E 0xB3 + pad + 0x42                   = 0x259
///   bx1_ptn   the four patterns and the fight's helpers          = 0x3D7
///   bx1_upd   mugetsu_update 0x2CE + pad + 0x24 + 0x10           = 0x303
///
/// mugetsu_180BB() is in the POSE object and not the gather one because
/// mugetsu_1812A()'s table needs an ODD prefix to land on an even offset and
/// take its pad; 0x6F supplies it, and 1821E's follows. That leaves 0x3D7 --
/// odd -- of helpers ahead of mugetsu_update(), which would put ITS table at
/// an odd offset and lose its pad, so mugetsu_update() takes the fourth
/// object and its table sits at 0x2CE from a zero prefix. Sum 0x9BF, the same
/// 0x9BF the one-object version had: only the boundaries moved.
///
/// The eleven functions called across an object boundary therefore lost their
/// `static`; they are declared in th04/main/boss/bx1.cpp. Every call is still
/// near, within MUGETSU_TEXT and within `main_03`, so no call site changed
/// length.

#include "platform.h"
#include "pc98.h"
// iatan2(), which the cross-ring pattern aims with.
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th03/hardware/palette.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/bomb.hpp"
#include "th02/v_colors.hpp"
#include "th04/snd/snd.h"
#include "th04/formats/std.hpp"
#include "th04/math/randring.hpp"
#include "th04/sprites/main_pat.h"
#include "th04/main/frames.h"
#include "th04/main/bg.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/null.hpp"
#include "th04/main/gather.hpp"
#include "th04/main/circle.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/player/player.hpp"
#include "th04/main/tile/bb.hpp"
#include "th04/main/midboss/midboss.hpp"
#include "th04/main/boss/boss.hpp"

// Declared FAR here, and only here, which is why
// th04/main/boss/bosses.hpp -- the header that declares it `near`, which
// is what it is -- is deliberately not included. A near reference under
// this object's `-zPmain_03` frames its offset on main_03, and
// mugetsu_gengetsu_bg_render() lives in main_01, which is a
// `Fixup overflow at MUGETSU_TEXT` at link time. kb/codegen/0162, and
// th04/main/boss/bx2_upd.cpp does the same thing for the same store.
// Nothing else in that header is reached from this object.
void pascal far mugetsu_gengetsu_bg_render(void);
// [mugetsu_phase2_mode] and this file's own mugetsu_phase2_next()
// declaration, which predates the lift.
#include "th04/main/boss/bx1.cpp"

/// The fight's own state
/// ---------------------
/// All three are th04_main.asm slots with no `public` of ZUN's, and this
/// object's functions are their only readers or writers in any of the five
/// binaries, so this parcel coined all three names. `[inferred]`.
extern "C" {
	// Added to [boss.phase_frame] before mugetsu_18044()'s gather `switch`,
	// which is how the three pose drivers put the same animation on three
	// different timelines. `_DATA` rather than `.data?`: it is initialised to
	// 0x10, which is mugetsu_180BB's value.
	extern int mugetsu_gather_frame_offset;

	// The current pose driver: mugetsu_180BB(), mugetsu_1812A() or
	// mugetsu_1821E().
	extern unsigned char (near *mugetsu_pose_func)(void);

	// Where the next gather animation is centred, and where the teleport in
	// the middle of a pose sequence puts the boss.
	extern SPPoint mugetsu_gather_center;
}

// The byte th04_main.asm already aliases as `_extra_boss_bomb_immunity`, with
// the same meaning and the same 32 frames Gengetsu's half of the Extra Stage
// gives it: while it is nonzero the fight takes damage through a wider fixed
// box and throws the result away. gengetsu_update() and gengetsu_20202() in
// th04/main/boss/bx2_upd.cpp are this file's twins on both counts, so the name
// and the constant are theirs rather than newly coined -- **the naming round
// that file's own comment says is owed covers this reader too.**
extern "C" unsigned char extra_boss_bomb_immunity;
static const int BOMB_IMMUNITY_FRAMES = 32;
/// ---------------------

/// The four patterns
/// -----------------
/// Every one of them is the pose driver's return value dispatched three ways:
/// 1 sets the volley up, 2 fires it on the driver's own interval, 3 hands
/// control back to mugetsu_update() by setting [boss.mode] to 255.

// The one [boss_statebyte] slot this fight uses, spelled the way
// th04_main.asm's own `boss_statebyte_t` overlay already names it.
#define direction boss_statebyte[15]

// An 8-way ring that speeds up and sweeps one way or the other, decided by a
// coin flip when it arms.
void near mugetsu_18314(void)
{
	switch(mugetsu_pose_func()) {
	case 1:
		bullet_template.group = BG_RING;
		bullet_template.count = 8;
		bullet_template.speed.v = TO_SP(2);
		bullet_template.angle = randring2_next16();
		direction = randring2_next16_and(1);
		break;

	case 2:
		if(stage_frame_mod4 == 0) {
			bullets_add_regular();

			// kb/codegen/0032 on both: each keeps its value live in AL across
			// the store rather than reading the template back.
			_AL = bullet_template.speed.v;
			_AL += 3;
			bullet_template.speed.v = _AL;
			if(direction != 0) {
				_AL = 4;
			} else {
				_AL = 0xFC;
			}
			_AL += bullet_template.angle;
			bullet_template.angle = _AL;
			snd_se_play(3);
		}
		break;

	case 3:
		boss.phase_frame = 0;
		boss.mode = -1;
		break;
	}
}

// Two decelerating cross rings that turn opposite ways, and then an aimed
// spread that widens as it fires.
void near mugetsu_1838A(void)
{
	switch(mugetsu_pose_func()) {
	case 1:
		snd_se_play(15);
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.patnum = PAT_BULLET16_N_CROSS_YELLOW;
		bullet_template.angle = 0;
		bullet_template.group = BG_RING;
		bullet_template.count = 32;
		bullet_template.special_motion = BSM_DECELERATE_THEN_TURN;
		bullet_template.speed.v = (TO_SP(2) + 8);
		bullet_special.turns_max = 2;
		bullet_template_special_angle.turn_by = -0x20;
		bullet_template_tune();
		bullets_add_special_fixedspeed();
		bullet_template_special_angle.turn_by = 0x20;
		bullets_add_special_fixedspeed();
		bullet_template.angle = iatan2(
			(player_pos.cur.y.v - boss.pos.cur.y.v),
			(player_pos.cur.x.v - boss.pos.cur.x.v)
		);
		bullet_template.delta.spread_angle = 0x42;
		break;

	case 2: {
		int frame = (boss.phase_frame & 7);
		if(_AX == 0) {
			_AL = bullet_template.delta.spread_angle;
			_AL += 0xF8;
			bullet_template.delta.spread_angle = _AL;
		}
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.patnum = PAT_BULLET16_D_BLUE;
		_AX = (frame * 12);
		_AL += TO_SP(2);
		bullet_template.speed.v = _AL;
		bullet_template.group = BG_SPREAD;
		bullet_template.count = 2;
		bullets_add_regular();
		if(stage_frame_mod4 == 0) {
			snd_se_play(3);
		}
		break;
	}

	case 3:
		boss.phase_frame = 0;
		boss.mode = -1;
		break;
	}
}

// A single backwards 40-way cloud ring, fired on the arming frame itself.
void near mugetsu_1845E(void)
{
	switch(mugetsu_pose_func()) {
	case 1:
		bullet_template.spawn_type = BST_BULLET16_CLOUD_BACKWARDS;
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.group = BG_RING;
		bullet_template.count = 40;
		bullet_template.speed.v = TO_SP(1);
		bullet_template.angle = randring2_next16();
		bullets_add_regular();
		snd_se_play(15);
		break;

	case 2:
		boss.phase_frame = 0;
		boss.mode = -1;
		break;
	}
}

// Pairs of turning bullets out of random points of a box around the boss, one
// turning each way.
void near mugetsu_184AC(void)
{
	switch(mugetsu_pose_func()) {
	case 1:
		bullet_template.special_motion = BSM_DECELERATE_THEN_TURN;
		bullet_template.group = BG_SINGLE;
		bullet_special.turns_max = 1;
		break;

	case 2:
		if(stage_frame_mod2 != 0) {
			bullet_template.spawn_type = BST_BULLET16;
			bullet_template.patnum = PAT_BULLET16_N_SMALL_BALL_YELLOW;
			_AL = randring2_next16_and(0x3F);
			_AL += TO_SP(1);
			bullet_template.speed.v = _AL;
			bullet_template.origin.x.v = (
				randring2_next16_mod(TO_SP(64)) +
				(boss.pos.cur.x.v - TO_SP(32))
			);
			bullet_template.origin.y.v = (
				randring2_next16_mod(TO_SP(32)) +
				(boss.pos.cur.y.v - TO_SP(26))
			);
			bullet_template.angle = 0;
			bullet_template_special_angle.turn_by = 0x40;
			bullets_add_special();
			_AL = randring2_next16_and(0x3F);
			_AL += TO_SP(1);
			bullet_template.speed.v = _AL;
			bullet_template.angle = 0x80;
			bullet_template_special_angle.turn_by = -0x40;
			bullets_add_special();
			snd_se_play(9);
		}
		break;

	case 3:
		boss.phase_frame = 0;
		boss.mode = -1;
		break;
	}
}

// The same box, but a 32-way ring per point and a random spawn type.
void near mugetsu_18556(void)
{
	switch(mugetsu_pose_func()) {
	case 1:
		bullet_template.group = BG_RING;
		bullet_template.count = 32;
		bullet_template_tune();
		break;

	case 2:
		if(stage_frame_mod8 == 0) {
			bullet_template.spawn_type = randring2_next16_and(BST_PELLET);
			bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
			_AL = randring2_next16_and(0x3F);
			_AL += TO_SP(1);
			bullet_template.speed.v = _AL;
			bullet_template.origin.x.v = (
				randring2_next16_mod(TO_SP(64)) +
				(boss.pos.cur.x.v - TO_SP(32))
			);
			bullet_template.origin.y.v = (
				randring2_next16_mod(TO_SP(32)) +
				(boss.pos.cur.y.v - TO_SP(26))
			);
			bullet_template.angle = randring2_next16();
			bullets_add_regular();
			snd_se_play(3);
		}
		break;

	case 3:
		boss.phase_frame = 0;
		boss.mode = -1;
		break;
	}
}

/// The final phase's two volleys
/// -----------------------------
/// Neither goes through a pose driver: phase 6 fires both on its own clock.

// A 16-way ring that accelerates with the phase, plus a 4-way one at twice the
// mirrored angle, with the ring's own angle sweeping back and forth.
void near mugetsu_185E4(void)
{
	if(stage_frame_mod8 == 0) {
		bullet_template.angle = boss.angle;
		bullet_template.group = BG_RING;
		bullet_template.count = 16;
		_AX = (boss.phase_frame / 256);
		_AL += TO_SP(2);
		bullet_template.speed.v = _AL;
		bullets_add_regular();
		bullet_template.patnum = PAT_BULLET16_D_BLUE;
		bullet_template.spawn_type = BST_BULLET16;
		_AL = bullet_template.angle;
		_AL = -_AL;
		_AL += _AL;
		bullet_template.angle = _AL;
		bullet_template.count = 4;
		_AL = bullet_template.speed.v;
		_AL += TO_SP(1);
		bullet_template.speed.v = _AL;
		bullets_add_regular();
		if((boss.phase_frame % 1024) < 512) {
			_AL = boss.angle;
			_AL += 3;
		} else {
			_AL = boss.angle;
			_AL += 0xFD;
		}
		boss.angle = _AL;
	}
}

// The fast 32-way ring the fight throws in on top of everything else once it
// has run long enough.
void near mugetsu_18655(void)
{
	if(stage_frame_mod8 == 0) {
		bullet_template.group = BG_RING;
		bullet_template.count = 32;
		bullet_template.patnum = PAT_BULLET16_D_BLUE;
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.angle = randring2_next16();
		bullet_template.speed.v = TO_SP(7);
		bullets_add_regular();
	}
}

// Ends one of the fight's phases: drop its items, blow it up, and reset every
// per-phase counter for the next one. It is the one proc of this fight that
// already had an ASM module of its own, which this lift deleted, and
// th04/main/boss/bx1.cpp already carried this signature.
void pascal near mugetsu_phase2_next(
	explosion_type_t explosion_type, int next_end_hp
)
{
	boss_items_drop();
	boss_explode_small(explosion_type);
	boss.phase++;
	boss.phase_frame = 0;
	boss.mode = 0;
	boss.phase_state.patterns_seen = 0;
	boss.hp = boss.phase_end_hp;
	boss.phase_end_hp = next_end_hp;
	mugetsu_phase2_mode = 0;
}

// The fight's hittest, and the frame tick that goes with it. Returns `true`
// only for the ordinary box, which is what phase 2 reads to award its bonus.
bool near mugetsu_186B9(void)
{
	if(extra_boss_bomb_immunity != 0) {
		boss_hittest_shots_damage(TO_SP(48), TO_SP(48), 10);
	} else if((boss.sprite <= 130) && (boss.sprite != 0)) {
		return boss_hittest_shots();
	}
	boss.phase_frame++;
	return false;
}
