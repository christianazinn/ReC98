/// Stage 3 Boss - Five Magic Stones
/// --------------------------------
/// The fight's two [boss_init]/[boss_end] entry points, the reset they share,
/// the nine-phase [boss_update] callback between them, and the animation that
/// eats the stage background at each of the north stone's two take-overs.
/// Together they are the tail of th02_main.asm's contribution to DIALOG_TEXT,
/// and their object is NOT th02/dialog.cpp's: the original has a byte of
/// padding between the two, and C++ objects link byte-aligned, so that byte is
/// CONTENT and a contribution boundary is the only thing that explains it. So
/// this is its own translation unit, linked between the dump and
/// th02/dialog.cpp, and it carries the pad itself (kb/codegen/0161).
///
/// The earlier lifts were prepended in dump order, so everything
/// stones_update() dispatches to - the per-frame movement, the eleven bullet
/// patterns and the take-over animation - is now above it in this same file.

// -zC, because the segment name would otherwise come from this file's own
// basename (kb/codegen/0105). -G, because the prolog is `push bp; mov bp, sp`
// with no ENTER (kb/codegen/0011). No -a2: nothing here emits a generated
// jump table.
#pragma option -zCDIALOG_TEXT -zPmain_03 -G

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/egc.hpp"
#include "th01/rank.h"
#include "th02/core/globals.hpp"
#include "th02/math/randring.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/tile/tile.hpp"
#include "th02/main/explode.hpp"
#include "th02/main/spark.hpp"
#include "th02/main/score.hpp"
#include "th02/main/item/item.hpp"
#include "th02/main/laser.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/boss/b3.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/shot.hpp"
#include "th02/snd/snd.h"
#include "th02/main/stage/stage.hpp"
#include "th02/main/stage/bonus.hpp"
#include "th02/main/dialog/dialog.hpp"
#include "th02/main/hud/overlay.hpp"
#include "th02/v_colors.hpp"
#include "th02/sprites/main_pat.h"
#include "th01/sprites/pellet.h"

// Coordinates
// -----------

static const pixel_t STONE_W = 32;
static const pixel_t STONE_H = 32;

extern "C" screen_x_t stone_left[STONE_COUNT];
extern "C" screen_y_t stone_top[STONE_COUNT];
// -----------

extern "C" uint8_t stone_hit_flash[STONE_COUNT];

// th02/main/enemy/enemies.cpp, retaining C linkage because this is a different
// object. th02/main/bgm_show.cpp declares it the same way.
extern "C" void far enemies_remove_all(void);

// How far each stone is into its kill animation, and the one array in this
// fight the dump splits the way kb/codegen/0123's whole-extent case describes:
// stones_11766() indexes it with `[stone]` in its ordinary arm and reaches the
// north stone's cell by constant index in its own, which is why IDA saw
// `word_1EB1E` (four words) and `word_1EB26` (one) rather than five.
// `[measured]` `_midboss3_kill_frame` sits directly above it in `_DATA`, for
// exactly the same job one fight earlier.
extern "C" int16_t stone_kill_frame[STONE_COUNT];

// The four cells of the shared per-rank bullet parameters that stones_11997()
// fills. th02/main/boss/b4.cpp declares the extent identically; `[measured]`
// the dump's `group_20670` IS cell 2, published all along under this name.
extern "C" uint8_t boss_rank_param[5];

// The angles the two phase-6 patterns sweep, one each, both counting DOWN by 3
// and both restarting the phase once they pass -0x38 - one on `>`, the other
// on `>=`. The dump's own hand names are kept; `angle_` is not an IDA
// placeholder prefix.
extern "C" uint8_t angle_22FAF;
extern "C" uint8_t angle_22FB0;

// The angle the shared pattern-1 sweep counts UP by 8, in `_DATA` rather than
// `_BSS` because it is the only one of the three with an initial value - and
// the only one reset by the pattern that uses it rather than seeded by the
// frame it starts on.
extern "C" uint8_t angle_1EB28;

// One aimed angle per stone, recomputed every 64th frame of phase 6's last
// pattern. One array, not five slots: stones_1200F() indexes it with the same
// [i] that walks [stone_left], and reaches cell 4 by constant index in its
// other arm. (kb/codegen/0123's whole-extent case.)
extern "C" uint8_t angle_22FB1[STONE_COUNT];

// The west laser of the symmetric pair stones_11D30() marches across the
// playfield; the east one is always spawned at (STONES_LASER_MIRROR - this).
// `[measured]` The original encodes the mirror as a bare 432; the identity
// below is the reading of it, not evidence for it - a 16-pixel-wide laser at
// [left] has its mirror image at this distance.
static const screen_x_t STONES_LASER_MIRROR = (
	PLAYFIELD_LEFT + PLAYFIELD_RIGHT - 16
);
extern "C" screen_x_t stones_laser_left;

// What stops that march from turning round the instant it reaches the far
// side. `[measured]` 0 for the whole outward march; set to 1 the frame it
// ends and then incremented once per frame; and once it passes 0x24 the
// inward march runs and the slot stays above 0x24 for all of it, until the
// pattern ends and resets it. So it is 0/counting/done rather than a plain
// frame counter, and the `jbe` that reads it is unsigned.
extern "C" uint8_t stones_laser_return_delay;

// th02/main/boss/b4.cpp and b5m.cpp declare them identically. The point every
// bullet-spawning subsystem aims at, set to (-1, -1) while there is nothing to
// aim at.
extern "C" int boss_pos_x;
extern "C" int boss_pos_y;


/// Eating the stage background
/// ---------------------------
/// Both take-over phases run the same animation over the single row of
/// [tile_ring] tiles at [y_22D9C]: three passes across the playfield's
/// TILES_X columns, each pass replacing every column's tile with the next of
/// three images. `[measured]` The columns are taken in pairs from the two
/// edges inwards, one pair per odd frame, so a pass is 12 frames and the whole
/// animation 36 - after which every column has had the third image stamped and
/// the phase may advance.
///
/// The two passes differ only in their three images: 41/42/43 going in,
/// 42/41/40 coming back out.
/// ---------------------------

