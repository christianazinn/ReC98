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
extern int player_option_patnum;
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
// ----

// Shots
// -----

static const uint8_t POWER_MAX = 128;

#define SHOT_W 16
#define SHOT_H 16
// -----
