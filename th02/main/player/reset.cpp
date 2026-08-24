/* ReC98
 * -----
 * TH02's per-attempt player reset. ZUN's object placed it in the same code
 * segment as the point number code, which is why it is compiled into that
 * translation unit here - see th02/pointnum.cpp.
 */

#include "platform.h"
#include "pc98.h"
// th02/resident.hpp, which this function needs for [resident] alone, has no
// include guard, and th02/main/continue.cpp already includes it earlier in this
// translation unit - see th02/pointnum.cpp.
#include "th02/main/main.hpp"
#include "th02/main/playperf.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/shot.hpp"
#include "th02/sprites/main_pat.h"

// Also declared in th02/main/player/shot.cpp, the other translation unit that
// touches them. See that file for what each one is.
extern "C" uint8_t option_shots_alive;
extern "C" int boss_pos_x;
extern "C" int boss_pos_y;
extern "C" int boss_pos_x_unused;
extern "C" int8_t shot_option_decay_interval;

// Three bytes this function writes and the whole binary then leaves alone.
// The census is closed: over every tracked file, each of them appears exactly
// twice - its storage in th02_main.asm and its single store below - with no
// indexed access through a neighbour and no raw-address reference. So they are
// ZUN bloat, and the only honest name is the one that says so, spelled the way
// th02/main/scroll.cpp spells [scroll_unused_2] and th04's shot_reset() spells
// [shot_unused] (kb/codegen/0123 for the zero-byte aliases behind them). The
// stem is the owning object's: 205DFh and 205E0h sit in the shot object's BSS
// run beside [option_shots_alive] and [shot_slot_i], 20611h in the player
// object's beside the shot-stream bytes below. Removing any of them would
// change the bytes.
extern "C" uint8_t shot_unused;
extern "C" uint8_t shot_unused_2;
extern "C" uint8_t player_unused;

// This function selects the fully powered shot patnums for the current
// shottype and resets shottype A's spread-angle delta. The delta stays signed:
// player_move_and_shoot() clamps it after both increasing and decreasing it.
extern "C" uint8_t shot_patnum_powered;
extern "C" uint8_t shot_option_patnum_powered;
extern "C" int8_t shot_a_spread_angle_delta;

void near player_reset(void)
{
	int i;

	for(i = 0; i < SHOT_COUNT; i++) {
		shots[i].flag = F_FREE;
	}

	option_shots_alive = 0;
	player_invincibility_time = 0;
	player_invincible_via_bomb = false;
	quit = false;
	stage_miss_count = 0;
	miss_frame = 0;
	shot_unused = 8;
	shot_unused_2 = 0;
	player_is_hit = PLAYER_NOT_HIT;
	miss_active = false;
	player_option_patnum = PAT_OPTION_A;
	boss_pos_x = -1;
	boss_pos_x_unused = -1;
	boss_pos_y = -1;
	shot_a_spread_angle_delta = 0;
	player_unused = 0;

	switch(resident->shottype) {
	case 0:
		playchar_shot_func = shot_a;
		playchar_speed_aligned_x = 5;
		playchar_speed_aligned_y = 5;
		playchar_speed_diagonal_x = 4;
		playchar_speed_diagonal_y = 4;
		shot_patnum_powered = 0x30;
		break;

	case 1:
		playchar_shot_func = shot_b;
		playchar_speed_aligned_x = 4;
		playchar_speed_aligned_y = 4;
		playchar_speed_diagonal_x = 3;
		playchar_speed_diagonal_y = 3;
		shot_patnum_powered = 0x34;
		shot_option_patnum_powered = 0x37;

		// `++`, not `+= 1`: the increment operator gets the dedicated
		// `INC byte ptr [mem]` form, while the compound assignment round-trips
		// through AL for three extra bytes (kb/codegen/0094). The byte store
		// comes from the SLOT (`_player_option_patnum db PAT_OPTION_A`), not
		// from [main_patnum_t], which th02/sprites/main_pat.h now measures as
		// word-wide; this comment used to claim the opposite.
		player_option_patnum++;
		break;

	case 2:
		playchar_shot_func = shot_c;
		playchar_speed_aligned_x = 3;
		playchar_speed_aligned_y = 3;
		playchar_speed_diagonal_x = 3;
		playchar_speed_diagonal_y = 3;
		shot_option_decay_interval = 3;
		shot_patnum_powered = 0x7D;
		shot_option_patnum_powered = 0x3B;

		// `+= 2` here, and the AL round trip it emits is what the original
		// does - the two spellings are not interchangeable in either
		// direction (kb/codegen/0094).
		player_option_patnum += 2;
		break;
	}
}
