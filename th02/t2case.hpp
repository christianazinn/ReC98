/* ReC98 — harness/ORACLE-TH02-MASTER (MOD BRANCH, NOT byte-identical)
 * -------------------------------------------------------------------
 * T2CASE1 / T2SPLT1 — TH02's oracle case container and trace container.
 *
 * Contracts: state/port/TXCASE_CONTRACT.md, state/port/TXSPLIT_CONTRACT.md.
 * Ported from `th01/t1case.hpp` (branch harness/ORACLE-TH01-MASTER); every
 * divergence from that reference is justified in
 * state/notes/oracle-th02-bringup.md §2.
 *
 * This code exists ONLY on a mod branch. It must never be merged into a
 * matching branch: it changes MAIN.EXE's layout by design.
 */

#ifndef TH02_T2CASE_HPP
#define TH02_T2CASE_HPP

#include "platform.h"

/// Container identity
/// ------------------

#define T2CASE_VERSION      1
#define T2CASE_HEADER_SIZE  64
#define T2CASE_STARTUP_SIZE 48
#define T2CASE_RECORD_SIZE  12

// Records
#define T2CASE_RECORD_INPUT   1
#define T2CASE_RECORD_CONTROL 2

// Phases. TH02's injection seam (th02_main.asm:1651) sits inside the per-frame
// gameplay function only, so an input record is always PHASE_GAMEPLAY. The
// field is kept because TXCASE_CONTRACT.md fixes the record prefix across all
// five games.
#define T2CASE_PHASE_GAMEPLAY     0
#define T2CASE_PHASE_INTERSTITIAL 1
#define T2CASE_PHASE_CONTROL      2

// Control codes (TXCASE_CONTRACT.md, "Control records"). CURSOR_RESET (2) is
// deliberately absent: [demo_frame] (th02/main/demo.h:5) is incremented only by
// the injector and is never reset mid-process, so TH02 has no cursor-reset site
// (DETERMINISTIC_STATE_TH02.md §7). It resets to 0 at each MAIN process start
// because it is initialized data (`_demo_frame dw 0`,
// th02/main/demo[data].asm:2) — a process boundary, which PROCESS_END already
// marks.
#define T2CASE_CONTROL_PROCESS_END 1
#define T2CASE_CONTROL_STAGE       3
#define T2CASE_CONTROL_TERMINAL    4

// `first_process` / `start_binary`
#define T2CASE_PROCESS_MAIN  1
#define T2CASE_PROCESS_MAINE 2
#define T2CASE_PROCESS_OP    3

// `source_kind`. 2 is the normal case for TH02: DEMO1/2/3.REC are members
// inside the retail packfile 東方封魔.録 (obfuscated directory, RLE payloads),
// all three with an original size of exactly DEMO_N * 2 = 14000 bytes, and
// master.lib's INT 21h hook serves them through demo_load()'s own file_ropen().
// See state/port/DEMO_CORPUS_STATUS.md. Value 3 does not exist in TxCASE.
#define T2CASE_SOURCE_DIRECT     1
#define T2CASE_SOURCE_NORMALIZED 2

// `producer`
#define T2CASE_PRODUCER_GAME_MOD 1
#define T2CASE_PRODUCER_HOST     3

// Flags
#define T2CASE_FLAG_ADVISORY_POSITIONS 0x0001
#define T2CASE_FLAG_SOURCE_CLIPPED     0x0002
#define T2CASE_FLAG_SPLICED_SOURCE     0x0004
#define T2CASE_FLAG_KNOWN              0x0007

struct t2case_header_t {
	char magic[8]; // "T2CASE1\0"
	uint16_t version;
	uint16_t header_size;
	uint16_t startup_size;
	uint16_t record_size;
	uint32_t payload_offset;
	uint32_t payload_size;
	uint32_t sample_count; // records with kind == T2CASE_RECORD_INPUT
	uint32_t record_count; // input plus control records
	uint8_t source_kind;
	uint8_t input_semantics;
	uint8_t ruleset_id;
	uint8_t scenario_id; // 0 normal start, 1-3 demo 1-3
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

// Deliberately NOT a memcpy of resident_t: an audited, explicitly typed field
// list, per TXCASE_CONTRACT.md "Startup blocks" (TH02 — 48 bytes) and
// state/re/DETERMINISTIC_STATE_TH02.md §3.
//
// [stage_id], [power] and [playperf] are MAIN-local, not resident: demo_load()
// pins them at th02_main.asm:1997, :1998 and :2001-2035. A non-demo case would
// have to record them explicitly, which is why they are in the block.
struct t2case_startup_t {
	uint32_t resident_frame; // resident_t::frame — RNG seed AND frame counter
	int32_t score;
	int32_t score_highest;
	uint16_t continues_used;
	int16_t skill;
	int8_t stage;
	int8_t rank;
	int8_t rem_lives;
	int8_t rem_bombs;
	uint8_t start_lives;
	uint8_t start_bombs;
	int8_t start_power;
	uint8_t shottype;
	int8_t bgm_mode;
	int8_t demo_num;
	int8_t debug;
	int8_t reduce_effects;
	uint8_t op_main_retval;
	int8_t stage_id;   // MAIN-local
	uint8_t power;     // MAIN-local
	int8_t playperf;   // MAIN-local
	uint8_t reserved[16]; // required zero
};

struct t2case_record_t {
	uint8_t kind;
	uint8_t phase;