// Per column, which of the three images is still owed to it: 0 for none, 1, 2
// or 3 for [tile_image_22FD2], [tile_image_22FD4] and [tile_image_22FD6].
// stones_1223E() below both blits and clears them.
extern "C" uint8_t stones_tile_pending[TILES_X];

// Which of the three passes is running, 0 to 2. It is what decides the value
// written into [stones_tile_pending], one higher than itself.
extern "C" uint8_t stones_tile_pass;

// How many column PAIRS of the current pass have been queued, 0 to 11. Also
// the index into the two tables below.
extern "C" uint8_t stones_tile_pair;

// How many columns have had the third image stamped. The animation is over at
// TILES_X, and unlike the two above it is never reset between passes.
extern "C" uint8_t stones_tile_cols_done;

// The three tile images of the pass currently running. The dump's own hand
// names are kept: an address-suffixed hand name is not an IDA placeholder, and
// these three are only ever the pass's first, second and third.
extern "C" int tile_image_22FD2;
extern "C" int tile_image_22FD4;
extern "C" int tile_image_22FD6;

// The column each pair covers, from the west edge and from the east one.
// `[measured]` 0…11 and 23…12, i.e. exactly `i` and `(TILES_X - 1 - i)`,
// written out as data rather than computed.
static const int STONES_TILE_COL_PAIRS = (TILES_X / 2);
extern "C" const uint8_t STONES_TILE_COLS_WEST[STONES_TILE_COL_PAIRS];
extern "C" const uint8_t STONES_TILE_COLS_EAST[STONES_TILE_COL_PAIRS];

// th02/main/dialog/dialog.cpp. dialog.hpp declares every dialog_script_*
// function but neither of these two, which is how th02/main/boss/b4.cpp and
// b5.cpp already declare them.
void near dialog_pre(void);
void near dialog_post(void);

// th02/main/stage/init.cpp, which declares it identically - including the
// non-const parameter, which is why [aBoss2_m] below is not const either.
// `[measured]` Stops the current KAJA song, snd_load()s [fn] over it with
// SND_LOAD_SONG, and starts it again; every boss init function in this binary
// switches its BGM through it.
extern "C" void far boss_bgm_load(char *fn);

// The Stage 3 boss BGM's file name, kept where th02_main.asm's own `_DATA`
// contribution defines it rather than re-emitted as a literal here: this
// translation unit contributes no initialized data at all today, so a literal
// would land after the dump's whole block and shift every byte between.
// Declared exactly the way th02/main/boss/b4.cpp already declares [aBoss3_m]
// for the same call one boss later.
extern "C" char aBoss2_m[];

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


// Defined below, and reached from stones_init() as the plain 3-byte near call
// the original encodes - both are in this one object now.
extern "C" void near stones_12778(void);


// Marks the tiles under every stone that is still on screen for redrawing,
// and retires the ones that were flagged for removal on the previous frame -
// after that final unblit, exactly the way lasers_invalidate() does. Installed
// into [boss_bg_render] by stage_init().
extern "C" void far stones_bg_render(void)
{
	register int i;

	for(i = 0; i < STONE_COUNT; i++) {
		if(stone_flag[i] < SF_REMOVED) {
			tiles_invalidate_rect(
				stone_left[i], stone_top[i], STONE_W, STONE_H
			);
			if(stone_flag[i] == SF_REMOVE) {
				stone_flag[i] = SF_REMOVED;
			}
		}
	}
}


// Blits every stone that is not being killed, at its scroll-wrapped row.
extern "C" void near stones_116EC(void)
{
	register int i;
	register vram_y_t y;

	for(i = 0; i < STONE_COUNT; i++) {
		if(stone_flag[i] < SF_KILL_ANIM) {
			// One statement per instruction: the compound expression goes
			// through AX and costs a `mov di, ax` on top.
			y = stone_top[i];
			y += scroll_line;
			if(y >= RES_Y) {
				y -= RES_Y;
			}
			if(stone_hit_flash[i] != 0) {
				snd_se_play(4);
				super_roll_put_1plane(
					stone_left[i], y, stone_patnum[i], 0, super_plane(V_WHITE)
				);
				stone_hit_flash[i] = 0;
			} else {
				super_roll_put(stone_left[i], y, stone_patnum[i]);
			}
		}
	}
}


// One frame of one stone's kill animation: sparks every frame, a sprite that
// advances every 8th, and - for the north stone only - the boss explosion ring
// on top. Returns `true` on the frame the stone reaches SF_REMOVE, which is
// what stones_update() counts towards the end of the fight.
extern "C" bool16 pascal near stones_11766(int stone)
{
	int patnum;
	int kill_frames;
	vram_y_t y;

	kill_frames = 0x40;
	sparks_add(
		(stone_left[stone] + 16), (stone_top[stone] + 12), (5 << 4), 2, false
	);
	patnum = 10;
	y = stone_top[stone];
	y += scroll_line;
	if(y >= RES_Y) {
		y -= RES_Y;
	}
	if(stone == STONE_NORTH) {
		kill_frames += 0x20;
		boss_explode_render(
			(stone_left[STONE_NORTH] + 8),
			(stone_top[STONE_NORTH] + 8),
			stone_kill_frame[STONE_NORTH]
		);

		// A second ring, 0x18 frames behind the first, once there is one.
		if(stone_kill_frame[STONE_NORTH] >= 0x18) {
			boss_explode_render(
				(stone_left[STONE_NORTH] + 8),
				(stone_top[STONE_NORTH] + 8),
				(stone_kill_frame[STONE_NORTH] - 0x18)
			);
		}
		if(stone_kill_frame[STONE_NORTH] < 0x20) {
			super_roll_put(
				stone_left[STONE_NORTH], y, stone_patnum[STONE_NORTH]
			);
			stone_kill_frame[STONE_NORTH]++;
			return false;
		}
		if(stone_kill_frame[STONE_NORTH] == 0x38) {
			boss_explode_angle_offset = 0xE0;
		}
		patnum += ((stone_kill_frame[STONE_NORTH] - 0x20) / 8);
	} else {
		patnum += (stone_kill_frame[stone] / 8);
	}
	stone_kill_frame[stone]++;
	if(stone_kill_frame[stone] >= kill_frames) {
		stone_kill_frame[stone] = 0;
		stone_flag[stone] = SF_REMOVE;
		return true;
	}
	super_put(stone_left[stone], y, patnum);
	return false;
}


