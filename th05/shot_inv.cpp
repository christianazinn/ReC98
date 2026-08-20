#pragma option -zCSHOT_INV_TEXT -zPmain_01

// sub_12017() and then sub_1214A() were the last two procs th05_main.asm
// contributed to this segment, and this object is the segment's only other
// contribution, so they have to be the FIRST code this translation unit emits,
// in that order, for every byte below them to keep its address (kb/codegen 0112
// + 0114). With both lifted, SHOT_INV_TEXT's root contribution is ZERO bytes.
//
// sub_1214A() is defined here rather than in
// a file of its own because a file would have to be named after it, and the
// name is exactly what this parcel does not have: the search that failed is
// recorded in state/notes/th05-main-tail-lifts.md, per
// kb/conventions/naming-precedents.md §3. The dump published the symbol under
// this same spelling before the lift, and th04/main/stage/loop.cpp has called
// it that way ever since the stage loop was decompiled, so nothing at the call
// site changes.

#include "platform.h"
#include "th04/main/player/player.hpp"
#include "th04/main/player/move.hpp"
#include "th04/main/player/shot.hpp"
#include "th04/main/player/bomb.hpp" // DEATHBOMB_WINDOW, and player_bomb()

// sub_12017()'s, both of them. th04/main/phase.hpp gains an include guard in
// the same parcel, because th04/main/boss/boss.hpp also reaches it and this
// object is now in a position to meet both (kb/codegen/0129). th05/resident.hpp
// is unguarded and nothing else in this translation unit pulls it, so this file
// owns it.
#include "th04/main/phase.hpp"   // PHASE_HP_FILL
#include "th05/resident.hpp"     // resident_t, and [resident] itself

// The miss knock-back's move lock, counted down in the `else` branch below.
// Same TU-local alias th04/main/player/bomb.cpp declares, for the same byte and
// for the reason recorded there; TH04 spells it `byte_259A3`.
extern "C" unsigned char byte_2CEBD;
#define miss_move_lock_time byte_2CEBD

// The movement subset of [key_det] that the previous frame's move actually
// accepted. Read and written by nothing else in MAIN.EXE, so its only evidence
// is the loop below; left under IDA's spelling for that reason, with the failed
// search recorded alongside sub_1214A()'s.
extern "C" input_t word_2CE9E;

// Fires one round of the current shottype's shot, installed from the shottype.
// Declared here rather than in a header because the two games disagree about
// its linkage: TH05 publishes `_playchar_shot_func`, TH04 keeps the same symbol
// private, and th02/main/player/player.hpp's declaration is fenced inside
// `#if (GAME == 2)`. state/notes/th04_continue_prompt.md records that
// divergence as one of the things blocking a name for sub_E4FC.
extern "C" nearfunc_t_near playchar_shot_func;

// ---------------------------------------------------------------------
// Everything below is sub_12017()'s, and every one of it is published by a
// root-dump BSS or data block and declared by no header this translation unit
// can safely reach. th04/main/hud/hud.hpp, th04/main/playperf.hpp,
// th04/main/item/item.hpp, th04/main/quit.hpp, th03/hardware/palette.hpp,
// th04/main/frames.h, th04/main/bullet/clearzap.hpp and
// libs/master.lib/pc98_gfx.hpp are all unguarded, and the last two are already
// pulled into this preprocessing pass by th04/main/player/render.cpp at the
// bottom of this file (kb/codegen/0129). Same block, and the same reason, that
// th05/main/continue.cpp gives for its own.
//
// The boss phase is the one exception that could come from a header:
// th04/main/boss/boss.hpp is guarded and declares it as the member
// `boss.phase`. It is spelled here as the flat `_boss_phase` the BSS also
// publishes (th04/main/boss/vars[bss].asm publishes both), because reaching
// boss.hpp would drag in two unguarded headers for one byte compare.
extern "C" unsigned char boss_phase;
extern "C" unsigned char stage_frame_mod2;
extern "C" unsigned char playperf;
extern "C" unsigned char dream;
extern "C" unsigned char lives;
extern "C" unsigned char bombs;
extern "C" unsigned char bullet_clear_time;
extern "C" unsigned char quit;
extern "C" bool palette_changed;
extern "C" unsigned int PaletteTone;

// The dump spells PHASE_HP_FILL's same 0 as PHASE_BOSS_HP_FILL
// (th04/main/phase.inc); the C++ name is the shorter one, from the header
// included at the top of this file.

