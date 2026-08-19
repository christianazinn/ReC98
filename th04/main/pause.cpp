/// Pause menu
/// ----------
/// Blocks the caller until the player either resumes or quits, and returns
/// which. Shared by TH04 and TH05, with exactly one difference: the input the
/// menu waits to be released on the way out.
///
/// (#included from th04/demo.cpp and th05/demo.cpp, ahead of the rest of that
/// object, because this function was the LAST thing both dumps contributed to
/// DEMO_TEXT and that object is the next contribution behind it in both games.
/// Growing it backwards puts the body at its original address and moves
/// nothing else (kb/codegen 0099 + 0114); no carve, no new segment name, no
/// Tupfile.lua line. Two wrapper edits rather than one at the head of
/// th04/main/demo.cpp, which would have renumbered seven live line-anchored
/// citations into that file for no gain.)
///
/// This file owns its own headers rather than borrowing demo.cpp's, which it
/// can only do because th04/hardware/inputvar.h grew an include guard in the
/// same commit: demo.cpp includes it directly and this file reaches it through
/// th04/hardware/input.h, so without the guard the two inclusions would be a
/// hard error. A collision set of one is where kb/codegen/0129 says guarding
/// is the cheap fix.

#include "libs/master.lib/pc98_gfx.hpp"
// [measured] NOT interchangeable, and only the oracle caught it: each game's
// header defines [input_reset_sense_interface] as a DIFFERENT function (TH04
// input_reset_sense(), TH05 input_reset_sense_held()), so building TH05
// against TH04's header assembles and links cleanly and calls the wrong
// routine. The residue was two far-call operands inside one segment, four
// bytes in total, with every other instruction identical.
#if (GAME == 5)
	#include "th05/hardware/input.h"
#else
	#include "th04/hardware/input.h"
#endif
#include "th04/main/pause.h"

// The menu's three TRAM rows, and the column all of them start at. Macros
// rather than `static const` only for consistency with the rest of this file's
// numbers; nothing here reaches __emit__().
#define PAUSE_LEFT 26
#define PAUSE_CAPTION_Y 12
#define PAUSE_RESUME_Y 14
#define PAUSE_QUIT_Y 15

// The menu's three gaiji strings, in th04/gaiji/pause[data].asm: the "PAUSE"
// caption, and the "resume" and "quit" choices.
extern "C" const char gsCHUUDAN[];
extern "C" const char gsSAIKAI[];
extern "C" const char gsSHUURYOU[];

// One blank TRAM row per menu row, in th04/main/pause[data].asm. Three
// separate identical strings in the original, not one reused three times.
extern "C" const char aGAME_PAUSE_SPACES_1[];
extern "C" const char aGAME_PAUSE_SPACES_2[];
extern "C" const char aGAME_PAUSE_SPACES_3[];

extern "C" int near pause(void)
{
	int quit = 0;

	// Wait for the key that opened the menu to be released, so that it does
	// not immediately register as a menu input below.
	while(key_det != INPUT_NONE) {
		input_reset_sense_interface();
	}

	gaiji_putsa(PAUSE_LEFT, PAUSE_CAPTION_Y, gsCHUUDAN, TX_YELLOW);
	gaiji_putsa(
		PAUSE_LEFT, PAUSE_RESUME_Y, gsSAIKAI, (TX_WHITE | TX_UNDERLINE)
	);
	gaiji_putsa(PAUSE_LEFT, PAUSE_QUIT_Y, gsSHUURYOU, TX_YELLOW);

	while(1) {
		input_wait_for_change(0);

		// Either direction toggles between the two choices; there are only
		// two, so the original spells that as `1 - quit` rather than as a
		// separate up and down case.
		if((key_det & INPUT_UP) || (key_det & INPUT_DOWN)) {
			quit = (1 - quit);

			// Both arms end in the same call with the same first three
			// arguments and differ only in the attribute, so `-O` cross-jumps
			// them into one shared `call` (kb/codegen/0097). Write both out.
			if(quit == 0) {
				gaiji_putsa(
					PAUSE_LEFT, PAUSE_RESUME_Y, gsSAIKAI,
					(TX_WHITE | TX_UNDERLINE)
				);
				gaiji_putsa(
					PAUSE_LEFT, PAUSE_QUIT_Y, gsSHUURYOU, TX_YELLOW
				);
			} else {
				gaiji_putsa(
					PAUSE_LEFT, PAUSE_RESUME_Y, gsSAIKAI, TX_YELLOW
				);
				gaiji_putsa(
					PAUSE_LEFT, PAUSE_QUIT_Y, gsSHUURYOU,
					(TX_WHITE | TX_UNDERLINE)
				);
			}
		}

		// [verified by the oracle] from the shape of the original: this is the
		// one exit that returns immediately, so it skips both the release wait
		// and the three blanking writes below. Whether that is observable is
		// [not measured] — it depends on what the caller draws next, and
		// nothing here establishes it.
		if(key_det & INPUT_Q) {
			return 1;
		}
		if(key_det & INPUT_CANCEL) {
			quit = 0;
			break;
		}
		if(key_det & INPUT_SHOT) {
			break;
		}
		if(key_det & INPUT_OK) {
			break;
		}
	}

	// And wait for the confirming key to be released as well, so that it does
	// not carry into the game. TH05 only cares about the cancel key here,
	// which is the games' single difference in this function.
	#if (GAME == 5)
		while(key_det & INPUT_CANCEL) {
			input_reset_sense_interface();
		}
	#else
		while(key_det != INPUT_NONE) {
			input_reset_sense_interface();
		}
	#endif

	text_putsa(PAUSE_LEFT, PAUSE_CAPTION_Y, aGAME_PAUSE_SPACES_1, TX_WHITE);
	text_putsa(PAUSE_LEFT, PAUSE_RESUME_Y, aGAME_PAUSE_SPACES_2, TX_WHITE);
	text_putsa(PAUSE_LEFT, PAUSE_QUIT_Y, aGAME_PAUSE_SPACES_3, TX_WHITE);
	return quit;
}
