// Midboss and boss state
// ----------------------

#include "pc98.h"

extern int16_t boss_phase_frame;

// Same indirection as used for the player position. Unfortunately not
// contiguous in memory, or else we could have created a struct for both.
// `near`, because marisa_bg_render() takes their address into the two near
// pointers below and the original computes only a 16-bit offset there.
// (kb/codegen/0003)
extern screen_x_t near boss_left_on_page[PAGE_COUNT];
extern screen_y_t near boss_top_on_page[PAGE_COUNT];
extern screen_x_t near* boss_left_on_back_page;
extern screen_y_t near* boss_top_on_back_page;

// TH02 tracks boss and midboss health via the increasing amount of damage
// dealt, instead of using a decreasing HP value.
extern int boss_damage;

// How long the player is invincible for after defeating a boss. Mirrors
// th04/main/boss/boss.hpp, which spells the same constant at that game's own
// value of 255; th02/th02.inc has held this one for the dump all along.
static const int BOSS_DEFEAT_INVINCIBILITY_FRAMES = 200;
// ----------------------
