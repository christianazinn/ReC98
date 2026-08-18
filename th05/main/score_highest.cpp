/// High score commit, and score reset
/// ----------------------------------
/// Folds [score] into [resident->score_highest] if this run beat the previous
/// record, then clears every per-run score variable.
///
/// This is TH05's occupant of the HUD_PNT_TEXT slot that TH04 fills with
/// score_reset() (th04/main/score_reset.cpp), and it is NOT the same function:
/// it walks the digits downward from the most significant one, deciding the
/// BCD comparison as it goes and copying every digit once the run is known to
/// be a record, and it has neither [score_unused] nor [extends_gained] to
/// clear. Related shape, different body -- two lifts, not one shared one.
///
/// (#included from th04/main/hud/points.cpp under `#if (GAME == 5)`, ahead of
/// lives.cpp, bombs.cpp and that file's own function, which is these four
/// functions' original address order inside HUD_PNT_TEXT. It has no #includes
/// of its own, for the reason lives.cpp states -- most of the headers in
/// points.cpp's closure have no include guard, so including any of them again
/// would be a hard error. kb/codegen/0129.)
///
/// Because this file shares a translation unit with points.cpp, lives.cpp and
/// bombs.cpp, its file-scope names are NOT file-local. Every one of them is
/// prefixed.

// Published by root-dump data blocks and declared by no header, exactly as
// th04/main/score_reset.cpp spells its own such block, and for the same
// reason: th04/main/score.hpp is one of the unguarded headers this translation
// unit cannot safely reach. TH05 clears three of TH04's five.
//
// [score] itself needs nothing -- th04/score.h is in points.cpp's closure
// through the resident header, and SCORE_DIGITS with it.
// ---------------------------------------------------------------------
extern unsigned long score_delta;
extern unsigned long score_delta_frame;
extern unsigned char hiscore_popup_shown;
// ---------------------------------------------------------------------

// The running state of the digit-by-digit BCD comparison, most significant
// digit first.
static const int SCORE_CMP_LOWER = 0; // Decided: not a record. Copy nothing.
static const int SCORE_CMP_EQUAL = 1; // Undecided: every digit so far tied.
static const int SCORE_CMP_HIGHER = 2; // Decided: a record. Copy every digit.

// [inferred from the original's frame] The comparison state lives in the DX
// pseudo-register because the original allocates no stack frame for it at all:
// the prolog is `push bp` / `mov bp, sp` / `push si` with no `sub sp`, so [i]
// takes SI as the one enregistered local and a second variable has nowhere
// else to go. Nothing in the loop is a call, so DX survives it.
extern "C" void near score_highest_update_and_reset(void)
{
	int i;

	_DX = SCORE_CMP_EQUAL;

	// Digit 0 is skipped for the reason th04/main/score_reset.cpp gives: it
	// IS [score.continues_used] (th04/score.h types the two as a union), and
	// neither the high score nor the reset may touch the continue counter.
	for(i = (SCORE_DIGITS - 1); i > 0; i--) {
		if(_DX == SCORE_CMP_EQUAL) {
			if(resident->score_highest.digits[i] < score.digits[i]) {
				_DX = SCORE_CMP_HIGHER;
			} else if(resident->score_highest.digits[i] > score.digits[i]) {
				_DX = SCORE_CMP_LOWER;
			}
		}
		if(_DX == SCORE_CMP_HIGHER) {
			resident->score_highest.digits[i] = score.digits[i];
		}
		score.digits[i] = 0;
	}

	score_delta = 0;
	score_delta_frame = 0;
	hiscore_popup_shown = 0;
}
