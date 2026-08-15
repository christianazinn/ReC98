/* ReC98 — harness/ORACLE-TH01-MASTER (MOD BRANCH, NOT byte-identical)
 * -------------------------------------------------------------------
 * TH01 oracle case recorder/player and T1SPLIT trace writer, for REIIDEN.EXE.
 *
 * Ported from `th03/main/t3case.cpp` on harness/TH03-ORACLE-MASTER. Structure,
 * naming and idioms follow that module deliberately; the justified divergences
 * are:
 *
 *  1. The cross-process carrier is a SECOND resident block under its own ID
 *     rather than a scratch array inside resident_t. TH03 could use
 *     `resident->unused_3[198]`; TH01's resident_t (th01/resident.hpp:42-72)
 *     has only two single spare bytes, so there is nowhere to put 24 bytes of
 *     cursor. A separate ResData block leaves resident_t's layout completely
 *     untouched, and `resident_free()` (th01/core/resstuff.cpp:67-73) only
 *     frees RES_ID, so it cannot collect ours by accident.
 *  2. The injection seam is `key_sense()`, redirected at its CALL SITES rather
 *     than at the symbol, and not an `fp_*` callback slot. Count them
 *     carefully: 14 redirected invocations in 3 lexical blocks covering 7
 *     groups (th01/main_01.cpp:204-211, th01/hardware/input.hpp:102-105,
 *     th01/main_01.cpp:250-251), not the "six" an earlier revision of this
 *     comment claimed — that six was the six groups on the unconditional path.
 *     See state/notes/th01-input-injection.md §9. TH01 has no such slot, and overwriting the
 *     output booleans would desynchronize input_sense()'s function-local
 *     `static uint8_t input_prev[16]` and make [input_bomb] — which is derived
 *     from double-tap history, not from a key — unreproducible.
 *  3. The cursor advances once per `input_sense(false)` call, not per game
 *     frame. TH01 increments [frame_rand] only in the main gameplay loop
 *     (th01/main_01.cpp:803), while at least six interstitial loops call
 *     input_sense() without touching it; keying on [frame_rand] desynchronizes
 *     at the very first one (th01/main_01.cpp:781-791).
 *  4. The 64-bit two-pass subsystem hash of TXSPLIT_CONTRACT.md §7 replaces
 *     T3SPLT1's single 32-bit DJB2 aggregate, so a divergence names the
 *     subsystem that diverged.
 *  6. Checkpoints are per PROCESS SEGMENT, not per round. TH01's stage-clear
 *     `execl` is guarded, so three of every five stage starts are mid-process
 *     and are NOT restorable at any payload size; see th01/t1case.hpp and
 *     state/notes/t1case-checkpoint.md.
 *  5. The packet encoder keeps `packet_open` and the delta basis as two
 *     separate flags. The reference has one and therefore emits a full
 *     keyframe at every disk flush; see the comment on [t1case_packet_open].
 *
 * Case version 2 carries the shared replay core's N-channel packet RLE
 * (state/port/REPLAY_CORE_CONTRACT.md §4) in place of version 1's fixed 16-byte
 * records. Same logical sample sequence, ~480x fewer bytes on the Gate A
 * corpus.
 *
 * None of the statics below are initialized data. A `_DATA` contribution from
 * this module would land between the original `_DATA` and `_BSS` inside
 * DGROUP. TH01 is 100% position-independent and a raw-offset survey of `th01/`
 * finds no hardcoded addresses at all, so this is not load-bearing here the
 * way it is for TH03 — but the discipline is free, so keep it.
 * See kb/conventions/th03-mod-layout-verification.md.
 */

#pragma option -zCT1CASE_TEXT

#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <dos.h>    // int86(), MK_FP(), the _AX/_CX/_DX pseudo-registers
#include <alloc.h>  // farmalloc(); see TH01_ORACLE_DELTA_INDEX.md D4
#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "th01/t1case.hpp"
#include "th01/t1protect.hpp"
#include "th01/core/resstuff.hpp"
#include "th01/formats/cfg.hpp"
#include "th01/resident.hpp"
#include "th01/hardware/input.hpp"
#include "th01/main/debug.hpp"
#include "th01/main/extend.hpp"
#include "th01/main/entity.hpp"
#include "th01/main/boss/boss.hpp"
#include "th01/main/bullet/pellet.hpp"
#include "th01/main/hud/hp.hpp"
#include "th01/main/player/bomb.hpp"
#include "th01/main/player/orb.hpp"
#include "th01/main/player/player.hpp"
#include "th01/main/player/shot.hpp"
#include "th01/main/stage/stages.hpp"

// File-scope globals that TH01 never declared in a header.
extern int8_t boss_id;      // th01/main_01.cpp:428
extern uscore_t score_bonus; // th01/main_01.cpp:83
extern uint32_t frame_since_start_of_binary; // th01/main_01.cpp:97
extern bool timer_initialized; // th01/main_01.cpp:85

// [measured] th01/resident.hpp:92,109,111 declare `end_flag`,
// `continues_per_scene` and `score_highest` unconditionally, but REIIDEN.EXE
// never defines them: all three are FUUIN-only file-scope copies
// (th01/fuuin_01.cpp:23,25,31). REIIDEN reads those three values through
// `resident->` directly, so this module must too. This corrects
// state/re/DETERMINISTIC_STATE_TH01.md §2, which states that the redundant
// copies rather than the resident fields are what gameplay reads — true for
// the other nine copies, false for these three.

/// Cross-process carrier
/// ---------------------
/// The layout, the region map and its compile-time proofs live in
/// th01/t1case.hpp next to the wire formats, because the carrier IS a format —
/// it is read by a different process image than the one that wrote it.

enum t1case_mode_t {
	T1CASE_DISABLED = 0,
	T1CASE_RECORD   = 1,
	T1CASE_PLAYBACK = 2,
	T1CASE_ERROR    = 3,

	// Not a recording mode: `T1CASE.CFG` = "g" runs the savestate detector's
	// verdict selftest and exits. Shipped rather than debug-only, because it is
	// the negative control the human runs before trusting a savestate result.
	T1CASE_SELFTEST = 4
};

enum t1case_text_id_t {
	T1T_OK_RECORD = 0,
	T1T_OK_PLAYBACK,
	T1T_OK_SELFTEST,

	// The ok/error split in t1case_write_text() is `id <= T1T_OK_INPUT_END`,
	// so every success code must sort at or before this one. Adding a success
	// after it silently reports "error:".
	T1T_OK_INPUT_END,
	T1T_ERR_CASE_HEADER,
	T1T_ERR_CASE_CREATE,
	T1T_ERR_FRAME_IO,
	T1T_ERR_DESYNC,
	T1T_ERR_SPLIT_OPEN,
	T1T_ERR_VERIFY,

	// The carrier disagrees with the case file it says it is in the middle of.
	// Before this parcel the equivalent evidence existed only as the HDR/FIN
	// pair in T1DIAG.TXT, i.e. as something a human read afterwards.
	T1T_ERR_HANDOFF,

	// §7.2's latch, both directions. The stream said the process segment was
	// over while the game was still asking for samples (EARLY), or the game
	// reached its process boundary while the stream still owed samples (LATE).
	// Both were `error:desync` before.
	T1T_ERR_CONTROL_EARLY,
	T1T_ERR_CONTROL_LATE,

	// The control packet's second byte names the process that EMITTED the
	// segment, and it is not the process consuming it. Distinct from `desync`
	// on purpose: REPLAY_CORE_CONTRACT.md 7.2 adopted the process id in place of
	// TH03's control-code parity precisely so that "resumed in the wrong binary"
	// is detected directly rather than inferred, and a check that reports the
	// generic symptom throws that away.
	T1T_ERR_PROCESS,

	// A checkpoint could not be captured or could not be restored. Kept
	// apart from `header` and `desync` because all three are reachable at
	// session start and only this one means the RESUME POINT is wrong.
	T1T_ERR_CHECKPOINT,

	T1T_ERR_RESIDENT
};

/// State (all BSS)
/// ---------------

static char T1CASE_CFG_FN[11];
static char T1CASE_BIN_FN[11];
static char T1CASE_SPLIT_FN[12];
static char T1CASE_DONE_FN[11];
static char T1CASE_DIAG_FN[11];
static char T1CASE_GUARD_FN[13];

// The two ResData IDs. Assembled at runtime for the same reason as the
// filenames: a string literal is initialized data, and this module must
// contribute none.
static char T1CASE_RES_ID_BUF[sizeof(T1CASE_RES_ID)];
static char T1CASE_RESIDENT_ID_BUF[sizeof(RES_ID)];

static bool t1case_paths_ready;

static t1case_header_t t1case_header;
static t1case_startup_t t1case_startup;
static t1case_res_t far *t1case_res;

static uint8_t t1case_mode;
static bool t1case_started;

// Checkpoints (REPLAY_CORE_CONTRACT.md 5). One scratch slot, reused by the
// capture, the restore and the array reservation - never more than one is in
// flight, and 80 bytes of BSS is cheaper than 80 bytes of stack in a module
// every call of which is reached from deep inside REIIDEN's own frames.
static t1case_checkpoint_t t1case_ckpt;

// Which checkpoint this playback was started from, and whether the restored
// startup block still owes its verify. The verify is deliberately NOT folded
// into [t1case_started]: a checkpoint start is `started` from the first
// instant, and the plain `!started` test would therefore skip the one check
// that looks at state which was RESTORED rather than played into.
static uint8_t t1case_ckpt_index;
static bool t1case_ckpt_verify_pending;

// The running FNV-1a over every slot this RUN has written, mirrored into the
// carrier so it survives the handoff and into the header so a reader can check
// the array it is about to trust.
static uint32_t t1case_checkpoint_checksum;

// The checkpoint index T1CASE.CFG asked for, e.g. "p3".
static uint8_t t1case_cfg_checkpoint;
static bool t1case_done_written;
static uint32_t t1case_sample_count;
static uint32_t t1case_record_count;
static uint32_t t1case_global_frame;
static uint32_t t1case_payload_checksum;
static uint32_t t1case_split_rows;

// The seven group bytes latched for the current input_sense() pass, and a
// pointer to input_sense()'s function-local [input_prev]. On playback the latch
// doubles as the decoder's channel state: an unchanged channel simply keeps the
// value the previous packet left there.
static uint8_t t1case_keys[T1CASE_GROUP_COUNT];
static uint8_t near *t1case_input_prev;

/// Packet RLE state
/// ----------------
/// REPLAY_CORE_CONTRACT.md §4. One shadow-state encoding, not TH03's two
/// (§1 item 1: MAIN packs `packet_size`/`charge`/`open` across two bitfield
/// bytes, MAINL packs the same information differently for the same wire
/// format). TH01 has one recorder, so plain statics — this module contributes
/// no initialized data either way, and clarity is worth more than four bytes.

// Total encoded bytes, INCLUDING bytes still sitting in [t1case_wbuf].
// Mirrors TH03's `replay_input_byte_count`, which is likewise advanced at
// buffer time so a flush writes at `offset + count - buffered`.
static uint32_t t1case_input_byte_count;

static uint8_t t1case_wbuf[T1CASE_WBUF_SIZE];
static uint16_t t1case_wbuf_len;

// The encoder's two flags, and the ONE place this port deliberately does not
// follow the reference. TH03 keeps a single packet-open bit and treats it as
// both "may I extend the buffered tag?" and "have I got a delta basis?"
// (th03/main/replay.cpp:3736 sets every change bit whenever the bit is clear),
// so its every disk flush emits a full keyframe. A keyframe is only REQUIRED
// where a reader may BEGIN reading — for TH01 that is the process-segment
// start, which the control packet already marks. Conflating the two costs
// +8,750 bytes on an 80,000-sample case. See state/notes/t1case-packet-rle.md.
static bool t1case_packet_open;   // cleared at every flush
static bool t1case_enc_prev_valid; // cleared only at a process-segment boundary

static uint8_t t1case_enc_prev[T1CASE_GROUP_COUNT];
static uint8_t t1case_packet_run;   // 1..T1CASE_PACKET_RUN_MAX
static uint8_t t1case_packet_phase;

// Offset of the open packet's tag byte within [t1case_wbuf]. TH03 instead
// caches the open packet's byte length in four bits of its shadow state and
// walks back with `write_buffer[size - packet_size]`
// (th03/main/replay.cpp:3731), which silently corrupts if a packet ever exceeds
// 15 bytes. Storing the offset has the same effect with no such ceiling.
static uint16_t t1case_packet_at;

static uint8_t t1case_rbuf[T1CASE_RBUF_SIZE];
static uint16_t t1case_rbuf_len;
static uint16_t t1case_rbuf_pos;
static uint8_t t1case_dec_run;    // samples left in the decoded packet
static uint8_t t1case_dec_phase;
static bool t1case_dec_prev_valid;

// Distinguishes a failed read from a stream that decoded into something
// invalid, so `error:frame-io` and `error:desync` stay meaningful.
static bool t1case_stream_io_error;

