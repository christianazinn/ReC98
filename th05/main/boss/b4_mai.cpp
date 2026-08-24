/// Stage 4 Boss - Mai, solo
/// ------------------------
/// The update function for the second half of the Stage 4 fight, entered when
/// Yuki is the one who was defeated first. th05_main.asm's mai_yuki_update()
/// installs it into [boss_update] in the same block that loads _DM08.TX2, and
/// installs yuki_update() there instead when Mai went down first -- whichever
/// of the pair survives arrives in [boss], which is the arrangement
/// th05/main/boss/b4_solo_fg.cpp documents from the rendering side and
/// th05/main/boss/b4_both.cpp spells as `#define mai boss` / `#define yuki
/// boss2`.
///
/// yuki_update() is the same function with Yuki's cels, HP and ball colour,
/// and is th05/main/boss/b4_yuki.cpp, which this file includes below because
/// Yuki's half sits ahead of Mai's in the segment. This body is the one the
/// root's contribution to main_035_TEXT ended with when it was lifted,
/// followed by the one-byte `-a2` pad and the ten-entry jump table that its
/// ten-case dispatch compiles to (kb/codegen 0104 + 0154 + 0160,
/// state/re/JUMP_TABLE_TAILS.md).
///
/// Compiled into th05/b4mai.cpp, an object of its own. th05/swords.cpp is the
/// segment's next contribution after the root, so this object's Tupfile.lua
/// line has to come BEFORE that one: TLINK lays a segment's contributions out
/// in link order (kb/codegen 0112 + 0114). An `#include` at the front of
/// th05/swords.cpp would have saved the line, but that object reaches
/// th05/main/bullet/sword.hpp, which is unguarded, so anything in front of it
/// either collides or forces five headers up the include order of a
/// translation unit that is already matched (kb/codegen/0129).

// b4balls_render() is in MIDBOSSX_TEXT and nullfunc_near() in PLAYER_B_TEXT,
// both group main_01, and this object is -zPmain_03. Turbo C++ frames every
// near code reference on the object's own group unless the declaration says
// which segment the target is in (kb/codegen/0162); the balanced
// `#pragma codeseg` pair is the same device th05/main/boss/bosses.hpp uses for
// the *_bg_render() family, and the bare pragma restores this file's own `-zC`
// default. th04/main/null.hpp is therefore NOT included: it declares
// nullfunc_near() with no segment at all.
//
// These come BEFORE th05/main/bullet/b4ball.hpp, and that order is load
// bearing: the first declaration a translation unit sees is the one Turbo
// frames the reference on, measured -- b4ball.hpp's own plain declaration,
// reached first, framed the store on MAIN_03 and left one instruction
// divergent. The pair cannot live in b4ball.hpp instead, because a
// `#pragma codeseg` in a shared header splits the segment contribution of
// every object that includes it, which moved b4balls_render() itself.
#pragma codeseg MIDBOSSX_TEXT
extern "C" void pascal near b4balls_render(void);
#pragma codeseg

#pragma codeseg PLAYER_B_TEXT
extern "C" void pascal near nullfunc_near(void);
#pragma codeseg

#include "th04/main/pattern.hpp"
#include "th04/snd/snd.h"
#include "th04/main/bg.hpp"
#include "th04/main/circle.hpp"
#include "th04/main/custom.hpp"
#include "th04/main/frames.h"
#include "th04/main/gather.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/rank.hpp"
#include "th04/math/randring.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th05/main/boss/bosses.hpp"
#include "th05/main/bullet/laser.hpp"
#include "th05/main/player/player.hpp"
#include "th05/sprites/main_pat.h"
#include "th05/main/bullet/b4ball.hpp"

// What this file still reaches in th05_main.asm
// --------------------------------------------

// The currently selected pattern of whichever of the pair is fighting solo,
// and the three tables the three pattern phases pick it from. One slot serves
// both characters because only one of them is ever alive here; yuki_update()
// drives the same variable from three tables of its own. Structural twin of
// [yumeko_pattern] and YUMEKO_PATTERNS_PHASE_2 in th05/main/boss/b5.cpp, and
// named on the same formula: the phase that draws from the table qualifies its
// name. All three are indexed by [boss.phase_state.patterns_seen] & 1, which
// is what sizes them at 2.
extern "C" pattern_oneshot_func_t mai_yuki_pattern;
extern "C" const pattern_oneshot_func_t MAI_PATTERNS_PHASE_3[2];
extern "C" const pattern_oneshot_func_t MAI_PATTERNS_PHASE_7[2];
extern "C" const pattern_oneshot_func_t MAI_PATTERNS_PHASE_9[2];

