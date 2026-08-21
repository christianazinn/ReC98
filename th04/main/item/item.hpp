#ifndef TH04_MAIN_ITEM_ITEM_HPP
#define TH04_MAIN_ITEM_ITEM_HPP

// Guarded because th04/enm_pos.cpp now reaches this file twice: once from the
// midboss4_update() lift at its front, and once through
// th04/main/enemy/enemy.hpp on the way to the two objects behind it. A second
// expansion rejects every `static const` and every enumerator in it
// (kb/codegen/0129). Byte-inert, and the same fix th04/main/phase.hpp already
// carries for the same reason.

#include "th04/main/playfld.hpp"
#include "th02/main/entity.hpp"

enum item_type_t {
#if GAME == 5
	IT_NONE = -2,
#endif
	IT_ENEMY_DROP_NEXT = -1,
	IT_POWER = 0,
	IT_POINT = 1,
	IT_DREAM = 2,
	IT_BIGPOWER = 3,
	IT_BOMB = 4,
	IT_1UP = 5,
	IT_FULLPOWER = 6,
	IT_COUNT,
};

struct item_t {
	entity_flag_t flag;
	char unused; // ZUN bloat
	PlayfieldMotion pos;
	unsigned char type;
	char unknown;
	int patnum;	// Assumed to be a 16×16 sprite.
	bool16 pulled_to_player;
};

#define ITEM_W 16
#define ITEM_H 16
#define ITEM_PULL_SPEED 10

static const int ITEM_COUNT = ((GAME == 5) ? 40 : 32);

extern item_t items[ITEM_COUNT];

extern const int ITEM_PATNUM[IT_COUNT];

// Called once per stage, from stage_init(). Seeds [enemy_drop_ring_p], resets
// the item splashes, and clears the two variables below it -- but does NOT
// clear the item array, which is why this is not TH02's items_init_and_reset().
// `far` and `extern "C"` because each dump published it undecorated beside a
// `proc far`.
extern "C" void far items_init(void);

void pascal near items_add(subpixel_t x, subpixel_t y, item_type_t type);

// The fixed ring of item types that IT_ENEMY_DROP_NEXT resolves to. Both games
// have 64 entries, in a different order; TH02's ITEM_SEMIRANDOM_RING is the
// same idea, down to the randomly seeded starting position.
#define ENEMY_DROP_RING_SIZE 64
extern const item_type_t ENEMY_DROPS[ENEMY_DROP_RING_SIZE];

// Position in ENEMY_DROPS, seeded with a random value in [0, 15] when the item
// subsystem is reset. Advanced once per REQUESTED enemy drop rather than once
// per spawned one, so the ring index is half of it and every other request
// spawns nothing.
extern unsigned char enemy_drop_ring_p;

// Increments or decrements [playperf] when reaching certain values.
extern unsigned char item_playperf_raise;
extern unsigned char item_playperf_lower;

#if GAME == 5
// Cleared by items_init() and read by nothing, in this game or in our tree:
// that single `mov` was its complete reference set in the dump. TH04 clears
// [dream_score] in the same slot, so this is most likely its vestige -- but
// the name records only what is measured, exactly as stage_init()'s own
// [stage_init_unused_0..3] do. ZUN bloat.
extern unsigned int items_init_unused;

extern unsigned int item_point_score_at_full_dream;

// The "dream" meter shown in the HUD, raised by collecting point items and
// capped at BAR_MAX. Every point item collected at BAR_MAX is worth
// [item_point_score_at_full_dream].
extern unsigned char dream;
#else
// The point value of a single point item, raised by collecting point items
// without missing. The HUD shows this ×10.
extern unsigned int dream_score;
#endif

// Items dropped when losing a life
// --------------------------------
#define ITEM_MISS_COUNT 5
typedef enum {
	MISS_FIELD_LEFT = 0,
	MISS_FIELD_CENTER = 1,
	MISS_FIELD_RIGHT = 2,
	MISS_FIELD_COUNT,
};
// Yes, these have Y first and X second.
extern const Subpixel ITEM_MISS_VELOCITIES[MISS_FIELD_COUNT][2][ITEM_MISS_COUNT];

// `far` and `extern "C"`, both measured rather than chosen: the assembly
// module that then held this body defined it `proc far` and published the
// undecorated, all-caps spelling of this name, which is Borland's decoration
// for a pascal function with C linkage (kb/codegen 0081 + 0086). It is C++
// itself now, in th04/main/item/miss_add.cpp, and `tcc -S` on that file emits
// the same `proc far` and the same public (kb/codegen/0152). No source file
// spells that decorated form any more, which is why it is described here
// rather than quoted. The declaration
// said `near` and had no `extern "C"` until th04/main/player/miss.cpp became
// the first C++ that ever called it; nothing had graded it before, because no
// translation unit both included this header and made the call.
extern "C" void pascal far items_miss_add(void);
// --------------------------------

// Collection counters
// -------------------

// Reset to 0 when moving to a new stage. TH04 stores this in a single byte,
// and increments it without TH05's POINT_ITEMS_MAX cap.
#if (GAME == 5)
extern unsigned int stage_point_items_collected;
#else
extern unsigned char stage_point_items_collected;
#endif

extern unsigned int items_spawned;
extern unsigned int items_collected;

#if GAME == 5
// Same value as [total_point_items_collected].
// Used for extends and end-of-game score bonus calculation.
extern unsigned int extend_point_items_collected;
#endif

extern unsigned int total_point_items_collected;

// TH04 includes items collected above the PoC during a bomb, TH05 doesn't.
extern unsigned int total_max_valued_point_items_collected;
// -------------------

extern bool items_pull_to_player;

void near items_invalidate();

#endif /* TH04_MAIN_ITEM_ITEM_HPP */
