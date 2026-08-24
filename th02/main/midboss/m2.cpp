/// Stage 2 midboss
/// ---------------
/// The whole entity: its invalidation callback, the single pattern it fires,
/// its defeat animation, and the per-frame update that is also its hit test and
/// its renderer. It owns no position of its own - it flies straight down the
/// middle of the playfield in [boss_left_on_page] / [boss_top_on_page], the way
/// every boss in this binary does - and its only private state is an alive flag
/// and a defeat clock.
///
/// A NEW OBJECT at the th02/dialog.cpp / th02/main/boss/b2m.cpp seam rather
/// than a prepend into b2m.cpp, and the reason is the same one
/// th02/main/midboss/mx.cpp gives for not being part of th02/main/boss/b6.cpp:
/// one file would be about two unrelated things. `[measured]` Every
/// BOSS_5_TEXT contribution in obj/th02/main.map carries ACBP=28, i.e. BYTE
/// segment alignment, so TLINK inserts nothing between contributions and a new
/// object exactly as long as the bytes the root gives up leaves every later
/// contribution at the offset it had. Cost: one Tupfile.lua line, which has to
/// sit BETWEEN th02/dialog.cpp and th02/main/boss/b2m.cpp - TLINK lays a
/// segment's contributions out in link order and th02_main.asm is the first
/// object it is handed, so that slot is the seam this lift needs.
///
/// Below this block in the dump are the Stage 2 scenery's two remaining procs
/// (stage_bg_flash_update(), the lightning flash, and
/// stage2_update_and_render(), Stage 2's
/// [stage_update_and_render]); above it is the whole of Meira. Neither belongs
/// here.

// -zC, because the segment name would otherwise come from this file's own
// basename and be M2_TEXT (kb/codegen/0105). -zPmain_03 for the near calls that
// leave this segment - tiles_invalidate_rect(), sparks_add(),
// bullets_add_16x16() and shots_hittest() are all in other segments of the same
// group, which is also how th02_main.asm reached them from these very four
// procs. -G because midboss2_update_and_render()'s prolog is
// `push bp; mov bp, sp; sub sp, 4` rather than an `enter 4, 0`
// (kb/codegen/0011). No -a2: nothing here emits a generated jump table - the
// frame dispatch is a compare chain - and no struct's stride is at stake
// (kb/codegen/0170).
#pragma option -zCBOSS_5_TEXT -zPmain_03 -G

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/math/subpixel.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/score.hpp"
#include "th02/main/spark.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/midboss/midboss.hpp"
#include "th02/main/player/shot.hpp"
#include "th02/main/tile/tile.hpp"
#include "th02/sprites/main_pat.h"
#include "th02/v_colors.hpp"

// th02/snd/snd.h, declared here rather than included, the way
// th02/main/midboss/m4.cpp and th02/main/boss/b4.cpp already declare it: snd.h
// has no include guard and pulls in three more unguarded headers for the sake
// of one function.
extern "C" void __cdecl snd_se_play(int new_se);

// `[measured]` Reused as this midboss's own defeat flag as well as
// [boss_phase]: 0 while it is alive, 1 for the frame the hit test below shoots
// it down, and back to 0 once the defeat animation has zoomed it away. Still a
// kb/codegen/0123 alias in th02_main.asm, where ~20 sites read and write it.
extern "C" uint8_t boss_phase;

// The point shottype B's homing shots aim at, set by whichever boss or midboss
// is on screen. Declared here the way th02/main/boss/b2.cpp, b4.cpp,
// th02/main/midboss/m4.cpp and th02/main/player/shot.cpp already declare them;
// no header owns them.
extern "C" int boss_pos_x;
extern "C" int boss_pos_y;

// The Stage 1 and 2 scenery's lightning-flash clock, which
// midboss2_update_and_render() below arms on its first frame and nothing else
// here touches. `[measured]` stage_bg_flash_update() counts it up and, over
// the eight frames from whichever frame it is due, alternates [PaletteTone]
// between 140 and 100 before restoring 100 and drawing the next due frame from
// randring2_next16(); stage1_update_and_render() arms it the same way and
// rika_end() clears it. All three readers are now C++, so the storage is a
// plain semantic name rather than an address alias.
extern "C" int16_t bg_flash_frame;

/// State
/// -----
/// Both are PLAIN RENAMES: every reference th02_main.asm had to either of them
/// was inside the four procs this parcel lifted.

// Whether the midboss is still on screen. Raised on its first frame, lowered
// when the defeat animation ends, and returned unchanged by
// midboss2_invalidate(), which is what stage_loop() copies into
// [midboss_active]. Same role and same `dw` width as [midboss4_active].
extern "C" bool16 midboss2_active;

