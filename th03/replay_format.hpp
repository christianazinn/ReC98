#ifndef TH03_REPLAY_FORMAT_HPP
#define TH03_REPLAY_FORMAT_HPP

#include "platform.h"

#define T3_REPLAY_USER_VERSION 5
#define T3_REPLAY_USER_INDEX_VERSION 4
#define T3_REPLAY_USER_PLAYER_COUNT 2
#define T3_REPLAY_USER_STAGE_COUNT 9
#define T3_REPLAY_USER_ROUND_SPLIT_COUNT 27
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
#define T3_REPLAY_USER_FLAG_SHIFT_INPUT 0x0002
#define T3_REPLAY_PACKET_PHASE_GAMEPLAY 0
#define T3_REPLAY_PACKET_PHASE_INTERSTITIAL 1
#define T3_REPLAY_PACKET_RUN_MAX 64
#define T3_REPLAY_PACKET_RUN_MASK 0x3F
#define T3_REPLAY_PACKET_PHASE_SHIFT 6
#define T3_REPLAY_PACKET_CHANGE_P1 0x01
#define T3_REPLAY_PACKET_CHANGE_P2 0x02
#define T3_REPLAY_PACKET_CHANGE_SP 0x04
#define T3_REPLAY_PACKET_CHANGE_SHIFT 0x08
#define T3_REPLAY_USER_SUMMARY_VALID 0x0001
#define T3_REPLAY_USER_SUMMARY_UNKNOWN 0xFF
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
	uint8_t stage_opponents[T3_REPLAY_USER_STAGE_COUNT];
	uint8_t stage_scores[
		T3_REPLAY_USER_STAGE_COUNT
	][T3_REPLAY_USER_PACKED_SCORE_SIZE];
	uint8_t final_score[T3_REPLAY_USER_PACKED_SCORE_SIZE];
	uint8_t reserved_metadata;
	uint16_t dos_date;
	char name[T3_REPLAY_USER_NAME_LEN];
};

struct replay_user_round_split_t {
	uint8_t stage_round;
	uint8_t route_winner;
	uint8_t score_p1[T3_REPLAY_USER_PACKED_SCORE_SIZE];
	uint8_t score_p2[T3_REPLAY_USER_PACKED_SCORE_SIZE];
};

struct replay_user_summary_ext_t {
	uint8_t flags;
	uint8_t round_reached_count;
	replay_user_round_split_t round_splits[T3_REPLAY_USER_ROUND_SPLIT_COUNT];
};

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
	uint8_t reserved[2];
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
	uint8_t reserved_metadata[6];
	uint16_t summary_flags;
	uint8_t final_route;
	uint8_t final_story_stage;
	uint8_t final_story_lives;
	uint8_t final_misses;
	uint8_t stage_reached_count;
	uint8_t final_score[T3_REPLAY_USER_PACKED_SCORE_SIZE];
	uint8_t stage_opponents[T3_REPLAY_USER_STAGE_COUNT];
};

#endif /* TH03_REPLAY_FORMAT_HPP */
