#include "th03/sprites/opwin.hpp"

// Coordinates
// -----------

static const pixel_t BOX_H = OPWIN_H;
static const pixel_t MAIN_W = 136;
static const pixel_t SUBMENU_W = 240;

enum {
	OPTION_W = 272,
};

static const screen_x_t BOX_LEFT = 160;
static const screen_y_t BOX_TOP = 264;

static const screen_y_t BOX_BOTTOM = (BOX_TOP + BOX_H);
static const screen_x_t BOX_MAIN_RIGHT = (BOX_LEFT + MAIN_W);
static const screen_x_t BOX_MAIN_CENTER_X = (BOX_LEFT + (MAIN_W / 2));
static const screen_x_t BOX_SUBMENU_CENTER_X = (BOX_LEFT + (SUBMENU_W / 2));
static const screen_x_t BOX_SUBMENU_RIGHT = (BOX_LEFT + SUBMENU_W);
enum {
	BOX_OPTION_CENTER_X = (BOX_LEFT + (OPTION_W / 2)),
	BOX_OPTION_RIGHT = (BOX_LEFT + OPTION_W),
};
// -----------

// Restores the 16×[BOX_H]-pixel column starting at ([left], BOX_TOP) from
// VRAM page 1. Returns with page 0 as the accessed page.
void pascal near box_column16_unput(uscreen_x_t left);

// Shows the animation that changes the size of the box.
void pascal near box_main_to_submenu_animate(screen_x_t box_right);
void pascal near box_submenu_to_main_animate(screen_x_t box_right);

// Shows either the long (shifting kanji and flashing) or short (just fading)
// title animation. Both of these return with the title image blitted to both
// VRAM pages, page 0 both accessed and shown, and parts 1 and 3 of the
// character selection CDG images loaded.
void near op_animate(void);
void near op_fadein_animate(bool restart_bgm);