// How far the midboss has zoomed out of the playfield after being shot down.
// Counts one per frame from 0 to 64, and is the only thing the defeat animation
// runs on - exactly like [midboss4_defeat_frame], including the `dw 0` in
// th02_main.asm's own `_DATA` rather than a reservation in `_BSS`.
extern "C" int16_t midboss2_defeat_frame;
/// -----

// Its sprite: a four-cel animation, plus the single frame it is blitted in
// white on after being hit. `[measured]` The cel is NOT a clock of its own - it
// is `(scroll_line >> 1) & 3`, so the midboss's animation is driven by the
// scenery scrolling past it and stops dead whenever the map does.
static const main_patnum_t MIDBOSS2_PATNUM = 137;
static const main_patnum_t MIDBOSS2_PATNUM_HIT = 136;
static const int MIDBOSS2_CELS = 4;

// Where it enters from and where it stops. `[measured]` It starts 32 pixels
// ABOVE the playfield and descends one pixel every 4th frame for 320 frames, so
// it is 48 pixels into the playfield by the time it starts firing.
static const screen_x_t MIDBOSS2_ENTER_LEFT = (
	PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 32
);
static const screen_y_t MIDBOSS2_ENTER_TOP = (PLAYFIELD_TOP - 32);
static const int MIDBOSS2_ENTER_FRAMES = 320;
static const int MIDBOSS2_DESCENT_INTERVAL_MASK = 3;

// The box its own shots are tested against. `[measured]` Its sprite is 64x64
// and the box sits at the sprite's top-left corner with no offset at all, so it
// is the sprite's full width and 6 rows short of its height - the bottom 6 rows
// of the midboss cannot be hit. `[measured 2026-08-24]` The width is 64 and NOT
// 4: the dump pushes both halves of this pair as ONE dword immediate, whose
// HIGH word is the width, so reading 40003Ah as a 4 and a 3Ah instead costs two
// bytes of immediate at each of the two hit-test call sites and nothing else -
// at exactly the right total length.
static const pixel_t MIDBOSS2_SHOT_HITBOX_W = 64;
static const pixel_t MIDBOSS2_SHOT_HITBOX_H = 58;

// What it takes to shoot it down, and what that is worth. `[measured]` BOTH
// conditions have to hold: more than 380 damage AND a wrapped VRAM row BELOW
// 304, so a hit that would finish it off while the scenery has scrolled it into
// the bottom 96 rows of VRAM does nothing but flash it and has to be repeated.
// That row is [boss_top_on_back_page] + [scroll_line] wrapped into [0; RES_Y[,
// so which screen positions can finish this midboss off depends on how far the
// map has scrolled and not on where the midboss is. It also leaves on its own
// after MIDBOSS2_LEAVE_FRAME whether it was ever hit or not, and that path pays
// no score at all.
static const int MIDBOSS2_DAMAGE_MAX = 380;
static const vram_y_t MIDBOSS2_DEFEAT_VRAM_Y_MAX = 304;
static const int MIDBOSS2_LEAVE_FRAME = 1600;

// Its one pattern's cadence: a bullet on every 8th frame of the first 64 frames
// of every 192, i.e. eight bullets and then 128 idle frames.
static const int MIDBOSS2_PATTERN_INTERVAL = 8;
static const int MIDBOSS2_PATTERN_PERIOD = 192;
static const int MIDBOSS2_PATTERN_BURST_FRAMES = 64;

// Its defeat animation's length, and the two numbers its zoom is built from.
// `[measured]` The zoomed sprite is a PATTERN NUMBER and not a zoom factor: it
// is super_zoom()'s third argument, which both libs/master.lib/pc98_gfx.hpp and
// libs/master.lib/super_zoom.asm's own header comment call `num`, with the
// factor fourth and fixed at 2 here.
static const int MIDBOSS2_DEFEAT_FRAMES = 64;
static const int MIDBOSS2_DEFEAT_ZOOM_PATNUM = 10;
// A SHIFT and not a divisor: the original is a bare `sar ax, 3`, and a signed
// `/ 8` under -O emits the negative-operand correction ahead of it.
// th02/main/midboss/m4.cpp's midboss4_19FAF() spells its copy `>> 3` too.
static const int MIDBOSS2_DEFEAT_ZOOM_PATNUM_SHIFT = 3;

