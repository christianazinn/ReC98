/* ReC98 — harness/ORACLE-TH01-MASTER (MOD BRANCH, NOT byte-identical)
 * -------------------------------------------------------------------
 * T1CASE1 / T1SPLT1 — TH01's oracle case container and trace container.
 *
 * Contracts: state/port/TXCASE_CONTRACT.md, state/port/TXSPLIT_CONTRACT.md.
 * Ported from TH03's `th03/t3case.hpp` (branch harness/TH03-ORACLE-MASTER);
 * every divergence from that reference is justified in
 * state/notes/oracle-th01-bringup.md.
 *
 * This code exists ONLY on a mod branch. It must never be merged into a
 * matching branch: it changes REIIDEN.EXE's layout by design.
 */

#ifndef TH01_T1CASE_HPP
#define TH01_T1CASE_HPP

#include "platform.h"

/// Container identity
/// ------------------

#define T1CASE_VERSION      1
#define T1CASE_HEADER_SIZE  64
#define T1CASE_STARTUP_SIZE 64
#define T1CASE_RECORD_SIZE  16

// The seven `key_sense()` groups TH01's gameplay path reads, in the fixed
// order they are stored in. Groups 7/5/8/9 are read directly by input_sense()
// (th01/main_01.cpp:183-190), 0 and 3 through the `input_pause_ok_sense` macro
// (th01/hardware/input.hpp:89-92, invoked at th01/main_01.cpp:217), and 6 only
// when [mode_test] is set (th01/main_01.cpp:229-230).
#define T1CASE_GROUP_COUNT 7

#define T1CASE_GI_0 0
#define T1CASE_GI_3 1
#define T1CASE_GI_5 2
#define T1CASE_GI_6 3
#define T1CASE_GI_7 4
#define T1CASE_GI_8 5
#define T1CASE_GI_9 6

// Records
#define T1CASE_RECORD_INPUT   1
#define T1CASE_RECORD_CONTROL 2

// Phases
#define T1CASE_PHASE_GAMEPLAY     0
#define T1CASE_PHASE_INTERSTITIAL 1
#define T1CASE_PHASE_CONTROL      2

// Control codes (TXCASE_CONTRACT.md, "Control records")
#define T1CASE_CONTROL_PROCESS_END   1
#define T1CASE_CONTROL_CURSOR_RESET  2
#define T1CASE_CONTROL_STAGE         3
#define T1CASE_CONTROL_TERMINAL      4

// `first_process` / `start_binary`
#define T1CASE_PROCESS_REIIDEN 1
#define T1CASE_PROCESS_FUUIN   2
#define T1CASE_PROCESS_OP      3

// `source_kind`. TH01 has no ZUN demo system at all, so 2 (normalized ZUN
// demo) can never occur and only 1 is valid. Value 3 does not exist in TxCASE.
#define T1CASE_SOURCE_DIRECT 1

// `producer`
#define T1CASE_PRODUCER_GAME_MOD 1
#define T1CASE_PRODUCER_HOST     3

// Flags
#define T1CASE_FLAG_ADVISORY_POSITIONS 0x0001
#define T1CASE_FLAG_SOURCE_CLIPPED     0x0002
#define T1CASE_FLAG_SPLICED_SOURCE     0x0004
#define T1CASE_FLAG_KNOWN              0x0007

struct t1case_header_t {
	char magic[8]; // "T1CASE1\0"
	uint16_t version;
	uint16_t header_size;
	uint16_t startup_size;
	uint16_t record_size;
	uint32_t payload_offset;
	uint32_t payload_size;
	uint32_t sample_count; // records with kind == T1CASE_RECORD_INPUT
	uint32_t record_count; // input plus control records
	uint8_t source_kind;
	uint8_t input_semantics;
	uint8_t ruleset_id;
	uint8_t scenario_id;
	uint8_t first_process;
	uint8_t producer;
	uint16_t flags;
	uint32_t case_id;
	uint32_t source_digest;
	uint32_t source_commit;
	uint32_t payload_checksum;
	uint32_t header_checksum;
	uint32_t total_size;
};

