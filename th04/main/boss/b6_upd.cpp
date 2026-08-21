/// Extra Stage Boss - Yuuka: the fight's own update function
/// ---------------------------------------------------------
/// (#included from th04/main_034.cpp, AHEAD of th04/main/boss/b3_upd.cpp.
/// Code is emitted in source order within an object, and this function sits
/// below Elly's whole fight in MAIN_034_TEXT, so this #include order *is* the
/// original address order — kb/codegen/0112 + 0114.)
///
/// yuuka6_bg_render() is th04/main/boss/bg.cpp and yuuka6_fg_render() is
/// main_012_TEXT; both are other objects. So is th04/main/boss/b6.cpp, which
/// is a declarations-only file that bg.cpp expands — deliberately NOT included
/// here, because it declares yuuka6_anim_vanish() and yuuka6_phase_next() with
/// C++ linkage, and this translation unit has to CALL both, which needs the
/// `extern "C"` spellings the dump actually publishes.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th03/hardware/palette.hpp"
#include "th02/main/player/player.hpp"
#include "th02/v_colors.hpp"
#include "th04/snd/snd.h"
#include "th04/formats/std.hpp"
#include "th04/math/randring.hpp"
#include "th04/sprites/main_pat.h"
#include "th04/main/frames.h"
#include "th04/main/bg.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/null.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/tile/bb.hpp"
#include "th04/main/boss/boss.hpp"

/// Constants
/// ---------
static const int YUUKA6_HP = 13300;

// The number of different paths Yuuka can take across the playfield during
// Phase 2. th04/main/boss/b6.cpp's PHASE2_FLY_PATHS, restated rather than
// included for the reason at the top of this file.
static const int PHASE2_FLY_PATHS = 2;

// Where yuuka6_1A439() is asked to fly to, in every phase that asks.
static const int FLY_TARGET_Y = 80;
/// ---------

/// Still ASM
/// ---------
/// Everything below sat above this function in ZUN's object and is `static`
/// there, so each one needed publishing to become linkable (kb/codegen/0123).
/// The seventeen `yuuka6_1?????` names are the dump's own address-suffixed
/// ones and **a naming round is owed for all of them**; every one of them is
/// reached from the `switch(boss.mode)` chains below and from nowhere else.
///
/// Two spellings, and the difference is the calling convention the dump's
/// `public` records: a `pascal` function publishes as the bare UPPERCASE name,
/// so `yuuka6_1A439` needed only a `public` line, while the `extern "C"` near
/// ones publish as `_name` and each needed a zero-byte `label near` alias.
extern "C" {
	// Runs on every frame of the fight, after the phase dispatch: Yuuka's own
	// sprite animation and everything she owns that moves.
	void near yuuka6_1A110(void);

	// Flies Yuuka to the given point, and returns `true` once she is there.
	// The only one of the seventeen that takes arguments.
	bool pascal near yuuka6_1A439(subpixel_t x, subpixel_t y);

	void near yuuka6_1A4A8(void);

	// Returns `true` once whatever it is running has finished.
	bool near yuuka6_1A503(void);

	// Phase 2's three patterns.
	void near yuuka6_1AB5D(void);
	void near yuuka6_1ABE5(void);
	void near yuuka6_1ACCC(void);

	// Phase 4's one.
	void near yuuka6_1AD6F(void);

	void near yuuka6_1ADDB(void);

	// The three patterns of phases 8 and 12…
	void near yuuka6_1AE8F(void);
	void near yuuka6_1AFA8(void);
	void near yuuka6_1B313(void);

	// …and the three of phase 14.
	void near yuuka6_1B099(void);
	void near yuuka6_1B1B1(void);
	void near yuuka6_1B22B(void);

	void near yuuka6_1B282(void);

	// th04/main/boss/b6_next.cpp. Really returns whether the hit it
	// processes took Yuuka's HP below zero; the call site below discards it.
	bool near yuuka6_1B3E2(void);

	// th04/main/boss/b4m.cpp, reached from this segment by the `procdesc`
	// th04_main.asm still carries for it.
	void near thicklasers_update_and_hittest(void);

	// th04/main/boss/b6_anim.asm.
	bool near yuuka6_anim_vanish(void);

	// th04/b6_next.cpp, its own object at the head of this segment's C++
	// half. `pascal`, and therefore published as the bare uppercase name.
	void pascal near yuuka6_phase_next(
		explosion_type_t explosion_type, int next_end_hp
	);

	// th04/main/boss/b4m.cpp.
	extern int midboss_frames_until;

	/// Yuuka's Extra fight state, all of it th04_main.asm `.data?`.
	extern int yuuka6_anim_frame;
	extern unsigned char yuuka6_sprite_flag;
	extern unsigned char yuuka6_phase2_fly_path;

	// Three more with no `public` of ZUN's, and no reader outside this
	// function and the seventeen above. **A naming round is owed** for all
	// three; they keep the dump's address-suffixed spellings.
	extern unsigned char yuuka6_25A02;
	extern unsigned char yuuka6_25A08;
	extern unsigned char yuuka6_25A1B;
}

