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

#include <stddef.h> // offsetof(), for the carrier's region-map proofs
#include "platform.h"

/// Container identity
/// ------------------

// Version 2 replaced version 1's fixed 16-byte records with the shared replay
// core's N-channel packet RLE (state/port/REPLAY_CORE_CONTRACT.md §4). The
// logical sample sequence is unchanged; only the payload encoding is.
#define T1CASE_VERSION      3
#define T1CASE_HEADER_SIZE  64
#define T1CASE_STARTUP_SIZE 64

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

// Phases. A packet's phase lives in the top two bits of its tag byte; 3 is
// not a valid phase and a decoder rejects it.
#define T1CASE_PHASE_GAMEPLAY     0
#define T1CASE_PHASE_INTERSTITIAL 1
#define T1CASE_PHASE_CONTROL      2

/// Packet RLE
/// ----------
/// REPLAY_CORE_CONTRACT.md §4, generalized from TH03's four fixed channels to
/// N declared channels of declared width. TH01 is the tightest fit of the five
/// games: seven 1-byte channels need seven of the change mask's eight bits.
///
///   byte 0  tag  = (phase << 6) | (run - 1)      run 1..64
///   byte 1  mask = bit i set <=> channel i changed; bit 7 is SPARE and must
///                  be zero
///   [...]   each changed channel's byte, in ascending channel index
///
/// A control packet is phase 2 and always exactly two bytes:
///   byte 0  tag  = 0x80 | control code
///   byte 1  the EMITTING process id
/// TH03 stores a fixed 0xA5 marker there. TH01 cannot: REIIDEN hands off to
/// ITSELF, so TH03's "controls alternate, beginning with MAIN" invariant proves
/// nothing here, and an explicit process id detects "resumed in the wrong
/// binary" directly (REPLAY_CORE_CONTRACT.md §7.2).
///
/// An unchanged channel retains its previous value WITHIN a process segment.
/// The first packet of a segment — at session start, and after every control
/// packet — is a complete keyframe with every change bit set.

#define T1CASE_PACKET_RUN_MAX     64
#define T1CASE_PACKET_RUN_MASK    0x3F
#define T1CASE_PACKET_PHASE_SHIFT 6

// Seven channels, so the low seven bits. Bit 7 is the one spare bit the format
// has left; do not spend it casually (REPLAY_CORE_CONTRACT.md §4.1).
#define T1CASE_PACKET_KEYFRAME_MASK ((1 << T1CASE_GROUP_COUNT) - 1)

// tag + mask + one byte per channel. Exactly TH03's 9, which is why its write
// buffer sizing argument carries over verbatim.
#define T1CASE_PACKET_SIZE_MAX (2 + T1CASE_GROUP_COUNT)

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

// Flags. ADVISORY_POSITIONS described v1's per-record `scenario_cursor`, which
// a packet stream does not carry; it is dead for TH01 and is never set. The bit
// number is not reused.
#define T1CASE_FLAG_ADVISORY_POSITIONS 0x0001
#define T1CASE_FLAG_SOURCE_CLIPPED     0x0002
#define T1CASE_FLAG_SPLICED_SOURCE     0x0004

// The recording ran out of checkpoint slots. Not an error: every sample is
// still recorded and every earlier checkpoint still restores. It marks the
// case as resumable only up to `checkpoint_count`.
#define T1CASE_FLAG_CHECKPOINTS_FULL   0x0008

#define T1CASE_FLAG_KNOWN              0x000F

struct t1case_header_t {
	char magic[8]; // "T1CASE1\0"
	uint16_t version;
	uint16_t header_size;
	uint16_t startup_size;

	// v1's `record_size`, at the same offset and width. A packet stream has no
	// fixed record size; what a decoder needs instead is how many channels the
	// change mask covers, which is the core's own per-game parameter
	// (REPLAY_CORE_CONTRACT.md §9).
	uint16_t channel_count;