// REPLAY_CORE_CONTRACT.md §7.2's control-pending latch: the decoder has met a
// control packet, i.e. the stream says this process segment is over.
//
// [measured] The reference keeps the same condition in its CARRIER
// (`replay_control_pending`, th03/mainl/replml.cpp:26-28) — but
// `mainl_replay_session_start()` (:1284) sets it false unconditionally, so it
// never crosses a process transition and is not handoff state at all. It is in
// the resident because MAINL's BSS layout is pinned by the accel path's bulk
// image (T3R_ACCEL_BSS_OFFSET/END), and adding a BSS byte there would move it.
// TH01 has no accel layer, so the latch is a plain static; the carrier flag of
// the same name is written from it so a post-mortem of a killed run can see it,
// and is likewise cleared at every session start.
static bool t1case_control_pending;

// Set when a control packet named a process id other than this binary's. Kept
// apart from the I/O and control-pending flags so all three stream failures name
// themselves.
static bool t1case_process_mismatch;

/// Small helpers
/// -------------

static void t1case_memclear(void far *buf, unsigned size)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(buf);

	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

// Assembled at runtime; see the module comment on initialized data.
static void t1case_paths_init(void)
{
	if(t1case_paths_ready) {
		return;
	}
	T1CASE_CFG_FN[0] = 'T';
	T1CASE_CFG_FN[1] = '1';
	T1CASE_CFG_FN[2] = 'C';
	T1CASE_CFG_FN[3] = 'A';
	T1CASE_CFG_FN[4] = 'S';
	T1CASE_CFG_FN[5] = 'E';
	T1CASE_CFG_FN[6] = '.';
	T1CASE_CFG_FN[7] = 'C';
	T1CASE_CFG_FN[8] = 'F';
	T1CASE_CFG_FN[9] = 'G';
	T1CASE_CFG_FN[10] = '\0';

	T1CASE_BIN_FN[0] = 'T';
	T1CASE_BIN_FN[1] = '1';
	T1CASE_BIN_FN[2] = 'C';
	T1CASE_BIN_FN[3] = 'A';
	T1CASE_BIN_FN[4] = 'S';
	T1CASE_BIN_FN[5] = 'E';
	T1CASE_BIN_FN[6] = '.';
	T1CASE_BIN_FN[7] = 'B';
	T1CASE_BIN_FN[8] = 'I';
	T1CASE_BIN_FN[9] = 'N';
	T1CASE_BIN_FN[10] = '\0';

	T1CASE_SPLIT_FN[0] = 'T';
	T1CASE_SPLIT_FN[1] = '1';
	T1CASE_SPLIT_FN[2] = 'S';
	T1CASE_SPLIT_FN[3] = 'P';
	T1CASE_SPLIT_FN[4] = 'L';
	T1CASE_SPLIT_FN[5] = 'I';
	T1CASE_SPLIT_FN[6] = 'T';
	T1CASE_SPLIT_FN[7] = '.';
	T1CASE_SPLIT_FN[8] = 'B';
	T1CASE_SPLIT_FN[9] = 'I';
	T1CASE_SPLIT_FN[10] = 'N';
	T1CASE_SPLIT_FN[11] = '\0';

	T1CASE_DONE_FN[0] = 'T';
	T1CASE_DONE_FN[1] = '1';
	T1CASE_DONE_FN[2] = 'D';
	T1CASE_DONE_FN[3] = 'O';
	T1CASE_DONE_FN[4] = 'N';
	T1CASE_DONE_FN[5] = 'E';
	T1CASE_DONE_FN[6] = '.';
	T1CASE_DONE_FN[7] = 'T';
	T1CASE_DONE_FN[8] = 'X';
	T1CASE_DONE_FN[9] = 'T';
	T1CASE_DONE_FN[10] = '\0';

	T1CASE_DIAG_FN[0] = 'T';
	T1CASE_DIAG_FN[1] = '1';
	T1CASE_DIAG_FN[2] = 'D';
	T1CASE_DIAG_FN[3] = 'I';
	T1CASE_DIAG_FN[4] = 'A';
	T1CASE_DIAG_FN[5] = 'G';
	T1CASE_DIAG_FN[6] = '.';
	T1CASE_DIAG_FN[7] = 'T';
	T1CASE_DIAG_FN[8] = 'X';
	T1CASE_DIAG_FN[9] = 'T';
	T1CASE_DIAG_FN[10] = '\0';

	// "\T1LAST.GRD" — ROOT-RELATIVE, and that is load-bearing rather than
	// cosmetic. The detector only ever scans the ROOT directory and matches on
	// the basename, so a CWD-relative name would find a same-named file in the
	// root and read ITS size instead. Every other file this module opens is
	// CWD-relative; this one must not be.
	// See state/notes/t1case-protect.md §10.1.
	T1CASE_GUARD_FN[0] = '\\';
	T1CASE_GUARD_FN[1] = 'T';
	T1CASE_GUARD_FN[2] = '1';
	T1CASE_GUARD_FN[3] = 'L';
	T1CASE_GUARD_FN[4] = 'A';
	T1CASE_GUARD_FN[5] = 'S';
	T1CASE_GUARD_FN[6] = 'T';
	T1CASE_GUARD_FN[7] = '.';
	T1CASE_GUARD_FN[8] = 'G';
	T1CASE_GUARD_FN[9] = 'R';
	T1CASE_GUARD_FN[10] = 'D';
	T1CASE_GUARD_FN[11] = '\0';

	// "T1CaseState"
	T1CASE_RES_ID_BUF[0] = 'T';
	T1CASE_RES_ID_BUF[1] = '1';
	T1CASE_RES_ID_BUF[2] = 'C';
	T1CASE_RES_ID_BUF[3] = 'a';
	T1CASE_RES_ID_BUF[4] = 's';
	T1CASE_RES_ID_BUF[5] = 'e';
	T1CASE_RES_ID_BUF[6] = 'S';
	T1CASE_RES_ID_BUF[7] = 't';
	T1CASE_RES_ID_BUF[8] = 'a';
	T1CASE_RES_ID_BUF[9] = 't';
	T1CASE_RES_ID_BUF[10] = 'e';
	T1CASE_RES_ID_BUF[11] = '\0';

	// "ReiidenConfig" — must match RES_ID (th01/resident.hpp:41) exactly.
	T1CASE_RESIDENT_ID_BUF[0] = 'R';
	T1CASE_RESIDENT_ID_BUF[1] = 'e';
	T1CASE_RESIDENT_ID_BUF[2] = 'i';
	T1CASE_RESIDENT_ID_BUF[3] = 'i';
	T1CASE_RESIDENT_ID_BUF[4] = 'd';
	T1CASE_RESIDENT_ID_BUF[5] = 'e';
	T1CASE_RESIDENT_ID_BUF[6] = 'n';
	T1CASE_RESIDENT_ID_BUF[7] = 'C';
	T1CASE_RESIDENT_ID_BUF[8] = 'o';
	T1CASE_RESIDENT_ID_BUF[9] = 'n';
	T1CASE_RESIDENT_ID_BUF[10] = 'f';
	T1CASE_RESIDENT_ID_BUF[11] = 'i';
	T1CASE_RESIDENT_ID_BUF[12] = 'g';
	T1CASE_RESIDENT_ID_BUF[13] = '\0';

	t1case_paths_ready = true;
}

/// File access
/// -----------
/// Turbo C++'s low-level I/O rather than master.lib's `file_*` API, which is
/// how the TH03 reference module does it. Two reasons, both TH01-specific:
///
///  1. `file_append.asm` is NOT among the master.lib routines th01_reiiden.asm
///     includes, and adding an include to that file would be adding code to an
///     original segment contribution — forbidden by CLAUDE.md. TH03 could add
///     five includes plus a paragraph pad to `th03/main[text].asm`; TH01's
///     rule does not allow the equivalent.
///  2. master.lib's `file_*` API has a single global handle that the game's own
///     packfile code shares. Independent descriptors remove that coupling
///     entirely instead of having to reason about it.

static int t1f_read_open(const char *fn)
{
	return open(fn, (O_RDONLY | O_BINARY));
}

// Truncating create.
static int t1f_create(const char *fn)
{
	return open(fn, (O_WRONLY | O_CREAT | O_TRUNC | O_BINARY), S_IWRITE);
}

// Non-truncating open-or-create, for in-place header rewrites and appends.
static int t1f_update(const char *fn)
{
	return open(fn, (O_WRONLY | O_CREAT | O_BINARY), S_IWRITE);
}

static bool t1f_write(int fd, const void far *buf, unsigned size)
{
	return (write(fd, const_cast<void far *>(buf), size) == static_cast<int>(size));
}

// Stack objects are SS-relative in this memory model, so every helper a caller
// may hand a local to takes a far pointer.
static uint32_t t1case_fnv1a(uint32_t hash, const void far *buf, unsigned size)
{
	const uint8_t far *p = reinterpret_cast<const uint8_t far *>(buf);

	while(size != 0) {
		hash ^= static_cast<uint32_t>(*p++);
		hash *= T1CASE_FNV1A_PRIME;
		size--;
	}
	return hash;
}

/// Subsystem hashing
/// -----------------
/// TXSPLIT_CONTRACT.md §7. Two independent FNV-1a/32 passes over the SAME
/// serialized byte sequence, so an 8086 needs only 32-bit arithmetic. Pass B
/// XORs the byte with its index, which makes the pair disagree on
/// transpositions and on runs of equal bytes.
///
/// Fields are serialized in a fixed declared order with explicit widths.
/// Never a struct, never padding, never a pointer, never a segment.

static uint32_t t1h_a;
static uint32_t t1h_b;
static uint16_t t1h_i;

static void t1h_begin(void)
{
	t1h_a = T1CASE_FNV1A_BASIS;
	t1h_b = T1SPLIT_PASSB_BASIS;
	t1h_i = 0;
}

static void t1h_u8(uint8_t value)
{
	t1h_a = ((t1h_a ^ static_cast<uint32_t>(value)) * T1CASE_FNV1A_PRIME);
	t1h_b = ((t1h_b ^ static_cast<uint32_t>(
		static_cast<uint8_t>(value ^ static_cast<uint8_t>(t1h_i & 0xFF))
	)) * T1CASE_FNV1A_PRIME);
	t1h_i++;
}

static void t1h_u16(uint16_t value)
{
	t1h_u8(static_cast<uint8_t>(value));
	t1h_u8(static_cast<uint8_t>(value >> 8));
}

static void t1h_u32(uint32_t value)
{
	t1h_u16(static_cast<uint16_t>(value));
	t1h_u16(static_cast<uint16_t>(value >> 16));
}

// x87 `double` is 8 bytes with a fixed little-endian layout. TH01 is the only
// one of the five games with floating-point gameplay state
// ([orb_force], [orb_velocity_y]), so this is the only game whose row depends
// on the FPU. `_control87(MCW_EM, MCW_EM)` at th01/main_01.cpp:495 masks every
// exception, so the value is always well-formed.
static void t1h_double(double value)
{
	// Stack objects are SS-relative in this memory model, so anything that
	// addresses a local goes through a far pointer.
	const uint8_t far *p = reinterpret_cast<const uint8_t far *>(&value);
	int i;

	for(i = 0; i < 8; i++) {
		t1h_u8(p[i]);
	}
}

static void t1h_commit(t1split_row_t far *row, int group)
{
	row->hash[(group * 2) + 0] = t1h_b;
	row->hash[(group * 2) + 1] = t1h_a;
}

static void t1h_group_rng(void)
{
	t1h_begin();
	t1h_u32(static_cast<uint32_t>(random_seed));
	t1h_u32(frame_rand);
}

static void t1h_group_run(void)
{
	int i;

	t1h_begin();
	t1h_u16(resident->stage_id);
	t1h_u8(static_cast<uint8_t>(route));
	t1h_u8(static_cast<uint8_t>(rank));
	t1h_u16(static_cast<uint16_t>(rem_lives));
	t1h_u8(static_cast<uint8_t>(rem_bombs));
	t1h_u8(static_cast<uint8_t>(credit_lives_extra));
	t1h_u32(static_cast<uint32_t>(score));
	t1h_u32(static_cast<uint32_t>(resident->score_highest));
	t1h_u32(static_cast<uint32_t>(continues_total));
	for(i = 0; i < SCENE_COUNT; i++) {
		t1h_u32(static_cast<uint32_t>(resident->continues_per_scene[i]));
	}
	for(i = 0; i < (STAGES_PER_SCENE - 1); i++) {
		t1h_u32(static_cast<uint32_t>(resident->bonus_per_stage[i]));
	}
	t1h_u16(resident->point_value);
	t1h_u16(static_cast<uint16_t>(resident->pellet_speed));
	t1h_u8(static_cast<uint8_t>(resident->end_flag));
	t1h_u16(static_cast<uint16_t>(stage_cleared));
	t1h_u8(static_cast<uint8_t>(game_cleared));
	t1h_u8(static_cast<uint8_t>(first_stage_in_scene));
	t1h_u8(static_cast<uint8_t>(stage_num));
	t1h_u16(stage_timer);
	t1h_u32(frame_since_start_of_binary);
}

