#ifndef TH04_MAIN_BOSS_B6ENT_HPP
#define TH04_MAIN_BOSS_B6ENT_HPP

// Extra Stage Boss Yuuka's two [custom_entities] overlays, and her fly path.
//
// Guarded, and split out of th04/main/boss/b6.cpp by MATCH-TH04-MAIN-034-CHAIN,
// because TWO objects now need these layouts:
//
//   th04/boss_bg.cpp   -- through th04/main/boss/bg.cpp, which expands b6.cpp
//                         and hosts chasecrosses_add() and safetycircle_open();
//   th04/b6_next.cpp   -- which hosts yuuka6_customs_update(), the one body
//                         that walks both overlays in one frame.
//
// It has to be a header rather than more of b6.cpp, and that is `[measured]`
// rather than a preference. b6.cpp declares yuuka6_phase_next() and the
// animation drivers, which th04/b6_next.cpp's object *defines* or re-declares
// with different linkage, so including that file there is a redeclaration
// error. Restating two struct layouts instead is what this campaign has just
// been burned by: b6.cpp's own safetycircle_t carried a four-byte gap as eight
// for as long as no C++ read past it, and only a one-byte-red oracle found it.
// One definition, two includers.

#include "th04/main/custom.hpp"

// Chasing cross bullets
// ---------------------

static const pixel_t CHASECROSS_W = 32;
static const pixel_t CHASECROSS_H = 32;
static const int CHASECROSS_KILL_FRAMES_PER_CEL = 4;

// One short of the block, because the safety circle below overlays the last
// slot. chasecrosses_add() nevertheless scans one PAST this; see the landmine
// it records in th04/main/boss/b6_spawn.cpp.
#define CHASECROSS_COUNT (CUSTOM_COUNT - 1)

enum chasecross_flag_t {
	CCF_FREE = 0,
	CCF_ALIVE = 1,
	CCF_KILL_ANIM = (PAT_ENEMY_KILL * CHASECROSS_KILL_FRAMES_PER_CEL),
	CCF_KILL_ANIM_END = (
		CCF_KILL_ANIM + (ENEMY_KILL_CELS * CHASECROSS_KILL_FRAMES_PER_CEL)
	),

	_chasecross_flag_t_FORCE_UINT8 = 0xFF
};

struct chasecross_t {
	chasecross_flag_t flag;
	unsigned char angle;
	PlayfieldPoint center;
	/* ------------------------- */ int8_t unused_1[4];
	PlayfieldPoint velocity;
	unsigned int age;
	/* ------------------------- */ int8_t unused_2[4];
	int hp;
	int damage_this_frame;
	SubpixelLength8 speed;
	/* ------------------------- */ int8_t padding;
};

#define chasecrosses (reinterpret_cast<chasecross_t *>(custom_entities))
// ---------------------

// Safety circle
// -------------
// The one Yuuka opens around the player at the start of her forward-parasol
// phase, in the last [custom_entities] slot.

enum safetycircle_flag_t {
	SCF_FREE = 0,
	SCF_GROW = 1,
	SCF_SHRINK = 2,

	_safetycircle_flag_t_FORCE_UINT8 = 0xFF
};

struct safetycircle_t {
	safetycircle_flag_t flag;
	/* ------------------------- */ int8_t unused_1;
	screen_point_t center;
	/* ------------------------- */ int8_t unused_2[8];
	unsigned int shrink_frame;
	pixel_t radius_filled;
	pixel_t radius_ring_distance;
	// FOUR, not eight. `[measured]` by MATCH-TH04-MAIN-034-HEAD off a
	// one-byte-red oracle: this array was 8, which put [col_ring] at offset
	// 0x1C where th04_main.asm's own `yuuka6_safetycircle_t` struc puts it at
	// 0x18, and made the whole overlay 30 bytes for a 26-byte custom_t slot.
	// Nothing caught it because no C++ had ever touched a field below
	// [radius_ring_distance] -- yuuka6_bg_update_and_render(), the only other
	// reader of this structure, stops above it -- so the oracle had no opinion
	// on the tail of a layout that was derived rather than read. The
	// size checks in th04/main/boss/b6_spawn.cpp are what make the next such
	// slip a compile error.
	/* ------------------------- */ int8_t unused_3[4];
	vc_t col_ring;
	/* ------------------------- */ int8_t padding;
};

#define safetycircle ( \
	reinterpret_cast<safetycircle_t &>(custom_entities[CUSTOM_COUNT - 1]) \
)
// -------------

// Phase 2's fly path
// ------------------

// The number of different paths Yuuka can take.
static const int PHASE2_FLY_PATHS = 2;

// The number of individual points on each fly path.
static const int PHASE2_FLY_NODES = 5;

extern const unsigned char YUUKA6_PHASE2_FLY_ANGLES[PHASE2_FLY_PATHS][
	PHASE2_FLY_NODES
];
// ------------------

#endif /* TH04_MAIN_BOSS_B6ENT_HPP */
