/// Stage 3 Boss - Elly: the fight's own update function
/// ----------------------------------------------------
/// (#included from th04/main_034.cpp. main_034_TEXT has no other C++
/// contribution, and TLINK lays a segment's contributions out in link order
/// with the root dump first, so this object lands at the segment's tail by
/// construction — which is where this function already was.
/// kb/codegen/0112 + 0114.)
///
/// elly_fg_render() is th04/main/boss/b3_fg.cpp and elly_bg_render() is
/// th04/main/boss/bg.cpp's neighbour in main_01; both are other objects.

#include "platform.h"
#include "pc98.h"
// iatan2(), which four of Elly's patterns aim with.
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th02/main/player/player.hpp"
#include "th03/hardware/palette.hpp"
#include "th03/math/polar.hpp"
#include "th04/snd/snd.h"
#include "th04/sprites/main_pat.h"
#include "th04/math/randring.hpp"
#include "th04/main/circle.hpp"
#include "th04/main/gather.hpp"
#include "th04/main/player/player.hpp"
#include "th04/main/bg.hpp"
#include "th04/main/frames.h"
#include "th04/main/homing.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/spark.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/tile/bb.hpp"
#include "th04/main/boss/boss.hpp"
// vector2() and shots_hittest(), which only the boomerang driver needs. Both
// appended at the END of this list rather than sorted into it: every function
// below was matched against the declarations visible in the original order.
#include "th04/math/vector.hpp"
#include "th04/main/player/shot.hpp"

/// Still ASM
/// ---------
// Declared FAR here, and only here: th04/main/boss/bosses.hpp declares the
// same function `near`, which is what it is, and that header is deliberately
// not included. A near reference under this object's `-zPmain_03` frames its
// offset on main_03, and elly_bg_render() lives in main_01. kb/codegen/0162.
void pascal far elly_bg_render(void);
/// ---------

/// Constants
/// ---------
static const int ELLY_HP = 6000;

// Each attack set costs this much, which is also what the set's own HP
// milestone is measured against.
static const int ELLY_HP_PER_SET = 1500;

// The fight starts on this [stage_frame] and not on a phase frame count.
static const int ELLY_FIGHT_START_FRAME = 9240;

// Frames the scythe spin between two patterns lasts.
static const int ELLY_SPIN_FRAMES = 32;

// Elly's own animation, played forwards over the boomerang wind-up and
// backwards while she catches it again. The first cel is also her idle pose.
// [inferred] the naming only, on the same terms as b3_fg.cpp's
// PAT_ELLY_BOOMERANG: nothing in the dump ties this patnum range to an asset.
static const int PAT_ELLY = 134;
static const int ELLY_CELS = 8;
/// ---------

/// Elly's patterns
/// ---------------
/// All fourteen of these sat directly above elly_update() in ZUN's object, and
/// every one of them is reached from its `switch(boss.mode)` or from another
/// of the fourteen and from nowhere else, so all fourteen are `static` here
/// and the eleven zero-byte `label` aliases th04_main.asm carried for them are
/// gone with the bodies. They keep the dump's address-suffixed names;
/// **a naming round is owed** for all fourteen.

/// Elly's fight state
/// ------------------
/// All of it th04_main.asm `.data?` with no `public` of ZUN's. With the
/// boomerang driver below no longer ASM, **nothing in the dump reads any of
/// these any more**, so the zero-byte `label` aliases kb/codegen/0123 needed
/// for them are gone and the dump's own labels carry the names directly.
/// **A naming round is owed** for every address-suffixed spelling here.
extern "C" {
	// Which of the five attack sets is running, 0…4. It indexes the dense
	// table that decides how [boss.mode] cycles, doubles as the explosion
	// type each set ends with, and sets the HP each set starts from.
	// `[inferred]`.
	extern unsigned char elly_pattern_set;

	// The boomerang driver's own state, 0…8. 0 is "no boomerang"; 1 and 2
	// are the wind-up and the outward flight, 3…6 the four playfield edges
	// it bounces off, 7 the flight back to Elly and 8 the catch animation.
	extern unsigned char elly_25A26;

	// Frames spent in the current [elly_25A26] state — but only states 1, 2
	// and 8 count, which is what makes it the wind-up/catch animation's cel
	// timer as well.
	extern unsigned int elly_25A34;

	// The boomerang's flight angle, and the ±1 the driver steers it by.
	extern unsigned char elly_25A36;
	extern unsigned char elly_25A38;

	// Its speed, in subpixels. Ticks up while it flies out, loses 4 on every
	// edge bounce, and gains 8 per frame on the way back.
	extern unsigned char elly_25A37;

	// The scythe orbit's tick, advanced by every pattern that calls
	// elly_1BC73().
	extern int elly_25A3A;
}