// The fight's per-frame housekeeping: the player's shots against every stone,
// the aim point for everything that spawns bullets, and the one pattern that
// runs regardless of phase.
extern "C" void near stones_11877(void)
{
	register int i;
	register int damage;

	stones_phase_frame_unused++;
	for(i = 0; i < STONE_COUNT; i++) {
		if(stone_flag[i] <= SF_ACTIVE) {
			// Assigned and tested in one expression: two statements make the
			// test read DI back, where the original tests the AX the call
			// returned in. (kb/codegen/0143, and b4.cpp says the same.)
			if((damage = shots_hittest(
				stone_left[i], stone_top[i], STONE_W, 40
			)) != 0 && (stone_flag[i] == SF_ACTIVE)) {
				stone_hit_flash[i] = 1;
				stone_damage[i] += damage;

				// The north stone takes more than the other four put
				// together, and the inner pair a little more than the outer.
				if(
					stone_damage[i] >= ((i == STONE_NORTH)
						? 930
						: ((i <= STONE_INNER_EAST) ? 140 : 120)
					)
				) {
					stone_flag[i] = SF_KILL_ANIM;
					score_delta += 30000;
					if(i <= STONE_OUTER_EAST) {
						items_add(
							(stone_left[i] + 8),
							(stone_top[i] + 12),
							// Every arm is cast, because [item_type_t] is
							// byte-sized and the original loads the whole of AX.
							((i == STONE_INNER_WEST)
								? static_cast<int>(IT_BOMB)
								: ((i <= IT_BOMB)
									? static_cast<int>(IT_BIGPOWER)
									: static_cast<int>(IT_BOMB)
								)
							)
						);
						snd_se_play(2);
					} else {
						// ZUN quirk: the item the north stone would drop is
						// replaced by the invincibility every boss defeat
						// grants, so the fight's last kill drops nothing.
						player_invincibility_time =
							BOSS_DEFEAT_INVINCIBILITY_FRAMES;
					}
				}
			}
		}
	}

	// Everything aims at the first stone still alive, in index order.
	for(i = 0; i < STONE_COUNT; i++) {
		if(stone_flag[i] == SF_ACTIVE) {
			boss_pos_x = (stone_left[i] + 8);
			boss_pos_y = (stone_top[i] + 8);
			break;
		}
	}

	// A punishment shot for standing in the top half of the playfield.
	if(player_topleft.y < (PLAYFIELD_TOP + 80)) {
		bullets_add_pellet(
			left_22D98,
			top_22D9A,
			(randring2_next8_and(0x0F) - 7),
			BG_1_AIMED,
			((4 << 4) + 13)
		);
	}
}


// Seeds the four [boss_rank_param] cells the fight uses, once per fight, from
// stones_12778(). `[measured]` all four are bullet groups except cell 1, which
// stones_11DF6() uses as the index its loop STARTS at, so Easy fires from two
// fewer stones.
extern "C" void near stones_11997(void)
{
	if(rank != RANK_EASY) {
		boss_rank_param[0] = BG_5_SPREAD_WIDE_AIMED;
		boss_rank_param[1] = 0;
		boss_rank_param[2] = BG_8_RING;
		boss_rank_param[3] = BG_4_RING;
	} else {
		boss_rank_param[0] = BG_3_SPREAD_WIDE_AIMED;
		boss_rank_param[1] = 2;
		boss_rank_param[2] = BG_4_RING;
		boss_rank_param[3] = BG_2_RING;
	}
}


// Phase 1's per-frame pattern: a rank-dependent spread from each of the two
// inner stones every 24th frame for the first 150 frames, then an aimed 16×16
// from each of the four outer ones every 16th frame, then a wait, then a
// restart.
extern "C" void near stones_119CD(void)
{
	register int i;

	if(boss_phase_frame < 30) {
		return;
	}
	if(boss_phase_frame < 180) {
		if((boss_phase_frame % 24) == 0) {
			for(i = 0; i < 2; i++) {
				if(stone_flag[i] == SF_ACTIVE) {
					bullets_add_pellet(
						(stone_left[i] + 12),
						(stone_top[i] + 8),
						0,
						boss_rank_param[0],
						((2 << 4) + 6)
					);
				}
			}
		}
	} else if(boss_phase_frame >= 200) {
		if(boss_phase_frame < 310) {
			if((boss_phase_frame & 0x0F) == 0) {
				for(i = 0; i < STONE_OUTER_WEST; i++) {
					if(stone_flag[i] == SF_ACTIVE) {
						bullets_add_16x16(
							(stone_left[i] + 12),
							(stone_top[i] + 8),
							0,
							BG_1_AIMED,
							PAT_BULLET16_OUTLINED_BALL_RED,
							(5 << 4)
						);
					}
				}
			}
		} else if(boss_phase_frame > 350) {
			boss_phase_frame = 0;
		}
	}
}


