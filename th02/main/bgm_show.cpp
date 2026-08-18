/* ReC98
 * -----
 * TH02's BGM title notification: the note gaiji and the track name that occupy
 * the bottom text row for the first 160 frames after a new track starts.
 * stage_loop() calls this once per frame.
 *
 * ZUN compiled it into main_01___TEXT, between the two null functions at the
 * head of that segment and egc_start_copy_1() at its end. That trailing macro
 * is what kept every proc in the segment unliftable, since a C++ object can
 * only ever append to a root contribution. It now sits in DEMO_TEXT's block
 * instead - same group, same address, one segment boundary moved - which turns
 * bgm_show() into the tail this object takes.
 */

// -G (optimize for speed) is what keeps the prolog at `push bp; mov bp, sp`.
// The -G- that this group's other leftovers compile with turns the same prolog
// into a single ENTER, which is not what the original has here.
// (kb/codegen/0011)
#pragma option -zCmain_01___TEXT -zPmain_01 -G

#include "platform.h"
#include "pc98.h"
#include "shiftjis.hpp"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/gaiji/gaiji.h"

// All three still live in th02_main.asm's own _DATA contribution.
// th02/main/stage/init.cpp declares the first two exactly this way, and is
// what writes [bgm_title_id] whenever a new track starts.
extern "C" uint8_t bgm_show_timer;
extern "C" uint8_t bgm_title_id;
extern "C" shiftjis_t near *BGM_TITLES[];

// 48 half-width spaces, reached through the `public` line th02_main.asm now
// carries (kb/codegen/0123). The dump still spells it this way in sub_C05D,
// which blanks two whole rows with the entire run; this function only needs
// the last 26 cells of it.
extern "C" const shiftjis_t aEMPTY[];

static const int BGM_TITLE_LEFT = 24;
static const int BGM_TITLE_TOP = 23;

// The title sits two cells to the right of the note gaiji.
static const int BGM_TITLE_TEXT_LEFT = (BGM_TITLE_LEFT + 2);

// Frames the notification stays up for.
static const int BGM_TITLE_FRAMES = 160;

// Offset into [aEMPTY] of the first cell this function has to blank, i.e. the
// 26 cells from [BGM_TITLE_LEFT] to the right edge of the text RAM.
static const int BGM_TITLE_BLANK = 22;

// Renders the note gaiji and the current track's title on the frame
// [bgm_show_timer] turns 1, then blanks the row again once the timer runs out.
// Doing nothing at 0 is what makes stage_loop()'s unconditional per-frame call
// cheap for the rest of a stage.
extern "C" void near bgm_show(void)
{
	if(bgm_show_timer == 0) {
		return;
	}
	if(bgm_show_timer == 1) {
		gaiji_putca(BGM_TITLE_LEFT, BGM_TITLE_TOP, gs_NOTES, TX_YELLOW);
		text_putsa(
			BGM_TITLE_TEXT_LEFT,
			BGM_TITLE_TOP,
			BGM_TITLES[bgm_title_id],
			TX_WHITE
		);
	}
	bgm_show_timer++;
	if(bgm_show_timer >= BGM_TITLE_FRAMES) {
		text_putsa(
			BGM_TITLE_LEFT,
			BGM_TITLE_TOP,
			(aEMPTY + BGM_TITLE_BLANK),
			TX_WHITE
		);
		bgm_show_timer = 0;
	}
}
