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
void replay_record_next_prepare(void);

#endif /* TH04_OP_REPLAY_HPP */
