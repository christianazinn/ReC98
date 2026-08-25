#include <stddef.h>
#include "pc98.h"
#include "th02/main/entity.hpp"

// Significant, internal digits. An additional 0 is appended to the on-screen
// representation.
static const int POINTNUM_DIGITS = 4;
static const int POINTNUM_COUNT = 20;

// pointnum.cpp declared this after its local -a2. Keep that native layout
// explicit while returning every include site to its prior packing state.
#pragma pack(push, 2)
struct CPointnums {
	vc_t col;
	int8_t unused;
	screen_x_t left[POINTNUM_COUNT];
	screen_y_t top[POINTNUM_COUNT][PAGE_COUNT];
	uint16_t points[POINTNUM_COUNT];
	entity_flag_t flag[POINTNUM_COUNT];
	uint8_t age[POINTNUM_COUNT];
	uint8_t op;
	uint8_t operand;
};
#pragma pack(pop)

// This record used to be private to pointnum.cpp, after its local -a2
// pragma. Keeping the proof in the header makes every include site validate
// the original pointnum BSS ABI.
typedef char pointnums_layout_check[(
	(sizeof(CPointnums) == 0xCC) &&
	(offsetof(CPointnums, col) == 0x00) &&
	(offsetof(CPointnums, unused) == 0x01) &&
	(offsetof(CPointnums, left) == 0x02) &&
	(offsetof(CPointnums, top) == 0x2A) &&
	(offsetof(CPointnums, points) == 0x7A) &&
	(offsetof(CPointnums, flag) == 0xA2) &&
	(offsetof(CPointnums, age) == 0xB6) &&
	(offsetof(CPointnums, op) == 0xCA) &&
	(offsetof(CPointnums, operand) == 0xCB)
) ? 1 : -1];

extern CPointnums pointnums;

// ZUN landmine: [points] is not limited to POINTNUM_DIGITS. Larger values will
// be truncated to their least significant POINTNUM_DIGITS, with their first
// digit being rendered as a glitched sprite.
void pascal near pointnums_add(
	screen_x_t left, screen_y_t top, uint16_t points
);

// These are called from the respective item functions.
void near pointnums_init_for_rank_and_reset(void);
void near pointnums_invalidate(void);
void near pointnums_update_and_render(void);