	uint32_t payload_offset;
	uint32_t payload_size;  // encoded byte length of the packet stream
	uint32_t sample_count;  // input samples; excludes control packets
	uint32_t record_count;  // input samples plus control packets
	uint8_t source_kind;
	uint8_t input_semantics;
	uint8_t ruleset_id;
	uint8_t scenario_id;
	uint8_t first_process;
	uint8_t producer;
	uint16_t flags;
	// v2's `case_id`, at the same offset and width. It was declared, never
	// written and validated for nothing (REPLAY_CORE_CONTRACT.md open item
	// 7), which is the trap that item calls it. Version 3 spends it on the
	// only thing the checkpoint array needs that its slots cannot carry:
	// where the array is and how much of it is real.
	uint8_t checkpoint_capacity; // slots reserved in the file; may be 0
	uint8_t checkpoint_count;    // slots actually written
	uint16_t checkpoint_stride;  // bytes per slot
	// v2's `source_digest`, at the same offset and width, and the second of
	// the three fields REPLAY_CORE_CONTRACT.md open item 7 records as
	// declared, never written and validated for nothing. It now covers the
	// checkpoint array: FNV-1a over the `checkpoint_count` written slots.
	//
	// The array needs its own cover because `header_checksum` spans only the
	// header and the startup block, and `payload_checksum` only the packet
	// stream - so before version 3 the whole array sat between two checksums
	// and under neither. A corrupted slot would have restored cleanly: the
	// cursors would be in range, and t1case_startup_verify() compares the
	// resident against the very block that was applied to it, so it agrees
	// with a corrupted block by construction.
	uint32_t checkpoint_checksum;
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

/// Per-round checkpoints
/// ---------------------
/// REPLAY_CORE_CONTRACT.md 5, and the measured answer to its 5.4 hypothesis.
///
/// A checkpoint is captured at the start of every PROCESS SEGMENT, and that is
/// not the same thing as every stage start. REIIDEN's stage-clear `execl` is
/// GUARDED - `if(stage_is_boss(stage_id) || (boss_id != BID_NONE))`,
/// th01/main_01.cpp - so within a five-stage scene only 3->4 and 4->5 start a
/// new process, and the other three stage transitions stay inside one. A fresh
/// process rebuilds its entire live state from `resident_t` plus constants
/// before the first input_sense() of the stage; a mid-process stage start does
/// not, and it additionally re-enters the `while(!input_shot)` wait, because a
/// fresh process sets [stage_wait_for_shot_to_begin] - which CONSUMES CASE
/// SAMPLES THE RECORDING DOES NOT CONTAIN. That last one is a control-flow
/// problem and no payload size fixes it.
///
/// So the payload is cheap because the resume point is a PROCESS ENTRY, not
/// because it is a stage start. Details: state/notes/t1case-checkpoint.md.
///
/// The slot, stride 80:
///
///   +0  u32 sample_count      | the core's 12-byte cursor prefix
///   +4  u32 global_frame      |
///   +8  u32 input_byte_count  |
///   +12 u32 payload_checksum  running FNV-1a over the stream consumed so far
///   +16 64B startup block     TH01's whole cross-process state
///
/// TH03's prefix has no checksum field, because its seek re-decodes from byte 0
/// and recomputes one. TH01 has no seek (TXCASE_CONTRACT.md), so without this
/// field a checkpoint-started playback could never satisfy
/// t1case_playback_final()'s checksum equality - the container's strongest
/// end-to-end check. Four bytes to avoid weakening a check to fit a feature.
///
/// NOT stored, because they are recomputable:
///   record_count == sample_count + checkpoint index. Exactly one control
///                   packet per process segment and exactly one checkpoint per
///                   process segment, so the index IS the number of controls
///                   already consumed. Witnessed at capture AND at restore, and
///                   fails closed either way.
///   split_rows   == (trace length - header) / row_size.
///   the stage/round DIRECTORY TH03 keeps beside its array
///                   (checkpoint_stage_round[cap], th03/replay_format.hpp:471):
///                   TH01's slot carries the whole startup block and `stage_id`
///                   is a field of it, so there is nothing left for a directory
///                   to say.

#define T1CASE_CHECKPOINT_PREFIX_SIZE 16
#define T1CASE_CHECKPOINT_STRIDE \
	(T1CASE_CHECKPOINT_PREFIX_SIZE + T1CASE_STARTUP_SIZE)

// Slots reserved in a case the GAME writes. Measured bound: the longest TH01
// recording to date is 13 REIIDEN processes
// (state/notes/t1case-handoff-carrier.md 5.3), and a full playthrough is 8 boss
// handoffs plus one per continue. 16 is the next power of two above the
// measured maximum. Overflow is not an error - the recorder keeps recording and
// sets T1CASE_FLAG_CHECKPOINTS_FULL, because a case that cannot be resumed from
// its 17th process is still a valid case.
//
// The CAPACITY IS A HEADER FIELD, not this constant. A host-converted case
// carries capacity 0 and therefore costs nothing, which is what keeps the
// archived Gate A cases comparable across the version bump.
#define T1CASE_CHECKPOINT_CAP 16

#define T1CASE_CHECKPOINT_ARRAY_OFFSET (T1CASE_HEADER_SIZE + T1CASE_STARTUP_SIZE)

struct t1case_checkpoint_t {
	uint32_t sample_count;
	uint32_t global_frame;
	uint32_t input_byte_count;
	uint32_t payload_checksum;
	t1case_startup_t startup;
};

/// Trace container
/// ---------------

#define T1SPLIT_VERSION       1
#define T1SPLIT_HEADER_SIZE   16
#define T1SPLIT_CRITICAL_SIZE 44
#define T1SPLIT_GROUPS        10
#define T1SPLIT_ROW_SIZE      (16 + T1SPLIT_CRITICAL_SIZE + (8 * T1SPLIT_GROUPS))

// Must be a power of two: the cadence test is a mask, not a modulo.
//
// This is also the packet stream's disk-flush cadence, exactly as TH03 ties
// T3_REPLAY_WRITE_BUFFER_SIZE to T3_REPLAY_DISK_INTERVAL_SAMPLES. Measured
// against TH01's real Gate A corpus, 64 costs 2,655 payload bytes on an
// 80,000-sample case and 128 costs 2,653 — so the core's 128 buys nothing here
// and TH01 keeps its 64. See state/notes/t1case-packet-rle.md §1.
#define T1SPLIT_INTERVAL_SAMPLES 64

// The savestate guard's cadence (REPLAY_CORE_CONTRACT.md §6.1), DELIBERATELY a
// separate constant from T1SPLIT_INTERVAL_SAMPLES even though both are 64.
// T1SPLIT_INTERVAL_SAMPLES also sizes T1CASE_WBUF_SIZE and sets the packet
// stream's flush cadence, so changing it changes recorded bytes and would
// invalidate the byte-identical T1SPLIT.BIN regression that steps 1 and 2 both
// rest on. The guard file grows one byte per checkpoint, so this is also the
// guard's growth rate. Must be a power of two: the test is a mask.
#define T1CASE_GUARD_INTERVAL_SAMPLES 64

// Large enough for the worst-case packet on each of the T1SPLIT_INTERVAL_SAMPLES
// logical samples between disk flushes, plus one control/overflow packet. Same
// cadence argument as tools/replay/FORMAT.md:752-755, with TH01's 64 in place
// of TH03's 128.
#define T1CASE_WBUF_SIZE ( \
	(T1CASE_PACKET_SIZE_MAX * T1SPLIT_INTERVAL_SAMPLES) + T1CASE_PACKET_SIZE_MAX \
)