/// The phase-5 laser pattern's own state
/// -------------------------------------
/// Four words in _BSS and a third pattern table, all reached only from
/// mai_1C34B() below and all still ZUN's storage. Zero-byte kb/codegen/0123
/// aliases; the naming is inferred from that one reader.

// How many manually controlled fixed lasers the pattern spawned: [rank] + 5,
// so 5 on Easy to 8 on Lunatic. MAI_LASER_SLOTS below is the ceiling
// mai_update() cleans up over, and is deliberately larger.
extern "C" int mai_laser_count;

// Angular velocity of the whole laser fan, in 1/16ths of an angle unit per
// frame. Ramps up to 0x80 over the first 112 frames of each 256-frame cycle,
// back down to 0 by frame 192, then up again.
extern "C" int mai_laser_angle_speed;

// [mai_laser_angle_speed] accumulated until it is worth a whole angle unit;
// the remainder carries over to the next frame.
extern "C" int mai_laser_angle_progress;

// The bullet pattern that runs alongside the lasers, re-picked once per
// 256-frame cycle from the three below, in order.
extern "C" pattern_loop_func_t mai_laser_bullet_pattern;
extern "C" const pattern_loop_func_t MAI_LASER_BULLET_PATTERNS[3];
/// -------------------------------------

// --------------------------------------------

// Mai's HP thresholds, each spelled after the phase it ends, the way
// th05/main/boss/b5.cpp spells Yumeko's. Yuki's five are in
// th05/main/boss/b4_yuki.cpp, which shares this translation unit, which is why
// both sets carry the character's name.
static const int MAI_HP_TOTAL = 7800;
static const int MAI_HP_PHASE_3_END = 5800;
static const int MAI_HP_PHASE_5_END = 2800;
static const int MAI_HP_PHASE_7_END = 1200;
static const int MAI_HP_PHASE_9_END = 0;

// Laser slots the phase-5 pattern can occupy. Only [mai_laser_count] of them
// are ever spawned, but mai_update() stops all ten when the phase ends.
static const int MAI_LASER_SLOTS = 10;

// [inferred] PAT_MAI + 12, the one cel of Mai's block that
// th05/main/boss/b4_solo_fg.cpp animates over four frames. Every pattern that
// fires bullets switches her to it. Kept file-local for the reason
// b4_solo_fg.cpp gives for spelling the same number 192 there: what the cel
// depicts has not been decided, only where it is.
static const int PAT_MAI_ANIMATED = (PAT_MAI + 12);

// The Stage 4 midboss's update function and its four helpers, which sit above
// the ball bullets in this segment and are therefore the first five bodies
// this object emits. Included from here rather than from th05/b4mai.cpp for
// the same reason as everything below: this file owns the object's unguarded
// headers (kb/codegen/0112 trap 0).
#include "th05/main/midboss/m4_updt.cpp"

// The ball bullets' reset and spawn functions, which sit above the state
// update in this segment and are therefore the first two bodies this object
// emits. They replace the assembly module th05_main.asm included at the end of
// its main_035_TEXT contribution, so this include has to come before the one
// below.
#include "th05/main/bullet/b4balls_add.cpp"

// The ball bullets' state update, which sits above Yuki's half in this segment
// and is therefore the FIRST body this object emits. Included from here for
// the same two reasons the Yuki include below carries: nothing above this line
// emits code, and this file owns the object's unguarded headers
// (kb/codegen/0112 trap 0). The one header b4balls_update() needs and this
// file does not include is unguarded too, so that file includes it and no
// later file in the object may (kb/codegen/0129).
#include "th05/main/bullet/b4balls_update.cpp"

