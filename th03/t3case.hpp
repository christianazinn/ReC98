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
//   offset 0x80  optional V11 runtime snapshot (935 bytes)
//   payload      record_count * 20 bytes

#include "platform.h"

#define T3CASE_VERSION        1
#define T3CASE_HEADER_SIZE    64
#define T3CASE_STARTUP_SIZE   64
#define T3CASE_PREFIX_SIZE    (T3CASE_HEADER_SIZE + T3CASE_STARTUP_SIZE)
#define T3CASE_SNAPSHOT_SIZE  935
#define T3CASE_RECORD_SIZE    20

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
// before both `player_update()` calls. A normalized V11 case carries mapped
// pre-Autofire input plus Charge so playback can derive that same boundary.
// Any change to the boundary requires a new semantics number.
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

// FNV-1a/32. Chosen over CRC32 because it needs no lookup table in a
// constrained DOS build and is trivial to reproduce on the host.
#define T3CASE_FNV1A_BASIS 0x811C9DC5UL
#define T3CASE_FNV1A_PRIME 0x01000193UL

// [flags] bits. Bit 0 exists because a normalized V11 case has no TH03 frame
// counters to carry; bit 1 because Autofire is a Replay Patch concept that TH03
// itself has no state for, so a normalized case must hand the Charge mask over
// and let playback finish the transform.
#define T3CASE_FLAG_ADVISORY_POSITIONS 0x0001
#define T3CASE_FLAG_CHARGE_IN_CONTROL  0x0002
#define T3CASE_FLAG_SNAPSHOT           0x0008
#define T3CASE_KNOWN_FLAGS ( \
	T3CASE_FLAG_ADVISORY_POSITIONS | \
	T3CASE_FLAG_CHARGE_IN_CONTROL | \
	T3CASE_FLAG_SNAPSHOT \
)

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
	uint32_t header_checksum; // header + startup + optional snapshot
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
	uint8_t autofire; // Replay Patch Autofire mask; 0 on a direct recording
	uint8_t reserved[12];
};

// Audited phase-2 runtime state for a normalized V11 case. TH03 first executes
// its ordinary round_startup() and sub_9EBF() path, then playback overwrites
// exactly the fields restored by Replay Patch V11. No pointers, padding, or
// unrelated gameplay BSS are present.
struct t3case_snapshot_t {
	int32_t random_seed;
	uint8_t randring[256];
	uint8_t randring_p;
	uint8_t formation_type_ring[256];
	uint8_t formation_pos_type_ring[256];
	uint8_t formation_p[T3CASE_PLAYER_COUNT];
	uint8_t cpu_charge_at_avail_ring[T3CASE_PLAYER_COUNT][64];
	uint8_t cpu_charge_at_avail_ring_p[T3CASE_PLAYER_COUNT];
	uint16_t player_center_x[T3CASE_PLAYER_COUNT];
	uint16_t player_center_y[T3CASE_PLAYER_COUNT];
	uint8_t player_halfhearts[T3CASE_PLAYER_COUNT];
	uint8_t player_invincibility_time[T3CASE_PLAYER_COUNT];
	uint8_t player_gauge_charge_speed[T3CASE_PLAYER_COUNT];
	uint16_t player_gauge_charged[T3CASE_PLAYER_COUNT];
	uint16_t player_gauge_avail[T3CASE_PLAYER_COUNT];
	uint8_t player_bombs[T3CASE_PLAYER_COUNT];
	uint8_t player_shot_active[T3CASE_PLAYER_COUNT];
	uint16_t player_cpu_frame[T3CASE_PLAYER_COUNT];
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
	uint16_t control; // Charge mask for input; process code for control
};

typedef char t3case_header_size_check[
	(sizeof(t3case_header_t) == T3CASE_HEADER_SIZE) ? 1 : -1
];
typedef char t3case_startup_size_check[
	(sizeof(t3case_startup_t) == T3CASE_STARTUP_SIZE) ? 1 : -1
];
typedef char t3case_snapshot_size_check[
	(sizeof(t3case_snapshot_t) == T3CASE_SNAPSHOT_SIZE) ? 1 : -1
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