// Playback read-ahead. Only the sequential decoder uses it; the oracle lineage
// has no seek (TXCASE_CONTRACT.md), so TH03's 4 KiB seek reader is not ported.
#define T1CASE_RBUF_SIZE 256

/// Handoff carrier
/// ---------------
/// REPLAY_CORE_CONTRACT.md §7, ported from th03/replay_handoff.hpp. TH03 carves
/// its carrier out of `resident_t::unused_3[198]`, so every field there is a
/// named INDEX into a raw byte array and the non-overlap assertions are `#error`
/// guards on that index arithmetic. TH01 cannot extend `resident_t` — it has two
/// spare bytes and its layout is binary identity — so the carrier is a SECOND
/// ResData block, `"T1CaseState"`, which means TH01 gets to declare a real
/// struct instead of carving an array.
///
/// Both are bindings of the same core region map. The map is declared below as
/// indices exactly like TH03's, the `#error` guards assert that no two regions
/// overlap and that the map fits, and the `offsetof` proofs at the bottom of
/// this file assert that the struct binding lands on the map. Getting a field
/// wrong is therefore a compile error in either binding style, which is the
/// property §7 asks for; the array-of-bytes shape is not.
///
/// Every region is relative to the START OF THE BLOCK, so index 0 is the
/// ResData ID that `resdata_create()` writes there itself.

#define T1CASE_RES_ID "T1CaseState"
#define T1CASE_RES_ID_SIZE 12 // sizeof(T1CASE_RES_ID)

// A four-byte magic, not the ResData ID. The ID only proves master.lib found
// A block under that name; the magic plus [carrier_version] prove that THIS
// build wrote it. A stale block left by an earlier mod build of REIIDEN has the
// right ID and the wrong layout, and before this parcel the only validity test
// was `id[0] == 'T'`.
#define T1CASE_RES_MAGIC_INDEX   T1CASE_RES_ID_SIZE
#define T1CASE_RES_MAGIC_SIZE    4
#define T1CASE_RES_MAGIC_0       'T'
#define T1CASE_RES_MAGIC_1       '1'
#define T1CASE_RES_MAGIC_2       'C'
#define T1CASE_RES_MAGIC_3       'S'
// Bumped to 3 by W3.1 step 4: the checkpoint-array checksum was inserted
// ahead of the protect region, which moves every protect byte. 101 B is still
// 7 paragraphs, exactly the band this comment already warned about, so
// `resdata_exist()` would otherwise hand a step-4 build a step-3 block whose
// protect state is offset by four.
//
// Bumped to 2 by W3.1 step 3: the protect region's interior layout is now
// defined, and that change lands INSIDE a constant paragraph count (97 B is 7
// paragraphs, and everything from 81 to 112 rounds the same way), so
// `resdata_exist()` will happily hand a step-3 build a step-2 block. The version
// byte is the only thing that catches it. See REPLAY_CORE_CONTRACT.md §7.4.
#define T1CASE_RES_VERSION       3

