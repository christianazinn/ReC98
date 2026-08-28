#ifndef TH02_REPLAY_FORMAT_HPP
#define TH02_REPLAY_FORMAT_HPP

#include <stddef.h>
#include "platform.h"

#ifndef T2REPLAY_EXACT_APPLY
#define T2REPLAY_EXACT_APPLY 0
#endif
#ifndef T2REPLAY_EXACT_TRACE
#define T2REPLAY_EXACT_TRACE 0
#endif

#define T2REPLAY_VERSION 1
#define T2REPLAY_HEADER_SIZE 128
#define T2REPLAY_PACKET_SIZE 4
#define T2REPLAY_START_SIZE 36
#define T2REPLAY_COMMAND_SIZE 52
#define T2REPLAY_SAVE_REQUEST_SIZE 20
#define T2REPLAY_NAME_LEN 8
#define T2REPLAY_RESERVED_NAME_OFFSET 0
#define T2REPLAY_RESERVED_TAIL_OFFSET \
	(T2REPLAY_RESERVED_NAME_OFFSET + T2REPLAY_NAME_LEN)
#define T2REPLAY_RESERVED_TAIL_SIZE 4
#define T2REPLAY_STAGE_COUNT 6
#define T2REPLAY_SLOT_COUNT 100
// A command-only capture target. MAIN writes this to T2RPY.TMP; OP chooses the
// eventual numbered slot after a finalized terminal capture exists.
#define T2REPLAY_TEMP_SLOT T2REPLAY_SLOT_COUNT
#define T2REPLAY_INPUT_SIZE_MAX 0x00400000UL

// Private semantic-checkpoint vocabulary for later exact TH02 seeks. This
// does not change the T2RPY1 user replay payload yet. Groups append by ID;
// a reader only accepts the complete vocabulary it implements.
#define T2REPLAY_CHECKPOINT_SCHEMA 4
#define T2REPLAY_CHECKPOINT_GROUP_SCHEMA 2
#define T2REPLAY_CHECKPOINT_HEADER_SIZE 40
#define T2REPLAY_CHECKPOINT_GROUP_SIZE 20
#define T2REPLAY_CHECKPOINT_GROUP_COUNT 12
#define T2REPLAY_CHECKPOINT_IDENTITY_SIZE 12
#define T2REPLAY_CHECKPOINT_RNG_SIZE 264
#define T2REPLAY_CHECKPOINT_RUN_SIZE 46
#define T2REPLAY_CHECKPOINT_FIELD_SIZE 20
#define T2REPLAY_CHECKPOINT_STAGE_VM_SIZE 8
#define T2REPLAY_CHECKPOINT_PACING_SIZE 12
#define T2REPLAY_CHECKPOINT_PLAYER_SIZE 700
#define T2REPLAY_CHECKPOINT_BOMB_SIZE 281
#define T2REPLAY_CHECKPOINT_BULLET_SIZE 2860
#define T2REPLAY_CHECKPOINT_LASER_SIZE 147
#define T2REPLAY_CHECKPOINT_ENEMY_SIZE 904
#define T2REPLAY_CHECKPOINT_EFFECT_SIZE 1696
#define T2REPLAY_CHECKPOINT_CAPTURE_SIZE 7230
#define T2REPLAY_CHECKPOINT_GROUP_MASK 0x00000FFFUL
#define T2REPLAY_CHECKPOINT_SOURCE_FINGERPRINT 0xC5E6F39DUL

// Private exact-restore envelope. This is intentionally distinct from the
// schema-4 capture-only common-world container above: the exact decoder must
// never accept a partly extended schema-4 record as an exact checkpoint.
// Schema 1 has a complete zero-payload directory and is validation-only.
#define T2REPLAY_EXACT_CHECKPOINT_SCHEMA 1
#define T2REPLAY_EXACT_GROUP_SCHEMA 1
#define T2REPLAY_EXACT_HEADER_SIZE 48
#define T2REPLAY_EXACT_GROUP_SIZE 20
#define T2REPLAY_EXACT_GROUP_COUNT 19
#define T2REPLAY_EXACT_CHECKPOINT_GROUP_MASK 0x0007FFFFUL
#define T2REPLAY_EXACT_CHECKPOINT_SIZE (T2REPLAY_EXACT_HEADER_SIZE + (T2REPLAY_EXACT_GROUP_COUNT * T2REPLAY_EXACT_GROUP_SIZE))
#define T2REPLAY_EXACT_CHECKPOINT_SOURCE_FINGERPRINT 0xE2C5A401UL
#define T2REPLAY_EXACT_BOUNDARY_GENERATION 1

