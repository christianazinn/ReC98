/* ReC98
 * -----
 * TH02's player shot hit test. ZUN's object for DIALOG_TEXT held this function
 * in front of the vertical boss lasers and the dialog code, so it is compiled
 * into th02/dialog.cpp's translation unit rather than into the shot
 * subsystem's own th02/main/player/shot.cpp. Every boss, midboss and stage
 * enemy calls it once per frame with its own hitbox, and adds the returned
 * damage to its HP counter.
 */

// The original's prolog is `push bp; mov bp, sp; sub sp, 12h`, which is -G;
// -G- would emit `ENTER 12h, 0` instead. (kb/codegen/0011) th02/main/laser.cpp,
// included right after this file, sets it again and turns it back off at its
// own end, so leaving it on here is safe.
// ZUN bloat: -2 rather than the build's default -3, and this function is the
// only thing in the object that needs it. Three independent 386-only forms are
// absent from the original and appear under -3: the two constant sparks_add()
// arguments are pushed as `push 1eh` + `push 1` rather than folded into a
// single `push 1e0001h` (kb/codegen/0125), the seven long forward branches out
// of the loop body are spelled `jcc short $+5` + `jmp near` rather than as
// `0F 8x` near conditionals, and the loop's backward branch is the same dance.
// Restored to -3 at the end of the file, before th02/main/laser.cpp is
// included. [measured]
#pragma option -2 -G

#include "platform.h"
#include "pc98.h"
#include "th02/hardware/pages.hpp"
#include "th02/main/entity.hpp"
#include "th02/main/frames.hpp"
#include "th02/main/score.hpp"
#include "th02/main/spark.hpp"
#include "th02/main/player/bomb.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/shot.hpp"

// Damage added on top of the flat 1 point of every hit when a regular player
// shot starts its decay animation, indexed by [shot_level]:
// 5, 5, 4, 4, 3, 3, 3, 2, 2, 1. The tables shrink as [shot_level] grows
// because a higher level puts more shots on screen per volley, so the total
// damage per volley stays roughly flat. [measured]
extern "C" int near shot_hit_damage[SHOT_LEVEL_MAX + 1];

// The same, for an option shot: 2, 2, 2, 2, 2, 1, 1, 1, 1, 1. Option shots
// never get the extra [shot_hit_damage] point pair below, and unlike player
// shots they are not consumed by the hit.
extern "C" int near shot_option_hit_damage[SHOT_LEVEL_MAX + 1];

// The same, for a player shot at or above SHOT_PATNUM_HIT_LARGE — i.e. exactly
// shottype C's fully powered shot: 0, 0, 8, 8, 7, 7, 6, 5, 4, 3. The two
// leading zeros are unreachable: shot_c() only assigns SHOT_PATNUM_LARGE from
// [shot_level] 2 on. [measured]
extern "C" int near shot_hit_damage_large[SHOT_LEVEL_MAX + 1];

// ANDed with [stage_frame] to gate the one point of damage a bomb deals per
// call. The still-ASM boss code sets it to 3 and to 1, i.e. every 4th and every
// 2nd frame respectively; the dump's initializer is 1. Compared byte-wide
// against a uint32_t [stage_frame], which is what pins it to uint8_t.
extern "C" uint8_t bomb_damage_frame_mask;

// ZUN quirk: The hit test covers 35 of the SHOT_COUNT == 38 slots, so shots in
// the last three slots fly, render and animate but can never damage anything.
// Every one of the seven other loops over [shots] in the binary is bound at 38;
// this one is the only outlier, and [i] has no other use in the function, so it
// is unambiguously the slot count. Slots are handed out by a linear scan from
// 0, so reaching slot 35 needs 36 shots alive at once — marginal, and the
// reachability measurement is still [open]. Damage dealt is simulated state, so
// fixing this desyncs replays: quirk, not bug.
// (state/notes/th02-shot-cluster.md §4/#1)
static const int SHOT_HITTEST_COUNT = 35;

// Pixels by which the hitbox passed in is grown towards the left and towards
// the top before any shot is tested against it.
static const pixel_t SHOT_HITTEST_MARGIN = 8;

