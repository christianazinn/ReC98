/// The verdict screen's 「気合い」 row
/// ----------------------------------
/// A bonus that has nothing to do with how the run was played and everything
/// to do with how much of a handicap the player took: continues used, bombs
/// carried, Turbo Mode, and graze count, plus a pseudo-random component whose
/// *upper bound* is the share of items the player failed to collect. Applied
/// to [skill] and rendered as a 3.2-digit percentage.
///
/// `[measured]` The proc immediately ahead of verdict_stats_put_and_wait() in
/// th04_maine.asm's MAINE_01_TEXT contribution, and the next one out of that
/// dump's only free end. Same kb/codegen/0098 tail lift as its neighbour: this
/// file is included by th04/staff.cpp immediately ahead of
/// th04/end/verdict_stats.cpp, so the two lifted bodies stay in dump order and
/// land at their original addresses. No carve, no new segment, no group-list
/// edit and no Tupfile.lua line.
///
/// `[measured]` TH05's counterpart is a different function that happens to
/// share this one's opening and closing: same RNG seeding, same continues
/// jump table, same Turbo Mode and graze terms, same fraction-of-a-million
/// render. Its middle is its own — graze counts triple, there is a
/// misses-minus-bombs term, and the clamp is 10,000 rather than 1,000,000 —
/// so no shared body is attempted here.

/// th04/hardware/grppsafx.h has no include guard and th04/end/verdict_digits.cpp,
/// which precedes this file in the same translation unit, already needs it —
/// as it does th04/resident.hpp, libs/master.lib/pc98_gfx.hpp and the
/// declarations of [skill] and graph_fraction_of_million_put(). Relying on the
/// host is the idiom for every other .cpp fragment in this chain.
#include "libs/master.lib/master.hpp"

/// `[measured]` 「％」, and the dump holds a byte-identical second copy of it
/// that the percentage renderer still uses, so it stays a `_DATA` byte there
/// under the `_0` suffix the dump uses for such a duplicate; `-d` would merge
/// a C++ literal pair into one and shift every following byte of that
/// contribution.
extern "C" const shiftjis_t PERCENT_MSG_0[];

/// kb/codegen/0096 + 0139: the original pads the byte before this function's
/// generated continues table, putting it at `0A05:1B25`. Restored to `-a1`
/// immediately after, so that nothing appended to this translation unit
/// inherits the padding silently.
#pragma option -a2

extern "C" void near skill_apply_and_graph_guts(void)
{
	// `[measured]` Declaration order is load-bearing: Turbo C++ gives the
	// first-declared local the slot closest to BP, and the original holds
	// [guts] at `bp-4` with [collected_fraction] behind it at `bp-8`.
	uint32_t guts;
	uint32_t collected_fraction;

	// ZUN quirk: the verdict is the only place that ever consumes the RNG seed
	// the resident structure carried out of the game, and it re-seeds from it
	// here rather than at MAINE.EXE's entry point — so the random term below
	// is a pure function of the run, and replaying the same run gives the same
	// 「気合い」 score.
	irand_init(resident->rand);

	// kb/codegen/0077: a dense table over 1…6, so Turbo C++ subtracts the
	// minimum and bounds the range at 5.
	switch(resident->credit_lives) {
	case 1:
		guts = 2500;
		break;
	case 2:
		guts = 2000;
		break;
	case 3:
		guts = 1500;
		break;
	case 4:
		guts = 1000;
		break;
	case 5:
		guts = 500;
		break;
	case 6:
		guts = 0;
		break;
	}

	// kb/codegen/0135: two cases, so a `cmp`/`jz` chain rather than a table.
	switch(resident->credit_bombs) {
	case 0:
		guts += 2500;
		break;
	case 1:
		guts += 1500;
		break;
	}

	if(resident->turbo_mode) {
		guts += 2000;
	}
	if(resident->graze) {
		// `[measured]` The doubling happens in 16 bits and is only then
		// widened, which is what the `unsigned int` field's own arithmetic
		// gives you. A ≥32,768 graze count therefore wraps before it is ever
		// added — not reachable in a real run, but the shape is load-bearing.
		guts += (resident->graze * 2);
	}

	// The share of items the player did NOT collect, in hundredths of a
	// percent. Same shape as skill_apply_and_graph_percentage()'s, except that
	// this one has no zero-total guard on the initial assignment.
	collected_fraction = 1000000;
	if(resident->items_spawned != resident->items_collected) {
		if(resident->items_spawned != 0) {
			collected_fraction = (collected_fraction / resident->items_spawned);
		} else {
			collected_fraction = 0;
		}
		collected_fraction = (resident->items_collected * collected_fraction);
	}
	collected_fraction = (1000000 - collected_fraction);
	// `[measured]` Compound assignment, not the spelled-out form: the original
	// loads the divisor before the dividend.
	collected_fraction /= 100;

	// ZUN quirk: the *worse* the item collection was, the wider this random
	// range gets — so sloppy item play can outscore perfect item play here,
	// and does so by up to 10,000 before the ×100 below.
	if(collected_fraction != 0) {
		guts += (irand() % collected_fraction);
	}

	guts *= 100;
	if(guts > 1000000) {
		guts = 1000000;
	}
	skill += guts;

	graph_fraction_of_million_put(192, 264, guts);
	graph_putsa_fx(288, 264, VERDICT_COL, PERCENT_MSG_0);
}

#pragma option -a1
