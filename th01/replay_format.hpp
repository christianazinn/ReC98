#ifndef TH01_REPLAY_FORMAT_HPP
#define TH01_REPLAY_FORMAT_HPP

/*
 * TH01's first user replay format. This is a compact, deliberately narrow
 * container for a full REIIDEN run. It has no in-game UI or seek support yet;
 * those need separate, measured control-flow work.
 */

#include "platform.h"
#include <stddef.h>

#define T1REPLAY_VERSION 1
#define T1REPLAY_HEADER_SIZE 128
#define T1REPLAY_START_SIZE 64
#define T1REPLAY_PACKET_SIZE 8
#define T1REPLAY_SLOT_COUNT 100
#define T1REPLAY_INPUT_SIZE_MAX 0x00400000UL

#define T1REPLAY_STATUS_RECORDING 1
#define T1REPLAY_STATUS_FINALIZED 2
#define T1REPLAY_STATUS_ERROR 3

#define T1REPLAY_COMMAND_RECORD 1
#define T1REPLAY_COMMAND_PLAYBACK 2

#define T1REPLAY_FLAG_RLE 0x0001
#define T1REPLAY_FLAG_KEY_LATCH 0x0002
#define T1REPLAY_FLAGS_KNOWN (T1REPLAY_FLAG_RLE | T1REPLAY_FLAG_KEY_LATCH)

#define T1REPLAY_INPUT_SEMANTICS_LATCHED_GROUPS 1

#define T1REPLAY_PACKET_CONTROL 0x80
#define T1REPLAY_PACKET_RUN_MASK 0x7F
#define T1REPLAY_PACKET_RUN_MAX (T1REPLAY_PACKET_RUN_MASK + 1)
#define T1REPLAY_CONTROL_PROCESS_END 1
#define T1REPLAY_CONTROL_TERMINAL 2

#define T1REPLAY_END_MENU 1
#define T1REPLAY_END_CLEAR 2

#define T1REPLAY_PROCESS_REIIDEN 1

#define T1REPLAY_FNV1A_BASIS 0x811C9DC5UL
#define T1REPLAY_FNV1A_PRIME 0x01000193UL

enum t1replay_input_group_t {
	T1RIG_0 = 0,
	T1RIG_3,
	T1RIG_5,
	T1RIG_6,
	T1RIG_7,
	T1RIG_8,
	T1RIG_9,
	T1REPLAY_INPUT_GROUP_COUNT,
};

// Only bits consumed by TH01's REIIDEN input path are replayed. Keeping the
// stream canonical prevents unrelated keyboard state from becoming format ABI.
#define T1REPLAY_INPUT_MASK_0 0x01
#define T1REPLAY_INPUT_MASK_3 0x10
#define T1REPLAY_INPUT_MASK_5 0x06
#define T1REPLAY_INPUT_MASK_6 0xC0
#define T1REPLAY_INPUT_MASK_7 0x3C
#define T1REPLAY_INPUT_MASK_8 0x48
#define T1REPLAY_INPUT_MASK_9 0x09

// This is intentionally an explicit field list, not a byte-copy of
// resident_t. It is the cross-EXE state REIIDEN actually consumes before the
// first input sample. Its exact shape is shared with the host parser.
struct t1replay_start_t {
	uint32_t resident_rand;
	int32_t score;
	int32_t continues_total;
	uint32_t hiscore;
	int32_t score_highest;
	int32_t bonus_per_stage[4];
	uint16_t continues_per_scene[4];
	uint16_t stage_id;
	uint16_t point_value;
	int16_t pellet_speed;
	int8_t rank;
	int8_t bgm_mode;
	int8_t rem_bombs;
	int8_t credit_lives_extra;
	int8_t rem_lives;
	int8_t route;
	int8_t end_flag;
	int8_t debug_mode;
	int8_t snd_need_init;
	int8_t mode_test;
	int8_t start_binary;
	uint8_t reserved[3];
};

struct t1replay_header_t {
	char magic[8]; // "T1RPY1\\0\\0"
	uint16_t version;
	uint16_t header_size;
	uint16_t packet_size;
	uint16_t flags;
	uint8_t status;
	uint8_t end_reason;
	uint8_t game_id;
	uint8_t input_semantics;
	uint8_t process_count;
	uint8_t reserved_0;
	uint32_t sample_count;
	uint32_t packet_count;
	uint32_t input_offset;
	uint32_t input_size;
	uint32_t payload_checksum;
	uint32_t start_checksum;
	uint32_t header_checksum;
	t1replay_start_t start;
	uint8_t reserved[14];
};

struct t1replay_packet_t {
	uint8_t tag;
	uint8_t keys[T1REPLAY_INPUT_GROUP_COUNT];
};

struct t1replay_command_t {
	char magic[8]; // "T1RPYC\\0\\0"
	uint8_t mode;
	uint8_t slot;
	uint8_t reserved[6];
};

typedef char t1replay_start_size_check[
	(sizeof(t1replay_start_t) == T1REPLAY_START_SIZE) ? 1 : -1
];
typedef char t1replay_header_size_check[
	(sizeof(t1replay_header_t) == T1REPLAY_HEADER_SIZE) ? 1 : -1
];
typedef char t1replay_packet_size_check[
	(sizeof(t1replay_packet_t) == T1REPLAY_PACKET_SIZE) ? 1 : -1
];
typedef char t1replay_command_size_check[
	(sizeof(t1replay_command_t) == 16) ? 1 : -1
];
typedef char t1replay_header_start_offset_check[
	(offsetof(t1replay_header_t, start) == 50) ? 1 : -1
];
typedef char t1replay_header_checksum_offset_check[
	(offsetof(t1replay_header_t, header_checksum) == 46) ? 1 : -1
];

#endif /* TH01_REPLAY_FORMAT_HPP */