#define T1CASE_RES_VERSION_INDEX (T1CASE_RES_MAGIC_INDEX + T1CASE_RES_MAGIC_SIZE)
#define T1CASE_RES_MODE_INDEX    (T1CASE_RES_VERSION_INDEX + 1)
#define T1CASE_RES_SLOT_INDEX    (T1CASE_RES_MODE_INDEX + 1)
#define T1CASE_RES_PROCESS_INDEX (T1CASE_RES_SLOT_INDEX + 1)
#define T1CASE_RES_SEQ_INDEX     (T1CASE_RES_PROCESS_INDEX + 1)
#define T1CASE_RES_FLAGS_INDEX   (T1CASE_RES_SEQ_INDEX + 2)

// The three cursors of §4.4, as one contiguous region so a clear is one loop —
// TH03's `T3_REPLAY_RES_SAMPLE_COUNT_INDEX .. _CURSOR_END_INDEX` sweep.
#define T1CASE_RES_CURSOR_INDEX       (T1CASE_RES_FLAGS_INDEX + 2)
#define T1CASE_RES_SAMPLE_COUNT_INDEX T1CASE_RES_CURSOR_INDEX
#define T1CASE_RES_GLOBAL_FRAME_INDEX (T1CASE_RES_SAMPLE_COUNT_INDEX + 4)
#define T1CASE_RES_INPUT_BYTES_INDEX  (T1CASE_RES_GLOBAL_FRAME_INDEX + 4)
#define T1CASE_RES_CURSOR_END_INDEX   (T1CASE_RES_INPUT_BYTES_INDEX + 4)

#define T1CASE_RES_RECORD_COUNT_INDEX T1CASE_RES_CURSOR_END_INDEX

// TH03 triple-aliases one 4-byte slot as the recorder's committed guard size
// and the player's checkpoint/stage index (th03/replay_handoff.hpp:11-16),
// because record and playback never both need it. Keep the union and its
// documentation; TH01 uses only the record half so far, and step 4 (checkpoints)
// is what fills the other.
#define T1CASE_RES_UNION_INDEX    (T1CASE_RES_RECORD_COUNT_INDEX + 4)
#define T1CASE_RES_COMMITTED_INDEX T1CASE_RES_UNION_INDEX
#define T1CASE_RES_CHECKPOINT_INDEX T1CASE_RES_UNION_INDEX

#define T1CASE_RES_CHECKSUM_INDEX (T1CASE_RES_UNION_INDEX + 4)
#define T1CASE_RES_SPLIT_ROWS_INDEX (T1CASE_RES_CHECKSUM_INDEX + 4)

// The running FNV-1a over every checkpoint slot this RUN has written. Kept in
// the carrier rather than recomputed, because a recording appends one slot per
// process and re-reading the whole array at every handoff to extend a hash is
// work proportional to the run length for no gain.
#define T1CASE_RES_CKPT_SUM_INDEX (T1CASE_RES_SPLIT_ROWS_INDEX + 4)

// Reserved for the savestate-protect detector (REPLAY_CORE_CONTRACT.md §6),
// which is W3.1 step 3. §6.1 records "~45 bytes of scratch that survives process
// transitions" as the ONE thing TH01's protect binding has nowhere to put, and
// naming the region now is what makes the non-overlap guards below able to
// prove step 3 does not collide with the cursors. No field inside it is defined
// yet: it is a reservation, not a set of dead declarations.
#define T1CASE_RES_PROTECT_INDEX (T1CASE_RES_CKPT_SUM_INDEX + 4)
#define T1CASE_RES_PROTECT_SIZE  45
#define T1CASE_RES_END_INDEX (T1CASE_RES_PROTECT_INDEX + T1CASE_RES_PROTECT_SIZE)

// The protect region's interior (W3.1 step 3), as offsets WITHIN `protect[]`.
// The committed size is deliberately NOT here: it reuses the §7 union at
// T1CASE_RES_UNION_INDEX, because duplicating it would make two copies of the
// one value the entire detection compares (state/notes/t1case-protect.md §0b).
// The sector buffer is deliberately NOT here either: it must not survive an
// `execl`, and TH01 self-`execl`s 8-10 times per run.
#define T1PRT_FLAGS_INDEX        0
#define T1PRT_GUARD_SECTOR_INDEX 1
#define T1PRT_GUARD_OFFSET_INDEX 5

