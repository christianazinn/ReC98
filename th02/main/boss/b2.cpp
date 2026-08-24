/// Stage 2 Boss - Meira
/// --------------------
/// The fight's [boss_init] and [boss_end] entry points. They are the last two
/// bodies of th02_main.asm's contribution to BOSS_5_TEXT, so the C++ that
/// replaces them has to be the first thing that contribution's successor emits
/// (kb/codegen/0099).
///
/// A NEW OBJECT rather than a prepend into th02/main/midboss/mx.cpp, which is
/// the successor the map names today. Two reasons, and the second is the one
/// that decides it: Meira is a stage boss and mx.cpp is the Extra Stage
/// midboss, so one file would be about two unrelated things; and mx.cpp carries
/// a scoped `-a2` for its pool struct, which is exactly the kind of neighbour
/// tools/pi-audit/carve_free_tails.py flags a PARITY RISK against for an
/// odd-length prepend. `[measured]` Every BOSS_5_TEXT contribution carries
/// ACBP=28 in obj/th02/main.map - BYTE alignment - so a new object exactly as
/// long as the bytes the root gives up leaves every later contribution at the
/// offset it had, and it costs one Tupfile.lua line between th02/dialog.cpp
/// and th02/main/midboss/mx.cpp.
///
/// Everything meira_update() dispatches to is still in the dump above, and so
/// is meira_update() itself, which is what the next parcel out of this block
/// takes. It carries a generated jump table with a one-byte pad, so THIS object
/// will need `-a2` when it grows to hold it, and that pad's parity will then be
/// a function of this object's own prefix (kb/codegen/0119 + 0160).

// -zC, because the segment name would otherwise come from this file's own
// basename and be B2_TEXT (kb/codegen/0105). -zPmain_03 for the near calls
// that leave this segment: every dialog_* entry point below is in DIALOG_TEXT
// and reachable near only because BOSS_5_TEXT is in the same group, which is
// also how th02_main.asm reached them from these very two procs. -G, because
// both prologs are `push bp; mov bp, sp` rather than an `enter`
// (kb/codegen/0011). No -a2: neither body here emits a generated jump table,
// and neither declares a struct whose stride an alignment could change
// (kb/codegen/0170).
#pragma option -zCBOSS_5_TEXT -zPmain_03 -G -a2

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/rank.h"
#include "th02/core/globals.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/hardware/input.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/explode.hpp"
#include "th02/main/spark.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/stage/bonus.hpp"
#include "th02/main/dialog/dialog.hpp"
#include "th02/main/hud/overlay.hpp"
#include "th02/main/tile/tile.hpp"
#include "th02/math/randring.hpp"
#include "th02/snd/snd.h"
#include "th02/sprites/main_pat.h"
#include "th02/v_colors.hpp"

// th02/main/dialog/dialog.cpp. dialog.hpp declares every dialog_script_*
// function but neither of these two, which is how th02/main/boss/b3.cpp, b4.cpp
// and b5.cpp already declare them.
void near dialog_pre(void);
void near dialog_post(void);

// th02/main/stage/init.cpp, which declares it identically - including the
// non-const parameter, which is why [aBoss4_m] below is not const either.
// `[measured]` Stops the current KAJA song, snd_load()s [fn] over it with
// SND_LOAD_SONG, and starts it again; every boss init function in this binary
// switches its BGM through it.
extern "C" void far sub_13ABB(char *fn);

// The Stage 2 boss BGM's file name, kept where th02_main.asm's own `_DATA`
// contribution defines it rather than re-emitted as a literal here: this
// translation unit contributes no initialized data at all, so a literal would
// land after the dump's whole `_DATA` block and shift every byte between.
// Declared exactly the way b3.cpp declares [aBoss2_m] and b4.cpp [aBoss3_m]
// for the same call one and two bosses later.
extern "C" char aBoss4_m[];

// The sprite the boss and midboss renderers blit, shared by all of them and
// written from ~150 sites across th02_main.asm. `patnum_2064E` is the dump's
// own spelling and is not an IDA placeholder; retiring the address suffix means
// ruling on all of those sites at once, which is its own parcel.
extern "C" int patnum_2064E;

