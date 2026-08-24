/// Stage 5 Boss - Yumeko
/// ---------------------
/// yumeko_update() was the last emitting proc of th05_main.asm's contribution
/// to main_035_TEXT: the `dw offset loc_…` run below it is the jump table its
/// own switch statement compiles to, not data the dump owns
/// (state/re/JUMP_TABLE_TAILS.md), so the C++ emits the alignment pad and the
/// table along with the function (kb/codegen 0099 + 0104).
///
/// That contribution had **no C++ successor at all**, and this file is the
/// answer to it rather than a block on it: TLINK lays a segment's
/// contributions out in link order and the root dump is the first object it is
/// handed, so a new object naming the segment lands at that segment's tail by
/// construction (kb/codegen 0112). One new translation unit and one
/// Tupfile.lua line -- no carve (kb/codegen/0080), no new segment name, no
/// group-list edit. This is the first lift out of the 0x2FE4 bytes of ZUN
/// assembly that block still holds, and every later one grows this object
/// backwards into the hole the same way th05/boss_4.cpp grew.
///
/// The `-a2` below is what puts the one-byte pad under the jump table. With
/// this object holding nothing else, the table's natural offset is the
/// function's own length, and the pad was read out of the OBJ's PUBDEF
/// records, never out of a `tcc -S` listing (kb/codegen 0159 + 0160).
///
/// What this file still reaches in th05_main.asm
/// --------------------------------------------
/// Everything Yumeko has except the two functions below: eight more bodies are
/// still ZUN assembly directly above, and so are the two pattern tables in
/// _DATA and the function pointer in _BSS that select between them. Six of
/// those bodies are reached from here, the other two only from the tables, and
/// none of the nine things this file names was published -- so the parcel adds
/// a zero-byte kb/codegen/0123 alias beside each of the nine. Every one of them
/// goes away again with the body or table it names, as the chain above is
/// lifted.

#pragma option -zCmain_035_TEXT -zPmain_03

#include "libs/master.lib/pc98_gfx.hpp"
#include "th04/main/pattern.hpp"
#include "th04/snd/snd.h"
#include "th04/main/bg.hpp"
#include "th04/main/gather.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/custom.hpp"
#include "th04/main/rank.hpp"
#include "th04/math/randring.hpp"
#include "th04/math/vector.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/player/shot.hpp"
#include "th05/main/boss/bosses.hpp"
#include "th05/main/player/player.hpp"
// Carries balanced `#pragma codeseg` pairs of its own, each closed by a bare
// one that restores this file's `-zC` default; b1.cpp, b3.cpp and b6.cpp all
// include it the same way.
#include "th05/main/bullet/laser.hpp"

// The two near function pointers this file stores point OUT of group main_03,
// and Turbo C++ frames every near code reference on the object's own `-zP`
// group unless the declaration says which segment the target is in
// (kb/codegen/0162). Declaring each one inside its real segment is what
// th05/main/boss/bosses.hpp already does for the *_bg_render() family, whose
// BOSS_BG_TEXT is in main_01 exactly like these two -- so this is that same
// balanced `#pragma codeseg` pair, not a new device. The bare pragma restores
// this file's own `-zC` default.
//
// th04/main/null.hpp is therefore NOT included: it declares nullfunc_near()
// with no segment at all, and TH04's copy of it is in a different one anyway.
#pragma codeseg PLAYER_B_TEXT
extern "C" void pascal near nullfunc_near(void);
#pragma codeseg

#pragma codeseg MIDBOSSX_TEXT
extern "C" void pascal near swords_render(void);
#pragma codeseg

#include "th05/main/bullet/sword.hpp"
#include "th05/sprites/main_pat.h"

// What this file still reaches in th05_main.asm
// ---------------------------------------------

// Yumeko's currently selected pattern, and the two tables it is picked from --
// the structural twin of [phase_2_3_pattern] and SARA_PATTERNS_PHASE_2_3 in
// b1.cpp, and of [shinki_phase_2_3_pattern] and SHINKI_PATTERNS_PHASE_2_3 in
// b6.cpp. Named on the same formula those two follow: the phase that draws
// from the table qualifies its name. Both are indexed by
// [boss.phase_state.patterns_seen] & 1, which is what sizes them at 2; the
// `dw 0` words behind each one in the dump are not indexed from anywhere and
// stay unnamed data.
extern "C" pattern_loop_func_t yumeko_pattern;
extern "C" const pattern_loop_func_t YUMEKO_PATTERNS_PHASE_2[2];
extern "C" const pattern_loop_func_t YUMEKO_PATTERNS_PHASE_5[2];

