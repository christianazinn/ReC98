/// GAME OVER
/// ---------
/// TH04's GAME OVER screen: the blocking sequence that hides the playfield,
/// flies a single bold-font G in ahead of the GAME OVER text, runs the continue
/// prompt, and then either restores the playfield for the continued attempt or
/// hands the process to MAINE.EXE for the score tally — together with the two
/// per-frame overlay fade steps it blocks on. The three were the entire tail of
/// EXECL_TEXT's root contribution.
///
/// ONE screen, two games. TH05's is th05/main/gameover.cpp, and the two fade
/// steps are byte-for-byte identical there; gameover() itself differs only in
/// the Extra-stage prologue below, which TH04 has and TH05 does not, and in
/// which of the dump's `"maine"` copies it launches through. The names, the
/// constants, the statement order and the two macros are all
/// th05/main/gameover.cpp's, so that the two files can be merged into one body
/// with a `#if (GAME == 5)` around the prologue as soon as both halves are on
/// the same branch. They are not merged here because TH05's lift has not
/// reached the integration branch yet.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/frmdelay.h"
#include "th02/main/execl.hpp"
#include "th02/main/playfld.hpp"
#include "th04/end/end.h"
#include "th04/main/end.hpp"
#include "th04/main/null.hpp"
// Also the only include of th04/gaiji/gaiji.h, which has no include guard.
#include "th04/main/hud/overlay.hpp"
#include "th04/hardware/input.h"
#include "th04/resident.hpp"
#include "th04/snd/snd.h"

// Gaiji string literal, still owned by the dump's data segment. Unlike the four
// strings the continue prompt uses, this one had no C-visible alias until a
// lift needed one -- th04/gaiji/gameover[data].asm said so in as many words,
// because the two dumps were its only readers.
extern "C" const char gGAMEOVER[];

// Two ASCII spaces, exactly the two TRAM cells one gaiji covers. Erases the
// previous frame of the sliding G below. The dump holds two identical copies
// because ZUN wrote the literal once per loop, the same way it holds four
// copies of `"maine"`; the second keeps the `_0` suffix the dump's own
// duplicates use. A C++ literal cannot stand in for either: `-d` is in the base
// cflags and merges duplicate strings, so two occurrences here would collapse
// into the one copy the original does not have.
extern "C" const char GAMEOVER_G_BLANK[];
extern "C" const char GAMEOVER_G_BLANK_0[];

// The fourth of the dump's four `"maine"` copies -- th04/main/end.cpp owns the
// other three and gives the reason a C++ string literal cannot be used instead.
extern "C" const char aMaine_2[];

// This screen's own fade counter, still a private [_BSS] label in th04_main.asm
// under a kb/codegen/0123 alias. Shaped like [overlay_fade] in
// th04/main/hud/overlay.cpp and for the same reason: the two halves never
// overlap in time, and each of the two steps below uses exactly one.
extern union {
	uint8_t in_frame;
	unsigned char out_time;
} gameover_fade; // = 0

// Declared here rather than through th04/main/stage/stage.hpp, which this
// translation unit cannot safely reach.
extern unsigned char stage_id;

// Defined by th04/main/continue.cpp, in the object that lands immediately after
// this one in the same segment. No header declares it.
unsigned char near continue_prompt(void);

// Turbo C++ compiled ZUN's far calls to same-code-group functions as
// `nop; push cs; call near ptr`, which no plain C++ far call reproduces.
// (kb/codegen/0014, kb/codegen/0083)
#define nopcall_same_group(func) _asm { \
	nop; \
	push	cs; \
	call	near ptr func; \
}

// Frames per cel of the fade. Half of the stage transition's
// OVERLAY_FADE_INTERVAL, which is the only number that differs between the two
// animations.
static const int GAMEOVER_FADE_INTERVAL = 4;

// The fade runs one frame past its last cel before it finishes, exactly like
// OVERLAY_FADE_FRAMES.
static const int GAMEOVER_FADE_FRAMES = (
	(OVERLAY_FADE_CELS + 1) * GAMEOVER_FADE_INTERVAL
);

// Both directions start from the most opaque cel rather than from either end of
// the frame range, so the black-out is instant and only the reveal is animated.
static const int GAMEOVER_FADE_TIME = (
	OVERLAY_FADE_CELS * GAMEOVER_FADE_INTERVAL
);

// Where the finished GAME OVER text sits, and the two turning points of the
// single G that flies in ahead of it: right edge of the playfield, in to
// [GAMEOVER_G_TURN], then back out to the left edge of the text.
static const tram_x_t GAMEOVER_G_FROM = 50;
static const tram_x_t GAMEOVER_G_TURN = 8;
static const tram_x_t GAMEOVER_TRAM_LEFT = 20;
static const tram_y_t GAMEOVER_TRAM_Y = 12;

// The last main stage. Dying there is the bad ending, and end_game_bad() does
// not return, so Extra Stage is the only way past that branch into the sequence
// below -- where continue_prompt() is the thing that refuses the continue.
// th04/common.h would give this as (MAIN_STAGE_COUNT - 1) and is deliberately
// NOT included: it also #defines STAGE_EXTRA, and th04/main/continue.cpp
// declares a `static const uint8_t STAGE_EXTRA` that the macro would rewrite
// into `static const uint8_t 6 = 6;` if the two ever shared a translation unit.
static const uint8_t STAGE_FINAL = 5;