// Schema 2 remains a private, capture-only T2XCK1 form. It registers the
// canonical schema-4 common payloads together with the pointer-free Stage 5
// Mima and generic actor owners. The remaining exact groups are explicitly
// deferred, so no reader can mistake this partial record for a seekable save.
#define T2REPLAY_EXACT_S5_MIMA_SCHEMA 2
#define T2REPLAY_EXACT_S5_MIMA_SOURCE_FINGERPRINT 0x50F9D052UL
#define T2REPLAY_EXACT_GROUP_FLAG_DEFERRED 0x01
#define T2REPLAY_EXACT_ACTOR_CORE_SIZE 23
#define T2REPLAY_EXACT_S5_MIMA_SIZE 164
#define T2REPLAY_EXACT_S5_MIMA_CAPTURE_SIZE ( \
	T2REPLAY_EXACT_CHECKPOINT_SIZE + \
	(T2REPLAY_CHECKPOINT_CAPTURE_SIZE - \
	 T2REPLAY_CHECKPOINT_HEADER_SIZE - \
	 (T2REPLAY_CHECKPOINT_GROUP_COUNT * T2REPLAY_CHECKPOINT_GROUP_SIZE)) + \
	T2REPLAY_EXACT_ACTOR_CORE_SIZE + \
	T2REPLAY_EXACT_S5_MIMA_SIZE \
)

// Schema 3 closes only Stage 5 Mima's logical TILE_LOGIC capture group. Its
// Stage FX, palette, callback, and redraw groups remain deferred, so it is
// still a private capture-only form rather than a seekable exact restore.
#define T2REPLAY_EXACT_S5_MIMA_TILE_SCHEMA 3
#define T2REPLAY_EXACT_S5_MIMA_TILE_SOURCE_FINGERPRINT 0xA11357C4UL
#define T2REPLAY_EXACT_S5_TILE_LOGIC_SIZE 685
#define T2REPLAY_EXACT_S5_MIMA_TILE_CAPTURE_SIZE ( \
	T2REPLAY_EXACT_S5_MIMA_CAPTURE_SIZE + \
	T2REPLAY_EXACT_S5_TILE_LOGIC_SIZE \
)

// Schema 4 closes only Stage 5 Mima's STAGE_FX group. It records the native
// quiescent generic background-particle contract; Mima's visible background
// state remains owned by ACTOR_STAGE. Palette, callback, and redraw groups
// remain deferred, so this is still a private capture-only form.
#define T2REPLAY_EXACT_S5MFX_SCHEMA 4
#define T2REPLAY_EXACT_S5MFX_SOURCE_FINGERPRINT 0xC0589116UL
#define T2REPLAY_EXACT_S5MFX_SIZE 11
#define T2REPLAY_EXACT_S5MFX_CAPTURE_SIZE ( \
	T2REPLAY_EXACT_S5_MIMA_TILE_CAPTURE_SIZE + \
	T2REPLAY_EXACT_S5MFX_SIZE \
)

// Schema 5 closes Stage 5 Mima's PALETTE group with only her mutable color-0
// value and semantic tone. Immutable stage colors and hardware registers are
// deliberately omitted. Callbacks and redraw remain deferred.
#define T2REPLAY_EXACT_S5PAL_SCHEMA 5
#define T2REPLAY_EXACT_S5PAL_SOURCE_FINGERPRINT 0x1190BDC0UL
#define T2REPLAY_EXACT_S5_PALETTE_SIZE 4
#define T2REPLAY_EXACT_S5PAL_CAPTURE_SIZE ( \
	T2REPLAY_EXACT_S5MFX_CAPTURE_SIZE + \
	T2REPLAY_EXACT_S5_PALETTE_SIZE \
)