// Phase 2's per-frame pattern: a six-way decelerating ring from each of the
// two outer stones. The interval shortens by 20 frames for every stone already
// removed, and drops from 110 to 90 frames at the 200-frame mark, so the fight
// speeds up both with progress and with time.
extern "C" void pascal near stones_11A87(int removed)
{
	register int i;
	register int stone;

	if(boss_phase_frame < 30) {
		return;
	}
	if(boss_phase_frame < 200) {
		if((boss_phase_frame % (110 - (removed * 20))) == 0) {
			for(stone = STONE_OUTER_WEST; stone < STONE_NORTH; stone++) {
				if(stone_flag[stone] == SF_ACTIVE) {
					for(i = 0; i < 6; i++) {
						bullets_add_16x16(
							(stone_left[stone] + 12),
							(stone_top[stone] + 8),
							(i * 0x2A),
							BSM_DECELERATE_THEN_TURN_AIMED,
							PAT_BULLET16_OUTLINED_BALL_GREEN,
							((3 << 4) + 8)
						);
					}
				}
			}
		}
	} else if((boss_phase_frame % (90 - (removed * 20))) == 0) {
		for(stone = STONE_OUTER_WEST; stone < STONE_NORTH; stone++) {
			if(stone_flag[stone] == SF_ACTIVE) {
				for(i = 0; i < 6; i++) {
					bullets_add_16x16(
						(stone_left[stone] + 12),
						(stone_top[stone] + 8),
						(i * 0x2A),
						BSM_DECELERATE_THEN_TURN_AIMED,
						PAT_BULLET16_OUTLINED_BALL_GREEN,
						((3 << 4) + 8)
					);
				}
			}
		}
	}
}


/// Phases 4 and 8's bullet patterns
/// --------------------------------
/// Six [stones_pattern] slots each. Slots 1, 3, 4 and 5 are the same function
/// in both phases; slots 0 and 2 differ, and the four below are those two
/// pairs. Each pattern zeroes [boss_phase_frame] when its cycle ends, which is
/// what advances the index.
/// --------------------------------

// Phase 4 slot 0: two rows of four fixed lasers, 30 frames apart.
extern "C" void near stones_11B5D(void)
{
	if(boss_phase_frame < 50) {
		return;
	}
	if(boss_phase_frame == 50) {
		laser_wait_frames = 36;
		lasers_add(48, 96, 16, 103);
		lasers_add(192, 96, 16, 103);
		lasers_add(240, 96, 16, 103);
		lasers_add(384, 96, 16, 103);
		return;
	}
	if(boss_phase_frame == 80) {
		lasers_add(80, 96, 16, 103);
		lasers_add(160, 96, 16, 103);
		lasers_add(272, 96, 16, 103);
		lasers_add(352, 96, 16, 103);
		boss_phase_frame = 0;
	}
}


// Slot 1 in both phases: a horizontally symmetric pellet pair every 4th frame,
// sweeping through a half-turn 8 angle units at a time.
extern "C" void near stones_11BFE(void)
{
	if((boss_phase_frame & 3) == 0) {
		bullets_add_pellet(
			left_22D98,
			top_22D9A,
			angle_1EB28,
			BG_2_SPREAD_HORIZONTALLY_SYMMETRIC,
			((4 << 4) + 6)
		);
		angle_1EB28 += 8;
		if(angle_1EB28 > 0x82) {
			boss_phase_frame = 0;
			angle_1EB28 = 0;
		}
	}
}


// Phase 4 slot 2: a laser straight down the player's own tile column, at an
// interval that shortens with [rank] - every 28 frames on Easy, every 16 on
// Lunatic.
extern "C" void near stones_11C37(void)
{
	screen_x_t left;

	if(boss_phase_frame < 24) {
		return;
	}
	if((boss_phase_frame % (28 - (rank * 3))) == 0) {
		laser_wait_frames = 30;
		left = ((player_topleft.x + (PLAYER_W / 2)) & 0xFFF0);
		lasers_add(left, 96, 16, 103);
	}
	if(boss_phase_frame >= 140) {
		boss_phase_frame = 0;
	}
}


// Phase 8 slot 2: a pellet pair straddling the aim by a spread that is random
// for the first 40 frames and then narrows with the frame counter, so the
// volley starts as a scatter and converges into an aimed stream.
extern "C" void near stones_11C8A(void)
{
	unsigned char angle_1;
	unsigned char angle_2;

	if(boss_phase_frame < 40) {
		if((boss_phase_frame % 6) == 0) {
			angle_1 = iatan2(
				(player_topleft.y - 32),
				(player_topleft.x - stone_left[STONE_NORTH])
			);
			angle_2 = (randring2_next8_and(0x1F) + 0x10);
			bullets_add_pellet(
				left_22D98, top_22D9A, (angle_1 + angle_2), BG_1, (5 << 4)
			);
			bullets_add_pellet(
				left_22D98, top_22D9A, (angle_1 - angle_2), BG_1, (5 << 4)
			);
		}
	} else if(boss_phase_frame < 102) {
		if((boss_phase_frame % 6) == 0) {
			angle_1 = iatan2(
				(player_topleft.y - 32),
				(player_topleft.x - stone_left[STONE_NORTH])
			);
			angle_2 = (0x5F - boss_phase_frame);
			bullets_add_pellet(
				left_22D98, top_22D9A, (angle_1 + angle_2), BG_1, (5 << 4)
			);
			bullets_add_pellet(
				left_22D98, top_22D9A, (angle_1 - angle_2), BG_1, (5 << 4)
			);
		}
	} else {
		boss_phase_frame = 0;
	}
}


/// The four both phases share
/// --------------------------

