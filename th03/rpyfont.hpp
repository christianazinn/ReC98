#ifndef TH03_RPYFONT_HPP
#define TH03_RPYFONT_HPP

#include "platform.h"

void far replay_font_slot_line_put(
	uint8_t slot, uint8_t sel, unsigned y, bool active, bool has_replay
);
void far replay_font_render_begin(void);
void far replay_font_render_end(void);
void far replay_font_cursor_move(
	screen_x_t left, uint8_t old_y, uint8_t y
);
void far replay_font_columns_put(void);
void far replay_font_detail_put(
	uint8_t slot, uint8_t stage_sel, bool stage_focus
);
void far replay_font_detail_empty_put(uint8_t slot);

#endif