/// Elly's boomerang
/// ----------------
/// b3_fg.cpp owns the naming rationale for both of these and for the enum;
/// this is the driver every one of the behaviours it documents is measured
/// out of.
enum elly_boomerang_flag_t {
	EBF_FREE = 0,
	EBF_THROWN = 1,
	// Blitted for one last frame, then back to EBF_FREE — see b3_fg.cpp.
	EBF_CAUGHT = 2,
};

extern "C" unsigned char elly_boomerang_flag;
extern "C" PlayfieldMotion elly_boomerang_pos;
/// ----------------

// Elly's boomerang, one frame. Runs on every frame of the fight, ahead of the
// phase dispatch in elly_update(), and is the only writer of [elly_25A26].
// `static`: elly_update() is its only caller, and it is the only one that ever
// was, so the zero-byte `label` alias th04_main.asm carried for it is gone
// with the body.
static void near elly_1B95C(void)
{
	unsigned char angle_delta;

	if(elly_boomerang_flag == EBF_CAUGHT) {
		elly_boomerang_flag = EBF_FREE;
	}
	switch(elly_25A26 - 1) {
	// The wind-up: four cels over 64 frames, with the throw itself on frame
	// 0x38. Falls through into the outward flight on the frame it ends.
	case 0:
		if((elly_25A34 < 0x40) && ((elly_25A34 & 7) == 0)) {
			boss.sprite = ((elly_25A34 >> 3) + PAT_ELLY);
		}
		if(elly_25A34 == 0) {
			snd_se_play(8);
			goto tick;
		}
		if(elly_25A34 == 0x38) {
			snd_se_play(9);
			elly_boomerang_flag = EBF_THROWN;
			elly_boomerang_pos.cur.x.v = boss.pos.cur.x.v;
			elly_boomerang_pos.cur.y.v = boss.pos.cur.y.v;
			goto tick;
		}
		if(elly_25A34 < 0x40) {
			goto tick;
		}
		boss.sprite = (PAT_ELLY + (ELLY_CELS - 1));
		elly_25A26 = 2;
		// fall through

	// The outward flight: homes in on the player for the first 0x50 frames,
	// then flies straight until it reaches a playfield edge.
	case 1:
		if(elly_25A34 < 0x50) {
			angle_delta = iatan2(
				(player_pos.cur.y.v - elly_boomerang_pos.cur.y.v),
				(player_pos.cur.x.v - elly_boomerang_pos.cur.x.v)
			);
			angle_delta -= elly_25A36;
			if((angle_delta < 0x80) && (angle_delta >= 0x10)) {
				elly_25A38 = 1;
			} else if((angle_delta >= 0x80) && (angle_delta <= 0xF0)) {
				elly_25A38 = -1;
			} else {
				goto aimed;
			}
			elly_25A37 += stage_frame_mod2;
			goto steered;
aimed:
			// Already aimed: no steering, and the speed ticks up by a full
			// unit rather than by [stage_frame_mod2].
			_AL = elly_25A37;
			_AL++;
			elly_25A37 = _AL;
			elly_25A38 = 0;
steered:
			elly_25A36 += elly_25A38;
		} else {
			elly_25A37 += stage_frame_mod2;
		}
		if(elly_boomerang_pos.cur.x.v <= TO_SP(64)) {
			elly_25A26 = 3;
		} else if(elly_boomerang_pos.cur.x.v >= TO_SP(320)) {
			elly_25A26 = 4;
		} else if(elly_boomerang_pos.cur.y.v >= TO_SP(304)) {
			elly_25A26 = 5;
		} else if(elly_boomerang_pos.cur.y.v <= TO_SP(32)) {
			elly_25A26 = 6;
		}
		goto tick;

	// The four edge bounces. Each costs 4 units of speed, and the boomerang
	// turns back once that has run it down to 4 or less. Only the right edge
	// does anything else: it forces the turn to be clockwise.
	case 2:
		_AL = elly_25A37;
		_AL += -4;
		elly_25A37 = _AL;
		elly_25A36 += elly_25A38;
		if(elly_25A37 <= 4) {
			goto turn_back;
		}
		break;

	case 3:
		_AL = elly_25A37;
		_AL += -4;
		elly_25A37 = _AL;
		elly_25A36 += elly_25A38;
		if(elly_25A38 != 0) {
			// Through _AL because the original reloads it: `-Z` would
			// otherwise reuse the copy the unconditional `+=` above left
			// there, and drop this `mov` (kb/codegen/0152 probe, 4 shapes).
			_AL = elly_25A38;
			elly_25A36 += _AL;
		} else {
			elly_25A36++;
		}
		if(elly_25A37 <= 4) {
			goto turn_back;
		}
		break;

	case 4:
		_AL = elly_25A37;
		_AL += -4;
		elly_25A37 = _AL;
		elly_25A36 += elly_25A38;
		if(elly_25A37 <= 4) {
			goto turn_back;
		}
		break;

	case 5:
		_AL = elly_25A37;
		_AL += -4;
		elly_25A37 = _AL;
		elly_25A36 += elly_25A38;
		if(elly_25A37 <= 4) {
			goto turn_back;
		}
		break;

turn_back:
		elly_25A26 = 7;
		break;

	// The flight back: re-aimed at Elly every frame, accelerating, and caught
	// inside a ±16-pixel box around her.
	case 6:
		elly_25A36 = iatan2(
			(boss.pos.cur.y.v - elly_boomerang_pos.cur.y.v),
			(boss.pos.cur.x.v - elly_boomerang_pos.cur.x.v)
		);
		_AL = elly_25A37;
		_AL += 8;
		elly_25A37 = _AL;
		if(
			((boss.pos.cur.x.v + TO_SP(-16)) < elly_boomerang_pos.cur.x.v) &&
			((boss.pos.cur.x.v + TO_SP(16)) > elly_boomerang_pos.cur.x.v) &&
			((boss.pos.cur.y.v + TO_SP(-16)) < elly_boomerang_pos.cur.y.v) &&
			((boss.pos.cur.y.v + TO_SP(16)) > elly_boomerang_pos.cur.y.v)
		) {
			elly_25A26 = 8;
			elly_boomerang_flag = EBF_CAUGHT;
			elly_25A34 = 0;
		}
		break;

	// The catch animation: the same four cels, backwards, over 32 frames.
	case 7:
		if((elly_25A34 < 0x20) && ((elly_25A34 & 7) == 0)) {
			boss.sprite = (((0x1F - elly_25A34) >> 2) + PAT_ELLY);
		}
		if(elly_25A34 >= 0x20) {
			boss.sprite = PAT_ELLY;
			elly_25A26 = 0;
		}
tick:
		elly_25A34++;
		break;
	}

	// Nothing above moves the boomerang: its velocity is rebuilt from the
	// angle and speed bytes on every frame, so update_seg3() only integrates.
	if(elly_boomerang_flag == EBF_THROWN) {
		vector2(
			elly_boomerang_pos.velocity.x.v,
			elly_boomerang_pos.velocity.y.v,
			elly_25A36,
			elly_25A37
		);

		// Shots deflect the boomerang rather than destroying it. Spelled out
		// rather than through shot.hpp's 3-argument overload: binding its
		// `const subpixel_t&` radii to two literals materialises two stack
		// temporaries, and the original's frame has room for neither.
		shot_hitbox_radius.x.v = TO_SP(32);
		shot_hitbox_radius.y.v = TO_SP(32);
		shot_hitbox_center.x.v = elly_boomerang_pos.cur.x.v;
		shot_hitbox_center.y.v = elly_boomerang_pos.cur.y.v;
		// UNSIGNED, and that is what the original says rather than a
		// convenience: a signed `>> 1` is `sar` and subtracts straight into
		// memory, where the original shifts with `shr` and goes through DX.
		elly_boomerang_pos.velocity.y.v -= (
			static_cast<unsigned int>(shots_hittest()) >> 1
		);
		elly_boomerang_pos.update_seg3();

		if(
			((elly_boomerang_pos.cur.x.v + TO_SP(-24)) < player_pos.cur.x.v) &&
			((elly_boomerang_pos.cur.x.v + TO_SP(24)) > player_pos.cur.x.v) &&
			((elly_boomerang_pos.cur.y.v + TO_SP(-24)) < player_pos.cur.y.v) &&
			((elly_boomerang_pos.cur.y.v + TO_SP(24)) > player_pos.cur.y.v)
		) {
			player_is_hit = true;
		}
	}
}

