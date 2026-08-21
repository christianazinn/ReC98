/// Extra Boss 2 - Gengetsu: the fight's own update function
/// --------------------------------------------------------
/// (#included from th04/main_036.cpp. main_036_TEXT has no other C++
/// contribution, and TLINK lays a segment's contributions out in link order
/// with the root dump first, so this object lands at the segment's tail by
/// construction — which is where this function already was.
/// kb/codegen/0112 + 0114.)
///
/// gengetsu_fg_render() and the background both live elsewhere;
/// th04/main/boss/bx2.hpp holds what this fight shares with them.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th02/main/player/bomb.hpp"
#include "th02/main/player/player.hpp"
#include "th03/hardware/palette.hpp"
#include "th04/snd/snd.h"
#include "th04/sprites/main_pat.h"
#include "th04/math/randring.hpp"
#include "th04/main/bg.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/player/player.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/bullet/laser_t.hpp"
#include "th04/main/tile/bb.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/bx2.hpp"

/// Still ASM
/// ---------
// Gengetsu's patterns and her three per-frame helpers, all of them in this
// same segment and all private to ZUN's object, so each needed a zero-byte
// `label` alias in th04_main.asm to become linkable (kb/codegen/0123). The
// address-suffixed names are the dump's own; naming them belongs to whoever
// lifts them.
extern "C" {
	// Runs the teleport, and returns `true` on the frame it lands.
	bool near gengetsu_1F97A(void);

	// Phase 6's one-shot, returning `true` once it is over.
	bool near gengetsu_1F9C5(void);

	// The four pattern PAIRS, one per [boss.mode] 0/1 in phases 2 through 5.
	void near gengetsu_1FAAA(void);
	void near gengetsu_1FAF7(void);
	void near gengetsu_1FB86(void);
	void near gengetsu_1FC46(void);
	void near gengetsu_1FD30(void);
	void near gengetsu_1FDFE(void);
	void near gengetsu_1FE6A(void);
	void near gengetsu_1FEDF(void);

	// Phases 7 and 8's patterns.
	void near gengetsu_20050(void);
	void near gengetsu_200B6(void);
	void near gengetsu_20195(void);

	// The hittest, which is also the sprite animation, and returns `true`
	// once the phase's HP threshold is reached.
	bool near gengetsu_20202(void);

	// The between-patterns filler the last five phases add once
	// [boss.phase_state] passes 18.
	void near gengetsu_2023B(void);
}

// Shared between Mugetsu and Gengetsu, and th04_main.asm publishes neither it
// nor a name for it. `[inferred]`, and **a naming round is owed**: a bomb sets
// it to 32 and every frame decrements it towards 0, and the only thing that
// reads it is the two Extra bosses' hittest helpers, which take the
// invincibility path while it is nonzero.
extern "C" unsigned char extra_boss_bomb_immunity;

// Declared FAR here, and only here: th04/main/boss/bosses.hpp declares the
// same function `near`, which is what it is, and that header is deliberately
// not included. A near reference under this object's `-zPmain_03` frames its
// offset on main_03, and mugetsu_gengetsu_bg_render() lives in main_01.
// kb/codegen/0162.
void pascal far mugetsu_gengetsu_bg_render(void);
/// ---------

/// Constants
/// ---------
static const int GENGETSU_HP = 18700;

// th04/main/boss/bx2.cpp's, repeated because that file is a translation unit
// rather than a header.
static const pixel_t WAVE_TARGET_MARGIN = (PLAYFIELD_W / 12);

// Frames of bomb immunity. The value is a bomb's length, not a constant this
// fight owns.
static const int BOMB_IMMUNITY_FRAMES = 32;

// [boss.phase_state] counts teleports, not patterns: past this many, every
// remaining phase adds gengetsu_2023B() on top of whatever else it runs…
static const int TELEPORTS_UNTIL_FILLER = 18;

// …and past THIS many the phase can no longer be ended by damage at all.
static const int TELEPORTS_UNTIL_UNKILLABLE = 22;
/// ---------

// The one thing the middle four phases do not share: phase 2 advances
// [boss.phase_state] BEFORE deriving [boss.mode] from it, and the other three
// do it after. Both spellings are one `INC` and one `AND`, in the two possible
// orders, so this is ZUN's and not a compiler artifact.
#define GENGETSU_TELEPORT_FIRST { \
	boss.phase_state.patterns_seen++; \
	boss.mode = (boss.phase_state.patterns_seen & 1); \
}
#define GENGETSU_TELEPORT_LATER { \
	boss.mode = (boss.phase_state.patterns_seen & 1); \
	boss.phase_state.patterns_seen++; \
}