// Spawns the Game Over item set. Its module is `include`d into
// th05_main.asm's main_033_TEXT, which is group main_03 where this segment is
// main_01 -- so this call is a REAL far call, not a same-group one, and the
// `near` in th04/main/item/item.hpp's declaration is TH04's answer, not this
// caller's.
extern "C" void pascal far items_miss_add(void);

// th04/main/playperf.asm, `arg_bx far` + `public PLAYPERF_LOWER`. The byte
// argument is pushed as a word, which is the `push 4` below.
extern "C" void pascal far playperf_lower(char delta);

extern "C" void pascal hud_dream_put(void);
extern "C" void pascal hud_lives_put(void);
extern "C" void pascal hud_bombs_put(void);
extern "C" void pascal snd_se_play(int new_se);

// Derives [shot_level] from [power], installs [playchar_shot_func] and
// tail-calls hud_power_put(). Still ASM, in SCORE_TEXT, and reached through the
// zero-byte alias that dump publishes for it (kb/codegen/0123). The failed name
// search is state/notes/th05_continue_prompt.md's, and th05/main/continue.cpp
// spells this same declaration for the same reason.
extern "C" void near sub_E4FC(void);

// Returns the player's answer to the continue prompt, which is why the store
// below is a byte out of AL. C++ since MATCH-TH05-MAIN-GAMEOVER; no header
// declares it.
unsigned char near gameover(void);

// MISS_ANIM_FLASH_AT now comes from th04/main/player/player.hpp, which this
// object reaches. It was a local copy here only because that header did not
// carry it when this body was lifted; a sibling parcel added it there and the
// two definitions collided in this one TU.

// Turbo C++ compiled ZUN's far calls to same-code-group functions as
// `nop; push cs; call near ptr`, which no plain C++ far call reproduces.
// (kb/codegen/0014, kb/codegen/0083)
#define nopcall_same_group(func) _asm { \
	nop; \
	push	cs; \
	call	near ptr func; \
}

// The same thing for the one same-group call that takes an argument: the
// argument push has to come BEFORE the nop, and a plain C++ call would emit it
// after. kb/codegen/0014 spells this shape with a compiler emit intrinsic
// instead of the inline push below. That form does not compile HERE, and the
// reason is worth the pointer because the diagnostic names a file this one
// never touches: th04/main/player/render.cpp at the bottom of this object
// pulls x86real.h, whose inline port readers use the same intrinsic on a
// PARAMETER, and an earlier use of it in the same translation unit makes those
// fail to compile. Measured both ways, plus a control, in kb/codegen/0156. The
// inline push is a different parser path and assembles to the same two bytes.
#define nopcall_same_group_1(func, arg) _asm { \
	push	arg; \
	nop; \
	push	cs; \
	call	near ptr func; \
}
// ---------------------------------------------------------------------

// The miss animation. Counts [miss_time] down; at the top of it, cashes the
// miss in (power, playperf, the Game Over item set and the resident miss
// counter); every frame, decays [dream], grows the explosion and flashes the
// palette; and at 0, re-centers the player and either spends a life or ends the
// game. sub_1214A() below is its only caller.
//
// Still under IDA's spelling: the search that failed is recorded in
// state/notes/th05-main-tail-lifts.md per kb/conventions/naming-precedents.md
// §3, and the dump published the symbol under this same spelling before the
// lift, so nothing at the call site changes.
extern "C" void near sub_12017(void)
{
	unsigned char power_lost;

	miss_time--;
	if(miss_time > MISS_ANIM_FRAMES) {
		return;
	}
	if(miss_time == MISS_ANIM_FRAMES) {
		player_pos.velocity.x.v = 0;
		player_pos.velocity.y.v = 0;
		power_overflow = 0;
		miss_explosion_radius = 0;
		items_miss_add();
		power_lost = (power / 4);
		if(power_lost > 0x10) {
			power_lost = 0x10;
		}
		power -= power_lost;
		nopcall_same_group(sub_E4FC);
		snd_se_play(2);
		if(playperf > 38) {
			playperf = 38;
		}
		nopcall_same_group_1(playperf_lower, 4);
		resident->miss_count++;
	}

	// Yes, the [dream] gauge decays on every frame of the miss animation, and
	// twice as fast during a boss's HP fill as it does at any other time.
	if(dream > 2) {
		if((boss_phase == PHASE_HP_FILL) || (stage_frame_mod2 != 0)) {
			dream -= 2;
		}
	} else {
		dream = 1;
	}
	nopcall_same_group(hud_dream_put);

	miss_explosion_radius += MISS_EXPLOSION_RADIUS_VELOCITY;

	// NOT `+=`, which is what the line above is and what this one looks like it
	// should be. `tcc -S` (kb/codegen/0152) grades the two spellings apart on a
	// BYTE and not on a word: `angle += 8` is one `add byte ptr [angle], 8`,
	// while the assignment below promotes to int, adds, and truncates back
	// through AL in three instructions -- which is what ZUN's binary has, 8
	// bytes against 5.
	miss_explosion_angle = (miss_explosion_angle + MISS_EXPLOSION_ANGLE_VELOCITY);

	if(miss_time >= (MISS_ANIM_FRAMES - MISS_ANIM_FLASH_AT)) {
		return;
	}

	// No flash on the last life: the game is about to be over anyway.
	if(lives > 1) {
		if(miss_time & 1) {
			PaletteTone = 150;
		} else {
			PaletteTone = 100;
		}
		palette_changed = true;
	}
	if(miss_time != 0) {
		return;
	}

	player_pos.cur.x.v = (192 * 16);
	player_pos.prev.x.v = (192 * 16);
	player_pos.cur.y.v = (368 * 16);
	player_pos.prev.y.v = (368 * 16);
	player_pos.velocity.x.v = 0;
	player_pos.velocity.y.v = -32;
	if(lives > 1) {
		lives--;
		nopcall_same_group(hud_lives_put);
		bombs = resident->credit_bombs;
		nopcall_same_group(hud_bombs_put);
		bullet_clear_time = 32;
	} else {
		quit = gameover();
	}
}

