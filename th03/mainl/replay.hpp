#ifndef TH03_MAINL_REPLAY_HPP
#define TH03_MAINL_REPLAY_HPP

#include "platform.h"
#include "th03/replay_format.hpp"

void far mainl_replay_session_start(void);
void far mainl_replay_input_mode_interface(void);
bool far mainl_replay_initial_stage_splash_skip(void);
void far mainl_replay_finish(replay_user_end_reason_t end_reason);

#endif /* TH03_MAINL_REPLAY_HPP */
