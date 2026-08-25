#ifndef TH04_MAIN_ORACLE_HPP
#define TH04_MAIN_ORACLE_HPP

// T4CASE1 / T5CASE1 — internal cross-branch validation packet for TH04/TH05.
//
// This is NOT a user-facing replay format. It exists only so that the same
// logical run can be replayed on several ReC98 branches and, later, on a
// native port, and so that the resulting state traces can be compared. It
// never converts back into any user format and embeds no original game data.
//
// The layout is fixed and fully described by `state/port/TXCASE_CONTRACT.md`
// and `state/port/TXSPLIT_CONTRACT.md` in the rec98-harness repository. There
// is deliberately no chunk framework, index, footer, or extension mechanism:
// while the format is private, a missing field is an explicit versioned layout
// change, not a negotiated capability.
//
//   offset 0x00                  header  (64 bytes)
//   offset 0x40                  startup (72 bytes TH04 / 128 bytes TH05)
//   offset 64 + startup_size     payload, record_count * 12 bytes
//
// TH04 and TH05 share this implementation the same way they already share
// `th04/main/demo.cpp`, but they emphatically do NOT share a startup block:
// the TH04 and TH05 resident structures are different layouts with different
// field sets.
// A shared name does not imply a shared body.

#include "platform.h"

#define ORACLE_VERSION      1
#define ORACLE_HEADER_SIZE  64
#define ORACLE_RECORD_SIZE  12

#if (GAME == 5)
	#define ORACLE_STARTUP_SIZE 128
	#define ORACLE_MAGIC_DIGIT  '5'
#else
	#define ORACLE_STARTUP_SIZE 72
	#define ORACLE_MAGIC_DIGIT  '4'
#endif

#define ORACLE_PREFIX_SIZE  (ORACLE_HEADER_SIZE + ORACLE_STARTUP_SIZE)

#define ORACLE_SCORE_DIGITS 8
#define ORACLE_RANDRING_SIZE 256

// [source_kind]: where the logical input stream originally came from. There is
// deliberately no value 3: the TH03 format reserves it for a snapshot-bearing
// normalized case, and TxCASE has no snapshot concept at all.
#define ORACLE_SOURCE_DIRECT     1
#define ORACLE_SOURCE_NORMALIZED 2

// [producer]: which build wrote this file. Provenance only; playback never
// branches on it.
#define ORACLE_PRODUCER_GAME_MOD    1
#define ORACLE_PRODUCER_SECOND_MOD  2
#define ORACLE_PRODUCER_HOST        3

// TH04/TH05 consume one logical sample per gameplay frame at exactly one
// boundary: the indirect call after `input_sense()` in the frame loop
// (`th04_main.asm:352-353`, `th05_main.asm:388-389`). A sample is the 8-bit
// `input_replay_t` subset of `key_det` plus the `shiftkey` boolean, which is
// precisely what ZUN's own `DEMO?.REC` stores. Any change to that boundary or
// to the stored width requires new semantics.
#define ORACLE_INPUT_SEMANTICS 1

#define ORACLE_RULESET_CLASSIC 1

#define ORACLE_PROCESS_MAIN  1
#define ORACLE_PROCESS_MAINE 2
#define ORACLE_PROCESS_OP    3

#define ORACLE_RECORD_INPUT   1
#define ORACLE_RECORD_CONTROL 2

#define ORACLE_PHASE_GAMEPLAY     0
#define ORACLE_PHASE_INTERSTITIAL 1
#define ORACLE_PHASE_CONTROL      2

#define ORACLE_CONTROL_PROCESS_END  1
#define ORACLE_CONTROL_CURSOR_RESET 2
#define ORACLE_CONTROL_STAGE        3
#define ORACLE_CONTROL_TERMINAL     4

// [flags] bits.
#define ORACLE_FLAG_ADVISORY_POSITIONS 0x0001
#define ORACLE_FLAG_SOURCE_CLIPPED     0x0002
#define ORACLE_FLAG_SPLICED_SOURCE     0x0004
#define ORACLE_KNOWN_FLAGS ( \
	ORACLE_FLAG_ADVISORY_POSITIONS | \
	ORACLE_FLAG_SOURCE_CLIPPED | \
	ORACLE_FLAG_SPLICED_SOURCE \
)

// FNV-1a/32. Chosen over CRC32 because it needs no lookup table in a
// constrained DOS build and is trivial to reproduce on the host.
#define ORACLE_FNV1A_BASIS_A 0x811C9DC5UL
#define ORACLE_FNV1A_BASIS_B 0x7EE3623AUL
#define ORACLE_FNV1A_PRIME   0x01000193UL