// Re-arms the boomerang: eight throws, aimed at the player, from this frame.
static void near elly_1BC3C(void)
{
	elly_25A37 = 8;
	elly_25A36 = iatan2(
		(player_pos.cur.y.v - boss.pos.cur.y.v),
		(player_pos.cur.x.v - boss.pos.cur.x.v)
	);
	elly_25A26 = 1;
	elly_25A34 = 0;
	elly_boomerang_flag = 0;
	elly_25A38 = 0;
}

// One frame of Elly's scythe orbit. [elly_25A3A] is the tick every pattern
// that calls this one advances; the six ranges below are the orbit's phases,
// and [boss.pos.prev.x] is reused as its radius rather than as a position.
static void near elly_1BC73(void)
{
	if(elly_25A3A < 128) {
		boss.pos.prev.x.v += 8;
		boss.angle = 96;
	} else if(elly_25A3A < 256) {
		boss.angle--;
	} else if(elly_25A3A < 384) {
		boss.pos.prev.x.v -= 8;
	} else if(elly_25A3A < 512) {
		boss.pos.prev.x.v += 8;
		boss.angle = 32;
	} else if(elly_25A3A < 640) {
		boss.angle++;
	} else if(elly_25A3A < 768) {
		boss.pos.prev.x.v -= 8;
	} else if(elly_25A3A >= 768) {
		boss.pos.prev.x.v += 8;
		boss.angle = 96;
		elly_25A3A = 0;
	}
	boss.pos.cur.x.v = polar_x(TO_SP(192), boss.pos.prev.x.v, boss.angle);
	boss.pos.cur.y.v = polar_y(TO_SP(96), boss.pos.prev.x.v, boss.angle);
}

