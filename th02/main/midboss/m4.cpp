/// Stage 4 midboss
/// ---------------
/// The whole entity: its invalidation callback, its sprite blit, its defeat
/// animation, its hit test, the five patterns it fires, and the per-frame
/// update that dispatches between them.
///
/// It shares [marisa_topleft], [marisa_intro_step] and [marisa_intro_direction]
/// with the Stage 4 boss rather than owning storage of its own, and its bodies
/// sit directly below th02/main/boss/b4.cpp's in the same nameless code
/// segment - so this translation unit needs no split and no new segment, it
/// just contributes to that segment between th02_main.asm's block and b4.cpp's.
/// (kb/codegen/0099)
///
/// NO `-a2` HERE, unlike b4.cpp, and that is the whole reason this is a
/// separate object rather than more of b4.cpp. `[measured]` Both this object's
/// jump table and marisa_update()'s live in main_03__TEXT, 0x1D46 bytes apart -
/// an EVEN distance - and the original pads exactly one of them: nothing before
/// off_1A419 here, one `db 0` before marisa_update()'s. kb/codegen/0154's rule
/// (`-a2` pads a table whose natural offset in the object is even) applies to
/// every table in an object against the same running offset, so one object can
/// never pad one of two even-separated tables and not the other. Two objects
/// can: b4.cpp keeps `-a2` and its pad, and this one drops `-a2` and gets no
/// padding at any length. (kb/codegen/0157)

// -G, because the original's prologs are `push bp; mov bp, sp` with no locals
// rather than an `ENTER`. (kb/codegen/0011)
#pragma option -zCmain_03__TEXT -zPmain_03 -G

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
#include "th02/main/frames.hpp"
#include "th02/math/randring.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/boss/b4.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/spark.hpp"
#include "th02/main/score.hpp"
#include "th02/main/item/item.hpp"
#include "th02/main/player/shot.hpp"
#include "th02/main/laser.hpp"
#include "th02/main/midboss/midboss.hpp"
#include "th02/v_colors.hpp"
#include "th02/sprites/bullet16.h"

// th02/snd/snd.h, declared here rather than included, the way
// th02/main/boss/b4.cpp and th02/main/laser.cpp already declare it: snd.h has
// no include guard and pulls in three more unguarded headers for the sake of
// one function.
extern "C" void __cdecl snd_se_play(int new_se);

// The sprite the boss and midboss renderers blit, shared by all of them and
// written from ~150 sites across th02_main.asm. `patnum_2064E` is the dump's
// own spelling and is not an IDA placeholder; retiring the address suffix
// means ruling on all of those sites at once, which is its own parcel.
// Declared exactly the way th02/main/boss/b4.cpp already declares it.
extern "C" int patnum_2064E;

// Which way this midboss flies, and the slot physically after
// [marisa_intro_step]. Also declared by th02/main/boss/b4.cpp, whose
// marisa_init() clears it; see that file for the census of its 18 uses, all of
// which are in this translation unit now.
extern "C" int marisa_intro_direction;

// The white flash every boss and midboss in this binary blits itself with for
// exactly one frame after being hit. Raised by midboss4_1A044() below, read
// and lowered again by midboss4_19F52(). Also declared by
// th02/main/boss/b4.cpp; see that file for the three-role census.
extern "C" bool boss_hit_flash;

// The point shottype B's homing shots aim at, set by whichever boss or midboss
// is on screen. Declared here the way th02/main/boss/b4.cpp,
// th02/main/player/shot.cpp and th02/main/player/reset.cpp already declare
// them; no header owns them.
extern "C" int boss_pos_x;
extern "C" int boss_pos_y;

// `[measured]` Reused as this midboss's own defeat/despawn flag as well as
// [boss_phase]: 0 while it is alive, 1 for the frame midboss4_1A044() shoots
// it down, and 2 once midboss4_19FAF() has finished zooming it away.
extern "C" uint8_t boss_phase;

// State
// -----

// How far this midboss has zoomed out of the playfield after being shot down.
// Counts one per frame from 0 to 64, and is the only thing midboss4_19FAF()
// runs on. `[measured]` A `dw 0` in th02_main.asm's own `_DATA`, two bytes
// after [marisa_damage_multiplier] and touched by no other proc, so this is a
// rename rather than a kb/codegen/0123 alias.
extern "C" int16_t midboss4_defeat_frame;

