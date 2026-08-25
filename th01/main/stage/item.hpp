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

struct t1replay_checkpoint_items_t;

void t1replay_items_checkpoint_export(t1replay_checkpoint_items_t *checkpoint);
void t1replay_items_checkpoint_import(
	const t1replay_checkpoint_items_t *checkpoint
);
