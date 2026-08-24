/// Continue prompt
/// ---------------
/// TH04's YES/NO continue prompt. Unlike TH02's continue_prompt(), which owns
/// the whole GAME OVER sequence, this one is *only* the prompt: the caller in
/// EXECL_TEXT runs the sliding GAME OVER gaiji and the overlay wipe, and calls
/// this afterwards.
///
/// TH04-only, and deliberately NOT part of th04/main/execl.cpp: th05/execl.cpp
/// #includes that file as a shared body for both games, and TH05's counterpart
/// to this function is still ASM in th05_main.asm. th04/execl.cpp #includes
/// this file ahead of it, which is what puts this body back at its original
/// address, before score_last_commit() and GameExecl(). (kb/codegen/0129)

#include "platform.h"
#include "pc98.h"
#include "th04/common.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/frmdelay.h"
#include "th04/gaiji/gaiji.h"
#include "th04/hardware/input.h"
#include "th04/main/quit.hpp"
#include "th04/resident.hpp"

// Gaiji string literals, still owned by the dump's data segment and reached
// through the `public` line th04/gaiji/gameover[data].asm now carries.
// [gCONTINUE_PROMPT] is [gCONTINUE?] there; the name follows TH02's, which
// renamed it for the same two reasons (a `?` is not a C++ identifier
// character, and th04/main/hiscore.cpp owns the plain gCONTINUE).
extern "C" const char gCONTINUE_PROMPT[];
extern "C" const char gYES[];
extern "C" const char gNO[];
extern "C" const char gCREDIT[];

// Each of these is published by a root-dump BSS or data block and declared by
// no header, so they are spelled here rather than reached through
// th04/main/hud/hud.hpp, th04/main/stage/stage.hpp and th04/main/hiscore.hpp.
// Same reason th04/main/execl.cpp and th04/main/score_reset.cpp give for their
// own such blocks: those headers are among the unguarded ones this translation
// unit cannot safely reach.
// ---------------------------------------------------------------------
extern unsigned char stage_id;
extern unsigned char power;
extern unsigned char dream_items_collected;
extern unsigned char continues_used;

void near score_reset(void);
void near hiscore_continue_enter(void);
extern "C" void pascal near hud_score_put(void);
extern "C" void pascal hud_bombs_put(void);
extern "C" void pascal hud_lives_put(void);

// Derives [shot_level] from [power], installs [playchar_shot_func], and
// redraws the power row. Shared with TH05 in th04/main/player/shot_level.cpp.
extern "C" void near player_shot_level_update(void);
// ---------------------------------------------------------------------

// Turbo C++ compiled ZUN's far calls to same-code-group functions as
// `nop; push cs; call near ptr`, which no plain C++ far call reproduces.
// (kb/codegen/0014, kb/codegen/0083)
#define nopcall_same_group(func) _asm { \
	nop; \
	push	cs; \
	call	near ptr func; \
}

// [POWER_MIN] is an ASM constant (th02/main/player/player.inc), with no TH04
// C++ header to reach it through; th02/main/player/player.hpp spells the same
// value for TH02.
static const uint8_t POWER_MIN = 1;

// The number of continues the player is given per credit. Also the reason the
// credit counter below is drawn from the [gb_0] gaiji.
static const uint8_t CONTINUES_MAX = 3;

// [STAGE_EXTRA] -- the Extra Stage, where a continue is not offered -- comes
// from th04/common.h, included above. It used to be re-declared here as a
// local `static const uint8_t`, which shadowed the upstream-published macro
// that 26 other sites across TH04 *and* TH05 consume, and compiled only
// because th04/common.h happened not to be in th04/execl.cpp's include
// closure. th05/main/continue.cpp already spells the hazard out for its own
// half of the same function. (Naming review round 16 section 6.1.)

// Runs the continue prompt, and returns Q_KEEP_RUNNING if the player chose to
// continue, having already reset the per-attempt state a continue needs.
//
// [verified-by-oracle] for the codegen: the return type is a plain byte
// rather than a quit_t because the caller zero-extends `al` itself
// (`mov ah, 0`), which a 2-byte enum return would not have needed.
//
// [inferred] for what the refusal value MEANS. It is 1, and it is *not*
// Q_QUIT_TO_OP even though it shares that value: the dump carries ZUN-adjacent
// commentary saying the caller compares against it to launch MAINE.EXE with
// the score tally instead, and the reading rests on that comment plus the
// caller's shape rather than on a run.
// (state/notes/th04_continue_prompt.md; naming review round 16 section 6.7.)
unsigned char near continue_prompt(void)
{
	// Both are byte temporaries in ZUN's frame, and only [atrb] is live
	// across a call. [credits_left] is also the digit drawn in the credit
	// counter, offset by [gb_0].
	unsigned char atrb;
	unsigned char credits_left;

	// 0 selects YES, 1 selects NO.
	int selected_no;

	// The previous frame's [key_det]. Starting it at 1 rather than 0 is what
	// blocks the prompt from reacting to the shot button that was still held
	// down when the player died: every frame that sees a nonzero value here
	// only re-reads [key_det] and loops, so the menu below does not run until
	// the player has released everything once.
	input_t held;

	if(stage_id == STAGE_EXTRA) {
		goto refuse;
	}
	selected_no = 0;
	held = 1;
	credits_left = (CONTINUES_MAX - continues_used);
	if(credits_left == 0) {
		goto refuse;
	}

	gaiji_putsa(19, 10, gCONTINUE_PROMPT, TX_WHITE);
	gaiji_putsa(24, 13, gYES, (TX_GREEN | TX_REVERSE));
	gaiji_putsa(25, 15, gNO, TX_WHITE);
	gaiji_putsa(19, 22, gCREDIT, TX_GREEN);
	gaiji_putca(33, 22, (credits_left + gb_0), TX_GREEN);
	input_reset_sense();

	while(1) {
		input_sense();
		if(!held) {
			held = key_det;
			if((held & INPUT_UP) || (held & INPUT_DOWN)) {
				selected_no = (1 - selected_no);
				if(!selected_no) {
					atrb = (TX_GREEN | TX_REVERSE);
				} else {
					atrb = TX_WHITE;
				}
				gaiji_putsa(24, 13, gYES, atrb);
				if(selected_no == 1) {
					atrb = (TX_GREEN | TX_REVERSE);
				} else {
					atrb = TX_WHITE;
				}
				gaiji_putsa(25, 15, gNO, atrb);
			}
			if(held & INPUT_CANCEL) {
				selected_no = 1;
				break;
			}
			if(held & INPUT_OK) {
				break;
			}
			if(held & INPUT_SHOT) {
				break;
			}
		} else {
			held = key_det;
		}
		input_reset_sense();
		frame_delay(1);
	}

	if(selected_no) {
		goto refuse;
	}

	hiscore_continue_enter();
	power = POWER_MIN;
	dream_items_collected = 0;
	resident->rem_bombs = resident->credit_bombs;
	resident->rem_lives = resident->credit_lives;
	nopcall_same_group(player_shot_level_update);
	nopcall_same_group(hud_lives_put);
	nopcall_same_group(hud_bombs_put);
	continues_used++;
	score_reset();
	hud_score_put();
	return Q_KEEP_RUNNING;

	// All three refusals -- Extra Stage, no credits left, and NO at the
	// prompt -- reach this through a single store placed after the continue
	// path, which is why it is a label rather than three inline returns.
	// Spelling them `return 1` instead moves this block ahead of that path
	// and re-points all three branches. Same shape as TH02's
	// th02/main/continue.cpp `resume:`. (kb/codegen/0144)
refuse:
	return 1;
}
