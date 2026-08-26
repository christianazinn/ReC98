#ifndef TH04_OP_REPLAY_HPP
#define TH04_OP_REPLAY_HPP

#include "pc98.h"
#include "th04/replay_format.hpp"

void replay_title_label_put(screen_y_t top, vc2 col);
void replay_title_desc_put(void);
void replay_practice_title_label_put(screen_y_t top, vc2 col);
void replay_practice_title_desc_put(void);
bool replay_browser(void);
bool replay_practice_setup(replay_start_config_t far *start);
bool replay_practice_record_prepare(
	const replay_start_config_t far *start
);
// Private emulator-test bootstrap. Accepts only an externally staged,
// arbitrary-target Practice record command and leaves it for MAIN to consume.
bool replay_private_record_command_start(
	replay_start_config_t far *start
);
void replay_command_clear(void);
void replay_record_next_prepare(void);

enum replay_op_bridge_func_t {
	ROBF_PLAYCHAR_MENU,
	ROBF_EXIT_PREPARE,
	ROBF_REGIST_VIEW_MENU,
	ROBF_MUSICROOM_MENU,
	ROBF_MAIN_CDG_LOAD,
};

// REPLAY_OP_TEXT is deliberately outside the stock OP_01 code group. Route
// calls to stock near functions through this far entry point in OP_MAIN_TEXT.
bool16 far replay_op_bridge(replay_op_bridge_func_t func);
void far replay_op_startup_dispatch(void);
void far replay_main_update_and_render(const char *main_bg_fn);

#endif /* TH04_OP_REPLAY_HPP */
