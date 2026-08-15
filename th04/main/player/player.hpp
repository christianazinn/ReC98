#include "th04/main/playfld.hpp"
#include "th02/main/player/player.hpp"

extern PlayfieldMotion player_pos;

void near player_pos_update_and_clamp(void);

// Miss
// ----

// Length of the miss animation, in frames. (th04/main/player/player.inc)
static const uint8_t MISS_ANIM_FRAMES = 32;

// Counts down from (MISS_ANIM_FRAMES + DEATHBOMB_WINDOW) once the player is
// hit, so a value ABOVE MISS_ANIM_FRAMES means the miss animation has not
// started yet and the death can still be taken back with a bomb.
extern unsigned char miss_time;
// ----

// Shots
// -----

static const uint8_t POWER_MAX = 128;

#define SHOT_W 16
#define SHOT_H 16
// -----
