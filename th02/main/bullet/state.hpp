#ifndef TH02_MAIN_BULLET_STATE_HPP
#define TH02_MAIN_BULLET_STATE_HPP

// Private native state shared only by bullet.cpp and the checkpoint codec.
// Public consumers must use bullet.hpp's routines, not this BSS ABI.

#include <stddef.h>
#include "th01/math/subpixel.hpp"
#include "th02/sprites/main_pat.h"

// bullet.hpp intentionally predates include guards. Every private consumer
// includes that public vocabulary first, then includes this state-only ABI.

enum bullet_size_type_t {
	BST_PELLET = 1,
	BST_BULLET16 = 2,
};

// bullet.cpp originally declared this before its later -a2 pragma. Preserve
// the default byte-packed native representation even if this private header
// is accidentally included from another local pragma scope.
#pragma pack(push, 1)
struct bullet_t {
	int8_t flag; // ACTUAL TYPE: entity_flag_t
	int8_t size_type; // ACTUAL TYPE: bullet_size_type_t
	SPPoint screen_topleft[PAGE_COUNT];
	SPPoint velocity;
	uint8_t patnum; // ACTUAL TYPE: main_patnum_t
	bullet_group_or_special_motion_t group_or_special_motion;
	unsigned char angle;
	SubpixelLength8 speed;
	union {
		uint8_t special_frame;
		uint8_t turns_done;
		uint8_t v;
	} u1;
	int8_t padding;
};
#pragma pack(pop)

// These checks live in the private include boundary. The state header is only
// valid when Subpixel's byte-sized specialization is the original one; a
// future include after a conflicting local packing pragma must fail loudly.
typedef char bullet_subpixel_layout_check[(sizeof(Subpixel) == 0x02) ? 1 : -1];
typedef char bullet_sppoint_layout_check[(sizeof(SPPoint) == 0x04) ? 1 : -1];
typedef char bullet_speed_layout_check[(
	sizeof(SubpixelLength8) == 0x01
) ? 1 : -1];
typedef char bullet_t_layout_check[(
	(sizeof(bullet_t) == 0x14) &&
	(offsetof(bullet_t, flag) == 0x00) &&
	(offsetof(bullet_t, size_type) == 0x01) &&
	(offsetof(bullet_t, screen_topleft) == 0x02) &&
	(offsetof(bullet_t, velocity) == 0x0A) &&
	(offsetof(bullet_t, patnum) == 0x0E) &&
	(offsetof(bullet_t, group_or_special_motion) == 0x0F) &&
	(offsetof(bullet_t, angle) == 0x10) &&
	(offsetof(bullet_t, speed) == 0x11) &&
	(offsetof(bullet_t, u1) == 0x12) &&
	(offsetof(bullet_t, padding) == 0x13)
) ? 1 : -1];

extern bullet_t bullets[BULLET_COUNT];
extern Subpixel8 rank_base_speed;
extern uint8_t rank_base_stack;
extern uint8_t bullet_stack;
extern int8_t easy_slow_skip_cycle;

#endif /* TH02_MAIN_BULLET_STATE_HPP */
