/// TH05's staff roll — the scene's script
/// --------------------------------------
/// The tail of th05_maine.asm's MAINE_01__TEXT block, and the proc that empties
/// that dump: with this body gone, th05_maine.asm contributes zero bytes of
/// code to TH05's MAINE.EXE.
///
/// It cannot go into th05/space.cpp, which took the head of the same block.
/// th05/verd_bmp.cpp now contributes the middle bytes that formerly came from
/// the verdict-bitmap ASM module, so this object still has to follow both the
/// dump and that object. Their Tupfile.lua order is POSITION-CRITICAL
/// (kb/codegen/0099 + 0112 + 0114).
///
/// Both pragmas have to precede every emitted byte (kb/codegen/0138), and both
/// are needed for the reasons th05/space.cpp spells out at length:
///
/// • -zC, because kb/codegen/0105's basename default would name this object's
///   code segment STAFFROL_TEXT.
/// • -zP, because the verdict screen's three entry points live in SCORE_TEXT,
///   and the calls to them only stay near because group_01 spans both segments
///   (kb/codegen/0104).
#pragma option -zCMAINE_01__TEXT -zPgroup_01

#include "th05/staff.hpp"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th04/formats/cdg.h"
// Brings th04/hardware/inputvar.h, which has no include guard and must
// therefore not be #included a second time.
#include "th05/hardware/input.h"
#include "th05/snd/snd.h"

/// master.lib's GRCG_OFF_CLOBBERING macro, which spills the port number to DX
/// rather than using the 8-bit immediate form. Spelled the same way
/// th05/space.cpp spells it.
#define grcg_off_clobbering_dx() outportb(0x7C, GC_OFF)

/// Storage that stays in th05_maine.asm, reached through the same
/// kb/codegen/0123 aliases th05/space.cpp reads it through: moving it would
/// move every address after it in that dump's _DATA and _BSS.
///
/// [staffroll_frame] is zeroed here and counted up by
/// staffroll_frame_and_flip(); [staffroll_measure_cur] is what that function
/// leaves behind for the two credit lines to time their fade-out on, and what
/// the two waits in this function poll.
extern "C" int staffroll_frame;
extern "C" int staffroll_measure_cur;

/// The filenames stay _DATA bytes of the root dump, and are published there by
/// renaming their IDA labels (kb/codegen/0123) — the same treatment TH04's
/// staff roll gives its own eleven.
extern "C" const char staff_bgm[];
extern "C" const char stf00_cdg[];
extern "C" const char stf01_cdg[];
extern "C" const char stf02_cdg[];
extern "C" const char stf03_cdg[];
extern "C" const char stf04_cdg[];
extern "C" const char stf05_cdg[];
extern "C" const char stf06_cdg[];
extern "C" const char stf07_cdg[];
extern "C" const char stf08_cdg[];
extern "C" const char stf09_cdg[];
extern "C" const char stf10_cdg[];
extern "C" const char stf01_bft[];
extern "C" const char stf00_bft[];

/// The verdict screen, in th05/end/ — a different object and a different
/// segment, but the same group, so `near` is correct.
void near verdict_stage_scores_put(void);
void near verdict_stats_put(void);
void near verdict_comment_put(void);

/// The top-left corner and the two colors the verdict screen is rendered with.
/// This function picks them for the run that renders the second screen ahead of
/// the staff roll; verdict_animate() picks a different set for the first one.
extern "C" screen_x_t verdict_left;
extern "C" vram_y_t verdict_top;
extern "C" vc2 verdict_col;
extern "C" vc2 verdict_comment_col;

// Constants
// ---------

// .BFT patterns this scene registers: stf01.bft brings the particle and star
// cels (0 - 7), stf00.bft the orb's four 32x32 ones (8 - 11). ZUN loads them in
// that order, and super_convert_tiny() then converts the 8 small ones.
static const int TINY_PATNUM_COUNT = 8;

// Blue component the palette's color 0 is raised to before the scene starts, so
// that space is not quite black. rain_phase_update() takes it from there.
static const int SPACE_BLUE = 48;

// The BGM measure the fade-in waits for, and the one the orb phase waits for on
// top of its own script.
static const int MEASURE_ORB_GATHER = 30;
static const int MEASURE_FIRST_CREDIT = 48;

// [PaletteTone] steps of the fade-in, one per frame, and the frames the gather
// animation runs for.
static const int FADE_IN_FRAMES = 100;
static const int ORB_GATHER_FRAMES = 32;

// Frames of the last credit line, counted from the moment the rain phase's last
// line is done. The verdict screen is snapped and shown at fixed points along
// the way, and the player takes over at the end.
static const int FRAME_VERDICT_STATS = 256;
static const int FRAME_VERDICT_COMMENT = 320;
static const int FRAME_VERDICT_SNAP = 350;
static const int FRAME_INTERACTIVE = 400;