// ---------------------------------------------

// Constants
// ---------

// [inferred] Yumeko's cels, from the same PAT_STAGE base that every stage's
// own sprites start at, and spelled after PAT_SHINKI_STILL / PAT_SHINKI_CAST,
// the attested formula for this pair one boss over. Kept file-local for the
// reason th05/main/boss/b3.cpp gives for Alice's: main_pat.h has no Stage 5
// boss block yet and is shared with every other TU in this binary.
static const int PAT_YUMEKO_STILL = (PAT_STAGE + 0);
static const int PAT_YUMEKO_CAST = (PAT_STAGE + 4);

// [inferred] Only ever shown while she is moving: during her entrance, and
// while yumeko_flystep_bounce() is carrying her.
static const int PAT_YUMEKO_FLY = (PAT_STAGE + 8);

// The sword sprites this fight blits through master.lib's *_tiny*() functions.
// ZUN converts them on the first frame of the HP fill because the dialog
// script did not do it, exactly as b6balls_load() does one boss over. This
// loop is what carries the third witness for the off-by-one that
// th05/sprites/main_pat.h carried in PAT_SWORD_last: it runs to 228, which is
// PAT_DECAY_SWORD_last only once that - 1 is there.

// Always denotes the last phase that ends with that amount of HP, exactly as
// shinki_hp_t in b6.cpp does. Phases 2/5, 3/6 and 4/7 share one arm each here,
// so the numbers are the only thing that tells the two halves of the fight
// apart.
enum yumeko_hp_t {
	HP_TOTAL = 8300,
	HP_PHASE_2_END = 7500,
	HP_PHASE_4_END = 5700,
	HP_PHASE_5_END = 4500,
	HP_PHASE_7_END = 2700,
	HP_PHASE_8_END = 1200,
	HP_PHASE_10_END = 0,
};
// ---------

// Game logic
// ----------

/// The shared flight step
/// ----------------------

// The box Yumeko bounces around inside during her phase 2 and 5 pattern
// selection. Narrower and much shorter than Louise's, and not derived from
// PLAYFIELD_* in the original either.
enum yumeko_fly_box_t {
	FLY_LEFT = 48,
	FLY_RIGHT = 336,
	FLY_TOP = 48,
	FLY_BOTTOM = 128,
};

// Seeds a random angle on the first frame of the step, then walks [boss.pos]
// along it, reflecting off each edge of the fly box. Returns whether [frames]
// have passed since the phase last reset [boss.phase_frame].
//
// The structural twin of louise_flystep_random() in th05/main/boss/b2.cpp,
// down to the parameter pair and the reflection idiom, and ported from it
// rather than derived a second time. Two differences, both Yumeko's: the box
// above, and the two cel changes -- she flies on PAT_YUMEKO_FLY and settles
// back onto PAT_YUMEKO_STILL on the frame this returns true.
// See boss_flystep_random(), which is a different function with a different
// interface.
static bool pascal near yumeko_flystep_bounce(subpixel_t speed, int frames)
{
	unsigned char angle;

	if(boss.phase_frame == 1) {
		angle = randring2_next16();
		vector2(
			boss.pos.velocity.x.v, boss.pos.velocity.y.v, angle, speed
		);
		boss.sprite = PAT_YUMEKO_FLY;
	}
	boss.pos.cur.x.v += boss.pos.velocity.x.v;
	boss.pos.cur.y.v += boss.pos.velocity.y.v;
	// kb/codegen/0053's shape, the other way round: the constant goes into AX
	// and the memory word is the multiplicand, which is the one-operand
	// `IMUL`. A plain `-1 * x` folds to the three-operand form instead, one
	// byte shorter. b2.cpp reflects Louise the same way.
	if(
		(boss.pos.cur.x.v <= TO_SP(FLY_LEFT)) ||
		(boss.pos.cur.x.v >= TO_SP(FLY_RIGHT))
	) {
		_AX = -1;
		_asm imul word ptr [boss+8]
		boss.pos.velocity.x.v = _AX;
	}
	if(
		(boss.pos.cur.y.v <= TO_SP(FLY_TOP)) ||
		(boss.pos.cur.y.v >= TO_SP(FLY_BOTTOM))
	) {
		_AX = -1;
		_asm imul word ptr [boss+10]
		boss.pos.velocity.y.v = _AX;
	}
	if(boss.phase_frame >= frames) {
		boss.sprite = PAT_YUMEKO_STILL;
		return true;
	}
	return false;
}
/// ----------------------

