/* ReC98
 * -----
 * TH02's main_01___TEXT: the BGM title notification that occupies the bottom
 * text row for the first 160 frames after a new track starts, plus the two
 * per-frame stage callbacks ZUN compiled next to it.
 *
 * The functions appear here in the order the segment holds them, because that
 * is what a kb/codegen/0099 tail lift requires: this object grows backwards
 * into the hole each proc leaves, so the newest lift is always at the top.
 *
 * The segment's own trailing macro, egc_start_copy_1(), is what used to keep
 * every proc in it unliftable, since a C++ object can only ever append to a
 * root contribution. It now sits in DEMO_TEXT's block instead - same group,
 * same address, one segment boundary moved (kb/codegen/0148) - which is what
 * opened this chain in the first place.
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
#include "th02/resident.hpp"
#include "th02/main/frames.hpp"
#include "th02/main/null.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/stage/callback.hpp" // needs stage_progression_t, above

// All three still live in th02_main.asm's own _DATA contribution.
// th02/main/stage/init.cpp declares the first two exactly this way, and is
// what writes [bgm_title_id] whenever a new track starts.
extern "C" uint8_t bgm_show_timer;
extern "C" uint8_t bgm_title_id;
extern "C" shiftjis_t near *BGM_TITLES[];

// The boss track's title ID, staged by stage_init() and moved into
// [bgm_title_id] by boss_activate_if_scroll_done() below. Declared exactly the
// way th02/main/stage/init.cpp declares it.
extern "C" uint8_t boss_bgm_title_id;

// Still ASM in th02_main.asm, reached through the kb/codegen/0123 aliases it
// carries. Both are called once, from boss_activate_if_scroll_done() below.
// ---------------------------------------------------------------------------
// Retires every live stage enemy: each [enemies] slot whose flag is not F_FREE
// becomes F_REMOVE — not F_FREE, so the enemies' own update pass is what
// actually reclaims them — and [enemies_loop_bound] is zeroed, which stops
// enemies_update_and_render() dead on the same frame. Also reached, through a
// nopcall, from the Stage 3 boss's own still-ASM setup routine, the one that
// arms stone_flag[] and that stones_init() belongs to. `_all` mirrors
// shots_free_all() (th02/main/player/shot.cpp), the same shape for the shot
// array; the verb is F_REMOVE's own, because this pass does not free.
// [inferred from the ASM]
extern "C" void far enemies_remove_all(void);

// Disables the stage enemies for the rest of the fight, by pointing BOTH the
// [enemies_invalidate] and [enemies_update_and_render] slots at one shared
// empty far function. That function is a SECOND copy of nullfunc_void, which
// ZUN compiled into a different segment of the same binary — five identical
// bytes at a different address, spelled nullfunc_void_2 in th02_main.asm after
// upstream's own frame_delay() / frame_delay_2() precedent for a duplicated
// compiled body. The exact inverse of lasers_callbacks_set() in
// th02/main/laser.cpp, whose stem this name mirrors. [inferred from the ASM]
extern "C" void far enemies_callbacks_null(void);

// 48 half-width spaces -- exactly the playfield's TRAM width -- reached
// through the `public` line th02_main.asm carries for it (kb/codegen/0123).
// Both users are now in this file: stage_title_unput() blanks two whole rows
// with the entire run, bgm_show() only needs the last 26 cells of it. The
// alias stays regardless: it is what the C++ links against.
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

// The stage title that main_entry() writes across TRAM rows 12 and 13 the
// moment a stage starts. Both rows are blanked with the whole of [aEMPTY],
// which is 48 cells -- the full playfield width -- rather than the two rows'
// actual extent.
static const int STAGE_TITLE_TOP = 12;
static const int STAGE_TITLE_FRAMES = 160;

// The boss fight's own end condition, installed into [stage_should_end_func]
// by boss_activate_if_scroll_done() below and called by stage_loop() once per
// frame from then on. It answers "is the stage over?" and, on the one frame
// where the answer is yes, also performs the ending: [stage_progression]
// reaches SP_CLEAR only after boss_update() has returned it, so this is where
// the boss's own teardown callback runs. Reading the name as a pure predicate
// would be wrong.
//
// Stage 3 blacks out palette color 0 first, mirroring what
// boss_activate_if_scroll_done() does for the same stage on the way in.
// [inferred: the stage ID is ZUN's, the symmetry is ours]
extern "C" bool16 far stage_should_end(void)
{
	// The `return false` is the LAST statement, not a guard clause at the top.
	// Spelling it as a guard is the same 52 bytes and still fails: TCC then
	// emits `je` over a false-return block placed right after the test, where
	// the original has `jne` forward to one shared at the end. That counter-
	// shape is kb/codegen/0074's, measured again here.
	if(stage_progression == SP_CLEAR) {
		if(stage_id == 2) {
			palette_set(0, 0, 0, 0);
			palette_show();
		}
		boss_end();
		return true;
	}
	return false;
}

// stage_init() installs this into [boss_activate_if_scroll_done_func] at the
// start of every stage, and stage_loop() calls it once per frame until the map
// has finished scrolling. That single frame is where the entire boss fight is
// wired up: the stage moves to SP_BOSS, the boss's own per-frame callbacks
// replace the stage's, [stage_should_end_func] gets the function that will end
// the stage once the boss is done, and the slot nulls itself so the rest of
// the fight costs one far call per frame and nothing else.
extern "C" void far boss_activate_if_scroll_done(void)
{
	if(scroll_done != 1) {
		return;
	}
	stage_progression = SP_BOSS;

	// Two stages need something turned off before their boss appears. Stage 4
	// is the only one whose background effect keeps running into the fight if
	// left alone -- stage_init() only installs [stage_update_and_render] there
	// when [reduce_effects] is clear, and this is what takes it back out.
	// Stage 3 instead blacks out palette color 0, which stones_init() then
	// fades back in. [inferred: the stage IDs are ZUN's, the reason is ours]
	if(stage_id == 3) {
		stage_update_and_render = nullfunc_void;
	} else if(stage_id == 2) {
		palette_set(0, 0, 0, 0);
		palette_show();
	}

	boss_init();
	enemies_remove_all();
	enemies_callbacks_null();
	boss_activate_if_scroll_done_func = nullfunc_void;

	// The first two are a staged/live PAIR: stage_init() picked this stage's
	// boss into the `_func` half, and only this frame promotes it into the
	// half stage_loop() calls. The third is not that shape at all -- the
	// `_func` there only separates the slot from the function of the same
	// name, the way [stage_title_unput_func] does. One suffix, two meanings;
	// th02/main/stage/callback.hpp says which is which per slot.
	boss_bg_render = boss_bg_render_func;
	boss_update = boss_update_func;
	stage_should_end_func = stage_should_end;

	// 0xFF, so that stage_loop()'s `scroll_cycle++` wraps it to 0 on the very
	// next frame. [uint8_t]
	scroll_cycle = -1;

	// Announces the boss track, through the same notification bgm_show()
	// renders at the bottom of the playfield.
	bgm_show_timer = 1;
	bgm_title_id = boss_bgm_title_id;
}

// stage_loop() calls this through [stage_title_unput_func] once per frame.
// It does nothing at all until [stage_frame] reaches 160, then takes the title
// down and nulls its own slot, so the cost for the rest of the stage is one
// far call and one 32-bit compare.
//
// A demo keeps its title up: main_entry() writes the blinking `gDEMO_PLAY`
// gaiji to row 12 instead of a stage name when [demo_num] is set, and skipping
// the blanking here is what leaves it on screen for the whole demo. The slot
// is still nulled in that case, so the check happens exactly once either way.
extern "C" void far stage_title_unput(void)
{
	if(stage_frame != STAGE_TITLE_FRAMES) {
		return;
	}
	if(!resident->demo_num) {
		text_putsa(PLAYFIELD_TRAM_LEFT, STAGE_TITLE_TOP, aEMPTY, TX_WHITE);
		text_putsa(
			PLAYFIELD_TRAM_LEFT, (STAGE_TITLE_TOP + 1), aEMPTY, TX_WHITE
		);
	}
	stage_title_unput_func = nullfunc_void;
}

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