// Yuki's half of the solo fight, which sits AHEAD of Mai's in this segment and
// therefore ahead of it in this object. Included from here rather than from
// th05/b4mai.cpp because it needs [mai_yuki_pattern] above, and because this
// file owns the object's unguarded headers (kb/codegen/0112 trap 0) -- moving
// them into the wrapper to put that file first would reorder nothing and risk
// the include-hoisting class for no gain.
#include "th05/main/boss/b4_yuki.cpp"

/// Danmaku patterns
/// ----------------
/// In their original address order, which is also the order the three
/// MAI_PATTERNS_PHASE_* tables and MAI_LASER_BULLET_PATTERNS index them in.
/// Every one of the seven "oneshot" patterns returns `true` on the frame it
/// hands the phase back, and every one of the three the laser pattern drives
/// returns nothing at all -- which is the whole difference between
/// pattern_oneshot_func_t and pattern_loop_func_t.
///
/// The three tail conditions the oneshot ones share, in ZUN's order: an exact
/// [boss.phase_frame] equality that ends the pattern, and then a random flight
/// step that only starts on the third pattern of the phase.
/// ----------------

// Phase 3, pattern A: eight spread pairs every 16 frames, each pair faster and
// wider than the last.
bool near mai_1BD2C(void)
{
	int i;

	if(boss.phase_frame == 32) {
		bullet_template.spawn_type = BST_NO_DECELERATE;
		bullet_template.group = BG_SPREAD;
		bullet_template.patnum = PAT_BULLET16_N_SMALL_BALL_BLUE;
		bullet_template.spread = 2;
		bullet_template.angle = 0x40;
		boss.sprite = PAT_MAI_ANIMATED;
	} else if(boss.phase_frame > 32) {
		if((boss.phase_frame % 16) == 0) {
			bullet_template.spread_angle_delta = 12;
			bullet_template.speed.set(3.0f);
			for(i = 0; i < 8; i++) {
				bullets_add_regular();
				bullet_template.spread_angle_delta += 8;
				bullet_template.speed.v += 6;
			}
			snd_se_play(3);
		}
		if(boss.phase_frame == 160) {
			boss.phase_frame = 0;
			boss.mode = 0;
			return true;
		}
		if(
			(boss.phase_state.patterns_seen >= 2) && (boss.phase_frame >= 64)
		) {
			boss_flystep_random(boss.phase_frame % 64);
		}
	}
	return false;
}

// Phase 3, pattern B: two mirrored decelerating stacks every 16 frames, their
// shared base angle walking around by 9 units each time.
bool near mai_1BDD0(void)
{
	if(boss.phase_frame == 32) {
		bullet_template.spawn_type = BST_NO_DECELERATE;
		bullet_template.group = BG_STACK;
		bullet_special.turns_max = 1;
		bullet_template.special_motion = BSM_DECELERATE_THEN_TURN;
		bullet_template.patnum = PAT_BULLET16_V_BLUE;
		bullet_template.stack = (rank + 4);
		bullet_template.stack_speed_delta.set(1.0f);
		bullet_template.speed.set(2.0f);
		boss.sprite = PAT_MAI_ANIMATED;
		boss_statebyte[15] = 0x60;
	} else if(boss.phase_frame > 32) {
		if((boss.phase_frame % 16) == 0) {
			if((boss.phase_frame % 32) == 0) {
				bullet_template.angle = boss_statebyte[15];
				bullet_template_special_angle.turn_by = -0x40;
			} else {
				bullet_template.angle = (0x80 - boss_statebyte[15]);
				bullet_template_special_angle.turn_by = 0x40;
			}
			bullets_add_special_fixedspeed();
			boss_statebyte[15] += 9;
			snd_se_play(3);
		}
		if(boss.phase_frame == 192) {
			boss.phase_frame = 0;
			boss.mode = 0;
			return true;
		}
		if(
			(boss.phase_state.patterns_seen >= 2) && (boss.phase_frame >= 64)
		) {
			boss_flystep_random(boss.phase_frame % 64);
		}
	}
	return false;
}