// The east/west laser wall: a symmetric pair every 8th frame, marching out to
// the playfield edges, pausing, and marching back.
extern "C" void near stones_11D30(void)
{
	if(boss_phase_frame < 16) {
		return;
	}
	if(boss_phase_frame == 16) {
		stones_laser_left = 32;
		stones_laser_return_delay = 0;
	}
	if((stones_laser_return_delay == 0) && ((boss_phase_frame & 7) == 0)) {
		laser_wait_frames = 16;
		lasers_add(stones_laser_left, 96, 3, 111);
		lasers_add((STONES_LASER_MIRROR - stones_laser_left), 96, 3, 111);
		stones_laser_left += 16;
		if(stones_laser_left > 176) {
			stones_laser_return_delay = 1;

			// ZUN quirk: the turn-round frame advances the march one extra
			// step, so the pair comes back from 16 pixels further out than it
			// ever fired at.
			stones_laser_left += 16;
		}
	}
	if(stones_laser_return_delay > 0x24) {
		if((boss_phase_frame & 7) == 0) {
			lasers_add(stones_laser_left, 96, 1, 111);
			lasers_add((STONES_LASER_MIRROR - stones_laser_left), 96, 1, 111);
			stones_laser_left -= 16;
			if(stones_laser_left < 64) {
				stones_laser_return_delay = 0;
				boss_phase_frame = 0;
				stones_laser_left = 32;
			}
		}
	} else if(stones_laser_return_delay != 0) {
		stones_laser_return_delay++;
	}
}


// A gravity bullet from each of the outer stones every 16th frame - from as
// many of them as [rank] allows, since the loop STARTS at a rank parameter.
extern "C" void near stones_11DF6(void)
{
	register int i;

	if((boss_phase_frame & 0x0F) == 0) {
		for(i = boss_rank_param[1]; i < STONE_NORTH; i++) {
			bullets_add_16x16(
				stone_left[i],
				stone_top[i],
				randring2_next8(),
				BSM_GRAVITY,
				PAT_BULLET16_OUTLINED_BALL_GREEN,
				(2 << 4)
			);
		}
	}
	if(boss_phase_frame > 110) {
		boss_phase_frame = 0;
	}
}


// A random-angle pellet from the fixed muzzle every 4th frame, at a speed that
// climbs with the frame counter, so the volley accelerates as it goes.
extern "C" void near stones_11E40(void)
{
	if((boss_phase_frame & 3) == 0) {
		bullets_add_pellet(
			left_22D98,
			top_22D9A,
			randring2_next8(),
			boss_rank_param[3],
			((boss_phase_frame >> 1) + ((1 << 4) + 14))
		);
	}
	if(boss_phase_frame > 130) {
		boss_phase_frame = 0;
	}
}


// Lasers at random tile columns: one every 36th frame for the first stretch,
// with an aimed pellet every 40th on anything above Easy, then three at once
// on frame 100.
extern "C" void near stones_11E76(void)
{
	register int i;
	register int col;

	if(boss_phase_frame < 16) {
		return;
	}
	if(boss_phase_frame < 80) {
		if((boss_phase_frame % 36) == 0) {
			laser_wait_frames = 30;
			col = ((randring2_next8() % 24) + 2);
			lasers_add((col << 4), 96, 12, 111);
		}
		if(((boss_phase_frame % 40) == 0) && (rank != RANK_EASY)) {
			bullets_add_pellet(
				left_22D98, top_22D9A, 0, BG_1_AIMED, ((4 << 4) + 6)
			);
		}
	} else if(boss_phase_frame < 120) {
		if(boss_phase_frame == 100) {
			laser_wait_frames = 30;
			for(i = 0; i < 3; i++) {
				col = ((randring2_next8() % 24) + 2);
				lasers_add((col << 4), 96, 12, 111);
			}
		}
	} else {
		boss_phase_frame = 0;
	}
}


/// Phase 6's three bullet patterns
/// -------------------------------
/// [stones_pattern] 0 and 2 both run the first of these, 1 the second and 3
/// the third; stones_update() advances the index whenever one of them zeroes
/// [boss_phase_frame], which is how each pattern decides its own length.
/// -------------------------------

// A mirrored pair of aimed 16×16 bullets from each of the four outer stones,
// every 30th frame, on the same downward sweep.
extern "C" void near stones_11F2F(void)
{
	register int i;

	if(boss_phase_frame < 16) {
		return;
	}
	if(boss_phase_frame == 16) {
		angle_22FAF = 0x29;
	}
	if((boss_phase_frame % 30) == 0) {
		for(i = 0; i < STONE_NORTH; i++) {
			bullets_add_16x16(
				stone_left[i],
				stone_top[i],
				angle_22FAF,
				BG_1_AIMED,
				PAT_BULLET16_OUTLINED_BALL_GREEN,
				((3 << 4) + 12)
			);
			bullets_add_16x16(
				stone_left[i],
				stone_top[i],
				-angle_22FAF,
				BG_1_AIMED,
				PAT_BULLET16_OUTLINED_BALL_GREEN,
				((3 << 4) + 12)
			);
		}
		angle_22FAF -= 3;
	}
	if(angle_22FAF >= 0xC8) {
		boss_phase_frame = 0;
	}
}


// A pair of pellets from the fixed muzzle every 8th frame, at mirrored angles
// that sweep together by 3 per shot until they pass -0x38.
extern "C" void near stones_11FB5(void)
{
	if(boss_phase_frame == 2) {
		angle_22FB0 = 0x3E;
	}
	if((boss_phase_frame & 7) == 0) {
		bullets_add_pellet(
			left_22D98,
			top_22D9A,
			(0 - angle_22FB0),
			boss_rank_param[2],
			(4 << 4)
		);
		bullets_add_pellet(
			left_22D98, top_22D9A, angle_22FB0, boss_rank_param[2], (4 << 4)
		);
		angle_22FB0 -= 3;

		// ZUN quirk: `>` here, `>=` in stones_11F2F() below, for the same
		// sweep against the same bound.
		if(angle_22FB0 > 0xC8) {
			boss_phase_frame = 0;
		}
	}
}


