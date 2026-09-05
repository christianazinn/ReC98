/// TH02's regular stage enemies
/// ----------------------------
/// Structures and globals that the stage's `STAGE?.DT1` file fills in.
/// Derived statically in state/notes/th02-enemy-records.md; the parcel that
/// wrote this header only decompiles the *loader*, so every field it does not
/// itself touch carries that note's evidence label rather than this one's.

#ifndef TH02_MAIN_ENEMY_ENEMY_HPP
#define TH02_MAIN_ENEMY_ENEMY_HPP

#include "platform.h"
#include "pc98.h"
#include "th02/main/entity.hpp"

/// Slots
/// -----
/// One per spawned enemy. th02/main/enemy/enemy.inc is the ASM mirror, and it
/// is the mirror the oracle grades: every field name it defines is substituted
/// into th02_main.asm in place of the displacement it replaces, so a wrong
/// offset changes the assembled bytes. Keep the two in step.

struct enemy_t {
	// Screen pixels, not subpixels — the velocity added to these is a signed
	// *byte*, and they are handed straight to super_roll_put(). One pair per
	// VRAM page, because TH02 unblits an enemy from where it was drawn on the
	// page being rebuilt rather than from a temporal `prev`; TH04 and TH05
	// have no per-page positions at all.
	screen_point_t pos_on_page[PAGE_COUNT]; // +0x00

	int script_ip;               // +0x08, byte offset into a 64-byte script

	// Frames since the enemy spawned. Doubles as the explosion cel timer once
	// [in_kill_anim] is set.
	int age;                     // +0x0A

	int template_id;             // +0x0C, index into [enemy_templates]
	entity_flag_t flag;          // +0x0E

	// Seeded from randring2_next8() rather than from 0, so two enemies of the
	// same type spawned on the same frame do not animate in lockstep.
	uint8_t anim_frame;          // +0x0F

	bool in_kill_anim;           // +0x10, suppresses the script VM
	uint8_t unused_1;            // +0x11, [open]: no instruction touches it
	int patnum_delta;            // +0x12, added to the template's base patnum

	// 0 = don't render, 1 = template patnum + [patnum_delta], 2 =
	// [patnum_delta] is the absolute patnum.
	int render_as;               // +0x14

	// The angle for this enemy's next shot — and, in script opcodes 161 and
	// 162 only, a loop counter. ZUN reuses the one field for both; opcode 161
	// increments it and compares it against the script's immediate, and the
	// shooting opcodes push only its low byte.
	int angle;                   // +0x16

	bool16 spawned_in_left_half; // +0x18, picks the sign of a few ±1 offsets
	int loop_i;                  // +0x1A, script opcode 167's loop counter
	pixel_delta_8_t velocity_x;  // +0x1C, signed: added through a `cbw`
	pixel_delta_8_t velocity_y;  // +0x1D

	// Normally an enemy only despawns on leaving the playfield horizontally.
	bool despawn_when_offscreen_vertically; // +0x1E

	uint8_t unused_2;            // +0x1F, [open]: no instruction touches it
	int damage;                  // +0x20, accumulated, compared against `hp`

	// Both are the *inverses* of TH04's and TH05's `can_be_damaged` and
	// `kills_player_on_collision`, and both are set by the script.
	bool not_shootable;          // +0x22
	bool no_player_collision;    // +0x23

	uint8_t pellet_group;        // +0x24, ACTUAL TYPE: bullet_group_or_special_motion_t
	uint8_t pellet_speed;        // +0x25
};

static const int ENEMY_COUNT = 25;

extern enemy_t enemies[ENEMY_COUNT];

// The upper exclusive slot bound for the live enemy pass. This is a semantic
// loop cursor, not an array capacity.
extern "C" uint8_t enemies_loop_bound;

/// Templates
/// ---------
/// One per enemy *type*, not per spawned enemy. The first 0x1C bytes of each
/// are read verbatim out of `STAGE?.DT1`; the remaining 8 are derived by
/// enemy_stagedata_load() from the sprite metrics and the script pool.

