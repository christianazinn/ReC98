/* ReC98
 * -----
 * TH02's GAME OVER screen and the continue prompt that follows it. ZUN's
 * object placed both at the end of the code segment that runs into the point
 * number code, which is why they are compiled into that translation unit here
 * - see th02/pointnum.cpp.
 */

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/rank.h"
#include "th02/resident.hpp"
#include "th02/core/globals.hpp"
#include "th02/gaiji/gaiji.h"
#include "th02/hardware/frmdelay.h"
#include "th02/hardware/input.hpp"
#include "th02/hiscore/regist.h"
#include "th02/main/hud/overlay.hpp"
#include "th02/main/item/item.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/replay.hpp"
#include "th02/main/score.hpp"
#include "th02/main/stage/stage.hpp"

// Gaiji string literals, still owned by the dump's data segment and reached
// through the `public` line th02/gaiji/gameover[data].asm now carries.
// [gCONTINUE_PROMPT] used to have the TASM-only `gCONTINUE?` spelling
// (`4377e6a6:th02/gaiji/gameover[data].asm:2`). The _PROMPT suffix also keeps
// it apart from the gCONTINUE that TH04 and TH05 publish, which is the
// unrelated scoredat name-entry string.
extern "C" const char gGAMEOVER[];
extern "C" const char gCONTINUE_PROMPT[];
extern "C" const char gYES[];
extern "C" const char gNO[];
extern "C" const char gCREDIT[];

// 16 ASCII spaces, exactly the 16 TRAM cells covered by [gGAMEOVER]'s 8 gaiji.
// Erases the previous frame of each sliding GAME OVER copy below.
extern "C" const char GAMEOVER_BLANK[];

// Turbo C++ compiled ZUN's far calls to same-code-group functions as
// `nop; push cs; call near ptr`. (kb/codegen/0014, kb/codegen/0083)
#define nopcall_same_group(func) _asm { \
	nop; \
	push	cs; \
	call	near ptr func; \
}

// The four GAME OVER copies all slide towards this one spot, and the prompt
// below reuses the column.
static const int GAMEOVER_LEFT = 18;
static const int GAMEOVER_TOP = 12;

// The number of continues the player is given per credit. Also the reason the
// credit counter below starts at the [gb_3] gaiji.
static const int CONTINUES_MAX = 3;

// Highlights whichever of the two continue options is currently selected, by
// redrawing both with the attributes the caller picked.
void pascal near continue_options_put(unsigned atrb_yes, unsigned atrb_no)
{
	gaiji_putsa(24, 14, gYES, atrb_yes);
	gaiji_putsa(25, 15, gNO, atrb_no);
}

// `ENTER 2, 0` rather than `push bp; mov bp, sp; sub sp, 2`, unlike every
// other function in this translation unit. (kb/codegen/0011)
#pragma option -G-

// Runs the GAME OVER animation, the high score registration menu, and the
// continue prompt. Returns whether the player chose to continue, having
// already reset the per-attempt state that a continue needs in that case.
bool16 near continue_prompt(void)
{
	// [ret] is declared ahead of [confirmable] but assigned after it: the two
	// are mentioned exactly six times each, so DI goes to whichever comes
	// first in the declaration, while the store order is the source order of
	// the two statements below. (kb/codegen/0010)
	bool16 ret;

	// Blocks the prompt from reacting to the shot button that was still held
	// down when the player died: 0 until the button is seen released, 1 from
	// then until it is pressed again, and only 2 accepts a confirmation.
	int confirmable;

	int i;

	confirmable = 0;
	ret = 1;

	nopcall_same_group(overlay_stage_leave_animate);
	palette_settone(50);
	nopcall_same_group(overlay_stage_enter_animate);

	// Four copies of GAME OVER, sliding in from below, above, the left and the
	// right, all arriving at (GAMEOVER_LEFT, GAMEOVER_TOP) at the same time.
	// Each is preceded by a blank at the cell it occupied on the last frame.
	for(i = (GAMEOVER_TOP + 9); i >= GAMEOVER_TOP; i--) {
		text_putsa (GAMEOVER_LEFT, (i + 1), GAMEOVER_BLANK, TX_WHITE);
		gaiji_putsa(GAMEOVER_LEFT, i, gGAMEOVER, TX_WHITE);
		text_putsa (GAMEOVER_LEFT, (23 - i), GAMEOVER_BLANK, TX_WHITE);
		gaiji_putsa(GAMEOVER_LEFT, (24 - i), gGAMEOVER, TX_WHITE);
		text_putsa ((29 - i), GAMEOVER_TOP, GAMEOVER_BLANK, TX_WHITE);
		gaiji_putsa((30 - i), GAMEOVER_TOP, gGAMEOVER, TX_WHITE);
		text_putsa ((i + 7), GAMEOVER_TOP, GAMEOVER_BLANK, TX_WHITE);
		gaiji_putsa((i + 6), GAMEOVER_TOP, gGAMEOVER, TX_WHITE);
		frame_delay(1);
	}
	frame_delay(30);

	key_det = 0;
	while(!key_det) {
		input_reset_sense();
	}
	overlay_wipe();
	regist_menu();

	// No prompt on the last credit, and none in Extra Stage.
	if((resident->continues_used < CONTINUES_MAX) && (rank < RANK_EXTRA)) {
		gaiji_putsa(
			GAMEOVER_LEFT, GAMEOVER_TOP, gCONTINUE_PROMPT, TX_WHITE
		);
		continue_options_put((TX_WHITE | TX_REVERSE), TX_WHITE);
		gaiji_putsa(GAMEOVER_LEFT, 20, gCREDIT, TX_GREEN);
		i = (gb_3 - resident->continues_used);
		gaiji_putca(32, 20, i, TX_GREEN);
		ret = 1;
		while(1) {
			input_reset_sense();
			if((confirmable == 0) && (key_det == 0)) {
				confirmable = 1;
			} else if((confirmable == 1) && (key_det != 0)) {
				confirmable = 2;
			}
			if(key_det & INPUT_UP) {
				ret = 1;
				continue_options_put((TX_WHITE | TX_REVERSE), TX_WHITE);
			}
			if(key_det & INPUT_DOWN) {
				ret = 0;
				continue_options_put(TX_WHITE, (TX_WHITE | TX_REVERSE));
			}
			if(
				(confirmable == 2) && (
					(key_det & INPUT_SHOT) ||
					(key_det & INPUT_OK) ||
					(key_det & INPUT_CANCEL)
				)
			) {
				break;
			}
			frame_delay(1);
		}
		if(!(key_det & INPUT_CANCEL)) {
			goto resume;
		}
	}
	ret = 0;

	// ZUN reached the reset below from both refusals - the one the player
	// makes at the prompt and the two that skip it - through this single
	// store, which is why it is a label rather than an `else` branch.
resume:
	if(ret) {
		replay_save_request_discard();
	} else if(replay_save_request_prompt_needed()) {
		palette_black_out(1);
		t2gosave_post_regist();
	}
	// A ternary, not `if(a < b) { a = b; }`: the original reloads [resident]
	// for the else operand and merges into a single store.
	resident->score_highest = (
		(resident->score_highest < score) ? score : resident->score_highest
	);
	lives = resident->start_lives;
	bombs = resident->start_bombs;
	power = POWER_MIN;
	resident->continues_used++;
	score_reset();
	item_bigpower_override = ((stage_id % 5) + 2);
	power_overflow = 10;
	palette_100();
	overlay_wipe();
	return ret;
}

#pragma option -G
