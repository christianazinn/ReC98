#ifndef TH01_REPLAY_FONT_HPP
#define TH01_REPLAY_FONT_HPP

#include "platform.h"
#include "pc98.h"

typedef uint8_t far *replay_op_font_t;

extern replay_op_font_t replay_op_font;

enum replay_op_font_fixed_t {
	REPLAY_OP_FONT_NUMERIC_CELL_W = 13,
	REPLAY_OP_FONT_ONE_INSET = 2,
};

bool16 pascal replay_op_font_load(void);
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
void pascal replay_op_font_put_numeric_cells(
	screen_x_t cell_left,
	vram_y_t top,
	const char far *str,
	unsigned count,
	int color
);

#endif