// Yumeko's seven pattern bodies, in their original order. Address-suffixed
// hand names, not placeholders: what each one shoots is measurable and is
// described above it, but nothing in the fight names them and there is no
// sibling boss to mirror a name off, so the address is the only thing that
// distinguishes them without inventing a reading.
//
// All seven open the same way -- a 16-frame 3-stack gather, then a one-shot
// setup on the frame the gather ends -- and all but the two that reset
// [boss.mode] themselves run until yumeko_update() takes them off.

// Phase 2's first pattern: a fan of swords sweeping across the playfield six
// angle units at a time, mirrored around 0x80 on half the runs.
void near yumeko_1CA42(void)
{
	if(boss.phase_frame == 16) {
		boss.sprite = PAT_YUMEKO_CAST;
		sword_template.twirl_time = 48;
		sword_template.speed.set(5.0f);
		sword_template.angle = 0x70;
		boss_statebyte[15] = randring2_next16_and(1);
	} else if((boss.phase_frame > 32) && ((boss.phase_frame % 2) == 0)) {
		if(boss_statebyte[15] != 0) {
			sword_template.angle = (0x80 - sword_template.angle);
		}
		vector2_at(
			sword_template.origin,
			boss.pos.cur.x.v,
			boss.pos.cur.y.v,
			to_sp(48.0f),
			sword_template.angle
		);
		swords_add();
		if(boss_statebyte[15] != 0) {
			sword_template.angle = (0x80 - sword_template.angle);
		}
		sword_template.angle -= 6;
		if(sword_template.angle <= 0x0C) {
			boss.phase_frame = 0;
			boss.mode = 0;
		}
	}
}

// Phase 2's second pattern: a ring of red balls, then one further ball every
// 4th frame at a random angle and a random distance from Yumeko.
void near yumeko_1CAD7(void)
{
	if(boss.phase_frame < 32) {
		gather_add_only_3stack((boss.phase_frame - 16), 7, 6);
		if(boss.phase_frame == 16) {
			boss.sprite = PAT_YUMEKO_CAST;
			bullet_template.spawn_type = (
				BST_CLOUD_BACKWARDS | BST_NO_DECELERATE
			);
			bullet_template.patnum = PAT_BULLET16_N_BALL_RED;
			bullet_template.group = BG_RING;
			bullet_template.speed.set(3.75f);
			bullet_template.spread = 16;
			bullet_template_tune();
			snd_se_play(8);
		}
	} else {
		if((boss.phase_frame % 4) == 0) {
			bullet_template.angle = randring2_next16();
			vector2_at(
				bullet_template.origin,
				boss.pos.cur.x.v,
				boss.pos.cur.y.v,
				randring2_next16_mod(to_sp(32.0f)),
				bullet_template.angle
			);
			bullets_add_regular();
			snd_se_play(3);
		}
		if(boss.phase_frame == 80) {
			boss.phase_frame = 0;
			boss.mode = 0;
		}
	}
}