// Phase 1: throw the boomerang once and wait for it to come back.
static void near elly_1BD23(void)
{
	if(boss.phase_frame == 32) {
		elly_1BC3C();
	}
	if(boss.phase_frame > 32) {
		if(elly_25A26 == 0) {
			boss.mode = 255;
			boss.phase_frame = 0;
		}
	}
}

// [boss.mode] 0: the boomerang plus a 5-way spread every 16th stage frame,
// walking one sixteenth of a turn anticlockwise per volley.
static void near elly_1BD4B(void)
{
	elly_1BC73();
	elly_25A3A++;
	if(boss.phase_frame == 16) {
		elly_1BC3C();
		bullet_template.angle = -0x40;
	}
	if(boss.phase_frame > 16) {
		if(stage_frame_mod16 == 0) {
			bullet_template.spawn_type = BST_BULLET16;
			bullet_template.patnum = PAT_BULLET16_D_YELLOW;
			bullet_template.speed.v = TO_SP(3);
			bullet_template.group = BG_SPREAD;
			bullet_template.count = 5;
			bullet_template.delta.spread_angle = 0x10;
			bullet_template_tune();
			bullets_add_regular();
			_AL = bullet_template.angle;
			_AL += -0x10;
			bullet_template.angle = _AL;
		}
		if(elly_25A26 == 0) {
			boss.mode = 255;
			boss.phase_frame = 0;
		}
	}
}

// The four-frame gather animation that opens Elly's three big patterns, and
// the schedule it hands them back: 0 once it is over, 1 while it is running,
// and 2 on the one frame the pattern fires. Its `switch` is sparse, which is
// what the value/jump table pair and the one padding byte behind this function
// are (kb/codegen/0160).
#pragma option -a2
static unsigned char near elly_1BDB4(void)
{
	if(boss.phase_frame > 32) {
		return 0;
	}
	switch(boss.phase_frame) {
	case 1:
		gather_template.center.x.v = boss.pos.cur.x.v;
		gather_template.center.y.v = boss.pos.cur.y.v;
		gather_template.ring_points = 8;
		gather_template.radius.v = TO_SP(192);
		gather_template.col = V_WHITE;
gathers:
		// One ring each way, so the two cross.
		gather_template.angle_delta = -2;
		gather_add_only();
		gather_template.angle_delta = 2;
		gather_add_only();
		break;

	case 0x10:
		circles_add_shrinking(boss.pos.cur.x.v, boss.pos.cur.y.v);
		circles_color = V_WHITE;
		// fall through
	case 8:
		gather_template.col = 7;
		goto gathers;

	case 0x20:
		return 2;
	}
	return 1;
}
#pragma option -a1

