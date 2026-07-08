#ifndef TH03_MAIN_REPLAY_HPP
#define TH03_MAIN_REPLAY_HPP

#include "platform.h"

void near replay_session_start(void);
void near replay_round_start(void);
void near replay_frame_io(void);
void near replay_route(uint8_t route);
void near replay_finish(uint8_t route);

#endif /* TH03_MAIN_REPLAY_HPP */
