/// Handoff between two stages
/// --------------------------
/// Frees everything the stage that just ended had loaded: the boss's .BB
/// font, the dialog script, the .STD and .MAP stage data, the superimposition
/// patterns the stage's sprites occupied, and the stage's .CDG slots.
/// main_entry() calls it once stage_loop() has asked for another stage, which
/// is where th04/main/entry.cpp already spelled the name this file adopts.
///
/// ONE body for both games. The two differ only in the two ranges they clear:
/// TH04 frees the boss faceset slots at the end of the .CDG array, TH05 the
/// per-stage ones at its front, and each game's superimposition range starts
/// at a different pattern.
///
/// (#included from th04/demo.cpp and th05/demo.cpp, ahead of
/// th04/main/pause.cpp, because this function became the LAST thing both
/// dumps contributed to DEMO_TEXT once pause() moved into that object. Growing
/// it backwards once more puts the body at its original address and moves
/// nothing else (kb/codegen 0099 + 0114); no carve, no new segment name, no
/// Tupfile.lua line. The same two wrapper edits pause() took, for the same
/// reason: an insertion at the head of th04/main/demo.cpp would renumber the
/// line-anchored citations into that file for no gain.)
///
/// This file reaches only guarded headers, so it can own its includes despite
/// being first in a translation unit that already has two other files' worth
/// of them. th04/formats/dialog.hpp, th04/formats/std.hpp and
/// th04/formats/map.hpp are the three it cannot reach -- none carries an
/// include guard, and each pulls further unguarded headers behind it -- so
/// their three prototypes are spelled below instead, the way
/// th04/main/continue.cpp spells its own (kb/codegen/0129).

#include "libs/master.lib/pc98_gfx.hpp"
#include "th03/formats/cdg.h"
#include "th04/formats/bb.h"
#include "th04/sprites/main_cdg.h"

// The three unguarded headers' worth of declarations, see above. All three are
// `near` in both games; only bb_boss_free(), which th04/formats/bb.h does
// declare, differs (far in TH04, near in TH05).
// ---------------------------------------------------------------------
void near dialog_free(void);
void near std_free(void);
void near map_free(void);
// ---------------------------------------------------------------------

// The two ranges the games disagree on.
//
// TH04's superimposition bounds are (PAT_STAGE, PAT_STAGE_last + 1) from
// th04/sprites/main_pat.h, spelled as literals because that header has a
// different, unguarded file behind the same name in TH05 and this one body is
// compiled for both. TH05's 180 has no name in th05/sprites/main_pat.h at all.
#if (GAME == 5)
	#define STAGE_PAT_FIRST 180
	#define STAGE_PAT_AFTER_LAST 256
	#define STAGE_CDG_FIRST CDG_PER_STAGE
	#define STAGE_CDG_AFTER_LAST (CDG_PER_STAGE_last + 1)
#else
	#define STAGE_PAT_FIRST 128
	#define STAGE_PAT_AFTER_LAST 256
	#define STAGE_CDG_FIRST CDG_FACESET_BOSS
	#define STAGE_CDG_AFTER_LAST (CDG_COUNT - 1)
#endif

void near stage_transition(void)
{
	int slot;

	bb_boss_free();
	dialog_free();
	std_free();
	map_free();
	super_clean(STAGE_PAT_FIRST, STAGE_PAT_AFTER_LAST);
	for(slot = STAGE_CDG_FIRST; slot < STAGE_CDG_AFTER_LAST; slot++) {
		cdg_free(slot);
	}
}

// Undefined again because this file is textually first in its translation
// unit, and the two behind it are not this function's business.
#undef STAGE_PAT_FIRST
#undef STAGE_PAT_AFTER_LAST
#undef STAGE_CDG_FIRST
#undef STAGE_CDG_AFTER_LAST
