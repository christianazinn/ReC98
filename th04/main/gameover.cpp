/// GAME OVER
/// ---------
/// The GAME OVER screen: the blocking sequence that hides the playfield, flies
/// a single bold-font G in ahead of the GAME OVER text, runs the continue
/// prompt, and then either restores the playfield for the continued attempt or
/// hands the process to MAINE.EXE for the score tally — together with the two
/// per-frame overlay fade steps it blocks on.
///
/// ONE screen, ONE body, two games. The two fade steps are byte-for-byte
/// identical; gameover() itself differs in exactly two places, and both are
/// marked below: the Extra-stage prologue, which TH04 has and TH05 does not,
/// and which of the dump's `"maine"` copies it launches through.
///
/// TH02 owns the same screen from inside its continue_prompt()
/// (th02/main/continue.cpp); TH04 and TH05 split the prompt out, which is why
/// there is a separate function here at all.
///
/// THE TWO HALVES LAND IN DIFFERENT SEGMENTS, AND THAT IS NOT AN OBSTACLE.
/// This body was the entire tail of EXECL_TEXT's root contribution in
/// th04_main.asm and the entire head of main__TEXT's in th05_main.asm. The
/// earlier TH05-only version of this file gave that as its first reason for
/// NOT sharing a body — but a segment name is set by the wrapper, not by the
/// body, and the two wrappers th04/gameover.cpp and th05/gameover.cpp already
/// carry different `-zC`/`-zP` lines for exactly that reason. The same
/// construction now carries th04/main/bullet/render.cpp across BOSS_FG_TEXT
/// and PLAYFLD_TEXT. Its second reason — that the TH04 arm could not be
/// verified from a parcel that held only the TH05 half, and that an
/// unverifiable arm is a guess compiled into nothing — was correct and has
/// simply expired: both halves are on the integration branch and one oracle
/// run now grades both.
///
/// The two fade steps are copies of the stage transition's pair in
/// th04/main/hud/overlay.cpp, with three differences: their own counter
/// instead of [overlay_fade], an interval of 4 frames per cel instead of 8,
/// and a `done` return value, because nothing installs them into [overlay1] --
/// the caller below spins on them instead. They still clear [overlay1] on the
/// last frame, exactly as the stage pair does.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/frmdelay.h"
#include "th02/main/execl.hpp"
#include "th02/main/playfld.hpp"
#include "th04/end/end.h"
#if (GAME == 4)
	// end_game_bad(), for the Extra-stage prologue below. TH05 does not have
	// that branch and must not name this header: it is the one include the two
	// closures do not share.
	#include "th04/main/end.hpp"
#endif
#include "th04/main/null.hpp"
#include "th04/main/replay.hpp"
// Also the only include of th04/gaiji/gaiji.h, which has no include guard;
// naming it again above is the one collision this TU's include closure has.
#include "th04/main/hud/overlay.hpp"
#if (GAME == 5)
	#include "th05/hardware/input.h"
	#include "th05/resident.hpp"
#else
	#include "th04/hardware/input.h"
	#include "th04/resident.hpp"
#endif
#include "th04/snd/snd.h"

// Gaiji string literal, still owned by the dump's data segment. Unlike the four
// strings the continue prompt uses, this one had no C-visible alias until a
// lift needed one -- th04/gaiji/gameover[data].asm said so in as many words,
// because the two dumps were its only readers.
extern "C" const char gGAMEOVER[];

// Two ASCII spaces, exactly the two TRAM cells one gaiji covers. Erases the
// previous frame of the sliding G below. The dump holds two identical copies
// because ZUN wrote the literal once per loop, the same way it holds several
// copies of `"maine"`; the second keeps the `_0` suffix the dump's own
// duplicates use. A C++ literal cannot stand in for either: `-d` is in the base
// cflags and merges duplicate strings, so two occurrences here would collapse
// into the one copy the original does not have.
extern "C" const char GAMEOVER_G_BLANK[];
extern "C" const char GAMEOVER_G_BLANK_0[];

// DIFFERENCE 1/2: the last of each dump's `"maine"` copies -- the fourth of
// TH04's four, the third of TH05's three. th04/main/end.cpp owns the rest in
// each game, and gives the reason a C++ string literal cannot be used instead.
// The two spellings are the dump's own; they are not interchangeable and there
// is no shared alias to hide the difference behind.
#if (GAME == 5)
	extern "C" const char aMaine_1[];
	#define GAMEOVER_MAINE aMaine_1
#else
	extern "C" const char aMaine_2[];
	#define GAMEOVER_MAINE aMaine_2
#endif

// This screen's own fade counter, still a private [_BSS] label in th04_main.asm
// under a kb/codegen/0123 alias. Shaped like [overlay_fade] in
// th04/main/hud/overlay.cpp and for the same reason: the two halves never
// overlap in time, and each of the two steps below uses exactly one.
extern union {
	uint8_t in_frame;
	unsigned char out_time;
} gameover_fade; // = 0

#if (GAME == 4)
	// Declared here rather than through th04/main/stage/stage.hpp, which this
	// translation unit cannot safely reach.
	extern unsigned char stage_id;

	// Turbo C++ compiled ZUN's far calls to same-code-group functions as
	// `nop; push cs; call near ptr`, which no plain C++ far call reproduces.
	// (kb/codegen/0014, kb/codegen/0083)
	#define nopcall_same_group(func) _asm { \
		nop; \
		push	cs; \
		call	near ptr func; \
	}
#endif

// Defined by each game's own th0?/main/continue.cpp, in the object that lands
// immediately after this one in the same segment. No header declares it.
unsigned char near continue_prompt(void);

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
// th04/common.h would give this as (MAIN_STAGE_COUNT - 1), and the reason this
// file used to give for not including it -- that th04/main/continue.cpp
// declares its own `static const uint8_t STAGE_EXTRA` which the header's macro
// would rewrite into `static const uint8_t 6 = 6;` -- was already false when it
// was written under this #if. That declaration was deleted, and the header
// included there instead, twelve hours earlier; th04/common.h is now the only
// place in the tree that spells STAGE_EXTRA. The clause was doubly false: under
// the GAME == 5 arm this translation unit reaches th04/common.h anyway, through
// th05/resident.hpp. The literal stays only because 5 is what the dump holds and
// the derivation is not what this file is for.
#if (GAME == 4)
	static const uint8_t STAGE_FINAL = 5;
#endif

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

	// DIFFERENCE 2/2: TH05 has nothing in place of this, and the whole 313 vs
	// 301 byte difference between the two originals is this block.
	#if (GAME == 4)
		if(stage_id == STAGE_FINAL) {
			nopcall_same_group(end_game_bad);
		}
	#endif

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
	replay_input_wait_for_change(0);
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
			push	offset GAMEOVER_MAINE;
			nop;
			push	cs;
			call	near ptr GameExecl;
		}
	}
	return i;
}
