/// Continue prompt
/// ---------------
/// TH05's YES/NO continue prompt. Like TH04's, and unlike TH02's
/// continue_prompt() which owns the whole GAME OVER sequence, this one is
/// *only* the prompt: the caller in main__TEXT runs the sliding GAME OVER
/// gaiji and the overlay wipe, and calls this afterwards.
///
/// Ported from th04/main/continue.cpp, which is the same function down to the
/// gaiji coordinates. It is a separate body rather than a `#if (GAME == 5)`
/// arm of that file for two independent reasons:
///
/// • The two land in different segments of different objects. TH04's is at the
///   head of EXECL_TEXT, appended by th04/execl.cpp; this one is at the head
///   of main__TEXT's C++ side, appended by th05/laser.cpp. th05/execl.cpp
///   #includes th04/main/execl.cpp as a shared body, so anything added to that
///   file compiles into both games — which is exactly why TH04's copy is not
///   in it either.
/// • Five of the roughly thirty statements differ, and none of the five is a
///   constant: the input loop calls TH05's input_reset_sense_held() once per
///   frame where TH04 pairs input_sense() with input_reset_sense(); [dream] is
///   set to 1 where TH04 clears [dream_items_collected]; the remaining bombs
///   and lives are TH05 globals rather than [resident] fields; the score is
///   reset through score_highest_update_and_reset() rather than
///   score_reset(); and the shot-level installer is a different proc.
///
/// The segment is named here because th05/laser.cpp's basename would otherwise
/// name this object's code segment after the wrapper instead (kb/codegen/0105).

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/frmdelay.h"
#include "th04/gaiji/gaiji.h"
#include "th05/hardware/input.h"
#include "th04/main/quit.hpp"
#include "th04/main/replay.hpp"
#include "th05/resident.hpp"

// Gaiji string literals, still owned by the dump's data segment and reached
// through the `public` line th04/gaiji/gameover[data].asm carries. That file
// is shared: th05_main.asm `include`s the same one, so the four aliases TH04's
// lift added already cover this game and nothing had to be published here.
// [gCONTINUE_PROMPT] is [gCONTINUE?] there; the name follows TH02's and TH04's,
// which renamed it for the same two reasons (a `?` is not a C++ identifier
// character, and th05/main/hiscore.cpp owns the plain gCONTINUE).
extern "C" const char gCONTINUE_PROMPT[];
extern "C" const char gYES[];
extern "C" const char gNO[];
extern "C" const char gCREDIT[];

// Each of these is published by a root-dump BSS or data block and declared by
// no header this translation unit can safely reach — th04/main/item/item.hpp
// and th04/main/hud/hud.hpp are both unguarded, and this file is compiled into
// the same preprocessing pass as th05/main/bullet/laser.cpp. Same reason
// th04/main/continue.cpp gives for its own such block.
// ---------------------------------------------------------------------
extern unsigned char stage_id;
extern unsigned char power;
extern unsigned char continues_used;

// The "dream" meter shown in the HUD. TH04's slot at this address holds the
// unrelated [dream_items_collected], which is why the two bodies disagree on
// both the name and the value.
extern unsigned char dream;

// TH05 keeps the remaining bombs and lives in its own globals; TH04 reads and
// writes [resident->rem_bombs] and [resident->rem_lives] instead, and TH05's
// resident_t has no such fields.
extern unsigned char bombs;
extern unsigned char lives;

void near hiscore_continue_enter(void);
extern "C" void near score_highest_update_and_reset(void);
extern "C" void pascal near hud_score_put(void);
extern "C" void pascal hud_bombs_put(void);
extern "C" void pascal hud_lives_put(void);

// Derives [shot_level] from [power], installs [playchar_shot_func], and
// redraws the power row. Shared with TH04 in th04/main/player/shot_level.cpp.
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

// [POWER_MIN] is an ASM constant (th02/main/player/player.inc), with no TH05
// C++ header to reach it through; th02/main/player/player.hpp spells the same
// value for TH02, and th04/main/continue.cpp does the same for TH04.
static const uint8_t POWER_MIN = 1;

// The number of continues the player is given per credit. Also the reason the
// credit counter below is drawn from the [gb_0] gaiji.
static const uint8_t CONTINUES_MAX = 3;

// [STAGE_EXTRA] -- the Extra Stage, where a continue is not offered -- is NOT
// spelled locally the way th04/main/continue.cpp spells it. th04/common.h
// already defines it, and th05/resident.hpp above reaches that header;
// redeclaring it as a `static const` expands the macro and fails to compile.
// TH04's copy gets away with the local spelling because th04/resident.hpp does
// not include th04/common.h.

#pragma codeseg main__TEXT main_01

// Runs the continue prompt, and returns Q_KEEP_RUNNING if the player chose to
// continue, having already reset the per-attempt state a continue needs.
// The refusal value is *not* Q_QUIT_TO_OP even though it is also 1: the caller
// compares against it to launch MAINE.EXE with the score tally instead, which
// is why this returns a plain byte rather than a quit_t.
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

	// One sense call per frame, at the top of the loop. TH04 instead resets
	// once before the loop and pairs an input_sense() at the top with an
	// input_reset_sense() at the bottom; TH05's held-key variant does both
	// halves in one call, so the prompt is a single statement shorter here.
	while(1) {
		replay_input_reset_sense_held_interstitial();
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
		frame_delay(1);
	}

	if(selected_no) {
		goto refuse;
	}

	hiscore_continue_enter();
	power = POWER_MIN;
	dream = 1;
	bombs = resident->credit_bombs;
	lives = resident->credit_lives;
	nopcall_same_group(player_shot_level_update);
	nopcall_same_group(hud_lives_put);
	nopcall_same_group(hud_bombs_put);
	continues_used++;
	score_highest_update_and_reset();
	hud_score_put();
	return Q_KEEP_RUNNING;

	// All three refusals -- Extra Stage, no credits left, and NO at the
	// prompt -- reach this through a single store placed after the continue
	// path, which is why it is a label rather than three inline returns.
	// Spelling each of them as an inline return of 1 instead moves this block
	// ahead of that path and re-points all three branches. Same shape as
	// TH02's th02/main/continue.cpp resume label, and as TH04's.
	// (kb/codegen/0144)
refuse:
	return 1;
}

#pragma codeseg
