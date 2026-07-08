#ifndef TH03_MAIN_REPLAY_HPP
#define TH03_MAIN_REPLAY_HPP

#include "platform.h"

void far replay_session_start(void);
void far replay_round_start(void);
void far replay_frame_io(void);
void far replay_route(uint8_t route);
void far replay_finish(uint8_t route);

#endif /* TH03_MAIN_REPLAY_HPP */
