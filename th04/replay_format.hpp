#ifndef TH04_REPLAY_FORMAT_HPP
#define TH04_REPLAY_FORMAT_HPP

#include <stddef.h>
#include "platform.h"

#define REPLAY_USER_VERSION 6
#define REPLAY_USER_VERSION_V5 5
#define REPLAY_USER_VERSION_LEGACY 4
#define REPLAY_USER_HEADER_SIZE 192
#define REPLAY_USER_PACKET_SIZE 4
#define REPLAY_USER_INPUT_SIZE_MAX 0x00400000UL
#define REPLAY_USER_STAGE_COUNT 7
#define REPLAY_STAGE_ENTRY_SIZE 76
#define REPLAY_STAGE_DIRECTORY_SIZE \
	(REPLAY_USER_STAGE_COUNT * REPLAY_STAGE_ENTRY_SIZE)
#define REPLAY_USER_INPUT_OFFSET \
	(REPLAY_USER_HEADER_SIZE + REPLAY_STAGE_DIRECTORY_SIZE)
#define REPLAY_USER_NAME_LEN 8
#define REPLAY_USER_SLOT_COUNT 100
#define REPLAY_PRACTICE_STOCK_MAX 15
#define REPLAY_SCORE_MAX 2559999990UL
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
#define REPLAY_CHECKPOINT_SOURCE_FINGERPRINT 0xDD19B1F3UL

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
#define REPLAY_COMMAND_FLAG_TEMP_CAPTURE 0x08
#define REPLAY_COMMAND_FLAG_DIAGNOSTIC 0x80
#define REPLAY_COMMAND_STAGE_SHIFT 4
#define REPLAY_COMMAND_STAGE_MASK 0x70
#define REPLAY_COMMAND_STAGE_NONE 0
#define REPLAY_COMMAND_KNOWN_FLAGS ( \
	REPLAY_COMMAND_FLAG_PRACTICE | REPLAY_COMMAND_FLAG_PRIVATE_TEST | \
	REPLAY_COMMAND_FLAG_NO_RECORD | REPLAY_COMMAND_FLAG_TEMP_CAPTURE | \
	REPLAY_COMMAND_STAGE_MASK | REPLAY_COMMAND_FLAG_DIAGNOSTIC \
)

#define REPLAY_SAVE_REQUEST_SCHEMA 1
#define REPLAY_SAVE_REQUEST_SIZE 20
#define REPLAY_SAVE_TXN_SCHEMA 1
#define REPLAY_SAVE_TXN_SIZE 20

#define REPLAY_USER_INPUT_SEMANTICS 1
#define REPLAY_USER_RULESET_STOCK 0
#define REPLAY_START_SCHEMA 2
#define REPLAY_START_SCHEMA_LEGACY 1

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
	RUS_PENDING = 4,
};

enum replay_save_request_source_t {
	RSRS_POSTGAME = 0,
	RSRS_PAUSE_SAVE_EXIT = 1,
};

enum replay_save_txn_state_t {
	RSTS_PREPARED = 1,
	RSTS_BACKUP_MOVED = 2,
	RSTS_INSTALLED = 3,
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

enum replay_seed_mode_t {
	RSM_RANDOM = 0,
	RSM_FIXED = 1,
	RSM_VALUE_MASK = 1,
	RSM_TIMEDOWN = 0x40,
	RSM_RANK_LOCK = 0x80,
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
	RCM_RESTART = 3,
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
	uint8_t seed_mode;
	// Schema 2 reclaims schema 1's nine-byte reserved tail. TH04 carries both
	// score accumulators across native stage boundaries; the popup latch is
	// shared presentation state in both games.
	uint32_t score_delta;
	uint32_t score_delta_frame;
	uint8_t hiscore_popup_shown;
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
	uint32_t stage_directory_checksum;
	// V5 reclaims V4's all-zero tail for run-wide host-performance telemetry.
	// These counters are presentation metadata: replay playback never consumes
	// them as gameplay state.
	uint32_t timed_frames;
	uint32_t slow_frames;
};

struct replay_stage_entry_t {
	replay_start_config_t start;
	uint32_t sample_index;
	uint32_t packet_index;
	uint32_t payload_checksum;
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

struct replay_save_request_t {
	char magic[8];
	uint8_t schema;
	uint8_t source;
	uint16_t reserved;
	uint32_t replay_header_checksum;
	uint32_t checksum;
};

struct replay_save_txn_t {
	char magic[8];
	uint8_t schema;
	uint8_t state;
	uint8_t slot;
	uint8_t destination_existed;
	uint32_t temp_header_checksum;
	uint32_t checksum;
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
typedef char replay_stage_entry_size_check[
	(sizeof(replay_stage_entry_t) == REPLAY_STAGE_ENTRY_SIZE) ? 1 : -1
];
typedef char replay_command_size_check[
	(sizeof(replay_command_t) == REPLAY_COMMAND_SIZE) ? 1 : -1
];
typedef char replay_save_request_size_check[
	(sizeof(replay_save_request_t) == REPLAY_SAVE_REQUEST_SIZE) ? 1 : -1
];
typedef char replay_save_txn_size_check[
	(sizeof(replay_save_txn_t) == REPLAY_SAVE_TXN_SIZE) ? 1 : -1
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
typedef char replay_user_header_timed_frames_offset_check[
	(offsetof(replay_user_header_t, timed_frames) == 0xB8) ? 1 : -1
];
typedef char replay_user_header_slow_frames_offset_check[
	(offsetof(replay_user_header_t, slow_frames) == 0xBC) ? 1 : -1
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
