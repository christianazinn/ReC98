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

// [inferred], not [measured]: derived only from the 4608-byte extent of
// [enemy_templates] divided by the 36-byte stride. Nothing bounds-checks the
// template index anywhere in the binary, so no loop proves this number.
static const int ENEMY_TEMPLATE_COUNT = 128;

extern enemy_template_t enemy_templates[ENEMY_TEMPLATE_COUNT];

/// Scripts
/// -------
/// A flat pool of fixed-size slots that templates point into. A template
/// either names an already-loaded script by index (the 0xFE escape in the
/// file) or carries a fresh 0xFF-terminated one, which is copied into the next
/// free slot.

static const int ENEMY_SCRIPT_SIZE = 64;
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