// The frames of the defeat animation that get a spark burst, and how big it is.
// `[measured]` A different shape from midboss4_19FAF()'s plain
// `(boss_phase_frame % 24) == 0`: this one masks the defeat clock and stops
// after 48 of the 64 frames, so the last quarter of the zoom is silent.
static const int MIDBOSS2_DEFEAT_SPARK_UNTIL = 48;
static const int MIDBOSS2_DEFEAT_SPARK_INTERVAL_MASK = 15;
static const int MIDBOSS2_DEFEAT_SPARK_PHASE = 5;
static const int MIDBOSS2_DEFEAT_SPARK_COUNT = 24;


bool16 midboss2_invalidate(void)
{
	boss_left_on_back_page = &boss_left_on_page[page_back];
	boss_top_on_back_page = &boss_top_on_page[page_back];
	tiles_invalidate_rect(
		*boss_left_on_back_page, *boss_top_on_back_page, 64, 64
	);
	*boss_left_on_back_page = boss_left_on_page[page_front];
	*boss_top_on_back_page = boss_top_on_page[page_front];
	return midboss2_active;
}


// The midboss's defeat animation: a 24-spark burst on four frames of it, over a
// super_zoom() pattern number that grows by 1 every 8 frames for 64 frames.
// Ends by parking [boss_pos_x] / [boss_pos_y] outside the playfield and
// clearing both this midboss's flag and [boss_phase], so a second appearance in
// the same stage starts from scratch.
static void near midboss2_14169(void)
{
	int zoom_patnum;

	if(
		(midboss2_defeat_frame < MIDBOSS2_DEFEAT_SPARK_UNTIL) &&
		((midboss2_defeat_frame & MIDBOSS2_DEFEAT_SPARK_INTERVAL_MASK) ==
			MIDBOSS2_DEFEAT_SPARK_PHASE)
	) {
		sparks_add(
			(*boss_left_on_back_page + 32),
			(*boss_top_on_back_page + 32),
			to_sp(8.0f),
			MIDBOSS2_DEFEAT_SPARK_COUNT,
			true
		);
	}

	// TWO STATEMENTS, and the base is NOT an initializer. Before the lift,
	// `mov di, 0Ah` came here after the spark burst
	// (`d9491e25:th02_main.asm:1997`). th02/main/midboss/m4.cpp's
	// midboss4_19FAF() spells its copy of this the same way.
	zoom_patnum = MIDBOSS2_DEFEAT_ZOOM_PATNUM;
	zoom_patnum += (
		midboss2_defeat_frame >> MIDBOSS2_DEFEAT_ZOOM_PATNUM_SHIFT
	);

	midboss2_defeat_frame++;
	if(midboss2_defeat_frame >= MIDBOSS2_DEFEAT_FRAMES) {
		midboss2_defeat_frame = 0;
		boss_phase_frame = 0;
		boss_phase = 0;
		midboss2_active = false;
		boss_pos_x = -1;
		boss_pos_y = -1;
	} else {
		register vram_y_t vram_y;

		vram_y = *boss_top_on_back_page;
		vram_y += scroll_line;
		if(vram_y >= RES_Y) {
			vram_y -= RES_Y;
		}
		super_zoom(*boss_left_on_back_page, vram_y, zoom_patnum, 2);
	}
}


// The midboss's one and only pattern: a single homing 16x16 bullet, on every 8th
// frame of the first 64 frames of every 192. `[measured]` Its angle is the low
// byte of [boss_phase_frame] times 4, so the eight bullets of a burst come out
// 32 angle units apart and the bursts do not repeat until the period and the
// angle wrap line up.
static void near midboss2_14203(void)
{
	if(
		((boss_phase_frame % MIDBOSS2_PATTERN_INTERVAL) == 0) &&
		((boss_phase_frame % MIDBOSS2_PATTERN_PERIOD) <
			MIDBOSS2_PATTERN_BURST_FRAMES)
	) {
		bullets_add_16x16(
			(*boss_left_on_back_page + 28),
			(*boss_top_on_back_page + 32),
			(boss_phase_frame << 2),
			BSM_CHASE,
			PAT_BULLET16_NOROI,
			((1 << 4) + 14)
		);
	}
}