// The long one: every 64th frame all five stones re-aim at the player, and
// every 4th frame after that they fire - pellets for the first six of the nine
// volleys in a cycle, then a stack-multiplier-free 4-ring of 16×16 bullets
// from frame 384 on, at the north stone's angle for all five.
extern "C" void near stones_1200F(void)
{
	register int i;

	if(boss_phase_frame < 60) {
		return;
	}
	if((boss_phase_frame % 64) == 0) {
		for(i = 0; i < STONE_COUNT; i++) {
			angle_22FB1[i] = iatan2(
				(player_topleft.y - 32), (player_topleft.x - stone_left[i])
			);
		}
	}
	if((boss_phase_frame & 3) == 0) {
		if(boss_phase_frame < 384) {
			if(((boss_phase_frame % 64) >> 2) <= 8) {
				for(i = 0; i < STONE_COUNT; i++) {
					bullets_add_pellet(
						(stone_left[i] + 12),
						(stone_top[i] + 12),
						angle_22FB1[i],
						BG_1,
						(7 << 4)
					);
				}
				snd_se_play(3);
			}
		} else {
			bullets_set_stack_multiplier(0);
			for(i = 0; i < STONE_COUNT; i++) {
				bullets_add_16x16(
					(stone_left[i] + 8),
					(stone_top[i] + 8),
					angle_22FB1[STONE_NORTH],
					BG_4_RING,
					PAT_BULLET16_BALL,
					(7 << 4)
				);
			}
			snd_se_play(3);
		}
	}
	if(boss_phase_frame >= 447) {
		boss_phase_frame = 0;
		bullets_set_stack_multiplier(1);
	}
}


// One frame of a stone's take-over animation: advances its sprite towards the
// taken-over cel and returns `true` on the frame it gets there, which is what
// lets the phase move on. The inner pair, the outer pair and the north stone
// each walk a different range - and the north stone's arm reaches its cells by
// constant index where the other two use [stone], which is the evidence
// kb/codegen/0123 cites for [stone_patnum] being one array.
extern "C" bool16 pascal near stones_120F7(int stone)
{
	if((stone == STONE_INNER_WEST) || (stone == STONE_INNER_EAST)) {
		stone_patnum[stone]++;
		if(stone_patnum[stone] >= 155) {
			stone_flag[stone] = SF_ACTIVE;
			return true;
		}
	} else if((stone == STONE_OUTER_WEST) || (stone == STONE_OUTER_EAST)) {
		if(stone_patnum[stone] < 152) {
			stone_patnum[stone]++;
		}
		if(stone_patnum[stone] == 152) {
			stone_patnum[stone] = 156;
		} else {
			stone_patnum[stone]++;
		}
		if(stone_patnum[stone] >= 159) {
			stone_flag[stone] = SF_ACTIVE;
			return true;
		}
	} else if(stone == STONE_NORTH) {
		if(stone_patnum[STONE_NORTH] < 152) {
			stone_patnum[STONE_NORTH]++;
		}
		if(stone_patnum[STONE_NORTH] == 152) {
			stone_patnum[STONE_NORTH] = 160;
		} else {
			stone_patnum[STONE_NORTH]++;
		}
		if(stone_patnum[STONE_NORTH] >= 163) {
			stone_flag[STONE_NORTH] = SF_ACTIVE;
			return true;
		}
	}
	return false;
}


// Phase 5's per-frame countdown on the north stone's sprite, which walks it
// back from its take-over cel to its resting one and turns it dormant on the
// way. `[measured]` returns 0 or 1 - 1 once the walk has reached cel 148 - and
// stones_update() stores that into [stones_pattern] as a one-shot latch rather
// than testing it.
//
// ZUN quirk: the first `if` decrements as well, so a cel of 160 or above
// advances by two rather than one.
extern "C" int near stones_121BA(void)
{
	if(stone_patnum[STONE_NORTH] >= 160) {
		stone_patnum[STONE_NORTH]--;
	}
	if(stone_patnum[STONE_NORTH] == 159) {
		stone_flag[STONE_NORTH] = SF_DORMANT;
		stone_patnum[STONE_NORTH] = 151;
	} else {
		stone_patnum[STONE_NORTH]--;
	}
	if(stone_patnum[STONE_NORTH] <= 148) {
		return 1;
	}
	return 0;
}


// Queues [image] onto the next column pair, on every other frame.
extern "C" void pascal near stones_121F3(uint8_t image)
{
	if((boss_phase_frame & 1) != 0) {
		stones_tile_pending[STONES_TILE_COLS_WEST[stones_tile_pair]] = image;
		stones_tile_pending[STONES_TILE_COLS_EAST[stones_tile_pair]] = image;
		stones_tile_pair++;
		if(stones_tile_pair >= STONES_TILE_COL_PAIRS) {
			stones_tile_pass++;
			stones_tile_pair = 0;
		}
	}
}


// Blits every column's queued image and clears the queue.
extern "C" void near stones_1223E(void)
{
	// `near`, because the original walks the array with a bare 16-bit SI.
	// A large-model far pointer would not fit a register at all, and Turbo C++
	// spills it to the frame and reaches it with `les bx` instead.
	// (th02/main/bg_particle.cpp does the same for the same reason.)
	register uint8_t near *pending;
	register int i;
	screen_x_t left;

	pending = stones_tile_pending;
	egc_start_copy_noframe();
	i = 0;
	left = PLAYFIELD_LEFT;
	while(i < TILES_X) {
		// ZUN quirk: the first two arms read the column through the walking
		// pointer and the third through the index, for the same slot. Both
		// spellings are kept, because they are what the original encodes.
		if(pending[0] == 1) {
			tile_ring_set_and_put_both_8(left, y_22D9C, tile_image_22FD2);
			pending[0] = 0;
		} else if(pending[0] == 2) {
			tile_ring_set_and_put_both_8(left, y_22D9C, tile_image_22FD4);
			pending[0] = 0;
		} else if(stones_tile_pending[i] == 3) {
			tile_ring_set_and_put_both_8(left, y_22D9C, tile_image_22FD6);
			pending[0] = 0;
			stones_tile_cols_done++;
		}
		i++;
		left += TILE_W;
		pending++;
	}
	egc_off();
}


