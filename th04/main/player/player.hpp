#ifndef TH04_MAIN_PLAYER_PLAYER_HPP
#define TH04_MAIN_PLAYER_PLAYER_HPP

// Guarded because th05/shot_inv.cpp reaches this file twice in one object:
// once through th04/main/player/render.cpp, the lift at the front of it, and
// once through th04/main/player/shot.hpp on the way to
// th04/main/player/shots_inv.cpp. It is the ONLY header in that intersection
// that collides -- the other members are either guarded already or declare
// nothing with an initializer -- so guarding this one file is the whole fix,
// and CONTRIBUTING.md's "only if the code structure necessitates it" is met
// the same way th04/main/enemy/enemy.hpp met it. (kb/codegen/0129)
#include "th04/main/playfld.hpp"
#include "th02/main/player/player.hpp"

extern PlayfieldMotion player_pos;

void near player_pos_update_and_clamp(void);

// Options
// -------
// The player's options are supposed to lag behind the player's movement by one
// frame, and therefore have to be tracked separately.
// (th04/main/player/option[bss].asm)

extern PlayfieldPoint player_option_pos_cur;
extern PlayfieldPoint player_option_pos_prev;

// A variable in TH04, where the option sprite cycles; a fixed PAT_OPTION in
// TH05. Not a `main_patnum_t`: the dump reserves a full `dw` for it and every
// access is a word one.
//
// Declared for TH04 only, because th04/main/player/option[bss].asm spells the
// TH05 name as an absolute EQUATE rather than storage — there is no object to
// point an `extern` at. player_render() is the only C++ reader in either game
// and supplies TH05's value itself.
#if (GAME != 5)
extern int player_option_patnum;
#endif
// -------

// Miss
// ----

// Length of the miss animation, in frames. (th04/main/player/player.inc)
static const uint8_t MISS_ANIM_FRAMES = 32;

// The miss explosion is rendered while [miss_time] is above this value, i.e.
// during the first (MISS_ANIM_FRAMES - MISS_ANIM_EXPLODE_UNTIL) frames of the
// miss animation. (th04/main/player/player.inc)
static const uint8_t MISS_ANIM_EXPLODE_UNTIL = 31;

// Sprites placed on the ring of the miss explosion. The second half of them
// is placed on a ring of half the radius, rotating the other way.
// (th04/main/player/player.inc)
static const int MISS_EXPLOSION_COUNT = 8;

static const pixel_t MISS_EXPLOSION_W = 48;
static const pixel_t MISS_EXPLOSION_H = 48;

static const subpixel_t MISS_EXPLOSION_RADIUS_VELOCITY = TO_SP(7);
static const uint8_t MISS_EXPLOSION_ANGLE_VELOCITY = 8;

// Counts down from (MISS_ANIM_FRAMES + DEATHBOMB_WINDOW) once the player is
// hit, so a value ABOVE MISS_ANIM_FRAMES means the miss animation has not
// started yet and the death can still be taken back with a bomb.
extern unsigned char miss_time;

// Ring parameters of the miss explosion, advanced once per frame while the
// animation runs.
extern subpixel_t miss_explosion_radius;
extern unsigned char miss_explosion_angle;

// Frames of invincibility granted after a miss. Both games share this value;
// TH02's own, different one is declared next to its own POWER_MAX in
// th02/main/player/player.hpp. (th04/main/player/player.inc)
static const uint8_t MISS_INVINCIBILITY_FRAMES = 192;
// ----

// Shots
// -----

static const uint8_t POWER_MAX = 128;

#define SHOT_W 16
#define SHOT_H 16
// -----

#endif /* TH04_MAIN_PLAYER_PLAYER_HPP */
