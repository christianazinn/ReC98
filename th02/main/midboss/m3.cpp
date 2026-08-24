/// Stage 3 midboss
/// ---------------
/// The whole entity: its invalidation callback, its two sprite blits, its
/// defeat animation, the single movement-and-fire routine that both copies
/// run, and the per-frame update that dispatches between them.
///
/// There are TWO of them, entering from opposite sides of the playfield, and
/// they own almost no state: [midboss3_flag] and [midboss3_damage] are the
/// Stage 3 boss's own arrays under a different name
/// (th02/main/boss/b3.hpp), and the sprite number is [stone_patnum]'s slot 0.
/// Their bodies sit directly above th02/main/boss/b3.cpp's in DIALOG_TEXT, so
/// this translation unit needs no split and no new segment - it just
/// contributes to that segment between th02_main.asm's block and b3.cpp's.
/// (kb/codegen/0099)
///
/// NO `-a2` HERE, exactly like b3.cpp: `[measured]` nothing in this object
/// emits a generated jump table, so there is no alignment to pin, and the
/// object below it does not pad either.

// -zC, because the segment name would otherwise come from this file's own
// basename (kb/codegen/0105). -G, because every prolog here is
// `push bp; mov bp, sp` rather than an `ENTER` (kb/codegen/0011).
#pragma option -zCDIALOG_TEXT -zPmain_03 -G

#include "platform.h"
#include "pc98.h"
#include "th01/rank.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/core/globals.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/tile/tile.hpp"
#include "th02/main/spark.hpp"
#include "th02/main/score.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/boss/b3.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/player/shot.hpp"
#include "th02/main/midboss/midboss.hpp"
#include "th02/v_colors.hpp"

// th02/snd/snd.h, declared here rather than included, the way
// th02/main/midboss/m4.cpp and th02/main/laser.cpp already declare it: snd.h
// has no include guard and pulls in three more unguarded headers for the sake
// of one function.
extern "C" void __cdecl snd_se_play(int new_se);

// The point shottype B's homing shots aim at, set by whichever boss or midboss
// is on screen. Declared here the way th02/main/midboss/m4.cpp,
// th02/main/boss/b4.cpp and th02/main/player/shot.cpp already declare them; no
// header owns them.
extern "C" int boss_pos_x;
extern "C" int boss_pos_y;

// The Stage 3 boss's per-stone sprite number, whose slot 0 this midboss reuses
// as its own - the two never coexist. Declared exactly the way
// th02/main/boss/b3.cpp declares it. `[measured]` The dump reaches
// [stone_patnum]'s very first word from both entities and no other slot from
// here.
extern "C" int16_t stone_patnum[STONE_COUNT];

// State
// -----

extern "C" int16_t midboss3_kill_frame[MIDBOSS3_COUNT];

// Same as for regular bosses.
// ZUN bloat: We only need two for each of these, not 5.
extern "C" screen_x_t midboss3_left_on_page[STONE_COUNT][PAGE_COUNT];
extern "C" screen_y_t midboss3_top_on_page[STONE_COUNT][PAGE_COUNT];
extern "C" screen_x_t* midboss3_left_on_back_page[STONE_COUNT];
extern "C" screen_y_t* midboss3_top_on_back_page[STONE_COUNT];

// The angle each of the two copies currently fires at, seeded to opposite
// sides of the circle at the start of the attack and turned by ±8 with every
// shot. `[measured]` One array, not two slots: midboss3_11308() reaches it
// with the loop variable in its ordinary arm and seeds both cells by constant
// index in the same breath, which is kb/codegen/0123's whole-extent case.
extern "C" uint8_t midboss3_angle[MIDBOSS3_COUNT];

// How many times the 448-frame attack routine has begun. Raised on the frame
// [boss_phase_frame] reaches 128 - which midboss3_11308() re-arms by resetting
// the counter to 127 once the routine runs out - and both copies are forced
// into their defeat animation once it reaches 4. So this is what ends the
// fight, and nothing else does.
extern "C" uint8_t midboss3_loops;
// -----