// Phases 2 through 5, which differ only in their two patterns, in that one
// ordering above, and in what the phase ends into.
#define gengetsu_pattern_phase( \
	pattern_0, pattern_1, on_teleport_done, explosion, next_end_hp \
) { \
	switch(boss.mode) { \
	case 0: \
		pattern_0(); \
		break; \
	case 1: \
		pattern_1(); \
		break; \
	case 255: \
		if(boss.phase_frame == 1) { \
			/* Every other teleport goes to the player, clamped away from */ \
			/* the playfield edges; the ones in between go anywhere in the */ \
			/* middle two thirds. */ \
			if(boss.phase_state.patterns_seen & 1) { \
				gengetsu_wave_target_x.v = player_pos.cur.x.v; \
				if(gengetsu_wave_target_x.v < TO_SP(WAVE_TARGET_MARGIN)) { \
					gengetsu_wave_target_x.v = TO_SP(WAVE_TARGET_MARGIN); \
				} else if(gengetsu_wave_target_x.v > TO_SP( \
					PLAYFIELD_W - WAVE_TARGET_MARGIN \
				)) { \
					gengetsu_wave_target_x.v = TO_SP( \
						PLAYFIELD_W - WAVE_TARGET_MARGIN \
					); \
				} \
			} else { \
				gengetsu_wave_target_x.v = (randring2_next16_mod(TO_SP( \
					PLAYFIELD_W - (WAVE_TARGET_MARGIN * 4) \
				)) + TO_SP(WAVE_TARGET_MARGIN * 2)); \
			} \
		} \
		if(gengetsu_1F97A()) { \
			on_teleport_done; \
			boss.phase_frame = 0; \
		} \
		break; \
	} \
	if( \
		(boss.phase_state.patterns_seen >= TELEPORTS_UNTIL_FILLER) && \
		(boss.mode != 255) \
	) { \
		gengetsu_2023B(); \
	} \
	if(boss.phase_state.patterns_seen < TELEPORTS_UNTIL_UNKILLABLE) { \
		if(!gengetsu_20202()) { \
			break; \
		} \
		boss_score_bonus(100); \
	} \
	boss_phase_next(explosion, next_end_hp); \
	boss.mode = 255; \
	gengetsu_wave_target_x.v = TO_SP(PLAYFIELD_W / 2); \
	break; \
}

void pascal far gengetsu_update(void)
{
	if(bombing) {
		extra_boss_bomb_immunity = BOMB_IMMUNITY_FRAMES;
	}
	if(extra_boss_bomb_immunity != 0) {
		extra_boss_bomb_immunity--;
	}

	bullet_template.origin.x.v = (boss.pos.cur.x.v - TO_SP(13));
	bullet_template.origin.y.v = (boss.pos.cur.y.v - TO_SP(48));
	bullet_template.spawn_type = BST_PELLET;

	switch(boss.phase) {
	case 0:
		gengetsu_20202();

		// Re-set on every frame of the entrance, not once.
		boss.hp = GENGETSU_HP;

		if(boss.phase_frame > 128) {
			boss.phase_end_hp = 14700;
			boss.phase++;
			boss.phase_frame = 0;
			snd_se_play(13);
			tiles_bb_col = V_WHITE;
			_asm mov word ptr bg_render_bombing_func, offset mugetsu_gengetsu_bg_render
		}
		break;

	case 1:
		gengetsu_20202();
		if(boss.phase_frame >= 64) {
			boss.phase++;
			boss.mode = 0;
			boss.phase_state.patterns_seen = 0;
			boss.phase_frame = 0;
			boss.pos.velocity.x.v = 0;
		}
		break;

	case 2:
		gengetsu_pattern_phase(
			gengetsu_1FAAA, gengetsu_1FAF7, GENGETSU_TELEPORT_FIRST,
			ET_CIRCLE, 12700
		);

	case 3:
		gengetsu_pattern_phase(
			gengetsu_1FB86, gengetsu_1FC46, GENGETSU_TELEPORT_LATER,
			ET_HORIZONTAL, 8900
		);

	case 4:
		gengetsu_pattern_phase(
			gengetsu_1FD30, gengetsu_1FDFE, GENGETSU_TELEPORT_LATER,
			ET_HORIZONTAL, 5500
		);

	case 5:
		gengetsu_pattern_phase(
			gengetsu_1FE6A, gengetsu_1FEDF, GENGETSU_TELEPORT_LATER,
			ET_HORIZONTAL, 3000
		);

	case 6:
		gengetsu_20202();
		if(gengetsu_1F9C5()) {
			boss.phase++;
			boss.phase_frame = 32;
		}
		break;

	case 7:
		gengetsu_20050();
		if(boss.phase_frame < 1500) {
			if(!gengetsu_20202()) {
				break;
			}
			boss_score_bonus(100);
		}
		boss_phase_next(ET_VERTICAL, 0);
		boss.mode = 255;
		gengetsu_wave_target_x.v = TO_SP(PLAYFIELD_W / 2);
		boss_statebyte[15] = 16;
		break;

	case 8:
		if(boss.phase_frame <= 3000) {
			gengetsu_200B6();
		} else {
			gengetsu_20195();
		}
		if(!gengetsu_20202() && (boss.phase_frame < 5000)) {
			break;
		}
		boss_explode_small(ET_NW_SE);
		boss.phase++;

		// The defeat bonus is the one thing that distinguishes killing
		// Gengetsu from surviving her: the timeout takes the same branch.
		if(boss.phase_frame < 5000) {
			boss.phase_state.defeat_bonus = true;
		} else {
			boss.phase_state.defeat_bonus = false;
		}
		boss.phase_frame = 0;
		boss.mode = 0;
		PaletteTone = 100;
		palette_changed = true;
		break;

	case 9:
		boss.phase_frame++;
		if(boss.phase_frame == 16) {
			boss_explode_small(ET_VERTICAL);
		}
		if(boss.phase_frame == 32) {
			boss_defeat_explode_big(ET_HORIZONTAL, 200);
			snd_se_play(12);

			// No Palettes write at all, unlike every other TH04 boss: the
			// Extra Stage's palette is whatever the fade left it as.
			palette_changed = true;
			player_invincibility_time = BOSS_DEFEAT_INVINCIBILITY_FRAMES;
		}
		break;

	default:
		boss_defeat_update();
		return;
	}

	// Homing bullets keep aiming at wherever she left from for the length of
	// a teleport, because the wave animation moves the sprite and not
	// [boss.pos].
	if(gengetsu_wave_amp == 0) {
		homing_target.x.v = boss.pos.cur.x.v;
		homing_target.y.v = boss.pos.cur.y.v;
	}
	thicklasers_update_and_hittest();
	hud_hp_update_and_render(boss.hp, GENGETSU_HP);
}
/// --------------------------------------------------------