// The [top] every pellet this midboss fires spawns at: 48 pixels below its own
// top-left corner, recomputed at the head of every frame. `[measured]` The
// dump reads it back in midboss4_1A1B6() only; the two other pattern functions
// that spawn pellets pass (PLAYFIELD_TOP + 80) literally instead, which is the
// same number only while the midboss is at its entrance height.
extern "C" screen_y_t midboss4_pellet_top;

// Whether the midboss is still on screen. Raised on the first frame of the
// fly-in, lowered when it either leaves the playfield sideways or finishes its
// defeat animation, and returned unchanged by midboss4_invalidate(), which is
// what stage_loop() copies into [midboss_active].
extern "C" bool16 midboss4_active;

// Which of the five patterns below is currently being fired. Re-rolled from
// randring2_next8() on every direction change.
extern "C" uint8_t midboss4_pattern;

// How many patterns the midboss has fired so far. `[measured]` It leaves after
// 12, compared unsigned, and nothing ever resets it except the fly-in - so a
// second appearance in the same stage starts the count over.
extern "C" uint8_t midboss4_patterns_seen;
// -----

// The pattern functions, and the two renderers. All are called from
// midboss4_update_and_render() below and from nowhere else - not even from
// th02_main.asm, which is why they need no `public` and are `static` here.

// The [scroll_step] this midboss is spawned at, which is also the sentinel
// value the item drop below tests for. See th02/main/stage/init.cpp.
static const int MIDBOSS4_SCROLL_STEP = 1632;

// The muzzle every pattern that fires a pellet aims from: 28 pixels right of
// the midboss's top-left corner, leaned one sprite-width further into the
// direction it is flying.
#define MIDBOSS4_MUZZLE_LEFT \
	(((marisa_intro_direction << 4) + marisa_topleft.x) + 28)


bool16 midboss4_invalidate(void)
{
	boss_left_on_back_page = &boss_left_on_page[page_back];
	boss_top_on_back_page = &boss_top_on_page[page_back];
	tiles_invalidate_rect(
		*boss_left_on_back_page, *boss_top_on_back_page, 64, 64
	);
	*boss_left_on_back_page = boss_left_on_page[page_front];
	*boss_top_on_back_page = boss_top_on_page[page_front];
	return midboss4_active;
}


// One frame of the midboss's sprite, wrapped around the 400-line VRAM the way
// super_roll_put() needs. Blits it in white for the single frame after it was
// hit, and lowers [boss_hit_flash] again in the same breath.
static void near midboss4_19F52(void)
{
	// Split across two statements, not `y = (marisa_topleft.y + scroll_line)`:
	// the one-expression form computes the sum in AX and moves it into SI
	// afterwards, where the original loads SI directly. (kb/codegen/0146)
	register screen_y_t y;

	y = marisa_topleft.y;
	y += scroll_line;
	if(y < 0) {
		y += RES_Y;
	} else if(y >= RES_Y) {
		y -= RES_Y;
	}
	if(boss_hit_flash) {
		snd_se_play(4);
		super_roll_put_1plane(
			marisa_topleft.x, y, patnum_2064E, 0, super_plane(V_WHITE)
		);
		boss_hit_flash = false;
	} else {
		super_roll_put(marisa_topleft.x, y, patnum_2064E);
	}
}


// The midboss's defeat animation: a 24-spark burst every 24 frames, over a
// super_zoom() factor that grows by 1 every 8 frames for 64 frames. Ends by
// parking [boss_pos_x] / [boss_pos_y] outside the playfield, handing
// [marisa_intro_direction] to the next appearance as -1, and re-arming
// [midboss_scroll_step].
static void near midboss4_19FAF(void)
{
	int zoom;

	if((boss_phase_frame % 24) == 0) {
		sparks_add(
			(marisa_topleft.x + 32),
			(marisa_topleft.y + 32),
			to_sp(8.0f),
			24,
			true
		);
	}
	zoom = 10;
	zoom += (midboss4_defeat_frame >> 3);
	midboss4_defeat_frame++;
	if(midboss4_defeat_frame >= 64) {
		midboss4_defeat_frame = 0;
		boss_phase_frame = 0;
		boss_phase = 2;
		boss_pos_x = -1;
		boss_pos_y = -1;
		midboss4_active = false;
		marisa_intro_direction = -1;
		midboss_scroll_step = MIDBOSS4_SCROLL_STEP;
	} else {
		register screen_y_t y;

		y = marisa_topleft.y;
		y += scroll_line;
		if(y >= RES_Y) {
			y -= RES_Y;
		}
		super_zoom(marisa_topleft.x, y, zoom, 2);
	}
}


