#ifndef TH04_REPLAY_FORMAT_HPP
#define TH04_REPLAY_FORMAT_HPP

#include <stddef.h>
#include "platform.h"

#define REPLAY_USER_VERSION 3
#define REPLAY_USER_HEADER_SIZE 192
#define REPLAY_USER_PACKET_SIZE 4
#define REPLAY_USER_INPUT_SIZE_MAX 0x00400000UL
#define REPLAY_USER_STAGE_COUNT 7
#define REPLAY_USER_NAME_LEN 8
#define REPLAY_USER_SLOT_COUNT 100
#define REPLAY_START_CONFIG_SIZE 64
#define REPLAY_COMMAND_SIZE 80
#define REPLAY_CHECKPOINT_SCHEMA 1
#define REPLAY_CHECKPOINT_HEADER_SIZE 40
#define REPLAY_CHECKPOINT_GROUP_SIZE 20
#define REPLAY_CHECKPOINT_GROUP_SCHEMA 1
#define REPLAY_CKPT_GROUPS_TH04 12
#define REPLAY_CKPT_GROUPS_TH05 13
#define REPLAY_CKPT_GROUPS_MAX 13
#define REPLAY_CHECKPOINT_SIZE_MAX 0xFFF0u

// Source fingerprint shared by the normalized TH04/TH05 checkpoint schemas.
// Change this only when checkpoint interpretation changes.
#define REPLAY_CHECKPOINT_SOURCE_FINGERPRINT 0x9533C814UL

#define REPLAY_USER_FLAG_RLE_INPUT 0x0001
#define REPLAY_USER_FLAG_SHIFT_INPUT 0x0002
#define REPLAY_USER_FLAG_PRACTICE 0x0004
#define REPLAY_USER_FLAG_CHECKPOINT 0x0008
#define REPLAY_USER_KNOWN_FLAGS ( \
	REPLAY_USER_FLAG_RLE_INPUT | \
	REPLAY_USER_FLAG_SHIFT_INPUT | \
	REPLAY_USER_FLAG_PRACTICE | \
	REPLAY_USER_FLAG_CHECKPOINT \
)

#define REPLAY_COMMAND_FLAG_PRACTICE 0x01
#define REPLAY_COMMAND_FLAG_PRIVATE_TEST 0x02
#define REPLAY_COMMAND_FLAG_NO_RECORD 0x04
#define REPLAY_COMMAND_KNOWN_FLAGS ( \
	REPLAY_COMMAND_FLAG_PRACTICE | REPLAY_COMMAND_FLAG_PRIVATE_TEST | \
	REPLAY_COMMAND_FLAG_NO_RECORD \
)

#define REPLAY_USER_INPUT_SEMANTICS 1
#define REPLAY_USER_RULESET_STOCK 0
#define REPLAY_START_SCHEMA 1

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

enum replay_start_kind_t {
	RSK_NATIVE = 0,
	RSK_STAGE = 1,
	RSK_CHAPTER = 2,
	RSK_MIDBOSS = 3,
	RSK_BOSS_PHASE = 4,
};

enum replay_checkpoint_group_id_t {
	RCGI_RNG = 0,
	RCGI_RUN = 1,
	RCGI_PLAYER = 2,
	RCGI_BULLETS = 3,
	RCGI_ENEMIES = 4,
	RCGI_ACTORS = 5,
	RCGI_ITEMS = 6,
	RCGI_SCORING = 7,
	RCGI_FIELD = 8,
	RCGI_EFFECTS = 9,
	RCGI_STAGE_VM = 10,
	RCGI_PACING = 11,
	RCGI_DIALOG = 12,
};

enum replay_checkpoint_codec_t {
	RCC_RAW = 0,
};

enum replay_checkpoint_section_t {
	RCS_CHAPTER_2 = 2,
	RCS_CHAPTER_3 = 3,
	RCS_MIDBOSS_PRIMARY = 0,
	RCS_MIDBOSS_SECONDARY = 1,
	RCS_TH04_MUGETSU = 0,
	RCS_TH04_GENGETSU = 1,
	RCS_TH05_PAIR = 0,
	RCS_TH05_MAI = 1,
	RCS_TH05_YUKI = 2,
};

enum replay_command_mode_t {
	RCM_NONE = 0,
	RCM_RECORD = 1,
	RCM_PLAYBACK = 2,
};