// Laser sub-pattern A: a 3-bullet spread every 4 frames over the first half of
// the cycle, its angle creeping around by 6 units a shot.
void near mai_1BE96(void)
{
	int frame_mod = ((boss.phase_frame - 32) % 256);

	if(frame_mod == 0) {
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.group = BG_SPREAD;
		bullet_template.special_motion = BSM_DECELERATE_THEN_TURN;
		bullet_template.patnum = PAT_BULLET16_V_BLUE;
		bullet_template.set_spread(3, 12);
		bullet_template.speed.set(3.0f);
		bullet_template.angle = 0x00;
		bullet_template_tune();
	}
	if((frame_mod <= 0x80) && ((frame_mod % 4) == 0)) {
		bullets_add_regular();
		bullet_template.angle += 6;
	}
}

// Laser sub-pattern B: the same, as a 24-bullet ring every 16 frames.
void near mai_1BEF4(void)
{
	int frame_mod = ((boss.phase_frame - 32) % 256);

	if(frame_mod == 0) {
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.group = BG_RING;
		bullet_template.special_motion = BSM_DECELERATE_THEN_TURN;
		bullet_template.patnum = PAT_BULLET16_V_BLUE;
		bullet_template.spread = 24;
		bullet_template.speed.set(3.0f);
		bullet_template.angle = 0x00;
		bullet_template_tune();
	}
	if((frame_mod <= 0x80) && ((frame_mod % 16) == 0)) {
		bullets_add_regular();
		bullet_template.angle++;
	}
}

// Laser sub-pattern C: three ball bullets every 16 frames instead of the
// spread, plus the random-angle pellets the template is left set up for.
void near mai_1BF4D(void)
{
	int frame_mod = ((boss.phase_frame - 32) % 256);

	if(frame_mod == 0) {
		b4ball_template.speed.set(3.0f);
		b4ball_template.angle = 0x80;
		b4ball_template.hp = 24;
		b4ball_template.revenge = true;
		b4ball_template.patnum_tiny_base = PAT_B4BALL_SNOW;
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.group = BG_RANDOM_ANGLE;
		bullet_template.patnum = 0;
		bullet_template.spread = 4;
		bullet_template.speed.set(2.0f);
		bullet_template_tune();
	}
	if((frame_mod <= 0x80) && ((frame_mod % 16) == 0)) {
		b4ball_template.angle += 0x10;
		b4balls_add();
		b4ball_template.angle -= 0x10;
		b4balls_add();
		b4ball_template.angle -= 0x10;
		b4balls_add();
		b4ball_template.angle += 4;
	}
}

// Phase 7, pattern A: mai_1BD2C()'s accelerating spread fan, with three aimed
// ball bullets thrown in every other cycle.
bool near mai_1BFDA(void)
{
	int i;

	if(boss.phase_frame == 32) {
		boss.sprite = PAT_MAI_ANIMATED;
		b4ball_template.speed.set(3.0f);
		b4ball_template.hp = 24;
		b4ball_template.revenge = true;
		b4ball_template.patnum_tiny_base = PAT_B4BALL_SNOW;
	} else if(boss.phase_frame > 32) {
		if((boss.phase_frame % 16) == 0) {
			bullet_template.spawn_type = BST_NO_DECELERATE;
			bullet_template.group = BG_SPREAD;
			bullet_template.patnum = PAT_BULLET16_N_OUTLINED_BALL_BLUE;
			bullet_template.spread = 2;
			bullet_template.angle = 0x40;
			bullet_template.spread_angle_delta = 12;
			bullet_template.speed.set(3.0f);
			for(i = 0; i < 8; i++) {
				bullets_add_regular();
				bullet_template.spread_angle_delta += 8;
				bullet_template.speed.v += 6;
			}
			snd_se_play(3);

			// Left set up for the ball bullets' cloud, which spawn with
			// BG_RANDOM_ANGLE rather than with this pattern's spread.
			bullet_template.group = BG_RANDOM_ANGLE;
			bullet_template.patnum = 0;
			bullet_template.spread = 3;
			bullet_template.speed.set(2.0f);

			if((boss.phase_frame % 32) == 0) {
				b4ball_template.angle = player_angle_from(
					b4ball_template.origin.x, b4ball_template.origin.y, 0x20
				);
				b4balls_add();
				b4ball_template.angle -= 0x20;
				b4balls_add();
				b4ball_template.angle -= 0x20;
				b4balls_add();
			}
		}
		if(boss.phase_frame == 224) {
			boss.phase_frame = 0;
			boss.mode = 0;
			return true;
		}
		if(
			(boss.phase_state.patterns_seen >= 2) && (boss.phase_frame >= 64)
		) {
			boss_flystep_random(boss.phase_frame % 64);
		}
	}
	return false;
}