// The 48-bullet backwards-cloud ring every one of those three patterns ends
// with, and the only thing that ends them.
static void near elly_1BE43(void)
{
	boss.mode = 255;
	boss.phase_frame = 0;
	bullet_template.angle = 0;
	bullet_template.spawn_type = BST_BULLET16_CLOUD_BACKWARDS;
	bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
	bullet_template.speed.v = TO_SP(2);
	bullet_template.group = BG_RING_AIMED;
	bullet_template.count = 48;
	bullet_template_tune();
	bullets_add_regular_fixedspeed();
}

// [boss.mode] 1: two sweeps of 2-way pellet spreads, the first walking
// clockwise and the second anticlockwise from a quarter turn on.
static void near elly_1BE78(void)
{
	_AX = elly_1BDB4();
	if(_AX != 0) {
		if(_AX != 2) {
			return;
		}
		_AL = iatan2(
			(player_pos.cur.y.v - boss.pos.cur.y.v),
			(player_pos.cur.x.v - boss.pos.cur.x.v)
		);
		_AL += -0x40;
		goto store_angle;
	}
	if((boss.phase_frame % 4) == 0) {
		snd_se_play(9);
	}
	if(boss.phase_frame < 72) {
		if((boss.phase_frame % 2) != 0) {
			return;
		}
		bullet_template.spawn_type = BST_PELLET;
		bullet_template.speed.v = TO_SP(4);
		bullet_template.group = BG_SPREAD;
		bullet_template.count = 2;
		bullet_template.delta.spread_angle = 0x0C;
		bullet_template_tune();
		bullets_add_regular();
		_AL = bullet_template.angle;
		_AL += 4;
		goto store_angle;
	}
	if(boss.phase_frame == 72) {
		_AL = bullet_template.angle;
		_AL += 0x40;
		goto store_angle;
	}
	if(boss.phase_frame < 144) {
		if((boss.phase_frame % 2) != 0) {
			return;
		}
		bullet_template.spawn_type = BST_PELLET;
		bullet_template.speed.v = TO_SP(4);
		bullet_template.group = BG_SPREAD;
		bullet_template.count = 2;
		bullet_template.delta.spread_angle = 0x0C;
		bullet_template_tune();
		bullets_add_regular();
		_AL = bullet_template.angle;
		_AL += -2;
store_angle:
		bullet_template.angle = _AL;
		return;
	}
	if(boss.phase_frame >= 144) {
		elly_1BE43();
	}
}

// [boss.mode] 2: the boomerang plus an aimed 8-ring every 16th frame.
static void near elly_1BF52(void)
{
	elly_1BC73();
	elly_25A3A++;
	if(boss.phase_frame == 16) {
		elly_1BC3C();
	}
	if(boss.phase_frame > 16) {
		if((boss.phase_frame % 16) == 0) {
			bullet_template.angle = 0;
			bullet_template.spawn_type = BST_PELLET;
			bullet_template.speed.v = TO_SP(2);
			bullet_template.group = BG_RING_AIMED;
			bullet_template.count = 8;
			bullets_add_regular();
		}
		if(elly_25A26 == 0) {
			boss.mode = 255;
			boss.phase_frame = 0;
		}
	}
}

// [boss.mode] 3: an accelerating fan of 4-way white spreads, walking 0x0B
// clockwise per volley.
static void near elly_1BFAB(void)
{
	_AX = elly_1BDB4();
	if(_AX != 0) {
		if(_AX != 2) {
			return;
		}
		_AL = iatan2(
			(player_pos.cur.y.v - boss.pos.cur.y.v),
			(player_pos.cur.x.v - boss.pos.cur.x.v)
		);
		_AL += -0x40;
		goto store_angle;
	}
	if((boss.phase_frame % 4) == 0) {
		snd_se_play(9);
	}
	if(boss.phase_frame < 80) {
		if((boss.phase_frame % 4) != 0) {
			return;
		}
		bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
		_AL = (boss.phase_frame / 8);
		_AL += (TO_SP(2) + 12);
		bullet_template.speed.v = _AL;
		bullet_template.group = BG_SPREAD;
		bullet_template.count = 4;
		bullet_template.delta.spread_angle = 0x0C;
		bullet_template.patnum = PAT_BULLET16_N_OUTLINED_BALL_WHITE;
		bullet_template_tune();
		bullets_add_regular();
		_AL = bullet_template.angle;
		_AL += 0x0B;
store_angle:
		bullet_template.angle = _AL;
		return;
	}
	if(boss.phase_frame >= 80) {
		elly_1BE43();
	}
}

