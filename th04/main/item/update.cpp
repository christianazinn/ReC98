/// Per-frame item movement, collection, and off-playfield removal
/// --------------------------------------------------------------
/// items_update() is ONE shared body for both games, and it earns that: the
/// two dumps are instruction-for-instruction identical from the loop entry
/// onwards, and TH04's extra 0x13 bytes (0x104 against TH05's 0xF1) are
/// *entirely* the one `pointnum_times_2` prologue armed below. The byte
/// arithmetic is the check that "the only difference" is a complete claim and
/// not merely a plausible one. (kb/codegen/0115 sibling compare, run before
/// any C++ was written.) The off-playfield penalty in this same file is the
/// opposite case: same role, same name, two genuinely different functions, so
/// it is spelled once per game.
///
/// The segment pragma lives in each game's wrapper rather than here, because
/// it only takes effect before any code is generated (kb/codegen/0112), and
/// the two games land this in differently-named segments anyway:
///   TH05  th05/main033.cpp -> main_033_TEXT, 1528:1F48, the tail of its
///         contribution, so a kb/codegen/0098 tail lift with no carve.
///   TH04  th04/it_updt.cpp -> IT_UPDT_TEXT, 13A9:A367, 0x969 bytes into what
///         used to be `main_035_TEXT`: a kb/codegen/0080 head-rename carve
///         that a second, later kb/codegen/0098 tail lift then extended
///         backwards by the 0x66 bytes of the penalty handler and its jump
///         table. Both halves of this file are now the whole object.

#include "th04/main/item/item.hpp"
#include "th04/main/item/splash.hpp"
#include "th04/main/player/player.hpp"
#include "th04/main/pointnum/pointnum.hpp"
#include "th04/math/vector.hpp"
#include "th02/snd/snd.h"
#include "libs/master.lib/master.hpp"
#include "th04/main/playperf.hpp"
#if (GAME == 5)
#include "th04/main/hud/hud.hpp"
#endif

// Still ASM, still unnamed, sitting directly above this function in their own
// dump. Reached through a bare `public` line added to the dump: `extern "C"` +
// `pascal` mangles to the all-uppercase, undecorated name (kb/codegen/0081),
// and TASM's `/mx` leaves *local* symbols case-insensitive, so
// `public SUB_16F54` over `sub_16F54 proc near` publishes exactly what TCC asks
// for and costs zero bytes — kb/codegen/0123's two-line `label` form is only
// needed when the C++ side is not `pascal`.
// Naming follows th04/main/execl.cpp's precedent for this exact case.
// The roles are [inferred from call sites]. These placeholder spellings are NOT
// licensed by a failed search: every one of these bodies is present in the
// dumps, directly above this function. They are retained only because naming
// ASM bodies across two games is its own parcel, and it belongs to the naming
// lane rather than to a codegen fix.
// Recorded with evidence in `state/notes/items_update.md`.
//
// The list started at four, went to three, and is now two: both games'
// off-playfield helpers have left it to become item_left_playfield() below,
// each lifted out of its own dump. That role was [inferred from call sites]
// when this list was written, and both bodies have since confirmed it — every
// item type either of them reacts to is one whose loss is a *penalty*.
#if (GAME == 5)
	extern "C" void pascal near sub_16F54(item_t near *item);
	#define item_collected(item)		sub_16F54(item)
#else
	// TH04's half of that `#define` is gone: the body has been read, it
	// confirms the inferred role, and it is now item_collected() itself, at
	// the front of this object. TH05's stays a placeholder behind the macro
	// until its own dump's sub_16F54() is lifted.
	#include "th04/main/item/collect.cpp"
#endif

