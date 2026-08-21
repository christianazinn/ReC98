/// Stage 4 Boss - Marisa: the fight's own update function
/// ------------------------------------------------------
/// (#included from th04/std_run.cpp, ahead of enemies_add() and std_run(),
/// which is this function's original address order at the head of
/// ENM_BTPL_TEXT's C++ object. That ZUN's object for this segment held
/// Marisa's fight, the enemy spawner and the stage script VM is
/// kb/codegen/0112.)
///
/// Marisa's patterns and her bits live in th04/main/boss/b4m.cpp, which is a
/// different segment (B4M_UPDATE_TEXT) and therefore a different object; only
/// the two functions that segment's own tail lift reached are C++ so far.
///
/// This file deliberately includes NO unguarded header that
/// th04/main/enemy/add.cpp or th04/formats/std_run.cpp includes after it --
/// th04/main/frames.h and th04/math/randring.hpp in particular
/// (kb/codegen/0129). The three declarations that would have come from those
/// are repeated below instead.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th02/main/player/player.hpp"
#include "th03/hardware/palette.hpp"
#include "th04/snd/snd.h"
#include "th04/sprites/main_pat.h"
#include "th04/main/bg.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/tile/bb.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/boss/boss.hpp"

/// From headers this object must not expand twice
/// ----------------------------------------------
// th04/main/frames.h, expanded by th04/formats/std_run.cpp below.
extern unsigned char stage_frame_mod4;

// th03/math/randring.hpp, reached by th04/main/enemy/add.cpp below.
uint16_t pascal near randring2_next16_and(uint16_t mask);
uint16_t pascal near randring2_next16_mod(uint16_t mask);
/// ----------------------------------------------

/// Still ASM
/// ---------
// Every one of Marisa's patterns, and the two per-frame helpers around them.
// All of these are in this same segment and all are private to ZUN's object,
// so each needed a zero-byte `label` alias in th04_main.asm to become linkable
// (kb/codegen/0123). The address-suffixed names are the dump's own: naming
// them is a separate decision from making them linkable, and it belongs to
// whoever lifts them.
extern "C" {
	// Everything the fight does on every frame regardless of pattern.
	void near marisa_16C6A(void);

	// The eleven [boss.mode] patterns, in the order the dense value table
	// lists them. 10 and 11 are the two that only ever run while no bit is
	// alive; 255 is not a pattern but the interval between two of them.
	void near marisa_16E9D(void);  // 0
	void near marisa_16F61(void);  // 1
	void near marisa_17079(void);  // 2
	void near marisa_1717D(void);  // 3
	void near marisa_17335(void);  // 4
	void near marisa_17491(void);  // 5
	void near marisa_1769E(void);  // 6
	void near marisa_1788E(void);  // 7
	void near marisa_16DFF(void);  // 10
	void near marisa_17813(void);  // 11

	// Damages Marisa by this frame's shot hits, divided by the number of bits
	// still alive plus one, and returns `true` once that took her below
	// [boss.phase_end_hp].
	bool near marisa_179BC(void);
}

// Already C++, in th04/main/boss/b4m.cpp -- B4M_UPDATE_TEXT, which is in this
// same main_03 group, so the call stays near. No TH04 header declares it yet.
void near marisa_flystep_random(void);