// [boss.mode] 4: two scythe steps per frame, and a mirrored pair of aimed
// single bullets every 8th stage frame.
static void near elly_1C044(void)
{
	elly_1BC73();
	elly_25A3A++;
	elly_1BC73();
	elly_25A3A++;
	if(boss.phase_frame == 16) {
		elly_1BC3C();
		bullet_template.angle = 0x40;
		bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
		bullet_template.speed.v = TO_SP(4);
		bullet_template.group = BG_SINGLE_AIMED;
		bullet_template.patnum = PAT_BULLET16_N_OUTLINED_BALL_BLUE;
		bullet_template_tune();
	}
	if(boss.phase_frame > 16) {
		if(stage_frame_mod8 == 0) {
			bullets_add_regular();
			_AL = bullet_template.angle;
			_AL = -_AL;
			bullet_template.angle = _AL;
			bullets_add_regular();
			_AL = bullet_template.angle;
			_AL = -_AL;
			_AL += -3;
			bullet_template.angle = _AL;
			snd_se_play(3);
		}
		if(elly_25A26 == 0) {
			boss.mode = 255;
			boss.phase_frame = 0;
		}
	}
}

// [boss.mode] 5: elly_1BFAB() mirrored — the fan walks the other way, and the
// aimed frame adds a quarter turn instead of subtracting one.
static void near elly_1C0BF(void)
{
	_AX = elly_1BDB4();
	if(_AX != 0) {
		if(_AX != 2) {
			return;
		}
		_AL = iatan2(
			(player_pos.cur.y.v - boss.pos.cur.y.v),
			(player_pos.cur.x.v - boss.pos.cur.x.v)
		);
plus_quarter:
		_AL += 0x40;
		goto store_angle;
	}
	if((boss.phase_frame % 4) == 0) {
		snd_se_play(9);
	}
	if(boss.phase_frame < 80) {
		if((boss.phase_frame % 4) != 0) {
			return;
		}
		bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
		_AL = (boss.phase_frame / 8);
		_AL += (TO_SP(2) + 12);
		bullet_template.speed.v = _AL;
		bullet_template.group = BG_SPREAD;
		bullet_template.count = 4;
		bullet_template.delta.spread_angle = 0x0C;
		bullet_template.patnum = PAT_BULLET16_N_OUTLINED_BALL_WHITE;
		bullet_template_tune();
		bullets_add_regular();
		_AL = bullet_template.angle;
		_AL += -0x0B;
store_angle:
		bullet_template.angle = _AL;
		return;
	}

	// The frame the fan ends on re-enters the quarter-turn block ABOVE with
	// the stored angle instead of an aimed one, which is why that block
	// carries a label rather than being duplicated.
	if(boss.phase_frame == 80) {
		_AL = bullet_template.angle;
		goto plus_quarter;
	}
	if(boss.phase_frame >= 80) {
		elly_1BE43();
	}
}

// [boss.mode] 6: two scythe steps per frame and an aimed 16-ring every 16th.
static void near elly_1C164(void)
{
	elly_1BC73();
	elly_25A3A++;
	elly_1BC73();
	elly_25A3A++;
	if(boss.phase_frame == 16) {
		elly_1BC3C();
		bullet_template.angle = 0;
		bullet_template.spawn_type = BST_PELLET;
		bullet_template.speed.v = TO_SP(2);
		bullet_template.group = BG_RING_AIMED;
		bullet_template.count = 16;
		bullet_template_tune();
	}
	if(boss.phase_frame > 16) {
		if((boss.phase_frame % 16) == 0) {
			bullets_add_regular();
			snd_se_play(3);
		}
		if(elly_25A26 == 0) {
			boss.mode = 255;
			boss.phase_frame = 0;
		}
	}
}