	// The game's own cursor: [demo_frame] (th02/main/demo.h:5). Diagnostic
	// only — the authoritative cursor is [frame_index]. 0xFFFF for control
	// records.
	uint16_t scenario_cursor;

	uint32_t frame_index; // dense, monotonic, never reset across the case

	// The full 16-bit [key_det] (th02/hardware/input.hpp:3,28). ZUN's own
	// DemoBuf is `input_t *` read as a whole word at `demo_frame * 2`
	// (th02_main.asm:2052-2058), so TH02 loses nothing by recording it
	// verbatim — unlike TH04/TH05, whose replay format is 8 bits wide.
	uint16_t key_det;

	uint16_t control; // control code for control records, else required zero
};

/// Trace container
/// ---------------

#define T2SPLIT_VERSION       1
#define T2SPLIT_HEADER_SIZE   16
#define T2SPLIT_CRITICAL_SIZE 44
#define T2SPLIT_GROUPS        10
#define T2SPLIT_ROW_SIZE      (16 + T2SPLIT_CRITICAL_SIZE + (8 * T2SPLIT_GROUPS))

// Must be a power of two: the cadence test is a mask, not a modulo.
//
// 16, not TXSPLIT_CONTRACT.md §4's default 128 and not TH01's 64. §4 marks the
// cadence `[open]` for TH01 and TH02 specifically because the RNG *is* the
// frame counter (resident->frame, th02_main.asm:747, :1787), so a divergence in
// *when* a frame happens is a divergence in *what* happens and wants a narrow
// localization window. TH02 can afford it: a MAIN process is bounded by
// DEMO_N - 50 = 6950 gameplay frames (th02_main.asm:2060), so 16 yields at most
// ~435 rows per process — the same order as TH01's 1261-row Gate A trace.
#define T2SPLIT_INTERVAL_SAMPLES 16

enum t2split_event_t {
	T2SPLIT_EVENT_START       = 1,
	T2SPLIT_EVENT_ROUND_START = 2,
	T2SPLIT_EVENT_INPUT_END   = 3,
	T2SPLIT_EVENT_ERROR       = 4,
	T2SPLIT_EVENT_CHECKPOINT  = 5,
	T2SPLIT_EVENT_FINISH      = 6,
	T2SPLIT_EVENT_ROUTE       = 7
};

struct t2split_header_t {
	char magic[8]; // "T2SPLT1\0"
	uint16_t version;     // per-game row schema version
	uint16_t header_size; // 16
	uint16_t row_size;    // T2SPLIT_ROW_SIZE
	uint16_t flags;       // copy of `version`, mirroring T3SPLT1's schema channel
};

struct t2split_row_t {
	/// Prefix — 16 bytes, shared shape with every other TxSPLIT game
	uint8_t event;
	uint8_t process;
	uint8_t stage_id; // truncated; the full value is in the critical block
	uint8_t rank;
	uint32_t global_frame;    // dense; matches the case's [frame_index]
	uint32_t scenario_cursor; // [demo_frame]; resets to 0 per MAIN process
	uint16_t input;           // the exact [key_det] injected on this frame
	uint8_t schema;           // copy of the header `version`
	uint8_t reserved0;

	/// Critical fields — 44 bytes, readable without a hash preimage.
	/// Ordered widest-first so the block is packed by construction under
	/// Turbo C++'s word alignment.
	uint32_t score;
	uint32_t random_seed;
	uint32_t samples_consumed;
	uint32_t stage_frame;
	uint32_t resident_frame;
	int32_t score_highest;
	uint16_t continues_used;
	uint16_t demo_frame;
	int16_t playperf;
	int16_t item_skill;
	uint8_t randring_p;
	uint8_t stage_progression;
	int8_t lives;
	int8_t bombs;
	uint8_t power;
	uint8_t playperf_max;
	uint8_t total_miss_count;
	uint8_t total_bombs_used;
	uint8_t stage_miss_count;
	uint8_t stage_bombs_used;
	uint8_t slowdown_factor;
	uint8_t quit;