// The five-byte rank-scaled parameter block every boss and midboss init
// function fills a contiguous prefix of, documented in full at
// th02/main/boss/b4.cpp and state/notes/th02-boss-rank-param.md. Meira uses
// three cells and all three of hers really are bullet groups: cell 1 goes
// straight to bullets_add_pellet() from meira_14BC2(), cell 2 from
// meira_14C76() and meira_14E30(), and cell 0 is copied into her own pattern
// state by meira_14A39().
extern "C" uint8_t boss_rank_param[5];

// th02/main/bullet/bullet.cpp, which owns the `[inferred]` licence for this
// name. NOT a per-boss progression counter - see [meira_phase] below - but the
// binary-wide "the boss on screen has been defeated" flag: 0 while Meira is
// fighting, and meira_update() switches to her defeat tail on the frame it
// turns 1.
extern "C" uint8_t boss_phase;

// The point shottype B's homing shots aim at, set by whichever boss is on
// screen. Declared here the way th02/main/boss/b4.cpp, th02/main/player/shot.cpp
// and th02/main/player/reset.cpp already declare them; no header owns them.
extern "C" int boss_pos_x;
extern "C" int boss_pos_y;

// The group of the bursts her patterns fire. Also declared in
// th02/main/boss/b2m.cpp, which holds three more of its writers.
extern "C" uint8_t meira_burst_group; // ACTUAL TYPE: bullet_group_or_special_motion_t

/// Meira's own fight state
/// -----------------------
/// All three are kb/codegen/0123 label aliases for now, because meira_update()
/// and meira_bg_render() are still ASM and hold references to every one of
/// them. They collapse into ordinary renames when this chain empties, exactly
/// the way Sigma's three did (state/notes/sigma_update.md).

// Which of her three pattern GROUPS she is in, advanced by meira_update() when
// [boss_damage] passes 700 and then 1500. NOT [boss_phase], which is the defeat
// flag above - the same distinction th02/main/boss/b3.cpp draws for
// [stones_phase], b5m.cpp for [mima_phase] and b6.cpp for [sigma_phase], and
// this is the fourth holder of that shape in this binary.
extern "C" uint8_t meira_phase;

// Which of the current group's patterns runs this frame; meira_update() cycles
// it and wraps it at four, three and two patterns respectively, and resets it
// to 0 at every group change. Same role as [sigma_pattern], [mima_pattern] and
// [marisa_pattern], and like them it is NOT [meira_phase].
extern "C" uint8_t meira_pattern;

// KEEPS ITS ADDRESS SUFFIX ON PURPOSE, which is the shape b4.cpp's
// [marisa_1AA60] already uses for a symbol whose name is a separate decision.
// `[measured]` It is a `dw` written only 0 and 1 - meira_14E9D() raises it and
// meira_init() below clears it - and read at exactly one site, where it gates
// meira_bg_render()'s pass over three per-page background slots. Naming it
// needs that renderer and that pattern, and neither is in this parcel; naming
// it from the gate alone would be a guess about what those three slots are.
extern "C" int16_t meira_250FE;
/// -----------------------


// Her sprite's extent, and it is derived rather than declared: meira_bg_render()
// invalidates a 64x64 rect at her top-left, and the dash below keeps her whole
// width inside the playfield by drawing its target from a range that is exactly
// PLAYFIELD_W minus this.
static const pixel_t MEIRA_W = 64;

/// Her afterimage trail
/// --------------------
/// Three slots of past positions per VRAM page, pushed at every 8th frame of the
/// teleport dash below and blitted as single-colour silhouettes. Only that one
/// pattern writes them; meira_bg_render() unputs the back page's three and then
/// copies the front page's over them, which is the same double-buffered history
/// every TH02 renderer keeps.
///
/// `[measured]` TWO PARALLEL ARRAYS and not an array of points: every walker
/// indexes them with `page * 6 + slot * 2` against two separate `offset`s, and a
/// point array would be one `offset` and a stride of 4.