// The hitbox both copies are tested with, 12 pixels right of their top-left
// corner. Wider than the 32×32 sprite pair is tall, and offset by only 12
// rather than the 16 that would centre it.
static const pixel_t MIDBOSS3_HITBOX_W = 40;
static const pixel_t MIDBOSS3_HITBOX_H = 30;

// The bullet muzzle, 28 pixels right of the top-left corner.
#define MIDBOSS3_MUZZLE_LEFT(i) (*midboss3_left_on_back_page[i] + 28)


// Marks the tiles under both copies for redrawing, and copies their positions
// from the front page to the back one. Returns `false` once both have finished
// their defeat animation, which is what stage_loop() copies into
// [midboss_active].
bool16 midboss3_invalidate(void)
{
	register int i;
	int removed;

	removed = 0;
	for(i = 0; i < MIDBOSS3_COUNT; i++) {
		if(midboss3_flag[i] == M3F_REMOVED) {
			removed++;
			continue;
		}
		midboss3_left_on_back_page[i] = &midboss3_left_on_page[i][page_back];
		midboss3_top_on_back_page[i] = &midboss3_top_on_page[i][page_back];
		tiles_invalidate_rect(
			*midboss3_left_on_back_page[i],
			*midboss3_top_on_back_page[i],
			64,
			32
		);
		*midboss3_left_on_back_page[i] = midboss3_left_on_page[i][page_front];
		*midboss3_top_on_back_page[i] = midboss3_top_on_page[i][page_front];
	}
	// A conditional expression, not the `!=` this reads as: the original falls
	// through to the 0 and branches to the 1, and `return (removed !=
	// MIDBOSS3_COUNT)` emits exactly the other way round.
	return ((removed == MIDBOSS3_COUNT) ? false : true);
}


// One copy's sprite, blitted white for the single frame it was hit on.
// Unlike every other entity in this binary, the flash is not latched in a
// [*_hit_flash] flag - the caller simply picks this renderer over the plain
// one below.
static void pascal near midboss3_11183(int i)
{
	// Split across two statements, not `y = (*top + scroll_line)`: the
	// one-expression form computes the sum in AX and moves it into the
	// register afterwards, where the original loads it directly.
	// (kb/codegen/0146)
	register vram_y_t y;

	snd_se_play(4);
	y = *midboss3_top_on_back_page[i];
	y += scroll_line;
	if(y >= RES_Y) {
		y -= RES_Y;
	}
	super_roll_put_1plane(
		*midboss3_left_on_back_page[i],
		y,
		stone_patnum[0],
		0,
		super_plane(V_WHITE)
	);
}


// One copy's sprite, at its scroll-wrapped row.
static void pascal near midboss3_111D1(int i)
{
	register vram_y_t y;

	y = *midboss3_top_on_back_page[i];
	y += scroll_line;
	if(y >= RES_Y) {
		y -= RES_Y;
	}
	super_roll_put(
		*midboss3_left_on_back_page[i], y, stone_patnum[0]
	);
}


// One frame of one copy's defeat animation: two sparks every frame, a sprite
// pair that advances every 6th, and a 32-spark ring at the end of the 48
// frames. Parks [boss_pos_x] / [boss_pos_y] outside the playfield on the way
// out, which the second copy then does again.
static void pascal near midboss3_1120F(int i)
{
	int patnum;
	vram_y_t y;

	sparks_add(
		(*midboss3_left_on_back_page[i] + 32),
		(*midboss3_top_on_back_page[i] + 16),
		to_sp(3.75f),
		1,
		false
	);
	patnum = 10;
	patnum += (midboss3_kill_frame[i] / 6);
	midboss3_kill_frame[i]++;
	if(midboss3_kill_frame[i] >= 48) {
		sparks_add(
			(*midboss3_left_on_back_page[i] + 32),
			(*midboss3_top_on_back_page[i] + 16),
			to_sp(8.0f),
			32,
			true
		);
		midboss3_kill_frame[i] = 0;
		midboss3_flag[i] = M3F_REMOVED;
		boss_pos_x = -1;
		boss_pos_y = -1;
	} else {
		y = *midboss3_top_on_back_page[i];
		y += scroll_line;
		if(y >= RES_Y) {
			y -= RES_Y;
		}
		super_roll_put(*midboss3_left_on_back_page[i], y, patnum);
		super_roll_put(
			(*midboss3_left_on_back_page[i] + 32), y, patnum
		);
	}
}


