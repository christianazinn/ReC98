#ifndef TH03_REPLAY_FORMAT_HPP
#define TH03_REPLAY_FORMAT_HPP

#include <stddef.h>
#include "platform.h"
#include "th03/resident.hpp"

#define T3_REPLAY_USER_VERSION_LEGACY 11
#define T3_REPLAY_USER_VERSION 14
#define T3_REPLAY_USER_INDEX_VERSION 9
#define T3_REPLAY_USER_PLAYER_COUNT 2
#define T3_REPLAY_USER_STAGE_COUNT 9
#define T3_REPLAY_USER_ROUND_SPLIT_COUNT 27
#define T3R_CKPT_COUNT_STORY 15
#define T3R_CKPT_COUNT_VS 3
#define T3R_CKPT_COUNT_PRACTICE 5
#define T3R_CKPT_COUNT_MAX T3R_CKPT_COUNT_STORY
#define T3R_ACCEL_COUNT_MAX 4
#define T3R_ACCEL_CODEC_LZSS4K 1
#define T3R_ACCEL_BSS_OFFSET 0x1D94
#define T3R_ACCEL_BSS_END 0x8DFA
#define T3R_ACCEL_BSS_SIZE (T3R_ACCEL_BSS_END - T3R_ACCEL_BSS_OFFSET)
#define T3R_ACCEL_RAW_SIZE ( \
	T3R_ACCEL_BSS_SIZE + (T3_REPLAY_USER_FORMATION_RING_SIZE * 2) \
)
#define T3_REPLAY_USER_NAME_LEN 8
#define T3_REPLAY_USER_SCORE_DIGITS 8
#define T3_REPLAY_USER_SCORE_DISPLAY_DIGITS 9
#define T3_REPLAY_USER_PACKED_SCORE_SIZE 4
#define T3_REPLAY_USER_RANDRING_SIZE 256
#define T3_REPLAY_USER_CPU_CHARGE_RING_SIZE 64
#define T3_REPLAY_USER_FORMATION_RING_SIZE 256
#define T3_REPLAY_USER_SLOT_COUNT 100
#define T3_REPLAY_USER_SLOT_NONE 0xFF
#define T3_REPLAY_INTERSTITIAL_ROUND_OR_RESULT_FRAME 0xFFFF
#define T3_REPLAY_INTERSTITIAL_ROUND_FRAME 0xFFFFFFFFUL
#define T3_REPLAY_USER_SAMPLE_SIZE_RLE 0
#define T3_REPLAY_USER_FLAG_RLE_INPUT 0x0001
#define T3_REPLAY_USER_FLAG_CHARGE_INPUT 0x0002
#define T3_REPLAY_USER_FLAG_PRACTICE 0x0004
#define T3_REPLAY_PACKET_PHASE_GAMEPLAY 0
#define T3_REPLAY_PACKET_PHASE_INTERSTITIAL 1
#define T3_REPLAY_PACKET_PHASE_CONTROL 2
#define T3_REPLAY_PACKET_RUN_MAX 64
#define T3_REPLAY_PACKET_RUN_MASK 0x3F
#define T3_REPLAY_PACKET_PHASE_SHIFT 6
#define T3_REPLAY_PACKET_CHANGE_P1 0x01
#define T3_REPLAY_PACKET_CHANGE_P2 0x02
#define T3_REPLAY_PACKET_CHANGE_SP 0x04
#define T3_REPLAY_PACKET_CHANGE_CHARGE 0x08
#define T3_REPLAY_PACKET_SIZE_MAX 9
#define T3_REPLAY_DISK_INTERVAL_SAMPLES 128
#define T3_REPLAY_WRITE_BUFFER_SIZE ( \
	T3_REPLAY_PACKET_SIZE_MAX * T3_REPLAY_DISK_INTERVAL_SAMPLES \
)
#define T3_REPLAY_PACKET_CONTROL_MARKER 0xA5
#define T3_REPLAY_PACKET_CONTROL_MAIN_END 0
#define T3_REPLAY_PACKET_CONTROL_MAINL_END 1
#define T3_REPLAY_USER_SUMMARY_VALID 0x0001
#define T3_REPLAY_USER_SUMMARY_ROUND_RESUME_PHASE 0x0002
#define T3R_SUMMARY_ROUND_RESUME_CURSOR 0x0004
#define T3R_SUMMARY_FIREBALL_GENERATION 0x0008
#define T3R_SUMMARY_ROUND_REAL_FRAMES 0x0010
#define T3R_SUMMARY_STAGE_CLEAR_BONUS 0x0020
#define T3R_SUMMARY_SLOWDOWN 0x0040
#define T3_REPLAY_USER_SUMMARY_CURRENT ( \
	T3_REPLAY_USER_SUMMARY_VALID | \
	T3_REPLAY_USER_SUMMARY_ROUND_RESUME_PHASE | \
	T3R_SUMMARY_ROUND_RESUME_CURSOR | \
	T3R_SUMMARY_FIREBALL_GENERATION | \
	T3R_SUMMARY_ROUND_REAL_FRAMES | \
	T3R_SUMMARY_STAGE_CLEAR_BONUS | \
	T3R_SUMMARY_SLOWDOWN \
)
#define T3_REPLAY_USER_SUMMARY_UNKNOWN 0xFF
#define T3R_RULESET_STOCK 0
#define T3R_RECORDING_FLAG_NETPLAY 0x01
#define T3R_RECORDING_FLAGS_KNOWN T3R_RECORDING_FLAG_NETPLAY
#define T3R_RECORDER_ROLE_UNKNOWN 0
#define T3R_RECORDER_ROLE_P1 1
#define T3R_RECORDER_ROLE_P2 2
#define T3R_RECORDER_SOURCE_UNKNOWN 0
#define T3R_RECORDER_SOURCE_PC98 1
#define T3R_RECORDER_SOURCE_NATIVE 2
#define T3R_RECORDER_SOURCE_IMPORTED 3
#define T3R_ACCOUNT_UUID_SIZE 16
#define T3R_MATCH_ID_SIZE 16
#define T3R_NAMETAG_MAX_BYTES 57
#define T3R_IDENTITY_RESERVED_SIZE 106
#define T3_REPLAY_USER_ROUND_STAGE_VS 0x0F
#define T3_REPLAY_USER_ROUND_VALUE_UNKNOWN 0x0F
#define T3_REPLAY_SPLIT_VERSION 1
#define T3_REPLAY_SPLIT_HEADER_SIZE 16
#define T3_REPLAY_SPLIT_ROW_SIZE 34