// Diagnostics. Everything from here on is evidence, never control flow.
#define T1PRT_DIAG_INDEX             7
#define T1PRT_DIAG_CODE_INDEX        T1PRT_DIAG_INDEX
#define T1PRT_DIAG_DRIVE_INDEX       8
#define T1PRT_DIAG_DOS_AX_INDEX      9
#define T1PRT_DIAG_BPS_INDEX        11
#define T1PRT_DIAG_ROOT_ENTS_INDEX  13
#define T1PRT_DIAG_ROOT_START_INDEX 15
#define T1PRT_DIAG_ROOT_SECS_INDEX  19
#define T1PRT_DIAG_SECTOR_INDEX     21
#define T1PRT_DIAG_OFFSET_INDEX     25
#define T1PRT_DIAG_EXPECTED_INDEX   27
#define T1PRT_DIAG_ACTUAL_INDEX     31
#define T1PRT_DIAG_I25_FLAGS_INDEX  35
#define T1PRT_DIAG_I25_STACK_INDEX  37
#define T1PRT_DIAG_END_INDEX        39
#define T1PRT_PROTECT_END_INDEX     T1PRT_DIAG_END_INDEX

// Sticky flag bits.
#define T1PRT_FLAG_INVALID 0x01
#define T1PRT_FLAG_LOCATED 0x02
#define T1PRT_FLAG_ERROR   0x04

// One boundary guard per region, exactly as §7.5 requires of the outer map.
#if (T1PRT_GUARD_SECTOR_INDEX < (T1PRT_FLAGS_INDEX + 1))
#error T1CASE protect: the guard sector overlaps the flags byte
#endif
#if (T1PRT_GUARD_OFFSET_INDEX < (T1PRT_GUARD_SECTOR_INDEX + 4))
#error T1CASE protect: the guard offset overlaps the guard sector
#endif
#if (T1PRT_DIAG_INDEX < (T1PRT_GUARD_OFFSET_INDEX + 2))
#error T1CASE protect: the diagnostics overlap the cached location
#endif
#if (T1PRT_DIAG_DRIVE_INDEX < (T1PRT_DIAG_CODE_INDEX + 1))
#error T1CASE protect: the diagnostic drive overlaps the diagnostic code
#endif
#if (T1PRT_DIAG_DOS_AX_INDEX < (T1PRT_DIAG_DRIVE_INDEX + 1))
#error T1CASE protect: the DOS AX diagnostic overlaps the drive
#endif
#if (T1PRT_DIAG_BPS_INDEX < (T1PRT_DIAG_DOS_AX_INDEX + 2))
#error T1CASE protect: the bytes-per-sector diagnostic overlaps DOS AX
#endif
#if (T1PRT_DIAG_ROOT_ENTS_INDEX < (T1PRT_DIAG_BPS_INDEX + 2))
#error T1CASE protect: the root entry count overlaps bytes-per-sector
#endif
#if (T1PRT_DIAG_ROOT_START_INDEX < (T1PRT_DIAG_ROOT_ENTS_INDEX + 2))
#error T1CASE protect: the root start overlaps the root entry count
#endif
#if (T1PRT_DIAG_ROOT_SECS_INDEX < (T1PRT_DIAG_ROOT_START_INDEX + 4))
#error T1CASE protect: the root sector count overlaps the root start
#endif
#if (T1PRT_DIAG_SECTOR_INDEX < (T1PRT_DIAG_ROOT_SECS_INDEX + 2))
#error T1CASE protect: the located sector overlaps the root sector count
#endif
#if (T1PRT_DIAG_OFFSET_INDEX < (T1PRT_DIAG_SECTOR_INDEX + 4))
#error T1CASE protect: the located offset overlaps the located sector
#endif
#if (T1PRT_DIAG_EXPECTED_INDEX < (T1PRT_DIAG_OFFSET_INDEX + 2))
#error T1CASE protect: the expected size overlaps the located offset
#endif
#if (T1PRT_DIAG_ACTUAL_INDEX < (T1PRT_DIAG_EXPECTED_INDEX + 4))
#error T1CASE protect: the actual size overlaps the expected size
#endif
#if (T1PRT_DIAG_I25_FLAGS_INDEX < (T1PRT_DIAG_ACTUAL_INDEX + 4))
#error T1CASE protect: the INT 25h flags overlap the actual size
#endif
#if (T1PRT_DIAG_I25_STACK_INDEX < (T1PRT_DIAG_I25_FLAGS_INDEX + 2))
#error T1CASE protect: the INT 25h stack word overlaps the INT 25h flags
#endif
#if (T1PRT_DIAG_END_INDEX < (T1PRT_DIAG_I25_STACK_INDEX + 2))
#error T1CASE protect: the region end overlaps the INT 25h stack word
#endif
#if (T1PRT_PROTECT_END_INDEX > T1CASE_RES_PROTECT_SIZE)
#error T1CASE protect: the interior map overflows the reserved region
#endif