// Phase 4: two aimed arrowheads from the left and right of Yumeko every other
// 64-frame tick, plus one thrown sword every [yumeko_interval_phase4] frames.
// 600 HP before the phase ends, the sword interval and the arrowhead window
// both tighten by rank and a small explosion fires once.
void near yumeko_1CB71(void)
{
	unsigned char angle;
	int tick;

	if(boss.phase_frame < 32) {
		gather_add_only_3stack((boss.phase_frame - 16), 7, 6);
		if(boss.phase_frame == 16) {
			boss.sprite = PAT_YUMEKO_CAST;
			bullet_template.spawn_type = (
				BST_CLOUD_FORWARDS | BST_NO_DECELERATE
			);
			bullet_template.patnum = PAT_BULLET16_V_RED;
			bullet_template.group = BG_SINGLE;
			bullet_template.speed.set(6.0f);
			bullet_template_tune();
			snd_se_play(8);
			sword_template.twirl_time = 32;
			sword_template.speed.set(4.75f);
			boss_statebyte[13] = 0;
			boss_statebyte[12] = 0x20;
		}
	} else {
		tick = (boss.phase_frame % 64);
		if((boss_statebyte[12] > tick) && !(tick & 1)) {
			bullet_template.origin.x.v -= to_sp(32.0f);
			bullet_template.origin.y.v -= to_sp(16.0f);
			if(tick == 0) {
				boss_statebyte[15] = player_angle_from(
					bullet_template.origin.x, bullet_template.origin.y, 0
				);
				boss_statebyte[14] = player_angle_from(
					(bullet_template.origin.x.v + to_sp(64.0f)),
					bullet_template.origin.y,
					0
				);
			}
			bullet_template.angle = boss_statebyte[15];
			bullets_add_regular();
			bullet_template.origin.x.v += to_sp(64.0f);
			bullet_template.angle = boss_statebyte[14];
			bullets_add_regular();
			snd_se_play(3);
		}
		if((boss.phase_frame % yumeko_interval_phase4) == 0) {
			angle = (randring2_next16_and(0x1F) - 0x0F);
			sword_template.origin.y.v = randring2_next16_mod(to_sp(96.0f));
			sword_template.origin.x.v = (
				randring2_next16_mod(to_sp(352.0f)) + to_sp(16.0f)
			);
			sword_template.angle = player_angle_from(
				sword_template.origin.x, sword_template.origin.y, angle
			);
			swords_add();
		}
		if(
			((boss.hp - boss.phase_end_hp) < 600) && (boss_statebyte[13] == 0)
		) {
			boss_statebyte[13] = 1;
			yumeko_interval_phase4 = select_for_rank(16, 8, 4, 4);
			boss_statebyte[12] = select_for_rank(40, 48, 52, 52);
			boss_explode_small(ET_CIRCLE);
			if(bullet_clear_time < 20) {
				bullet_clear_time = 20;
			}
		}
	}
}

// Phase 5's first pattern: two BSM_EXACT_LINEAR spread pairs every 8th frame,
// one at a 48-pixel radius and one at 32, with the base angle negated between
// the two pairs and walked back by 8 afterwards.
void near yumeko_1CCD3(void)
{
	if(boss.phase_frame < 16) {
		gather_add_only_3stack((boss.phase_frame - 1), 7, 6);
		if(boss.phase_frame == 4) {
			boss.sprite = PAT_YUMEKO_CAST;
			bullet_template.spawn_type = BST_NO_DECELERATE;
			bullet_template.group = BG_SPREAD;
			bullet_template.special_motion = BSM_EXACT_LINEAR;
			bullet_template.speed.set(3.5f);
			bullet_template.set_spread(5, 2);
			snd_se_play(8);
			boss_statebyte[15] = 0x60;
			boss_statebyte[14] = 0x40;
		}
	} else {
		if((boss.phase_frame % 8) == 0) {
			bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
			vector2_at(
				bullet_template.origin,
				boss.pos.cur.x.v,
				boss.pos.cur.y.v,
				to_sp(48.0f),
				boss_statebyte[15]
			);
			bullet_template.angle = (boss_statebyte[15] + boss_statebyte[14]);
			bullets_add_special();
			vector2_at(
				bullet_template.origin,
				boss.pos.cur.x.v,
				boss.pos.cur.y.v,
				to_sp(48.0f),
				static_cast<unsigned char>(boss_statebyte[15] + 0x80)
			);
			bullet_template.angle += 0x80;
			bullets_add_special();

			bullet_template.patnum = 0;
			boss_statebyte[15] = -boss_statebyte[15];
			vector2_at(
				bullet_template.origin,
				boss.pos.cur.x.v,
				boss.pos.cur.y.v,
				to_sp(32.0f),
				boss_statebyte[15]
			);
			bullet_template.angle = (boss_statebyte[15] - boss_statebyte[14]);
			bullets_add_special();
			vector2_at(
				bullet_template.origin,
				boss.pos.cur.x.v,
				boss.pos.cur.y.v,
				to_sp(32.0f),
				static_cast<unsigned char>(boss_statebyte[15] + 0x80)
			);
			bullet_template.angle += 0x80;
			bullets_add_special();

			boss_statebyte[15] = -boss_statebyte[15];
			boss_statebyte[15] -= 8;
			boss_statebyte[14] -= 6;
			snd_se_play(3);
		}
		if(boss.phase_frame == 256) {
			boss.phase_frame = 0;
			boss.mode = 0;
		}
	}
}