// Deliberately NOT a memcpy of resident_t: this is an audited, explicitly
// typed field list, and the writer converts. See TXCASE_CONTRACT.md,
// "Startup blocks", and state/re/DETERMINISTIC_STATE_TH01.md §2.
struct t1case_startup_t {
	uint32_t resident_rand; // resident_t::rand; seeds [frame_rand]
	int32_t score;
	int32_t continues_total;
	uint32_t hiscore;
	int32_t score_highest;
	int32_t bonus_per_stage[4]; // STAGES_PER_SCENE - 1
	uint16_t continues_per_scene[4]; // SCENE_COUNT
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

	// Recorded separately from [debug_mode] because it changes the number of
	// key_sense() calls per frame (th01/main_01.cpp:228-243) and is therefore
	// an input-stream input. Required zero for an oracle case.
	int8_t mode_test;

	int8_t start_binary; // matches header.first_process
	int8_t reserved[3];  // required zero
};

struct t1case_record_t {
	uint8_t kind;
	uint8_t phase;

	// The game's own cursor: the low 16 bits of [frame_rand]. Diagnostic only
	// — the authoritative cursor is [frame_index]. 0xFFFF for control records.
	uint16_t scenario_cursor;

	uint32_t frame_index; // dense, monotonic, never reset across the case

	// The values t1case_key_sense() must return for groups 0, 3, 5, 6, 7, 8, 9
	// in that fixed order. For a control record, keys[0..1] carry the 16-bit
	// control code and keys[2..6] are required zero.
	uint8_t keys[T1CASE_GROUP_COUNT];

	uint8_t reserved; // required zero
};

/// Trace container
/// ---------------

#define T1SPLIT_VERSION       1
#define T1SPLIT_HEADER_SIZE   16
#define T1SPLIT_CRITICAL_SIZE 44
#define T1SPLIT_GROUPS        10
#define T1SPLIT_ROW_SIZE      (16 + T1SPLIT_CRITICAL_SIZE + (8 * T1SPLIT_GROUPS))

// Must be a power of two: the cadence test is a mask, not a modulo.
#define T1SPLIT_INTERVAL_SAMPLES 64

enum t1split_event_t {
	T1SPLIT_EVENT_START       = 1,
	T1SPLIT_EVENT_ROUND_START = 2,
	T1SPLIT_EVENT_INPUT_END   = 3,
	T1SPLIT_EVENT_ERROR       = 4,
	T1SPLIT_EVENT_CHECKPOINT  = 5,
	T1SPLIT_EVENT_FINISH      = 6,
	T1SPLIT_EVENT_ROUTE       = 7
};

struct t1split_header_t {
	char magic[8]; // "T1SPLT1\0"
	uint16_t version;     // per-game row schema version
	uint16_t header_size; // 16
	uint16_t row_size;    // T1SPLIT_ROW_SIZE
	uint16_t flags;       // copy of `version`, mirroring T3SPLT1's schema channel
};

struct t1split_row_t {
	/// Prefix — 16 bytes, shared shape with every other TxSPLIT game
	uint8_t event;
	uint8_t process;
	uint8_t stage_id; // truncated; the full value is in the critical block
	uint8_t rank;
	uint32_t global_frame;    // dense; matches the case's [frame_index]
	uint32_t scenario_cursor; // low 32 bits of [frame_rand]; NOT monotonic
	uint16_t input;           // digest of the seven group bytes, see below
	uint8_t schema;           // copy of the header `version`
	uint8_t reserved0;

	/// Critical fields — 44 bytes, readable without a hash preimage
	uint32_t score;
	uint32_t frame_rand;
	uint32_t random_seed;
	uint32_t samples_consumed;
	int32_t score_highest;
	uint32_t bomb_frame;
	int16_t rem_lives;
	int16_t boss_hp;
	int16_t player_left;
	int16_t pellet_speed;
	uint16_t point_value;
	uint16_t stage_id_full;
	int8_t rem_bombs;
	int8_t credit_lives_extra;
	int8_t route;
	int8_t end_flag;
	int8_t boss_id;
	int8_t boss_phase;
	int8_t stage_cleared;
	int8_t player_is_hit;

