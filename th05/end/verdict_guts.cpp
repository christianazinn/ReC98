/// The verdict screen's 「気合い」 row
/// ----------------------------------
/// A bonus for the handicap the player took rather than for how the run was
/// played: continues used, bombs carried, Turbo Mode, graze count, and the
/// misses the player did not spend a bomb on.
///
/// `[measured]` TH04's counterpart is th04/end/verdict_guts.cpp, and it is a
/// DIFFERENT function that shares only this one's opening and closing — same
/// RNG seeding, same six-case continues table, same Turbo Mode and graze
/// terms, same fraction-of-a-million render. Everything between is TH05's
/// own: graze counts triple rather than double, the bombs table has a third
/// case, there is a misses-minus-bombs term where TH04 has a pseudo-random
/// item-collection one, and the clamp is a two-sided [0, 10,000] on the value
/// BEFORE the ×100 where TH04 clamps the product at 1,000,000. That file
/// reached the same conclusion from the other side and declined a shared
/// body; this file carries that decision out.
///
/// `[measured]` This was the FIRST proc of th05_maine.asm's SCORE_TEXT block,
/// so it comes out through the head: th05/regist.cpp contributes to that
/// segment immediately ahead of the dump, and appending this file to that
/// object's tail lands the body at its original address (kb/codegen/0098 +
/// 0114). No carve, no new segment, no group-list edit and no Tupfile.lua
/// line.
///
/// `[measured]` The six-entry `jmp cs:` table Turbo C++ generates from the
/// continues `switch` is emitted after the epilogue and came out of the dump
/// with the body. Unlike TH04's copy this one needs NO `#pragma option -a2`:
/// the original puts its table at `0A54:2496`, which is already even, so the
/// `-a1` state th05/regist.cpp restores at the end of its own `-a2` scope
/// emits the same bytes. Adding `-a2` here would be a no-op today and a trap
/// for whatever is appended next.
///
/// This file has no `#include`s of its own. th05/regist.cpp's chain has
/// already pulled in th05/resident.hpp and th04/hardware/grppsafx.h — both
/// without include guards, via th04/hiscore/regist_menu.cpp — and, through
/// th04/end/verdict_digits.cpp, [skill], VERDICT_COL and
/// graph_fraction_of_million_put(). Relying on the host is the idiom for
/// every other .cpp fragment in this chain.

/// `[measured]` The verdict overlay's origin, published by zero-byte
/// `label word` aliases in th05_maine.asm (kb/codegen/0123) beside
/// [verdict_col]. th04/end/verdict_digits.cpp declares only the colour, so
/// these two are this file's to declare inside th05/regist.cpp's translation
/// unit — th05/end/verdict_scores.cpp declares the same pair in the OTHER
/// object that grows into this dump.
extern "C" screen_x_t verdict_left;
extern "C" vram_y_t verdict_top;

/// `[measured]` 「％」, and the dump holds a byte-identical FIRST copy of it
/// that its still-ASM screen body renders, so this one stays a `_DATA` byte
/// there under the `_0` suffix the dump uses for such a duplicate: the build
/// compiles with `-d`, and moving it out would collapse the pair into a
/// single literal and shift every following byte of that contribution. Same
/// reasoning, and the same name, as TH04's copy.
extern "C" const shiftjis_t PERCENT_MSG_0[];

void near skill_apply_and_graph_guts(void)
{
	int32_t guts;

	// ZUN quirk: the verdict is the only place that ever consumes the RNG seed
	// the resident structure carried out of the game, and it re-seeds from it
	// here rather than at MAINE.EXE's entry point. TH04 does the same.
	//
	// `[measured]` TH05 never actually draws from the generator afterwards —
	// where TH04 spends the seed on a random item-collection term, every term
	// below is deterministic. The seeding is vestigial, and kept because the
	// original keeps it.
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

	// kb/codegen/0135: three cases, so a `cmp`/`jz` chain rather than a table.
	switch(resident->credit_bombs) {
	case 0:
		guts += 2500;
		break;
	case 1:
		guts += 1500;
		break;
	case 2:
		guts += 1000;
		break;
	}

	if(resident->turbo_mode) {
		guts += 2000;
	}
	if(resident->graze) {
		// `[measured]` The tripling happens in 16 bits and is only then
		// widened, which is what the `unsigned int` field's own arithmetic
		// gives you. A graze count of ≥21,846 therefore wraps before it is
		// ever added — not reachable in a real run, but the shape is
		// load-bearing.
		guts += (resident->graze * 3);
	}

	// ZUN quirk: unsigned fields, but a SIGNED result — bombing more often
	// than you died subtracts, and it is the only term that can, which is
	// what makes the clamp below two-sided where TH04's is one-sided.
	guts += ((resident->miss_count - resident->bombs_used) * 200L);

	if(guts < 0) {
		guts = 0;
	} else if(guts > 10000) {
		guts = 10000;
	}
	guts *= 100;
	skill += guts;

	graph_fraction_of_million_put(
		(verdict_left + 176), (verdict_top + 216), guts
	);
	graph_putsa_fx(
		(verdict_left + 272), (verdict_top + 216), VERDICT_COL, PERCENT_MSG_0
	);
}