// Phase 7, pattern B: two mirrored spread stacks every 16 frames, turning in
// alternating directions and opening up by 8 units per pair.
bool near mai_1C0E4(void)
{
	if(boss.phase_frame == 32) {
		bullet_template.spawn_type = BST_NO_DECELERATE;
		bullet_template.group = BG_SPREAD_STACK;
		bullet_special.turns_max = 1;
		bullet_template.special_motion = BSM_DECELERATE_THEN_TURN;
		bullet_template.patnum = PAT_BULLET16_V_BLUE;
		bullet_template.set_spread_stack(2, 8, 2, 0.5f);
		bullet_template.speed.set(3.0f);
		boss.sprite = PAT_MAI_ANIMATED;
		bullet_template_special_angle.turn_by = -0x4A;
	} else if(boss.phase_frame > 32) {
		if((boss.phase_frame % 16) == 0) {
			bullet_template.angle = 0x80;
			bullets_add_special_fixedspeed();
			bullet_template_special_angle.turn_by = (
				-bullet_template_special_angle.turn_by
			);
			bullet_template.angle = 0x00;
			bullets_add_special_fixedspeed();
			bullet_template_special_angle.turn_by = (
				-bullet_template_special_angle.turn_by + 8
			);
			snd_se_play(3);
		}
		if(boss.phase_frame == 192) {
			boss.phase_frame = 0;
			boss.mode = 0;
			return true;
		}
		if(
			(boss.phase_state.patterns_seen >= 2) && (boss.phase_frame >= 64)
		) {
			boss_flystep_random(boss.phase_frame % 64);
		}
	}
	return false;
}

// Phase 9, pattern A: one revenge ball every 4 frames, sweeping through a
// half-circle whose direction reverses each time the pattern runs.
bool near mai_1C194(void)
{
	if(boss.phase_frame == 32) {
		boss.sprite = PAT_MAI_ANIMATED;
		b4ball_template.speed.set(1.75f);
		b4ball_template.hp = 6;
		b4ball_template.revenge = true;
		b4ball_template.angle = 0x80;
		b4ball_template.patnum_tiny_base = PAT_B4BALL_SNOW;
	} else if(boss.phase_frame > 32) {
		if(boss.phase_frame <= 128) {
			bullet_template.spawn_type = BST_NO_DECELERATE;
			bullet_template.group = BG_RANDOM_ANGLE_AND_SPEED;
			bullet_template.patnum = PAT_BULLET16_N_SMALL_BALL_BLUE;
			bullet_template.spread = 8;
			bullet_template.speed.set(1.5f);
			if((boss.phase_frame % 4) == 0) {
				b4ball_template.angle = boss_statebyte[10];
				b4balls_add();
				boss_statebyte[10] += boss_statebyte[11];
				snd_se_play(3);
			}
		} else {
			boss_statebyte[11] = -boss_statebyte[11];
			if(boss_statebyte[11] < 0x7F) {
				boss_statebyte[10] = 0x00;
			} else {
				boss_statebyte[10] = 0x80;
			}
			boss.phase_frame = 0;
			boss.mode = 0;
			return true;
		}
	}
	return false;
}

