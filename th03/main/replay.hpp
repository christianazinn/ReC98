#ifndef TH03_MAIN_REPLAY_HPP
#define TH03_MAIN_REPLAY_HPP

#include "platform.h"

#define REPLAY_PAUSE_RESUME 0
#define REPLAY_PAUSE_SAVE_EXIT 1
#define REPLAY_PAUSE_DISCARD_EXIT 2

void far replay_session_start(void);
void far replay_round_start(void);
void far replay_frame_io(void);
void far replay_input_sense_held(void);
bool far replay_prompt_skip(void);
uint8_t far replay_pause_menu(void);
void far replay_user_record_discard_on_exit(void);
void far replay_route(uint8_t route);
void far replay_finish(uint8_t route);

#endif /* TH03_MAIN_REPLAY_HPP */