// The animation itself, run once per frame from phase 3 and reset on the
// frame the phase's 49th one comes round. Returns `true` once every column has
// been eaten, which is what advances the phase.
extern "C" bool16 near stones_122B5(void)
{
	register int i;

	if(boss_phase_frame < 49) {
		return false;
	}
	if(boss_phase_frame == 49) {
		for(i = 0; i < TILES_X; i++) {
			stones_tile_pending[i] = 0;
		}
		stones_tile_pair = 0;
		stones_tile_pass = 0;
		stones_tile_cols_done = 0;
		tile_image_22FD2 = 41;
		tile_image_22FD4 = 42;
		tile_image_22FD6 = 43;
	} else if(stones_tile_pass == 0) {
		stones_121F3(1);
	} else if(stones_tile_pass == 1) {
		stones_121F3(2);
	} else if(stones_tile_pass == 2) {
		stones_121F3(3);
	}
	stones_1223E();
	if(stones_tile_cols_done >= TILES_X) {
		return true;
	}
	return false;
}


// The same animation for phase 5, running the three images back the other way
// so that the stage background returns.
extern "C" bool16 near stones_1232F(void)
{
	register int i;

	if(boss_phase_frame < 49) {
		return false;
	}
	if(boss_phase_frame == 49) {
		for(i = 0; i < TILES_X; i++) {
			stones_tile_pending[i] = 0;
		}
		stones_tile_pair = 0;
		stones_tile_pass = 0;
		stones_tile_cols_done = 0;
		tile_image_22FD2 = 42;
		tile_image_22FD4 = 41;
		tile_image_22FD6 = 40;
	} else if(stones_tile_pass == 0) {
		stones_121F3(1);
	} else if(stones_tile_pass == 1) {
		stones_121F3(2);
	} else if(stones_tile_pass == 2) {
		stones_121F3(3);
	}
	stones_1223E();
	if(stones_tile_cols_done >= TILES_X) {
		return true;
	}
	return false;
}


// The stones' [boss_update] callback, installed by stage_init(). Nine phases,
// one `else if` arm each, and the fight ends when all five stones have reached
// SF_REMOVED - not when a phase counter runs out, which is why the arms only
// ever advance [stones_phase] and never return SP_CLEAR themselves.
//
// `int` rather than [stage_progression_t], for the reason marisa_update() in
// th02/main/boss/b4.cpp records: the enum is byte-sized, but the original
// returns its value in the whole of AX.
extern "C" int far stones_update(void)
{
	register int i;
	register int removed;

	removed = 0;
	boss_phase_frame++;
	stones_timeout_frame++;
	stones_11877();
	stones_116EC();
	for(i = 0; i < STONE_COUNT; i++) {
		if(stone_flag[i] == SF_KILL_ANIM) {
			stones_11766(i);
		} else if(stone_flag[i] == SF_REMOVED) {
			removed++;
		}
	}
	if(removed >= STONE_COUNT) {
		return SP_CLEAR;
	}

	// Phase 0: the two inner stones fade in over 14 frames, one sprite cel
	// every second or fourth frame, and go active on frame 64.
	if(stones_phase == 0) {
		if(
			(boss_phase_frame == 50) || (boss_phase_frame == 52) ||
			(boss_phase_frame == 54) || (boss_phase_frame == 58) ||
			(boss_phase_frame == 60) || (boss_phase_frame == 62)
		) {
			stone_patnum[STONE_INNER_WEST]++;
			stone_patnum[STONE_INNER_EAST]++;
		} else if(boss_phase_frame == 64) {
			stone_patnum[STONE_INNER_WEST]++;
			stone_patnum[STONE_INNER_EAST]++;
			boss_phase_frame = 0;
			stone_flag[STONE_INNER_WEST] = SF_ACTIVE;
			stone_flag[STONE_INNER_EAST] = SF_ACTIVE;
			stones_phase = 1;
			stones_phase_frame_unused = 0;
		}

	// Phase 1: the inner pair. The 1300-frame deadline force-kills whichever
	// of the two is still alive, and the outer pair is then taken over.
	} else if(stones_phase == 1) {
		stones_119CD();
		if(stones_timeout_frame > 1300) {
			if(stone_flag[STONE_INNER_WEST] == SF_ACTIVE) {
				stone_flag[STONE_INNER_WEST] = SF_KILL_ANIM;
			}
			if(stone_flag[STONE_INNER_EAST] == SF_ACTIVE) {
				stone_flag[STONE_INNER_EAST] = SF_KILL_ANIM;
			}
		}
		if((removed >= 2) && ((boss_phase_frame % 3) == 0)) {
			// ZUN quirk: only the second call's result is looked at. The west
			// stone is taken over on the same frames regardless, and the two
			// always finish together, so the phase never advances early.
			stones_120F7(STONE_OUTER_WEST);
			if(stones_120F7(STONE_OUTER_EAST)) {
				stones_phase = 2;
				boss_phase_frame = 0;
				stones_phase_frame_unused = 0;
			}
			stones_timeout_frame = 0;
		}

	// Phase 2: the outer pair, on the same deadline. Once all four are gone,
	// the north stone becomes the thing every bullet aims at.
	} else if(stones_phase == 2) {
		if(stones_timeout_frame > 1300) {
			if(stone_flag[STONE_OUTER_WEST] == SF_ACTIVE) {
				stone_flag[STONE_OUTER_WEST] = SF_KILL_ANIM;
			}
			if(stone_flag[STONE_OUTER_EAST] == SF_ACTIVE) {
				stone_flag[STONE_OUTER_EAST] = SF_KILL_ANIM;
			}
		}
		if(removed >= 4) {
			stones_phase = 3;
			boss_phase_frame = 0;
			stones_pattern = 0;
			stones_phase_frame_unused = 0;
			boss_pos_x = (stone_left[STONE_NORTH] + 8);
			boss_pos_y = 32;
		} else {
			stones_11A87(removed);
		}

	// Phase 3: the north stone's take-over.
	} else if(stones_phase == 3) {
		if(
			stones_122B5() && ((boss_phase_frame % 3) == 0) &&
			stones_120F7(STONE_NORTH)
		) {
			stones_phase = 4;
			boss_phase_frame = 0;
			stones_phase_frame_unused = 0;
			stones_timeout_frame = 0;
		}

	// Phase 4: the north stone's first six patterns, in order. Each one zeroes
	// [boss_phase_frame] when its cycle ends, which is what advances the index.
	} else if(stones_phase == 4) {
		if(stones_pattern == 0) {
			stones_11B5D();
		} else if(stones_pattern == 1) {
			stones_11BFE();
		} else if(stones_pattern == 2) {
			stones_11C37();
		} else if(stones_pattern == 3) {
			stones_11DF6();
		} else if(stones_pattern == 4) {
			stones_11E40();
		} else if(stones_pattern == 5) {
			stones_11E76();
		}
		if(boss_phase_frame == 0) {
			stones_pattern++;
			stones_phase_frame_unused = 0;
			if(stones_pattern > 5) {
				stones_phase = 5;
				stones_pattern = 0;
			}
		}
		if(stone_damage[STONE_NORTH] >= 500) {
			stone_damage[STONE_NORTH] = 500;
		}

	// Phase 5: the stone roams. Its destination is picked once, on the first
	// multiple-of-3 frame, and [stones_pattern] latches it rather than
	// indexing anything.
	} else if(stones_phase == 5) {
		if(((boss_phase_frame % 3) == 0) && (stones_pattern == 0)) {
			stones_pattern = stones_121BA();
		}
		if(stones_1232F()) {
			stones_phase = 6;
			boss_phase_frame = 0;
			stones_phase_frame_unused = 0;
			stones_timeout_frame = 0;
			stones_pattern = 0;
		}

	// Phase 6: four patterns, of which the first and third are the same one.
	} else if(stones_phase == 6) {
		if(stones_pattern == 0) {
			stones_11FB5();
		} else if(stones_pattern == 1) {
			stones_11F2F();
		} else if(stones_pattern == 2) {
			stones_11FB5();
		} else if(stones_pattern == 3) {
			stones_1200F();
		}
		if(boss_phase_frame == 0) {
			stones_pattern++;
			stones_phase_frame_unused = 0;
			if(stones_pattern > 3) {
				stones_phase = 7;
			}
		}

	// Phase 7: the second take-over, shape for shape the same as phase 3's.
	} else if(stones_phase == 7) {
		if(
			stones_122B5() && ((boss_phase_frame % 3) == 0) &&
			stones_120F7(STONE_NORTH)
		) {
			stones_phase = 8;
			boss_phase_frame = 0;
			stones_phase_frame_unused = 0;
			stones_timeout_frame = 0;
			stones_pattern = 0;
		}

	// Phase 8: the final loop. Its six patterns are phase 4's with two
	// replaced, and the index wraps to 1 rather than 0, so the first one runs
	// exactly once per fight. 2000 frames in it, the stone dies and the fight
	// with it.
	} else if(stones_phase == 8) {
		if(stones_pattern == 0) {
			stones_11D30();
		} else if(stones_pattern == 1) {
			stones_11BFE();
		} else if(stones_pattern == 2) {
			stones_11C8A();
		} else if(stones_pattern == 3) {
			stones_11DF6();
		} else if(stones_pattern == 4) {
			stones_11E40();
		} else if(stones_pattern == 5) {
			stones_11E76();
		}
		if(boss_phase_frame == 0) {
			stones_pattern++;
			stones_phase_frame_unused = 0;
			if(stones_pattern > 5) {
				stones_pattern = 1;
			}
			if(stones_timeout_frame > 2000) {
				stone_flag[STONE_NORTH] = SF_KILL_ANIM;
			}
		}
	}
	return SP_BOSS;
}


