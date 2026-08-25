#ifndef TH01_REPLAY_OP_HPP
#define TH01_REPLAY_OP_HPP

#include "platform.h"
#include "th01/replay_format.hpp"

enum t1replay_op_action_t {
	T1ROA_NONE,
	T1ROA_RETURN,
	T1ROA_PLAYBACK,
	T1ROA_PRACTICE_RECORD,
	T1ROA_PRACTICE_UNRECORDED,
};

enum t1replay_practice_section_t {
	T1RPS_STAGE_START,
	T1RPS_CHAPTER,
	T1RPS_BOSS_START,
};

struct t1replay_practice_start_t {
	uint8_t scene;
	uint8_t route;
	uint8_t section;
	uint8_t chapter;
	int8_t rank;
	int32_t score;
	int8_t lives;
	int8_t bombs;
	uint16_t point_value;
	int16_t pellet_speed;
	uint32_t rand;
};

struct t1replay_op_result_t {
	t1replay_op_action_t action;
	uint8_t slot;
};

void t1replay_op_replay_enter(void);
void t1replay_op_practice_enter(
	int8_t rank, int8_t lives, int8_t bombs, uint32_t rand
);
void t1replay_op_restore(void);
void t1replay_op_command_clear(void);
bool t1replay_op_record_prepare(void);
void t1replay_op_main_choice_put(
	int choice, int center_x, int top, int col, int fx
);
t1replay_op_result_t t1replay_op_replay_update(void);
t1replay_op_result_t t1replay_op_practice_update(void);
void t1replay_op_practice_start_get(t1replay_practice_start_t& start);

#endif /* TH01_REPLAY_OP_HPP */