static void t1h_group_player(void)
{
	int i;

	t1h_begin();
	t1h_u16(static_cast<uint16_t>(player_left));
	t1h_u8(static_cast<uint8_t>(player_deflecting));
	t1h_u8(static_cast<uint8_t>(player_sliding));
	t1h_u8(static_cast<uint8_t>(player_is_hit));
	t1h_u16(static_cast<uint16_t>(player_invincible));
	t1h_u16(static_cast<uint16_t>(player_invincibility_time));

	t1h_u16(static_cast<uint16_t>(orb_cur_left));
	t1h_u16(static_cast<uint16_t>(orb_cur_top));
	t1h_u16(static_cast<uint16_t>(orb_in_portal));
	t1h_u16(static_cast<uint16_t>(orb_rotation_frame));
	t1h_u16(static_cast<uint16_t>(orb_velocity_x));
	t1h_u16(static_cast<uint16_t>(orb_force_frame));
	t1h_double(orb_force);
	t1h_double(orb_velocity_y_get());

	t1h_u32(bomb_frame);
	t1h_u8(static_cast<uint8_t>(bomb_damaging));
	t1h_u16(static_cast<uint16_t>(bomb_doubletap_frame));
	t1h_u8(static_cast<uint8_t>(bombs_extra_per_life_lost));
	t1h_u16(static_cast<uint16_t>(cardcombo_cur));
	t1h_u16(static_cast<uint16_t>(cardcombo_max));

	// Shot slots. `left`/`top` are gameplay positions on a plain SoA template
	// (th01/main/entity.hpp:15-22) with no blitting bookkeeping of its own.
	for(i = 0; i < SHOT_COUNT; i++) {
		t1h_u16(static_cast<uint16_t>(Shots.left[i]));
		t1h_u16(static_cast<uint16_t>(Shots.top[i]));
		t1h_u8(static_cast<uint8_t>(Shots.moving[i]));
		t1h_u8(Shots.decay_frame[i]);
		t1h_u16(static_cast<uint16_t>(Shots.unknown[i]));
	}
}

static void t1h_group_bullets(void)
{
	const Pellet *p;
	int i;

	t1h_begin();

	// Only gameplay fields. `prev_left`/`prev_top` are the unput (unblit)
	// bookkeeping and `not_rendered` is a flicker-reduction flag that
	// explicitly "does not disable hit testing" (th01/main/bullet/pellet.hpp:133),
	// so all three are presentation state and are excluded, as is
	// `interlace_field` ("rendering pellets at odd or even indices this
	// frame?", :164) and the cloud sprite position.
	for(i = 0; i < PELLET_COUNT; i++) {
		p = Pellets.t1case_slot(i);
		t1h_u8(static_cast<uint8_t>(p->moving));
		t1h_u8(p->motion_type);
		t1h_u16(static_cast<uint16_t>(p->cur_left.v));
		t1h_u16(static_cast<uint16_t>(p->cur_top.v));
		t1h_u16(static_cast<uint16_t>(p->velocity.x.v));
		t1h_u16(static_cast<uint16_t>(p->velocity.y.v));
		t1h_u16(static_cast<uint16_t>(p->spin_center.x.v));
		t1h_u16(static_cast<uint16_t>(p->spin_center.y.v));
		t1h_u16(static_cast<uint16_t>(p->spin_velocity.x.v));
		t1h_u16(static_cast<uint16_t>(p->spin_velocity.y.v));
		t1h_u16(static_cast<uint16_t>(p->from_group));
		t1h_u16(static_cast<uint16_t>(p->age));
		t1h_u16(static_cast<uint16_t>(p->speed.v));
		t1h_u16(static_cast<uint16_t>(p->decay_frame));
		t1h_u16(static_cast<uint16_t>(p->cloud_frame));
		t1h_u16(static_cast<uint16_t>(p->angle));
		t1h_u8(static_cast<uint8_t>(p->sling_direction));
	}
	t1h_u16(static_cast<uint16_t>(Pellets.t1case_alive_count()));
	t1h_u8(static_cast<uint8_t>(Pellets.spawn_with_cloud));
	t1h_u16(static_cast<uint16_t>(pellet_interlace));
}

static void t1h_group_boss(void)
{
	t1h_begin();
	t1h_u8(static_cast<uint8_t>(boss_id));
	t1h_u16(static_cast<uint16_t>(boss_hp));
	t1h_u8(static_cast<uint8_t>(boss_phase));
	t1h_u16(static_cast<uint16_t>(boss_phase_frame));
}

static void t1h_group_scoring(void)
{
	t1h_begin();
	t1h_u32(static_cast<uint32_t>(score_bonus));
	t1h_u16(static_cast<uint16_t>(extend_next));
	t1h_u16(pellet_destroy_score_delta);
}

static void t1h_group_hud(void)
{
	t1h_begin();
	t1h_u16(hud_hp_first_white);
	t1h_u16(hud_hp_first_redwhite);
}

// Group 9 is TH01-specific and has no analogue in the other four games.
// [input_prev] is a function-local static inside input_sense()
// (th01/main_01.cpp:158) and is unreachable without the pointer that
// t1case_frame_io() latches. Hashing it is the ONLY way to prove the injector
// did not desynchronize the edge detector, which is TH01's single most likely
// failure mode — so it is in the schema from version 1, not "after self-play
// is stable" (state/re/DETERMINISTIC_STATE_TH01.md §6).
static void t1h_group_input(void)
{
	int i;

	t1h_begin();
	if(t1case_input_prev != nullptr) {
		for(i = 0; i < 16; i++) {
			t1h_u8(t1case_input_prev[i]);
		}
	}
	t1h_u8(static_cast<uint8_t>(input_up));
	t1h_u8(static_cast<uint8_t>(input_down));
	t1h_u8(input_lr);
	t1h_u8(static_cast<uint8_t>(input_shot));
	t1h_u8(static_cast<uint8_t>(input_strike));
	t1h_u8(static_cast<uint8_t>(input_ok));
	t1h_u8(static_cast<uint8_t>(paused));
	t1h_u8(static_cast<uint8_t>(input_bomb));
	t1h_u8(static_cast<uint8_t>(input_mem_enter));
	t1h_u8(static_cast<uint8_t>(input_mem_leave));
	for(i = 0; i < T1CASE_GROUP_COUNT; i++) {
		t1h_u8(t1case_keys[i]);
	}
}

/// Status and diagnostics
/// ----------------------

// Status text is emitted one character at a time so this module contributes no
// initialized data; see the module comment.
static int t1case_text_fd;

static void t1case_write_char(char c)
{
	char buf[1];

	buf[0] = c;
	t1f_write(t1case_text_fd, buf, 1);
}

static void t1case_write_text(uint8_t id)
{
	t1case_write_char((id <= T1T_OK_INPUT_END) ? 'o' : 'e');
	if(id <= T1T_OK_INPUT_END) {
		t1case_write_char('k');
	} else {
		t1case_write_char('r');
		t1case_write_char('r');
		t1case_write_char('o');
		t1case_write_char('r');
	}
	t1case_write_char(':');
	switch(id) {
	case T1T_OK_RECORD:
		t1case_write_char('r'); t1case_write_char('e'); t1case_write_char('c');
		t1case_write_char('o'); t1case_write_char('r'); t1case_write_char('d');
		break;
	case T1T_OK_PLAYBACK:
		t1case_write_char('p'); t1case_write_char('l'); t1case_write_char('a');
		t1case_write_char('y'); t1case_write_char('b'); t1case_write_char('a');
		t1case_write_char('c'); t1case_write_char('k');
		break;
	case T1T_OK_SELFTEST:
		t1case_write_char('s'); t1case_write_char('e'); t1case_write_char('l');
		t1case_write_char('f'); t1case_write_char('t'); t1case_write_char('e');
		t1case_write_char('s'); t1case_write_char('t');
		break;
	case T1T_OK_INPUT_END:
		t1case_write_char('i'); t1case_write_char('n'); t1case_write_char('p');
		t1case_write_char('u'); t1case_write_char('t'); t1case_write_char('-');
		t1case_write_char('e'); t1case_write_char('n'); t1case_write_char('d');
		break;
	case T1T_ERR_CASE_HEADER:
		t1case_write_char('h'); t1case_write_char('e'); t1case_write_char('a');
		t1case_write_char('d'); t1case_write_char('e'); t1case_write_char('r');
		break;
	case T1T_ERR_CASE_CREATE:
		t1case_write_char('c'); t1case_write_char('r'); t1case_write_char('e');
		t1case_write_char('a'); t1case_write_char('t'); t1case_write_char('e');
		break;
	case T1T_ERR_FRAME_IO:
		t1case_write_char('f'); t1case_write_char('r'); t1case_write_char('a');
		t1case_write_char('m'); t1case_write_char('e'); t1case_write_char('-');
		t1case_write_char('i'); t1case_write_char('o');
		break;
	case T1T_ERR_DESYNC:
		t1case_write_char('d'); t1case_write_char('e'); t1case_write_char('s');
		t1case_write_char('y'); t1case_write_char('n'); t1case_write_char('c');
		break;
	case T1T_ERR_SPLIT_OPEN:
		t1case_write_char('s'); t1case_write_char('p'); t1case_write_char('l');
		t1case_write_char('i'); t1case_write_char('t');
		break;
	case T1T_ERR_VERIFY:
		t1case_write_char('v'); t1case_write_char('e'); t1case_write_char('r');
		t1case_write_char('i'); t1case_write_char('f'); t1case_write_char('y');
		break;
	case T1T_ERR_HANDOFF:
		t1case_write_char('h'); t1case_write_char('a'); t1case_write_char('n');
		t1case_write_char('d'); t1case_write_char('o'); t1case_write_char('f');
		t1case_write_char('f');
		break;
	case T1T_ERR_CONTROL_EARLY:
		t1case_write_char('c'); t1case_write_char('t'); t1case_write_char('l');
		t1case_write_char('-'); t1case_write_char('e'); t1case_write_char('a');
		t1case_write_char('r'); t1case_write_char('l'); t1case_write_char('y');
		break;
	case T1T_ERR_CONTROL_LATE:
		t1case_write_char('c'); t1case_write_char('t'); t1case_write_char('l');
		t1case_write_char('-'); t1case_write_char('l'); t1case_write_char('a');
		t1case_write_char('t'); t1case_write_char('e');
		break;
	case T1T_ERR_PROCESS:
		t1case_write_char('p'); t1case_write_char('r'); t1case_write_char('o');
		t1case_write_char('c'); t1case_write_char('e'); t1case_write_char('s');
		t1case_write_char('s');
		break;
	case T1T_ERR_CHECKPOINT:
		t1case_write_char('c'); t1case_write_char('k'); t1case_write_char('p');
		t1case_write_char('t');
		break;
	default:
		t1case_write_char('r'); t1case_write_char('e'); t1case_write_char('s');
		t1case_write_char('i'); t1case_write_char('d'); t1case_write_char('e');
		t1case_write_char('n'); t1case_write_char('t');
		break;
	}
}

static void t1case_done_write(uint8_t status)
{
	if(t1case_done_written) {
		return;
	}
	t1case_paths_init();
	t1case_text_fd = t1f_create(T1CASE_DONE_FN);
	if(t1case_text_fd >= 0) {
		t1case_write_text(status);
		t1case_write_char('\r');
		t1case_write_char('\n');
		close(t1case_text_fd);
	}
	t1case_done_written = true;
}

static char t1case_hex(uint8_t nibble)
{
	return static_cast<char>(
		(nibble < 10) ? ('0' + nibble) : ('A' + (nibble - 10))
	);
}

// One fixed-width line per milestone, flushed immediately, so a run that dies
// still leaves a usable trace. [tag] is exactly three characters.
static void t1case_diag(char t0, char t1, char t2, uint32_t a, uint32_t b)
{
	char line[24];
	int fd;
	int i;

	t1case_paths_init();
	line[0] = t0;
	line[1] = t1;
	line[2] = t2;
	line[3] = ' ';
	for(i = 0; i < 8; i++) {
		line[4 + i] = t1case_hex(static_cast<uint8_t>((a >> ((7 - i) * 4)) & 0xF));
	}
	line[12] = ' ';
	for(i = 0; i < 8; i++) {
		line[13 + i] = t1case_hex(static_cast<uint8_t>((b >> ((7 - i) * 4)) & 0xF));
	}
	line[21] = '\r';
	line[22] = '\n';
	fd = t1f_update(T1CASE_DIAG_FN);
	if(fd < 0) {
		return;
	}
	lseek(fd, 0L, SEEK_END);
	t1f_write(fd, line, 23);
	close(fd);
}

bool16 far t1case_active(void)
{
	return ((t1case_mode == T1CASE_RECORD) || (t1case_mode == T1CASE_PLAYBACK));
}

void far t1case_diag_note(char t0, char t1, char t2, uint32_t a, uint32_t b)
{
	// Only trace a run that actually has a case; never touch a stock run.
	if(!t1case_paths_ready) {
		return;
	}
	t1case_diag(t0, t1, t2, a, b);
}

/// Control surface
/// ---------------