// Runs the stones' post-battle dialog and the stage clear bonus, then advances
// to Stage 4. Installed into [boss_end] by stage_init(); marisa_end() in
// th02/main/boss/b4.cpp is the same function one stage later, with the Stage 4
// script in place of the generic one.
extern "C" void far stones_end(void)
{
	dialog_pre();
	dialog_script_generic_part_animate(DS_POSTBOSS);
	stage_clear_bonus_animate();
	overlay_stage_leave_animate();
	stage_id++;
}


// Runs the stones' pre-battle dialog, switches to their BGM, resets the fight,
// and turns the laser subsystem on for the rest of the stage. Installed into
// [boss_init] by stage_init().
extern "C" void far stones_init(void)
{
	dialog_pre();
	dialog_script_generic_part_animate(DS_PREBOSS);
	dialog_post();

	// boss_bgm_load() and lasers_callbacks_set() are both far, and both land in
	// this same physical segment, so the original reaches them through the
	// linker-relaxed `nop; push cs; call near ptr` form that no plain C++ far
	// call reproduces. (kb/codegen/0083) That form cannot see the C++
	// expressions either, so boss_bgm_load()'s far pointer argument and its
	// __cdecl cleanup are hand-spelled with it.
	//
	// `[measured]` The cleanup is this call's own 4 bytes and nothing else -
	// dialog_pre() and dialog_post() take no arguments and
	// dialog_script_generic_part_animate() is `pascal` and cleans itself - so
	// the island does NOT have to reach backwards the way marisa_init()'s does
	// (kb/codegen/0083's addendum).
	__emit__(0x1E);	// push ds
	_asm { push offset aBoss2_m; }
	__emit__(0x90);	// nop
	__emit__(0x0E);	// push cs
	_asm { call near ptr boss_bgm_load; }
	__emit__(0x83, 0xC4, 0x04);	// add sp, 4

	stones_12778();

	__emit__(0x90);	// nop
	__emit__(0x0E);	// push cs
	_asm { call near ptr lasers_callbacks_set; }
}


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
