#ifndef TH03_REPLAY_FORMAT_HPP
#define TH03_REPLAY_FORMAT_HPP

#include "platform.h"

#define T3_REPLAY_USER_VERSION 2
#define T3_REPLAY_USER_INDEX_VERSION 2
#define T3_REPLAY_USER_PLAYER_COUNT 2
#define T3_REPLAY_USER_STAGE_COUNT 9
#define T3_REPLAY_USER_SCORE_DIGITS 8
#define T3_REPLAY_USER_RANDRING_SIZE 256
#define T3_REPLAY_USER_CPU_CHARGE_RING_SIZE 64
#define T3_REPLAY_USER_FORMATION_RING_SIZE 256
#define T3_REPLAY_USER_SLOT_COUNT 100
#define T3_REPLAY_USER_SLOT_NONE 0xFF
#define T3_REPLAY_INTERSTITIAL_ROUND_OR_RESULT_FRAME 0xFFFF
#define T3_REPLAY_INTERSTITIAL_ROUND_FRAME 0xFFFFFFFFUL

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
};

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
	uint32_t input_crc32;
	uint32_t snapshot_crc32;
	uint8_t reserved[62];
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
	uint32_t resident_rand;
	uint32_t random_seed_snapshot;
	uint32_t input_crc32;
	uint32_t snapshot_crc32;
	uint8_t reserved[20];
};

#endif /* TH03_REPLAY_FORMAT_HPP */