static uint8_t t1case_cfg_mode(void)
{
	char cfg[64];
	int read_len;
	int i;
	int fd;
	int value = 0;
	char mode = '\0';

	t1case_paths_init();
	t1case_memclear(cfg, sizeof(cfg));
	fd = t1f_read_open(T1CASE_CFG_FN);
	if(fd < 0) {
		return T1CASE_DISABLED;
	}
	read_len = read(fd, cfg, (sizeof(cfg) - 1));
	close(fd);
	if(read_len < 0) {
		return T1CASE_DISABLED;
	}

	for(i = 0; i < read_len; i++) {
		if(
			(cfg[i] != ' ') && (cfg[i] != '\t') &&
			(cfg[i] != '\r') && (cfg[i] != '\n')
		) {
			mode = cfg[i];
			break;
		}
	}

	// An optional decimal immediately after the mode character selects the
	// checkpoint to resume from: "p3" plays the case from checkpoint 3. "p"
	// and "p0" are the same thing, the case's own beginning, so the whole
	// existing control surface keeps its meaning unchanged.
	t1case_cfg_checkpoint = 0;
	for(i = (i + 1); i < read_len; i++) {
		if((cfg[i] < '0') || (cfg[i] > '9')) {
			break;
		}
		value = ((value * 10) + (cfg[i] - '0'));
		if(value > 255) {
			value = 255; // rejected below against `checkpoint_count`
		}
	}
	t1case_cfg_checkpoint = static_cast<uint8_t>(value);
	if((mode == 'r') || (mode == 'R')) {
		return T1CASE_RECORD;
	}
	if((mode == 'p') || (mode == 'P')) {
		return T1CASE_PLAYBACK;
	}
	if((mode == 'g') || (mode == 'G')) {
		return T1CASE_SELFTEST;
	}
	return T1CASE_DISABLED;
}

/// Carrier
/// -------
/// REPLAY_CORE_CONTRACT.md §7. The two landmines that section records were both
/// paid for on this branch and both still apply:
///
///  1. `resdata_create()` writes the ID string to offset 0 of the new block, so
///     a blanket clear after creating it makes the very next `resdata_exist()`
///     miss. Clear everything EXCEPT `id`.
///  2. Resume precedence is CARRIER-FIRST: T1CASE.CFG is consulted only when the
///     block is absent or invalid. Without that a self-restarted REIIDEN
///     re-applies the startup block and restarts the case from record zero,
///     which for TH01 is 8-10 opportunities per run.

static bool t1case_res_magic_ok(void)
{
	return (
		(t1case_res != 0) &&
		(t1case_res->magic[0] == T1CASE_RES_MAGIC_0) &&
		(t1case_res->magic[1] == T1CASE_RES_MAGIC_1) &&
		(t1case_res->magic[2] == T1CASE_RES_MAGIC_2) &&
		(t1case_res->magic[3] == T1CASE_RES_MAGIC_3) &&
		(t1case_res->carrier_version == T1CASE_RES_VERSION)
	);
}

// Stamps identity and zeroes every region the core owns. Mirrors
// `replay_resident_handoff_mode_set()` (th03/main/replay.cpp:4604), which
// likewise clears first and stamps the magic afterwards.
static void t1case_res_stamp(void)
{
	unsigned i;

	t1case_res->magic[0] = T1CASE_RES_MAGIC_0;
	t1case_res->magic[1] = T1CASE_RES_MAGIC_1;
	t1case_res->magic[2] = T1CASE_RES_MAGIC_2;
	t1case_res->magic[3] = T1CASE_RES_MAGIC_3;
	t1case_res->carrier_version = T1CASE_RES_VERSION;
	t1case_res->mode = T1CASE_DISABLED;
	t1case_res->slot = T1CASE_SLOT_NONE;
	t1case_res->process_id = T1CASE_PROCESS_REIIDEN;
	t1case_res->process_seq = 0;
	t1case_res->flags = 0;
	t1case_res->sample_count = 0;
	t1case_res->global_frame = 0;
	t1case_res->input_byte_count = 0;
	t1case_res->record_count = 0;
	t1case_res->committed = 0;
	t1case_res->payload_checksum = T1CASE_FNV1A_BASIS;
	t1case_res->split_rows = 0;
	t1case_res->checkpoint_checksum = T1CASE_FNV1A_BASIS;
	for(i = 0; i < T1CASE_RES_PROTECT_SIZE; i++) {
		t1case_res->protect[i] = 0;
	}
}

static bool t1case_res_open(bool create)
{
	t1case_res = ResData<t1case_res_t>::exist(T1CASE_RES_ID_BUF);
	if(t1case_res) {
		// A block under our ID whose magic does not check out is a stale one
		// from an earlier mod build, left resident by an `execl` chain. On the
		// first-process path it must be RE-STAMPED, not adopted: adopting it
		// would inherit whatever cursors that build left behind.
		if(create && !t1case_res_magic_ok()) {
			t1case_res_stamp();
		}
		return true;
	}
	if(!create) {
		return false;
	}
	t1case_res = ResData<t1case_res_t>::create(T1CASE_RES_ID_BUF);
	if(!t1case_res) {
		return false;
	}
	t1case_res_stamp();
	return true;
}

// Obtains resident_t, creating it when this process is the case's first.
// REIIDEN normally relies on OP having created the block
// (th01/core/resstuff.cpp:13-40); an oracle run starts REIIDEN directly, so
// the case has to stand in for OP. Fresh blocks get ZUN's own configuration
// defaults (th01/formats/cfg.hpp:14-17, mirroring th01/main_01.cpp:80-83);
// they are then either overwritten by the case's startup block on playback, or
// captured into it on recording, so no value here is ever assumed.
static bool t1case_resident_ensure(void)
{
	resident_t __seg *seg = ResData<resident_t>::exist(T1CASE_RESIDENT_ID_BUF);

	if(!seg) {
		seg = ResData<resident_t>::create(T1CASE_RESIDENT_ID_BUF);
		if(!seg) {
			return false;
		}
		resident = seg;

		// Clear everything EXCEPT `id`. `resdata_create()` writes the ID
		// string to offset 0 of the new block (libs/master.lib/resdata.asm,
		// `RSDCREATE_ALLOC_OK`: `xor DI,DI` / `rep movsb`), which is precisely
		// why `resident_t`'s first member is `char id[sizeof(RES_ID)]`.
		// Clearing it would make the very next `resdata_exist()` fail to match
		// the block that was just created.
		t1case_memclear(
			(reinterpret_cast<uint8_t far *>(resident) + sizeof(resident->id)),
			(sizeof(resident_t) - sizeof(resident->id))
		);
		resident->rank = CFG_RANK_DEFAULT;
		resident->bgm_mode = CFG_BGM_MODE_DEFAULT;
		resident->rem_bombs = CFG_CREDIT_BOMBS_DEFAULT;
		resident->credit_lives_extra = CFG_CREDIT_LIVES_EXTRA_DEFAULT;
		resident->rem_lives = (CFG_CREDIT_LIVES_EXTRA_DEFAULT + 2);
		resident->debug_mode = DM_OFF;
		resident->end_flag = ES_NONE;
		return true;
	}
	resident = seg;
	return true;
}

static void t1case_handoff_load(void)
{
	t1case_sample_count = t1case_res->sample_count;
	t1case_record_count = t1case_res->record_count;
	t1case_global_frame = t1case_res->global_frame;
	t1case_input_byte_count = t1case_res->input_byte_count;
	t1case_payload_checksum = t1case_res->payload_checksum;
	t1case_split_rows = t1case_res->split_rows;
	t1case_checkpoint_checksum = t1case_res->checkpoint_checksum;
	t1case_started = ((t1case_res->flags & T1CASE_RES_FLAG_STARTED) != 0);
}

static void t1case_handoff_store(void)
{
	uint16_t flags;

	if(!t1case_res) {
		return;
	}
	t1case_res->mode = t1case_mode;
	t1case_res->slot = T1CASE_SLOT_NONE;
	t1case_res->process_id = T1CASE_PROCESS_REIIDEN;

	flags = (t1case_res->flags & T1CASE_RES_FLAG_DONE);
	if(t1case_started) {
		flags |= T1CASE_RES_FLAG_STARTED;
	}
	if(t1case_control_pending) {
		flags |= T1CASE_RES_FLAG_CONTROL_PENDING;
	}
	t1case_res->flags = flags;

	t1case_res->sample_count = t1case_sample_count;
	t1case_res->record_count = t1case_record_count;
	t1case_res->global_frame = t1case_global_frame;
	t1case_res->input_byte_count = t1case_input_byte_count;
	t1case_res->payload_checksum = t1case_payload_checksum;
	t1case_res->split_rows = t1case_split_rows;
	t1case_res->checkpoint_checksum = t1case_checkpoint_checksum;
}

// Ends the case: a later REIIDEN process must not resume a run that is over.
//
// The block STAYS VALID and stays findable. The previous revision wrote
// `id[0] = '\0'` so that `resdata_exist()` would miss, which does end the case
// for the process that wrote it — and hands the NEXT process a run with no
// carrier at all, so t1case_session_start() falls through to T1CASE.CFG and
// starts the whole case again from record zero with the startup block
// re-applied. REIIDEN self-`execl`s 8-10 times per run, and TH01's playback
// reaches input-end mid-run by construction, so that path is reachable rather
// than theoretical. A DONE latch in a still-valid carrier is the same "the case
// is over" statement made in a way the next process can read.
static void t1case_handoff_clear(void)
{
	if(t1case_res) {
		t1case_res->mode = T1CASE_DISABLED;
		t1case_res->flags |= T1CASE_RES_FLAG_DONE;
	}
}

// Cross-checks the resumed carrier against the case file's own header. This is
// the HDR/FIN equality that T1DIAG.TXT has always exposed to a human reader,
// enforced by the game instead: on a record resume the header on disk is the
// one the previous process last wrote, so its counters and the carrier's must
// agree exactly, and on a playback resume the carrier's cursors must lie inside
// the case they claim to index. A carrier that survived `execl` with one field
// stale produces a case that is silently wrong at exactly one splice point.
static bool t1case_handoff_verify(void)
{
	// The two identity fields, checked rather than merely written. `slot` is
	// NONE for the whole oracle lineage (TXCASE_CONTRACT.md's control surface is
	// one fixed T1CASE.BIN, not a numbered slot), and `process_id` is the
	// carrier-side counterpart of the control packet's second byte: the stream
	// says which process WROTE a segment, the carrier says which process STORED
	// the cursors. Only REIIDEN is instrumented, so neither can differ today
	// without the module being wrong — which is exactly what a check is for.
	if(t1case_res->slot != T1CASE_SLOT_NONE) {
		return false;
	}
	if(t1case_res->process_id != T1CASE_PROCESS_REIIDEN) {
		return false;
	}
	if(t1case_mode == T1CASE_RECORD) {
		return (
			(t1case_header.record_count == t1case_record_count) &&
			(t1case_header.sample_count == t1case_sample_count) &&
			(t1case_header.payload_size == t1case_input_byte_count) &&

			// The checkpoint array, from both directions: the carrier's running
			// hash against the one the previous process wrote into the header.
			(t1case_header.checkpoint_checksum ==
				t1case_checkpoint_checksum)
		);
	}
	return (
		(t1case_record_count <= t1case_header.record_count) &&
		(t1case_sample_count <= t1case_header.sample_count) &&
		(t1case_input_byte_count <= t1case_header.payload_size)
	);
}

#include "th01/main/t1protect.cpp"

/// Packet codec
/// ------------
/// Ported from th03/main/replay.cpp: the encoder at :3678 with its
/// extend-in-place run growth at :3714, the decoder at :3870, and the flush
/// tail of replay_user_header_write() at :2898-2917. Not ported: the seek
/// reader (:3998) and replay_user_decoder_seek() (:4037), because an oracle
/// case is written once and read front to back and has no mid-stream resume;
/// and the autofire shot-bit re-derivation at :3701, which is TH03 gameplay.

static uint8_t t1case_rle_tag(uint8_t phase, uint8_t run)
{
	return static_cast<uint8_t>(
		(phase << T1CASE_PACKET_PHASE_SHIFT) | (run - 1)
	);
}

static bool t1case_buffer_u8(uint8_t value)
{
	if(t1case_wbuf_len >= T1CASE_WBUF_SIZE) {
		return false;
	}
	t1case_wbuf[t1case_wbuf_len++] = value;
	t1case_input_byte_count++;
	return true;
}

// Commits every buffered byte and folds it into [payload_checksum].
//
// ALWAYS closes the open packet, even on the empty-buffer path. Extend-in-place
// rewrites [t1case_wbuf][t1case_packet_at], which after a flush addresses a byte
// that is no longer the open packet's tag; TH03 clears the same bit for the same
// reason at th03/main/replay.cpp:2917. The delta basis is deliberately NOT
// invalidated here.
static bool t1case_stream_flush(void)
{
	uint32_t offset;
	int fd;

	t1case_packet_open = false;
	if(t1case_wbuf_len == 0) {
		return true;
	}
	offset = (
		t1case_header.payload_offset +
		t1case_input_byte_count - t1case_wbuf_len
	);
	fd = t1f_update(T1CASE_BIN_FN);
	if(fd < 0) {
		return false;
	}
	lseek(fd, static_cast<long>(offset), SEEK_SET);
	if(!t1f_write(fd, t1case_wbuf, t1case_wbuf_len)) {
		close(fd);
		return false;
	}
	close(fd);
	t1case_payload_checksum = t1case_fnv1a(
		t1case_payload_checksum, t1case_wbuf, t1case_wbuf_len
	);
	t1case_wbuf_len = 0;
	return true;
}