// The block master.lib is asked to allocate. Sized from the map, never the
// other way around.
#define T1CASE_RES_SIZE T1CASE_RES_END_INDEX

/// Compile-time non-overlap assertions, one per region.
/// TH03 asserts only that its last region fits (`#if
/// (T3_REPLAY_RES_TIMING_END_INDEX > 198) #error`) plus one hardcoded tripwire
/// on a single boundary (`T3_KEYCONFIG_RES_END_INDEX != 98`). That catches an
/// overflow and one specific collision; it does NOT catch two interior regions
/// growing into each other, which is the failure the guards are for. Assert
/// every boundary.

#if (T1CASE_RES_MAGIC_INDEX < T1CASE_RES_ID_SIZE)
#error T1CASE carrier: the magic overlaps the ResData ID
#endif
#if (T1CASE_RES_VERSION_INDEX < (T1CASE_RES_MAGIC_INDEX + T1CASE_RES_MAGIC_SIZE))
#error T1CASE carrier: the version byte overlaps the magic
#endif
#if (T1CASE_RES_MODE_INDEX < (T1CASE_RES_VERSION_INDEX + 1))
#error T1CASE carrier: mode overlaps the version byte
#endif
#if (T1CASE_RES_SLOT_INDEX < (T1CASE_RES_MODE_INDEX + 1))
#error T1CASE carrier: slot overlaps mode
#endif
#if (T1CASE_RES_PROCESS_INDEX < (T1CASE_RES_SLOT_INDEX + 1))
#error T1CASE carrier: the process id overlaps slot
#endif
#if (T1CASE_RES_SEQ_INDEX < (T1CASE_RES_PROCESS_INDEX + 1))
#error T1CASE carrier: the process sequence overlaps the process id
#endif
#if (T1CASE_RES_FLAGS_INDEX < (T1CASE_RES_SEQ_INDEX + 2))
#error T1CASE carrier: flags overlap the process sequence
#endif
#if (T1CASE_RES_CURSOR_INDEX < (T1CASE_RES_FLAGS_INDEX + 2))
#error T1CASE carrier: the cursor region overlaps flags
#endif
#if (T1CASE_RES_CURSOR_END_INDEX != (T1CASE_RES_CURSOR_INDEX + 12))
#error T1CASE carrier: the cursor region is not the three 32-bit cursors
#endif
#if (T1CASE_RES_RECORD_COUNT_INDEX < T1CASE_RES_CURSOR_END_INDEX)
#error T1CASE carrier: the record count overlaps the cursor region
#endif
#if (T1CASE_RES_UNION_INDEX < (T1CASE_RES_RECORD_COUNT_INDEX + 4))
#error T1CASE carrier: the committed/checkpoint union overlaps the record count
#endif
#if (T1CASE_RES_CHECKSUM_INDEX < (T1CASE_RES_UNION_INDEX + 4))
#error T1CASE carrier: the payload checksum overlaps the union
#endif
#if (T1CASE_RES_SPLIT_ROWS_INDEX < (T1CASE_RES_CHECKSUM_INDEX + 4))
#error T1CASE carrier: the split row count overlaps the payload checksum
#endif
#if (T1CASE_RES_CKPT_SUM_INDEX < (T1CASE_RES_SPLIT_ROWS_INDEX + 4))
#error T1CASE carrier: the checkpoint checksum overlaps the split row count
#endif
#if (T1CASE_RES_PROTECT_INDEX < (T1CASE_RES_CKPT_SUM_INDEX + 4))
#error T1CASE carrier: the protect region overlaps the checkpoint checksum
#endif
#if (T1CASE_RES_END_INDEX > T1CASE_RES_SIZE)
#error T1CASE carrier: the region map overflows the block
#endif

// `slot`. The oracle lineage has no numbered slots — TXCASE_CONTRACT.md's
// control surface is a single fixed T1CASE.BIN — so TH01 stores NONE and
// VALIDATES it, rather than leaving the byte undefined. The user lineage's
// 0..99 range (REPLAY_CORE_CONTRACT.md §8) is what the field exists for.
#define T1CASE_SLOT_NONE 0xFF