enum replay_user_status_t {
	RUS_EMPTY = 0,
	RUS_RECORDING = 1,
	RUS_FINALIZED = 2,
	RUS_PARTIAL = 3,
	RUS_ERROR = 4,
};

enum replay_user_end_reason_t {
	RUER_NONE = 0,
	RUER_COMPLETE = 1,
	RUER_MENU_RETURN = 2,
	RUER_INPUT_END = 3,
	RUER_PARTIAL = 4,
	RUER_ERROR = 5,
	RUER_GAME_OVER = 6,
};

enum replay_user_background_phase_t {
	T3R_BACKGROUND_INITIAL = 0,
	T3R_BACKGROUND_TYPE_A_STEADY = 1,
	T3R_BACKGROUND_TYPE_B_STEADY = 2,
};

enum replay_user_result_phase_t {
	T3R_RESULT_IDLE = 0,
	T3R_RESULT_OPENING = 1,
	T3R_RESULT_CLOSING = 2,
};

enum replay_split_event_t {
	RSE_START = 1,
	RSE_ROUND_START = 2,
	RSE_INPUT_END = 3,
	RSE_ERROR = 4,
	RSE_CHECKPOINT = 5,
	RSE_FINISH = 6,
	RSE_ROUTE = 7,
};

struct replay_split_header_t {
	char magic[8];
	uint16_t version;
	uint16_t header_size;
	uint16_t row_size;
	uint16_t flags;
};

struct replay_split_row_t {
	uint8_t event;
	uint8_t route;
	uint8_t game_mode;
	uint8_t story_stage;
	uint8_t round_id;
	int8_t winner;
	uint8_t round_speed;
	uint8_t reserved;
	uint32_t global_frame;
	uint32_t round_frame;
	uint16_t round_or_result_frame;
	uint8_t score_p1[T3_REPLAY_USER_PACKED_SCORE_SIZE];
	uint8_t score_p2[T3_REPLAY_USER_PACKED_SCORE_SIZE];
	int32_t resident_rand;
	uint32_t state_hash;
};

