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
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/main/player/player.hpp"
#include "th03/hardware/palette.hpp"
#include "th04/snd/snd.h"
#include "th04/sprites/main_pat.h"
#include "th04/main/bg.hpp"
#include "th04/main/frames.h"
#include "th04/main/homing.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/spark.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/tile/bb.hpp"
#include "th04/main/boss/boss.hpp"

/// Still ASM
/// ---------
// Elly's patterns and her one per-frame helper, all of them in this same
// segment and all private to ZUN's object, so each needed a zero-byte `label`
// alias in th04_main.asm to become linkable (kb/codegen/0123). The
// address-suffixed names are the dump's own; naming them belongs to whoever
// lifts them.
extern "C" {
	// Runs on every frame of the fight, before the phase dispatch.
	void near elly_1B95C(void);

	// The scythe spin between two patterns.
	void near elly_1BC73(void);

	// Phase 1's only pattern.
	void near elly_1BD23(void);

	// The nine [boss.mode] patterns of the fight proper, in the order the
	// sparse value table lists them.
	void near elly_1BD4B(void);  // 0
	void near elly_1BE78(void);  // 1
	void near elly_1BF52(void);  // 2
	void near elly_1BFAB(void);  // 3
	void near elly_1C044(void);  // 4
	void near elly_1C0BF(void);  // 5
	void near elly_1C164(void);  // 6
	void near elly_1C1CF(void);  // 7
	void near elly_1C251(void);  // 8
}

// Elly's fight state, all of it th04_main.asm `.data?` with no `public` of
// ZUN's. **A naming round is owed for all four.**
extern "C" {
	// Which of the five attack sets is running, 0…4. It indexes the dense
	// table that decides how [boss.mode] cycles, doubles as the explosion
	// type each set ends with, and sets the HP each set starts from.
	// `[inferred]`, and the only one of the four whose every read and write
	// is inside this function.
	extern unsigned char elly_pattern_set;

	// The other three keep the dump's address-suffixed spellings: this
	// function only zeroes or bumps them, and everything that reads them is
	// still ASM — `elly_1B95C` and the patterns for the first two, and a
	// caller in another segment entirely for `elly_25A27`.
	extern unsigned char elly_25A26;
	extern unsigned char elly_25A27;
	extern int elly_25A3A;
}

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
/// ---------

void pascal far elly_update(void)
{
	elly_1B95C();

	switch(boss.phase) {
	case 0:
		elly_25A27 = 0;
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
			elly_25A27 = 0;
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
