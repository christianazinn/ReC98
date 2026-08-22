#include "th04/sprites/main_pat.h"
#include "th04/main/custom.hpp"

#define phase2_fly_path  	yuuka6_phase2_fly_path
#define PHASE2_FLY_ANGLES	YUUKA6_PHASE2_FLY_ANGLES

// Constants
// ---------

static const pixel_t BG_SHAPE_W = 16;
static const pixel_t BG_SHAPE_H = 16;

static const pixel_t CHASECROSS_W = 32;
static const pixel_t CHASECROSS_H = 32;
static const int CHASECROSS_KILL_FRAMES_PER_CEL = 4;

// The number of different paths Yuuka can take.
static const int PHASE2_FLY_PATHS = 2;

// The number of individual points on each fly path.
static const int PHASE2_FLY_NODES = 5;

extern const unsigned char PHASE2_FLY_ANGLES[PHASE2_FLY_PATHS][
	PHASE2_FLY_NODES
];
// ---------

// Structures
// ----------

#define BG_SHAPE_COUNT 56

struct bg_shape_t {
	SPPoint pos;
	unsigned char angle;
	SubpixelLength8 speed;
};

// 1 additional unused one, for some reason?
extern bg_shape_t bg_shapes[BG_SHAPE_COUNT + 1];

extern main_patnum_t bg_shape_patnum;
extern Subpixel bg_shape_flyout_speed;

// Called on every frame for each shape after [pos] was updated. Can implement
// custom clipping and respawning behavior.
extern void (near pascal *near bg_shape_clip)(bg_shape_t near& shape);

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

// Chasing cross bullets
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
	// one-byte-red oracle: this array was 8 here, which put [col_ring] at
	// offset 0x1C where th04_main.asm's own `yuuka6_safetycircle_t` struc puts
	// it at 0x18, and made the whole overlay 30 bytes for a 26-byte custom_t
	// slot. Nothing caught it because no C++ had ever touched a field below
	// [radius_ring_distance] -- yuuka6_bg_update_and_render(), the only other
	// reader of this structure, stops above it -- so the oracle had no opinion
	// on the tail of a layout that was derived rather than read. The
	// static_assert below is what makes the next such slip a compile error.
	/* ------------------------- */ int8_t unused_3[4];
	vc_t col_ring;
	/* ------------------------- */ int8_t padding;
};

// Both overlays must fit the slot they reinterpret, and both are checked --
// but NOT here: platform.h's static_assert() expands to an expression, so it
// only compiles inside a function body. The two checks live at the top of
// th04/main/boss/b6_spawn.cpp's functions, which is the first C++ that writes
// either structure past the fields yuuka6_bg_update_and_render() reads.

#define safetycircle ( \
	reinterpret_cast<safetycircle_t &>(custom_entities[CUSTOM_COUNT - 1]) \
)
// ----------

// These indicate the state of the last completed animation, and are only used
// to decide whether to trigger a transition to a different state via one of
// the _anim_* functions.
enum yuuka6_sprite_flag_t {
	VANISHED = 0,
	PARASOL_BACK_OPEN = 1,
	PARASOL_BACK_CLOSED = 2,
	PARASOL_FORWARD = 3,
	PARASOL_LEFT = 4,
	PARASOL_SHIELD = 8,
};

extern int yuuka6_anim_frame;
extern yuuka6_sprite_flag_t yuuka6_sprite_flag;
extern uint8_t phase2_fly_path;

// Both of these live in B6_SPAWN_TEXT, the kb/codegen/0080 anchor carved off
// main_034_TEXT's head, and are DEFINED in th04/main/boss/b6_spawn.cpp, which
// th04/main/boss/bg.cpp -- the one file that expands this one -- includes at
// its end. The pragma pair has to sit around the DECLARATIONS and not just
// around the definitions: Turbo C++ binds a function's code segment when it
// first SEES the function, so a declaration read under the object's default
// segment defeats a later `#pragma codeseg` silently, with a link that works
// and a map that is hundreds of bytes wrong (kb/codegen/0155).
#pragma codeseg B6_SPAWN_TEXT main_03

// Spawns one chasing cross at Yuuka's position, or does nothing if all of them
// are busy.
void pascal near chasecrosses_add(
	unsigned char angle, subpixel_length_8_t speed
)
;

// Opens the safety circle on the player's position. `extern "C"`, because the
// undecorated spelling is the one th04_main.asm published for it while it was
// still ASM, under the placeholder name yuuka6_1A0D1.
extern "C" void near safetycircle_open(void)
;

#pragma codeseg

// Call these once per frame to run the indicated sprite animation. All of
// these return true once the animation is finished.
bool near yuuka6_anim_parasol_back_close(void)
;
bool near yuuka6_anim_parasol_back_open(void)
;
bool near yuuka6_anim_parasol_back_pull_forward(void)
;
bool near yuuka6_anim_parasol_back_pull_left(void)
;
bool near yuuka6_anim_parasol_left_spin_back(void)
;
bool near yuuka6_anim_vanish(void)
;
bool near yuuka6_anim_appear(void)
;
bool near yuuka6_anim_parasol_shield(void)
;

void pascal near yuuka6_phase_next(
	explosion_type_t explosion_type, int next_end_hp
);