	/// Subsystem hashes — 10 x 64-bit, little-endian, low word first.
	/// hash[2*g] is pass B, hash[(2*g) + 1] is pass A
	/// (TXSPLIT_CONTRACT.md §7: hash64 = (passA << 32) | passB).
	uint32_t hash[T2SPLIT_GROUPS * 2];
};

// Subsystem groups, numbered exactly as
// state/re/DETERMINISTIC_STATE_TH02.md §4.
#define T2SPLIT_G_RNG      0
#define T2SPLIT_G_RUN      1
#define T2SPLIT_G_PLAYER   2
#define T2SPLIT_G_BULLETS  3
#define T2SPLIT_G_ENEMIES  4
#define T2SPLIT_G_ITEMS    5
#define T2SPLIT_G_SCORING  6
#define T2SPLIT_G_FIELD    7
#define T2SPLIT_G_SPARKS   8
#define T2SPLIT_G_PACING   9

/// Hashes
/// ------

#define T2CASE_FNV1A_BASIS 0x811C9DC5UL
#define T2CASE_FNV1A_PRIME 0x01000193UL
#define T2SPLIT_PASSB_BASIS 0x7EE3623AUL

/// Build-time size proofs
/// ----------------------
/// Turbo C++ 4.0J accepts the negative-array-size idiom at file scope, which
/// the C++17 `static_assert()` in platform.h cannot be (it is an expression).

typedef char t2case_header_size_check[
	(sizeof(t2case_header_t) == T2CASE_HEADER_SIZE) ? 1 : -1
];
typedef char t2case_startup_size_check[
	(sizeof(t2case_startup_t) == T2CASE_STARTUP_SIZE) ? 1 : -1
];
typedef char t2case_record_size_check[
	(sizeof(t2case_record_t) == T2CASE_RECORD_SIZE) ? 1 : -1
];
typedef char t2split_header_size_check[
	(sizeof(t2split_header_t) == T2SPLIT_HEADER_SIZE) ? 1 : -1
];
typedef char t2split_row_size_check[
	(sizeof(t2split_row_t) == T2SPLIT_ROW_SIZE) ? 1 : -1
];

/// The seam
/// --------
/// Names are plain C++ so that they mangle exactly as ZUN's own do, which is
/// what lets `th02_main.asm` name them in a `nopcall` operand without any
/// change to that file's length. Nothing here is `extern "C"`.

// The injector, in place of `DemoPlay`. Redirected from the `nopcall DemoPlay`
// operand at th02_main.asm:1651 — an operand change only, so the original code
// segment keeps every offset. Advances the case cursor by exactly one record,
// overwrites [key_det] and increments [demo_frame], exactly where ZUN's
// DemoPlay does both.
//
// ORACLE DEVIATION: stock DemoPlay aborts the demo whenever `key_det != 0` on
// entry (th02_main.asm:2050-2051, loc_C20B at :2063-2069). A TxCASE player must
// NOT reproduce that guard — it makes playback depend on the host keyboard,
// which is the opposite of an oracle (TXCASE_CONTRACT.md, "The abort guard must
// not be reproduced"). [key_det] is overwritten unconditionally.
void t2case_frame_io(void);

// Round start. Redirected from the `nopcall @overlay_stage_enter_animate$qv`
// operand at th02_main.asm:750 ONLY — that is the site TXSPLIT_CONTRACT.md §3
// names, immediately after stage init `sub_B3DA` (th02_main.asm:972-1292),
// which contains `randring_fill()` at :1004. The two other call sites, :1326
// (continue/respawn) and :2176 (game over), are deliberately left stock. Calls
// overlay_stage_enter_animate() itself.
void t2case_stage_enter(void);

// Pre-init: applies (playback) or captures (recording) the case's startup
// block. Called at the end of game_init_main() (th02/core/initmain.cpp), which
// th02_main.asm:704 invokes before `resident->demo_num` is read at :738, before
// demo_load at :740 and before `random_seed = resident->frame` at :747.
void t2case_session_start(void);

// Process handoff. Called from game_exit() (th02/core/exit.cpp), which
// GameExecl (th02_main.asm:2354-2372) invokes at :2368 — the single point every
// MAIN.EXE handoff passes through, whether the target is MAINE or OP.
void t2case_process_exit(void);

// True once a case is being recorded or played back in this process.
bool16 t2case_active(void);

#endif /* TH02_T2CASE_HPP */