// Carrier flags.
//
// CONTROL_PENDING is §7.2's latch. TH03 keeps it at
// `T3_REPLAY_RES_MAINL_CONTROL_INDEX` and MAINL uses it to HOLD THE LAST INPUT
// STEADY until the interpreter reaches its natural exit. TH01 latches the same
// condition and does the opposite with it — see t1case_frame_io() for why an
// early control packet is a desync here and must not be waited out.
//
// DONE marks a case that has ENDED. Before this parcel the end state was
// encoded by destroying the block (`id[0] = '\0'`), which makes the very next
// process fall through to T1CASE.CFG and start the case over from record zero —
// exactly the failure §7's carrier-first precedence exists to prevent, reached
// from the other direction.
#define T1CASE_RES_FLAG_STARTED         0x0001
#define T1CASE_RES_FLAG_CONTROL_PENDING 0x0002
#define T1CASE_RES_FLAG_DONE            0x0004

struct t1case_res_t {
	// `resdata_create()` writes the ID string to offset 0 of the new block
	// (libs/master.lib/resdata.asm, RSDCREATE_ALLOC_OK: `xor DI,DI` /
	// `rep movsb`), which is why `resident_t`'s first member is its own id too.
	// Clearing it makes the very next `resdata_exist()` fail to match.
	char id[T1CASE_RES_ID_SIZE];

	char magic[T1CASE_RES_MAGIC_SIZE];
	uint8_t carrier_version;
	uint8_t mode;
	uint8_t slot;

	// The process that last stored this carrier. Always REIIDEN today, because
	// it is the only instrumented binary; the field is what lets a future FUUIN
	// or OP binding detect "resumed in the wrong binary" from the CARRIER, the
	// way the control packet's second byte detects it from the STREAM.
	uint8_t process_id;

	uint16_t process_seq;
	uint16_t flags;

	uint32_t sample_count;
	uint32_t global_frame;

	// The third cursor of REPLAY_CORE_CONTRACT.md §4.4: where in the packet
	// stream the next process resumes. Sound only because the record side
	// guarantees a process boundary lands on a packet boundary — t1case_finish()
	// emits a control packet, which closes the open packet by construction.
	uint32_t input_byte_count;

	uint32_t record_count;
	uint32_t committed; // the §7 union; see T1CASE_RES_UNION_INDEX
	uint32_t payload_checksum;
	uint32_t split_rows;
	uint32_t checkpoint_checksum;
	uint8_t protect[T1CASE_RES_PROTECT_SIZE];
};

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
// The checkpoint slot binds to the same kind of map the carrier does, so it
// gets the same kind of proof: the prefix boundary and the total stride, not
// just the total. A slot whose fields are all the right width can still be
// laid out wrong, and a stride check alone passes that.
typedef char t1case_checkpoint_prefix_check[
	(offsetof(t1case_checkpoint_t, startup) == T1CASE_CHECKPOINT_PREFIX_SIZE) ?
	1 : -1
];
typedef char t1case_checkpoint_stride_check[
	(sizeof(t1case_checkpoint_t) == T1CASE_CHECKPOINT_STRIDE) ? 1 : -1
];
// The three fields that replaced `case_id` must land exactly on its four
// bytes, or every v2-era offset in the host reader moves.
typedef char t1case_header_ckpt_sum_offset_check[
	(offsetof(t1case_header_t, checkpoint_checksum) == 44) ? 1 : -1
];
typedef char t1case_header_ckpt_cap_offset_check[
	(offsetof(t1case_header_t, checkpoint_capacity) == 40) ? 1 : -1
];
typedef char t1case_header_ckpt_count_offset_check[
	(offsetof(t1case_header_t, checkpoint_count) == 41) ? 1 : -1
];
typedef char t1case_header_ckpt_stride_offset_check[
	(offsetof(t1case_header_t, checkpoint_stride) == 42) ? 1 : -1
];
// The capacity is a u8 in the header.
typedef char t1case_checkpoint_cap_check[
	((T1CASE_CHECKPOINT_CAP > 0) && (T1CASE_CHECKPOINT_CAP <= 255)) ? 1 : -1
];
// The codec's invariants. TH03 leaves every one of these unchecked
// (state/notes/t1case-packet-rle.md), and each is silent when violated.
typedef char t1case_packet_run_check[
	(T1CASE_PACKET_RUN_MAX == (T1CASE_PACKET_RUN_MASK + 1)) ? 1 : -1
];
typedef char t1case_packet_phase_shift_check[
	((1 << T1CASE_PACKET_PHASE_SHIFT) == (T1CASE_PACKET_RUN_MASK + 1)) ? 1 : -1
];
typedef char t1case_packet_mask_check[
	(T1CASE_PACKET_KEYFRAME_MASK == 0x7F) ? 1 : -1
];
typedef char t1case_wbuf_size_check[
	(T1CASE_WBUF_SIZE >= (T1CASE_PACKET_SIZE_MAX * T1SPLIT_INTERVAL_SAMPLES)) ?
	1 : -1
];
// The cadence test is `& (N - 1)`, so N must be a power of two.
typedef char t1split_interval_pot_check[
	((T1SPLIT_INTERVAL_SAMPLES &
		(T1SPLIT_INTERVAL_SAMPLES - 1)) == 0) ? 1 : -1
];
typedef char t1case_guard_interval_pot_check[
	((T1CASE_GUARD_INTERVAL_SAMPLES &
		(T1CASE_GUARD_INTERVAL_SAMPLES - 1)) == 0) ? 1 : -1
];

