#ifndef TH02_REPLAY_FORMAT_HPP
#define TH02_REPLAY_FORMAT_HPP

#include <stddef.h>
#include "platform.h"

#define T2REPLAY_VERSION 1
#define T2REPLAY_HEADER_SIZE 128
#define T2REPLAY_PACKET_SIZE 4
#define T2REPLAY_START_SIZE 36
#define T2REPLAY_COMMAND_SIZE 52
#define T2REPLAY_STAGE_COUNT 6
#define T2REPLAY_SLOT_COUNT 100
#define T2REPLAY_INPUT_SIZE_MAX 0x00400000UL

#define T2REPLAY_FLAG_RLE_INPUT 0x0001
#define T2REPLAY_FLAG_FULL_INPUT 0x0002
#define T2REPLAY_KNOWN_FLAGS (T2REPLAY_FLAG_RLE_INPUT | T2REPLAY_FLAG_FULL_INPUT)

#define T2REPLAY_STATUS_RECORDING 1
#define T2REPLAY_STATUS_FINALIZED 2
#define T2REPLAY_STATUS_ERROR 3

#define T2REPLAY_COMMAND_RECORD 1
#define T2REPLAY_COMMAND_PLAYBACK 2
#define T2REPLAY_COMMAND_FLAG_PRACTICE 0x01
#define T2REPLAY_COMMAND_KNOWN_FLAGS T2REPLAY_COMMAND_FLAG_PRACTICE

#define T2REPLAY_INPUT_SEMANTICS_KEY_DET 1
#define T2REPLAY_RULESET_STOCK 0

#define T2REPLAY_PHASE_GAMEPLAY 0
#define T2REPLAY_PHASE_PAUSE 1
#define T2REPLAY_PHASE_DIALOG 2
#define T2REPLAY_PHASE_CONTROL 3
#define T2REPLAY_PACKET_PHASE_SHIFT 6
#define T2REPLAY_PACKET_RUN_MASK 0x3F
#define T2REPLAY_PACKET_RUN_MAX 64

#define T2REPLAY_CONTROL_STAGE_START 1
#define T2REPLAY_CONTROL_TERMINAL 2

#define T2REPLAY_END_GAME_OVER 1
#define T2REPLAY_END_CLEAR 2

#define T2REPLAY_FNV1A_BASIS 0x811C9DC5UL
#define T2REPLAY_FNV1A_PRIME 0x01000193UL

struct t2replay_start_t {
	uint32_t resident_frame;
	uint32_t random_seed;
	int32_t score;
	uint32_t score_highest;
	uint16_t continues_used;
	int16_t skill;
	int8_t stage;
	uint8_t rank;
	int8_t rem_lives;
	int8_t rem_bombs;
	uint8_t start_lives;
	uint8_t start_bombs;
	int8_t start_power;
	uint8_t shottype;
	uint8_t bgm_mode;
	uint8_t reduce_effects;
	uint8_t debug;
	uint8_t reserved[5];
};

struct t2replay_header_t {
	char magic[8];
	uint16_t version;
	uint16_t header_size;
	uint16_t packet_size;
	uint16_t flags;
	uint8_t status;
	uint8_t end_reason;
	uint8_t game_id;
	uint8_t ruleset;
	uint8_t input_semantics;
	uint8_t stage_count;
	uint8_t stage_reached;
	uint8_t terminal_stage;
	uint32_t sample_count;
	uint32_t packet_count;
	uint32_t input_offset;
	uint32_t input_size;
	uint32_t payload_checksum;
	uint32_t header_checksum;
	t2replay_start_t start;
	uint32_t stage_scores[T2REPLAY_STAGE_COUNT];
	int32_t score_final;
	int8_t lives_final;
	int8_t bombs_final;
	uint8_t power_final;
	uint8_t reserved_0;
	uint8_t reserved[12];
};

struct t2replay_packet_t {
	uint8_t tag;
	uint8_t input_low;
	uint8_t input_high;
	uint8_t arg;
};

struct t2replay_command_t {
	char magic[8];
	uint8_t mode;
	uint8_t slot;
	uint8_t flags;
	uint8_t reserved_0;
	t2replay_start_t start;
	uint8_t reserved[4];
};

typedef char t2replay_start_size_check[
	(sizeof(t2replay_start_t) == T2REPLAY_START_SIZE) ? 1 : -1
];
typedef char t2replay_header_size_check[
	(sizeof(t2replay_header_t) == T2REPLAY_HEADER_SIZE) ? 1 : -1
];
typedef char t2replay_packet_size_check[
	(sizeof(t2replay_packet_t) == T2REPLAY_PACKET_SIZE) ? 1 : -1
];
typedef char t2replay_command_size_check[
	(sizeof(t2replay_command_t) == T2REPLAY_COMMAND_SIZE) ? 1 : -1
];
typedef char t2replay_header_checksum_offset_check[
	(offsetof(t2replay_header_t, header_checksum) == 0x2C) ? 1 : -1
];
typedef char t2replay_header_start_offset_check[
	(offsetof(t2replay_header_t, start) == 0x30) ? 1 : -1
];

#endif /* TH02_REPLAY_FORMAT_HPP */