struct oracle_header_t {
	char magic[8]; // "T4CASE1\0" / "T5CASE1\0"
	uint16_t version;
	uint16_t header_size;
	uint16_t startup_size;
	uint16_t record_size;
	uint32_t payload_offset;
	uint32_t payload_size;
	uint32_t sample_count; // records with kind == ORACLE_RECORD_INPUT
	uint32_t record_count; // all records
	uint8_t source_kind;
	uint8_t input_semantics;
	uint8_t ruleset_id;
	uint8_t scenario_id; // 0 normal start, 1-4 demo, 5 TH05 Extra
	uint8_t first_process;
	uint8_t producer;
	uint16_t flags;
	uint32_t case_id;       // compact link into an ignored host manifest
	uint32_t source_digest; // digest of the source DEMO?.REC when normalized
	uint32_t source_commit; // first 4 bytes of the ReC98 commit, as ASCII
	uint32_t payload_checksum;
	uint32_t header_checksum; // header prefix + startup block
	uint32_t total_size;
};

// Audited startup description. Deliberately NOT a memcpy of `resident_t`: the
// widths below are the format's, not the game's, and the writer converts.
//
// There is no runtime snapshot and no SNAPSHOT flag. TH04/TH05 have ZUN's own
// demo system, so the scenario start is pinned by the game's own code
// (`th04_main.asm:689-698`, `th05_main.asm:769-780`) and the correct post-init
// behavior is to compare and fail, never to overwrite.
#if (GAME == 5)
struct oracle_startup_t {
	int32_t random_seed;   // master.lib `random_seed`, 318 on the demo path
	int32_t resident_rand; // `resident->rand`
	uint32_t slow_frames;
	uint32_t frames;
	uint8_t score_last[ORACLE_SCORE_DIGITS];
	uint8_t score_highest[ORACLE_SCORE_DIGITS];
	uint16_t std_frames;
	uint16_t items_spawned;
	uint16_t items_collected;
	uint16_t point_items_collected;
	uint16_t max_valued_point_items_collected;
	uint16_t enemies_gone;
	uint16_t enemies_killed;
	uint16_t graze;
	uint8_t credit_lives;
	uint8_t credit_bombs;
	uint8_t cfg_lives;
	uint8_t cfg_bombs;
	// `rank` is a required recorded field, not a constant: neither
	// `th05/op/start.cpp:57-107` nor `th04/op/start.cpp:53-86` writes
	// `resident->rank`, yet `th04/main/ems.cpp:64-69` reads it, so demo
	// playback is difficulty-dependent.
	uint8_t rank;
	uint8_t bgm_mode;
	uint8_t se_mode;
	uint8_t stage;
	uint8_t playchar;
	uint8_t turbo_mode;
	// `debug` is named `debug_mode` in ASM and it is live: TH05 overrides
	// `resident->stage` from `debug_stage` and `_power` from `debug_power`
	// before the demo gate (`th05_main.asm:754-763`).
	uint8_t debug;
	uint8_t debug_stage;
	uint8_t debug_power;
	uint8_t end_sequence;
	uint8_t miss_count;
	uint8_t bombs_used;
	uint8_t demo_stage;
	uint8_t demo_num;
	uint8_t zunsoft_shown;
	uint8_t unknown;
	uint8_t stage_id; // MAIN-local, not resident
	uint8_t power;    // MAIN-local, not resident
	uint8_t playperf; // MAIN-local, not resident
	// One byte, and it earns its place. `ems_allocate_and_preload_eyecatch()`
	// sets `Ems = nullptr` when EMS is absent or too small
	// (`th04/main/ems.cpp:75-78`) and eight sites branch on it. An oracle that
	// silently depends on the host's EMS configuration is not an oracle.
	uint8_t ems_present;
	uint8_t stage_score[6][ORACLE_SCORE_DIGITS];
	uint8_t reserved[8];
};
#else
struct oracle_startup_t {
	int32_t random_seed;   // master.lib `random_seed`, 318 on the demo path
	int32_t resident_rand; // `resident->rand`
	uint32_t slow_frames;
	uint32_t frames;
	uint8_t score_last[ORACLE_SCORE_DIGITS];
	uint16_t std_frames;
	uint16_t items_spawned;
	uint16_t items_collected;
	uint16_t point_items_collected;
	uint16_t max_valued_point_items_collected;
	uint16_t enemies_gone;
	uint16_t enemies_killed;
	uint16_t graze;
	uint8_t rem_lives;
	uint8_t credit_lives;
	uint8_t rem_bombs;
	uint8_t credit_bombs;
	uint8_t rank; // see the TH05 comment above; identical reasoning
	uint8_t bgm_mode;
	uint8_t se_mode;
	uint8_t stage;
	uint8_t playchar_ascii;
	uint8_t stage_ascii;
	uint8_t shottype;
	uint8_t end_type_ascii;
	uint8_t end_sequence;
	uint8_t miss_count;
	uint8_t bombs_used;
	uint8_t cfg_lives;
	uint8_t cfg_bombs;
	uint8_t demo_stage;
	uint8_t demo_num;
	uint8_t debug;
	uint8_t turbo_mode;
	uint8_t zunsoft_shown;
	uint8_t stage_id; // MAIN-local, not resident
	uint8_t power;    // MAIN-local, not resident
	uint8_t playperf; // MAIN-local, not resident
	uint8_t ems_present;
	uint8_t reserved[6];
};
#endif

