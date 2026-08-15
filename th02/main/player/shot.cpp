/* ReC98
 * -----
 * TH02's player shot subsystem. The three shottype spawners (shot_a() below,
 * shot_b() and shot_c() still in ASM in the rest of this segment) scan [shots]
 * for a free slot and then call shot_add() / shot_option_add(), which is why
 * neither of those takes the slot or the spawn Y as a parameter.
 */

// The original's prolog is a plain `push bp; mov bp, sp` with no locals, which
// is -G. -G- would emit `ENTER 0, 0` even with no frame to set up.
// (kb/codegen/0011)
#pragma option -zCSHOT_TEXT -zPmain_01 -G

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/math/randring.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/shot.hpp"

// The patnum the player-shot spawner writes into the shot it creates.
// player_move_and_shoot() sets it from the shottype before each volley; naming
// it needs that function's shottype table, not this one.
extern "C" uint8_t byte_1E519;

// The same, for option shots.
extern "C" uint8_t byte_1E51A;

// Ramps between 0 and 26 while the player holds or releases a movement key;
// shot_a() fans its two outermost SHOT_LEVEL_MAX shots out by this much.
// Signed, and player_move_and_shoot() relies on that when it ramps back down.
extern "C" int8_t byte_20610;

// Latches that this button press has already fired its option volley.
// shot_b() and shot_c() set it once their volley completes and skip the option
// loop while it is 1; shot_a(), which has no option shots, only ever clears it.
extern "C" uint8_t byte_205DE;

// The point shottype B's homing shots aim at. Every writer is still-ASM boss
// or midboss code publishing its own position, and both axes are set to 0xFFFF
// while no boss is on screen — which is exactly what shot_b()'s `> 0` guard on
// [boss_pos_y] tests, since 0xFFFF read as a signed int is -1.
extern "C" int boss_pos_x;
extern "C" int boss_pos_y;

// ZUN bloat: [boss_pos_x] again, but only while the shot is fully powered.
// Written by shot_b() and by the still-ASM per-shottype player reset, and read
// by nothing at all — the dump census over the whole binary is closed. It looks
// like the homing target of a cut feature that would have let the OPTION shots
// home too. Reproducing the writes is required for the match; do not delete it.
extern "C" int boss_pos_x_unused;

// The vector length of a player shot, in pixels. Identical to TH04 and TH05's
// shot_velocity_set().
static const int SHOT_VELOCITY = 12;

void pascal near shot_add(pixel_t left_offset, unsigned char angle)
{
	register shot_t near *p;

	_DL = angle;
	p = &shots[shot_slot_i];
	_CX = (page_back * 2);

	shot_flag_and_decay_cel(p) = F_ALIVE;

	// Both indexed with [page_back * 2] rather than [page_back], which is why
	// the two arrays have to be spelled separately: `x_words[_CX + 1]` would
	// make Turbo C++ 4.0J emit the `+ 1` as an `INC BX` instead of folding it
	// into the store's displacement.
	#define x_words(p) reinterpret_cast<subpixel_t near *>(&(p)->pos_on_page[0].x)
	#define y_words(p) reinterpret_cast<subpixel_t near *>(&(p)->pos_on_page[0].y)
	x_words(p)[_CX] = TO_SP(player_topleft.x + left_offset);
	y_words(p)[_CX] = shot_spawn_top;
	#undef y_words
	#undef x_words

	p->velocity.x.v = ((CosTable8[_DL] * SHOT_VELOCITY) >> SUBPIXEL_BITS);
	p->velocity.y.v = ((SinTable8[_DL] * SHOT_VELOCITY) >> SUBPIXEL_BITS);

	shot_patnum_and_from_option(p) = byte_1E519;
}

