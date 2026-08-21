/// Stage 3 Boss - Five Magic Stones
/// --------------------------------
/// Only the reset so far. It is the tail of th02_main.asm's contribution to
/// DIALOG_TEXT, and its object is NOT th02/dialog.cpp's: the original has a
/// byte of padding between the two, and C++ objects link byte-aligned, so that
/// byte is CONTENT and a contribution boundary is the only thing that explains
/// it. So this is its own translation unit, linked between the dump and
/// th02/dialog.cpp, and it carries the pad itself (kb/codegen/0161).

// -zC, because the segment name would otherwise come from this file's own
// basename (kb/codegen/0105). -G, because the prolog is `push bp; mov bp, sp`
// with no ENTER (kb/codegen/0011). No -a2: nothing here emits a generated
// jump table.
#pragma option -zCDIALOG_TEXT -zPmain_03 -G

#include "platform.h"
#include "pc98.h"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/explode.hpp"
#include "th02/main/laser.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/boss/b3.hpp"
#include "th01/sprites/pellet.h"

// Coordinates
// -----------

static const pixel_t STONE_W = 32;

extern "C" screen_x_t stone_left[STONE_COUNT];
extern "C" screen_y_t stone_top[STONE_COUNT];
// -----------

extern "C" uint8_t stone_hit_flash[STONE_COUNT];

// Still ASM, in th02_main.asm's own MAIN_03 block, and already published there.
// th02/main/bgm_show.cpp declares it the same way.
extern "C" void far enemies_remove_all(void);

// Still ASM, further up in DIALOG_TEXT. It fills four cells of
// [boss_rank_param] behind a two-way branch on [rank]; the function below is
// its only caller, and reaches it as a plain near call.
extern "C" void near stones_11997(void);

// The sprite each stone is currently blitted with. `[measured]` one array of
// STONE_COUNT words, not the four separate slots IDA saw: stones_116EC's
// render loop indexes it across the whole extent with `bx = i + i`, and
// stones_120F7 spells the same increment two ways - through the index for
// stones 0-3 and against the constant-index cell for stone 4.
// (kb/codegen/0123's 2-D case, and the test it prescribes.)
// Element 0 doubles as the Stage 3 MIDBOSS's sprite, which is the same storage
// sharing b3.hpp already documents for the flag and the damage.
extern "C" int16_t stone_patnum[STONE_COUNT];

// The fight's own 0-to-8 progression, one arm of stones_update()'s `if`/`else
// if` chain each. NOT the published [boss_phase], which is the shared
// alive/defeated flag and which nothing in this segment reads;
// [marisa_intro_step] is the precedent for a boss keeping its own counter
// beside it.
// `[measured]` all nine reads are `cmp` + `jnz`, so the signedness is a choice
// rather than a measurement.
extern "C" uint8_t stones_phase;

// The pattern index inside phases 4, 6 and 8, advanced by the pattern
// functions themselves - each one zeroes [boss_phase_frame] at the end of its
// cycle, and the phase arm turns that into an increment. `[measured]`
// genuinely unsigned: three of its reads are `cmp` + `jbe`. Phase 8 wraps it to
// 1 rather than 0, so that phase's first pattern runs once per fight, and
// phase 5 borrows the slot as a one-shot latch rather than as an index.
extern "C" uint8_t stones_pattern;

// The deadline that makes the fight survivable, incremented at the top of
// stones_update() and reset at every phase change. `[measured]` 1300 frames
// force-kills whichever of a pair is still alive in phases 1 and 2, and 2000
// force-kills the north stone in phase 8, which ends the fight. TH02 needs a
// second counter because [boss_phase_frame] is what the pattern functions
// wrap; TH05's b1.cpp reads its own `boss.phase_frame >= 1300` for the same
// job. ZUN bloat: 32 bits for a value that never exceeds 2000.
extern "C" int32_t stones_timeout_frame;

// ZUN bloat: incremented once per frame and zeroed at every phase and pattern
// change, and `[measured]` never read - no `cmp`, no load, no push, anywhere in
// the binary, and no neighbouring slot can reach it by index. It would have
// held "frames since this phase or pattern began", which is exactly what
// [stones_timeout_frame] ended up doing.
extern "C" int16_t stones_phase_frame_unused;

// The fixed muzzle the stones' pellets spawn from, and the VRAM row
// stones_1223E() eats the stage background from. All three are written once,
// here, and read nowhere else in this object; the dump's own hand names are
// kept, because an address-suffixed hand name is not an IDA placeholder.
extern "C" screen_x_t left_22D98;
extern "C" screen_y_t top_22D9A;
extern "C" vram_y_t y_22D9C;


// Resets every piece of Stage 3 boss state that survives a stage transition,
// and re-seeds the five stones' resting positions.
extern "C" void near stones_12778(void)
{
	register int i;

	boss_phase_frame = 0;
	for(i = 0; i < STONE_COUNT; i++) {
		stone_flag[i] = SF_DORMANT;
		stone_damage[i] = 0;
		stone_patnum[i] = 148;
		stone_hit_flash[i] = 0;
	}
	stone_left[STONE_INNER_WEST] = (PLAYFIELD_LEFT + 16 + (1 * 80));
	stone_top[STONE_INNER_WEST] = (PLAYFIELD_TOP + 16);
	stone_left[STONE_INNER_EAST] = (PLAYFIELD_LEFT + 16 + (3 * 80));
	stone_top[STONE_INNER_EAST] = (PLAYFIELD_TOP + 16);
	stone_left[STONE_OUTER_WEST] = (PLAYFIELD_LEFT + 16 + (0 * 80));
	stone_top[STONE_OUTER_WEST] = (PLAYFIELD_TOP + 32);
	stone_left[STONE_OUTER_EAST] = (PLAYFIELD_LEFT + 16 + (4 * 80));
	stone_top[STONE_OUTER_EAST] = (PLAYFIELD_TOP + 32);
	stone_left[STONE_NORTH] = (PLAYFIELD_LEFT + 16 + (2 * 80));
	stone_top[STONE_NORTH] = (PLAYFIELD_TOP + 16);
	top_22D9A = (PLAYFIELD_TOP + 24);
	left_22D98 = (PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - (PELLET_W / 2));

	// enemies_remove_all() is far and lands in this same physical segment, so
	// the original reaches it through the linker-relaxed `nop; push cs;
	// call near ptr` form that no plain C++ far call reproduces.
	// (kb/codegen/0083)
	__emit__(0x90);	// nop
	__emit__(0x0E);	// push cs
	_asm { call near ptr enemies_remove_all; }

	laser_wait_frames = 12;
	stones_phase_frame_unused = 0;
	stones_timeout_frame = 0;
	stones_phase = 0;
	stones_pattern = 0;
	boss_explode_angle_offset = 0x20;
	y_22D9C = 96;
	y_22D9C += scroll_line;
	if(y_22D9C >= RES_Y) {
		y_22D9C -= RES_Y;
	}
	stones_11997();
}

// The byte of padding between this object's contribution and
// th02/dialog.cpp's, which the dump used to carry as a `db 0` at the end of its
// own DIALOG_TEXT block. A file-scope codestring is emitted where it stands in
// source order, so this one lands after the function above and nowhere else.
// (kb/codegen/0161)
#pragma codestring "\x00"