// One copy's entire movement and fire routine, a single 448-frame timeline
// that [midboss3_loops] then restarts: turn on the spot while firing, sweep
// sideways with a spread every 8th frame, scroll down out of the playfield and
// back in through the top, then sweep back the other way. Both copies run it
// with mirrored steps.
static void pascal near midboss3_11308(int i)
{
	// None of these is `register`: the original keeps the *parameter* in SI and
	// puts [top] in DI, which is kb/codegen/0146's plain
	// `near f(int p)` + `int q` row. Marking [top] `register` moves it to SI
	// and pushes [i] the other way; before the lift, every access used
	// `mov bx, si` (`0fc3dbd0:th02_main.asm:3287`).
	int step;
	uint8_t group;
	screen_y_t top;

	// A conditional expression rather than two assignments: the original
	// picks the step in AX and stores it to the frame once, at the join.
	step = ((i == 0) ? 8 : -8);
	top = (*midboss3_top_on_back_page[i] + 12);
	if(boss_phase_frame < 152) {
		if(boss_phase_frame == 128) {
			midboss3_angle[0] = 0x00;
			midboss3_angle[1] = 0x80;
		}
		if(!(boss_phase_frame & 3)) {
			midboss3_angle[i] += step;
			bullets_add_pellet(
				MIDBOSS3_MUZZLE_LEFT(i),
				top,
				midboss3_angle[i],
				BG_1,
				to_sp(2.5f)
			);
		}
	} else if(boss_phase_frame < 192) {
		*midboss3_left_on_back_page[i] += step;
		if(!(boss_phase_frame & 7)) {
			midboss3_angle[i] += (step * 2);
			// Two assignments rather than a conditional expression: the
			// original stores into the frame slot in both arms rather than
			// picking the group in AL and storing once at the join, and it
			// tests for the *inequality*, which is what puts the wider spread
			// into the fall-through arm.
			if(rank != RANK_EASY) {
				group = BG_4_SPREAD_MEDIUM;
			} else {
				group = BG_2_SPREAD_MEDIUM;
			}
			bullets_add_pellet(
				MIDBOSS3_MUZZLE_LEFT(i),
				top,
				(0x80 - midboss3_angle[i]),
				group,
				to_sp(2.5f)
			);
		}
	} else if(boss_phase_frame < 352) {
		*midboss3_top_on_back_page[i] += 2;
		if(*midboss3_top_on_back_page[i] >= RES_Y) {
			*midboss3_top_on_back_page[i] -= RES_Y;
		}
	} else if(boss_phase_frame < 376) {
		if(!(boss_phase_frame & 3)) {
			midboss3_angle[i] -= step;
			bullets_add_pellet(
				MIDBOSS3_MUZZLE_LEFT(i),
				top,
				midboss3_angle[i],
				BG_1,
				to_sp(3.125f)
			);
		}
	} else if(boss_phase_frame < 416) {
		*midboss3_left_on_back_page[i] -= step;
		if(!(boss_phase_frame & 3)) {
			midboss3_angle[i] -= step;
			bullets_add_pellet(
				MIDBOSS3_MUZZLE_LEFT(i),
				top,
				midboss3_angle[i],
				BG_1,
				to_sp(3.125f)
			);
		}
	} else if(boss_phase_frame < 576) {
		*midboss3_top_on_back_page[i] -= 2;
		if(*midboss3_top_on_back_page[i] < 0) {
			*midboss3_top_on_back_page[i] += RES_Y;
		}
		if((boss_phase_frame % 60) == 0) {
			bullets_add_pellet(
				MIDBOSS3_MUZZLE_LEFT(i),
				top,
				0x00,
				BG_1_AIMED,
				to_sp(2.5f)
			);
		}
	} else {
		boss_phase_frame = 127; // Skip the scroll-in animation
	}
}


