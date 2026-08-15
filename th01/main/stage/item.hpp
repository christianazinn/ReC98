/// Bombs
/// -----

// Spawns a new bomb item from the position of the card in the given slot.
void items_bomb_add(int from_card_slot);

void items_bomb_render(void);
void items_bomb_reset(void);
void items_bomb_unput_update_render(void);
/// -----

/// Point
/// -----

// Spawns a new point item from the position of the card in the given slot.
void items_point_add(int from_card_slot);

void items_point_render(void);
void items_point_reset(void);
void items_point_unput_update_render(void);
/// -----

inline void items_render(void) {
	items_bomb_render();
	items_point_render();
}

inline void items_reset(void) {
	items_bomb_reset();
	items_point_reset();
}

inline void items_unput_update_render(void) {
	items_bomb_unput_update_render();
	items_point_unput_update_render();
}

#ifdef T1CASE
/// ORACLE-TH01 (mod branch only): read-only access for the T1SPLIT group-5
/// subsystem hash, mirroring CPellets::t1case_slot()
/// (th01/main/bullet/pellet.hpp:234-245).
///
/// An accessor rather than an `extern` because `struct item_t`,
/// `union item_flag_state_t` and both ITEM_*_COUNT constants are defined
/// INSIDE th01/main/stage/item.cpp and are unnameable from any other
/// translation unit — the arrays themselves have external linkage, but their
/// type does not.
///
/// Slots [0, T1CASE_ITEM_BOMBS) are [items_bomb], the rest [items_point].
#define T1CASE_ITEM_BOMBS  4
#define T1CASE_ITEM_POINTS 10
#define T1CASE_ITEM_SLOTS  (T1CASE_ITEM_BOMBS + T1CASE_ITEM_POINTS)

// Number of `int`s t1case_item_get() writes: left, top, velocity_y, flag,
// flag_state — a fixed declared order of FIELDS, never a struct image
// (TXSPLIT_CONTRACT.md §7). `unknown_zero` is excluded: item.cpp:90 writes it
// zero and nothing ever reads it.
#define T1CASE_ITEM_FIELDS 5

void t1case_item_get(int slot, int far *out);

// Slots whose `flag` is not IF_FREE.
int t1case_items_alive(void);
#endif
