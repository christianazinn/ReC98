#ifndef TH03_MENU_FONT_HPP
#define TH03_MENU_FONT_HPP

#include "platform.h"
#include "pc98.h"

typedef uint8_t __seg *menu_font_t;

extern menu_font_t far menu_font;

bool16 pascal menu_font_load(const unsigned char far *restore_pf_fn);
void pascal menu_font_free(void);

pixel_t pascal menu_font_width(const char far *str);
pixel_t pascal menu_font_width_n(const char far *str, unsigned count);

void pascal menu_font_put(
	screen_x_t left, vram_y_t top, const char far *str, int color
);
void pascal menu_font_put_n(
	screen_x_t left,
	vram_y_t top,
	const char far *str,
	unsigned count,
	int color
);
void pascal menu_font_put_centered(
	screen_x_t center_x, vram_y_t top, const char far *str, int color
);
void pascal menu_font_put_right(
	screen_x_t right, vram_y_t top, const char far *str, int color
);

// Keeps existing fixed-cell columns while rendering each non-space token
// proportionally. [cell_left] is a pixel coordinate, not a TRAM column.
void pascal menu_font_put_cells(
	screen_x_t cell_left, vram_y_t top, const char far *str, int color
);

// Restores an aligned rectangle from graphics page 1 to page 0 and leaves
// page 0 accessed. Callers own the page lifecycle around this helper.
void pascal menu_font_restore_rect(
	screen_x_t left, vram_y_t top, pixel_t w, pixel_t h
);

#endif