// Schema 6 closes the final two capture groups with semantic recipe IDs.
// CALLBACKS names the complete Stage 5 Mima callback table; REDRAW names the
// native one-frame reveal transaction. Neither group stores code pointers,
// VRAM, hardware registers, or compiler-layout state. Apply remains deferred.
#define T2REPLAY_EXACT_S5CBRD_SCHEMA 6
#define T2REPLAY_EXACT_S5CBRD_SOURCE_FINGERPRINT 0x4EBF53CEUL
#define T2REPLAY_EXACT_S5_CALLBACK_SIZE 1
#define T2REPLAY_EXACT_S5_REDRAW_SIZE 1
#define T2REPLAY_EXACT_S5CBRD_CAPTURE_SIZE ( \
	T2REPLAY_EXACT_S5PAL_CAPTURE_SIZE + \
	T2REPLAY_EXACT_S5_CALLBACK_SIZE + \
	T2REPLAY_EXACT_S5_REDRAW_SIZE \
)

// Private direct-apply request. This binds a schema-6 envelope to one
// finalized T2RPY1 stream position without changing either public format.
#define T2REPLAY_EXACT_APPLY_REQUEST_VERSION 1
#define T2REPLAY_EXACT_APPLY_REQUEST_SIZE 48
#define T2REPLAY_EXACT_APPLY_FILE_SIZE ( \
	T2REPLAY_EXACT_APPLY_REQUEST_SIZE + \
	T2REPLAY_EXACT_S5CBRD_CAPTURE_SIZE \
)

// The public seek wire remains separate from T2RPY1 and T2RCFG2. The native
// reader is compiled only into the private exact-apply profile until its
// fresh-process equivalence matrix is complete.
#define T2REPLAY_PUBLIC_SEEK_VERSION 1
#define T2REPLAY_PUBLIC_SEEK_HEADER_SIZE 64
#define T2REPLAY_PUBLIC_SEEK_ENTRY_SIZE 48
#define T2REPLAY_PUBLIC_SEEK_REQUEST_SIZE 48
#define T2REPLAY_PUBLIC_SEEK_FORMAT_FINGERPRINT 0x3A7D2C61UL
#define T2REPLAY_PUBLIC_SEEK_TARGET_STAGE 1
#define T2REPLAY_PUBLIC_SEEK_TARGET_CHAPTER 2
#define T2REPLAY_PUBLIC_SEEK_TARGET_MIDBOSS 3
#define T2REPLAY_PUBLIC_SEEK_TARGET_BOSS 4

#define T2REPLAY_FLAG_RLE_INPUT 0x0001
#define T2REPLAY_FLAG_FULL_INPUT 0x0002
#define T2REPLAY_FLAG_PAUSE_RESTART 0x0004
#define T2REPLAY_FLAG_PRACTICE 0x0008
#define T2REPLAY_REQUIRED_FLAGS \
	(T2REPLAY_FLAG_RLE_INPUT | T2REPLAY_FLAG_FULL_INPUT)
#define T2REPLAY_DEFAULT_FLAGS \
	(T2REPLAY_REQUIRED_FLAGS | T2REPLAY_FLAG_PAUSE_RESTART)
#define T2REPLAY_KNOWN_FLAGS \
	(T2REPLAY_DEFAULT_FLAGS | T2REPLAY_FLAG_PRACTICE)

#define T2REPLAY_STATUS_RECORDING 1
#define T2REPLAY_STATUS_FINALIZED 2
#define T2REPLAY_STATUS_ERROR 3

#define T2REPLAY_COMMAND_RECORD 1
#define T2REPLAY_COMMAND_PLAYBACK 2
#define T2REPLAY_COMMAND_PRACTICE 3
#define T2REPLAY_COMMAND_RESTART 4
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
#define T2REPLAY_END_MENU_RETURN 3

#define T2REPLAY_SAVE_REQUEST_SCHEMA 1
#define T2REPLAY_SAVE_REQUEST_GAME_OVER T2REPLAY_END_GAME_OVER
#define T2REPLAY_SAVE_REQUEST_CLEAR T2REPLAY_END_CLEAR
#define T2REPLAY_SAVE_REQUEST_MENU_RETURN T2REPLAY_END_MENU_RETURN

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