// Pixels a large shot's position is moved up and to the left by when it starts
// decaying, so that its 32×32 decay animation stays centered on the 16×16 shot
// it replaces.
static const pixel_t SHOT_DECAY_LARGE_OFFSET = 8;

// Right-shift applied to both velocity components of a player shot that starts
// decaying, so that the decay animation drifts on rather than stopping dead.
static const int SHOT_DECAY_VELOCITY_SHIFT = 3;

// The lowest patnum that earns a decaying player shot any [shot_hit_damage] at
// all. Identical to shots_update_and_render()'s SHOT_PATNUM_ANIMATED, so a
// powered shot deals extra damage and an unpowered one does not.
static const int SHOT_PATNUM_HIT_ANIMATED = 0x32;

// The lowest patnum that earns [shot_hit_damage_large] instead. ZUN bloat: 4
// below SHOT_PATNUM_LARGE (0x7C), the threshold the other two functions in the
// subsystem use for the same distinction. No patnum any spawner assigns falls
// into the gap, so the two thresholds always agree. [measured]
static const int SHOT_PATNUM_HIT_LARGE = 0x78;

// Byte offsets of [pos_on_page][0].x and .y within a shot_t, and the stride
// between the two pages. uint8_t rather than int, and spelled out rather than
// as `sizeof(SPPoint)`: the original computes the hoisted page offset entirely
// in 8-bit registers (`mov al, _page_back` / `shl al, 2` / `add al, 4`), which
// Turbo C++ 4.0J only does when every operand and the destination are
// byte-sized, and `sizeof` would be a size_t. (kb/codegen/0029)
static const uint8_t SHOT_POS_PAGE_STRIDE = 4;
static const uint8_t SHOT_POS_X_OFFSET = 2;
static const uint8_t SHOT_POS_Y_OFFSET = 4;

// A third indexing idiom for [pos_on_page], different from both shot_add()'s
// word indexing and shot_c()'s SPPoint indexing: the byte offset ZUN hoists out
// of the loop already points at the .y half, and .x is reached at -2 from it.
// That is why these two are separate macros rather than uses of
// shots_update_and_render()'s pos_on_back().
#define shot_x_on_back(shot, page_offset) \
	(*reinterpret_cast<subpixel_t near *>( \
		reinterpret_cast<uint8_t near *>(shot) + (page_offset) - \
		(SHOT_POS_Y_OFFSET - SHOT_POS_X_OFFSET) \
	))
#define shot_y_on_back(shot, page_offset) \
	(*reinterpret_cast<subpixel_t near *>( \
		reinterpret_cast<uint8_t near *>(shot) + (page_offset) \
	))

// The same two accesses, but off the copy of [page_offset] that the original
// keeps live in BX across the whole hit branch rather than re-deriving it per
// access. Spelling the register explicitly is what pins that: Turbo C++ 4.0J
// re-materialises `mov al, [page_offset] / mov ah, 0 / mov bx, ax` at every
// access written in terms of the local. (kb/codegen/0130)
#define shot_x_at_bx(shot) 	(*reinterpret_cast<subpixel_t near *>( 		reinterpret_cast<uint8_t near *>(shot) + _BX - 		(SHOT_POS_Y_OFFSET - SHOT_POS_X_OFFSET) 	))
#define shot_y_at_bx(shot) 	(*reinterpret_cast<subpixel_t near *>( 		reinterpret_cast<uint8_t near *>(shot) + _BX 	))

// Reads one of the three damage tables at the already-scaled byte offset in
// [_BX], the same byte-offset-into-a-word-table idiom as the two macros above.
// Subscripting the table instead would make Turbo C++ 4.0J re-derive the index.
// (kb/codegen/0054)
#define damage_at(table) (*reinterpret_cast<int near *>( \
	reinterpret_cast<uint8_t near *>(table) + _BX \
))

