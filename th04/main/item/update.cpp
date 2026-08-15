/// Per-frame item movement, collection, and off-playfield removal
/// --------------------------------------------------------------
/// ONE shared body for both games, and it earns that: the two dumps are
/// instruction-for-instruction identical from the loop entry onwards, and
/// TH04's extra 0x13 bytes (0x104 against TH05's 0xF1) are *entirely* the one
/// `pointnum_times_2` prologue armed below. The byte arithmetic is the check
/// that "the only difference" is a complete claim and not merely a plausible
/// one. (kb/codegen/0115 sibling compare, run before any C++ was written.)
///
/// The segment pragma lives in each game's wrapper rather than here, because
/// it only takes effect before any code is generated (kb/codegen/0112), and
/// the two games land this in differently-named segments anyway:
///   TH05  th05/main033.cpp -> main_033_TEXT, 1528:1F8E, the LAST proc of its
///         contribution, so a kb/codegen/0098 tail lift with no carve.
///   TH04  th04/it_updt.cpp -> IT_UPDT_TEXT, 13A9:A3CD, 0x9CF bytes into
///         `main_035_TEXT`, so a kb/codegen/0080 head-rename carve.

#include "th04/main/item/item.hpp"
#include "th04/main/item/splash.hpp"
#include "th04/main/player/player.hpp"
#include "th04/main/pointnum/pointnum.hpp"
#include "th04/math/vector.hpp"
#include "th02/snd/snd.h"
#include "libs/master.lib/master.hpp"

// Still ASM, still unnamed, one pair per game, both sitting directly above
// this function in their own dump. Reached through a bare `public` line added
// to the dump: `extern "C"` + `pascal` mangles to the all-uppercase,
// undecorated name (kb/codegen/0081), and TASM's `/mx` leaves *local* symbols
// case-insensitive, so `public SUB_16F54` over `sub_16F54 proc near` publishes
// exactly what TCC asks for and costs zero bytes — kb/codegen/0123's two-line
// `label` form is only needed when the C++ side is not `pascal`.
// Naming follows th04/main/execl.cpp:49-57's precedent for this exact case.
// The roles are [inferred from call sites]. These four placeholder spellings
// are NOT licensed by a failed search: all four bodies are present in the
// dumps, directly above this function (`sub_16F54` / `sub_171C8` in
// `th05_main.asm`, `sub_1DBAE` / `sub_1DDF7` in `th04_main.asm`). They are
// retained only because naming four ASM bodies across two games is its own
// parcel, and it belongs to the naming lane rather than to a codegen fix.
// Recorded with evidence in `state/notes/items_update.md`.
#if (GAME == 5)
	extern "C" void pascal near sub_16F54(item_t near *item);
	extern "C" void pascal near sub_171C8(item_t near *item);
	#define item_collected(item)		sub_16F54(item)
	#define item_left_playfield(item)	sub_171C8(item)
#else
	extern "C" void pascal near sub_1DBAE(item_t near *item);
	extern "C" void pascal near sub_1DDF7(item_t near *item);
	#define item_collected(item)		sub_1DBAE(item)
	#define item_left_playfield(item)	sub_1DDF7(item)
#endif

// The collection box, relative to the player's center: ITEM_COLLECT_DIST_LEFT
// and ITEM_COLLECT_DIST_TOP are the offsets from that center to the box's two
// declared edges, ITEM_COLLECT_W and ITEM_COLLECT_H its size. The X axis is
// symmetric — ITEM_COLLECT_DIST_LEFT is exactly half of ITEM_COLLECT_W — and
// the Y axis is not: ITEM_COLLECT_DIST_TOP is 24 of ITEM_COLLECT_H's 38. That
// asymmetry is why this is spelled out rather than going through
// overlap_1d_inplace_fast().
//
// Whether the declared Y constant is named for the correct side is round 11's
// adjudicated R1/R2 question and is deliberately left alone here; this comment
// only stops asserting a side that no constant declares.
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
#if (GAME == 4)
	// TH04's one addition, and the whole of its extra 0x13 bytes: the flag is
	// armed once up front, ahead of the loop, as well as per-item inside it.
	// So a frame that pulls no items still leaves it *cleared* rather than
	// stale — which is the same end state TH05 reaches by clearing it after
	// the loop, one frame later.
	if(items_pull_to_player) {
		pointnum_times_2 = true;
	} else {
		pointnum_times_2 = false;
	}
#endif
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
			item_left_playfield(p);
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
		// The two subtractions here are inline ASM rather than plain
		// pseudo-register compound assignments, because the original encodes
		// them in the assembler direction, `29 C3` and `29 D3`
		// (`sub r/m16, r16`), while Turbo C++ compiles a pseudo-register
		// subtraction to `2B D8` / `2B DA` (`sub r16, r/m16`). Same
		// instruction, same length, different bytes — and
		// `mzdiff --semantic` cannot tell the two encodings apart, so only
		// funcdiff ever sees the difference. kb/codegen/0037, in the
		// direction that needs no `db` byte pins.
		_BX = player_pos.cur.x.v;
		_BX += to_sp(ITEM_COLLECT_DIST_LEFT);
		asm { sub	bx, ax; }	// bx -= item x
		if(_BX > to_sp(ITEM_COLLECT_W)) {
			goto missed;
		}
		_BX = player_pos.cur.y.v;
		_BX += to_sp(ITEM_COLLECT_DIST_TOP);
		asm { sub	bx, dx; }	// bx -= item y
		if(_BX > to_sp(ITEM_COLLECT_H)) {
			goto missed;
		}
		item_collected(p);
		snd_se_play(11);
		p->flag = F_REMOVE;
		continue;

missed:
		p->pos.velocity.y.v++;
	}

	item_splashes_update();
	pointnum_times_2 = false;
}
