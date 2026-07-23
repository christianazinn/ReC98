#ifndef TH03_MAIN_REPLAY_HPP
#define TH03_MAIN_REPLAY_HPP

#include "platform.h"
#include "th03/replay_build.hpp"

#define REPLAY_PAUSE_RESUME 0
#define REPLAY_PAUSE_RESTART 1
#define REPLAY_PAUSE_SAVE_EXIT 2
#define REPLAY_PAUSE_DISCARD_EXIT 3

void far replay_round_reset_seed_capture(void);
void far replay_session_start(void);
void far replay_round_start(void);
void far replay_frame_io(void);
void far replay_frame_publish(void);
void far replay_frame_delay(void);
void far replay_overlay_put(void);
void far replay_input_sense_held(void);
// Returns a one-shot pause request in CF without altering the sampled input.
extern "C" void far replay_pause_request_poll(void);
bool far replay_prompt_skip(void);
uint8_t far replay_pause_menu(void);
void far replay_user_record_discard_on_exit(void);
void far replay_restart_request(void);
void far replay_route(uint8_t route);
void far replay_finish(uint8_t route);

#endif /* TH03_MAIN_REPLAY_HPP */