// ZUN quirk: A 16-bit add to the low word of a 32-bit [score_t]. Every other
// writer in the binary adds 32 bits wide, so a carry out of the low word is
// dropped here and only here, costing 65,536 points. Extends are granted from
// [score], so a fix moves simulated state and this is a quirk rather than a
// bug. Mechanism [measured]; whether [score_delta] can actually sit within
// [damage] of 0xFFFF between two commits is [open].
#define score_delta_low (*reinterpret_cast<int16_t near *>(&score_delta))

// Tests every player and option shot against the given hitbox, consumes the
// player shots that hit, and returns the total damage dealt — which it also
// adds to [score_delta]. [left] and [top] are the hitbox's top-left corner on
// the back page, in screen pixels; [w] and [h] its size. Callers fold the two
// sizes into a single 32-bit PUSH. (kb/codegen/0125)
int pascal near shots_hittest(
	screen_x_t left, screen_y_t top, pixel_t w, pixel_t h
)
{
	register shot_t near *shot;

	// SI and DI, in declaration order. [shot_top] has to be a register variable
	// rather than a plain local or a bare subexpression: without it, Turbo C++
	// 4.0J spends DI on [damage] instead, which moves every one of the nine
	// stack locals below. The sparks_add() calls then re-spell the expression
	// instead of passing [shot_top], which is what the original does — DI still
	// holds the value at both call sites and is not read there.
	register screen_y_t shot_top;

	// Declaration order is the stack layout: [bp-2] down to [bp-12h], with
	// [page_offset]'s single byte at [bp-0Bh] leaving [bp-0Ch] unused.
	int i;
	screen_x_t shot_left;
	screen_y_t top_edge;
	int damage;
	screen_x_t right;
	unsigned char page_offset;
	int damage_hit;
	int damage_option;
	int damage_large;

	// The original builds the scaled table index directly in BX and re-uses it
	// for all three loads. An ordinary `table[shot_level]` subscript cannot do
	// that: Turbo C++ 4.0J's CSE scope does not reach the index subtree, so it
	// re-derives `shot_level * 2` through AL/AH/AX per statement and copies it
	// to BX each time, costing 19 bytes. Writing the two byte halves of BX and
	// scaling in place (kb/codegen/0034, kb/codegen/0030) pins the index where
	// ZUN has it, and reading each table off that byte offset rather than
	// subscripting it keeps the load a single `mov ax, table[bx]`
	// (kb/codegen/0054). This is kb/codegen/0130's rule applied to an index
	// rather than to an operand: a value ZUN pins in a non-default register is
	// a statement about what the compiler must not be allowed to recompute.
	// [verified-by-oracle]
	_BL = shot_level;
	_BH = 0;
	_BX += _BX;
	damage_hit = damage_at(shot_hit_damage);
	damage_option = damage_at(shot_option_hit_damage);
	damage_large = damage_at(shot_hit_damage_large);
	damage = 0;

	// ZUN quirk: [right] is computed from the *unshifted* [left], three
	// statements before the shift below, and only the player branch uses it.
	// The option branch spells the same edge as `(left + w)` after the shift,
	// so its X window is 8 pixels narrower and shifted rather than widened,
	// while both Y windows and both left edges are identical. A single
	// `right = (left + w)` placed after the shift would have made all three
	// consistent, which is what makes this a statement-ordering oversight
	// rather than an intentional asymmetry. Which shot hits which enemy is
	// simulated state, so this is a quirk, not a bug.
	// (state/notes/th02-shot-cluster.md §4/#2)
	right = (left + w);

	page_offset = ((page_back * SHOT_POS_PAGE_STRIDE) + SHOT_POS_Y_OFFSET);
	top_edge = (top - SHOT_HITTEST_MARGIN);
	left -= SHOT_HITTEST_MARGIN;
	shot = shots;
	i = 0;
	do {
		if(shot->flag != F_ALIVE) {
			goto next_shot;
		}
		if(!shot->from_option) {
			if(shot->decay_cel != 0) {
				goto next_shot;
			}
			shot_left = TO_PIXEL(shot_x_on_back(shot, page_offset));
			if(!((shot_left > left) && (shot_left < right))) {
				goto next_shot;
			}
			_BX = page_offset;
			shot_top = TO_PIXEL(shot_y_at_bx(shot));
			if((shot_top > top_edge) && ((top_edge + h) > shot_top)) {
				damage++;

				// Read 16 bits wide, together with [from_option] — which is
				// known to be 0 inside this branch, so the comparison really is
				// against [patnum] alone. The pair is both read and written at
				// word width across the subsystem, which is what says ZUN's
				// source names a 16-bit lvalue here rather than casting once.
				// (state/notes/th02-shot-cluster.md §4/#4)
				if(shot_patnum_and_from_option(shot) >= SHOT_PATNUM_HIT_ANIMATED) {
					shot->velocity.x.v >>= SHOT_DECAY_VELOCITY_SHIFT;
					shot->velocity.y.v >>= SHOT_DECAY_VELOCITY_SHIFT;
					if(
						shot_patnum_and_from_option(shot) >=
						SHOT_PATNUM_HIT_LARGE
					) {
						shot->decay_cel = 1;
						shot_x_at_bx(shot) -= static_cast<unsigned>(
							TO_SP(SHOT_DECAY_LARGE_OFFSET)
						);
						shot_y_at_bx(shot) -= static_cast<unsigned>(
							TO_SP(SHOT_DECAY_LARGE_OFFSET)
						);
						damage += damage_large;
					} else {
						shot->decay_cel = 2;
						damage += damage_hit;
					}
				}
				sparks_add(
					shot_left,
					TO_PIXEL(shot_y_on_back(shot, page_offset)),
					(TO_SP(1) + 14),
					1,
					false
				);
			}
		} else {
			shot_left = TO_PIXEL(shot_x_on_back(shot, page_offset));
			if(!((shot_left > left) && ((left + w) > shot_left))) {
				goto next_shot;
			}
			shot_top = TO_PIXEL(shot_y_on_back(shot, page_offset));
			if((shot_top > top_edge) && ((top_edge + h) > shot_top)) {
				// An option shot is not consumed by a hit: [decay_cel] is
				// raised from its spawn value of 1 to 2, which starts
				// shots_update_and_render()'s decay animation, and every later
				// frame takes this branch again with [decay_cel] already at 2
				// and keeps dealing damage without spawning a second spark.
				if(shot->decay_cel == 1) {
					sparks_add(
						shot_left,
						TO_PIXEL(shot_y_on_back(shot, page_offset)),
						(TO_SP(2) + 8),
						1,
						false
					);
					shot->decay_cel = 2;
				}
				damage += damage_option;
			}
		}
next_shot:
		i++;
		shot++;
	} while(i < SHOT_HITTEST_COUNT);
	// The cast is what keeps the AND byte-wide: [stage_frame] is 32-bit, and
	// without it Turbo C++ 4.0J widens the uint8_t mask into AX and ANDs 16
	// bits, where the original tests the frame counter's low byte against AL.
	// Operand order matters too: naming the mask first swaps the two operands
	// and tests the mask in memory instead. [measured]
	if(bombing && (
		(static_cast<uint8_t>(stage_frame) & bomb_damage_frame_mask) == 0
	)) {
		damage++;
	}
	// The jump is to the very next instruction and Turbo C++ 4.0J emits no
	// branch for it, but the label is a join point, and a join point discards
	// `-Z`'s "AX still holds [damage]" fact — which is what makes the original
	// re-load [damage] for the return after having just added it into
	// [score_delta]. Without the label the add's operand is reused and the
	// function comes out 3 bytes short. This is kb/codegen/0140's mechanism at
	// a `goto` rather than at a `continue`, the counter-shape that entry lists
	// as untested. What construct ZUN's own source put here is [open]; this is
	// the shape that reproduces the bytes. [verified-by-oracle]
	score_delta_low += damage;
	goto done;
done:
	return damage;
}

#undef score_delta_low
#undef shot_y_on_back
#undef shot_x_on_back

// Back to the build's default target, so that th02/main/laser.cpp and
// th02/main/dialog/dialog.cpp do not silently inherit the -2 above.
#pragma option -3