// These values are persistent exact-checkpoint wire IDs. In particular, they
// are never function, data, allocator, or graphics-memory addresses.
enum t2replay_exact_group_id_t {
	T2RXGI_IDENTITY = T2RCGI_IDENTITY,
	T2RXGI_RNG = T2RCGI_RNG,
	T2RXGI_RUN = T2RCGI_RUN,
	T2RXGI_FIELD = T2RCGI_FIELD,
	T2RXGI_STAGE_VM = T2RCGI_STAGE_VM,
	T2RXGI_PACING = T2RCGI_PACING,
	T2RXGI_PLAYER = T2RCGI_PLAYER,
	T2RXGI_BOMB = T2RCGI_BOMB,
	T2RXGI_BULLET = T2RCGI_BULLET,
	T2RXGI_LASER = T2RCGI_LASER,
	T2RXGI_ENEMY = T2RCGI_ENEMY,
	T2RXGI_EFFECT = T2RCGI_EFFECT,
	T2RXGI_ACTOR_CORE = 12,
	T2RXGI_ACTOR_STAGE = 13,
	T2RXGI_STAGE_FX = 14,
	T2RXGI_TILE_LOGIC = 15,
	T2RXGI_PALETTE = 16,
	T2RXGI_CALLBACKS = 17,
	T2RXGI_REDRAW = 18,
};

enum t2replay_exact_actor_tag_t {
	T2REAT_NONE = 0,
	T2REAT_S1_MIDBOSS = 1,
	T2REAT_S1_RIKA = 2,
	T2REAT_S2_MIDBOSS = 3,
	T2REAT_S2_MEIRA = 4,
	T2REAT_S3_MIDBOSS = 5,
	T2REAT_S3_STONES = 6,
	T2REAT_S4_MIDBOSS = 7,
	T2REAT_S4_MARISA = 8,
	T2REAT_S5_MIMA = 9,
	T2REAT_EX_MIDBOSS = 10,
	T2REAT_EX_SIGMA = 11,
};

enum t2replay_exact_actor_mode_t {
	T2REAM_SCROLL = 0,
	T2REAM_ACTIVE = 1,
	T2REAM_DEFEAT = 2,
	T2REAM_TRANSITION = 3,
};

enum t2replay_exact_stage_fx_tag_t {
	T2RESFT_NONE = 0,
	T2RESFT_S1_SCENERY = 1,
	T2RESFT_S2_SCENERY = 2,
	T2RESFT_S3_RING = 3,
	T2RESFT_S4_MARISA_FIELD = 4,
	T2RESFT_S5_MIMA_FIELD = 5,
	T2RESFT_EX_SIGMA_FIELD = 6,
};

// A profile names one validated set of stage callback slots. Individual
// pointer rebinding is deliberately deferred to the common-apply parcel.
enum t2replay_exact_callback_profile_t {
	T2RECP_S1_SCROLL = 0,
	T2RECP_S1_MIDBOSS = 1,
	T2RECP_S1_RIKA = 2,
	T2RECP_S2_SCROLL = 3,
	T2RECP_S2_MIDBOSS = 4,
	T2RECP_S2_MEIRA = 5,
	T2RECP_S3_SCROLL = 6,
	T2RECP_S3_MIDBOSS = 7,
	T2RECP_S3_STONES = 8,
	T2RECP_S4_SCROLL = 9,
	T2RECP_S4_MIDBOSS = 10,
	T2RECP_S4_MARISA = 11,
	T2RECP_S5_SCROLL = 12,
	T2RECP_S5_MIMA = 13,
	T2RECP_EX_SCROLL = 14,
	T2RECP_EX_MIDBOSS = 15,
	T2RECP_EX_SIGMA = 16,
};

enum t2replay_exact_redraw_recipe_t {
	T2RERR_NATIVE_ONE_FRAME_REVEAL = 1,
};

