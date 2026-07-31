#ifndef TH03_T3CASE_HPP
#define TH03_T3CASE_HPP

// T3CASE1 — internal cross-branch validation packet for TH03.
//
// This is NOT a user-facing replay format. It exists only so that the same
// logical run can be replayed on several ReC98 branches and, later, on a
// native `libth03`, and so that the resulting state traces can be compared.
// It never converts back into a user replay (V11/V12), carries no save UX,
// and embeds no original game data.
//
// Layout is fixed and fully described by `state/port/T3CASE1_FORMAT.md` in the
// rec98-harness repository. There is deliberately no chunk framework, index,
// footer, or extension mechanism: while the format is private, a missing field
// is an explicit versioned layout change, not a negotiated capability.
//
//   offset 0x00  header  (64 bytes)
//   offset 0x40  startup (64 bytes)
//   offset 0x80  payload (record_count * 20 bytes)

#include "platform.h"

#define T3CASE_VERSION        1
#define T3CASE_HEADER_SIZE    64
#define T3CASE_STARTUP_SIZE   64
#define T3CASE_RECORD_SIZE    20
#define T3CASE_PAYLOAD_OFFSET (T3CASE_HEADER_SIZE + T3CASE_STARTUP_SIZE)

#define T3CASE_STAGE_COUNT  9
#define T3CASE_PLAYER_COUNT 2
#define T3CASE_SCORE_DIGITS 8

// [source_kind]: where the logical input stream originally came from.
#define T3CASE_SOURCE_MASTER_DIRECT  1
#define T3CASE_SOURCE_NORMALIZED_V11 2
#define T3CASE_SOURCE_NORMALIZED_V12 3

// [producer]: which build wrote this file. Provenance only; playback never
// branches on it.
#define T3CASE_PRODUCER_MASTER_MOD      1
#define T3CASE_PRODUCER_ANNIVERSARY_MOD 2
#define T3CASE_PRODUCER_HOST            3

// TH03 consumes 16-bit logical `input_t` values after `input_mode()` and
// before both `player_update()` calls, with Replay Patch Autofire off. Any
// change to that boundary requires a new semantics number.
#define T3CASE_INPUT_SEMANTICS 1

#define T3CASE_RULESET_CLASSIC 1

#define T3CASE_PROCESS_MAIN  1
#define T3CASE_PROCESS_MAINL 2

#define T3CASE_RECORD_INPUT   1
#define T3CASE_RECORD_CONTROL 2

#define T3CASE_PHASE_GAMEPLAY     0
#define T3CASE_PHASE_INTERSTITIAL 1
#define T3CASE_PHASE_CONTROL      2

#define T3CASE_CONTROL_MAIN_END  1
#define T3CASE_CONTROL_MAINL_END 2

// [post_init_flags]: narrowly proven state restored *after* normal round
// initialization because the ordinary path cannot recreate it. Every bit needs
// an evidence entry in the implementation note before it may be set.
#define T3CASE_POST_INIT_RANDRING_P 0x0001

// FNV-1a/32. Chosen over CRC32 because it needs no lookup table in a
// constrained DOS build and is trivial to reproduce on the host.
#define T3CASE_FNV1A_BASIS 0x811C9DC5UL
#define T3CASE_FNV1A_PRIME 0x01000193UL

struct t3case_header_t {
	char magic[8]; // "T3CASE1\0"
	uint16_t version;
	uint16_t header_size;
	uint16_t startup_size;
	uint16_t record_size;
	uint32_t payload_offset;
	uint32_t payload_size;
	uint32_t sample_count; // records with kind == T3CASE_RECORD_INPUT
	uint32_t record_count; // all records
	uint8_t source_kind;
	uint8_t input_semantics;
	uint8_t ruleset_id;
	uint8_t scenario_id; // `resident->game_mode` at the deterministic start
	uint8_t first_process;
	uint8_t producer;
	uint16_t flags;
	uint32_t case_id;       // compact link into an ignored host manifest
	uint32_t source_digest; // digest of the source .RPY when normalized
	uint32_t source_commit; // first 4 bytes of the ReC98 commit, as ASCII
	uint32_t payload_checksum;
	uint32_t header_checksum; // over bytes 0x00..0x7F with this field zeroed
	uint32_t total_size;
};