extern "C" void near sub_1214A(void)
{
	input_t input;
	bool retry;

	if(player_invincibility_time != 0) {
		player_invincibility_time--;
		player_is_hit = false;
	} else if(player_is_hit) {
		miss_time = (MISS_ANIM_FRAMES + DEATHBOMB_WINDOW);
		player_is_hit = false;
		player_invincibility_time = MISS_INVINCIBILITY_FRAMES;

		// Left as the raw value ZUN wrote: the only other write to this byte
		// in either game is stage_init()'s reset to 0, so there is no second
		// site to take a constant's name from.
		miss_move_lock_time = 72;

		player_pos.velocity.x.v = 0;
		player_pos.velocity.y.v = 0;
	}

	if(miss_move_lock_time == 0) {
		player_pos.velocity.x.v = 0;
		player_pos.velocity.y.v = 0;

		// If the full movement input is not one player_move() accepts, drop
		// the directions the previous frame accepted and try the remainder,
		// exactly once. [retry] is what limits it to that one further attempt,
		// and also gates the write-back below: a move that took the second
		// attempt does not become the next frame's mask.
		//
		// Spelled with an explicit label because the original falls straight
		// into the test and jumps back over it, while a `while` gets rotated
		// into `jmp test` / body / test -- kb/codegen/0048's shape, in the
		// direction that entry does not cover.
		input = (key_det & INPUT_MOVEMENT);
		retry = true;
	move_retry:
		if(
			(player_move(input) == MOVE_INVALID) && retry &&
			(word_2CE9E != input)
		) {
			input &= ~word_2CE9E;
			retry = false;
			goto move_retry;
		}

		if(shiftkey) {
			player_pos.velocity.x.v /= 2;
			player_pos.velocity.y.v /= 2;
		}
		player_pos_update_and_clamp();
		if(retry) {
			word_2CE9E = input;
		}

		if((key_det & INPUT_SHOT) && (shot_time == 0)) {
			shot_time = SHOT_CYCLE_FRAMES;
		}
		if(shot_time > SHOT_CYCLE_FRAMES) {
			shot_time = 0;
		}
		if(shot_time != 0) {
			playchar_shot_func();
			shot_time--;
		}
	} else {
		player_pos.update_seg1();
		miss_move_lock_time--;
	}

	// The options lag one frame behind, which is why the current position is
	// the one the player had before this frame's velocity was applied.
	player_option_pos_prev = player_option_pos_cur;
	player_option_pos_cur = player_pos.cur;
	player_option_pos_cur.x.v -= player_pos.velocity.x.v;
	player_option_pos_cur.y.v -= player_pos.velocity.y.v;

	if(key_det & INPUT_BOMB) {
		player_bomb();
	}
	if(miss_time != 0) {
		sub_12017();
	}
}

#undef miss_move_lock_time
#undef nopcall_same_group
#undef nopcall_same_group_1

// player_render() was the tail `include` of th05_main.asm's contribution to
// this segment, and lands immediately after the function above (kb/codegen
// 0112 + 0114). Same shape as the TH05 arm of th04/main/end.cpp.
#include "th04/main/player/render.cpp"

#include "th04/main/player/shots_inv.cpp"