typedef char replay_split_header_size_check[
	(sizeof(replay_split_header_t) == T3_REPLAY_SPLIT_HEADER_SIZE) ? 1 : -1
];
typedef char replay_split_row_size_check[
	(sizeof(replay_split_row_t) == T3_REPLAY_SPLIT_ROW_SIZE) ? 1 : -1
];

struct replay_user_practice_t {
	uint8_t preset;
	uint8_t stage;
	uint8_t round;
	uint8_t stock;
	uint8_t extends_gained;
	uint8_t cpu_timer;
	uint8_t round_speed;
	uint8_t bullet_speed;
	uint8_t p1_spell;
	uint8_t cpu_spell;
	uint8_t boss_level;
	uint8_t cpu_damage;
	uint16_t initial_cpu_safety_frames;
	uint8_t p1_gauge;
	uint8_t cpu_gauge;
};

typedef char replay_user_practice_size_check[
	(sizeof(replay_user_practice_t) == 16) ? 1 : -1
];

struct replay_user_story_summary_t {
	uint8_t stage_opponents[T3_REPLAY_USER_STAGE_COUNT];
	uint8_t stage_scores[
		T3_REPLAY_USER_STAGE_COUNT
	][T3_REPLAY_USER_PACKED_SCORE_SIZE];
};

struct replay_user_practice_summary_t {
	replay_user_practice_t config;
	uint8_t reserved[29];
};

union replay_user_scenario_summary_t {
	replay_user_story_summary_t story;
	replay_user_practice_summary_t practice;
};

typedef char replay_user_scenario_summary_size_check[
	(sizeof(replay_user_scenario_summary_t) == 45) ? 1 : -1
];

struct replay_user_header_t {
	char magic[8];
	uint16_t version;
	uint16_t header_size;
	uint16_t sample_size;
	uint16_t flags;
	uint8_t status;
	uint8_t end_reason;
	uint8_t game_mode;
	uint8_t rank;
	uint8_t key_mode;
	uint8_t playchar_p1;
	uint8_t playchar_p2;
	uint8_t story_stage;
	uint8_t is_cpu_p1;
	uint8_t is_cpu_p2;
	uint32_t sample_count;
	uint32_t final_frame_count;
	uint32_t resident_rand;
	uint32_t random_seed_snapshot;
	uint32_t input_offset;
	uint32_t input_size;
	uint32_t snapshot_offset;
	uint32_t snapshot_size;
	uint16_t summary_flags;
	uint8_t final_route;
	uint8_t final_game_mode;
	uint8_t final_story_stage;
	uint8_t final_round_id;
	uint8_t final_winner;
	uint8_t final_story_lives;
	uint8_t final_misses;
	uint8_t stage_reached_count;
	replay_user_scenario_summary_t scenario;
	uint8_t final_score[T3_REPLAY_USER_PACKED_SCORE_SIZE];
	uint8_t autofire;
	uint16_t dos_date;
	char name[T3_REPLAY_USER_NAME_LEN];
};

struct replay_user_round_split_t {
	uint8_t stage_round;
	uint8_t route_winner;
	uint8_t score_p1[T3_REPLAY_USER_PACKED_SCORE_SIZE];
	uint8_t score_p2[T3_REPLAY_USER_PACKED_SCORE_SIZE];
	uint32_t real_frames;
};

struct replay_user_stage_clear_bonus_t {
	uint8_t total[T3_REPLAY_USER_PACKED_SCORE_SIZE];
	uint8_t max_combo;
	uint8_t gauge_attacks;
	uint8_t boss_attacks;
	uint8_t boss_reversals;
	uint8_t boss_panics;
	uint8_t remaining_lives;
};

typedef char replay_user_round_split_size_check[
	(sizeof(replay_user_round_split_t) == 14) ? 1 : -1
];
typedef char replay_user_stage_clear_bonus_size_check[
	(sizeof(replay_user_stage_clear_bonus_t) == 10) ? 1 : -1
];

typedef char replay_user_header_size_check[
	(sizeof(replay_user_header_t) == 128) ? 1 : -1
];