// The midboss's entry point, and everything but the invalidation: its fly-in,
// its sprite, its hit test, its pattern and its defeat. Installed into
// [midboss_update_and_render] by stage_init().
void midboss2_update_and_render(void)
{
	int cel;
	int damage;
	register vram_y_t vram_y;

	// `= 0` ON THE DECLARATION, unlike the two zoom counters above: the
	// original's `xor di, di` is the first instruction after the prolog, and
	// both arms below overwrite it before it is ever read.
	register main_patnum_t patnum = 0;

	boss_pos_x = 216;
	boss_pos_y = (*boss_top_on_back_page + 24);
	boss_phase_frame++;
	if(boss_phase_frame == 1) {
		boss_left_on_page[0] = MIDBOSS2_ENTER_LEFT;
		boss_left_on_page[1] = MIDBOSS2_ENTER_LEFT;
		boss_top_on_page[0] = MIDBOSS2_ENTER_TOP;
		boss_top_on_page[1] = MIDBOSS2_ENTER_TOP;
		boss_damage = 0;
		midboss2_active = true;
		bg_flash_frame = 1;
	} else if(boss_phase_frame < MIDBOSS2_ENTER_FRAMES) {
		// THE CEL IS ASSIGNED AND THEN NEVER READ AGAIN, in both arms: the
		// original stores it to `[bp-2]` and adds the base to the AX it still
		// holds (kb/codegen/0140). Written out because the store is in the
		// binary, and duplicated because the two arms are the HEADS of their
		// blocks - `-O` only merges tails.
		cel = ((scroll_line >> 1) & (MIDBOSS2_CELS - 1));
		patnum = (cel + MIDBOSS2_PATNUM);

		if((boss_phase_frame & MIDBOSS2_DESCENT_INTERVAL_MASK) == 0) {
			(*boss_top_on_back_page)++;
		}
		vram_y = *boss_top_on_back_page;
		vram_y += scroll_line;
		if(vram_y < 0) {
			vram_y += RES_Y;
		} else if(vram_y >= RES_Y) {
			vram_y -= RES_Y;
		}
		if(shots_hittest(
			*boss_left_on_back_page,
			*boss_top_on_back_page,
			MIDBOSS2_SHOT_HITBOX_W,
			MIDBOSS2_SHOT_HITBOX_H
		) != 0) {
			snd_se_play(4);
			super_roll_put_1plane(
				*boss_left_on_back_page, vram_y, MIDBOSS2_PATNUM_HIT, 0,
				super_plane(V_WHITE)
			);

			// `++`, not `+= damage`. `[measured]` The whole fly-in is worth one
			// damage point per FRAME the midboss is hit at all, no matter how
			// many shots landed in the box - so the 380 it takes to shoot this
			// midboss down cannot be paid before the descent is over, and the
			// arm below is where every fight is actually decided.
			boss_damage++;
		} else {
			// WRITTEN OUT IN BOTH ARMS ON PURPOSE, here and at the bottom.
			// kb/codegen/0097: `-O` tail-merges the two, leaving one copy plus
			// the original's long `jz` from here into the middle of the other -
			// and the shared tail is branch-free, so kb/codegen/0144's `goto`
			// exception does not apply.
			super_roll_put(*boss_left_on_back_page, vram_y, patnum);
		}
	} else {
		cel = ((scroll_line >> 1) & (MIDBOSS2_CELS - 1));
		patnum = (cel + MIDBOSS2_PATNUM);

		if(boss_phase != 0) {
			midboss2_14169();
		} else {
			midboss2_14203();
			vram_y = *boss_top_on_back_page;
			vram_y += scroll_line;
			if(vram_y < 0) {
				vram_y += RES_Y;
			} else if(vram_y >= RES_Y) {
				vram_y -= RES_Y;
			}

			// Assigned and tested in one expression, which is kb/codegen/0143:
			// at three mentions of [damage] Turbo C++ enregisters it, and the
			// original keeps it on the frame beside [cel].
			if((damage = shots_hittest(
				*boss_left_on_back_page,
				*boss_top_on_back_page,
				MIDBOSS2_SHOT_HITBOX_W,
				MIDBOSS2_SHOT_HITBOX_H
			)) != 0) {
				boss_damage += damage;
				// `>=` ON THE SECOND ARM, and no length check can see the
				// difference. Before the lift, `jle` entered the flash on the
				// damage test and `jl` entered DEFEAT on the row test
				// (`d9491e25:th02_main.asm:2192`), i.e. it
				// falls into the flash when the row is >= 304 - so the row half
				// of this `||` is the survivor's condition, and spelling it
				// `<` emits a `jge` of exactly the same two bytes.
				if(
					(boss_damage <= MIDBOSS2_DAMAGE_MAX) ||
					(vram_y >= MIDBOSS2_DEFEAT_VRAM_Y_MAX)
				) {
					snd_se_play(4);
					super_roll_put_1plane(
						*boss_left_on_back_page, vram_y,
						MIDBOSS2_PATNUM_HIT, 0, super_plane(V_WHITE)
					);
				} else {
					snd_se_play(2);
					boss_phase = 1;
					score_delta += 20000;
				}
			} else {
				if(boss_phase_frame > MIDBOSS2_LEAVE_FRAME) {
					boss_phase = 1;
				}
				super_roll_put(*boss_left_on_back_page, vram_y, patnum);
			}
		}
	}
}
