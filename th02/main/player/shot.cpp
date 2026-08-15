/* ReC98
 * -----
 * TH02's player shot subsystem. The three shottype spawners in the still-ASM
 * part of this segment (shot_a(), shot_b(), shot_c()) scan [shots] for a free
 * slot and then call into here, which is why neither function below takes the
 * slot or the spawn Y as a parameter.
 */

// The original's prolog is a plain `push bp; mov bp, sp` with no locals, which
// is -G. -G- would emit `ENTER 0, 0` even with no frame to set up.
// (kb/codegen/0011)
#pragma option -zCSHOT_TEXT -zPmain_01 -G

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/shot.hpp"

// The patnum the player-shot spawner writes into the shot it creates.
// player_move_and_shoot() sets it from the shottype before each volley; naming
// it needs that function's shottype table, not this one.
extern "C" uint8_t byte_1E519;

// The same, for option shots.
extern "C" uint8_t byte_1E51A;

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