struct replay_start_config_t {
	uint8_t schema;
	uint8_t kind;
	uint8_t stage;
	uint8_t section;
	uint8_t phase;
	uint8_t rank;
	uint8_t playchar;
	uint8_t shottype;
	uint8_t lives;
	uint8_t bombs;
	uint8_t power;
	uint8_t dream;
	uint8_t playperf;
	uint8_t continues_used;
	uint8_t extends_gained;
	uint8_t turbo_mode;
	uint32_t score;
	int32_t resident_rand;
	int32_t random_seed;
	uint16_t graze;
	uint16_t std_frames;
	uint16_t items_spawned;
	uint16_t items_collected;
	uint16_t point_items_collected;
	uint16_t max_valued_point_items_collected;
	uint16_t enemies_gone;
	uint16_t enemies_killed;
	uint8_t miss_count;
	uint8_t bombs_used;
	uint8_t credit_lives;
	uint8_t credit_bombs;
	uint16_t stage_point_items_collected;
	uint16_t stage_graze;
	uint16_t power_overflow;
	uint8_t reserved[10];
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
	uint8_t input_semantics;
	uint8_t stage_reached;
	uint8_t checkpoint_schema;
	uint32_t sample_count;
	uint32_t packet_count;
	uint32_t input_offset;
	uint32_t input_size;
	uint32_t checkpoint_offset;
	uint32_t checkpoint_size;
	uint32_t payload_checksum;
	uint32_t checkpoint_checksum;
	uint32_t header_checksum;
	uint32_t score_final;
	uint16_t dos_date;
	uint16_t dos_time;
	char name[REPLAY_USER_NAME_LEN];
	uint32_t stage_scores[REPLAY_USER_STAGE_COUNT];
	replay_start_config_t start;
	uint32_t source_fingerprint;
	uint32_t state_digest;
	uint8_t lives_final;
	uint8_t bombs_final;
	uint8_t power_final;
	uint8_t dream_final;
	uint8_t reserved[12];
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
	uint8_t flags;
	uint8_t reserved_0;
	replay_start_config_t start;
	uint8_t reserved[4];
};

struct replay_checkpoint_header_t {
	char magic[8];
	uint16_t schema;
	uint16_t header_size;
	uint8_t game_id;
	uint8_t start_kind;
	uint8_t stage;
	uint8_t section;
	uint8_t phase;
	uint8_t group_count;
	uint16_t flags;
	uint32_t total_size;
	uint32_t source_fingerprint;
	uint32_t state_digest;
	uint32_t decoded_size;
	uint32_t container_checksum;
};

struct replay_checkpoint_group_t {
	uint8_t id;
	uint8_t schema;
	uint8_t codec;
	uint8_t flags;
	uint32_t offset;
	uint32_t stored_size;
	uint32_t decoded_size;
	uint32_t checksum;
};

typedef char replay_start_config_size_check[
	(sizeof(replay_start_config_t) == REPLAY_START_CONFIG_SIZE) ? 1 : -1
];
typedef char replay_user_header_size_check[
	(sizeof(replay_user_header_t) == REPLAY_USER_HEADER_SIZE) ? 1 : -1
];
typedef char replay_user_packet_size_check[
	(sizeof(replay_user_packet_t) == REPLAY_USER_PACKET_SIZE) ? 1 : -1
];
typedef char replay_command_size_check[
	(sizeof(replay_command_t) == REPLAY_COMMAND_SIZE) ? 1 : -1
];
typedef char replay_checkpoint_header_size_check[
	(sizeof(replay_checkpoint_header_t) == REPLAY_CHECKPOINT_HEADER_SIZE) ? 1 : -1
];
typedef char replay_checkpoint_group_size_check[
	(sizeof(replay_checkpoint_group_t) == REPLAY_CHECKPOINT_GROUP_SIZE) ? 1 : -1
];
typedef char replay_user_header_checksum_offset_check[
	(offsetof(replay_user_header_t, header_checksum) == 0x38) ? 1 : -1
];
typedef char replay_user_header_stage_scores_offset_check[
	(offsetof(replay_user_header_t, stage_scores) == 0x4C) ? 1 : -1
];
typedef char replay_user_header_start_offset_check[
	(offsetof(replay_user_header_t, start) == 0x68) ? 1 : -1
];
typedef char replay_start_score_offset_check[
	(offsetof(replay_start_config_t, score) == 0x10) ? 1 : -1
];
typedef char replay_start_stage_points_offset_check[
	(offsetof(replay_start_config_t, stage_point_items_collected) == 0x30) ? 1 : -1
];
typedef char replay_start_stage_graze_offset_check[
	(offsetof(replay_start_config_t, stage_graze) == 0x32) ? 1 : -1
];
typedef char replay_checkpoint_total_size_offset_check[
	(offsetof(replay_checkpoint_header_t, total_size) == 0x14) ? 1 : -1
];
typedef char replay_checkpoint_checksum_offset_check[
	(offsetof(replay_checkpoint_header_t, container_checksum) == 0x24) ? 1 : -1
];
typedef char replay_checkpoint_group_offset_check[
	(offsetof(replay_checkpoint_group_t, offset) == 0x04) ? 1 : -1
];

#endif /* TH04_REPLAY_FORMAT_HPP */