// The midboss's hit test. Its hitbox is 48x48 from [marisa_topleft] + (8, 0),
// and unlike every boss it cannot collide with the player at all.
//
// `[measured]` It is only defeated at 400 damage AND while its wrapped VRAM
// row is between 96 and 336, so a hit that would finish it off during the wrap
// does nothing and has to be repeated. The drop is an extra life on the first
// appearance and a bomb on every later one, which is what makes
// [midboss_scroll_step] readable as an appearance counter here.
static void near midboss4_1A044(void)
{
	// One local for both roles, which is what the original's register
	// allocation says: [damage] and the item ID are the same DI across the
	// whole body, and [y] is the only `register`, so a second local of its own
	// would have had to land on the frame. Assigned and tested in one
	// expression so that the test stays `or ax, ax` (kb/codegen/0143); the
	// three extra references the item reuse adds are what lifts it off the
	// frame and into DI in the first place.
	int damage;
	register screen_y_t y;

	if((damage = shots_hittest(
		(marisa_topleft.x + 8), marisa_topleft.y, 48, 48
	)) != 0) {
		boss_hit_flash = true;
		boss_damage += damage;
		y = marisa_topleft.y;
		y += scroll_line;
		if(y >= RES_Y) {
			y -= RES_Y;
		}
		if((boss_damage >= 400) && (y < 336) && (y > 96)) {
			snd_se_play(2);
			boss_phase = 1;
			score_delta += 50000;
			if(midboss_scroll_step == MIDBOSS4_SCROLL_STEP) {
				damage = IT_1UP;
			} else {
				damage = IT_BOMB;
			}
			items_add((marisa_topleft.x + 24), marisa_topleft.y, damage);
		}
	}
}


// Pattern 0: one straight pellet every 16 frames.
static void near midboss4_1A0CE(void)
{
	if((boss_phase_frame % 16) == 0) {
		snd_se_play(3);
		bullets_add_pellet(
			MIDBOSS4_MUZZLE_LEFT, (PLAYFIELD_TOP + 80), 0x40, BG_1, to_sp(5.0f)
		);
	}
}


// Pattern 1: a spread every 16 frames, at a speed that grows with the frame
// number rather than being constant - so the spread gets faster the longer the
// pattern runs.
static void near midboss4_1A103(void)
{
	uint8_t group;

	if((boss_phase_frame % 16) == 0) {
		snd_se_play(3);
		// A conditional expression rather than two assignments: the original
		// picks the group in AL and stores it to the frame once, at the join.
		group = (
			(rank == RANK_EASY) ? BG_4_SPREAD_WIDE : BG_5_SPREAD_MEDIUM
		);
		bullets_add_pellet(
			MIDBOSS4_MUZZLE_LEFT,
			(PLAYFIELD_TOP + 80),
			0x40,
			group,
			(boss_phase_frame - to_sp(1.25f))
		);
	}
}


// Pattern 2: a single 16x16 ball every 8 frames, and the only pattern that
// plays no sound effect of its own.
static void near midboss4_1A151(void)
{
	if((boss_phase_frame % 8) == 0) {
		bullets_add_16x16(
			MIDBOSS4_MUZZLE_LEFT,
			(PLAYFIELD_TOP + 80),
			0x40,
			BSM_CHASE,
			PAT_BULLET16_OUTLINED_BALL_BEIGE,
			8
		);
	}
}


// Pattern 3: a laser every (16 - (rank * 2)) frames, which is the only place
// in this entity where [rank] changes the rate rather than the shape.
static void near midboss4_1A17E(void)
{
	if((boss_phase_frame % (16 - (rank * 2))) == 0) {
		laser_wait_frames = 16;
		lasers_add(MIDBOSS4_MUZZLE_LEFT, 80, 6, 0x6F);
	}
}