struct replay_user_summary_ext_t {
	uint8_t flags;
	uint8_t round_reached_count;
	replay_user_round_split_t round_splits[T3_REPLAY_USER_ROUND_SPLIT_COUNT];
	replay_user_stage_clear_bonus_t stage_clear_bonuses[
		T3_REPLAY_USER_STAGE_COUNT
	];
	uint32_t timed_frames;
	uint32_t slow_frames;
	uint8_t checkpoint_count;
	uint8_t checkpoint_stage_round[T3R_CKPT_COUNT_MAX];
};

#define T3_REPLAY_USER_SUMMARY_EXT_V11_SIZE 272

typedef char replay_user_summary_ext_size_check[
	(sizeof(replay_user_summary_ext_t) == 494) ? 1 : -1
];

// V14 keeps the proven 622-byte V13 prefix byte-for-byte and appends portable
// product identity. Account UUIDs remain authoritative; these match-time
// nametags are only offline fallbacks for viewers without profile-service
// access. A zero length means that no fallback was available.
struct replay_user_identity_ext_t {
	uint8_t ruleset;
	uint8_t recording_flags;
	uint8_t recorder_role;
	uint8_t recorder_source;
	uint8_t player_uuid[T3_REPLAY_USER_PLAYER_COUNT][T3R_ACCOUNT_UUID_SIZE];
	uint8_t match_id[T3R_MATCH_ID_SIZE];
	uint8_t player_nametag_length[T3_REPLAY_USER_PLAYER_COUNT];
	char player_nametag[
		T3_REPLAY_USER_PLAYER_COUNT
	][T3R_NAMETAG_MAX_BYTES];
	uint8_t reserved[T3R_IDENTITY_RESERVED_SIZE];
};

typedef char replay_user_identity_ext_size_check[
	(sizeof(replay_user_identity_ext_t) == 274) ? 1 : -1
];

// Keep OP's frequently accessed round rows compact and load the larger V14
// telemetry into separate far arrays. This avoids moving OP's original near
// data while still exposing every V14 detail page.
struct replay_user_menu_round_split_t {
	uint8_t stage_round;
	uint8_t route_winner;
	uint8_t score_p1[T3_REPLAY_USER_PACKED_SCORE_SIZE];
	uint8_t score_p2[T3_REPLAY_USER_PACKED_SCORE_SIZE];
};

struct replay_user_menu_summary_ext_t {
	uint8_t flags;
	uint8_t round_reached_count;
	replay_user_menu_round_split_t round_splits[
		T3_REPLAY_USER_ROUND_SPLIT_COUNT
	];
	uint8_t checkpoint_count;
	uint8_t checkpoint_stage_round[T3R_CKPT_COUNT_MAX];
};

typedef char replay_user_menu_summary_ext_size_check[
	(sizeof(replay_user_menu_summary_ext_t) == 288) ? 1 : -1
];

