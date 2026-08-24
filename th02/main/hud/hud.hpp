#ifndef TH02_MAIN_HUD_HUD_HPP
#define TH02_MAIN_HUD_HUD_HPP

// Guarded for the reason th04/main/player/shot.hpp states in full: two body
// files that both include this one now meet in a single translation unit,
// and Turbo C++ 4.02 rejects the second expansion of every `static const`
// and every declaration below. Ordering the includes so only one of them
// wins would work today and break at the next host; a guard is the
// invariant. Byte-inert: this file only declares.

#include "pc98.h"

// Coordinates
// -----------

static const tram_x_t HUD_LEFT = 56;
static const tram_cell_amount_t HUD_TRAM_W = 16;

static const tram_y_t HUD_HISCORE_Y = 4;
static const tram_y_t HUD_SCORE_Y = 6;
#if (GAME == 2)
// TH04 and TH05 include this header for the shared columns and the two score
// rows, but lay out every other row differently — bombs 11, lives 13, power
// bar 22, rank 23 — and define their own constants in th04/main/hud/hud.cpp.
// [HUD_POWER_Y] in particular is 22 there, which is TH02's rank row.
static const tram_y_t HUD_BOMBS_Y = 15;
static const tram_y_t HUD_LIVES_Y = 17;
static const tram_y_t HUD_POWER_Y = 20;
static const tram_y_t HUD_RANK_Y = 22;
#endif

static const tram_cell_amount_t HUD_LABEL_PADDING = 1;
static const tram_cell_amount_t HUD_LABEL_W = (2 * GAIJI_TRAM_W);
static const tram_cell_amount_t HUD_LABEL_PADDED_W = (
	HUD_LABEL_PADDING + HUD_LABEL_W + HUD_LABEL_PADDING
);

static const tram_x_t HUD_LABELED_LEFT = (HUD_LEFT + HUD_LABEL_PADDED_W);
static const tram_cell_amount_t HUD_LABELED_W = (
	HUD_TRAM_W - HUD_LABEL_PADDED_W
);
// -----------

#if (GAME == 2)
// Yup, this also commits changes to [power] to the [shot_level], which
// absolutely doesn't belong here.
void near player_shot_level_update_and_hud_power_put(void);

void near hud_lives_put(void);
void near hud_bombs_put(void);

// Renders the entire HUD, reflecting all current values.
void near hud_put(void);
#endif
#endif /* TH02_MAIN_HUD_HUD_HPP */