// Pattern 4: four randomly aimed pellets at once, every 16 frames, and the
// only pattern that spawns from [midboss4_pellet_top] rather than from the
// fixed (PLAYFIELD_TOP + 80).
static void near midboss4_1A1B6(void)
{
	register int i;

	if((boss_phase_frame % 16) == 0) {
		snd_se_play(7);
		for(i = 0; i < 4; i++) {
			bullets_add_pellet(
				MIDBOSS4_MUZZLE_LEFT,
				midboss4_pellet_top,
				(randring2_next8_and(0x3F) + 0x20),
				BG_1,
				to_sp(4.375f)
			);
		}
	}
}


// The midboss's entire per-frame update. Flies in diagonally for 32 frames
// while firing a pellet every 8th one, then sweeps horizontally, reversing
// direction and re-rolling its pattern every 114 frames until it has fired 12
// of them, and finally leaves through whichever side of the playfield it
// reaches first.
void midboss4_update_and_render(void)
{
	boss_pos_x = (marisa_topleft.x + 24);
	boss_pos_y = (marisa_topleft.y + 24);
	boss_phase_frame++;
	if(boss_phase_frame == 1) {
		if(marisa_intro_direction == 0) {
			boss_left_on_page[0] = (PLAYFIELD_LEFT + 32);
			patnum_2064E = 150;
		} else {
			boss_left_on_page[0] = (PLAYFIELD_RIGHT - 32 - 64);
			patnum_2064E = 149;
		}
		boss_left_on_page[1] = boss_left_on_page[0];
		boss_top_on_page[0] = (PLAYFIELD_TOP - 32);
		boss_top_on_page[1] = (PLAYFIELD_TOP - 32);
		boss_left_on_back_page = boss_left_on_page;
		boss_top_on_back_page = boss_top_on_page;
		boss_damage = 0;
		boss_phase = 0;
		boss_hit_flash = false;
		marisa_intro_step = 0;
		// Again a conditional expression rather than two assignments: the
		// original picks the sign in AX and stores it once, at the join.
		marisa_intro_direction = ((marisa_intro_direction == 0) ? 1 : -1);
		midboss4_patterns_seen = 0;
		midboss4_pattern = 0;
		midboss4_active = true;
	}
	midboss4_pellet_top = (*boss_top_on_back_page + 48);
	marisa_topleft.y = *boss_top_on_back_page;
	if(boss_phase == 0) {
		if(marisa_intro_step == 0) {
			*boss_left_on_back_page += (marisa_intro_direction << 3);
			*boss_top_on_back_page += 2;
			marisa_topleft.x = *boss_left_on_back_page;
			marisa_topleft.y = *boss_top_on_back_page;
			if(!(boss_phase_frame & 7)) {
				snd_se_play(7);
				bullets_add_pellet(
					MIDBOSS4_MUZZLE_LEFT,
					midboss4_pellet_top,
					0x40,
					BG_1,
					to_sp(5.0f)
				);
			}
			if(boss_phase_frame >= 32) {
				marisa_intro_step = 1;
				boss_phase_frame = 2; // Skip the fly-in animation
				patnum_2064E = 148;
				marisa_intro_direction *= -1;
				midboss4_pattern = (randring2_next8() % 5);
			}
		} else if(marisa_intro_step == 1) {
			if(boss_phase_frame > 50) {
				if(marisa_intro_direction == -1) {
					patnum_2064E = 149;
				} else {
					patnum_2064E = 150;
				}
				*boss_left_on_back_page += (marisa_intro_direction << 2);
				marisa_topleft.x = *boss_left_on_back_page;
				switch(midboss4_pattern) {
				case 0:	midboss4_1A0CE();	break;
				case 1:	midboss4_1A103();	break;
				case 2:	midboss4_1A151();	break;
				case 3:	midboss4_1A17E();	break;
				case 4:	midboss4_1A1B6();	break;
				}
				if(midboss4_patterns_seen < 12) {
					if(boss_phase_frame >= 114) {
						boss_phase_frame = 2; // Skip the fly-in animation
						patnum_2064E = 148;
						marisa_intro_direction *= -1;
						midboss4_pattern = (randring2_next8() % 5);
						midboss4_patterns_seen++;
					}
				} else if(
					(marisa_topleft.x <= 0) || (marisa_topleft.x >= 384)
				) {
					boss_phase_frame = 0;
					marisa_intro_direction = -1;
					midboss_scroll_step = MIDBOSS4_SCROLL_STEP;
					midboss4_active = false;
					return;
				}
			}
		}
	}
	if(boss_phase != 0) {
		midboss4_19FAF();
		return;
	}
	midboss4_1A044();
	midboss4_19F52();
}
