#ifndef TH03_RPYFONT_HPP
#define TH03_RPYFONT_HPP

#include "platform.h"

enum replay_font_cell_width_t {
	REPLAY_FONT_NUMERIC_CELL_W = 12,
	REPLAY_FONT_NAME_CELL_W = 13,
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
	uint8_t slot, uint8_t stage_sel, bool stage_focus
);
void far replay_font_detail_empty_put(uint8_t slot);

#endif