struct replay_user_snapshot_t {
	uint32_t resident_rand;
	uint32_t random_seed_snapshot;
	uint8_t rank;
	uint8_t key_mode;
	uint8_t game_mode;
	uint8_t story_stage;
	uint8_t story_lives;
	uint8_t rem_credits;
	uint8_t skill;
	uint8_t demo_num;
	uint8_t pid_winner;
	uint8_t show_score_menu;
	uint8_t op_animation_fast;
	uint8_t is_cpu[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t playchar_paletted[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t story_opponents[T3_REPLAY_USER_STAGE_COUNT];
	uint8_t score_last[
		T3_REPLAY_USER_PLAYER_COUNT
	][T3_REPLAY_USER_SCORE_DIGITS];
	uint8_t randring_p;
	uint8_t formation_p[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t cpu_charge_at_avail_ring_p[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t autofire;
	uint8_t reserved[1];
	uint8_t randring[T3_REPLAY_USER_RANDRING_SIZE];
	uint8_t formation_type_ring[T3_REPLAY_USER_FORMATION_RING_SIZE];
	uint8_t formation_pos_type_ring[T3_REPLAY_USER_FORMATION_RING_SIZE];
	uint8_t cpu_charge_at_avail_ring[
		T3_REPLAY_USER_PLAYER_COUNT
	][T3_REPLAY_USER_CPU_CHARGE_RING_SIZE];
	uint16_t player_center_x[T3_REPLAY_USER_PLAYER_COUNT];
	uint16_t player_center_y[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t player_halfhearts[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t player_invincibility_time[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t player_gauge_charge_speed[T3_REPLAY_USER_PLAYER_COUNT];
	uint16_t player_gauge_charged[T3_REPLAY_USER_PLAYER_COUNT];
	uint16_t player_gauge_avail[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t player_bombs[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t player_shot_active[T3_REPLAY_USER_PLAYER_COUNT];
	uint16_t player_cpu_frame[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t reserved_player[1];
};

// V12 stores only state that cannot be regenerated from the round-reset RNG
// seed. [formation_first] disambiguates the original randomizer's
// uninitialized first comparison.
struct replay_user_snapshot_compact_t {
	uint32_t resident_rand;
	uint32_t random_seed_snapshot;
	uint8_t rank;
	uint8_t key_mode;
	uint8_t game_mode;
	uint8_t story_stage;
	uint8_t story_lives;
	uint8_t rem_credits;
	uint8_t skill;
	uint8_t demo_num;
	uint8_t pid_winner;
	uint8_t show_score_menu;
	uint8_t op_animation_fast;
	uint8_t is_cpu[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t playchar_paletted[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t story_opponents[T3_REPLAY_USER_STAGE_COUNT];
	uint8_t score_last[
		T3_REPLAY_USER_PLAYER_COUNT
	][T3_REPLAY_USER_SCORE_DIGITS];
	uint8_t autofire;
	uint16_t player_center_x[T3_REPLAY_USER_PLAYER_COUNT];
	uint16_t player_center_y[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t player_halfhearts[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t player_invincibility_time[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t player_gauge_charge_speed[T3_REPLAY_USER_PLAYER_COUNT];
	uint16_t player_gauge_charged[T3_REPLAY_USER_PLAYER_COUNT];
	uint16_t player_gauge_avail[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t player_bombs[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t player_shot_active[T3_REPLAY_USER_PLAYER_COUNT];
	uint16_t player_cpu_frame[T3_REPLAY_USER_PLAYER_COUNT];
	uint32_t round_reset_seed;
	uint8_t formation_first;
};

#define T3R_SNAPSHOT_COMMON_SIZE 48
#define T3R_SNAPSHOT_PLAYER_RUNTIME_SIZE 30

typedef char replay_user_snapshot_compact_size_check[
	(sizeof(replay_user_snapshot_compact_t) == 84) ? 1 : -1
];
typedef char replay_user_snapshot_common_offset_check[
	(offsetof(replay_user_snapshot_compact_t, autofire) ==
	 T3R_SNAPSHOT_COMMON_SIZE) ? 1 : -1
];
typedef char replay_user_snapshot_full_common_offset_check[
	(offsetof(replay_user_snapshot_t, randring_p) ==
	 T3R_SNAPSHOT_COMMON_SIZE) ? 1 : -1
];
typedef char replay_user_snapshot_runtime_offset_check[
	(offsetof(replay_user_snapshot_compact_t, player_center_x) == 49) ? 1 : -1
];
typedef char replay_user_snapshot_seed_offset_check[
	(offsetof(replay_user_snapshot_compact_t, round_reset_seed) == 79) ? 1 : -1
];
typedef char replay_user_snapshot_formation_offset_check[
	(offsetof(replay_user_snapshot_compact_t, formation_first) == 83) ? 1 : -1
];
typedef char replay_user_snapshot_full_runtime_offset_check[
	(offsetof(replay_user_snapshot_t, player_center_x) == 951) ? 1 : -1
];

struct replay_user_round_state_t {
	uint8_t round_id;
	uint8_t rounds_won[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t score[
		T3_REPLAY_USER_PLAYER_COUNT
	][T3_REPLAY_USER_SCORE_DIGITS];
	uint8_t round_speed;
	uint8_t bullet_speed;
	uint8_t spell_rank[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t boss_rank;
	uint8_t cpu_damage;
	uint8_t extends_gained;
	uint16_t cpu_safety_frames[T3_REPLAY_USER_PLAYER_COUNT];
};

typedef char replay_user_round_state_size_check[
	(sizeof(replay_user_round_state_t) == 30) ? 1 : -1
];

// State that the native retry path intentionally leaves live across rounds.
// A fresh-process checkpoint start must restore it after the ordinary round
// reset to reproduce the same state as an in-process retry.
struct replay_user_round_carry_t {
	uint8_t cpu_dodge_strategy[T3_REPLAY_USER_PLAYER_COUNT];
	uint16_t human_movement_last[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t spell_ready_frames[T3_REPLAY_USER_PLAYER_COUNT];
	uint16_t combo_bonus_max[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t combo_hits_max[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t shot_cycle[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t boss_panic_fired[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t gba_flag_next[T3_REPLAY_USER_PLAYER_COUNT];
	int8_t cpu_shot_decision;
	uint8_t background_phase;
	uint8_t combo_time[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t combo_hits_highest[T3_REPLAY_USER_PLAYER_COUNT];
	uint16_t combo_bonus_total[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t chain_ring_p[T3_REPLAY_USER_PLAYER_COUNT];
	uint8_t chain_hits[T3_REPLAY_USER_PLAYER_COUNT][16];
	uint8_t chain_pellet_and_fireball_value[
		T3_REPLAY_USER_PLAYER_COUNT
	][16];
	uint8_t chain_charge_fireball[T3_REPLAY_USER_PLAYER_COUNT][16];
	uint8_t chain_charge_exatt[T3_REPLAY_USER_PLAYER_COUNT][16];
	uint8_t randring_p;
	uint8_t result_phase;
	uint8_t fireball_generation_prev;
};

typedef char replay_user_round_carry_size_check[
	(sizeof(replay_user_round_carry_t) == 163) ? 1 : -1
];

#define T3R_STAGE_CKPT_PREFIX_SIZE 12
#define T3R_STAGE_CKPT_V11_SIZE ( \
	T3R_STAGE_CKPT_PREFIX_SIZE + \
	sizeof(replay_user_snapshot_t) \
)
#define T3R_STAGE_CKPT_V12_SIZE ( \
	T3R_STAGE_CKPT_PREFIX_SIZE + \
	sizeof(replay_user_snapshot_compact_t) + \
	sizeof(replay_user_round_state_t) + \
	sizeof(replay_user_round_carry_t) \
)
#define T3R_STAGE_CKPT_SIZE T3R_STAGE_CKPT_V12_SIZE
#define T3R_STAGE_CKPTS_V11_SIZE ( \
	T3_REPLAY_USER_STAGE_COUNT * T3R_STAGE_CKPT_V11_SIZE \
)

typedef char replay_user_checkpoint_size_check[
	(T3R_STAGE_CKPT_SIZE == 289) ? 1 : -1
];

struct replay_user_accel_temp_header_t {
	char magic[4];
	uint8_t checkpoint;
	uint8_t codec;
	uint16_t header_size;
	uint16_t raw_size;
	uint16_t packed_size;
	uint32_t state_hash;
};

struct replay_user_accel_desc_t {
	uint8_t checkpoint;
	uint8_t codec;
	uint16_t raw_size;
	uint32_t offset;
	uint32_t packed_size;
	uint32_t state_hash;
};

struct replay_user_accel_footer_t {
	char magic[8];
	uint16_t version;
	uint16_t footer_size;
	uint8_t count;
	uint8_t reserved[3];
	replay_user_accel_desc_t records[T3R_ACCEL_COUNT_MAX];
};

typedef char replay_user_accel_temp_header_size_check[
	(sizeof(replay_user_accel_temp_header_t) == 16) ? 1 : -1
];
typedef char replay_user_accel_desc_size_check[
	(sizeof(replay_user_accel_desc_t) == 16) ? 1 : -1
];
typedef char replay_user_accel_footer_size_check[
	(sizeof(replay_user_accel_footer_t) == 80) ? 1 : -1
];
typedef char replay_user_accel_raw_size_check[
	(T3R_ACCEL_RAW_SIZE <= 0xFFFFUL) ? 1 : -1
];

inline uint8_t replay_user_checkpoint_capacity(
	uint8_t game_mode, uint16_t flags
)
{
	if(game_mode == GM_STORY) {
		return T3R_CKPT_COUNT_STORY;
	}
	if(flags & T3_REPLAY_USER_FLAG_PRACTICE) {
		return T3R_CKPT_COUNT_PRACTICE;
	}
	return T3R_CKPT_COUNT_VS;
}

inline uint32_t replay_user_checkpoint_reservation_size(
	uint16_t version, uint8_t game_mode, uint16_t flags
)
{
	if(version == T3_REPLAY_USER_VERSION_LEGACY) {
		return (
			(game_mode == GM_STORY) ?
				static_cast<uint32_t>(T3R_STAGE_CKPTS_V11_SIZE) :
				static_cast<uint32_t>(T3R_STAGE_CKPT_V11_SIZE)
		);
	}
	return (
		static_cast<uint32_t>(
			replay_user_checkpoint_capacity(game_mode, flags)
		) * static_cast<uint32_t>(T3R_STAGE_CKPT_SIZE)
	);
}

inline uint16_t replay_user_summary_ext_size(uint16_t version)
{
	return (
		(version == T3_REPLAY_USER_VERSION_LEGACY) ?
			T3_REPLAY_USER_SUMMARY_EXT_V11_SIZE :
			sizeof(replay_user_summary_ext_t)
	);
}

inline uint16_t replay_user_checkpoint_size(uint16_t version)
{
	if(version == T3_REPLAY_USER_VERSION_LEGACY) {
		return T3R_STAGE_CKPT_V11_SIZE;
	}
	return T3R_STAGE_CKPT_SIZE;
}

inline bool replay_user_version_supported(uint16_t version)
{
	return (version == T3_REPLAY_USER_VERSION);
}

inline bool replay_user_version_has_round_state(uint16_t version)
{
	return (version != T3_REPLAY_USER_VERSION_LEGACY);
}

inline bool replay_user_version_has_round_carry(uint16_t version)
{
	return (version == T3_REPLAY_USER_VERSION);
}

inline uint16_t replay_user_header_size(uint16_t version)
{
	return static_cast<uint16_t>(
		sizeof(replay_user_header_t) + replay_user_summary_ext_size(version) +
		((version == T3_REPLAY_USER_VERSION) ?
		 sizeof(replay_user_identity_ext_t) : 0)
	);
}

inline uint32_t replay_user_input_offset(
	uint16_t version, uint8_t game_mode, uint16_t flags
)
{
	return (
		static_cast<uint32_t>(replay_user_header_size(version)) +
		replay_user_checkpoint_reservation_size(version, game_mode, flags)
	);
}

struct replay_user_sample_t {
	uint32_t frame_index;
	uint16_t input_mp_p1;
	uint16_t input_mp_p2;
	uint16_t input_sp;
	uint16_t round_or_result_frame;
	uint32_t round_frame;
};

struct replay_user_index_header_t {
	char magic[8];
	uint16_t version;
	uint16_t header_size;
	uint16_t entry_size;
	uint16_t slot_count;
	uint8_t next_slot;
	uint8_t reserved[47];
};

struct replay_user_index_entry_t {
	uint8_t used;
	uint8_t slot_id;
	uint8_t status;
	uint8_t end_reason;
	uint8_t game_mode;
	uint8_t rank;
	uint8_t key_mode;
	uint8_t playchar_p1;
	uint8_t playchar_p2;
	uint8_t story_stage;
	uint8_t is_cpu_p1;
	uint8_t is_cpu_p2;
	uint32_t sample_count;
	uint32_t final_frame_count;
	char name[T3_REPLAY_USER_NAME_LEN];
	uint16_t dos_date;
	uint8_t autofire;
	uint8_t replay_flags;
	uint8_t reserved_metadata[4];
	uint16_t summary_flags;
	uint8_t final_route;
	uint8_t final_story_stage;
	uint8_t final_story_lives;
	uint8_t final_misses;
	uint8_t stage_reached_count;
	uint8_t final_score[T3_REPLAY_USER_PACKED_SCORE_SIZE];
	uint8_t stage_opponents[T3_REPLAY_USER_STAGE_COUNT];
};

typedef char replay_user_index_entry_size_check[
	(sizeof(replay_user_index_entry_t) == 56) ? 1 : -1
];

#endif /* TH03_REPLAY_FORMAT_HPP */
