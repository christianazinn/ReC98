#include "th04/sprites/main_pat.h"
// Yuuka's two [custom_entities] overlays and her fly path, which
// th04/b6_next.cpp's object needs as well. Also supplies
// th04/main/custom.hpp, which this file used to name directly.
#include "th04/main/boss/b6ent.hpp"

#define phase2_fly_path  	yuuka6_phase2_fly_path
#define PHASE2_FLY_ANGLES	YUUKA6_PHASE2_FLY_ANGLES

// Constants
// ---------

static const pixel_t BG_SHAPE_W = 16;
static const pixel_t BG_SHAPE_H = 16;
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

// One frame of every chasing cross and of the safety circle. NOT hosted by
// this file's expander: it makes near calls into main_03, and an object whose
// `-zP` names main_01 frames those on the wrong group -- one of them overflowed
// its fixup and the rest would have linked to silent garbage. It lives in
// th04/b6_next.cpp's object, which is already `-zPmain_03`, and is declared
// there. (kb/codegen/0104; MATCH-TH04-MAIN-034-CHAIN measured it.)

// Opens the safety circle on the player's position.
extern "C" void near safetycircle_open(void)
;

#pragma codeseg

// Call these once per frame to run the indicated sprite animation. All of
// these return true once the animation is finished.
//
// `extern "C"`, and that is a correction rather than a decoration: every one of
// these publishes an undecorated name from th04/main/boss/b6_next.cpp, and
// yuuka6_phase_next() publishes the bare UPPERCASE spelling a `pascal` function
// gets. Declared with C++ linkage, as they were until
// MATCH-TH04-MAIN-034-CHAIN, they resolve against nothing at all -- which is
// exactly what th04/main/boss/b6_next.cpp's note on yuuka6_phase_next() says,
// and it was harmless only for as long as no expander of this file called one.
// th04/main/boss/b6_spawn.cpp now does.
extern "C" {

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

}