// One fixed-size payload record.
//
// [frame_index] is dense, monotonic and never reset across the whole case.
// [scenario_cursor] is the game's own `stage_frame`, which TH05 resets mid-run
// during the Extra splice (`th05/main/dialog/dialog.cpp:271`). They are two
// different numbers and both are stored.
struct oracle_record_t {
	uint8_t kind;
	uint8_t phase;
	uint16_t scenario_cursor; // 0xFFFF for control records
	uint32_t frame_index;
	uint8_t key_det_replay; // `input_replay_t`
	uint8_t shiftkey;
	uint16_t control; // control code for control records, 0 for input
};

typedef char oracle_header_size_check[
	(sizeof(oracle_header_t) == ORACLE_HEADER_SIZE) ? 1 : -1
];
typedef char oracle_startup_size_check[
	(sizeof(oracle_startup_t) == ORACLE_STARTUP_SIZE) ? 1 : -1
];
typedef char oracle_record_size_check[
	(sizeof(oracle_record_t) == ORACLE_RECORD_SIZE) ? 1 : -1
];

/// Trace container — `state/port/TXSPLIT_CONTRACT.md`
/// -------------------------------------------------

// Per-game row schema version. TH04 and TH05 versions are unrelated numbers;
// bump this on every field, order, or normalization change.
//
//   version 1  groups 0 (RNG) and 1 (run/scenario counters). row_size 64.
//   version 2  adds groups 2 (player) and 3 (bullets).       row_size 80.
//              The final two reserved bytes of the v1 critical block become
//              `bullets_alive`; the writer is already walking
//              the bullet array for group 3's hash.
//   version 3  adds groups 4 (enemies), 5 (actor core), and 6 (items,
//              gather circles, and point numbers). row_size is 104.
//
// The prefix and the first 30 bytes of the critical block are unchanged, so a
// v1 artifact stays readable forever. A reader keyed on the version REJECTS a
// file whose `row_size` disagrees rather than reinterpreting fields under a
// schema that does not describe them, as required by the harness contract.
#define ORACLE_SPLIT_VERSION       4
#define ORACLE_SPLIT_HEADER_SIZE   16
#define ORACLE_SPLIT_PREFIX_SIZE   16
#define ORACLE_SPLIT_CRITICAL_SIZE 32

// The harness schema-growth rule starts at groups 0-1 and says not to
// repeat TH03's mistake of deferring entities, bullets and enemies
// indefinitely -- those groups are exactly the ones whose absence forced
// separate diagnostics later."
//
// Group 2 is the player (position, shots, bomb state); group 3 is the bullet
// array plus the custom-entity block; groups 4-6 are enemies, the shared actor
// core, and item-related entities. Schema 4 adds scoring/rank, logical field
// state, and gameplay effects as groups 7-9. Groups 10-11 remain to be added;
// each bumps this version again. Group 5 deliberately says "core":
// stage-specific boss state remains part of the next additive inventory rather
// than being hidden behind native memory dumps here.
#define ORACLE_SPLIT_HASH_COUNT 10
#define ORACLE_HASH_GROUP_RNG     0
#define ORACLE_HASH_GROUP_RUN     1
#define ORACLE_HASH_GROUP_PLAYER  2
#define ORACLE_HASH_GROUP_BULLETS 3
#define ORACLE_HASH_GROUP_ENEMIES 4
#define ORACLE_HASH_GROUP_ACTORS  5
#define ORACLE_HASH_GROUP_ITEMS   6
#define ORACLE_HASH_GROUP_SCORING 7
#define ORACLE_HASH_GROUP_FIELD   8
#define ORACLE_HASH_GROUP_EFFECTS 9
#define ORACLE_SPLIT_ROW_SIZE ( \
	ORACLE_SPLIT_PREFIX_SIZE + \
	ORACLE_SPLIT_CRITICAL_SIZE + \
	(8 * ORACLE_SPLIT_HASH_COUNT) \
)

