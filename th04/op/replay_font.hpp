#ifndef TH04_OP_REPLAY_FONT_HPP
#define TH04_OP_REPLAY_FONT_HPP

#include "platform.h"
#include "pc98.h"

typedef uint8_t __seg *replay_op_font_t;

extern replay_op_font_t replay_op_font;

bool16 pascal replay_op_font_load(const unsigned char far *restore_pf_fn);
void pascal replay_op_font_free(void);

pixel_t pascal replay_op_font_width(const char far *str);
void pascal replay_op_font_put(
	screen_x_t left, vram_y_t top, const char far *str, int color
);
void pascal replay_op_font_put_n(
	screen_x_t left,
	vram_y_t top,
	const char far *str,
	unsigned count,
	int color
);
void pascal replay_op_font_put_centered(
	screen_x_t center_x, vram_y_t top, const char far *str, int color
);
void pascal replay_op_font_put_right(
	screen_x_t right, vram_y_t top, const char far *str, int color
);
void pascal replay_op_font_put_cells(
	screen_x_t cell_left, vram_y_t top, const char far *str, int color
);

#endif