static const int MEIRA_AFTERIMAGE_SLOTS = 3;

// The silhouette's sprite and its plane. `[measured]` The original's plane
// constant is 0xFFC1 - exactly one more than super_plane(V_WHITE), and
// super_plane() puts the ONE'S COMPLEMENT of the colour in its low nibble - so
// the colour index is 14. th02/v_colors.hpp names only V_WHITE, and naming 14
// needs a look at the loaded palette rather than at this code, so it stays the
// index it is.
static const main_patnum_t MEIRA_AFTERIMAGE_PATNUM = 147;
static const int MEIRA_AFTERIMAGE_COLOR = 14;

extern "C" screen_x_t near meira_afterimage_left[PAGE_COUNT][
	MEIRA_AFTERIMAGE_SLOTS
];
extern "C" screen_y_t near meira_afterimage_top[PAGE_COUNT][
	MEIRA_AFTERIMAGE_SLOTS
];
/// --------------------

/// Her teleport dash
/// -----------------
/// Where she is going, where she came from, and how far along she is. All five
/// are referenced only from the pattern that uses them, so all five are plain
/// renames rather than kb/codegen/0123 aliases.

// `[measured]` A FIXED STEP COUNT recomputed from the origin every frame, not an
// accumulated velocity - so the dash eases in: step 1 covers a 64th of the
// distance and step 64 covers all of it. The pattern gives it the 64 frames from
// 51 to 114 inclusive, so the budget and the step count match exactly and she
// always lands on her target.
static const int MEIRA_DASH_STEPS = 64;

extern "C" screen_x_t meira_dash_origin_x;
extern "C" screen_y_t meira_dash_origin_y;
extern "C" screen_x_t meira_dash_target_x;
extern "C" screen_y_t meira_dash_target_y;
extern "C" int meira_dash_step;
/// -----------------


// One burst of her second phase's third pattern, fired on three separate frames
// of it. `static` because meira_14E30() below is its only caller anywhere - the
// dump never published it either.
static void near meira_14DFC(void)
{
	snd_se_play(3);
	patnum_2064E = 143;
	bullets_add_pellet(
		(*boss_left_on_back_page + 24),
		(*boss_top_on_back_page + 32),
		0x00,
		boss_rank_param[2],
		((4 << 4) + 10)
	);
}


// Phase 1 pattern 2: a wind-up, then three bursts of the rank-scaled spread 9
// and 8 frames apart, then a reset. `[measured]` The sprite writes between the
// bursts are not no-ops - each burst sets 143 and every frame after it sets 142
// back - so she flickers between two frames for the whole pattern.
static void near meira_14E30(void)
{
	if(boss_phase_frame < 50) {
	} else if(boss_phase_frame == 50) {
		snd_se_play(9);
		patnum_2064E = 142;
		meira_burst_group = BG_1_RANDOM_ANGLE;
	} else if(boss_phase_frame < 99) {
	} else if(boss_phase_frame == 99) {
		meira_14DFC();
	} else if(boss_phase_frame < 108) {
		patnum_2064E = 142;
	} else if(boss_phase_frame == 108) {
		meira_14DFC();
	} else if(boss_phase_frame < 116) {
		patnum_2064E = 142;
	} else if(boss_phase_frame == 116) {
		meira_14DFC();
	} else {
		patnum_2064E = 141;
		boss_phase_frame = 0;
	}
}