// Default certification cadence, not a truth condition. A debug run may
// shorten this to localize a divergence window.
#define ORACLE_SPLIT_INTERVAL_SAMPLES 128

enum oracle_split_event_t {
	ORACLE_EVENT_START       = 1,
	ORACLE_EVENT_ROUND_START = 2,
	ORACLE_EVENT_INPUT_END   = 3,
	ORACLE_EVENT_ERROR       = 4,
	ORACLE_EVENT_CHECKPOINT  = 5,
	ORACLE_EVENT_FINISH      = 6,
	ORACLE_EVENT_ROUTE       = 7,
};

struct oracle_split_header_t {
	char magic[8]; // "T4SPLT1\0" / "T5SPLT1\0"
	uint16_t version;
	uint16_t header_size;
	uint16_t row_size;
	uint16_t flags;
};

struct oracle_split_hash_t {
	uint32_t pass_b; // low half
	uint32_t pass_a; // high half
};

struct oracle_split_row_t {
	// Prefix — identical shape in all four games.
	uint8_t event;
	uint8_t process;
	uint8_t stage_id;
	uint8_t rank;
	uint32_t global_frame;
	uint32_t scenario_cursor;
	uint16_t input; // (shiftkey << 8) | key_det_replay
	uint8_t schema;
	uint8_t reserved0;

	// Critical fields — stored plainly, because a human and a comparator both
	// need to read them without a hash preimage.
	uint32_t random_seed;
	uint16_t randring_p;
	uint16_t stage_graze;
	uint8_t score[ORACLE_SCORE_DIGITS]; // LEBCD, the game's own representation
	uint32_t samples_consumed;
	uint8_t rem_lives;
	uint8_t rem_bombs;
	uint8_t credit_lives;
	uint8_t credit_bombs;
	uint8_t power;
	uint8_t playperf;
	uint8_t end_sequence;
	uint8_t quit;
	uint8_t bombing;
	// TH05 only; what distinguishes "before the Extra splice" from "after".
	uint8_t dialog_sequence_id;
	// Schema 2. Live entries in `bullets[BULLET_COUNT]`, i.e. those whose
	// `flag != F_FREE`. Plain rather than hashed, because when the group 3
	// hash goes red this is the first number a human needs, and the writer
	// counts it for free while serializing the array. Zero in a v1 file, where
	// these two bytes were reserved.
	uint16_t bullets_alive;

	// Subsystem hashes, group 0 first.
	oracle_split_hash_t hashes[ORACLE_SPLIT_HASH_COUNT];
};

typedef char oracle_split_header_size_check[
	(sizeof(oracle_split_header_t) == ORACLE_SPLIT_HEADER_SIZE) ? 1 : -1
];
typedef char oracle_split_row_size_check[
	(sizeof(oracle_split_row_t) == ORACLE_SPLIT_ROW_SIZE) ? 1 : -1
];
/// -------------------------------------------------

/// The two hooks
/// -------------

// Called as the very first statement of `ems_allocate_and_preload_eyecatch()`
// (`th04/main/ems.cpp:57`), which MAIN runs immediately after
// `game_init_main()` and `random_seed = resident->rand`
// (`th04_main.asm:301-304`, `th05_main.asm:342-345`) and well before the demo
// gate in the original setup routine. This is the earliest point at which the packfile is open
// (so `file_ropen()` works), the resident structure exists, and NOTHING has yet
// been derived from it — `ems_allocate_and_preload_eyecatch()` itself reads
// `resident->stage` and `resident->rank` on its very next lines.
//
// Record mode pins the scenario the way OP's `start_demo()` would have
// (`th04/op/start.cpp:53-86`, `th05/op/start.cpp:57-107`) and captures the
// startup block. Playback mode writes the case's startup block into
// `resident_t`. In both cases the game's own gate, `demo_load()`, stage init
// and `randring_fill()` then run completely unmodified.
//
// Hooking here rather than in OP is what makes the oracle independent of the
// attract-mode idle timeout AND of OP's per-frame `resident->rand++`
// (`th04/op/m_main.cpp:660`), which would otherwise make two independent
// recordings of the same scenario disagree on a recorded field.
void oracle_entry(void);

// True once a case is being recorded or played back.
bool oracle_active(void);

// Called from `DemoPlay()` in place of the stock `DemoBuf` read. Assigns
// `key_det` and `shiftkey` for this frame, appends or consumes one payload
// record, and emits trace rows at the contract's cadence.
//
// Returns false when the run has reached its terminal boundary and the caller
// should invoke `demo_end()`.
bool oracle_frame(uint16_t shift_offset);
/// --------------------------------------

#endif /* TH04_MAIN_ORACLE_HPP */
