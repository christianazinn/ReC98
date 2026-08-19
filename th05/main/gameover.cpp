/// GAME OVER screen
/// ----------------
/// TH05's GAME OVER sequence: the caller of continue_prompt(), and the two
/// per-frame overlay fade steps it blocks on. Together they were the entire
/// root-ASM contribution to main__TEXT.
///
/// TH02 owns the same screen from inside its continue_prompt()
/// (th02/main/continue.cpp); TH04 and TH05 split the prompt out, which is why
/// there is a separate function here at all.
///
/// TH04's copy is still ASM, as sub_E541 in th04_main.asm's EXECL_TEXT. The
/// two bodies differ in exactly one statement -- TH04 opens with
///
///     if(stage_id == 5) { nopcall_same_group(end_game_bad); }
///
/// which TH05 has nothing in place of, and that accounts for the whole 313 vs
/// 301 byte difference. This is NOT written as a shared body with a
/// `#if (GAME == 4)` arm, for the same first reason th05/main/continue.cpp
/// gives: the two land in different segments of different objects, TH04's at
/// the head of EXECL_TEXT and this one at the head of main__TEXT. The arm
/// could not be verified from this parcel either -- the TH04 lift needs
/// th04_main.asm and the TH04 half of the link list, neither of which this
/// parcel holds -- and an unverifiable arm is a guess compiled into nothing.
/// When TH04's row is taken, promoting this file to a shared body is a
/// mechanical change; the note records what it would cost.
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
#include "th04/main/null.hpp"
// Also the only include of th04/gaiji/gaiji.h, which has no include guard;
// naming it again above is the one collision this TU's include closure has.
#include "th04/main/hud/overlay.hpp"
#include "th04/snd/snd.h"
#include "th05/hardware/input.h"
#include "th05/resident.hpp"

// Gaiji string literal, still owned by the dump's data segment. Unlike the
// four strings the continue prompt uses, this one had no C-visible alias until
// this lift needed one: th04/gaiji/gameover[data].asm said so in as many words,
// because th04_main.asm was its only reader. That file is shared, so the alias
// added there covers TH04's own future lift as well.
extern "C" const char gGAMEOVER[];

// Two ASCII spaces, exactly the two TRAM cells one gaiji covers. Erases the
// previous frame of the sliding 'G' below. The dump holds two identical
// copies because ZUN wrote the literal once per loop, the same way it holds
// three copies of `"maine"`; the second keeps the `_0` suffix the dump's own
// duplicates use.
extern "C" const char GAMEOVER_G_BLANK[];
extern "C" const char GAMEOVER_G_BLANK_0[];

// The third of the dump's three `"maine"` copies -- th04/main/end.cpp owns the
// other two, and gives the reason a C++ string literal cannot be used instead.
extern "C" const char aMaine_1[];

// This screen's own fade counter. Shaped like [overlay_fade] in
// th04/main/hud/overlay.cpp and for the same reason: the two halves never
// overlap in time, and each of the two functions below uses exactly one.
extern union {
	uint8_t in_frame;
	unsigned char out_time;
} gameover_fade; // = 0

// Defined by th05/main/continue.cpp, which lands immediately after this object
// in the same segment. No header declares it.
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

// Both directions start from the most opaque cel rather than from either end
// of the frame range, so the black-out is instant and only the reveal is
// animated.
static const int GAMEOVER_FADE_TIME = (
	OVERLAY_FADE_CELS * GAMEOVER_FADE_INTERVAL
);

// Where the finished GAME OVER text sits, and the two turning points of the
// single 'G' that flies in ahead of it: right edge of the playfield, in to
// [GAMEOVER_G_TURN], then back out to the left edge of the text.
static const tram_x_t GAMEOVER_G_FROM = 50;
static const tram_x_t GAMEOVER_G_TURN = 8;
static const tram_x_t GAMEOVER_TRAM_LEFT = 20;
static const tram_y_t GAMEOVER_TRAM_Y = 12;

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
// overlay_stage_enter_update_and_render().
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
// and then either restores the playfield for the continued attempt or hands
// the process over to MAINE.EXE for the score tally. The return value is what
// the caller stores into [quit]; only the zero case ever reaches it, because
// the other one has already replaced this process.
unsigned char near gameover(void)
{
	// One word in ZUN's frame, used for two things that are never live at the
	// same time: first the X column of the sliding 'G', then the prompt's
	// answer, which is assigned from a byte-sized return value and read back
	// as a byte for this function's own. TH02's continue_prompt() reuses its
	// own `i` across the same two roles.
	int i;

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
		_asm {
			push	ds;
			push	offset aMaine_1;
			nop;
			push	cs;
			call	near ptr GameExecl;
		}
	}
	return i;
}
