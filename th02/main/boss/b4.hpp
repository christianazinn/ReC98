// Stage 4 boss state
// ------------------

#include "pc98.h"

/// Orbs
/// ----
/// The four orbs Marisa summons around herself, spins outwards at a growing
/// radius, and connects with lines into a rotating ring. Each one is shot down
/// separately, and drops an item when it is.

static const int MARISA_ORB_COUNT = 4;

enum marisa_orb_flag_t {
	MOF_ALIVE = 0,
	MOF_KILL_ANIM = 1,
	MOF_REMOVED = 2,

	_marisa_orb_flag_t_FORCE_INT16 = 0x7FFF,
};

extern marisa_orb_flag_t marisa_orb_flag[MARISA_ORB_COUNT];

// Same page-indexed indirection as used for the boss itself (boss.hpp), except
// that the cached pointers are far ones, and that the two arrays *are*
// contiguous in memory. The remaining four per-orb arrays are still ASM-only:
// [marisa_orb_damage] (raised by 1 per frame in which any player shot overlaps
// the orb, and the orb is removed at 110), [marisa_orb_hit_flash] (set on the
// same frame, and cleared by the renderer after one white blit), and
// [marisa_orb_radius] and its angle, which together place the orb relative to
// Marisa's own center.
extern screen_x_t marisa_orb_left_on_page[PAGE_COUNT][MARISA_ORB_COUNT];
extern screen_y_t marisa_orb_top_on_page[PAGE_COUNT][MARISA_ORB_COUNT];
extern screen_x_t* marisa_orb_left_on_back_page[MARISA_ORB_COUNT];
extern screen_y_t* marisa_orb_top_on_back_page[MARISA_ORB_COUNT];
/// ----
// ------------------
