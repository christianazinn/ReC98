#pragma option -zCSHOT_INV_TEXT -zPmain_01

// sub_1214A() was the tail proc of th05_main.asm's contribution to this
// segment, and this object is the segment's only other contribution, so it has
// to be the FIRST code this translation unit emits for every byte below it to
// keep its address (kb/codegen 0112 + 0114). It is defined here rather than in
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

// The miss animation: counts [miss_time] down, spends a life at 0, and drives
// the palette flash. Still assembly, immediately above this function in
// th05_main.asm's SHOT_INV_TEXT, and reached through the zero-byte `label`
// alias that dump now publishes for it (kb/codegen/0123). This is its only
// call site.
extern "C" void near sub_12017(void);

// Fires one round of the current shottype's shot, installed from the shottype.
// Declared here rather than in a header because the two games disagree about
// its linkage: TH05 publishes `_playchar_shot_func`, TH04 keeps the same symbol
// private, and th02/main/player/player.hpp's declaration is fenced inside
// `#if (GAME == 2)`. state/notes/th04_continue_prompt.md records that
// divergence as one of the things blocking a name for sub_E4FC.
extern "C" nearfunc_t_near playchar_shot_func;

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

// player_render() was the tail `include` of th05_main.asm's contribution to
// this segment, and lands immediately after the function above (kb/codegen
// 0112 + 0114). Same shape as the TH05 arm of th04/main/end.cpp.
#include "th04/main/player/render.cpp"

#include "th04/main/player/shots_inv.cpp"
