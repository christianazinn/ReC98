/// Score reset
/// -----------
/// Clears every per-run score variable. Called when a new game starts and
/// again on a continue.
///
/// (#included from th04/main/hud/points.cpp, ahead of lives.cpp, bombs.cpp and
/// that file's own function, which is these four functions' original address
/// order inside HUD_PNT_TEXT. It has no #includes of its own, for the reason
/// lives.cpp states -- 12 of the 22 headers in points.cpp's closure have no
/// include guard. kb/codegen/0129.)
///
/// Nothing about this function is HUD code, and the translation unit it lands
/// in is named for a HUD row. That is ZUN's object layout rather than a
/// judgement: this body was the last thing in HUD_PNT_TEXT's root
/// contribution, and the C++ object for that segment already appended
/// directly after it, so this is the only place it can land without a carve.
///
/// Because this file shares a translation unit with points.cpp, lives.cpp and
/// bombs.cpp, its file-scope names are NOT file-local.
///
/// Named for TH02's score_reset(), which th02/main/score.hpp declares with the
/// same role; th04/main/player/bomb.cpp's docblock already lists score_reset()
/// among the singular scalar-state resets this tree names that way.
///
/// Assembly in TH05, and NOT the same function: TH05's proc in this exact slot
/// is score_highest_update_and_reset(), which walks the digits downward while
/// folding each one into resident->score_highest, and which has neither
/// [score_unused] nor [extends_gained] to clear. Related shape, different body
/// -- so the two are two lifts, not one shared one.

// Each of these is published by a root-dump BSS or data block and declared by
// no header. Spelled here rather than added to th04/main/score.hpp, for the
// reason th04/main/execl.cpp gives for its own such block: that header is one
// of the unguarded ones this translation unit cannot safely reach.
//
// [score] itself needs nothing -- th04/score.h is already in points.cpp's
// closure through th04/resident.hpp, and SCORE_DIGITS with it.
// ---------------------------------------------------------------------
extern unsigned long score_delta;
extern unsigned long score_delta_frame;
extern unsigned char score_unused;
extern unsigned char extends_gained;
extern unsigned char hiscore_popup_shown;
// ---------------------------------------------------------------------

void near score_reset(void)
{
	// The digit loop starts at 1, not 0. [score] is a union (th04/score.h)
	// whose element 0 IS [continues_used], and a score reset must not clear
	// the continue counter that MAINE.EXE reads back out of the resident
	// structure. Skipping element 0 is how ZUN spells that.
	int i;

	for(i = 1; i < SCORE_DIGITS; i++) {
		score.digits[i] = 0;
	}

	score_delta = 0;
	score_delta_frame = 0;
	score_unused = 0;
	extends_gained = 0;
	hiscore_popup_shown = 0;
}