// Phase 2 pattern 0, and the pattern that turns the afterimage trail ON: a
// wind-up that also widens every special motion's turn limit, then three
// bouncing billiard balls, then the reset that hands over to the dash below -
// which is then the only pattern that ever runs again (see meira_update()).
static void near meira_14E9D(void)
{
	register int i;

	if(boss_phase_frame < 50) {
	} else if(boss_phase_frame == 50) {
		snd_se_play(9);
		patnum_2064E = 142;

		// AND NOBODY EVER PUTS IT BACK. meira_init() sets it to 2 for the whole
		// fight and this raises it to 5 on the way into her last phase, so the
		// bullets it is about to fire outlive the pattern that fired them.
		bullet_special.u3.turns_max = 5;
	} else if(boss_phase_frame >= 99) {
		patnum_2064E = 146;
		for(i = 0; i < 3; i++) {
			bullets_add_16x16(
				(*boss_left_on_back_page + 24),
				(*boss_top_on_back_page + 24),
				randring2_next8(),
				BSM_BOUNCE_LEFT_RIGHT_TOP_BOTTOM,
				static_cast<main_patnum_t>(
					(i & 1) + PAT_BULLET16_BILLIARD_BALL_RED
				),
				((3 << 4) + 6)
			);
		}
		boss_phase_frame = 0;
		meira_250FE = 1;
	}
}


// Phase 2 pattern 1, and her last: a 64-step interpolated dash to a random point
// of the playfield trailing three afterimages, a landing, then two more bouncing
// billiard balls, then a restart. Nothing after this ever changes
// [meira_pattern] again, so this is the pattern she dies in.
static void near meira_14F16(void)
{
	int x;
	int y;
	screen_x_t near *afterimage_left;
	screen_y_t near *afterimage_top;
	register int i;

	if(boss_phase_frame < 50) {
		return;
	}
	afterimage_left = meira_afterimage_left[page_back];
	afterimage_top = meira_afterimage_top[page_back];

	// The trail's own clock, and it is NOT the dash's: the slots shift on every
	// 8th frame of the whole pattern, which includes the wind-up frame and the
	// 50 frames between the landing and the bullets.
	if((boss_phase_frame & 7) == 0) {
		afterimage_left[0] = afterimage_left[1];
		afterimage_top[0] = afterimage_top[1];
		afterimage_left[1] = afterimage_left[2];
		afterimage_top[1] = afterimage_top[2];
		afterimage_left[2] = *boss_left_on_back_page;
		afterimage_top[2] = *boss_top_on_back_page;
	}

	if(boss_phase_frame == 50) {
		// All three slots start on top of her, so the trail grows out of her
		// rather than appearing behind her.
		for(i = 0; i < MEIRA_AFTERIMAGE_SLOTS; i++) {
			afterimage_left[i] = *boss_left_on_back_page;
			afterimage_top[i] = *boss_top_on_back_page;
		}

		// `[measured]` The x range is PLAYFIELD_W minus her own width, so her
		// whole sprite lands inside the playfield. The y range is whatever one
		// byte of the ring gives, which is 256 of the playfield's 368 rows, so
		// she never dashes into the bottom third of it.
		meira_dash_target_x = (
			(randring2_next16() % (PLAYFIELD_W - MEIRA_W)) + PLAYFIELD_LEFT
		);
		meira_dash_target_y = (randring2_next8() + PLAYFIELD_TOP);
		meira_dash_origin_x = *boss_left_on_back_page;
		meira_dash_origin_y = *boss_top_on_back_page;
		meira_dash_step = 1;
	} else if(boss_phase_frame <= 114) {
		x = (
			((meira_dash_target_x - meira_dash_origin_x) * meira_dash_step) /
			MEIRA_DASH_STEPS
		);
		y = (
			((meira_dash_target_y - meira_dash_origin_y) * meira_dash_step) /
			MEIRA_DASH_STEPS
		);
		y += meira_dash_origin_y;
		*boss_left_on_back_page = (x + meira_dash_origin_x);
		*boss_top_on_back_page = y;
		meira_dash_step++;
	} else if(boss_phase_frame == 114) {
		// ZUN BUG: UNREACHABLE, and the branch above is why - it already
		// covers 114. So the landing sound effect never plays, and the arm is
		// still in the binary, which is why it is still here.
		snd_se_play(9);
	} else if(boss_phase_frame >= 164) {
		patnum_2064E = 146;
		for(i = 0; i < 2; i++) {
			bullets_add_16x16(
				(*boss_left_on_back_page + 24),
				(*boss_top_on_back_page + 24),
				randring2_next8(),
				BSM_BOUNCE_LEFT_RIGHT_TOP_BOTTOM,
				static_cast<main_patnum_t>(
					i + PAT_BULLET16_BILLIARD_BALL_RED
				),
				((3 << 4) + 6)
			);
		}
		boss_phase_frame = 0;
	} else {
		// The 49 frames between the landing and the bullets render nothing,
		// which is what freezes the trail in place while she stands still.
		return;
	}

	for(i = 0; i < MEIRA_AFTERIMAGE_SLOTS; i++) {
		// A slot that still holds her CURRENT position is not drawn, which is
		// what keeps the trail from stacking three silhouettes under her before
		// the dash starts. `[measured]` EITHER axis matching is enough, so a
		// purely horizontal or purely vertical dash draws no trail at all.
		//
		// NESTED `if`s AND NOT TWO `continue`s, which is kb/codegen/0140 read
		// in the direction its 2026-08-22 amendment describes: the original
		// re-uses the AX it loaded for the second comparison as the value it
		// assigns to [y], and a `continue` discards that tracked fact and
		// reloads. Both forms emit the same two `je`s to the same address,
		// because the guarded block is the last thing in the loop body.
		if(afterimage_left[i] != *boss_left_on_back_page) {
			if(afterimage_top[i] != *boss_top_on_back_page) {
				y = afterimage_top[i];
				y += scroll_line;
				if(y >= RES_Y) {
					y -= RES_Y;
				}
				super_roll_put_1plane(
					afterimage_left[i], y, MEIRA_AFTERIMAGE_PATNUM, 0,
					super_plane(MEIRA_AFTERIMAGE_COLOR)
				);
			}
		}
	}
}


