#include <stddef.h>
#include "pc98.h"
#include "th01/math/subpixel.hpp"
#include "th02/main/entity.hpp"
#include "th02/main/score.hpp"

enum item_type_t {
	IT_POWER = 0,
	IT_POINT = 1,
	IT_BOMB = 2,
	IT_BIGPOWER = 3,
	IT_1UP = 4,
	IT_COUNT,

	_item_type_t_FORCE_UINT8 = 0xFF
};

#define ITEM_COUNT 20

// Both native records were originally declared under the default -a1 in
// item.cpp. The push/pop prevents an include from inheriting a caller's local
// -a2 and restores that caller's state immediately afterwards.
#pragma pack(push, 1)
struct item_pos_t {
	screen_x_t screen_left;
	Subpixel screen_top;
};

struct item_t {
	entity_flag_t flag;
	item_type_t type;
	item_pos_t pos[PAGE_COUNT];
	Subpixel velocity_y;
	pixel_t velocity_x_during_bounce;
	int age;
};
#pragma pack(pop)

// Kept next to the shared declaration so an include before a local -a2
// pragma cannot silently alter the native slot layout.
typedef char item_pos_t_layout_check[(
	(sizeof(item_pos_t) == 0x04) &&
	(offsetof(item_pos_t, screen_left) == 0x00) &&
	(offsetof(item_pos_t, screen_top) == 0x02)
) ? 1 : -1];
typedef char item_t_layout_check[(
	(sizeof(item_t) == 0x10) &&
	(offsetof(item_t, flag) == 0x00) &&
	(offsetof(item_t, type) == 0x01) &&
	(offsetof(item_t, pos) == 0x02) &&
	(offsetof(item_t, velocity_y) == 0x0A) &&
	(offsetof(item_t, velocity_x_during_bounce) == 0x0C) &&
	(offsetof(item_t, age) == 0x0E)
) ? 1 : -1];

extern item_t items[ITEM_COUNT];

// Turns the next N power or point items spawned via items_add() into big power
// items. Used for recharging power after using a continue after a Game Over.
extern unsigned int item_bigpower_override;

// Spawns the Game Over item set on the next call to items_miss_add().
// ZUN bloat: Both turning this into a parameter or hardcoding the condition
// (as TH04 and TH05 do it) would have been better than this.
extern bool items_miss_add_gameover;

extern "C" uint8_t item_semirandom_ring_p;
extern "C" uint8_t item_semirandom_cycle;
extern "C" uint8_t item_drop_cycle;
extern "C" uint8_t item_collect_skill;
extern score_t item_score_this_frame;

// Also picks a new starting point inside the hardcoded randomized item cycle
// used by items_add_semirandom().
void near items_init_and_reset(void);

// Unconditionally spawns a big power item if [item_bigpower_override] is > 0.
// Otherwise, spawns a bomb item with a 1-in-512 chance, or the next item along
// a hardcoded ring of power and point items.
void pascal items_add_semirandom(screen_x_t left, screen_y_t top);

void pascal items_add(int type, screen_x_t left, screen_y_t top);

// Spawns each of the items dropped when losing a life at the given position.
void pascal near items_miss_add(screen_x_t left, screen_y_t top);

void near items_invalidate(void);

void near items_update_and_render(void);
