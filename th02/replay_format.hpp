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

// Private semantic-checkpoint vocabulary for later exact TH02 seeks. This
// does not change the T2RPY1 user replay payload yet. Groups append by ID;
// a reader only accepts the complete vocabulary it implements.
#define T2REPLAY_CHECKPOINT_SCHEMA 3
#define T2REPLAY_CHECKPOINT_GROUP_SCHEMA 1
#define T2REPLAY_CHECKPOINT_HEADER_SIZE 40
#define T2REPLAY_CHECKPOINT_GROUP_SIZE 20
#define T2REPLAY_CHECKPOINT_GROUP_COUNT 12
#define T2REPLAY_CHECKPOINT_IDENTITY_SIZE 12
#define T2REPLAY_CHECKPOINT_RNG_SIZE 264
#define T2REPLAY_CHECKPOINT_RUN_SIZE 43
#define T2REPLAY_CHECKPOINT_FIELD_SIZE 20
#define T2REPLAY_CHECKPOINT_STAGE_VM_SIZE 8
#define T2REPLAY_CHECKPOINT_PACING_SIZE 12
#define T2REPLAY_CHECKPOINT_PLAYER_SIZE 700
#define T2REPLAY_CHECKPOINT_BOMB_SIZE 279
#define T2REPLAY_CHECKPOINT_BULLET_SIZE 2860
#define T2REPLAY_CHECKPOINT_LASER_SIZE 147
#define T2REPLAY_CHECKPOINT_ENEMY_SIZE 904
#define T2REPLAY_CHECKPOINT_EFFECT_SIZE 1694
#define T2REPLAY_CHECKPOINT_CAPTURE_SIZE 7223
#define T2REPLAY_CHECKPOINT_GROUP_MASK 0x00000FFFUL
#define T2REPLAY_CHECKPOINT_SOURCE_FINGERPRINT 0x4C77B71AUL

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

enum t2replay_checkpoint_group_id_t {
	T2RCGI_IDENTITY = 0,
	T2RCGI_RNG = 1,
	T2RCGI_RUN = 2,
	T2RCGI_FIELD = 3,
	T2RCGI_STAGE_VM = 4,
	T2RCGI_PACING = 5,
	T2RCGI_PLAYER = 6,
	T2RCGI_BOMB = 7,
	T2RCGI_BULLET = 8,
	T2RCGI_LASER = 9,
	T2RCGI_ENEMY = 10,
	T2RCGI_EFFECT = 11,
};

enum t2replay_checkpoint_codec_t {
	T2RCC_RAW = 0,
};

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
typedef char t2rck_capture_size_check[
	(T2REPLAY_CHECKPOINT_CAPTURE_SIZE == (
		T2REPLAY_CHECKPOINT_HEADER_SIZE +
		(T2REPLAY_CHECKPOINT_GROUP_COUNT * T2REPLAY_CHECKPOINT_GROUP_SIZE) +
		T2REPLAY_CHECKPOINT_IDENTITY_SIZE +
		T2REPLAY_CHECKPOINT_RNG_SIZE +
		T2REPLAY_CHECKPOINT_RUN_SIZE +
		T2REPLAY_CHECKPOINT_FIELD_SIZE +
		T2REPLAY_CHECKPOINT_STAGE_VM_SIZE +
		T2REPLAY_CHECKPOINT_PACING_SIZE +
		T2REPLAY_CHECKPOINT_PLAYER_SIZE +
		T2REPLAY_CHECKPOINT_BOMB_SIZE +
		T2REPLAY_CHECKPOINT_BULLET_SIZE +
		T2REPLAY_CHECKPOINT_LASER_SIZE +
		T2REPLAY_CHECKPOINT_ENEMY_SIZE +
		T2REPLAY_CHECKPOINT_EFFECT_SIZE
	)) ? 1 : -1
];
typedef char t2replay_header_checksum_offset_check[
	(offsetof(t2replay_header_t, header_checksum) == 0x2C) ? 1 : -1
];
typedef char t2replay_header_start_offset_check[
	(offsetof(t2replay_header_t, start) == 0x30) ? 1 : -1
];
#endif /* TH02_REPLAY_FORMAT_HPP */
