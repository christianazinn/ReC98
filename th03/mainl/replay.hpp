#ifndef TH03_MAINL_REPLAY_HPP
#define TH03_MAINL_REPLAY_HPP

#include "platform.h"
#include "libs/master.lib/master.hpp"
#include "th03/replay_format.hpp"

void far mainl_replay_session_start(void);
void far mainl_replay_input_mode_interface(void);
void far mainl_replay_transition_finish(void);
void far mainl_replay_exit_to_main(void);
bool far mainl_replay_initial_stage_splash_skip(void);
bool far mainl_replay_stage_start_selected(void);
int far mainl_replay_stage_transition_needed(void);
uint8_t far mainl_replay_resume_take(void);
bool far mainl_replay_finish(
	replay_user_end_reason_t end_reason, uint8_t save_prompt_mode
);
bool far mainl_replay_clear_playback_finish(void);
void far mainl_staffroll_fade_wait(void);

int MASTER_RET mainl_language_file_ropen(const char MASTER_PTR *filename);
void MASTER_RET mainl_language_file_close(void);

#endif /* TH03_MAINL_REPLAY_HPP */