// The one that is not `extern "C"`: the dump spells it with a lower-case C++
// mangled name, which is what a non-`pascal` C++ function publishes as.
bool near yuuka6_phase2_fly(void);

// th04/main/boss/b6.cpp's yuuka6_sprite_flag_t, restated as the two
// enumerators this function needs rather than included — see the top.
static const int Y6SF_VANISHED = 0;
static const int Y6SF_PARASOL_BACK_OPEN = 1;
static const int Y6SF_PARASOL_BACK_CLOSED = 2;

// Declared FAR, for the reason th04/main/boss/b3_upd.cpp gives for
// elly_bg_render() in this same object: a near reference under
// `-zPmain_03` frames its offset on main_03, and yuuka6_bg_render() lives
// in main_01. Without this, TLINK reports a fixup overflow against it in
// this segment. th04/main/boss/bosses.hpp declares it `near`, which
// is what it is, and is deliberately not included. kb/codegen/0162.
void pascal far yuuka6_bg_render(void);
/// ---------

void pascal far yuuka6_update(void)
{
	switch(boss.phase) {
	// The entrance. Yuuka's fight ends the stage script, and pushes the
	// midboss that will never come out of reach for good.
	case 0:
		if(boss.phase_frame == 0) {
			stage_vm = nullfunc_far;
			midboss_frames_until = 0;
			yuuka6_25A08 = 0;
			yuuka6_25A1B = 0;
		}
		boss_hittest_shots_invincible();
		if(boss.phase_frame > 128) {
			boss.phase++;
			boss.phase_frame = 0;
			snd_se_play(13);
			yuuka6_25A02 = 0;
			_asm { mov word ptr bg_render_bombing_func, offset yuuka6_bg_render }
			tiles_bb_col = V_WHITE;
		}
		break;

	case 1:
		boss_hittest_shots_invincible();
		if(boss.phase_frame >= 64) {
			boss.phase++;
			boss.pos.velocity.x.v = 0;
			boss.phase_state.patterns_seen = 0;
			boss.mode = 0;
			boss.hp = YUUKA6_HP;
			boss.phase_end_hp = 10600;
			boss.phase_frame = 0;
			yuuka6_anim_frame = 0;
			yuuka6_sprite_flag = Y6SF_PARASOL_BACK_OPEN;
			yuuka6_phase2_fly_path = randring2_next16_and(PHASE2_FLY_PATHS - 1);
		}
		break;

	// Phase 2: three patterns, with a flight across the playfield between
	// each two of them, and ten of those flights end the phase.
	case 2:
		switch(boss.mode) {
		case 0:
			yuuka6_1AB5D();
			break;
		case 1:
			yuuka6_1ABE5();
			break;
		case 2:
			yuuka6_1ACCC();
			break;
		case 255:
			if(yuuka6_phase2_fly()) {
				boss.mode = (boss.phase_state.patterns_seen % 3);
				if(boss.phase_state.patterns_seen >= 10) {
					goto phase_2_over;
				}
			}
			break;
		}
		if(boss.sprite == 0) {
			goto frame_only;
		}
		if(!boss_hittest_shots()) {
			break;
		}
		boss_score_bonus(20);
		boss_items_drop();
phase_2_over:
		yuuka6_phase_next(ET_CIRCLE, 7600);
		break;

	case 3:
		boss.phase_frame++;
		if(yuuka6_1A439(TO_SP(PLAYFIELD_W / 2), TO_SP(FLY_TARGET_Y))) {
			boss.phase++;
			boss.phase_frame = 0;
			boss.phase_state.patterns_seen = 0;
		}
		break;

	// Phase 4: one pattern, and a flight to a random column between two runs
	// of it.
	case 4:
		switch(boss.mode) {
		case 0:
			yuuka6_1AD6F();
			break;
		case 255:
			if(yuuka6_1A439(
				(randring2_next16_mod(TO_SP(288)) + TO_SP(48)),
				TO_SP(FLY_TARGET_Y)
			)) {
				boss.mode = 0;
				if(boss.phase_state.patterns_seen >= 10) {
					goto phase_4_over;
				}
			}
			break;
		}
		if(boss.sprite == 0) {
			goto frame_only;
		}
		if(boss.mode == 255) {
			goto frame_only;
		}
		if(!boss_hittest_shots()) {
			break;
		}
		boss_score_bonus(20);
		boss_items_drop();
phase_4_over:
		yuuka6_phase_next(ET_CIRCLE, 5400);
		yuuka6_25A08 = 1;
		break;

	// The vanish that opens both of Yuuka's two invisible halves.
	case 5:
	case 9:
		boss.phase_frame++;
		if(yuuka6_sprite_flag != Y6SF_VANISHED) {
			yuuka6_anim_vanish();
			break;
		}
		boss.phase++;
		boss.phase_frame = 0;
		break;

	case 6:
	case 10:
		yuuka6_1ADDB();
		boss.phase_frame++;
		yuuka6_1A4A8();
		if(boss.phase_frame < 320) {
			break;
		}
		if(boss.pos.cur.y.v != TO_SP(FLY_TARGET_Y)) {
			break;
		}
		boss_explode_small(ET_SW_NE);
		if(bullet_clear_time < 20) {
			bullet_clear_time = 20;
		}
		boss.phase++;
		boss.phase_frame = 0;
		yuuka6_anim_frame = 0;
		break;

	case 7:
	case 11:
		boss_hittest_shots();
		if(!yuuka6_1A503()) {
			break;
		}
		boss.phase++;
		boss.phase_state.patterns_seen = 0;
		boss.mode = 255;
		boss.phase_frame = 0;
		yuuka6_anim_frame = 0;
		yuuka6_sprite_flag = Y6SF_PARASOL_BACK_CLOSED;
		yuuka6_25A1B = 1;
		yuuka6_25A02 = 255;
		break;

	// Both of the invisible halves' pattern phases: three patterns, picked at
	// random but never twice in a row, with a flight between each two.
	case 8:
	case 12:
		switch(boss.mode) {
		case 0:
			yuuka6_1AE8F();
			break;
		case 1:
			yuuka6_1AFA8();
			break;
		case 2:
			yuuka6_1B313();
			break;
		case 255:
			if(yuuka6_1A439(
				(randring2_next16_mod(TO_SP(144)) + TO_SP(48)),
				TO_SP(FLY_TARGET_Y)
			)) {
				do {
					boss.mode = randring2_next16_mod(3);
				} while(yuuka6_25A02 == boss.mode);
				yuuka6_25A02 = boss.mode;
				if(boss.phase_state.patterns_seen >= 10) {
					goto phase_8_over;
				}
			}
			break;
		}
		if(boss.sprite == 0) {
			goto frame_only;
		}
		if(boss.mode <= 2) {
			boss_hittest_shots();
			yuuka6_1B3E2();
		} else {
			boss.phase_frame++;
		}
		if(boss.hp > boss.phase_end_hp) {
			break;
		}
		boss_score_bonus(20);
		boss_items_drop();
phase_8_over:
		// TWO calls, not one with a conditional argument: `pascal` packs two
		// constant arguments into a single `pushd`, and a ternary picks the
		// second one in AX instead, which is a different instruction stream.
		// Turbo C++ then merges the two `call`s itself.
		if(boss.phase == 8) {
			yuuka6_phase_next(ET_CIRCLE, 3400);
		} else {
			yuuka6_phase_next(ET_CIRCLE, 1200);
		}
		if(boss.phase == 9) {
			yuuka6_25A08 = 1;
		}
		yuuka6_25A1B = 0;
		break;

		// Reached from phases 2 and 4 as well: the frame counter is the only
		// thing that advances while Yuuka has no sprite.
frame_only:
		boss.phase_frame++;
		break;

	case 13:
		boss_hittest_shots_invincible();
		if(!yuuka6_1A503()) {
			break;
		}
		boss.phase++;
		boss.phase_state.patterns_seen = 0;
		boss.mode = 0;
		boss.phase_frame = 0;
		yuuka6_anim_frame = 0;
		yuuka6_sprite_flag = Y6SF_PARASOL_BACK_CLOSED;
		yuuka6_25A1B = 1;
		break;

	// The last pattern phase, and the only one whose flight walks the pattern
	// index rather than picking it.
	case 14:
		switch(boss.mode) {
		case 0:
			yuuka6_1B099();
			break;
		case 1:
			yuuka6_1B1B1();
			break;
		case 2:
			yuuka6_1B22B();
			break;
		case 255:
			boss.phase_state.patterns_seen++;
			boss.mode = (boss.phase_state.patterns_seen % 3);
			if(boss.phase_state.patterns_seen >= 18) {
				goto phase_14_over;
			}
			break;
		}
		if(!boss_hittest_shots()) {
			break;
		}
		boss_score_bonus(20);
		boss_items_drop();
phase_14_over:
		yuuka6_phase_next(ET_HORIZONTAL, 0);
		boss.sprite = 146;
		break;

	case 15:
		boss_hittest_shots();
		if(boss.phase_frame >= 128) {
			boss.phase++;
			bullet_template.spawn_type = BST_PELLET;
			bullet_template.angle = 0;
		}
		break;

	// The last stand: 2500 frames, or one more hit.
	case 16:
		yuuka6_1B282();
		if(!boss_hittest_shots()) {
			if(boss.phase_frame < 2500) {
				break;
			}
		}
		boss_explode_small(ET_NW_SE);
		boss.phase++;
		if(boss.phase_frame < 2500) {
			boss.phase_state.defeat_bonus = true;
		} else {
			boss.phase_state.defeat_bonus = false;
		}
		boss.phase_frame = 0;
		boss.mode = 0;
		PaletteTone = 100;
		palette_changed = true;
		break;

	case 17:
		boss.phase_frame++;
		if(boss.phase_frame == 16) {
			boss_explode_small(ET_VERTICAL);
		}
		if(boss.phase_frame == 32) {
			boss_defeat_explode_big(ET_SW_NE, 70);
			snd_se_play(12);
			palette_changed = true;
			player_invincibility_time = BOSS_DEFEAT_INVINCIBILITY_FRAMES;
		}
		break;

	default:
		boss_defeat_update();
		return;
	}

	homing_target.x.v = boss.pos.cur.x.v;
	homing_target.y.v = boss.pos.cur.y.v;
	thicklasers_update_and_hittest();
	yuuka6_1A110();
	hud_hp_update_and_render(boss.hp, YUUKA6_HP);
}