// Phase 9, pattern B: four fixed lasers at a random fan angle, and mirrored
// bouncing stacks every 8 frames underneath them.
bool near mai_1C23D(void)
{
	int angle_delta;

	if(boss.phase_frame == 16) {
		boss.sprite = PAT_MAI_ANIMATED;
		angle_delta = (randring2_next16_and(0x0F) + 0x10);
		laser_template.active_at_age.grow = 32;
		laser_template.shrink_at_age = 90;

		// Centers the fan of four on straight down, which is 64 in this
		// game's angle units.
		laser_template.coords.angle = (
			64 - angle_delta - (angle_delta / 2)
		);

		laser_fixed_spawn(0);
		laser_template.coords.angle += angle_delta;
		laser_fixed_spawn(1);
		laser_template.coords.angle += angle_delta;
		laser_fixed_spawn(2);
		laser_template.coords.angle += angle_delta;
		laser_fixed_spawn(3);
		boss_statebyte[13] = -8;
	} else if(boss.phase_frame > 32) {
		if(boss.phase_frame <= 128) {
			if((boss.phase_frame % 8) == 0) {
				bullet_template.spawn_type = BST_NO_DECELERATE;
				bullet_template.group = BG_STACK;
				bullet_template.patnum = PAT_BULLET16_V_BLUE;
				bullet_template.set_stack(8, 0.375f);
				bullet_template.speed.set(1.5f);
				bullet_template.special_motion = BSM_BOUNCE_LEFT_RIGHT_TOP;
				bullet_special.turns_max = 1;
				bullet_template.angle = boss_statebyte[13];
				bullets_add_special();
				bullet_template.angle = (0x80 - boss_statebyte[13]);
				bullets_add_special();
				boss_statebyte[13] += 8;
				if(boss_statebyte[13] > 0x30) {
					boss_statebyte[13] = -8;
				}
				snd_se_play(3);
			}

			// Left set up for the pellet cloud, on every frame rather than
			// only on the ones that spawn the stacks.
			bullet_template.spawn_type = BST_NO_DECELERATE;
			bullet_template.group = BG_RANDOM_ANGLE_AND_SPEED;
			bullet_template.patnum = PAT_BULLET16_N_SMALL_BALL_BLUE;
			bullet_template.spread = 6;
			bullet_template.speed.set(1.5f);
		} else {
			boss.phase_frame = 0;
			boss.mode = 0;
			return true;
		}
	}
	return false;
}

// Phase 5: the rotating laser fan. Spawns [mai_laser_count] manually
// controlled fixed lasers on frame 32 and then drives them forever, in
// 256-frame cycles of grow / shrink / reverse, with one of
// MAI_LASER_BULLET_PATTERNS running alongside. Never returns `true`: only
// mai_update()'s own HP check ends this phase.
bool near mai_1C34B(void)
{
	// ZUN reuses one stack word for two unrelated values -- the random base
	// angle of the fan on the spawn frame, and (phase_frame - 32) % 256 on
	// every frame after it. Two locals would be two stack words.
	int frame_mod_or_angle;
	int i;

	if(boss.phase_frame == 32) {
		laser_template.col = 8;
		laser_template.coords.width.nonshrink = 8;
		frame_mod_or_angle = randring2_next16();
		mai_laser_count = (rank + 5);
		for(i = 0; i < mai_laser_count; i++) {
			laser_template.coords.angle = (
				((i << 8) / mai_laser_count) + frame_mod_or_angle
			);
			laser_manual_fixed_spawn(i);
		}
		mai_laser_angle_progress = 0;
		mai_laser_angle_speed = 1;
		mai_laser_bullet_pattern = mai_1BE96;
		boss_statebyte[15] = 1;
		boss_statebyte[10] = 0;
	} else if(boss.phase_frame > 32) {
		frame_mod_or_angle = ((boss.phase_frame - 32) % 256);
		for(i = 0; i < mai_laser_count; i++) {
			lasers[i].coords.origin = boss.pos.cur;
		}
		if(frame_mod_or_angle == 160) {
			for(i = 0; i < mai_laser_count; i++) {
				laser_manual_grow(i);
			}
		} else if(frame_mod_or_angle == 212) {
			for(i = 0; i < mai_laser_count; i++) {
				lasers[i].flag = LF_FIXED_SHRINK_AND_WAIT_TO_GROW;
			}
		} else if(frame_mod_or_angle == 180) {
			boss_statebyte[15] = -boss_statebyte[15];
		}

		if(frame_mod_or_angle <= 112) {
			mai_laser_angle_speed += stage_frame_mod2;
			if(mai_laser_angle_speed > 0x80) {
				mai_laser_angle_speed = 0x80;
			}
		} else if(frame_mod_or_angle <= 192) {
			mai_laser_angle_speed -= 2;
			if(mai_laser_angle_speed < 0) {
				mai_laser_angle_speed = 0;
			}
		} else {
			mai_laser_angle_speed += stage_frame_mod2;

			// [inferred] Skips the rest of the cycle, so the fan can start
			// growing again early. A 1-in-32 chance per frame across the 43
			// frames the window is open.
			if(
				(frame_mod_or_angle > 212) &&
				(randring2_next16_and(0x1F) == 0)
			) {
				boss.phase_frame += (255 - frame_mod_or_angle);
			}
		}

		if(frame_mod_or_angle == 0) {
			mai_laser_bullet_pattern = MAI_LASER_BULLET_PATTERNS[
				(boss_statebyte[10] % 3)
			];
			boss_statebyte[10]++;
		}
		if(boss.phase_frame >= 288) {
			mai_laser_bullet_pattern();
			if((boss.phase_frame >= 544) && (frame_mod_or_angle <= 96)) {
				boss_flystep_random(frame_mod_or_angle - 32);
			}
		}

		mai_laser_angle_progress += mai_laser_angle_speed;
		if(mai_laser_angle_progress >= 0x10) {
			frame_mod_or_angle = (
				(mai_laser_angle_progress / 0x10) * boss_statebyte[15]
			);
			mai_laser_angle_progress %= 0x10;
			for(i = 0; i < mai_laser_count; i++) {
				lasers[i].coords.angle += frame_mod_or_angle;
			}
		}
	}
	return false;
}
/// ----------------