// Encodes the seven latched group bytes as one logical sample.
static bool t1case_encode_sample(uint8_t phase)
{
	uint8_t mask = 0;
	int i;

	// Never let a packet straddle a flush (th03/main/replay.cpp:3712).
	if(
		(t1case_wbuf_len > (T1CASE_WBUF_SIZE - T1CASE_PACKET_SIZE_MAX)) &&
		!t1case_stream_flush()
	) {
		return false;
	}

	if(
		t1case_packet_open &&
		(t1case_packet_phase == phase) &&
		(t1case_packet_run < T1CASE_PACKET_RUN_MAX)
	) {
		for(i = 0; i < T1CASE_GROUP_COUNT; i++) {
			if(t1case_enc_prev[i] != t1case_keys[i]) {
				break;
			}
		}
		if(i == T1CASE_GROUP_COUNT) {
			// Extend in place: rewrite the already-buffered tag byte. Growing a
			// run costs ZERO additional bytes, and that is the entire reason the
			// ratio is what it is (REPLAY_CORE_CONTRACT.md §4.2).
			t1case_packet_run++;
			t1case_wbuf[t1case_packet_at] = t1case_rle_tag(
				phase, t1case_packet_run
			);
			return true;
		}
	}

	if(!t1case_enc_prev_valid) {
		mask = T1CASE_PACKET_KEYFRAME_MASK;
	} else {
		for(i = 0; i < T1CASE_GROUP_COUNT; i++) {
			if(t1case_keys[i] != t1case_enc_prev[i]) {
				mask |= static_cast<uint8_t>(1 << i);
			}
		}
	}
	t1case_packet_at = t1case_wbuf_len;
	if(
		!t1case_buffer_u8(t1case_rle_tag(phase, 1)) ||
		!t1case_buffer_u8(mask)
	) {
		return false;
	}
	for(i = 0; i < T1CASE_GROUP_COUNT; i++) {
		if(mask & (1 << i)) {
			if(!t1case_buffer_u8(t1case_keys[i])) {
				return false;
			}
		}
	}
	for(i = 0; i < T1CASE_GROUP_COUNT; i++) {
		t1case_enc_prev[i] = t1case_keys[i];
	}
	t1case_enc_prev_valid = true;
	t1case_packet_open = true;
	t1case_packet_run = 1;
	t1case_packet_phase = phase;
	return true;
}

static bool t1case_encode_control(uint8_t control)
{
	if(
		((t1case_wbuf_len + 2) > T1CASE_WBUF_SIZE) && !t1case_stream_flush()
	) {
		return false;
	}
	t1case_packet_open = false;
	if(
		!t1case_buffer_u8(static_cast<uint8_t>(
			(T1CASE_PHASE_CONTROL << T1CASE_PACKET_PHASE_SHIFT) |
			(control & T1CASE_PACKET_RUN_MASK)
		)) ||
		!t1case_buffer_u8(T1CASE_PROCESS_REIIDEN)
	) {
		return false;
	}
	// A control packet ends the process segment, so the next packet must be a
	// self-contained keyframe: the process that reads it starts at
	// [input_byte_count] and has never seen an earlier channel value.
	t1case_enc_prev_valid = false;
	return true;
}

// Returns the next stream byte, or -1. Sets [t1case_stream_io_error] only for a
// genuine read failure; running out of declared payload is a desync, not I/O.
static int t1case_stream_u8(void)
{
	uint32_t remaining;
	uint32_t offset;
	unsigned want;
	uint8_t value;
	int fd;

	if(t1case_rbuf_pos >= t1case_rbuf_len) {
		if(t1case_input_byte_count >= t1case_header.payload_size) {
			return -1;
		}
		remaining = (t1case_header.payload_size - t1case_input_byte_count);
		want = (
			(remaining > static_cast<uint32_t>(T1CASE_RBUF_SIZE)) ?
			T1CASE_RBUF_SIZE : static_cast<unsigned>(remaining)
		);
		offset = (t1case_header.payload_offset + t1case_input_byte_count);
		fd = t1f_read_open(T1CASE_BIN_FN);
		if(fd < 0) {
			t1case_stream_io_error = true;
			return -1;
		}
		lseek(fd, static_cast<long>(offset), SEEK_SET);
		if(read(fd, t1case_rbuf, want) != static_cast<int>(want)) {
			close(fd);
			t1case_stream_io_error = true;
			return -1;
		}
		close(fd);
		t1case_rbuf_len = static_cast<uint16_t>(want);
		t1case_rbuf_pos = 0;
	}
	value = t1case_rbuf[t1case_rbuf_pos++];
	t1case_input_byte_count++;

	// One FNV-1a step, inline: the same fold t1case_fnv1a() applies, without a
	// far pointer to a stack byte per byte of the stream.
	t1case_payload_checksum = (
		(t1case_payload_checksum ^ static_cast<uint32_t>(value)) *
		T1CASE_FNV1A_PRIME
	);
	return value;
}

// Decodes one logical sample into [t1case_keys], refilling from the stream when
// the current packet's run is spent. [phase] is what the game is doing right
// now and must equal the stream's; a mismatch is the cursor desync this whole
// design exists to catch, and is fatal rather than skipped
// (th03/main/replay.cpp:4287).
static bool t1case_decode_sample(uint8_t phase)
{
	int tag;
	int mask;
	int value;
	int i;

	if(t1case_dec_run == 0) {
		tag = t1case_stream_u8();
		if(tag < 0) {
			return false;
		}
		if(
			static_cast<uint8_t>(tag >> T1CASE_PACKET_PHASE_SHIFT) ==
			T1CASE_PHASE_CONTROL
		) {
			// §7.2's latch. The stream says this process segment is over while
			// the game is still asking for samples.
			//
			// TH01 does NOT do what MAINL does here. MAINL holds the last input
			// steady and drains, because its exit is an interpreter reaching a
			// natural end and the stream can legitimately run out first. TH01's
			// process boundaries are `execl` sites reached by game logic, and
			// game logic is a deterministic function of the recorded input, so
			// in a correct playback the control packet arrives exactly when the
			// game reaches the boundary. Waiting it out would mask the desync
			// this whole module exists to detect. Latch and fail, by name.
			t1case_control_pending = true;
			return false;
		}
		if(
			static_cast<uint8_t>(tag >> T1CASE_PACKET_PHASE_SHIFT) >
			T1CASE_PHASE_INTERSTITIAL
		) {
			return false; // phase 3 does not exist
		}
		mask = t1case_stream_u8();
		if(mask < 0) {
			return false;
		}
		if(mask & ~T1CASE_PACKET_KEYFRAME_MASK) {
			return false; // the spare bit is required zero
		}
		if(
			!t1case_dec_prev_valid &&
			(mask != T1CASE_PACKET_KEYFRAME_MASK)
		) {
			return false; // a process segment must open with a keyframe
		}
		for(i = 0; i < T1CASE_GROUP_COUNT; i++) {
			if(mask & (1 << i)) {
				value = t1case_stream_u8();
				if(value < 0) {
					return false;
				}
				t1case_keys[i] = static_cast<uint8_t>(value);
			}
		}
		t1case_dec_phase = static_cast<uint8_t>(
			tag >> T1CASE_PACKET_PHASE_SHIFT
		);
		t1case_dec_run = static_cast<uint8_t>(
			(tag & T1CASE_PACKET_RUN_MASK) + 1
		);
		t1case_dec_prev_valid = true;
	}
	if(t1case_dec_phase != phase) {
		return false;
	}
	t1case_dec_run--;
	return true;
}

static bool t1case_decode_control(uint8_t control)
{
	int tag;
	int process;

	// The stream must be at an exact packet boundary: a run that still owes
	// samples means the process ended earlier than the recording did. The other
	// half of §7.2's latch — CONTROL_LATE, reported by the caller.
	if(t1case_dec_run != 0) {
		return false;
	}
	tag = t1case_stream_u8();
	if(tag < 0) {
		return false;
	}
	if(
		static_cast<uint8_t>(tag >> T1CASE_PACKET_PHASE_SHIFT) !=
		T1CASE_PHASE_CONTROL
	) {
		return false;
	}
	if(static_cast<uint8_t>(tag & T1CASE_PACKET_RUN_MASK) != control) {
		return false;
	}

	// "Resumed in the wrong binary", detected directly rather than inferred from
	// TH03's control-code parity, which REIIDEN's self-handoff makes vacuous
	// (REPLAY_CORE_CONTRACT.md §7.2).
	process = t1case_stream_u8();
	if(process != T1CASE_PROCESS_REIIDEN) {
		t1case_process_mismatch = true;
		return false;
	}
	t1case_control_pending = false;
	t1case_dec_prev_valid = false;
	return true;
}

/// Case file I/O
/// -------------
/// Turbo C++ low-level I/O (delta D4), so every access is
/// open -> seek -> read/write -> close and no descriptor is left open across a
/// game call. The packet stream is buffered on both sides, so a recording
/// touches the file once per T1SPLIT_INTERVAL_SAMPLES samples rather than once
/// per sample as version 1 did.

static void t1case_header_checksum_set(void)
{
	uint32_t hash;

	t1case_header.header_checksum = 0;
	hash = t1case_fnv1a(
		T1CASE_FNV1A_BASIS, &t1case_header, sizeof(t1case_header)
	);
	hash = t1case_fnv1a(hash, &t1case_startup, sizeof(t1case_startup));
	t1case_header.header_checksum = hash;
}

// Commits the packet stream and then rewrites the header, in that order: the
// header declares [payload_size] and [payload_checksum], and both must describe
// bytes that are already on disk. TH03 folds the same flush into its own header
// write for the same reason (th03/main/replay.cpp:2898-2917).
static bool t1case_header_write(bool create)
{
	int fd;
	int i;

	t1case_paths_init();
	if(create) {
		// Truncate first; the flush below writes into the file this creates.
		fd = t1f_create(T1CASE_BIN_FN);
		if(fd < 0) {
			return false;
		}
		close(fd);
	}
	if(!t1case_stream_flush()) {
		return false;
	}
	t1case_header.record_count = t1case_record_count;
	t1case_header.sample_count = t1case_sample_count;
	t1case_header.payload_size = t1case_input_byte_count;
	t1case_header.total_size = (
		t1case_header.payload_offset + t1case_header.payload_size
	);
	t1case_header.payload_checksum = t1case_payload_checksum;
	t1case_header_checksum_set();

	fd = t1f_update(T1CASE_BIN_FN);
	if(fd < 0) {
		return false;
	}
	lseek(fd, 0L, SEEK_SET);
	if(!t1f_write(fd, &t1case_header, sizeof(t1case_header))) {
		close(fd);
		return false;
	}
	if(!t1f_write(fd, &t1case_startup, sizeof(t1case_startup))) {
		close(fd);
		return false;
	}

	// The checkpoint array is reserved PHYSICALLY at creation, right here,
	// where the file position is already T1CASE_CHECKPOINT_ARRAY_OFFSET. It
	// has to exist rather than be an lseek hole, because `total_size` counts
	// it from the first header write onwards and t1case_header_read()'s
	// physical-length check - which a RESUMING RECORD PROCESS runs against
	// its own case - would otherwise reject the case the previous process
	// just created.
	if(create) {
		t1case_memclear(&t1case_ckpt, sizeof(t1case_ckpt));
		for(i = 0; i < T1CASE_CHECKPOINT_CAP; i++) {
			if(!t1f_write(fd, &t1case_ckpt, sizeof(t1case_ckpt))) {
				close(fd);
				return false;
			}
		}
	}
	close(fd);
	return true;
}