// [boss.mode] 7: four random-angle 16-rings from the four corners of a 64×64
// box around Elly, all on the one frame the gather ends.
static void near elly_1C1CF(void)
{
	_AX = elly_1BDB4();
	if(_AX != 0) {
		if(_AX != 2) {
			return;
		}
		bullet_template.spawn_type = BST_PELLET;
		bullet_template.speed.v = TO_SP(2);
		bullet_template.group = BG_RING;
		bullet_template.count = 16;
		bullet_template.angle = randring2_next16();
		bullet_template.origin.x.v -= TO_SP(32);
		bullet_template_tune();
		bullets_add_regular();
		bullet_template.angle = randring2_next16();
		bullet_template.origin.x.v += TO_SP(64);
		bullets_add_regular();
		bullet_template.angle = randring2_next16();
		bullet_template.origin.x.v -= TO_SP(32);
		bullet_template.origin.y.v -= TO_SP(32);
		bullets_add_regular();
		bullet_template.angle = randring2_next16();
		bullet_template.origin.y.v += TO_SP(64);
		bullets_add_regular();
		snd_se_play(9);
	}
	if(boss.phase_frame >= 80) {
		elly_1BE43();
	}
}

// [boss.mode] 8, the last one: two scythe steps per frame, an aimed 16-ring
// every 16th, and — below 200 HP — two random-angle pellets on EVERY frame.
static void near elly_1C251(void)
{
	elly_1BC73();
	elly_25A3A++;
	elly_1BC73();
	elly_25A3A++;
	if(boss.phase_frame == 16) {
		elly_1BC3C();
	}
	if(boss.phase_frame > 16) {
		if((boss.phase_frame % 16) == 0) {
			bullet_template.spawn_type = BST_BULLET16;
			bullet_template.patnum = PAT_BULLET16_D_BLUE;
			bullet_template.speed.v = (TO_SP(3) + 8);
			bullet_template.group = BG_RING_AIMED;
			bullet_template.count = 16;
			bullet_template.angle = 0;
			bullet_template_tune();
			bullets_add_regular();
			snd_se_play(3);
		}
		if(boss.hp <= 200) {
			bullet_template.spawn_type = BST_PELLET;
			bullet_template.speed.v = TO_SP(2);
			bullet_template.group = BG_RANDOM_ANGLE;
			bullet_template.count = 2;
			bullet_template_tune();
			bullets_add_regular();
		}
		if(elly_25A26 == 0) {
			boss.mode = 255;
			boss.phase_frame = 0;
		}
	}
}
/// ---------------

