/* ReC98
 * -----
 * MAIN.EXE's three accesses to the score file that aren't part of the
 * registration menu: seeding the in-game high score display, and recording a
 * cleared main game or Extra Stage. ZUN's object for this code segment held
 * them right after the registration menu, which is why they are compiled into
 * the same translation unit here — see th02/regist_m.cpp.
 */

#include "th01/rank.h"
#include "th02/common.h"
#include "th02/main/score.hpp"

// The initializer templates for the two local arrays below already sit in
// th02_main.asm's own `_DATA` contribution, and their addresses are
// load-bearing: re-emitting them from here would grow `_DATA` by 10 bytes and
// shift every later byte in it (kb/codegen/0084). So the arrays are
// copy-initialized from the ASM symbols instead, and the struct wrapper is
// what makes Turbo C++ 4.0J emit the same `SCOPY@` call the original uses.
#undef GAME_CLEAR_CONSTANTS
#undef EXTRA_CLEAR_FLAGS

struct game_clear_constants_t { int v[SHOTTYPE_COUNT]; };
struct extra_clear_flags_t { unsigned char v[SHOTTYPE_COUNT]; };

extern const game_clear_constants_t GAME_CLEAR_CONSTANTS;
extern const extra_clear_flags_t EXTRA_CLEAR_FLAGS;

// Loads the score file, then seeds the in-game high score display from its
// top entry. MAINE.EXE's score_highest_get() is the same function without the
// continue count, which MAIN.EXE keeps in the last decimal digit.
void far hiscore_get(void)
{
	scoredat_init();
	hiscore = (
		((hi.score.score[0] / 10) >= score) ? (hi.score.score[0] / 10) : score
	);
	hiscore_continues = (hi.score.score[0] % 10);
}

// Records that the main 5 stages have been cleared with the shot type the
// player is currently using.
//
// ZUN quirk: [cleared] is a field of a *rank*-specific structure that stores a
// *shot type*-specific value, so both functions below have to temporarily
// overwrite [rank] with the shot type to select the right one — and
// scoredat_is_extra_unlocked() in MAINE.EXE then reads them back the same way.
void far scoredat_cleared_set(void)
{
	game_clear_constants_t game_clear_constants = GAME_CLEAR_CONSTANTS;
	char rank_save = rank;

	rank = resident->shottype;
	scoredat_load();
	hi.score.cleared = game_clear_constants.v[rank];
	scoredat_save();
	rank = rank_save;
}

// Records that the Extra Stage has been cleared with the shot type the player
// is currently using. Unlike the function above, these are bit flags, and they
// all live in the Lunatic structure.
void far scoredat_extra_cleared_set(void)
{
	extra_clear_flags_t extra_clear_flags = EXTRA_CLEAR_FLAGS;
	char rank_save = rank;

	rank = RANK_LUNATIC;
	scoredat_load();
	hi.score.cleared |= extra_clear_flags.v[resident->shottype];
	scoredat_save();
	rank = rank_save;
}