// VRAM bytes the verdict screen scrolls by per frame, as a row count times the
// width of one row.
static const pixel_t VERDICT_SCROLL_H = 8;

// Which way the verdict screen is currently being scrolled. Both moves are
// one-shot: the state goes back to VERDICT_SCROLL_NONE as soon as the bitmap
// offset hits the end it was headed for.
static const int VERDICT_SCROLL_NONE = 0;
static const int VERDICT_SCROLL_DOWN = 1;
static const int VERDICT_SCROLL_UP = 2;
// ---------

void near staffroll_animate(void)
{
	int i;
	int credit_done;
	int verdict_bitmap_offset;
	int verdict_scroll;

	// The second verdict screen, rendered at a different origin and in
	// different colors from the one verdict_animate() has already shown, and
	// snapped out of VRAM before the staff roll paints over it.
	verdict_left = 32;
	verdict_col = 13;
	verdict_comment_col = 13;
	verdict_top = 16;

	palette_settone(0);

	// ZUN bloat: TDW mode would have been faster for all three of these.
	grcg_setcolor(GC_RMW, 1);
	graph_accesspage(0); grcg_boxfill_8(0, 0, (RES_X - 1), (RES_Y - 1));
	grcg_off_clobbering_dx();
	verdict_stage_scores_put();
	verdict_bitmap_snap(1 * VERDICT_SCREEN_SIZE);
	grcg_setcolor(GC_RMW, 1);
	grcg_boxfill_8(0, 0, (RES_X - 1), (RES_Y - 1));
	graph_accesspage(1); grcg_boxfill_8(0, 0, (RES_X - 1), (RES_Y - 1));
	grcg_off_clobbering_dx();

	snd_load(staff_bgm, SND_LOAD_SONG);
	snd_kaja_func(KAJA_SONG_PLAY, 0);

	cdg_load_all_noalpha( 0, stf00_cdg);
	cdg_load_all_noalpha( 1, stf01_cdg);
	cdg_load_all_noalpha( 2, stf02_cdg);
	cdg_load_all_noalpha( 3, stf03_cdg);
	cdg_load_all_noalpha( 4, stf04_cdg);
	cdg_load_all_noalpha( 5, stf05_cdg);
	cdg_load_all_noalpha( 6, stf06_cdg);
	cdg_load_all_noalpha( 7, stf07_cdg);
	cdg_load_all_noalpha( 8, stf08_cdg);
	cdg_load_all_noalpha( 9, stf09_cdg);
	cdg_load_all_noalpha(10, stf10_cdg);
	super_entry_bfnt(stf01_bft);
	super_entry_bfnt(stf00_bft);
	for(i = 0; i < TINY_PATNUM_COUNT; i++) {
		super_convert_tiny(i);
	}
	Palettes[0].c.b = SPACE_BLUE;

	space_reset();
	i = 0;
	staffroll_frame = 0;
	do {
		if(i <= FADE_IN_FRAMES) {
			palette_settone(i);
		}
		staffroll_frame_and_flip();
		i++;
	} while(staffroll_measure_cur < MEASURE_ORB_GATHER);

	orb_gather_start();
	for(i = 0; i < ORB_GATHER_FRAMES; i++) {
		orb_gather_animate();
		staffroll_frame_and_flip();
	}
	orb_gather_end();

	// The orb phase's own script has to be done AND the BGM has to have
	// reached its own cue before the first credit line is shown.
	do {
		i = orb_phase_update();
		staffroll_frame_and_flip();
	} while(!i || (staffroll_measure_cur < MEASURE_FIRST_CREDIT));

	/// The orb phase's credit lines
	/// ----------------------------
	/// The first and third of these run two lines at once: the second line is
	/// only started once the first one has been on screen for [i] frames, and
	/// restarting that counter is also how a finished second line makes room
	/// for the next one.

	i = 0;
	credit_done = 0;
	do {
		orb_phase_update();
		if(i > 128) {
			if(credit_2_animate(528, 240, 1, 76)) {
				i = 0;
			}
		}
		credit_done = credit_animate(464, 192, 0, 76);
		staffroll_frame_and_flip();
		i++;
	} while(!credit_done);

	credit_done = 0;
	do {
		orb_phase_update();
		credit_done = credit_animate(464, 200, 2, 92);
		staffroll_frame_and_flip();
	} while(!credit_done);

	i = 0;
	credit_done = 0;
	do {
		orb_phase_update();
		if(i > 256) {
			if(credit_2_animate(464, 224, 4, 120)) {
				i = 0;
			}
		}
		credit_done = credit_animate(464, 176, 3, 120);
		staffroll_frame_and_flip();
		i++;
	} while(!credit_done);
	/// ----------------------------

	orb_burst();

	/// The rain phase's credit lines
	/// -----------------------------
	/// One at a time, at the same place, each held until the BGM reaches its
	/// own measure.

	do {
		i = rain_phase_update();
		staffroll_frame_and_flip();
	} while(!i);

	credit_done = 0;
	do {
		rain_phase_update();
		credit_done = credit_animate(176, 200, 5, 172);
		staffroll_frame_and_flip();
	} while(!credit_done);

	do {
		rain_phase_update();
		credit_done = credit_animate(176, 200, 6, 188);
		staffroll_frame_and_flip();
	} while(!credit_done);

	do {
		rain_phase_update();
		credit_done = credit_animate(176, 200, 7, 204);
		staffroll_frame_and_flip();
	} while(!credit_done);

	do {
		rain_phase_update();
		credit_done = credit_animate(176, 200, 8, 220);
		staffroll_frame_and_flip();
	} while(!credit_done);

	do {
		rain_phase_update();
		credit_done = credit_animate(176, 200, 9, 236);
		staffroll_frame_and_flip();
	} while(!credit_done);
	/// -----------------------------

	/// The last credit line, and the verdict screen behind it
	/// ------------------------------------------------------
	/// This one is given a [measure] the BGM can never report, so it stays on
	/// screen and this loop is left by the player rather than by the line.

	i = 0;
	verdict_bitmap_offset = 0;
	verdict_scroll = VERDICT_SCROLL_NONE;
	while(1) {
		input_reset_sense_held();

		// Twice each, because the scene is double-buffered.
		if((i == FRAME_VERDICT_STATS) || (i == (FRAME_VERDICT_STATS + 1))) {
			verdict_stats_put();
		} else if(
			(i == FRAME_VERDICT_COMMENT) || (i == (FRAME_VERDICT_COMMENT + 1))
		) {
			verdict_comment_put();
		} else if(i == FRAME_VERDICT_SNAP) {
			verdict_bitmap_snap(0);
		}

		rain_phase_update();
		credit_animate(176, 368, 10, CREDIT_MEASURE_HOLD);
		i++;
		if(i >= FRAME_INTERACTIVE) {
			if(key_det & INPUT_CANCEL) {
				staffroll_frame_and_flip();
				break;
			}
			if(key_det & INPUT_BOMB) {
				staffroll_frame_and_flip();
				break;
			}
			if(verdict_scroll == VERDICT_SCROLL_NONE) {
				if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
					if(verdict_bitmap_offset == 0) {
						verdict_scroll = VERDICT_SCROLL_DOWN;
					} else {
						// Already at the second screen, so there is nothing
						// left to show and this ends the staff roll instead.
						staffroll_frame_and_flip();
						break;
					}
				}
				if(key_det & INPUT_DOWN) {
					if(verdict_bitmap_offset == 0) {
						verdict_scroll = VERDICT_SCROLL_DOWN;
					}
				}
				if(key_det & INPUT_UP) {
					if(verdict_bitmap_offset == VERDICT_SCREEN_SIZE) {
						verdict_scroll = VERDICT_SCROLL_UP;
					}
				}
			}
			if(verdict_scroll == VERDICT_SCROLL_DOWN) {
				verdict_bitmap_offset += (
					VERDICT_SCROLL_H * VERDICT_BITMAP_VRAM_W
				);
				// `[measured]` The original compares this end of the travel
				// UNSIGNED and the other one signed, and only the cast
				// reproduces that: Turbo C++ folds [VERDICT_SCREEN_SIZE] into
				// a literal whose type is its value's, so the constant cannot
				// carry the unsignedness into the comparison however it is
				// declared. Both spellings agree on every value this can
				// hold — the offset is at most one scroll step past the end.
				if(
					static_cast<size_t>(verdict_bitmap_offset) >
					VERDICT_SCREEN_SIZE
				) {
					verdict_bitmap_offset = VERDICT_SCREEN_SIZE;
					verdict_scroll = VERDICT_SCROLL_NONE;
				}
				verdict_bitmap_put(verdict_bitmap_offset);
			} else if(verdict_scroll == VERDICT_SCROLL_UP) {
				verdict_bitmap_offset -= (
					VERDICT_SCROLL_H * VERDICT_BITMAP_VRAM_W
				);
				if(verdict_bitmap_offset < 0) {
					verdict_bitmap_offset = 0;
					verdict_scroll = VERDICT_SCROLL_NONE;
				}
				verdict_bitmap_put(verdict_bitmap_offset);
			}
		}
		staffroll_frame_and_flip();
	}
	/// ------------------------------------------------------

	snd_kaja_func(KAJA_SONG_FADE, 4);
	i = FADE_IN_FRAMES;
	do {
		palette_settone(i);
		rain_phase_update();
		staffroll_frame_and_flip();
		i--;
	} while(i > 0);

	cdg_free_all();
	super_free();
	grc_setclip(0, 0, (RES_X - 1), (RES_Y - 1));
}