// Both copies' per-frame update. Scrolls them in from the top for 128 frames
// while they can already be shot, then runs the attack routine above until
// [midboss3_loops] retires them. Installed into [midboss_update_and_render] by
// stage_init().
void midboss3_update_and_render(void)
{
	// ZUN bloat: Counted exactly the way midboss3_invalidate() counts it, and
	// then thrown away - nothing ever reads it. That function's copy is the
	// one that decides when the midboss is gone.
	int removed;

	register int i;
	int damage;

	removed = 0;
	if(midboss3_flag[0] == M3F_ALIVE) {
		boss_pos_x = (midboss3_left_on_page[0][page_back] + 24);
		boss_pos_y = (midboss3_top_on_page[0][page_back] + 16);
	} else {
		boss_pos_x = (midboss3_left_on_page[1][page_back] + 24);
		boss_pos_y = (midboss3_top_on_page[1][page_back] + 16);
	}
	boss_phase_frame++;
	if(boss_phase_frame == 1) {
		midboss3_left_on_page[0][0] = PLAYFIELD_LEFT;
		midboss3_left_on_page[0][1] = PLAYFIELD_LEFT;
		midboss3_top_on_page[0][0] = -16;
		midboss3_top_on_page[0][1] = -16;
		midboss3_damage[0] = 0;
		midboss3_left_on_page[1][0] = (PLAYFIELD_RIGHT - 64);
		midboss3_left_on_page[1][1] = (PLAYFIELD_RIGHT - 64);
		midboss3_top_on_page[1][0] = midboss3_top_on_page[0][0];
		midboss3_top_on_page[1][1] = midboss3_top_on_page[0][0];
		midboss3_damage[1] = 0;
		midboss3_loops = 0;
	} else if(boss_phase_frame < 128) {
		stone_patnum[0] = ((scroll_line & 3) + 144);
		for(i = 0; i < MIDBOSS3_COUNT; i++) {
			*midboss3_top_on_back_page[i] += scroll_delta;
			if((damage = shots_hittest(
				(*midboss3_left_on_back_page[i] + 12),
				*midboss3_top_on_back_page[i],
				MIDBOSS3_HITBOX_W,
				MIDBOSS3_HITBOX_H
			)) != 0) {
				midboss3_damage[i] += damage;
				midboss3_11183(i);
			} else {
				midboss3_111D1(i);
			}
		}
	} else {
		stone_patnum[0] = ((scroll_line & 3) + 144);
		if(boss_phase_frame == 128) {
			midboss3_loops++;
		}
		for(i = 0; i < MIDBOSS3_COUNT; i++) {
			if(midboss3_flag[i] == M3F_KILL_ANIM) {
				midboss3_1120F(i);
			} else if(midboss3_flag[i] == M3F_ALIVE) {
				midboss3_11308(i);
				if((damage = shots_hittest(
					(*midboss3_left_on_back_page[i] + 12),
					*midboss3_top_on_back_page[i],
					MIDBOSS3_HITBOX_W,
					MIDBOSS3_HITBOX_H
				)) != 0) {
					midboss3_damage[i] += damage;
					// Spelled as the survivor's condition, not the kill's: the
					// original branches *past* the flash on `jg`, so the kill
					// arm is the one that sinks below it.
					if(midboss3_damage[i] <= 280) {
						midboss3_11183(i);
					} else {
						snd_se_play(2);
						midboss3_flag[i] = M3F_KILL_ANIM;
						score_delta += 20000;
					}
				} else {
					midboss3_111D1(i);
				}
				if(midboss3_loops >= 4) {
					midboss3_flag[i] = M3F_KILL_ANIM;
				}
			} else if(midboss3_flag[i] == M3F_REMOVED) {
				removed++;
			}
		}
	}
}
