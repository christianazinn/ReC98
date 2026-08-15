/// Per-frame item movement, collection, and off-playfield removal
/// --------------------------------------------------------------
/// The segment pragma lives in the th05/main033.cpp wrapper rather than here:
/// it only takes effect before any code is generated (kb/codegen/0112).
///
/// TH04 has the same function — `th04_main.asm`'s `items_update`, 0x104 bytes
/// at 13A9:A3CD — and it is the same function, not merely the same name: from
/// the loop entry onwards the two bodies are instruction-for-instruction
/// identical, and TH04's extra 0x13 bytes are entirely one `pointnum_times_2`
/// prologue ahead of the loop. It is NOT lifted here, because it sits 0x9CF
/// bytes into `main_035_TEXT`'s single contribution and therefore needs a
/// kb/codegen/0080 carve first, while TH05's is the last proc of its segment
/// and needs none (kb/codegen/0098).
///
/// So this file is deliberately TH05-only and carries NO `#if (GAME == 4)`
/// arm. An arm that nothing compiles is an arm that nothing has ever proven:
/// th04/main/midboss/m3.cpp spent this campaign as dead code with three
/// unverified declarations in it, and that is the mistake not to repeat.

#include "th04/main/item/item.hpp"
#include "th04/main/item/splash.hpp"
#include "th04/main/player/player.hpp"
#include "th04/main/pointnum/pointnum.hpp"
#include "th04/math/vector.hpp"
#include "th02/snd/snd.h"
#include "libs/master.lib/master.hpp"

// Both still live in th05_main.asm's `main_033_TEXT` contribution, directly
// above this function, and are reached through the zero-byte `public` alias
// the dump already uses for its own `pascal` exports (kb/codegen/0123).
// sub_16F54() applies a collected item's effect; sub_171C8() runs the
// bookkeeping for one that left the playfield.
extern "C" void pascal near sub_16F54(item_t near *item);
extern "C" void pascal near sub_171C8(item_t near *item);

// The collection box, relative to the player's center. Note the asymmetry on
// the Y axis: an item is collected from 24 pixels above the player's center
// down to only 14 below it, which is why this is spelled out rather than
// going through overlap_1d_inplace_fast().
#define ITEM_COLLECT_DIST_LEFT 24
#define ITEM_COLLECT_W 48
#define ITEM_COLLECT_DIST_TOP 24
#define ITEM_COLLECT_H 38

// That test is spelled out below as statements over _BX, with a `goto` for the
// miss branch, and it has to be. Two things were measured on the way here:
//
// 1. It cannot read like an ordinary expression. The item's own coordinate is
//    still live in _AX / _DX at this point, so `(player + dist) - _AX` makes
//    Turbo C++ compute the left operand into its default AX accumulator and
//    emit `sub ax, ax` -- which zeroes the coordinate and makes every item on
//    screen collectable. That is a behaviour bug, not just a size difference,
//    and it is what this parcel's first probe actually built.
// 2. It cannot be a macro folded into an `&&` chain either. Wrapping the same
//    _BX sequence in a comma expression makes the value cross a boolean
//    context, and Turbo C++ then materializes it as 0/1 in AX
//    (`mov ax, 1` / `jmp` / `xor ax, ax` / `or ax, ax` / `je`) instead of
//    branching on the flags -- 22 bytes of it, and it clobbers _AX as well.
//
// Both are general to this campaign, not to this function.
// [verified-by-oracle: the third probe below is byte-exact]

extern "C" void pascal items_update(void)
{
	item_t near *p;
	int i;
	unsigned char angle;

	p = items;
	for(i = 0; i < ITEM_COUNT; (i++, p++)) {
		if(p->flag == F_FREE) {
			continue;
		} else if(p->flag == F_REMOVE) {
			p->flag = F_FREE;
			continue;
		}

		if(items_pull_to_player) {
			pointnum_times_2 = true;
			p->pulled_to_player = true;
			angle = iatan2(
				(player_pos.cur.y.v - p->pos.cur.y.v),
				(player_pos.cur.x.v - p->pos.cur.x.v)
			);
			vector2_near(p->pos.velocity, angle, to_sp(ITEM_PULL_SPEED));
		} else if(p->pulled_to_player) {
			p->pos.velocity.x.v = 0;
			p->pos.velocity.y.v = 0;
			p->pulled_to_player = false;
		}

		/* DX:AX = */ p->pos.update_seg3();
		if(
			(static_cast<subpixel_t>(_AX) <= to_sp(0 - (ITEM_W / 2))) ||
			(static_cast<subpixel_t>(_AX) >= to_sp(PLAYFIELD_W + (ITEM_W / 2))) ||
			(static_cast<subpixel_t>(_DX) >= to_sp(PLAYFIELD_H + (ITEM_H / 2)))
		) {
			p->flag = F_REMOVE;
			sub_171C8(p);
			continue;
		}

		// Items are clamped to the top edge rather than removed there.
		if(static_cast<subpixel_t>(_DX) < to_sp(0 - (ITEM_H / 2))) {
			p->pos.cur.y.v = to_sp(0 - (ITEM_H / 2));
		}
		// Once the item stops rising, it stops drifting sideways as well.
		if(p->pos.velocity.y.v >= 0) {
			p->pos.velocity.x.v = 0;
		}

		if(miss_time != 0) {
			goto missed;
		}
		_BX = player_pos.cur.x.v;
		_BX += to_sp(ITEM_COLLECT_DIST_LEFT);
		_BX -= _AX;
		if(_BX > to_sp(ITEM_COLLECT_W)) {
			goto missed;
		}
		_BX = player_pos.cur.y.v;
		_BX += to_sp(ITEM_COLLECT_DIST_TOP);
		_BX -= _DX;
		if(_BX > to_sp(ITEM_COLLECT_H)) {
			goto missed;
		}
		sub_16F54(p);
		snd_se_play(11);
		p->flag = F_REMOVE;
		continue;

missed:
		p->pos.velocity.y.v++;
	}

	item_splashes_update();
	pointnum_times_2 = false;
}