// One cel of the fade, over the whole playfield. Identical to
// th04/main/hud/overlay.cpp's overlay_fade_put() apart from the interval, and
// duplicated rather than shared because that one is a macro private to that
// translation unit.
#define gameover_fade_put(frame) { \
	if((frame % GAMEOVER_FADE_INTERVAL) == 0) { \
		unsigned char cel_num = (frame / GAMEOVER_FADE_INTERVAL); \
		if(cel_num != 0) { \
			tram_y_t y = PLAYFIELD_TRAM_TOP; \
			while(y < PLAYFIELD_TRAM_BOTTOM) { \
				tram_x_t x = PLAYFIELD_TRAM_LEFT; \
				while(x < PLAYFIELD_TRAM_RIGHT) { \
					gaiji_putca( \
						x, y, ((g_OVERLAY_FADE_last + 1) - cel_num), TX_BLACK \
					); \
					x += GAIJI_TRAM_W; \
				} \
				y++; \
			} \
		} \
	} \
}

// Runs one frame of the fade that ends with the playfield area of TRAM filled
// with empty gaiji cells, and returns whether that was the last one. Same
// naming as TH02's blocking overlay_stage_enter_animate() and TH04/TH05's
// overlay_stage_enter_update_and_render(). Nothing installs either of these two
// into [overlay1] -- gameover() below spins on them instead -- but they still
// clear it on their last frame, exactly as the stage pair does.
bool near overlay_gameover_enter_update_and_render(void)
{
	if(gameover_fade.in_frame >= GAMEOVER_FADE_FRAMES) {
		overlay_wipe();
		overlay1 = nullfunc_near;
		return true;
	}
	gameover_fade_put(gameover_fade.in_frame);
	gameover_fade.in_frame++;
	return false;
}

// Runs one frame of the fade that ends with the playfield area of TRAM filled
// with opaque black cells, and returns whether that was the last one.
bool near overlay_gameover_leave_update_and_render(void)
{
	if(gameover_fade.out_time == 0) {
		overlay_black();
		overlay1 = nullfunc_near;
		return true;
	}
	gameover_fade.out_time--;
	gameover_fade_put(gameover_fade.out_time);
	return false;
}

// Spins on one of the two fade steps above until it reports itself finished.
// Written out at each of its four call sites in the original rather than
// factored into a function, so it is a macro here.
#define gameover_fade_animate(step) { \
	while(1) { \
		if(step()) { \
			break; \
		} \
		frame_delay(1); \
	} \
}

// Hides the playfield, runs the GAME OVER animation and the continue prompt,
// and then either restores the playfield for the continued attempt or hands the
// process over to MAINE.EXE for the score tally. The return value is what the
// caller stores into [quit]; only the zero case ever reaches it, because the
// other one has already replaced this process.
unsigned char near gameover(void)
{
	// One word in ZUN's frame, used for two things that are never live at the
	// same time: first the X column of the sliding G, then the prompt's answer,
	// which is assigned from a byte-sized return value and read back as a byte
	// for this function's own. TH02's continue_prompt() reuses one local
	// across the same two roles. It stays in memory rather than in SI because
	// the `nopcall_same_group()` below demotes it out of a register
	// (kb/codegen/0143); TH05's, which has no such block, is the same shape for
	// the ordinary reason.
	int i;

	if(stage_id == STAGE_FINAL) {
		nopcall_same_group(end_game_bad);
	}

	gameover_fade.out_time = GAMEOVER_FADE_TIME;
	gameover_fade_animate(overlay_gameover_leave_update_and_render);

	palette_settone(50);
	gameover_fade_animate(overlay_gameover_enter_update_and_render);

	i = GAMEOVER_G_FROM;
	while(i > GAMEOVER_G_TURN) {
		gaiji_putca(i, GAMEOVER_TRAM_Y, gb_G, TX_WHITE);
		frame_delay(1);
		text_putsa(i, GAMEOVER_TRAM_Y, GAMEOVER_G_BLANK, TX_WHITE);
		i -= GAIJI_TRAM_W;
	}
	i = GAMEOVER_G_TURN;
	while(i < GAMEOVER_TRAM_LEFT) {
		gaiji_putca(i, GAMEOVER_TRAM_Y, gb_G, TX_WHITE);
		frame_delay(1);
		text_putsa(i, GAMEOVER_TRAM_Y, GAMEOVER_G_BLANK_0, TX_WHITE);
		i += GAIJI_TRAM_W;
	}
	gaiji_putsa(GAMEOVER_TRAM_LEFT, GAMEOVER_TRAM_Y, gGAMEOVER, TX_WHITE);
	input_wait_for_change(0);
	overlay_wipe();

	i = continue_prompt();

	gameover_fade.out_time = GAMEOVER_FADE_TIME;
	gameover_fade_animate(overlay_gameover_leave_update_and_render);

	if(i == 0) {
		palette_settone(100);
		gameover_fade_animate(overlay_gameover_enter_update_and_render);
		overlay_wipe();
	} else {
		resident->end_sequence = ES_SCORE;
		snd_kaja_func(KAJA_SONG_FADE, 4);
		palette_black_out(4);

		// Same linker-relaxed far call as th04/main/end.cpp's launchers.
		// (kb/codegen/0014)
		_asm {
			push	ds;
			push	offset aMaine_2;
			nop;
			push	cs;
			call	near ptr GameExecl;
		}
	}
	return i;
}