// The handoff carrier's struct binding against the core region map. Every
// offset is asserted, not just the total size: a struct whose fields are all
// the right width can still bind to the wrong map if the compiler pads
// differently than the map assumes, and a size check alone would pass.
typedef char t1case_res_id_offset_check[
	(offsetof(t1case_res_t, magic) == T1CASE_RES_MAGIC_INDEX) ? 1 : -1
];
typedef char t1case_res_version_offset_check[
	(offsetof(t1case_res_t, carrier_version) == T1CASE_RES_VERSION_INDEX) ? 1 : -1
];
typedef char t1case_res_mode_offset_check[
	(offsetof(t1case_res_t, mode) == T1CASE_RES_MODE_INDEX) ? 1 : -1
];
typedef char t1case_res_slot_offset_check[
	(offsetof(t1case_res_t, slot) == T1CASE_RES_SLOT_INDEX) ? 1 : -1
];
typedef char t1case_res_process_offset_check[
	(offsetof(t1case_res_t, process_id) == T1CASE_RES_PROCESS_INDEX) ? 1 : -1
];
typedef char t1case_res_seq_offset_check[
	(offsetof(t1case_res_t, process_seq) == T1CASE_RES_SEQ_INDEX) ? 1 : -1
];
typedef char t1case_res_flags_offset_check[
	(offsetof(t1case_res_t, flags) == T1CASE_RES_FLAGS_INDEX) ? 1 : -1
];
typedef char t1case_res_sample_offset_check[
	(offsetof(t1case_res_t, sample_count) == T1CASE_RES_SAMPLE_COUNT_INDEX) ?
	1 : -1
];
typedef char t1case_res_frame_offset_check[
	(offsetof(t1case_res_t, global_frame) == T1CASE_RES_GLOBAL_FRAME_INDEX) ?
	1 : -1
];
typedef char t1case_res_bytes_offset_check[
	(offsetof(t1case_res_t, input_byte_count) == T1CASE_RES_INPUT_BYTES_INDEX) ?
	1 : -1
];
typedef char t1case_res_records_offset_check[
	(offsetof(t1case_res_t, record_count) == T1CASE_RES_RECORD_COUNT_INDEX) ?
	1 : -1
];
typedef char t1case_res_union_offset_check[
	(offsetof(t1case_res_t, committed) == T1CASE_RES_UNION_INDEX) ? 1 : -1
];
typedef char t1case_res_checksum_offset_check[
	(offsetof(t1case_res_t, payload_checksum) == T1CASE_RES_CHECKSUM_INDEX) ?
	1 : -1
];
typedef char t1case_res_rows_offset_check[
	(offsetof(t1case_res_t, split_rows) == T1CASE_RES_SPLIT_ROWS_INDEX) ? 1 : -1
];
typedef char t1case_res_ckpt_sum_offset_check[
	(offsetof(t1case_res_t, checkpoint_checksum) == T1CASE_RES_CKPT_SUM_INDEX) ?
	1 : -1
];
typedef char t1case_res_protect_offset_check[
	(offsetof(t1case_res_t, protect) == T1CASE_RES_PROTECT_INDEX) ? 1 : -1
];
typedef char t1case_res_size_check[
	(sizeof(t1case_res_t) == T1CASE_RES_SIZE) ? 1 : -1
];
typedef char t1case_res_id_size_check[
	(sizeof(T1CASE_RES_ID) == T1CASE_RES_ID_SIZE) ? 1 : -1
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

// True once a case is being recorded or played back in this process.
bool16 far t1case_active(void);

// Post-init verify plus the `start` / `round_start` trace rows.
void far t1case_round_start(void);

// Process handoff. [terminal] marks the case's last process.
void far t1case_finish(bool16 terminal);

// Appends one fixed-width line to T1DIAG.TXT. [t0..t2] is a three-character
// tag. Exposed so lifecycle sites outside this module can record a milestone
// that a later process would otherwise be unable to distinguish from "never
// reached".
void far t1case_diag_note(char t0, char t1, char t2, uint32_t a, uint32_t b);

#if defined(__cplusplus)
}
#endif

#endif /* TH01_T1CASE_HPP */