// Marisa's fight state, all of it th04_main.asm `.data?` bytes with no
// `public` of ZUN's, and all of it reached from nowhere but this segment.
// Five of the six are written and read entirely inside marisa_update(), so
// their names are `[inferred]` from this function alone and a naming round is
// owed for all six.
//
// [marisa_25671] is the exception and keeps the dump's address-suffixed
// spelling: this function only ever writes it 2, and its two readers
// (`marisa_16C05` and `marisa_16E9D`) are still ASM, so nothing here measures
// what it means.
extern "C" {
	// The pattern [boss.mode] last ran, so that rolling a new one can reject
	// a repeat.
	extern unsigned char marisa_pattern_prev;

	// [bits_alive] at the moment the current pattern was chosen. Zero here
	// AND zero now is what makes an interval count as bit-less.
	extern unsigned char marisa_bits_at_pattern_start;

	// Direction of the background's blue pulse: `false` while color 0 climbs
	// to 192, `true` while it falls back to 38.
	extern bool marisa_pulse_dimming;

	extern unsigned char marisa_25671;

	// Bit-less intervals since the last time the bits were respawned. The
	// second one forces pattern 0, which is the one that spawns them.
	extern unsigned char marisa_patterns_without_bits;

	// How many of the three HP milestones (4500, 2500, 1000) have already
	// paid out. Doubles as the explosion type each one shows.
	extern unsigned char marisa_explode_milestone;
}

// th04/main/boss/b4m.cpp's [bits_alive], which th04_main.asm does publish.
extern uint8_t bits_alive;

// b4m.cpp spells this one the same way, for the same slot.
#define flystep_pointreflected_tick boss_statebyte[13]

// Declared FAR here, and only here. th04/main/boss/bosses.hpp declares it
// `near`, which is what it is -- but a near reference under this object's
// `-zPmain_03` frames its offset on main_03, and this function is in main_01.
// kb/codegen/0162.
void pascal far reimu_marisa_bg_render(void);
/// ---------

/// Constants
/// ---------
// Marisa's cels are absolute patnums, like Yuuka's: marisa_fg_render() blits
// [boss.sprite] as it stands, with no per-boss base added.
static const int PAT_MARISA_FIGHT = (PAT_STAGE + 1);

static const int MARISA_HP = 6000;

// The fight ends on its own after this many intervals, and that timeout is the
// one way to reach the defeat phase without the bonus.
static const int MARISA_INTERVALS_MAX = 52;

// Frames an interval lasts before the next pattern is rolled.
static const int MARISA_INTERVAL_FRAMES = 64;

// [boss.mode] values that are not patterns.
static const unsigned char MODE_PATTERN_0 = 0;
static const unsigned char MODE_INTERVAL = 255;

// The two patterns that run while no bit is alive, picked between at random.
static const unsigned char MODE_BITLESS_FIRST = 10;
/// ---------