static bool t1case_header_read(void)
{
	uint32_t stored;
	uint32_t computed;
	long physical_size;
	int fd;
	int i;

	t1case_paths_init();
	fd = t1f_read_open(T1CASE_BIN_FN);
	if(fd < 0) {
		return false;
	}
	physical_size = lseek(fd, 0L, SEEK_END);
	lseek(fd, 0L, SEEK_SET);
	if(
		read(fd, &t1case_header, sizeof(t1case_header)) !=
		static_cast<int>(sizeof(t1case_header))
	) {
		close(fd);
		return false;
	}
	if(
		read(fd, &t1case_startup, sizeof(t1case_startup)) !=
		static_cast<int>(sizeof(t1case_startup))
	) {
		close(fd);
		return false;
	}
	close(fd);

	if(
		(t1case_header.magic[0] != 'T') || (t1case_header.magic[1] != '1') ||
		(t1case_header.magic[2] != 'C') || (t1case_header.magic[3] != 'A') ||
		(t1case_header.magic[4] != 'S') || (t1case_header.magic[5] != 'E') ||
		(t1case_header.magic[6] != '1') || (t1case_header.magic[7] != '\0')
	) {
		return false;
	}
	if(
		(t1case_header.version != T1CASE_VERSION) ||
		(t1case_header.header_size != T1CASE_HEADER_SIZE) ||
		(t1case_header.startup_size != T1CASE_STARTUP_SIZE) ||
		(t1case_header.channel_count != T1CASE_GROUP_COUNT) ||
		(t1case_header.input_semantics != 1) ||
		(t1case_header.ruleset_id != 1) ||
		(t1case_header.source_kind != T1CASE_SOURCE_DIRECT) ||
		(t1case_header.first_process != T1CASE_PROCESS_REIIDEN) ||
		(t1case_header.flags & ~static_cast<uint16_t>(T1CASE_FLAG_KNOWN)) ||
		// The checkpoint array's geometry, and `payload_offset` derived from
		// it rather than fixed. A host-converted case declares capacity 0 and
		// therefore keeps v2's 128, which is what lets the archived Gate A
		// cases cross the version bump without growing.
		(t1case_header.checkpoint_stride != T1CASE_CHECKPOINT_STRIDE) ||
		(t1case_header.checkpoint_count >
			t1case_header.checkpoint_capacity) ||
		(t1case_header.payload_offset != (
			static_cast<uint32_t>(T1CASE_CHECKPOINT_ARRAY_OFFSET) +
			(static_cast<uint32_t>(t1case_header.checkpoint_capacity) *
				T1CASE_CHECKPOINT_STRIDE)
		)) ||
		(t1case_header.sample_count > t1case_header.record_count) ||
		// Every record costs at least the two bytes of a minimal packet, and a
		// run covers at most T1CASE_PACKET_RUN_MAX samples. Both bounds are
		// cheap and both reject a header whose counters cannot describe the
		// stream it declares.
		(t1case_header.payload_size <
			(((t1case_header.record_count + (T1CASE_PACKET_RUN_MAX - 1)) /
				T1CASE_PACKET_RUN_MAX) * 2UL)) ||
		(t1case_header.total_size !=
			(t1case_header.payload_offset + t1case_header.payload_size))
	) {
		return false;
	}
	// Readers reject trailing bytes and truncation. The TH03 reference module
	// never checks this on the guest side and leaves it to the host, which
	// means a physically short case is only caught later, as a read failure
	// mid-run. Checking it here makes the failure attributable.
	if(
		(physical_size < 0) ||
		(static_cast<uint32_t>(physical_size) != t1case_header.total_size)
	) {
		return false;
	}
	// Required-zero fields.
	for(i = 0; i < 3; i++) {
		if(t1case_startup.reserved[i] != 0) {
			return false;
		}
	}
	// An oracle case must not enable the debug key path: [mode_test] changes
	// the number of key_sense() calls per input_sense() pass.
	if((t1case_startup.mode_test != 0) || (t1case_startup.debug_mode != 0)) {
		return false;
	}
	if(t1case_startup.start_binary != t1case_header.first_process) {
		return false;
	}

	stored = t1case_header.header_checksum;
	t1case_header.header_checksum = 0;
	computed = t1case_fnv1a(
		T1CASE_FNV1A_BASIS, &t1case_header, sizeof(t1case_header)
	);
	computed = t1case_fnv1a(computed, &t1case_startup, sizeof(t1case_startup));
	t1case_header.header_checksum = stored;
	return (stored == computed);
}

static bool t1case_playback_final(void)
{
	return (
		(t1case_sample_count == t1case_header.sample_count) &&
		(t1case_record_count == t1case_header.record_count) &&
		// Version 1 could not check this: with fixed records, consuming
		// `record_count` records consumed the payload by definition. A packet
		// stream can decode the right number of samples out of the wrong number
		// of bytes, so the byte cursor is an independent witness.
		(t1case_input_byte_count == t1case_header.payload_size) &&
		(t1case_payload_checksum == t1case_header.payload_checksum)
	);
}

/// Split trace
/// -----------

static bool t1case_split_write_header(void)
{
	t1split_header_t header;
	int fd;

	t1case_paths_init();
	t1case_memclear(&header, sizeof(header));
	header.magic[0] = 'T';
	header.magic[1] = '1';
	header.magic[2] = 'S';
	header.magic[3] = 'P';
	header.magic[4] = 'L';
	header.magic[5] = 'T';
	header.magic[6] = '1';
	header.magic[7] = '\0';
	header.version = T1SPLIT_VERSION;
	header.header_size = T1SPLIT_HEADER_SIZE;
	header.row_size = T1SPLIT_ROW_SIZE;
	header.flags = T1SPLIT_VERSION;

	fd = t1f_create(T1CASE_SPLIT_FN);
	if(fd < 0) {
		return false;
	}
	if(!t1f_write(fd, &header, sizeof(header))) {
		close(fd);
		return false;
	}
	close(fd);
	return true;
}

// The row's `input` word: a compact digest of the seven group bytes, so a TSV
// export shows the injected input without a preimage. Low byte is the union of
// every group, high byte is a position-weighted mix, which distinguishes the
// same total set arriving in different groups.
static uint16_t t1case_input_digest(void)
{
	uint8_t both = 0;
	uint8_t mix = 0;
	int i;

	for(i = 0; i < T1CASE_GROUP_COUNT; i++) {
		both |= t1case_keys[i];
		mix = static_cast<uint8_t>(
			(mix << 1) ^ (mix >> 7) ^ t1case_keys[i] ^ static_cast<uint8_t>(i)
		);
	}
	return static_cast<uint16_t>(both | (static_cast<uint16_t>(mix) << 8));
}

static void t1case_split_row(uint8_t event)
{
	t1split_row_t row;
	int fd;

	if((t1case_mode == T1CASE_DISABLED) || (t1case_mode == T1CASE_ERROR)) {
		return;
	}
	t1case_paths_init();
	fd = t1f_update(T1CASE_SPLIT_FN);
	if(fd < 0) {
		t1case_mode = T1CASE_ERROR;
		t1case_done_write(T1T_ERR_SPLIT_OPEN);
		return;
	}
	lseek(fd, 0L, SEEK_END);
	t1case_memclear(&row, sizeof(row));

	row.event = event;
	row.process = T1CASE_PROCESS_REIIDEN;
	row.stage_id = static_cast<uint8_t>(resident->stage_id);
	row.rank = static_cast<uint8_t>(rank);
	row.global_frame = t1case_global_frame;
	row.scenario_cursor = frame_rand;
	row.input = t1case_input_digest();
	row.schema = T1SPLIT_VERSION;

	row.score = static_cast<uint32_t>(score);
	row.frame_rand = frame_rand;
	row.random_seed = static_cast<uint32_t>(random_seed);
	row.samples_consumed = t1case_sample_count;
	row.score_highest = resident->score_highest;
	row.bomb_frame = bomb_frame;
	row.rem_lives = static_cast<int16_t>(rem_lives);
	row.boss_hp = static_cast<int16_t>(boss_hp);
	row.player_left = static_cast<int16_t>(player_left);
	row.pellet_speed = static_cast<int16_t>(resident->pellet_speed);
	row.point_value = resident->point_value;
	row.stage_id_full = resident->stage_id;
	row.rem_bombs = rem_bombs;
	row.credit_lives_extra = credit_lives_extra;
	row.route = route;
	row.end_flag = static_cast<int8_t>(resident->end_flag);
	row.boss_id = boss_id;
	row.boss_phase = boss_phase;
	row.stage_cleared = static_cast<int8_t>(stage_cleared);
	row.player_is_hit = static_cast<int8_t>(player_is_hit);

	t1h_group_rng();      t1h_commit(&row, T1SPLIT_G_RNG);
	t1h_group_run();      t1h_commit(&row, T1SPLIT_G_RUN);
	t1h_group_player();   t1h_commit(&row, T1SPLIT_G_PLAYER);
	t1h_group_bullets();  t1h_commit(&row, T1SPLIT_G_BULLETS);
	t1h_group_boss();     t1h_commit(&row, T1SPLIT_G_BOSS);
	// Groups 5 (stage objects/cards/items) and 8 (geometry/effects) are [open]
	// for schema 2; they hash a declared-empty sequence and therefore emit the
	// two basis constants unchanged. That is deliberately recognizable rather
	// than silently zero. See state/notes/oracle-th01-bringup.md.
	t1h_begin();          t1h_commit(&row, T1SPLIT_G_STAGEOBJ);
	t1h_group_scoring();  t1h_commit(&row, T1SPLIT_G_SCORING);
	t1h_group_hud();      t1h_commit(&row, T1SPLIT_G_HUD);
	t1h_begin();          t1h_commit(&row, T1SPLIT_G_EFFECTS);
	t1h_group_input();    t1h_commit(&row, T1SPLIT_G_INPUT);

	if(!t1f_write(fd, &row, sizeof(row))) {
		t1case_mode = T1CASE_ERROR;
	}
	close(fd);
	t1case_split_rows++;
	t1case_handoff_store();
}

static void t1case_input_error(uint8_t status)
{
	t1case_split_row(T1SPLIT_EVENT_ERROR);
	t1case_mode = T1CASE_ERROR;
	t1case_handoff_clear();
	t1case_done_write(status);
}

/// Startup block
/// -------------

// Writes the current `resident_t` into [dst]. Takes a destination because a
// checkpoint capture on a RESUMED recording must not touch the global
// [t1case_startup], which at that moment holds the case's own startup block
// as read back from the header - and which t1case_header_write() would then
// write over the case's identity with a mid-run state.
static void t1case_startup_capture(t1case_startup_t far *dst)
{
	int i;

	t1case_memclear(dst, sizeof(t1case_startup_t));
	dst->resident_rand = resident->rand;
	dst->score = resident->score;
	dst->continues_total = resident->continues_total;
	dst->hiscore = resident->hiscore;
	dst->score_highest = resident->score_highest;
	for(i = 0; i < (STAGES_PER_SCENE - 1); i++) {
		dst->bonus_per_stage[i] = resident->bonus_per_stage[i];
	}
	for(i = 0; i < SCENE_COUNT; i++) {
		dst->continues_per_scene[i] = resident->continues_per_scene[i];
	}
	dst->stage_id = resident->stage_id;
	dst->point_value = resident->point_value;
	dst->pellet_speed = static_cast<int16_t>(resident->pellet_speed);
	dst->rank = resident->rank;
	dst->bgm_mode = static_cast<int8_t>(resident->bgm_mode);
	dst->rem_bombs = resident->rem_bombs;
	dst->credit_lives_extra = resident->credit_lives_extra;
	dst->rem_lives = resident->rem_lives;
	dst->route = resident->route;
	dst->end_flag = static_cast<int8_t>(resident->end_flag);
	dst->debug_mode = resident->debug_mode;
	dst->snd_need_init = resident->snd_need_init;
	dst->mode_test = 0;
	dst->start_binary = T1CASE_PROCESS_REIIDEN;
}

static void t1case_startup_apply(void)
{
	int i;

	resident->rand = t1case_startup.resident_rand;
	resident->score = t1case_startup.score;
	resident->continues_total = t1case_startup.continues_total;
	resident->hiscore = t1case_startup.hiscore;
	resident->score_highest = t1case_startup.score_highest;
	for(i = 0; i < (STAGES_PER_SCENE - 1); i++) {
		resident->bonus_per_stage[i] = t1case_startup.bonus_per_stage[i];
	}
	for(i = 0; i < SCENE_COUNT; i++) {
		resident->continues_per_scene[i] = t1case_startup.continues_per_scene[i];
	}
	resident->stage_id = t1case_startup.stage_id;
	resident->point_value = t1case_startup.point_value;
	resident->pellet_speed = t1case_startup.pellet_speed;
	resident->rank = t1case_startup.rank;
	resident->bgm_mode = static_cast<bgm_mode_t>(t1case_startup.bgm_mode);
	resident->rem_bombs = t1case_startup.rem_bombs;
	resident->credit_lives_extra = t1case_startup.credit_lives_extra;
	resident->rem_lives = t1case_startup.rem_lives;
	resident->route = t1case_startup.route;
	resident->end_flag = static_cast<end_sequence_t>(t1case_startup.end_flag);
	resident->snd_need_init = t1case_startup.snd_need_init;

	// [debug_mode] is forced off rather than restored: it gates [mode_test],
	// which changes the number of key_sense() calls per input_sense() pass,
	// and it makes REIIDEN block on scanf() at th01/main_01.cpp:507.
	resident->debug_mode = DM_OFF;
}

// Post-init VERIFY, not restore (TXCASE_CONTRACT.md, "Post-init verify, not
// post-init restore"). The correct behavior is to compare and fail, never to
// overwrite a value the normal path produced.
static bool t1case_startup_verify(void)
{
	int i;

	if(
		(resident->rand != t1case_startup.resident_rand) ||
		(resident->score != t1case_startup.score) ||
		(resident->continues_total != t1case_startup.continues_total) ||
		(resident->score_highest != t1case_startup.score_highest) ||
		(resident->stage_id != t1case_startup.stage_id) ||
		(resident->point_value != t1case_startup.point_value) ||
		(resident->pellet_speed != t1case_startup.pellet_speed) ||
		(resident->rank != t1case_startup.rank) ||
		(resident->rem_bombs != t1case_startup.rem_bombs) ||
		(resident->credit_lives_extra != t1case_startup.credit_lives_extra) ||
		(resident->route != t1case_startup.route)
	) {
		return false;
	}
	for(i = 0; i < SCENE_COUNT; i++) {
		if(resident->continues_per_scene[i] !=
			t1case_startup.continues_per_scene[i]) {
			return false;
		}
	}
	// [mode_test] must be off, or the input stream's shape changes.
	return (mode_test == false);
}