void pascal near shot_option_add(
	pixel_t left_offset, subpixel_t velocity_x, subpixel_t velocity_y
)
{
	register shot_t near *p;

	p = &shots[shot_slot_i];
	_DX = (page_back * 2);

	// [decay_cel] starts at 1 rather than 0: shots_update_and_render() runs a
	// different, [byte_1E518]-paced animation on option shots and uses this
	// field as its counter instead of as a decay counter.
	shot_flag_and_decay_cel(p) = ((1 << 8) + F_ALIVE);

	#define x_words(p) reinterpret_cast<subpixel_t near *>(&(p)->pos_on_page[0].x)
	#define y_words(p) reinterpret_cast<subpixel_t near *>(&(p)->pos_on_page[0].y)
	x_words(p)[_DX] = TO_SP(
		*player_option_left_left_on_back_page + left_offset
	);
	y_words(p)[_DX] = TO_SP(*player_option_left_top_on_back_page);
	#undef y_words
	#undef x_words

	p->velocity.x.v = velocity_x;
	p->velocity.y.v = velocity_y;

	// The high byte is [from_option].
	shot_patnum_and_from_option(p) = (byte_1E51A + (1 << 8));
}

// The distance from the player's top edge to the Y every shot spawns at.
static const pixel_t SHOT_SPAWN_TOP_OFFSET = 32;

void near shot_a(void)
{
	// [volley_i] counts the shots of this volley that have been spawned so
	// far, and each shot_level's case sets [volley_last] to the index of its
	// last one. Both are register variables, so they must be declared in this
	// order to land in SI and DI respectively.
	register int volley_i;
	register int volley_last;

	// One scratch int, reused for three unrelated purposes across the
	// shot_level cases: the X offset for levels 2 and 3, the extra spread
	// angle for levels 4 and 5, and the angle itself for 6, 8 and 9. It has to
	// stay on the stack — levels 4 and 5 read its low byte. (kb/codegen/0131)
	int tmp;

	volley_i = 0;
	volley_last = 0;
	tmp = 0;
	shot_spawn_top = TO_SP(player_topleft.y + SHOT_SPAWN_TOP_OFFSET);
	for(shot_slot_i = 0; shot_slot_i < SHOT_COUNT; shot_slot_i++) {
		if(shots[shot_slot_i].flag != F_FREE) {
			continue;
		}
		switch(shot_level) {
		case 0:
			shot_add(8, 192);
			break;

		case 1:
			shot_add(8, (randring1_next8_and(3) + 190));
			break;

		case 2:
			if(volley_i == 1) {
				tmp = 16;
			}
			shot_add(tmp, (randring1_next8_and(7) + 188));
			volley_last = 1;
			break;

		case 3:
			if(volley_i == 1) {
				tmp = 16;
			}
			shot_add(tmp, 192);
			volley_last = 1;
			break;

		// Level 5 is level 4 with the two outer shots spread 2 further apart,
		// spelled as a fallthrough rather than as a spread variable set to 0
		// in both branches — which is why the jump table's entries for 4 and 5
		// point at descending addresses.
		case 5:
			tmp = 2;
		case 4:
			if(volley_i == 0) {
				shot_add(8, 192);
			} else if(volley_i == 1) {
				shot_add(8, (187 - tmp));
			} else {
				shot_add(8, (tmp + 197));
			}
			volley_last = 2;
			break;

		case 6:
			if(volley_i == 0) {
				shot_add(0, 192);
			} else if(volley_i == 1) {
				shot_add(16, 192);
			} else {
				tmp = (randring1_next8_and(31) + 177);
				shot_add(8, tmp);
			}
			volley_last = 3;
			break;

		case 7:
			if(volley_i == 0) {
				shot_add(0, 192);
			} else if(volley_i == 1) {
				shot_add(16, 192);
			} else if(volley_i == 2) {
				shot_add(8, 184);
			} else {
				shot_add(8, 200);
			}
			volley_last = 3;
			break;

		case 8:
			if(volley_i == 0) {
				tmp = 192;
			} else if(volley_i == 1) {
				tmp = 184;
			} else if(volley_i == 2) {
				tmp = 200;
			} else if(volley_i == 3) {
				tmp = 176;
			} else {
				tmp = 208;
			}
			shot_add(8, tmp);
			volley_last = 4;
			break;

		case SHOT_LEVEL_MAX:
			if(volley_i == 0) {
				shot_add(0, 192);
			} else if(volley_i == 1) {
				shot_add(16, 192);
			} else if(volley_i == 2) {
				// 0x34 is player_move_and_shoot()'s
				// SHOT_PATNUM_A_UNPOWERED: the two angled shots keep the
				// unpowered sprite even though the volley is fully powered.
				byte_1E519 = 0x34;
				tmp = 179;
				shot_add(8, tmp);
			} else if(volley_i == 3) {
				tmp = 205;
				shot_add(8, tmp);
			} else if(volley_i == 4) {
				byte_1E519 = 0x36;
				tmp = (192 - byte_20610);
				shot_add(-24, tmp);
			} else {
				tmp = (byte_20610 + 192);
				shot_add(40, tmp);
			}
			volley_last = 5;
			break;
		}
		if(volley_i == volley_last) {
			break;
		}
		volley_i++;
	}
	byte_205DE = 0;
}