struct enemy_template_t {
	int unused;                  // +0x00, [open]: no instruction touches it
	screen_y_t spawn_top;        // +0x02
	pixel_t bullet_origin_x;     // +0x04
	pixel_t bullet_origin_y;     // +0x06
	int patnum;                  // +0x08, first sprite of the animation
	int anim_cels;               // +0x0A
	int anim_frames_per_cel;     // +0x0C
	int explode_sprite;          // +0x0E
	uint8_t item;                // +0x10
	uint8_t unused_2;            // +0x11, [open]
	long score;                  // +0x12
	int hp;                      // +0x16
	bool no_player_collision;    // +0x18
	uint8_t unused_3;            // +0x19, [open]

	// Frames between two automatic shots. Scaled by rank at load time: doubled
	// on Easy, halved on Hard and Lunatic, i.e. the *interval* moves opposite
	// to the difficulty. (This is why +0x1A is the autofire interval and not
	// the HP at +0x16 — scaling HP this way would make Easy the harder rank.)
	int autofire_interval;       // +0x1A

	// Sprite size, derived from master.lib's [super_patsize] entry for
	// [patnum] rather than stored in the file.
	pixel_t w;                   // +0x1C
	pixel_t h;                   // +0x1E

	// Points into [enemy_scripts]. Far, because the enemy update code reaches
	// it from another group.
	const uint8_t far *script;   // +0x20
};

// Bytes of each template that come straight out of the file. The other 8 are
// derived on load.
static const int ENEMY_TEMPLATE_FILE_SIZE = 0x1C;

// A CEILING, not a count, and [inferred] rather than [measured]: it is the
// 4608-byte extent of [enemy_templates] divided by the 36-byte stride, and
// nothing else. `byte_25976` has exactly two instruction references in the
// whole binary and neither range-checks anything; enemy_stagedata_load() takes
// the number of templates straight out of the file with no clamp.
//
// ZUN landmine: A stage file that declares more than this many templates
// overruns the array into [spawn_grid].
static const int ENEMY_TEMPLATE_COUNT = 128;

extern enemy_template_t enemy_templates[ENEMY_TEMPLATE_COUNT];

/// Scripts
/// -------
/// A flat pool of fixed-size slots that templates point into. A template
/// either names an already-loaded script by index (the 0xFE escape in the
/// file) or carries a fresh 0xFF-terminated one, which is copied into the next
/// free slot.

static const int ENEMY_SCRIPT_SIZE = 64;

// Same status as ENEMY_TEMPLATE_COUNT above: a ceiling, [inferred] from the
// 2560-byte extent of [enemy_scripts] divided by the 64-byte stride, and not a
// number any instruction knows. `byte_22FDC` and `byte_22FDB` have *zero*
// instruction references left in the dump now that the loader is C++.
static const int ENEMY_SCRIPT_COUNT = 40;

extern uint8_t enemy_scripts[ENEMY_SCRIPT_COUNT][ENEMY_SCRIPT_SIZE];

// Number of slots of [enemy_scripts] used so far, and therefore the index of
// the next free one. Reset to 0 at the top of every stage load.
// ZUN landmine: Never bounds-checked against ENEMY_SCRIPT_COUNT.
extern uint8_t enemy_scripts_used;

/// Spawn grid
/// ----------
/// [SPAWN_COLUMN_COUNT] parallel arrays of [spawn_rows] entries each, one row
/// per scroll step of the stage. Column 0 is read out of the file as words;
/// every other column is read as bytes, with 0xFF widened to -1 rather than to
/// 255.

static const int SPAWN_COLUMN_COUNT = 0x31;

extern int far *spawn_grid[SPAWN_COLUMN_COUNT];
extern int spawn_rows;
/// ----------

// Frees every column of [spawn_grid] and nulls it. Called immediately before
// the loader below, so a stage load never leaks the previous stage's grid.
extern "C" void far enemy_stagedata_free(void);

// Loads `STAGE?.DT1` for the current [stage_id]: the tile map, the enemy
// templates with their scripts, and the spawn grid.
extern "C" void far enemy_stagedata_load(void);

#endif /* TH02_MAIN_ENEMY_ENEMY_HPP */