/// Per-round checkpoints
/// ---------------------
/// REPLAY_CORE_CONTRACT.md 5. The array's geometry and the reasoning behind the
/// 80-byte slot are in th01/t1case.hpp; this is the capture/restore pair.
///
/// One checkpoint per PROCESS SEGMENT, captured in t1case_session_start() from
/// the `resident_t` the previous process handed over, before the game has read
/// a single field of it. That is the entire trick: a fresh REIIDEN rebuilds all
/// of its live state from `resident_t` plus constants, so the state at that
/// instant IS the checkpoint. Nothing else has to be stored, and nothing has to
/// be regenerated - TH01 has no RNG rings, and `random_seed` is re-derived by
/// the game's own `irand_init(frame_rand)` immediately before the round-start
/// row.

// Reads slot [index] into [t1case_ckpt].
static bool t1case_checkpoint_read(uint8_t index)
{
	int fd;

	if(
		(index >= t1case_header.checkpoint_count) ||
		(t1case_header.checkpoint_stride != T1CASE_CHECKPOINT_STRIDE)
	) {
		return false;
	}
	fd = t1f_read_open(T1CASE_BIN_FN);
	if(fd < 0) {
		return false;
	}
	lseek(fd, static_cast<long>(
		static_cast<uint32_t>(T1CASE_CHECKPOINT_ARRAY_OFFSET) +
		(static_cast<uint32_t>(index) * T1CASE_CHECKPOINT_STRIDE)
	), SEEK_SET);
	if(
		read(fd, &t1case_ckpt, sizeof(t1case_ckpt)) !=
		static_cast<int>(sizeof(t1case_ckpt))
	) {
		close(fd);
		return false;
	}
	close(fd);
	return true;
}

// Captures this process segment's checkpoint and appends it to the array.
//
// [index] is the process sequence, and that identity is the whole basis for
// recomputing `record_count` at restore instead of storing it. So it is CHECKED
// here rather than assumed: every process segment ends with exactly one control
// packet, so `record_count - sample_count` must equal the number of segments
// already closed. If that ever stops holding - a second control code emitted
// mid-segment would do it - the restore would silently resume with the wrong
// record cursor, and a checkpoint that restores 90% of a cursor is worse than
// no checkpoint at all. Fail closed, as REPLAY_CORE_CONTRACT.md 5.1 requires of
// every regenerated field.
static bool t1case_checkpoint_write(uint16_t index)
{
	int fd;

	if(t1case_record_count != (t1case_sample_count + index)) {
		return false;
	}
	if(index >= T1CASE_CHECKPOINT_CAP) {
		// Not an error. Every sample is still recorded and every earlier
		// checkpoint still restores; the case is simply not resumable past
		// here, and says so.
		t1case_header.flags |= T1CASE_FLAG_CHECKPOINTS_FULL;
		return true;
	}
	t1case_ckpt.sample_count = t1case_sample_count;
	t1case_ckpt.global_frame = t1case_global_frame;
	t1case_ckpt.input_byte_count = t1case_input_byte_count;
	t1case_ckpt.payload_checksum = t1case_payload_checksum;
	t1case_startup_capture(&t1case_ckpt.startup);

	fd = t1f_update(T1CASE_BIN_FN);
	if(fd < 0) {
		return false;
	}
	lseek(fd, static_cast<long>(
		static_cast<uint32_t>(T1CASE_CHECKPOINT_ARRAY_OFFSET) +
		(static_cast<uint32_t>(index) * T1CASE_CHECKPOINT_STRIDE)
	), SEEK_SET);
	if(!t1f_write(fd, &t1case_ckpt, sizeof(t1case_ckpt))) {
		close(fd);
		return false;
	}
	close(fd);

	// The slot is on disk BEFORE the header claims it, and the caller rewrites
	// the header afterwards. A process killed between the two leaves a case
	// whose array is one slot shorter than it could have been, never one whose
	// last declared slot is garbage.
	t1case_header.checkpoint_count = static_cast<uint8_t>(index + 1);
	t1case_checkpoint_checksum = t1case_fnv1a(
		t1case_checkpoint_checksum, &t1case_ckpt, sizeof(t1case_ckpt)
	);
	t1case_header.checkpoint_checksum = t1case_checkpoint_checksum;
	return true;
}

// Starts this playback at checkpoint [index] instead of at the case's own
// beginning.
// Folds every declared slot and compares against the header. Without this the
// array is the one region of the container no checksum covers, and a slot
// corrupted in place restores CLEANLY: its cursors are in range and
// t1case_startup_verify() compares the resident against the same block that
// was just applied to it, so it agrees with the corruption.
static bool t1case_checkpoint_array_verify(void)
{
	uint32_t hash = T1CASE_FNV1A_BASIS;
	int i;

	for(i = 0; i < t1case_header.checkpoint_count; i++) {
		if(!t1case_checkpoint_read(static_cast<uint8_t>(i))) {
			return false;
		}
		hash = t1case_fnv1a(hash, &t1case_ckpt, sizeof(t1case_ckpt));
	}
	return (hash == t1case_header.checkpoint_checksum);
}

static bool t1case_checkpoint_restore(uint8_t index)
{
	uint32_t records;

	if(!t1case_checkpoint_array_verify()) {
		return false;
	}
	if(!t1case_checkpoint_read(index)) {
		return false;
	}

	// `record_count` is REGENERATED rather than stored, so every bound it must
	// satisfy is checked before anything is applied.
	records = (t1case_ckpt.sample_count + index);
	if(
		(records > t1case_header.record_count) ||
		(t1case_ckpt.sample_count > t1case_header.sample_count) ||
		(t1case_ckpt.input_byte_count > t1case_header.payload_size) ||

		// [measured] A TH01 BINDING CHECK, not a core rule. t1case_frame_io()
		// increments [t1case_sample_count] and [t1case_global_frame] exactly
		// once each per call and nothing else writes either, so on TH01 the two
		// are the same number - TH03 separates them only because MAINL throttles
		// samples to hardware VSync (th03/mainl/replml.cpp:1359). Asserting a
		// redundancy is free and catches a mutated cursor by name; relying on it
		// to SAVE four bytes would not be, which is why the field is stored.
		(t1case_ckpt.global_frame != t1case_ckpt.sample_count)
	) {
		return false;
	}

	t1case_sample_count = t1case_ckpt.sample_count;
	t1case_global_frame = t1case_ckpt.global_frame;
	t1case_input_byte_count = t1case_ckpt.input_byte_count;
	t1case_payload_checksum = t1case_ckpt.payload_checksum;
	t1case_record_count = records;

	// The checkpoint's startup block REPLACES the case's own for the rest of
	// this process, so that t1case_startup_apply() and t1case_startup_verify()
	// both act on the state this run is actually resuming into, with no special
	// case in either. Safe because a checkpoint start is playback-only and the
	// playback path never writes the header back.
	t1case_startup = t1case_ckpt.startup;
	t1case_startup_apply();

	// Already `started`: the trace must be comparable row for row against the
	// tail of a full playback, and a full playback's process k emits
	// ROUND_START, not START. The verify is scheduled separately rather than
	// dropped with it - see t1case_round_start().
	t1case_started = true;
	t1case_ckpt_verify_pending = true;
	return true;
}

/// Public seam
/// -----------

int far t1case_key_sense(int keygroup)
{
	if(t1case_mode == T1CASE_DISABLED) {
		return key_sense(keygroup);
	}
	switch(keygroup) {
	case 0: return t1case_keys[T1CASE_GI_0];
	case 3: return t1case_keys[T1CASE_GI_3];
	case 5: return t1case_keys[T1CASE_GI_5];
	case 6: return t1case_keys[T1CASE_GI_6];
	case 7: return t1case_keys[T1CASE_GI_7];
	case 8: return t1case_keys[T1CASE_GI_8];
	case 9: return t1case_keys[T1CASE_GI_9];
	}
	// Not part of TH01's gameplay path; fall through to the real routine so a
	// future consumer cannot silently read a stale latch.
	return key_sense(keygroup);
}

void far t1case_frame_io(uint8_t near *prev)
{
	uint8_t phase;

	t1case_input_prev = prev;
	if((t1case_mode == T1CASE_DISABLED) || (t1case_mode == T1CASE_ERROR)) {
		return;
	}

	// A record belongs to the gameplay phase exactly when the main gameplay
	// loop is live. [timer_initialized] is set at th01/main_01.cpp:795, right
	// after the wait-for-shot interstitial and right before the loop, and
	// cleared at :905 when the loop exits.
	phase = (
		(timer_initialized == true) ?
		T1CASE_PHASE_GAMEPLAY : T1CASE_PHASE_INTERSTITIAL
	);

	if(t1case_mode == T1CASE_RECORD) {
		// Sense every group, in the doubled-and-OR'd shape ZUN uses as a
		// PC-98 keyboard-UART guard, so a recording and a playback drive the
		// game through byte-identical code below this point.
		t1case_keys[T1CASE_GI_0] = static_cast<uint8_t>(key_sense(0) | key_sense(0));
		t1case_keys[T1CASE_GI_3] = static_cast<uint8_t>(key_sense(3) | key_sense(3));
		t1case_keys[T1CASE_GI_5] = static_cast<uint8_t>(key_sense(5) | key_sense(5));
		t1case_keys[T1CASE_GI_6] = static_cast<uint8_t>(key_sense(6) | key_sense(6));
		t1case_keys[T1CASE_GI_7] = static_cast<uint8_t>(key_sense(7) | key_sense(7));
		t1case_keys[T1CASE_GI_8] = static_cast<uint8_t>(key_sense(8) | key_sense(8));
		t1case_keys[T1CASE_GI_9] = static_cast<uint8_t>(key_sense(9) | key_sense(9));

		if(!t1case_encode_sample(phase)) {
			t1case_input_error(T1T_ERR_FRAME_IO);
			return;
		}
		t1case_record_count++;
		t1case_sample_count++;
	} else {
		if(t1case_record_count >= t1case_header.record_count) {
			t1case_split_row(T1SPLIT_EVENT_INPUT_END);
			t1case_mode = T1CASE_DISABLED;
			t1case_handoff_clear();
			t1case_done_write(T1T_OK_INPUT_END);
			return;
		}
		t1case_stream_io_error = false;
		t1case_control_pending = false;
		if(!t1case_decode_sample(phase)) {
			t1case_input_error(
				t1case_stream_io_error ? T1T_ERR_FRAME_IO :
				(t1case_control_pending ? T1T_ERR_CONTROL_EARLY : T1T_ERR_DESYNC)
			);
			return;
		}
		t1case_record_count++;
		t1case_sample_count++;
	}

	t1case_global_frame++;

	// The savestate checkpoint. Deliberately BEFORE the trace checkpoint and on
	// its own cadence constant. Gameplay is never interrupted by the verdict:
	// the only effect of a detection is that the case stops being saveable, and
	// the evidence lands in T1DIAG.TXT.
	if(
		(t1case_mode == T1CASE_RECORD) &&
		((t1case_global_frame & (T1CASE_GUARD_INTERVAL_SAMPLES - 1)) == 0)
	) {
		if(!t1prt_checkpoint(T1CASE_GUARD_FN)) {
			if(t1prt_invalid()) {
				t1prt_guard_marker_set(T1CASE_GUARD_FN); // *** THE POISON ***
			}
			t1prt_diag_emit();
		}
	}
	if((t1case_global_frame & (T1SPLIT_INTERVAL_SAMPLES - 1)) == 0) {
		t1case_split_row(T1SPLIT_EVENT_CHECKPOINT);
		if((t1case_mode == T1CASE_RECORD) && !t1case_header_write(false)) {
			t1case_input_error(T1T_ERR_FRAME_IO);
			return;
		}
	}
	t1case_handoff_store();
}