/// What meira_update() dispatches to, and where each of them lives
/// ---------------------------------------------------------------
/// The nine remaining procs meira_update() reaches across an object boundary.
/// Every one takes no argument and ends in a bare `retn`; only the first
/// returns anything. The seven patterns keep the dump's address-suffixed hand
/// names, exactly the way b3.cpp's stones_1*() and b4.cpp's marisa_1AA60() do:
/// naming a pattern needs the pattern, and this object holds none of these.
///
/// The two at the bottom are NOT patterns. meira_update() calls them
/// unconditionally on every frame of the fight, past every phase branch, so
/// they are her movement and her render halves.

// Her defeat animation, and the only one with a result: two explosion rings, a
// spark burst and either her still sprite or a zoom of it, returning true on
// the frame the animation runs out. STILL ASM in th02_main.asm, and still a
// kb/codegen/0123 alias published for this object's sake.
extern "C" bool16 near meira_14519(void);

// These six are th02/main/boss/b2m.cpp, NOT this object - see the head of this
// file for the one pad byte that forced the split.
extern "C" void near meira_1483B(void);
extern "C" void near meira_148FD(void);
extern "C" void near meira_14A39(void);
extern "C" void near meira_14B33(void);
extern "C" void near meira_14BC2(void);
extern "C" void near meira_14C76(void);

// Her hittest-and-render half, still ASM and still an alias.
extern "C" void near meira_145E1(void);

// Her slash pool's per-frame pass, th02/main/boss/b2m.cpp. Was meira_14726,
// and the pool is what made it nameable: the 40 slots it walks are her dash
// slashes, lethal for as long as their trail lasts and then blooming into a
// burst of bullets.
extern "C" void near meira_slashes_update_and_render(void);
/// ---------------------------------------------------------------

// How far into her sprite the point shottype B's homing shots aim at sits.
// `[measured]` The same 40 on both axes, and her sprite is 64x64, so it really
// is her centre - unlike the Extra Stage midboss's pellet offsets, which are a
// quarter of a tile.
static const pixel_t MEIRA_CENTER_OFFSET = 40;

// The [boss_damage] each of her first two phases ends at. `[measured]` Both
// tests are `>`, and both are only reached on a frame that also ends a pattern,
// so she always finishes the pattern she is in.
static const int MEIRA_PHASE_0_DAMAGE_MAX = 700;
static const int MEIRA_PHASE_1_DAMAGE_MAX = 1500;

