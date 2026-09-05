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
#define T2LIFE_DIAG_SCHEMA 1
#define T2LIFE_DIAG_RECORD_SIZE 18

enum t2practice_diag_event_t {
	T2PDE_OP_BEGIN = 1,
	T2PDE_OP_COMMAND_WRITE = 2,
	T2PDE_OP_HANDOFF = 3,
	T2PDE_MAIN_COMMAND_ADMISSION = 4,
	T2PDE_TARGET_APPLY_BEGIN = 5,
	T2PDE_TARGET_APPLY_RESULT = 6,
	T2PDE_MAIN_PROGRESS = 7,
};

enum t2practice_diag_main_progress_t {
	T2PDMP_REPLAY_ENTRY = 1,
	T2PDMP_SOUND_READY,
	T2PDMP_GAMEPLAY_INIT,
	T2PDMP_STAGE_INIT,
	T2PDMP_REPLAY_STAGE_START,
	T2PDMP_STAGE_OVERLAY,
	T2PDMP_STAGE_LOOP_CALL,
	T2PDMP_GAMEPLAY_SOUND,
	T2PDMP_GAMEPLAY_HISCORE,
	T2PDMP_GAMEPLAY_EYE,
	T2PDMP_GAMEPLAY_BFT,
	T2PDMP_GAMEPLAY_CONVERT,
	T2PDMP_GAMEPLAY_GAIJI,
	T2PDMP_GAMEPLAY_BOMB,
	T2PDMP_STAGE_BEGIN,
	T2PDMP_STAGE_RESOURCES,
	T2PDMP_STAGE_CALLBACKS,
	T2PDMP_STAGE_TILES_READY,
	T2PDMP_STAGE_VSYNC_READY,
	T2PDMP_STAGE_COMPLETE,
	T2PDMP_STAGE_SCREEN_READY,
	T2PDMP_STAGE_BFT_READY,
	T2PDMP_STAGE_MAP_READY,
	T2PDMP_STAGE_MPN_READY,
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

// Private cross-process acceptance milestones. These intentionally describe
// the patch lifecycle rather than any ZUN state, and compile only into T2PD
// diagnostic binaries.
enum t2practice_lifecycle_milestone_t {
	T2PDLM_OP_MENU = 1,
	T2PDLM_LANGUAGE_RETURN,
	T2PDLM_MAIN_HEAP_ADMITTED,
	T2PDLM_GAMEPLAY_INITIALIZED,
	T2PDLM_PRACTICE_TARGET_APPLIED,
	T2PDLM_TITLE_DEMO_MAIN,
	// `largest_paras` carries the initialized stage ID for this event.
	T2PDLM_STAGE_INITIALIZED,
	T2PDLM_MAINE_HEAP_ADMITTED,
	// `largest_paras` is nonzero only when the post-close raw guard witness
	// admitted recording. It keeps 5D01h capability separate from the verdict.
	T2PDLM_REPLAY_GUARD_ADMITTED,
	// Private no-input route: OP initialized resident state and is about to
	// execute the same command-bearing handoff as interactive Practice.
	T2PDLM_OP_DIRECT_LAUNCH,
	// Fine-grained MAIN admission probes. These are private-only evidence for
	// the adaptive DOS heap floor; they never affect release pacing or state.
	T2PDLM_MAIN_PLANES_READY,
	T2PDLM_MAIN_VSYNC_READY,
	T2PDLM_MAIN_EGC_READY,
	T2PDLM_MAIN_400LINE_READY,
	T2PDLM_MAIN_PACKFILE_READY,
	// Kept separate from MAIN so a title-side allocation cannot be misread as
	// proof that the following executable reached its own admission boundary.
	T2PDLM_OP_HEAP_ADMITTED,
	// Earliest MAIN process boundaries, before any allocator or packfile use.
	T2PDLM_MAIN_ENTRY,
	T2PDLM_MAIN_CFG_LOADED,
	T2PDLM_MAIN_REPLAY_ENTRY_BEGIN,
	T2PDLM_MAIN_REPLAY_PATHS_READY,
	T2PDLM_MAIN_COMMAND_OPENED,
	T2PDLM_MAIN_COMMAND_READ,
	T2PDLM_MAIN_COMMAND_CLOSED,
	T2PDLM_MAIN_COMMAND_SIZE_VALID,
	T2PDLM_MAIN_COMMAND_STRUCT_VALID,
	T2PDLM_MAIN_COMMAND_VALIDATION_MASK,
	T2PDLM_MAIN_START_VALID_BEGIN,
	T2PDLM_MAIN_START_VALID_END,
	T2PDLM_MAIN_START_CORE_BEGIN,
	T2PDLM_MAIN_START_TARGET_RESOLVED,
	T2PDLM_MAIN_START_SCALARS_VALID,
	T2PDLM_MAIN_START_RESERVED_VALID,
	T2PDLM_MAIN_PRACTICE_CORE_VALID,
	T2PDLM_MAIN_COMMAND_DIAG_BEGIN,
	T2PDLM_MAIN_COMMAND_DIAG_END,
	T2PDLM_MAIN_REPLAY_COMMAND_READY,
	T2PDLM_MAIN_PRACTICE_START_APPLIED,
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

struct t2practice_lifecycle_record_t {
	char magic[8];
	uint8_t schema;
	uint8_t milestone;
	uint16_t largest_paras;
	uint16_t chosen_paras;
	uint16_t available_paras;
	uint16_t high_water_paras;
};

typedef char t2practice_lifecycle_record_size_check[
	(sizeof(t2practice_lifecycle_record_t) == T2LIFE_DIAG_RECORD_SIZE) ? 1 : -1
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
void t2practice_diag_main_progress(
	enum t2practice_diag_main_progress_t progress, int8_t stage
);
bool16 far t2practice_diag_no_sound(void);
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
// The launcher removes T2LIFE.BIN before a diagnostic run. OP entries append
// their own milestone so natural MAIN/MAINE -> OP returns remain observable.
void t2practice_diag_lifecycle_op_menu_enter(void);
void t2practice_diag_lifecycle(
	enum t2practice_lifecycle_milestone_t milestone,
	uint16_t largest_paras, uint16_t chosen_paras, uint16_t available_paras
);
// PFSTART installs an INT 21h archive hook. Private diagnostics must bypass
// that hook while touching their own loose files, exactly as the hook does
// for an internal recursive DOS call. These are diagnostic-only and nesting
// safe so the public game paths remain untouched.
void t2practice_diag_io_bypass_begin(void);
void t2practice_diag_io_bypass_end(void);

#else

#define t2practice_diag_clear() ((void)0)
#define t2practice_diag_reset(mode, flags, start) ((void)0)
#define t2practice_diag_op_command(reason, mode, flags, start) ((void)0)
#define t2practice_diag_op_handoff(mode, flags, start) ((void)0)
#define t2practice_diag_main_command(reason, command) ((void)0)
#define t2practice_diag_main_progress(progress, stage) ((void)0)
#define t2practice_diag_no_sound() 0
#define t2practice_diag_apply_begin(stage, target, map_length, spawn_rows) ((void)0)
#define t2practice_diag_target_scroll(target_scroll_step) ((void)0)
#define t2practice_diag_top_map_row(top_map_row) ((void)0)
#define t2practice_diag_spawn_first_trigger(trigger) ((void)0)
#define t2practice_diag_spawn_upper_bound(spawn_upper_bound) ((void)0)
#define t2practice_diag_failure(reason) ((void)0)
#define t2practice_diag_constructor_result(result) ((void)0)
#define t2practice_diag_apply_end(result) ((void)0)
#define t2practice_diag_lifecycle_op_menu_enter() ((void)0)
#define t2practice_diag_lifecycle(milestone, largest, chosen, available) ((void)0)
#define t2practice_diag_io_bypass_begin() ((void)0)
#define t2practice_diag_io_bypass_end() ((void)0)

#endif

#endif /* TH02_PRACTICE_DIAG_HPP */