// Phase 5's second pattern: an aimed 5-spread and a mirrored 4-spread of green
// arrowheads every 8th frame, the pair walking around Yumeko by 8 angle units
// at a time.
void near yumeko_1CE0D(void)
{
	if(boss.phase_frame < 16) {
		gather_add_only_3stack((boss.phase_frame - 1), 7, 6);
		if(boss.phase_frame == 4) {
			boss.sprite = PAT_YUMEKO_CAST;
			bullet_template.spawn_type = BST_NO_DECELERATE;
			bullet_template.group = BG_SPREAD_AIMED;
			bullet_template.speed.set(3.5f);
			bullet_template.spread_angle_delta = 20;
			bullet_template.angle = 0;
			bullet_template.patnum = PAT_BULLET16_D_GREEN;
			bullet_template_tune();
			snd_se_play(8);
			boss_statebyte[15] = 0x80;
		}
	} else {
		if((boss.phase_frame % 8) == 0) {
			vector2_at(
				bullet_template.origin,
				boss.pos.cur.x.v,
				boss.pos.cur.y.v,
				to_sp(48.0f),
				boss_statebyte[15]
			);
			bullet_template.spread = 5;
			bullets_add_regular();
			vector2_at(
				bullet_template.origin,
				boss.pos.cur.x.v,
				boss.pos.cur.y.v,
				to_sp(48.0f),
				static_cast<unsigned char>(0x80 - boss_statebyte[15])
			);
			bullet_template.spread = 4;
			bullets_add_regular();
			boss_statebyte[15] += 8;
			snd_se_play(3);
		}
		if(boss.phase_frame == 192) {
			boss.phase_frame = 0;
			boss.mode = 0;
		}
	}
}

// Phase 7: yumeko_1CB71()'s arrowhead pair again, but with blue arrowheads,
// one aimed shoot-out laser and TWO thrown swords per interval, and 500 HP
// rather than 600 as the tightening point.
void near yumeko_1CED9(void)
{
	unsigned char angle;
	int i;
	int tick;

	if(boss.phase_frame < 32) {
		gather_add_only_3stack((boss.phase_frame - 16), 7, 6);
		if(boss.phase_frame == 16) {
			boss.sprite = PAT_YUMEKO_CAST;
			bullet_template.spawn_type = (
				BST_CLOUD_FORWARDS | BST_NO_DECELERATE
			);
			bullet_template.patnum = PAT_BULLET16_V_BLUE;
			bullet_template.group = BG_SINGLE;
			bullet_template.speed.set(6.0f);
			bullet_template_tune();
			snd_se_play(8);
			laser_template.age = 24;
			laser_template.shootout_speed.set(6.25f);
			laser_template.coords.width.nonshrink = 6;
			laser_template.col = 8;

			// The same union member th05/main/boss/b2.cpp spells [moveout]
			// for its own shoot-out laser; th05_main.asm's equate for the
			// offset is the fixed-laser reading of it.
			laser_template.active_at_age.moveout = 28;

			boss_statebyte[13] = 0;
			boss_statebyte[12] = 0x20;
			sword_template.twirl_time = 32;
			sword_template.speed.set(4.75f);
		}
	} else {
		tick = (boss.phase_frame % 64);
		if((boss_statebyte[12] > tick) && !(tick & 1)) {
			bullet_template.origin.x.v -= to_sp(32.0f);
			bullet_template.origin.y.v -= to_sp(16.0f);
			if(tick == 0) {
				boss_statebyte[15] = player_angle_from(
					bullet_template.origin.x, bullet_template.origin.y, 0
				);
				boss_statebyte[14] = player_angle_from(
					(bullet_template.origin.x.v + to_sp(64.0f)),
					bullet_template.origin.y,
					0
				);
			}
			bullet_template.angle = boss_statebyte[15];
			bullets_add_regular();
			bullet_template.origin.x.v += to_sp(64.0f);
			bullet_template.angle = boss_statebyte[14];
			bullets_add_regular();
			snd_se_play(3);
		}
		if((boss.phase_frame % yumeko_interval_phase7) == 0) {
			laser_template.coords.origin.y.v = to_sp(32.0f);
			laser_template.coords.origin.x.v = (
				randring2_next16_mod(to_sp(256.0f)) + to_sp(64.0f)
			);
			laser_template.coords.angle = player_angle_from(
				laser_template.coords.origin.x,
				laser_template.coords.origin.y,
				0
			);
			lasers_shootout_add();
			for(i = 0; i < 2; i++) {
				angle = (randring2_next16_and(0x1F) - 0x0F);
				sword_template.origin.y.v = randring2_next16_mod(to_sp(96.0f));
				sword_template.origin.x.v = (
					randring2_next16_mod(to_sp(352.0f)) + to_sp(16.0f)
				);
				sword_template.angle = player_angle_from(
					sword_template.origin.x, sword_template.origin.y, angle
				);
				swords_add();
			}
		}
		if(
			((boss.hp - boss.phase_end_hp) < 500) && (boss_statebyte[13] == 0)
		) {
			boss_statebyte[13] = 1;
			yumeko_interval_phase7 = select_for_rank(34, 28, 20, 20);
			boss_statebyte[12] = select_for_rank(40, 48, 52, 48);
			boss_explode_small(ET_CIRCLE);
			if(bullet_clear_time < 20) {
				bullet_clear_time = 20;
			}
		}
	}
}