	/// Subsystem hashes — 10 x 64-bit, little-endian, low word first.
	/// hash[2*g] is pass B, hash[(2*g) + 1] is pass A
	/// (TXSPLIT_CONTRACT.md §7: hash64 = (passA << 32) | passB).
	uint32_t hash[T1SPLIT_GROUPS * 2];
};

// Subsystem groups, numbered exactly as state/re/DETERMINISTIC_STATE_TH01.md §6.
#define T1SPLIT_G_RNG      0
#define T1SPLIT_G_RUN      1
#define T1SPLIT_G_PLAYER   2
#define T1SPLIT_G_BULLETS  3
#define T1SPLIT_G_BOSS     4
#define T1SPLIT_G_STAGEOBJ 5 // [open] schema 2
#define T1SPLIT_G_SCORING  6
#define T1SPLIT_G_HUD      7
#define T1SPLIT_G_EFFECTS  8 // [open] schema 2
#define T1SPLIT_G_INPUT    9

/// Hashes
/// ------

#define T1CASE_FNV1A_BASIS 0x811C9DC5UL
#define T1CASE_FNV1A_PRIME 0x01000193UL
#define T1SPLIT_PASSB_BASIS 0x7EE3623AUL

/// Build-time size proofs
/// ----------------------
/// Turbo C++ 4.0J accepts the negative-array-size idiom at file scope, which
/// the C++17 `static_assert()` in platform.h cannot be (it is an expression).

typedef char t1case_header_size_check[
	(sizeof(t1case_header_t) == T1CASE_HEADER_SIZE) ? 1 : -1
];
typedef char t1case_startup_size_check[
	(sizeof(t1case_startup_t) == T1CASE_STARTUP_SIZE) ? 1 : -1
];
typedef char t1case_record_size_check[
	(sizeof(t1case_record_t) == T1CASE_RECORD_SIZE) ? 1 : -1
];
typedef char t1split_header_size_check[
	(sizeof(t1split_header_t) == T1SPLIT_HEADER_SIZE) ? 1 : -1
];
typedef char t1split_row_size_check[
	(sizeof(t1split_row_t) == T1SPLIT_ROW_SIZE) ? 1 : -1
];

/// The seam
/// --------
/// Every keyboard byte REIIDEN.EXE observes flows through these. The injector
/// replaces master.lib's key_sense() at the CALL SITE rather than at the
/// symbol, so ZUN's edge-detection logic, [input_prev] and the [input_bomb]
/// double-tap derivation all run completely unmodified.

#if defined(__cplusplus)
extern "C" {
#endif

// Advances the case cursor by exactly one record and latches this pass's
// seven group bytes. Called once at the top of input_sense(), AFTER the
// `reset_repeat` early-return — input_sense(true) issues no key_sense() call
// and must not consume a record.
//
// [prev] is input_sense()'s function-local `static uint8_t input_prev[16]`,
// which is otherwise unreachable and is the only way to prove the injector did
// not desynchronize the edge detector.
void far t1case_frame_io(uint8_t near *prev);

// Returns the latched byte for [keygroup]. Indexed by the frame cursor, never
// by a call counter: each group is sensed twice and OR'd as a keyboard-UART
// guard, and `a | a == a` makes the repeat exact.
int far t1case_key_sense(int keygroup);

// Pre-init: applies the case's startup block, creating resident_t when this
// process is the case's first. Called from REIIDEN's main() after
// resident_stuff_get() and before irand_init(frame_rand).
void far t1case_session_start(void);

// Post-init verify plus the `start` / `round_start` trace rows.
void far t1case_round_start(void);

// Process handoff. [terminal] marks the case's last process.
void far t1case_finish(bool16 terminal);

#if defined(__cplusplus)
}
#endif

#endif /* TH01_T1CASE_HPP */