#if (GAME == 5)
/// The penalty for letting an item fall off the playfield
/// ------------------------------------------------------
/// TH05 only. TH04's twin is the `#else` arm below, and it is a *different*
/// function rather than a sibling to share a body with: it switches over all
/// six types through a jump table and pushes its result through
/// [item_playperf_lower], while this one tests three types with a compare
/// chain and has no accumulator at all.
/// Only the two `playperf_lower()` cases survive into TH05, and even they lost
/// their delayed-accumulation path.
///
/// The three types that carry a penalty are exactly the three whose loss costs
/// the player something concrete. Everything else — power, big power, full
/// power, and the dream item itself — falls off for free. [inferred from the
/// body; the switch has no default case]
///
/// hud_dream_put() is called on *every* path, including the types that do
/// nothing, which is how the meter gets its unconditional per-item refresh.

void pascal near item_left_playfield(item_t near *item)
{
	switch(item->type) {
	case IT_POINT:
		// Asymmetric on purpose, and both halves are load-bearing.
		// `dream > 1` is `cmp 1` / `JBE`; `dream < BAR_MAX` is `cmp 80h` /
		// `JNB`, the exact shape kb/codegen/0092 predicts and the one
		// th04/main/hud/dream.cpp had to spell *around*. Turbo C++ 4.0J takes
		// each relational operator literally, so neither may be rewritten
		// into a `<=`/`>=` form.
		if((dream > 1) && (dream < BAR_MAX)) {
			dream--;
		}
		break;
	case IT_BOMB:
		playperf_lower(2);
		break;
	case IT_1UP:
		playperf_lower(4);
		break;
	}
	hud_dream_put();
}
/// ------------------------------------------------------
#else
/// The penalty for letting an item fall off the playfield
/// ------------------------------------------------------
/// TH04 only, and the reason the two games cannot share this body: *every*
/// declared item type except IT_FULLPOWER is handled, and four of those six
/// only add to [item_playperf_lower] rather than lowering the rank there and
/// then. That accumulator is drained in one step once it reaches 64, and the
/// drain is a `playperf_lower(1)` *on top of* whatever the type itself already
/// did — so the cheap item types still pay, just later and jointly.
/// [inferred from the body]
///
/// The deltas rank the types by what losing one is worth: 1 for either power
/// item, 2 for a point item, 4 for a dream item, and the two that a player
/// actually loses something by dropping — bomb and 1up — skip the accumulator
/// and lower the rank immediately, by the same 2 and 4. IT_FULLPOWER is the
/// switch's only uncovered value and falls off for free.
/// [inferred from the body; the jump table has six entries against
/// item_type_t's seven non-negative values]
///
/// Dense-range `switch` form (kb/codegen/0135's third row: a range check plus
/// a direct `jmp cs:[bx+tbl]`), so the emitted table is indexed by value while
/// the bodies follow *source* order — which is why the two power items are
/// written first and IT_POINT/IT_DREAM before IT_BOMB/IT_1UP.
/// [verified-by-oracle]

void pascal near item_left_playfield(item_t near *item)
{
	switch(item->type) {
	case IT_POWER:
	case IT_BIGPOWER:
		item_playperf_lower++;
		break;
	case IT_POINT:
		item_playperf_lower += 2;
		break;
	case IT_DREAM:
		item_playperf_lower += 4;
		break;
	case IT_BOMB:
		playperf_lower(2);
		break;
	case IT_1UP:
		playperf_lower(4);
		break;
	}
	if(item_playperf_lower >= 64) {
		// The threshold and the drain are NOT the same number: 64 in, 48 out.
		// So the accumulator keeps a remainder of at least 16 across a drain
		// instead of returning to 0, and the next drain therefore needs only
		// 48 further points rather than 64. Both constants are literal in the
		// dump — the test is `cmp` against 40h and the drain is encoded in the
		// assembler's `add al, -48` direction, not as a subtraction — and
		// neither is a symbol. As the removed dump said in as many words:
		// and that's why we don't declare symbols for the increment and
		// decrement periods of these... The asymmetry is preserved exactly as
		// ZUN wrote it; whether it is deliberate pacing or an off-by-one is
		// not decidable from this function.
		// [verified-by-oracle: both constants; the intent is NOT established]
		item_playperf_lower -= 48;
		playperf_lower(1);
	}
}
/// ------------------------------------------------------
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