// Phase 8: swords thrown in from alternating sides while a horizontal band
// walks down the playfield, then five accelerating aimed volleys of blue
// arrowheads. [boss2.pos.cur.y] is the band's height -- Yuki's leftover
// coordinate reused as scratch state.
void near yumeko_1D085(void)
{
	if(boss.phase_frame < 32) {
		gather_add_only_3stack((boss.phase_frame - 16), 7, 6);
		if(boss.phase_frame != 16) {
			return;
		}
		boss.sprite = PAT_YUMEKO_CAST;
		bullet_template.spawn_type = BST_NO_DECELERATE;
		bullet_template.patnum = PAT_BULLET16_D_BLUE;
		bullet_template.group = BG_SPREAD;
		bullet_template.spread = 5;
		bullet_template.spread_angle_delta = select_for_rank(24, 16, 12, 10);
		bullet_template.speed.set(8.0f);
		snd_se_play(8);
		sword_template.twirl_time = 32;
		sword_template.speed.set(4.0f);
		boss2.pos.cur.y.v = randring2_next16_mod(to_sp(32.0f));
		boss_statebyte[15] = select_for_rank(0x28, 0x1E, 0x18, 0x10);
		boss_statebyte[14] = 0;
		boss_statebyte[13] = 0;
	} else if(boss_statebyte[13] == 0) {
		if((boss.phase_frame % 8) != 0) {
			return;
		}
		sword_template.angle = boss_statebyte[14];
		if(boss_statebyte[14] == 0) {
			sword_template.origin.x.v = to_sp(16.0f);
		} else {
			sword_template.origin.x.v = to_sp(PLAYFIELD_W - 16);
		}
		sword_template.origin.y.v = boss2.pos.cur.y.v;
		swords_add();
		boss2.pos.cur.y.v += (boss_statebyte[15] << 4);
		boss_statebyte[14] += 0x80;
		if(boss2.pos.cur.y.v >= to_sp(376.0f)) {
			boss2.pos.cur.y.v = randring2_next16_mod(to_sp(32.0f));
			boss_statebyte[13]++;
		}
	} else if((boss.phase_frame % 4) == 0) {
		if(bullet_template.speed.v > to_sp8(6.0f)) {
			bullet_template.speed.v = 8;
			snd_se_play(15);
			boss_statebyte[13]++;
			if(boss_statebyte[13] > 5) {
				boss_statebyte[13] = 0;
				return;
			}
			bullet_template.angle = player_angle_from(
				bullet_template.origin.x, bullet_template.origin.y, 0
			);
		}
		bullet_template.speed.v += 8;
		bullets_add_regular();
	}
}

