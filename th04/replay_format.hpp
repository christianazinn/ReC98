#ifndef TH04_REPLAY_FORMAT_HPP
#define TH04_REPLAY_FORMAT_HPP

#include <stddef.h>
#include "platform.h"

#define REPLAY_USER_VERSION 1
#define REPLAY_USER_HEADER_SIZE 128
#define REPLAY_USER_PACKET_SIZE 4
#define REPLAY_USER_INPUT_SIZE_MAX 0x00400000UL
#define REPLAY_USER_STAGE_COUNT 7
#define REPLAY_USER_NAME_LEN 8
#define REPLAY_USER_SLOT_COUNT 100

#define REPLAY_USER_FLAG_RLE_INPUT 0x0001
#define REPLAY_USER_FLAG_SHIFT_INPUT 0x0002
#define REPLAY_USER_FLAG_PRACTICE 0x0004
#define REPLAY_USER_KNOWN_FLAGS ( \
	REPLAY_USER_FLAG_RLE_INPUT | \
	REPLAY_USER_FLAG_SHIFT_INPUT | \
	REPLAY_USER_FLAG_PRACTICE \
)

#define REPLAY_USER_INPUT_SEMANTICS 1
#define REPLAY_USER_RULESET_STOCK 0

#define REPLAY_PACKET_PHASE_GAMEPLAY 0
#define REPLAY_PACKET_PHASE_INTERSTITIAL 1
#define REPLAY_PACKET_PHASE_CONTROL 2
#define REPLAY_PACKET_PHASE_INVALID 3
#define REPLAY_PACKET_PHASE_SHIFT 6
#define REPLAY_PACKET_RUN_MASK 0x3F
#define REPLAY_PACKET_RUN_MAX 64

#define REPLAY_CONTROL_STAGE_START 1
#define REPLAY_CONTROL_TERMINAL 2

#define REPLAY_FNV1A_BASIS 0x811C9DC5UL
#define REPLAY_FNV1A_PRIME 0x01000193UL

enum replay_user_status_t {
	RUS_EMPTY = 0,
	RUS_RECORDING = 1,
	RUS_FINALIZED = 2,
	RUS_ERROR = 3,
};

enum replay_user_end_reason_t {
	RUER_NONE = 0,
	RUER_COMPLETE = 1,
	RUER_MENU_RETURN = 2,
	RUER_GAME_OVER = 3,
	RUER_INPUT_END = 4,
	RUER_ERROR = 5,
};

enum replay_user_mode_t {
	RUM_STORY = 0,
	RUM_PRACTICE = 1,
};

enum replay_command_mode_t {
	RCM_NONE = 0,
	RCM_RECORD = 1,
	RCM_PLAYBACK = 2,
};

struct replay_user_header_t {
	char magic[8];
	uint16_t version;
	uint16_t header_size;
	uint16_t packet_size;
	uint16_t flags;
	uint8_t status;
	uint8_t end_reason;
	uint8_t game_id;
	uint8_t ruleset;
	uint8_t mode;
	uint8_t rank;
	uint8_t playchar;
	uint8_t shottype;
	uint8_t start_stage;
	uint8_t start_section;
	uint8_t stage_reached;
	uint8_t input_semantics;
	uint32_t sample_count;
	uint32_t packet_count;
	uint32_t input_offset;
	uint32_t input_size;
	uint32_t checkpoint_offset;
	uint32_t checkpoint_size;
	uint32_t payload_checksum;
	uint32_t header_checksum;
	int32_t resident_rand;
	int32_t random_seed;
	uint32_t score_start;
	uint32_t score_final;
	uint8_t credit_lives;
	uint8_t credit_bombs;
	uint8_t lives_start;
	uint8_t bombs_start;
	uint8_t power_start;
	uint8_t dream_start;
	uint8_t lives_final;
	uint8_t bombs_final;
	uint16_t dos_date;
	uint16_t dos_time;
	char name[REPLAY_USER_NAME_LEN];
	uint32_t stage_scores[REPLAY_USER_STAGE_COUNT];
	uint8_t turbo_mode;
	uint8_t reserved[3];
};

struct replay_user_packet_t {
	uint8_t tag;
	uint8_t input_low;
	uint8_t input_high;
	uint8_t shift;
};

struct replay_command_t {
	char magic[8];
	uint8_t mode;
	uint8_t slot;
	uint8_t reserved[6];
};

typedef char replay_user_header_size_check[
	(sizeof(replay_user_header_t) == REPLAY_USER_HEADER_SIZE) ? 1 : -1
];
typedef char replay_user_packet_size_check[
	(sizeof(replay_user_packet_t) == REPLAY_USER_PACKET_SIZE) ? 1 : -1
];
typedef char replay_command_size_check[
	(sizeof(replay_command_t) == 16) ? 1 : -1
];
typedef char replay_user_header_checksum_offset_check[
	(offsetof(replay_user_header_t, header_checksum) == 0x38) ? 1 : -1
];
typedef char replay_user_header_stage_scores_offset_check[
	(offsetof(replay_user_header_t, stage_scores) == 0x60) ? 1 : -1
];
typedef char replay_user_header_turbo_mode_offset_check[
	(offsetof(replay_user_header_t, turbo_mode) == 0x7C) ? 1 : -1
];

#endif /* TH04_REPLAY_FORMAT_HPP */