void pascal far elly_update(void)
{
	elly_1B95C();

	switch(boss.phase) {
	case 0:
		elly_boomerang_flag = 0;
		elly_25A26 = 0;
		boss.phase++;
		boss.mode = 0;
		Palettes[0].c.b = 128;
		palette_changed = true;
		boss.hp = ELLY_HP;
		boss.phase_end_hp = ELLY_HP;
		break;

	case 1:
		boss.pos.update_seg3();
		switch(boss.mode) {
		case 0:
			elly_1BD23();
			break;
		case 255:
			// Four quarters of a slow left-right sweep, one per
			// [boss.phase_state], each 64 frames long.
			if(boss.phase_frame <= 64) {
				if(
					(boss.phase_state.patterns_seen == 0) ||
					(boss.phase_state.patterns_seen == 3)
				) {
					boss.pos.velocity.x.v = -TO_SP(1);
				} else if(
					(boss.phase_state.patterns_seen == 1) ||
					(boss.phase_state.patterns_seen == 2)
				) {
					boss.pos.velocity.x.v = TO_SP(1);
				}
			} else {
				if(boss.phase_state.patterns_seen < 3) {
					boss.phase_state.patterns_seen++;
				} else {
					boss.phase_state.patterns_seen = 0;
				}
				boss.mode = 0;
				boss.phase_frame = 0;
				boss.pos.velocity.x.v = 0;
			}
			break;
		}
		boss.phase_frame++;
		boss_hittest_shots_invincible();

		// The entrance is on the stage timer, so it ends at the same point
		// however long the player took to get here.
		if(stage_frame >= ELLY_FIGHT_START_FRAME) {
			boss.phase++;
			boss.phase_frame = 0;
			snd_se_play(13);
			boss.pos.velocity.y.v = 8;
			_asm mov word ptr bg_render_bombing_func, offset elly_bg_render
			tiles_bb_col = 0;
		}
		break;

	case 2:
		boss.pos.update_seg3();
		if(boss.pos.cur.x.v < TO_SP(192)) {
			boss.pos.velocity.x.v = TO_SP(2);
		} else if(boss.pos.cur.x.v >= TO_SP(193)) {
			boss.pos.velocity.x.v = -TO_SP(2);
		} else {
			boss.pos.velocity.x.v = 0;
		}
		boss_hittest_shots_invincible();
		if(boss.phase_frame >= 32) {
			boss.pos.velocity.x.v = 0;
			Palettes[0].c.b = 0;
			palette_changed = true;
			elly_25A3A = 0;
			boss.pos.cur.x.v = TO_SP(192);
			boss.pos.cur.y.v = TO_SP(96);
			boss.pos.prev.x.v = 0;
			boss_phase_next(ET_NONE, 0);
			elly_pattern_set = 0;
		}
		break;

	case 3:
		bullet_template.origin.x.v = boss.pos.cur.x.v;
		bullet_template.origin.y.v = boss.pos.cur.y.v;
		switch(boss.mode) {
		case 0:
			elly_1BD4B();
			break;
		case 1:
			elly_1BE78();
			break;
		case 2:
			elly_1BF52();
			break;
		case 3:
			elly_1BFAB();
			break;
		case 4:
			elly_1C044();
			break;
		case 5:
			elly_1C0BF();
			break;
		case 6:
			elly_1C164();
			break;
		case 7:
			elly_1C1CF();
			break;
		case 8:
			elly_1C251();
			break;
		case 255:
			if(boss.phase_frame < ELLY_SPIN_FRAMES) {
				elly_1BC73();
				elly_25A3A++;
			} else {
				boss.phase_state.patterns_seen++;
				switch(elly_pattern_set) {
				case 0:
					boss.mode = (boss.phase_state.patterns_seen % 2);
					if(boss.phase_state.patterns_seen < 8) {
						break;
					}
					// Falls through, where the four sets below jump.
set_over:
					boss_explode_small(
						static_cast<explosion_type_t>(elly_pattern_set)
					);
					boss.mode = 255;
					elly_pattern_set++;
					boss.hp = (
						ELLY_HP - (elly_pattern_set * ELLY_HP_PER_SET)
					);
					break;
				case 1:
					boss.mode = (boss.phase_state.patterns_seen % 4);
					if(boss.phase_state.patterns_seen >= 16) {
						goto set_over;
					}
					break;
				case 2:
					boss.mode = ((boss.phase_state.patterns_seen % 4) + 2);
					if(boss.phase_state.patterns_seen >= 24) {
						goto set_over;
					}
					break;
				case 3:
					boss.mode = ((boss.phase_state.patterns_seen % 4) + 4);
					if(boss.phase_state.patterns_seen >= 32) {
						goto set_over;
					}
					break;
				case 4:
					boss.mode = ((boss.phase_state.patterns_seen % 4) + 5);
					if(boss.phase_state.patterns_seen >= 40) {
						boss.phase_state.patterns_seen = 0;
						goto phase_over;
					}
					break;
				}
				boss.phase_frame = 0;
			}
			break;
		}
		if(boss_hittest_shots()) {
			boss.phase_state.defeat_bonus = true;
phase_over:
			boss.phase++;
			sparks_add_circle(
				boss.pos.cur.x, boss.pos.cur.y, TO_SP(8), 48
			);
			boss_explode_small(ET_VERTICAL);
			boss.phase_frame = 0;
		}

		// One item drop and one set skip per HP milestone, and the four
		// thresholds are not evenly spaced.
		if(
			((elly_pattern_set == 0) && (boss.hp <= 4700)) ||
			((elly_pattern_set == 1) && (boss.hp <= 3300)) ||
			((elly_pattern_set == 2) && (boss.hp <= 2100)) ||
			((elly_pattern_set == 3) && (boss.hp <= 700))
		) {
			boss_items_drop();
			bullets_clear();
			boss_score_bonus(10);
			boss_explode_small(
				static_cast<explosion_type_t>(elly_pattern_set)
			);
			boss.mode = 255;
			boss.phase_frame = 0;
			elly_pattern_set++;
		}
		break;

	case 4:
		boss.phase_frame++;
		if(boss.phase_frame == 16) {
			boss_explode_small(ET_VERTICAL);
		}
		if(boss.phase_frame == 32) {
			boss_defeat_explode_big(ET_HORIZONTAL, 40);
			snd_se_play(12);
			player_invincibility_time = BOSS_DEFEAT_INVINCIBILITY_FRAMES;
			elly_25A26 = 0;
			elly_boomerang_flag = 0;
		}
		break;

	default:
		boss_defeat_update();
		return;
	}

	homing_target.x.v = boss.pos.cur.x.v;
	homing_target.y.v = boss.pos.cur.y.v;
	hud_hp_update_and_render(boss.hp, ELLY_HP);
}
/// ----------------------------------------------------