// Audited startup description. Deliberately NOT a dump of initialized
// gameplay BSS: restoring a memory image would overwrite divergent
// initialization results and falsely declare startup logic correct.
struct t3case_startup_t {
	int32_t resident_rand; // `resident->rand`
	int32_t random_seed;   // master.lib `random_seed`
	uint8_t game_mode;
	uint8_t rank;
	uint8_t key_mode;
	uint8_t story_stage;
	uint8_t story_lives;
	uint8_t rem_credits;
	uint8_t skill;
	uint8_t demo_num;
	uint8_t is_cpu[T3CASE_PLAYER_COUNT];
	int8_t playchar_paletted[T3CASE_PLAYER_COUNT];
	int8_t story_opponents[T3CASE_STAGE_COUNT];
	int8_t pid_winner;
	uint8_t show_score_menu;
	uint8_t op_animation_fast;
	uint8_t score_last[T3CASE_PLAYER_COUNT][T3CASE_SCORE_DIGITS];
	uint16_t post_init_flags;
	uint8_t post_init_randring_p;
	uint8_t reserved[13];
};

// One fixed-size payload record. [frame_index], [round_frame], and
// [round_or_result_frame] are what make this a *verifier* rather than a mere
// input tape: playback rejects the case the moment a recorded position stops
// agreeing with the live one.
struct t3case_record_t {
	uint8_t kind;
	uint8_t phase;
	uint16_t round_or_result_frame;
	uint32_t frame_index;
	uint32_t round_frame;
	uint16_t input_mp_p1;
	uint16_t input_mp_p2;
	uint16_t input_sp;
	uint16_t control; // 0 unless kind == T3CASE_RECORD_CONTROL
};

typedef char t3case_header_size_check[
	(sizeof(t3case_header_t) == T3CASE_HEADER_SIZE) ? 1 : -1
];
typedef char t3case_startup_size_check[
	(sizeof(t3case_startup_t) == T3CASE_STARTUP_SIZE) ? 1 : -1
];
typedef char t3case_record_size_check[
	(sizeof(t3case_record_t) == T3CASE_RECORD_SIZE) ? 1 : -1
];

// Canonical validation output. Byte-compatible with the `T3SPLT1` stream that
// the Replay Patch already emits, so `tools/replay/export_splits.ps1` and
// `tools/replay/compare_splits.ps1` work unchanged against these branches.
#define T3CASE_SPLIT_VERSION     1
#define T3CASE_SPLIT_HEADER_SIZE 16
#define T3CASE_SPLIT_ROW_SIZE    34
#define T3CASE_SPLIT_PACKED_SCORE_SIZE 4

// Default certification cadence, not a truth condition. A debug run may
// shorten this to localize a divergence window.
#define T3CASE_SPLIT_INTERVAL_SAMPLES 128

enum t3case_split_event_t {
	T3CASE_EVENT_START       = 1,
	T3CASE_EVENT_ROUND_START = 2,
	T3CASE_EVENT_INPUT_END   = 3,
	T3CASE_EVENT_ERROR       = 4,
	T3CASE_EVENT_CHECKPOINT  = 5,
	T3CASE_EVENT_FINISH      = 6,
	T3CASE_EVENT_ROUTE       = 7,
};

struct t3case_split_header_t {
	char magic[8]; // "T3SPLT1"
	uint16_t version;
	uint16_t header_size;
	uint16_t row_size;
	uint16_t flags;
};

struct t3case_split_row_t {
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
	uint8_t score_p1[T3CASE_SPLIT_PACKED_SCORE_SIZE];
	uint8_t score_p2[T3CASE_SPLIT_PACKED_SCORE_SIZE];
	int32_t resident_rand;
	uint32_t state_hash;
};

typedef char t3case_split_header_size_check[
	(sizeof(t3case_split_header_t) == T3CASE_SPLIT_HEADER_SIZE) ? 1 : -1
];
typedef char t3case_split_row_size_check[
	(sizeof(t3case_split_row_t) == T3CASE_SPLIT_ROW_SIZE) ? 1 : -1
];

// Version of the curated state-hash field inventory. Bump on every expansion;
// the inventory itself is recorded in `state/notes/th03-t3case-verifier.md`.
#define T3CASE_STATE_HASH_SCHEMA 1

#endif /* TH03_T3CASE_HPP */