// The LAST pattern index of each of the first two phases, and the spelling is
// measured rather than chosen: the original wraps with a `jbe` against the last
// index, not a `jb` against the count, so a `>= COUNT` test is the same two
// bytes with the wrong branch sense. Phase 2 wraps at neither - see below.
// The first phase is a dense `switch` and compiles to a jump table; the other
// two are compare chains, which is what Turbo C++ emits below four cases
// (kb/codegen/0043).
static const uint8_t MEIRA_PHASE_0_PATTERN_MAX = 3;
static const uint8_t MEIRA_PHASE_1_PATTERN_MAX = 2;


// Her per-frame [boss_update] callback: the aim point, then one pattern of the
// current phase, then the phase and pattern bookkeeping, then her movement and
// her render. Returns SP_CLEAR on the frame her defeat animation finishes.
// Installed into [boss_update_func] by stage_init().
//
// `int` rather than [stage_progression_t], for the reason b3.cpp's
// stones_update() and b4.cpp's marisa_update() already record: the enum is
// byte-sized under -b-, but the original returns its value in the whole of AX.
extern "C" int far meira_update(void)
{
	register screen_y_t center_y;

	boss_phase_frame++;

	// TWO STATEMENTS, and the split is what keeps [center_y] in SI: written as
	// one expression, Turbo C++ accumulates in AX and adds a `mov si, ax`.
	// [boss_pos_x] below needs no such care - it has no register to stay in.
	center_y = *boss_top_on_back_page;
	center_y += MEIRA_CENTER_OFFSET;
	boss_pos_x = (*boss_left_on_back_page + MEIRA_CENTER_OFFSET);
	boss_pos_y = center_y;

	if(boss_phase != 0) {
		if(meira_14519()) {
			return SP_CLEAR;
		}
	} else {
		if(meira_phase == 0) {
			switch(meira_pattern) {
			case 0:	meira_1483B();	break;
			case 1:	meira_148FD();	break;
			case 2:	meira_14A39();	break;
			case 3:	meira_14B33();	break;
			}
			if(boss_phase_frame == 0) {
				meira_pattern++;
				if(meira_pattern > MEIRA_PHASE_0_PATTERN_MAX) {
					meira_pattern = 0;
				}

				// `[measured]` The extra condition is this phase's alone, and
				// it is a pattern number rather than a frame: she will not
				// leave phase 0 on the pattern that follows pattern 0, so the
				// phase change always happens after one of the other three.
				if(
					(boss_damage > MEIRA_PHASE_0_DAMAGE_MAX) &&
					(meira_pattern != 1)
				) {
					meira_phase++;
					meira_pattern = 0;
				}
			}
		} else if(meira_phase == 1) {
			switch(meira_pattern) {
			case 0:	meira_14BC2();	break;
			case 1:	meira_14C76();	break;
			case 2:	meira_14E30();	break;
			}
			if(boss_phase_frame == 0) {
				meira_pattern++;
				if(meira_pattern > MEIRA_PHASE_1_PATTERN_MAX) {
					meira_pattern = 0;
				}
				if(boss_damage > MEIRA_PHASE_1_DAMAGE_MAX) {
					meira_phase++;
					meira_pattern = 0;
				}
			}
		} else if(meira_phase == 2) {
			switch(meira_pattern) {
			case 0:	meira_14E9D();	break;
			case 1:	meira_14F16();	break;
			}

			// AND THE LAST PHASE NEVER WRAPS BACK TO 0: pattern 0 runs once,
			// and every pattern after it is pattern 1 forever. So her final
			// phase is an opening move followed by one attack on a loop, and
			// there is no phase 3 to escalate into.
			if(boss_phase_frame == 0) {
				meira_pattern = 1;
			}
		}
		meira_145E1();
	}
	meira_slashes_update_and_render();
	return SP_BOSS;
}