#pragma option -a2

void pascal mai_update(void)
{
	int i;

	homing_target = boss.pos.cur;
	bullet_template.origin = boss.pos.cur;
	gather_template.center = boss.pos.cur;
	laser_template.coords.origin = boss.pos.cur;
	b4ball_template.origin = boss.pos.cur;
	boss.phase_frame++;

	switch(boss.phase) {
	case PHASE_HP_FILL:
		if(boss.phase_frame == 1) {
			boss.hp = MAI_HP_TOTAL;
			boss.phase_end_hp = MAI_HP_PHASE_3_END;
			gather_template.radius.set(BOSS_W / 1.0f);
			gather_template.angle_delta = 0x02;
			gather_template.ring_points = 8;
			boss.sprite = PAT_B4_STILL;
			boss_sprite_left = PAT_B4_LEFT;
			boss_sprite_right = PAT_B4_RIGHT;
			boss_sprite_stay = PAT_B4_STILL;
			b4ball_template.patnum_tiny_base = PAT_B4BALL_SNOW;
		}
		boss_hittest_shots_invincible();

		// Timeout condition
		if(boss.phase_frame >= 64) {
			// Next phase
			boss.phase++;
			boss.phase_frame = 0;
			snd_se_play(13);
			bg_render_bombing_func = mai_yuki_bg_render;
		}
		break;

	case PHASE_BOSS_ENTRANCE_BB:
		boss_hittest_shots_invincible();

		// Timeout condition
		if(boss.phase_frame >= 64) {
			// Next phase

			// [inferred] PAT_YUKI + 8, a cel out of *Yuki's* half of the
			// Stage 4 sprite block, in Mai's update function. yuki_update()
			// sets the same number at the same point, and the phase below
			// replaces it with Mai's own (PAT_MAI + 8) as soon as she has
			// flown into place. Preserved, not fixed (prime directive 8).
			boss.sprite = (PAT_YUKI + 8);

			boss.phase++;
			boss.phase_frame = 0;
			boss_custombullets_render = b4balls_render;
		}
		break;

	case 2:
		boss_hittest_shots_invincible();
		if(boss_flystep_towards(to_sp(PLAYFIELD_W / 2), to_sp(96.0f))) {
			// Next phase
			boss.sprite = (PAT_MAI + 8);
			boss.phase++;
			boss.phase_frame = 0;
			boss.mode = 1;
			boss.phase_state.patterns_seen = 0;
			boss_explode_small(ET_VERTICAL);
			circles_add_growing(boss.pos.cur.x, boss.pos.cur.y);
			mai_yuki_pattern = mai_1BD2C;
			boss_sprite_left = (PAT_MAI + 10);
			boss_sprite_right = (PAT_MAI + 9);
			boss_sprite_stay = (PAT_MAI + 8);
		}
		break;

	case 3:
		switch(boss.mode) {
		case 0:
			if(boss_flystep_random(boss.phase_frame - 32)) {
				boss.phase_frame = 0;
				boss.mode++;
				boss.phase_state.patterns_seen++;

				// Timeout condition
				if(boss.phase_state.patterns_seen >= 36) {
					goto phase_3_timed_out;
				}
				mai_yuki_pattern = MAI_PATTERNS_PHASE_3[
					boss.phase_state.patterns_seen & 1
				];
			}
			break;

		// [measured] a second labelled arm, not a default one: the dispatch
		// compiles to a zero test, a compare against 1, and a fallthrough
		// jump, which is a two-label switch with no default arm.
		case 1:
			mai_yuki_pattern();
			break;
		}
		if(boss_hittest_shots()) {
			boss_score_bonus(10);
phase_3_timed_out:
			// Next phase
			boss_phase_next(ET_NW_SE, MAI_HP_PHASE_5_END);
		}
		break;

	case 4:
		boss_hittest_shots();
		if(boss_flystep_towards(to_sp(PLAYFIELD_W / 2), to_sp(128.0f))) {
			// Next phase
			boss.phase++;
			boss.phase_frame = 0;
		}
		break;

	case 5:
		mai_1C34B();
		if(boss.phase_frame < 5000) {
			if(!boss_hittest_shots()) {
				break;
			}
			boss_score_bonus(10);
		}

		// Next phase
		for(i = 0; i < MAI_LASER_SLOTS; i++) {
			laser_stop(i);
		}
		boss_phase_next(ET_HORIZONTAL, MAI_HP_PHASE_7_END);
		break;

	case 6:
		boss_hittest_shots();
		if(boss_flystep_towards(to_sp(PLAYFIELD_W / 2), to_sp(96.0f))) {
			goto phase_6_8_next;
		}
		break;

	case 7:
		switch(boss.mode) {
		case 0:
			if(boss_flystep_random(boss.phase_frame - 32)) {
				boss.phase_frame = 0;
				boss.mode++;
				boss.phase_state.patterns_seen++;

				// Timeout condition
				if(boss.phase_state.patterns_seen >= 36) {
					goto phase_7_timed_out;
				}
				mai_yuki_pattern = MAI_PATTERNS_PHASE_7[
					boss.phase_state.patterns_seen & 1
				];
			}
			break;

		case 1:
			mai_yuki_pattern();
			break;
		}
		if(boss_hittest_shots()) {
			boss_score_bonus(10);
phase_7_timed_out:
			// Next phase
			boss_phase_next(ET_NW_SE, MAI_HP_PHASE_9_END);
		}
		break;

	case 8:
		boss_hittest_shots();
		if(boss_flystep_towards(to_sp(PLAYFIELD_W / 2), to_sp(96.0f))) {
phase_6_8_next:
			// Next phase
			boss.phase++;
			boss.phase_frame = 0;

			// [inferred] Read by the phase-9 patterns and by mai_1C34B(),
			// which are still assembly; left as raw indices for the reason
			// th05/main/boss/b5.cpp gives for Yumeko's, i.e. that naming a
			// shared slot from one of its several users is how a name gets
			// contradicted by the next lift.
			boss_statebyte[10] = 0x80;
			boss_statebyte[11] = -4;
		}
		break;

	case 9:
		switch(boss.mode) {
		case 0:
			if(boss_flystep_random(boss.phase_frame - 4)) {
				boss.phase_frame = 0;
				boss.mode++;
				boss.phase_state.patterns_seen++;

				// Timeout condition
				if(boss.phase_state.patterns_seen >= 20) {
					boss.phase_state.defeat_bonus = false;
					goto phase_9_defeated;
				}
				mai_yuki_pattern = MAI_PATTERNS_PHASE_9[
					boss.phase_state.patterns_seen & 1
				];
			}
			break;

		case 1:
			mai_yuki_pattern();
			break;
		}
		if(boss_hittest_shots()) {
			boss.phase_state.defeat_bonus = true;
phase_9_defeated:
			// Next phase
			boss_explode_small(ET_VERTICAL);
			boss.phase_frame = 0;
			boss.phase = PHASE_BOSS_EXPLODE_SMALL;
			b4balls_reset();
			boss_custombullets_render = nullfunc_near;
		}
		break;

	default:
		boss_defeat_update(70);
		return;
	}

	b4balls_update();
	hud_hp_update_and_render(boss.hp, MAI_HP_TOTAL);
}

#pragma option -a1