// Phase 10: a 3-stack gather, then one BSM_DECELERATE_THEN_TURN ring every
// 16th frame, alternately turning clockwise and counter-clockwise and mirroring
// its base angle around 0x80 on the clockwise ones.
//
// This body is in the parcel for a second, purely mechanical reason: it is
// 0xA5 bytes long, and the ODD prefix is what puts the `-a2` pad in front of
// yumeko_update()'s jump table. At a zero prefix the pad is absent -- read off
// this object's own SEGDEF, not off a listing, per kb/codegen 0159 + 0160,
// which is also why the direction was probed rather than predicted.
//
// [boss_statebyte] slots used here are left as raw indices, the way
// th04/main/midboss/mx_update.cpp leaves its own: [14] and [15] are shared
// with the six Yumeko bodies still in th05_main.asm, and naming them from one
// of the seven is how a name gets contradicted by the next lift.
void near yumeko_1D1C6(void)
{
	if(boss.phase_frame < 32) {
		gather_add_only_3stack((boss.phase_frame - 16), 7, 6);
		if(boss.phase_frame == 16) {
			boss.sprite = PAT_YUMEKO_CAST;
			bullet_template.spawn_type = BST_NO_DECELERATE;
			bullet_template.patnum = PAT_BULLET16_V_BLUE;
			bullet_template.group = BG_RING;
			bullet_template.spread = 18;
			bullet_template.special_motion = BSM_DECELERATE_THEN_TURN;
			bullet_template.speed.set(2.375f);
			bullet_template_tune();
			snd_se_play(8);
			boss_statebyte[15] = 0;
			bullet_special.turns_max = 2;
			boss_statebyte[14] = 0;
		}
	} else if((boss.phase_frame % 16) == 0) {
		bullet_template.angle = boss_statebyte[14];
		if(boss_statebyte[15] & 1) {
			bullet_template_special_angle.turn_by = 0x40;
			bullet_template.angle = (0x80 - bullet_template.angle);
		} else {
			bullet_template_special_angle.turn_by = -0x40;
		}
		bullets_add_special_fixedspeed();
		boss_statebyte[14]++;
		boss_statebyte[15]++;
		snd_se_play(3);
	}
}

#pragma option -a2

