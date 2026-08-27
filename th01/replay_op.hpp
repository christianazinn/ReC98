#ifndef TH01_REPLAY_OP_HPP
#define TH01_REPLAY_OP_HPP

#include "platform.h"
#include "th01/replay_format.hpp"

enum t1replay_op_action_t {
	T1ROA_NONE,
	T1ROA_RETURN,
	T1ROA_PLAYBACK,
	T1ROA_PRACTICE_RECORD,
};

struct t1replay_op_result_t {
	t1replay_op_action_t action;
	uint8_t slot;
};

void t1replay_op_replay_enter(void);
bool t1replay_op_pending_enter(void);
void t1replay_op_practice_enter(
	int8_t rank, int8_t lives, int8_t bombs, uint32_t rand
);
void t1replay_op_restore(void);
void t1replay_op_command_clear(void);
bool t1replay_op_record_prepare(void);
#if T1REPLAY_EXACT_TRACE
bool t1replay_op_exact_bootstrap(void);
#endif
void t1replay_op_main_choice_put(
	int choice, int center_x, int top, int col, int fx
);
void t1replay_op_language_choice_put(
	int left, int top, int col, int fx
);
bool t1replay_op_language_toggle(void);
void t1replay_op_music_title_put(
	int left, int top, int col, int fx, int track, const shiftjis_t *japanese
);
t1replay_op_result_t t1replay_op_replay_update(void);
t1replay_op_result_t t1replay_op_practice_update(void);
void t1replay_op_practice_start_get(t1replay_practice_start_t& start);

#endif /* TH01_REPLAY_OP_HPP */