enum t2replay_exact_resource_id_t {
	T2RERI_STAGE_1 = 0,
	T2RERI_STAGE_2 = 1,
	T2RERI_STAGE_3 = 2,
	T2RERI_STAGE_4 = 3,
	T2RERI_STAGE_5 = 4,
	T2RERI_EXTRA = 5,
};

// Stable clean-Practice recipe IDs stored in t2replay_start_t::reserved[0].
// Stage Start remains 0 so native Story recordings retain an all-zero tail.
enum t2replay_practice_target_t {
	T2RPT_STAGE_START = 0,
	T2RPT_STAGE1_CHAPTER2 = 1,
	T2RPT_STAGE2_CHAPTER2 = 2,
	T2RPT_STAGE3_CHAPTER2 = 3,
	T2RPT_STAGE4_CHAPTER2 = 4,
	T2RPT_STAGE4_CHAPTER3 = 5,
	T2RPT_EXTRA_CHAPTER2 = 6,
	T2RPT_STAGE1_MIDBOSS = 7,
	T2RPT_STAGE1_BOSS_PHASE1 = 8,
	T2RPT_STAGE1_BOSS_PHASE2 = 9,
	T2RPT_STAGE1_BOSS_PHASE3 = 10,
	T2RPT_STAGE2_MIDBOSS = 11,
	T2RPT_STAGE2_BOSS_PHASE1 = 12,
	T2RPT_STAGE2_BOSS_PHASE2 = 13,
	T2RPT_STAGE2_BOSS_PHASE3 = 14,
	T2RPT_STAGE3_MIDBOSS = 15,
	T2RPT_STAGE3_BOSS_START = 16,
	T2RPT_STAGE4_MIDBOSS_FIRST = 17,
	T2RPT_STAGE4_MIDBOSS_SECOND = 18,
	T2RPT_STAGE4_BOSS_START = 19,
	T2RPT_STAGE5_BOSS_START = 20,
	T2RPT_EXTRA_MIDBOSS = 21,
	T2RPT_EXTRA_BOSS_START = 22,
	T2RPT_STAGE3_INNER_PAIR = 23,
	T2RPT_STAGE3_OUTER_PAIR = 24,
	T2RPT_STAGE4_BOSS_PHASE1 = 25,
	T2RPT_STAGE5_BOSS_PHASE1 = 26,
	T2RPT_EXTRA_BOSS_PHASE1 = 27,
	T2RPT_STAGE4_BOSS_ROUND2 = 28,
	T2RPT_STAGE5_BOSS_PHASE3 = 29,
	T2RPT_EXTRA_BOSS_PHASE3 = 30,
	T2RPT_STAGE4_BOSS_ROUND3 = 31,
	T2RPT_STAGE5_BOSS_PHASE5 = 32,
	T2RPT_EXTRA_BOSS_PHASE5 = 33,
};

#define T2REPLAY_PRACTICE_TARGET_OFFSET 0
#define T2REPLAY_PRACTICE_RESERVED_OFFSET 1
#define T2REPLAY_PRACTICE_RESERVED_SIZE 4

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

// The pending-save handoff is deliberately separate from T2RPY.TMP: this
// compact checksummed request binds OP admission to one finalized header.
struct t2replay_save_request_t {
	char magic[8];
	uint8_t schema;
	uint8_t source;
	uint16_t reserved;
	uint32_t replay_header_checksum;
	uint32_t checksum;
};

struct t2replay_exact_apply_request_t {
	char magic[8];
	uint16_t version;
	uint16_t header_size;
	uint32_t total_size;
	uint32_t envelope_size;
	uint32_t replay_header_checksum;
	uint32_t packet_anchor;
	uint32_t sample_anchor;
	uint8_t slot;
	uint8_t stage_id;
	uint8_t phase;
	uint8_t run_offset;
	uint32_t prefix_checksum;
	uint32_t request_checksum;
	uint32_t reserved;
};

struct t2replay_public_seek_header_t {
	char magic[8];
	uint16_t version;
	uint16_t header_size;
	uint16_t entry_size;
	uint16_t entry_count;
	uint32_t total_size;
	uint32_t replay_header_checksum;
	uint32_t replay_payload_checksum;
	uint32_t format_fingerprint;
	uint32_t replay_sample_count;
	uint32_t replay_packet_count;
	uint32_t directory_checksum;
	uint32_t sidecar_checksum;
	uint8_t reserved[16];
};