// The X distance between the left and the right option, in pixels.
static const pixel_t OPTION_DISTANCE = 48;

// shot_b() is the one function in this TU whose generated switch tables are
// padded: the original has a `db 0` between the epilogue and the first table,
// putting both tables at odd addresses. -a2 reproduces it, and it is needed
// ONLY here — shot_a()'s single table is unpadded in the original and stays
// unpadded under -a2, so the pragma is scoped rather than file-wide.
// (kb/codegen/0096, kb/codegen/0138)
#pragma option -a2

void near shot_b(void)
{
	// Same register allocation as shot_a(): SI and DI, in declaration order.
	register int volley_i;
	register int volley_last;

	// The same single scratch int shot_a() reuses for three unrelated roles.
	// Levels 6 and 7 read its low byte, which pins it to the stack.
	// (kb/codegen/0131)
	int tmp;

	// 0 while this call is still spawning the player's own shots, 1 once it
	// has moved on to the option shots. This is the structural idea of the
	// function: ONE slot scan serves both volleys. When the player volley
	// completes, this latches, [volley_i] restarts, and the same `for` runs
	// the option half over whatever slots are still free.
	unsigned char options_phase;

	volley_i = 0;
	volley_last = 0;
	tmp = 0;
	options_phase = 0;
	shot_spawn_top = TO_SP(player_topleft.y + SHOT_SPAWN_TOP_OFFSET);
	for(shot_slot_i = 0; shot_slot_i < SHOT_COUNT; shot_slot_i++) {
		if(shots[shot_slot_i].flag != F_FREE) {
			continue;
		}
		if(options_phase == 0) {
			switch(shot_level) {
			case 0:
				shot_add(8, 192);
				break;

			case 1:
				shot_add(8, (randring1_next8_and(3) + 190));
				break;

			case 2:
				shot_add(8, (randring1_next8_and(7) + 188));
				break;

			// Two complete calls, not one call with a conditional first
			// argument: Turbo C++ cross-jumps the identical tails back to the
			// point where they diverge, which leaves the `push` of the X
			// offset duplicated inside each branch. A ternary instead
			// materialises the offset in AX and pushes it once, which is 2
			// bytes longer and not what the original does.
			case 3:
				if(volley_i == 0) {
					shot_add(0, (randring1_next8_and(7) + 188));
				} else {
					shot_add(16, (randring1_next8_and(7) + 188));
				}
				volley_last = 1;
				break;

			case 4:
				if(volley_i == 1) {
					tmp = 16;
				}
				shot_add(tmp, 192);
				volley_last = 1;
				break;

			case 5:
				if(volley_i == 0) {
					shot_add(0, 190);
				} else {
					shot_add(16, 194);
				}
				volley_last = 1;
				break;

			// Level 7 is level 6 with the two outer shots spread 2 further
			// apart, spelled as a fallthrough rather than as a spread variable
			// zeroed in both branches — the same idiom shot_a() uses at levels
			// 5 and 4, two levels lower, and the reason this jump table's
			// entries for 6 and 7 point at descending addresses.
			case 7:
				tmp = 2;
			case 6:
				if(volley_i == 0) {
					shot_add(8, 192);
				} else if(volley_i == 1) {
					shot_add(8, (187 - tmp));
				} else {
					shot_add(8, (tmp + 197));
				}
				volley_last = 2;
				break;

			case 8:
				if(volley_i == 0) {
					shot_add(0, 192);
				} else if(volley_i == 1) {
					shot_add(16, 192);
				} else {
					tmp = (randring1_next8_and(31) + 177);
					shot_add(8, tmp);
				}
				volley_last = 3;
				break;

			case SHOT_LEVEL_MAX:
				if(volley_i == 0) {
					shot_add(0, 192);
				} else if(volley_i == 1) {
					shot_add(16, 192);
				} else if(volley_i == 2) {
					// 0x32 is player_move_and_shoot()'s powered shottype B
					// patnum; the two homing shots keep it for the rest of
					// the volley.
					byte_1E519 = 0x32;
					if(boss_pos_y > 0) {
						tmp = iatan2(
							((boss_pos_y - player_topleft.y) - 32),
							(boss_pos_x - player_topleft.x)
						);
						tmp += (randring1_next8_and(7) - 3);
					} else {
						tmp = 183;
					}
					shot_add(0, tmp);
				} else if(volley_i == 3) {
					if(boss_pos_y > 0) {
						tmp = iatan2(
							((boss_pos_y - player_topleft.y) - 32),
							((boss_pos_x - player_topleft.x) - 16)
						);
						tmp += (randring1_next8_and(7) - 3);
					} else {
						tmp = 201;
					}
					shot_add(16, tmp);
				}
				volley_last = 3;
				break;
			}
			if(volley_i == volley_last) {
				options_phase = 1;
				volley_i = 0;
				continue;
			}
		} else {
			if(shot_level < 2) {
				break;
			}
			if(shot_level == SHOT_LEVEL_MAX) {
				boss_pos_x_unused = boss_pos_x;
			} else {
				boss_pos_x_unused = -1;
			}
			if(byte_205DE != 0) {
				break;
			}
			switch(shot_level) {
			case 2:
			case 3:
			case 4:
				shot_option_add((volley_i * OPTION_DISTANCE), 0, -4);
				volley_last = 1;
				break;

			case 5:
			case 6:
			case 7:
				if(volley_i == 0) {
					shot_option_add(0, 0, -16);
				} else if(volley_i == 1) {
					shot_option_add(48, 0, -16);
				} else if(volley_i == 2) {
					shot_option_add(-8, -3, -6);
				} else {
					shot_option_add(56, 3, -6);
				}
				volley_last = 3;
				break;

			// ZUN quirk: This case has no `break`, so it falls through into
			// SHOT_LEVEL_MAX's. Both halves call shot_option_add(), which
			// writes into [shots][shot_slot_i], and neither [shot_slot_i] nor
			// [volley_i] moves in between — so at shot_level 8 every option
			// shot is spawned twice into the same slot and the second spawn
			// overwrites the first. The net effect is that shot_level 8's
			// option shots fly SHOT_LEVEL_MAX's wider pattern, and the six
			// vectors below are computed and discarded on every single shot.
			// [volley_last] is 5 either way, so the volley size is unaffected.
			case 8:
				if(volley_i == 0) {
					shot_option_add(0, 0, -34);
				} else if(volley_i == 1) {
					shot_option_add(48, 0, -34);
				} else if(volley_i == 2) {
					shot_option_add(-8, -5, -28);
				} else if(volley_i == 3) {
					shot_option_add(56, 5, -28);
				} else if(volley_i == 4) {
					shot_option_add(-16, -10, -24);
				} else {
					shot_option_add(64, 10, -24);
				}
				volley_last = 5;
				// falls through
			case SHOT_LEVEL_MAX:
				if(volley_i == 0) {
					shot_option_add(0, 0, -54);
				} else if(volley_i == 1) {
					shot_option_add(48, 0, -54);
				} else if(volley_i == 2) {
					shot_option_add(-8, -16, -50);
				} else if(volley_i == 3) {
					shot_option_add(56, 16, -50);
				} else if(volley_i == 4) {
					shot_option_add(-16, -32, -40);
				} else {
					shot_option_add(64, 32, -40);
				}
				volley_last = 5;
				break;
			}
			if(volley_i == volley_last) {
				byte_205DE = 1;
				break;
			}
		}
		volley_i++;
	}
}

// Back to the build's default alignment, so that the next function appended to
// this TU does not silently inherit shot_b()'s table padding.
#pragma option -a1
