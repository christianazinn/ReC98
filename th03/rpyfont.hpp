#ifndef TH03_RPYFONT_HPP
#define TH03_RPYFONT_HPP

#include "platform.h"
#include "th03/menu_font.hpp"

enum replay_font_cell_width_t {
	REPLAY_FONT_NUMERIC_CELL_W = MENU_FONT_NUMERIC_CELL_W,
	REPLAY_FONT_NAME_CELL_W = 13,
	REPLAY_FONT_CAPITAL_I_ADVANCE = 3,
	REPLAY_FONT_ONE_INSET = MENU_FONT_ONE_INSET,
};

void pascal far replay_font_put_fixed_n(
	int left,
	int top,
	const char far *str,
	unsigned count,
	int cell_w,
	int color
);
void far replay_font_slot_line_put(
	uint8_t slot, uint8_t sel, unsigned y, bool active, bool has_replay
);
void far replay_font_columns_put(bool clear);
void far replay_font_detail_put(
	uint8_t slot,
	uint8_t stage_sel,
	bool stage_focus,
	bool show_unreached_opponents
);
void far replay_font_detail_empty_put(uint8_t slot);

#endif