void pascal far marisa_update(void)
{
	int pattern;

	bullet_template.origin.x.v = (boss.pos.cur.x.v - TO_SP(20));
	bullet_template.origin.y.v = (boss.pos.cur.y.v - TO_SP(8));

	switch(boss.phase) {
	case 0:
		if(boss.phase_frame == 0) {
			boss.hp = MARISA_HP;
			marisa_25671 = 2;
		}
		boss_hittest_shots_invincible();
		if(boss.phase_frame > 96) {
			boss.phase++;
			Palettes[0].c.r = 0;
			Palettes[0].c.g = 0;
			Palettes[0].c.b = 7;
			palette_changed = true;
			boss.phase_frame = 0;
			snd_se_play(13);
			_asm mov word ptr bg_render_bombing_func, offset reimu_marisa_bg_render
			tiles_bb_col = V_WHITE;
			marisa_pulse_dimming = false;
		}
		break;

	case 1:
		boss.phase_frame++;
		boss_hittest_shots_invincible();
		if(boss.phase_frame >= 128) {
			boss.phase++;
			boss.pos.velocity.x.v = 0;
			boss.phase_state.patterns_seen = 0;
			boss.mode = MODE_BITLESS_FIRST;
			boss.phase_frame = 0;
			boss.sprite = PAT_MARISA_FIGHT;
			marisa_patterns_without_bits = 1;
			marisa_explode_milestone = 0;
			marisa_pattern_prev = MODE_BITLESS_FIRST;
			marisa_bits_at_pattern_start = 0;
			flystep_pointreflected_tick = 0;
		}
		break;

	case 2:
		switch(boss.mode) {
		case 0:
			marisa_16E9D();
			break;
		case 1:
			marisa_16F61();
			break;
		case 2:
			marisa_17079();
			break;
		case 3:
			marisa_1717D();
			break;
		case 4:
			marisa_17335();
			break;
		case 5:
			marisa_17491();
			break;
		case 6:
			marisa_1769E();
			break;
		case 7:
			marisa_1788E();
			break;
		case 10:
			marisa_16DFF();
			break;
		case 11:
			marisa_17813();
			break;
		case MODE_INTERVAL:
			marisa_flystep_random();
			if(boss.phase_frame >= MARISA_INTERVAL_FRAMES) {
				boss.phase_state.patterns_seen++;
				flystep_pointreflected_tick = 0;
				if((marisa_bits_at_pattern_start == 0) && (bits_alive == 0)) {
					marisa_patterns_without_bits++;
					if(marisa_patterns_without_bits >= 2) {
						boss.mode = MODE_PATTERN_0;
						marisa_patterns_without_bits = 0;
					} else {
						boss.mode = (
							randring2_next16_and(1) + MODE_BITLESS_FIRST
						);
					}
				} else {
					// Rerolled until it differs, which is why the eighth
					// pattern is only ever reachable through the bit-less
					// branch above.
					do {
						pattern = (randring2_next16_mod(7) + 1);
					} while(marisa_pattern_prev == pattern);
					boss.mode = pattern;
					marisa_pattern_prev = pattern;
					marisa_bits_at_pattern_start = bits_alive;
				}
				boss.phase_frame = 0;
				if(boss.phase_state.patterns_seen >= MARISA_INTERVALS_MAX) {
					boss.phase_state.defeat_bonus = false;
					goto phase_over;
				}
			}
			break;
		}
		if(marisa_179BC()) {
			boss.phase_state.defeat_bonus = true;
phase_over:
			boss_explode_small(ET_HORIZONTAL);
			boss.phase++;
			boss.phase_frame = 0;
		}

		if(stage_frame_mod4 == 0) {
			// kb/codegen/0032, and NOT `Palettes[0].c.b += 2`: a compound
			// assignment through Palette::operator []() materialises the far
			// reference it returns into a `LES BX` pair, which costs this
			// function 8 bytes of frame and 0x24 bytes of code. The original
			// is a byte load, a byte add and a byte store.
			if(!marisa_pulse_dimming) {
				_AL = Palettes[0].c.b;
				_AL += 2;
				Palettes[0].c.b = _AL;
				if(Palettes[0].c.b >= 192) {
					marisa_pulse_dimming = true;
				}
			} else {
				_AL = Palettes[0].c.b;
				_AL += -2;
				Palettes[0].c.b = _AL;
				if(Palettes[0].c.b <= 38) {
					marisa_pulse_dimming = false;
				}
			}
			palette_changed = true;
		}

		if(
			((boss.hp <= 4500) && (marisa_explode_milestone == 0)) ||
			((boss.hp <= 2500) && (marisa_explode_milestone == 1)) ||
			((boss.hp <= 1000) && (marisa_explode_milestone == 2))
		) {
			boss_items_drop();
			bullets_clear();
			boss_score_bonus(10);
			boss_explode_small(
				static_cast<explosion_type_t>(marisa_explode_milestone)
			);
			marisa_explode_milestone++;
		}
		break;

	case 3:
		boss.phase_frame++;
		if(boss.phase_frame == 16) {
			boss_explode_small(ET_VERTICAL);
		}
		if(boss.phase_frame == 32) {
			boss_defeat_explode_big(ET_SW_NE, 40);
			snd_se_play(12);

			// Only two of the three components, unlike every other TH04 boss.
			Palettes[0].c.r = 0;
			Palettes[0].c.b = 0;
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
	marisa_16C6A();
	hud_hp_update_and_render(boss.hp, MARISA_HP);
}
/// ------------------------------------------------------
