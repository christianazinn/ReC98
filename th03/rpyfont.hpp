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

enum replay_font_diagnostic_t {
	RFD_SAMPLES,
	RFD_FRAMES,
	RFD_BYTES,
	RFD_RNG,
};

enum replay_detail_page_t {
	RDP_SPLITS = 0,
	RDP_CLEAR_BONUSES,
	RDP_TIMERS,
	RDP_COUNT,
};

enum replay_font_save_question_t {
	RFSQ_SAVE,
	RFSQ_OVERWRITE,
	RFSQ_QUIT,
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
	uint8_t checkpoint_sel,
	bool checkpoint_focus,
	uint8_t detail_page,
	bool show_unreached_opponents
);
void far replay_font_practice_settings_modal_put(void);
void far replay_font_detail_empty_put(uint8_t slot);
void far replay_font_diagnostic_line_put(
	unsigned y, uint8_t label, uint32_t value
);
uint8_t far replay_font_page_top(uint8_t selected);
uint8_t far replay_font_page_left(uint8_t selected);
uint8_t far replay_font_page_right(uint8_t selected);
void far replay_font_page_put(uint8_t selected, unsigned y);
void far replay_font_save_dialog_put(
	uint8_t question, uint8_t slot, bool selected_yes
);
void far replay_font_save_complete_put(void);

#endif
