#ifndef TH02_PRACTICE_DIAG_HPP
#define TH02_PRACTICE_DIAG_HPP

#include "th02/replay_format.hpp"

#ifndef T2REPLAY_PRACTICE_DIAGNOSTICS
#ifdef T2PD
#define T2REPLAY_PRACTICE_DIAGNOSTICS T2PD
#else
#define T2REPLAY_PRACTICE_DIAGNOSTICS 0
#endif
#endif
#ifndef T2REPLAY_PRACTICE_DIAGNOSTICS_BUILD_ID
#ifdef T2PID
#define T2REPLAY_PRACTICE_DIAGNOSTICS_BUILD_ID T2PID
#else
#define T2REPLAY_PRACTICE_DIAGNOSTICS_BUILD_ID 0
#endif
#endif

#define T2PRACT_DIAG_SCHEMA 1
#define T2PRACT_DIAG_RECORD_SIZE 32

enum t2practice_diag_event_t {
	T2PDE_OP_BEGIN = 1,
	T2PDE_OP_COMMAND_WRITE = 2,
	T2PDE_OP_HANDOFF = 3,
	T2PDE_MAIN_COMMAND_ADMISSION = 4,
	T2PDE_TARGET_APPLY_BEGIN = 5,
	T2PDE_TARGET_APPLY_RESULT = 6,
};

enum t2practice_diag_reason_t {
	T2PDR_NONE = 0,
	T2PDR_OP_COMMAND_CREATE,
	T2PDR_OP_COMMAND_WRITE,
	T2PDR_OP_WITNESS_CREATE,
	T2PDR_OP_WITNESS_WRITE,
	T2PDR_MAIN_COMMAND_MISSING,
	T2PDR_MAIN_COMMAND_READ,
	T2PDR_MAIN_COMMAND_SIZE,
	T2PDR_MAIN_COMMAND_INVALID,
	T2PDR_STAGE_MISMATCH,
	T2PDR_TARGET_UNKNOWN,
	T2PDR_MAP_LENGTH,
	T2PDR_TARGET_SCROLL,
	T2PDR_MAP_ROW,
	T2PDR_MAP_SECTION,
	T2PDR_SPAWN_ARGUMENT,
	T2PDR_SPAWN_ROWS,
	T2PDR_SPAWN_GRID,
	T2PDR_SPAWN_TRIGGER,
	T2PDR_TERMINAL_FIELD,
	T2PDR_CHAPTER_FIELD,
	T2PDR_CONSTRUCTOR,
	T2PDR_TARGET_APPLY,
};

enum t2practice_diag_constructor_result_t {
	T2PDCR_NOT_ATTEMPTED = 0,
	T2PDCR_SUCCESS = 1,
	T2PDCR_FAILURE = -1,
};

#if T2REPLAY_PRACTICE_DIAGNOSTICS

struct t2practice_diag_record_t {
	char magic[8];
	uint8_t schema;
	uint8_t build_id;
	uint8_t event;
	uint8_t reason;
	uint8_t command_mode;
	uint8_t command_flags;
	int8_t stage;
	uint8_t target;
	int16_t map_length;
	int16_t target_scroll_step;
	int16_t top_map_row;
	int16_t spawn_rows;
	int16_t spawn_first_trigger;
	int16_t spawn_upper_bound;
	int8_t constructor_result;
	uint8_t reserved;
	uint16_t checksum;
};

typedef char t2practice_diag_record_size_check[
	(sizeof(t2practice_diag_record_t) == T2PRACT_DIAG_RECORD_SIZE) ? 1 : -1
];

void t2practice_diag_clear(void);
void t2practice_diag_reset(
	uint8_t mode, uint8_t flags, const t2replay_start_t far *start
);
void t2practice_diag_op_command(
	enum t2practice_diag_reason_t reason, uint8_t mode, uint8_t flags,
	const t2replay_start_t far *start
);
void t2practice_diag_op_handoff(
	uint8_t mode, uint8_t flags, const t2replay_start_t far *start
);
void t2practice_diag_main_command(
	enum t2practice_diag_reason_t reason, const t2replay_command_t far *command
);
void t2practice_diag_apply_begin(
	int8_t stage, uint8_t target, int map_length, int spawn_rows
);
void t2practice_diag_target_scroll(int target_scroll_step);
void t2practice_diag_top_map_row(int top_map_row);
void t2practice_diag_spawn_first_trigger(int trigger);
void t2practice_diag_spawn_upper_bound(int spawn_upper_bound);
void t2practice_diag_failure(enum t2practice_diag_reason_t reason);
void t2practice_diag_constructor_result(bool16 result);
void t2practice_diag_apply_end(bool16 result);

#else

#define t2practice_diag_clear() ((void)0)
#define t2practice_diag_reset(mode, flags, start) ((void)0)
#define t2practice_diag_op_command(reason, mode, flags, start) ((void)0)
#define t2practice_diag_op_handoff(mode, flags, start) ((void)0)
#define t2practice_diag_main_command(reason, command) ((void)0)
#define t2practice_diag_apply_begin(stage, target, map_length, spawn_rows) ((void)0)
#define t2practice_diag_target_scroll(target_scroll_step) ((void)0)
#define t2practice_diag_top_map_row(top_map_row) ((void)0)
#define t2practice_diag_spawn_first_trigger(trigger) ((void)0)
#define t2practice_diag_spawn_upper_bound(spawn_upper_bound) ((void)0)
#define t2practice_diag_failure(reason) ((void)0)
#define t2practice_diag_constructor_result(result) ((void)0)
#define t2practice_diag_apply_end(result) ((void)0)

#endif

#endif /* TH02_PRACTICE_DIAG_HPP */