void far t1case_session_start(void)
{
	bool resumed;
	bool carrier_done;

	t1case_paths_init();
	t1case_payload_checksum = T1CASE_FNV1A_BASIS;
	t1case_checkpoint_checksum = T1CASE_FNV1A_BASIS;

	// The sector buffer belongs to THIS process. TH01 self-`execl`s 8-10 times
	// per run, so a segment inherited from the previous image would be 8-10
	// chances to free memory this process does not own.
	t1prt_local_reset();

	// Codec state, explicitly, even though BSS starts zeroed: this is a fresh
	// PROCESS SEGMENT in both directions. Neither the encoder's delta basis nor
	// the decoder's channel values may carry over from the previous process,
	// which is exactly why the control packet forces the next packet to be a
	// keyframe (REPLAY_CORE_CONTRACT.md §4).
	t1case_input_byte_count = 0;
	t1case_wbuf_len = 0;
	t1case_packet_open = false;
	t1case_enc_prev_valid = false;
	t1case_packet_run = 0;
	t1case_packet_phase = T1CASE_PHASE_GAMEPLAY;
	t1case_packet_at = 0;
	t1case_rbuf_len = 0;
	t1case_rbuf_pos = 0;
	t1case_dec_run = 0;
	t1case_dec_phase = T1CASE_PHASE_GAMEPLAY;
	t1case_dec_prev_valid = false;
	t1case_stream_io_error = false;
	t1case_control_pending = false;
	t1case_process_mismatch = false;
	t1case_ckpt_index = 0;
	t1case_ckpt_verify_pending = false;

	// The carrier wins; T1CASE.CFG is only the first-process fallback
	// (REPLAY_CORE_CONTRACT.md §7, landmine 2). Three outcomes, not two:
	//
	//   valid + RECORD/PLAYBACK -> resume this process from the carrier
	//   valid + DONE            -> the case is OVER; stay disabled and do NOT
	//                              consult T1CASE.CFG, or the case restarts
	//   absent or invalid       -> first process; T1CASE.CFG decides
	//
	// Validity is the magic plus the carrier version, not `id[0] == 'T'`: the
	// ID only proves master.lib found a block under that name, which a stale
	// block from an older mod build also does.
	carrier_done = false;
	if(t1case_res_open(false) && t1case_res_magic_ok()) {
		if(t1case_res->flags & T1CASE_RES_FLAG_DONE) {
			carrier_done = true;
			t1case_mode = T1CASE_DISABLED;
		} else {
			t1case_mode = t1case_res->mode;
			if(
				(t1case_mode != T1CASE_RECORD) &&
				(t1case_mode != T1CASE_PLAYBACK)
			) {
				t1case_mode = T1CASE_DISABLED;
			}
		}
	} else {
		t1case_res = 0;
		t1case_mode = T1CASE_DISABLED;
	}
	resumed = (t1case_mode != T1CASE_DISABLED);
	if(!resumed && !carrier_done) {
		t1case_mode = t1case_cfg_mode();
	}
	if(t1case_mode == T1CASE_SELFTEST) {
		t1prt_verdict_selftest();
		t1case_mode = T1CASE_DISABLED;
		return;
	}
	if(t1case_mode == T1CASE_DISABLED) {
		return;
	}
	if(!resumed && !t1case_res_open(true)) {
		t1case_mode = T1CASE_DISABLED;
		t1case_done_write(T1T_ERR_RESIDENT);
		return;
	}
	if(!t1case_resident_ensure()) {
		t1case_mode = T1CASE_DISABLED;
		t1case_handoff_clear();
		t1case_done_write(T1T_ERR_RESIDENT);
		return;
	}

	// ORACLE DEVIATION, deliberate and recorded: an active case always runs
	// with BGM off.
	//
	// [emu] REIIDEN's `mdrv2_resident()` gate (th01/main_01.cpp) passes in the
	// first process but fails in the second: after a continue-menu `execl`, the
	// INT 0xF2 vector still points somewhere (measured 0912:21AC) but the
	// "Mdrv2System" magic at that segment is gone, so the driver's memory was
	// reused across the handoff. An oracle whose multi-process cases depend on
	// a TSR surviving `execl` is not an oracle.
	//
	// Forcing BGM_MODE_OFF is sufficient AND safe: `mdrv2_active` starts false
	// (th01/snd/mdrv2.cpp:42) and is only ever set by
	// `mdrv2_enable_if_board_installed()` (:147), which main() calls only under
	// `bgm_mode == BGM_MODE_MDRV2`. Every other entry point is guarded by
	// `if(mdrv2_active)`, so no `geninterrupt(0xF2)` can reach the dangling
	// vector. Audio is explicitly outside the trace schema
	// (TXSPLIT_CONTRACT.md §7: no renderer or audio bytes), and this is applied
	// identically while recording and while playing back, so it cannot make a
	// case disagree with its own lineage.
	resident->bgm_mode = BGM_MODE_OFF;
	if(resumed) {
		t1case_handoff_load();
	}
	t1case_diag(
		'S', 'E', 'S', t1case_mode, (resumed ? 1UL : 0UL)
	);

	// The two carrier facts SES cannot show: which process of the chain this is,
	// and where in the packet stream it resumes. The byte cursor is the one
	// step 1 added and the one no diagnostic has ever exposed, so a handoff
	// checker could not see the third cursor at all.
	t1case_diag(
		'S', 'E', 'Q',
		(t1case_res ? static_cast<uint32_t>(t1case_res->process_seq) : 0UL),
		t1case_input_byte_count
	);

	// The checkpoint alignment pair: the trace row index and the sample cursor
	// this process segment starts at. A gate that compares a checkpoint-started
	// run against the TAIL of a full run needs both, and no diagnostic carried
	// the row index before - so the comparison could not have been made at all.
	t1case_diag('C', 'K', 'P', t1case_split_rows, t1case_sample_count);

	if(t1case_mode == T1CASE_PLAYBACK) {
		if(!t1case_header_read()) {
			t1case_mode = T1CASE_ERROR;
			t1case_handoff_clear();
			t1case_done_write(T1T_ERR_CASE_HEADER);
			return;
		}
		if(!resumed) {
			if(t1case_cfg_checkpoint != 0) {
				if(!t1case_checkpoint_restore(t1case_cfg_checkpoint)) {
					t1case_mode = T1CASE_ERROR;
					t1case_handoff_clear();
					t1case_done_write(T1T_ERR_CHECKPOINT);
					return;
				}
				t1case_ckpt_index = t1case_cfg_checkpoint;
				t1case_diag(
					'C', 'K', 'R', t1case_ckpt_index, t1case_record_count
				);
			} else {
				t1case_startup_apply();
			}
		}
	} else if(!resumed) {
		t1case_startup_capture(&t1case_startup);
		t1case_memclear(&t1case_header, sizeof(t1case_header));
		t1case_header.magic[0] = 'T';
		t1case_header.magic[1] = '1';
		t1case_header.magic[2] = 'C';
		t1case_header.magic[3] = 'A';
		t1case_header.magic[4] = 'S';
		t1case_header.magic[5] = 'E';
		t1case_header.magic[6] = '1';
		t1case_header.magic[7] = '\0';
		t1case_header.version = T1CASE_VERSION;
		t1case_header.header_size = T1CASE_HEADER_SIZE;
		t1case_header.startup_size = T1CASE_STARTUP_SIZE;
		t1case_header.channel_count = T1CASE_GROUP_COUNT;
		t1case_header.checkpoint_capacity = T1CASE_CHECKPOINT_CAP;
		t1case_header.checkpoint_count = 0;
		t1case_header.checkpoint_stride = T1CASE_CHECKPOINT_STRIDE;
		t1case_header.checkpoint_checksum = T1CASE_FNV1A_BASIS;
		t1case_header.payload_offset = (
			static_cast<uint32_t>(T1CASE_CHECKPOINT_ARRAY_OFFSET) +
			(static_cast<uint32_t>(T1CASE_CHECKPOINT_CAP) *
				T1CASE_CHECKPOINT_STRIDE)
		);
		t1case_header.source_kind = T1CASE_SOURCE_DIRECT;
		t1case_header.input_semantics = 1;
		t1case_header.ruleset_id = 1;
		t1case_header.scenario_id = 0;
		t1case_header.first_process = T1CASE_PROCESS_REIIDEN;
		t1case_header.producer = T1CASE_PRODUCER_GAME_MOD;
	} else {
		// Resuming a recording in a later REIIDEN process: keep appending to
		// the case the first process created.
		if(!t1case_header_read()) {
			t1case_mode = T1CASE_ERROR;
			t1case_handoff_clear();
			t1case_done_write(T1T_ERR_CASE_HEADER);
			return;
		}
	}
	// Cross-check the resumed carrier against the case file's own header. This
	// MUST happen before the record path below rewrites that header from the
	// carrier's counters, which would make them agree by construction and turn
	// the check into a tautology.
	if(resumed && !t1case_handoff_verify()) {
		t1case_mode = T1CASE_ERROR;
		t1case_handoff_clear();
		t1case_done_write(T1T_ERR_HANDOFF);
		return;
	}
	if(t1case_mode == T1CASE_RECORD) {
		if(!t1case_header_write(!resumed)) {
			t1case_mode = T1CASE_ERROR;
			t1case_handoff_clear();
			t1case_done_write(T1T_ERR_CASE_CREATE);
			return;
		}

		// One checkpoint per process segment, indexed BY the process
		// sequence. The order is: header (so the file and its array exist),
		// then the slot, then the header again (so `checkpoint_count` only
		// ever claims slots that are already on disk).
		t1case_ckpt_index = static_cast<uint8_t>(
			t1case_res ? t1case_res->process_seq : 0
		);
		if(!t1case_checkpoint_write(
			t1case_res ? t1case_res->process_seq : 0
		)) {
			t1case_mode = T1CASE_ERROR;
			t1case_handoff_clear();
			t1case_done_write(T1T_ERR_CHECKPOINT);
			return;
		}
		if(!t1case_header_write(false)) {
			t1case_mode = T1CASE_ERROR;
			t1case_handoff_clear();
			t1case_done_write(T1T_ERR_CASE_CREATE);
			return;
		}
	}
	// The savestate guard belongs to a RUN, not to a process: creating it is
	// what resets the detector's sticky state, so it happens exactly once, in
	// the first process of a recording. A failure disables saving and is
	// recorded, but never interrupts the game.
	if((t1case_mode == T1CASE_RECORD) && !resumed) {
		if(!t1prt_guard_create(T1CASE_GUARD_FN)) {
			t1prt_diag_emit();
		}
	}
	if(!resumed) {
		if(!t1case_split_write_header()) {
			t1case_mode = T1CASE_ERROR;
			t1case_done_write(T1T_ERR_SPLIT_OPEN);
			return;
		}
	}
	if(t1case_res) {
		t1case_res->process_seq++;
	}
	t1case_handoff_store();
	t1case_diag('H', 'D', 'R', t1case_header.record_count, t1case_global_frame);
}

void far t1case_round_start(void)
{
	if((t1case_mode == T1CASE_DISABLED) || (t1case_mode == T1CASE_ERROR)) {
		return;
	}
	// The post-init verify runs for a fresh playback AND for a
	// checkpoint-started one. A checkpoint start is `started` from the first
	// instant, so that its trace is comparable row for row against the tail of
	// a full playback - but it is also the single place where the verify
	// matters most, because the state being checked was RESTORED rather than
	// played into. Letting `started` gate both would skip exactly the check
	// that covers the restore.
	if(
		(t1case_mode == T1CASE_PLAYBACK) &&
		(!t1case_started || t1case_ckpt_verify_pending) &&
		!t1case_startup_verify()
	) {
		t1case_ckpt_verify_pending = false;
		t1case_input_error(T1T_ERR_VERIFY);
		return;
	}
	t1case_ckpt_verify_pending = false;
	if(!t1case_started) {
		t1case_started = true;
		t1case_split_row(T1SPLIT_EVENT_START);
	} else {
		t1case_split_row(T1SPLIT_EVENT_ROUND_START);
	}
	t1case_handoff_store();
}

void far t1case_finish(bool16 terminal)
{
	uint8_t control;
	bool final_case;
	bool late = false;

	if((t1case_mode == T1CASE_DISABLED) || (t1case_mode == T1CASE_ERROR)) {
		return;
	}
	control = static_cast<uint8_t>(
		terminal ? T1CASE_CONTROL_TERMINAL : T1CASE_CONTROL_PROCESS_END
	);

	if(t1case_mode == T1CASE_RECORD) {
		if(!t1case_encode_control(control)) {
			t1case_input_error(T1T_ERR_FRAME_IO);
			return;
		}
		t1case_record_count++;

		// Commits the control packet, so [input_byte_count] in the carrier is a
		// packet boundary the next process can resume at
		// (REPLAY_CORE_CONTRACT.md §4.4 item 1).
		if(!t1case_header_write(false)) {
			t1case_input_error(T1T_ERR_FRAME_IO);
			return;
		}
	} else {
		t1case_stream_io_error = false;

		// A run that still owes samples is the LATE half of §7.2: the game
		// reached its process boundary before the recording did. Sampled before
		// the decode, because a successful decode requires [dec_run] == 0.
		late = (t1case_dec_run != 0);
		t1case_process_mismatch = false;
		if(!t1case_decode_control(control)) {
			t1case_input_error(
				t1case_stream_io_error ? T1T_ERR_FRAME_IO :
				(t1case_process_mismatch ? T1T_ERR_PROCESS :
					(late ? T1T_ERR_CONTROL_LATE : T1T_ERR_DESYNC))
			);
			return;
		}
		t1case_record_count++;
	}

	t1case_split_row(T1SPLIT_EVENT_FINISH);
	t1case_diag('F', 'I', 'N', t1case_record_count, t1case_global_frame);

	// The byte cursor as the outgoing process leaves it, so the next process's
	// SEQ line can be checked against it. On the record path the flush above has
	// already committed the control packet, so this is a packet boundary
	// (REPLAY_CORE_CONTRACT.md §4.4 item 1) and not merely a byte count.
	t1case_diag('F', 'B', 'C', t1case_input_byte_count, t1case_sample_count);

	final_case = (
		(terminal != false) ||
		((t1case_mode == T1CASE_PLAYBACK) && t1case_playback_final())
	);
	if(final_case) {
		if((t1case_mode == T1CASE_PLAYBACK) && !t1case_playback_final()) {
			t1case_input_error(T1T_ERR_DESYNC);
			return;
		}
		t1case_handoff_clear();
		if(t1case_mode == T1CASE_RECORD) {
			t1prt_diag_emit();
		}
		t1prt_local_free();
		t1case_done_write(
			(t1case_mode == T1CASE_RECORD) ? T1T_OK_RECORD : T1T_OK_PLAYBACK
		);
		t1case_mode = T1CASE_DISABLED;
		return;
	}
	t1case_handoff_store();
}