struct t2replay_public_seek_entry_t {
	uint8_t stage_id;
	uint8_t target_kind;
	uint8_t actor_tag;
	uint8_t actor_mode;
	uint8_t stage_fx_tag;
	uint8_t callback_profile;
	uint8_t redraw_recipe;
	uint8_t capture_generation;
	uint16_t checkpoint_schema;
	uint8_t group_count;
	uint8_t reserved_0;
	uint32_t sample_anchor;
	uint32_t packet_anchor;
	uint32_t prefix_checksum;
	uint32_t checkpoint_offset;
	uint32_t checkpoint_size;
	uint32_t checkpoint_checksum;
	uint32_t semantic_digest;
	uint32_t source_fingerprint;
	uint32_t reserved_1;
};

struct t2replay_public_seek_request_t {
	char magic[8];
	uint16_t version;
	uint16_t header_size;
	uint8_t slot;
	uint8_t reserved_0;
	uint16_t entry_index;
	uint32_t replay_header_checksum;
	uint32_t replay_payload_checksum;
	uint32_t sidecar_checksum;
	uint32_t request_checksum;
	uint8_t reserved[16];
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
typedef char t2replay_save_request_size_check[
	(sizeof(t2replay_save_request_t) == T2REPLAY_SAVE_REQUEST_SIZE) ? 1 : -1
];
typedef char t2replay_exact_apply_request_size_check[
	(sizeof(t2replay_exact_apply_request_t) ==
	 T2REPLAY_EXACT_APPLY_REQUEST_SIZE) ? 1 : -1
];
typedef char t2replay_public_seek_header_size_check[
	(sizeof(t2replay_public_seek_header_t) ==
	 T2REPLAY_PUBLIC_SEEK_HEADER_SIZE) ? 1 : -1
];
typedef char t2replay_public_seek_entry_size_check[
	(sizeof(t2replay_public_seek_entry_t) ==
	 T2REPLAY_PUBLIC_SEEK_ENTRY_SIZE) ? 1 : -1
];
typedef char t2replay_public_seek_request_size_check[
	(sizeof(t2replay_public_seek_request_t) ==
	 T2REPLAY_PUBLIC_SEEK_REQUEST_SIZE) ? 1 : -1
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
typedef char t2rec_envelope_size_check[
	(T2REPLAY_EXACT_CHECKPOINT_SIZE == 428) ? 1 : -1
];
typedef char t2rec_s5_mima_capture_size_check[
	(T2REPLAY_EXACT_S5_MIMA_CAPTURE_SIZE == 7565) ? 1 : -1
];
typedef char t2rec_s5_mima_stage_fx_capture_size_check[
	(T2REPLAY_EXACT_S5MFX_CAPTURE_SIZE == 8261) ? 1 : -1
];
typedef char t2rec_s5_mima_palette_capture_size_check[
	(T2REPLAY_EXACT_S5PAL_CAPTURE_SIZE == 8265) ? 1 : -1
];
typedef char t2rec_s5_mima_callback_redraw_capture_size_check[
	(T2REPLAY_EXACT_S5CBRD_CAPTURE_SIZE == 8267) ? 1 : -1
];
typedef char t2replay_header_checksum_offset_check[
	(offsetof(t2replay_header_t, header_checksum) == 0x2C) ? 1 : -1
];
typedef char t2replay_header_start_offset_check[
	(offsetof(t2replay_header_t, start) == 0x30) ? 1 : -1
];
typedef char t2replay_header_name_size_check[
	(sizeof(((t2replay_header_t *)0)->reserved) ==
	 (T2REPLAY_NAME_LEN + T2REPLAY_RESERVED_TAIL_SIZE)) ? 1 : -1
];
typedef char t2replay_header_name_offset_check[
	((offsetof(t2replay_header_t, reserved) +
	  T2REPLAY_RESERVED_NAME_OFFSET) == 0x74) ? 1 : -1
];
#endif /* TH02_REPLAY_FORMAT_HPP */