// Runs Meira's post-battle dialog and the stage clear bonus, then advances to
// Stage 3. Installed into [boss_end] by stage_init(); stones_end() in
// th02/main/boss/b3.cpp is the same function one stage later, and differs only
// in the key_delay() and the [spark_accel_x] reset below.
extern "C" void far meira_end(void)
{
	dialog_pre();
	dialog_script_generic_part_animate(DS_POSTBOSS);
	stage_clear_bonus_animate();
	key_delay();
	overlay_stage_leave_animate();
	stage_id++;

	// Her stage tilts the sparks sideways; Stage 3 gets them back upright.
	spark_accel_x.v = 0;
}


// Runs Meira's pre-battle dialog, flashes her onto both pages, switches to her
// BGM and resets the fight. Installed into [boss_init] by stage_init().
extern "C" void far meira_init(void)
{
	// The VRAM row her entrance frame is blitted at, which is not her screen y
	// - see below.
	register vram_y_t vram_y;

	patnum_2064E = 141;
	dialog_pre();
	dialog_script_stage2_pre_intro_animate();

	boss_left_on_page[0] = (PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 32);
	boss_left_on_page[1] = boss_left_on_page[0];
	boss_top_on_page[0] = (PLAYFIELD_TOP + 32);
	boss_top_on_page[1] = (PLAYFIELD_TOP + 32);
	boss_left_on_back_page = &boss_left_on_page[page_back];
	boss_top_on_back_page = &boss_top_on_page[page_back];

	palette_white_out(1);

	// Her y is a SCREEN y in [boss_top_on_page] and a VRAM ROW here, and the
	// wrap is the conversion: the tile layer scrolls under her, so the row she
	// has to be blitted at moves with [scroll_line] even though she does not.
	vram_y = (PLAYFIELD_TOP + 32);
	vram_y += scroll_line;
	if(vram_y >= RES_Y) {
		vram_y -= RES_Y;
	}
	super_roll_put(*boss_left_on_back_page, vram_y, patnum_2064E);

	// sub_13ABB() is far and lands in this same physical segment, so the
	// original reaches it through the linker-relaxed `nop; push cs; call near
	// ptr` form that no plain C++ far call reproduces (kb/codegen/0083). That
	// form cannot see the C++ expressions either, so its far pointer argument
	// and its __cdecl cleanup are hand-spelled with it.
	//
	// `[measured]` The cleanup is this call's own 4 bytes and nothing else -
	// every other call in this body either takes no argument or is `pascal` and
	// cleans itself - so the island does NOT have to reach backwards the way
	// marisa_init()'s does (kb/codegen/0083's addendum). Nothing in it names a
	// register, so [vram_y] stays in SI.
	__emit__(0x1E);	// push ds
	_asm { push offset aBoss4_m; }
	__emit__(0x90);	// nop
	__emit__(0x0E);	// push cs
	_asm { call near ptr sub_13ABB; }
	__emit__(0x83, 0xC4, 0x04);	// add sp, 4

	palette_white_in(1);
	dialog_script_generic_part_animate(DS_PREBOSS);
	dialog_post();

	boss_damage = 0;
	boss_phase = 0;

	// Her aimed spreads are the special motion this bounds, and 2 turns is the
	// lowest any boss in this binary sets it to.
	bullet_special.u3.turns_max = 2;

	boss_explode_angle_offset = 0;
	meira_phase = 0;
	meira_pattern = 0;
	meira_250FE = 0;

	// `[measured]` Cell 0 is rank-INVARIANT, so the difficulty of her fight is
	// carried entirely by the two spreads: five bullets wide and medium-aimed
	// above Easy, three of each on it.
	if(rank != RANK_EASY) {
		boss_rank_param[0] = BG_1_RANDOM_ANGLE;
		boss_rank_param[1] = BG_5_SPREAD_WIDE;
		boss_rank_param[2] = BG_5_SPREAD_MEDIUM_AIMED;
	} else {
		boss_rank_param[0] = BG_1_RANDOM_ANGLE;
		boss_rank_param[1] = BG_3_SPREAD_WIDE;
		boss_rank_param[2] = BG_3_SPREAD_MEDIUM_AIMED;
	}
}