void pascal yumeko_update(void)
{
	int i;

	homing_target = boss.pos.cur;
	bullet_template.origin = boss.pos.cur;
	gather_template.center = boss.pos.cur;
	sword_template.origin = boss.pos.cur;
	boss.phase_frame++;

	switch(boss.phase) {
	case PHASE_HP_FILL:
		if(boss.phase_frame == 1) {
			boss.hp = HP_TOTAL;
			boss.phase_end_hp = HP_PHASE_2_END;
			gather_template.radius.set(BOSS_W / 1.0f);
			gather_template.angle_delta = 0x02;
			gather_template.ring_points = 8;
			boss.sprite = PAT_YUMEKO_STILL;
			boss.pos.velocity.x.set(4.0f);
			for(i = TINY_SWORD_START; i < TINY_SWORD_END; i++) {
				super_convert_tiny(i);
			}
			boss_sprite_left = PAT_YUMEKO_STILL;
			boss_sprite_right = PAT_YUMEKO_STILL;
			boss_sprite_stay = PAT_YUMEKO_STILL;
		}

		// Yuki's sprite is still on screen from the Stage 4 fight, and gets
		// pushed off the top of the playfield here rather than by the stage
		// transition. She stays shootable the whole way up.
		if(boss2.pos.cur.y >= to_sp(-32.0f)) {
			boss2.pos.cur.y.v -= to_sp(1.0f);

			// Spelled out rather than through the shots_hittest() overload
			// that takes the box: that one copies the two radii through stack
			// temporaries, and this function has no stack frame at all.
			shot_hitbox_radius.x.v = to_sp(24.0f);
			shot_hitbox_radius.y.v = to_sp(24.0f);
			shot_hitbox_center = boss2.pos.cur;
			if(shots_hittest()) {
				snd_se_play(10);
			}
		}

		if(boss.phase_frame < 64) {
			boss_hittest_shots_invincible();
		} else {
			boss.sprite = PAT_YUMEKO_FLY;
			boss.pos.cur.x.v += to_sp(2.0f);

			// Timeout condition
			if(boss.pos.cur.x.v >= to_sp(192.0f)) {
				// Next phase
				boss.sprite = PAT_YUMEKO_STILL;
				boss.phase++;
				boss.phase_frame = 0;
				snd_se_play(13);
				bg_render_bombing_func = yumeko_bg_render;
			}
		}
		break;

	case PHASE_BOSS_ENTRANCE_BB:
		boss_hittest_shots_invincible();

		// Timeout condition
		if(boss.phase_frame >= 64) {
			// Next phase
			boss.phase++;
			boss.phase_frame = 0;
			boss.mode = 1;
			boss.phase_state.patterns_seen = 0;
			yumeko_pattern = yumeko_1CA42;
			boss_custombullets_render = swords_render;
		}
		break;

	case 2:
	case 5:
		switch(boss.mode) {
		case 0:
			if(yumeko_flystep_bounce(to_sp(2.0f), 64)) {
				boss.phase_frame = 0;
				boss.mode++;
				boss.phase_state.patterns_seen++;

				// Timeout condition
				if(boss.phase_state.patterns_seen >= 20) {
					goto phase_2_5_timed_out;
				}
				if(boss.phase == 2) {
					yumeko_pattern = YUMEKO_PATTERNS_PHASE_2[
						boss.phase_state.patterns_seen & 1
					];
				} else {
					yumeko_pattern = YUMEKO_PATTERNS_PHASE_5[
						boss.phase_state.patterns_seen & 1
					];
				}
			}
			break;

		// [measured] a second labelled arm, not a default one: the dispatch
		// compiles to a zero test, a compare against 1, and a fallthrough
		// jump, which is a two-label switch with no default arm rather than a
		// one-label one.
		case 1:
			yumeko_pattern();
			break;
		}
		if((boss.mode != 0) && boss_hittest_shots()) {
			boss_score_bonus(10);
phase_2_5_timed_out:
			// Next phase
			if(boss.phase == 2) {
				boss_phase_next(ET_CIRCLE, HP_PHASE_4_END);
			} else {
				boss_phase_next(ET_HORIZONTAL, HP_PHASE_7_END);
			}
		}
		break;

	case 3:
	case 6:
		boss_hittest_shots();
		if(boss_flystep_towards(to_sp(PLAYFIELD_W / 2), to_sp(64.0f))) {
			// Next phase
			boss.phase++;
			boss.phase_frame = 0;
			if(boss.phase == 4) {
				yumeko_pattern = yumeko_1CB71;
			} else {
				yumeko_pattern = yumeko_1CED9;
			}
		}
		break;

	case 4:
	case 7:
		yumeko_pattern();
		if(boss.phase_frame < 2000) {
			if(!boss_hittest_shots()) {
				break;
			}
			boss_score_bonus(10);
		}

		// Next phase
		if(boss.phase == 4) {
			boss_phase_next(ET_NW_SE, HP_PHASE_5_END);
			yumeko_pattern = yumeko_1CCD3;
		} else {
			boss_phase_next(ET_NW_SE, HP_PHASE_8_END);
		}
		boss.mode = 1;
		break;

	case 8:
		yumeko_1D085();
		if(boss.phase_frame < 2000) {
			if(!boss_hittest_shots()) {
				break;
			}
			boss_score_bonus(10);
		}

		// Next phase
		boss_phase_next(ET_NW_SE, HP_PHASE_10_END);
		break;

	case 9:
		boss_hittest_shots();
		if(boss_flystep_towards(to_sp(PLAYFIELD_W / 2), to_sp(96.0f))) {
			// Next phase
			boss.phase++;
			boss.phase_frame = 0;
			boss.phase_state.patterns_seen = 0;
		}
		break;

	case 10:
		yumeko_1D1C6();
		if(boss.phase_frame < 1200) {
			if(!boss_hittest_shots()) {
				break;
			}
			boss.phase_state.defeat_bonus = true;
		}

		// Next phase
		boss_explode_small(ET_VERTICAL);
		boss.phase_frame = 0;
		boss.phase = PHASE_BOSS_EXPLODE_SMALL;
		boss_custombullets_render = nullfunc_near;
		break;

	default:
		boss_defeat_update(65);
		return;
	}

	swords_update();
	hud_hp_update_and_render(boss.hp, HP_TOTAL);
}

#pragma option -a1
// ----------
