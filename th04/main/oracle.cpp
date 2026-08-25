#pragma option -zCORACLE_TEXT

// T4CASE1 / T5CASE1 verifier for TH04 and TH05 MAIN.
//
// Validation-only mod code. It is NOT part of any bit-identical match branch
// and the binaries it produces are intentionally nonmatching. Nothing here may
// be described as matching an original binary.
//
// Two modes, selected by the first non-blank character of T4CASE.CFG /
// T5CASE.CFG:
//
//   r  record   run ZUN's own attract demo, capture the audited startup
//               description plus every injected sample into T?CASE.BIN, and
//               emit T?SPLIT.BIN
//   p  playback apply the recorded startup, inject the recorded input stream,
//               verify frame alignment, and emit T?SPLIT.BIN
//
// Absent or unrecognized config leaves the game completely untouched.
//
// Record mode doubles as the normalizer. ZUN's DEMO?.REC files are members of
// the retail packfile (`th03/formats/pfopen.asm:54-66`; the directory is
// encrypted and member payloads are RLE-compressed, which is why a plain grep
// never found them), and master.lib's INT 21h hook
// (`libs/master.lib/pfint21.asm`) serves them transparently through
// `file_ropen()`. So the game itself reads the stock demo and writes the
// normalized case, and no original bytes ever leave `originals/`.
//
// Playback deliberately re-derives everything it can instead of restoring a
// memory image. There is no runtime snapshot: the scenario start is pinned by
// ZUN's own code (`th04_main.asm:689-698`, `th05_main.asm:769-780`), so the
// startup block is written *before* `demo_load()` computes anything from it,
// and the game's own initialization then runs unmodified.

// Most ReC98 game headers carry no include guards, so this list has to be
// exact rather than defensive: including a header that another one already
// pulled in is a hard error ("Variable ... is initialized more than once").
// Three headers this file needs are therefore NOT listed, because a listed one
// already supplies them:
//
//   th04/hardware/inputvar.h  (`key_det`, `shiftkey`)
//                                <- th04/main/player/move.hpp
//                                     -> th04/hardware/input.h
//   th04/main/player/player.hpp  (`player_pos`, `power`, `POWER_MAX`)
//                                <- th04/main/player/shot.hpp
//   th02/math/randring.hpp  (`randring[]`)
//                                <- th04/main/player/shot.hpp
//                                     -> th04/math/randring.hpp
#include "platform.h"
#include "libs/master.lib/master.hpp"
#include "th04/common.h"
#include "th04/end/end.h"
#include "th04/formats/std.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/custom.hpp"
#include "th04/main/demo.hpp"
#include "th04/main/ems.hpp"
#include "th04/main/frames.h"
#include "th04/main/gather.hpp"
#include "th04/main/item/splash.hpp"
#include "th04/main/midboss/midboss.hpp"
#include "th04/main/oracle.hpp"
#include "th04/main/playperf.hpp"
#include "th04/main/player/bomb.hpp"
#include "th04/main/player/move.hpp"
#include "th04/main/player/shot.hpp"
#include "th04/main/pointnum/pointnum.hpp"
#include "th04/main/quit.hpp"
#include "th04/main/rank.hpp"
#include "th04/main/score.hpp"
#include "th04/main/stage/stage.hpp"
#include "th04/oracle_build.hpp"
#if (GAME == 5)
	#include "th05/main/boss/boss.hpp"
	#include "th05/main/enemy/enemy.hpp"
	#include "th05/playchar.h"
	#include "th05/resident.hpp"
#else
	#include "th04/main/boss/boss.hpp"
	#include "th04/main/enemy/enemy.hpp"
	#include "th04/playchar.h"
	#include "th04/resident.hpp"
#endif
#include "th04/main/boss/backdrop.hpp"
#include "th04/main/boss/explode.hpp"

// `th02/math/randring[bss].asm:10-15`: a *word*-sized cursor whose low byte
// alone is incremented by the TH04/TH05 accessors
// (`th04/math/randring.inc:1-33`). Hash the full uint16; never reuse TH02's
// byte-sized serializer.
extern uint16_t randring_p;

// `th04/main/play[bss].asm`, `th04_main.asm:30125`, `th05_main.asm:20203`.
extern uint8_t power;

// `th04/main/bullet/update[bss].asm:16-18`.
extern uint16_t stage_graze;

// `th04/score[data].asm:1-2`.
extern uint8_t extends_gained;

// The linked BSS spelling; `item.hpp`'s longer declaration has no definition.
// Existing runtime users (`item/collect.cpp`, `execl.cpp`, `replay.cpp`) all
// publish and consume this spelling.
extern unsigned int total_max_valued_point_items;

// `th04/main/item/splash_u.cpp`; intentionally absent from the public header
// until another runtime consumer needed it. The oracle serializes the ring
// cursor because it decides which splash slot the next item spawn reuses.
extern unsigned char item_splash_last_id;

#if (GAME == 5)
	// `th05_main.asm:19959-19960`. TH05 has no `rem_lives` / `rem_bombs` in
	// `resident_t`; those counters are MAIN-local. This closes the `[open]`
	// item in `state/re/DETERMINISTIC_STATE_TH05.md` §3.
	extern uint8_t lives;
	extern uint8_t bombs;

	// `th05/main/dialog/dialog.cpp:248`. Deterministic gameplay state for a
	// TH05 demo case: it is what distinguishes "before the Extra splice" from
	// "after".
	extern int8_t dialog_sequence_id;
#endif

/// Module state
/// ------------
/// None of this is initialized data. A `_DATA` contribution from this module
/// would be placed between the original `_DATA` and `_BSS` inside DGROUP and
/// would shift every original BSS offset. TH04/TH05 carry no hardcoded raw BSS
/// offsets the way TH03 does, but the rule in
/// kb/conventions/th03-mod-layout-verification.md is cheap to honour and the
/// built map is checked against it, so filenames are assembled and status text
/// is emitted one character at a time.

static char ORACLE_CFG_FN[11];
static char ORACLE_BIN_FN[11];
static char ORACLE_SPLIT_FN[12];
static char ORACLE_DONE_FN[11];
static char ORACLE_DIAG_FN[11];
static bool oracle_paths_ready;

enum oracle_text_id_t {
	ORT_OK_RECORD = 0,
	ORT_OK_PLAYBACK,
	ORT_OK_INPUT_END,
	ORT_ERR_CASE_HEADER,
	ORT_ERR_CASE_CREATE,
	ORT_ERR_CASE_FINALIZE,
	ORT_ERR_FRAME_IO,
	ORT_ERR_DESYNC,
	ORT_ERR_STARTUP,
	ORT_ERR_SPLIT_OPEN,
	ORT_ERR_UNSUPPORTED
};

enum oracle_mode_t {
	ORACLE_DISABLED = 0,
	ORACLE_RECORD   = 1,
	ORACLE_PLAYBACK = 2,
	ORACLE_ERROR    = 3
};

// Serialized identity of the frame-loop injection hook. Never its address:
// the callback slot is pointer-shaped state and differs between lineages.
#define ORACLE_HOOK_DEMO 1

// A whole record buffer, so the frame loop performs one file operation every
// [ORACLE_RECBUF_COUNT] frames instead of one per frame. Per-frame disk I/O
// would be host-timing-dependent work inside the measurement window.
#define ORACLE_RECBUF_COUNT 64

static oracle_header_t oracle_header;
static oracle_startup_t oracle_startup;
static oracle_record_t oracle_recbuf[ORACLE_RECBUF_COUNT];
static uint16_t oracle_recbuf_len;  // valid entries in [oracle_recbuf]
static uint16_t oracle_recbuf_pos;  // playback read cursor within the buffer
static uint32_t oracle_recbuf_base; // record index of oracle_recbuf[0]
static oracle_mode_t oracle_mode;
static uint32_t oracle_global_frame;
static uint32_t oracle_sample_count;
static uint32_t oracle_record_count;
static uint32_t oracle_payload_checksum;
static uint32_t oracle_split_size;
static uint32_t oracle_diag_size;
static bool oracle_started;
static bool oracle_done_written;
static bool oracle_finished;
/// ------------

/// Raw INT 21h file I/O
/// --------------------
/// TH04's MAIN links master.lib's write side (`th04_main.asm:69-90` has
/// `dos_axdx`, `file_append`, `file_create`, `file_write`, `file_seek`,
/// `file_exist`), but TH05's does NOT (`th05_main.asm:60-95` has only the read
/// side). Adding those procedures would mean growing an original segment
/// contribution, which CLAUDE.md forbids outright. A private INT 21h layer
/// therefore keeps ONE shared implementation across both games and touches no
/// ASM at all. It also uses its own DOS handle, so it never disturbs
/// master.lib's single global file state — a robustness gain over the TH03
/// verifier, which had to interleave with `file_ropen()`'s handle.
///
/// This is safe while `pfstart()` is active. `libs/master.lib/pfint21.asm`
/// intercepts an open only in read mode with no packfile member currently open
/// (`_Open`: `test AL,0fh` then `or DI,DI`), and passes read, write, seek and
/// close straight through whenever `BX` does not match its own handle.

#define ORACLE_FP_SEG(p) ((unsigned)(((unsigned long)(void far *)(p)) >> 16))
#define ORACLE_FP_OFF(p) ((unsigned)((unsigned long)(void far *)(p)))

#define ORACLE_ACCESS_READ 0
#define ORACLE_ACCESS_RW   2

static int oracle_dos_open(const char far *fn, unsigned char access)
{
	unsigned fn_seg = ORACLE_FP_SEG(fn);
	unsigned fn_off = ORACLE_FP_OFF(fn);
	int result;

	_asm {
		push	ds
		mov	dx, fn_off
		mov	ds, fn_seg
		mov	ah, 3Dh
		mov	al, access
		int	21h
		pop	ds	/* POP does not touch flags, so CF survives */
		sbb	dx, dx	/* -1 if CF, 0 otherwise */
		or	ax, dx
		mov	result, ax
	}
	return result;
}

static int oracle_dos_create(const char far *fn)
{
	unsigned fn_seg = ORACLE_FP_SEG(fn);
	unsigned fn_off = ORACLE_FP_OFF(fn);
	int result;

	_asm {
		push	ds
		mov	dx, fn_off
		mov	ds, fn_seg
		mov	ah, 3Ch
		xor	cx, cx
		int	21h
		pop	ds
		sbb	dx, dx
		or	ax, dx
		mov	result, ax
	}
	return result;
}

static void oracle_dos_close(int fh)
{
	_asm {
		mov	bx, fh
		mov	ah, 3Eh
		int	21h
	}
}

static unsigned oracle_dos_read(int fh, void far *buf, unsigned len)
{
	unsigned buf_seg = ORACLE_FP_SEG(buf);
	unsigned buf_off = ORACLE_FP_OFF(buf);
	unsigned result;

	_asm {
		push	ds
		mov	bx, fh
		mov	cx, len
		mov	dx, buf_off
		mov	ds, buf_seg
		mov	ah, 3Fh
		int	21h
		pop	ds
		sbb	cx, cx
		not	cx	/* 0 on error, 0FFFFh on success */
		and	ax, cx
		mov	result, ax
	}
	return result;
}

static unsigned oracle_dos_write(int fh, const void far *buf, unsigned len)
{
	unsigned buf_seg = ORACLE_FP_SEG(buf);
	unsigned buf_off = ORACLE_FP_OFF(buf);
	unsigned result;

	_asm {
		push	ds
		mov	bx, fh
		mov	cx, len
		mov	dx, buf_off
		mov	ds, buf_seg
		mov	ah, 40h
		int	21h
		pop	ds
		sbb	cx, cx
		not	cx
		and	ax, cx
		mov	result, ax
	}
	return result;
}

static bool oracle_dos_seek(int fh, uint32_t pos)
{
	unsigned pos_hi = static_cast<unsigned>(pos >> 16);
	unsigned pos_lo = static_cast<unsigned>(pos & 0xFFFFUL);
	unsigned failed;

	_asm {
		mov	bx, fh
		mov	cx, pos_hi
		mov	dx, pos_lo
		mov	ax, 4200h
		int	21h
		sbb	ax, ax
		neg	ax
		mov	failed, ax
	}
	return (failed == 0);
}

// Opens for read/write, creating the file when it does not exist. The position
// is left at zero, so a caller that wants to rewrite a header simply writes,
// and a caller that wants to append seeks to a size it tracks itself.
static int oracle_dos_open_rw(const char far *fn)
{
	int fh = oracle_dos_open(fn, ORACLE_ACCESS_RW);

	if(fh < 0) {
		fh = oracle_dos_create(fn);
	}
	return fh;
}
/// --------------------

// Stack objects are SS-relative in this memory model, so every helper that a
// caller may hand a local to takes a far pointer.
static void oracle_memclear(void far *buf, unsigned size)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(buf);

	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

// Explicit, rather than a struct assignment. Assigning the whole struct emits
// a call to Turbo C++'s structure-copy helper, which TH04's MAIN links but
// TH05's does not — pulling it in would grow TH05's original `_TEXT`
// contribution, which the layout gate rightly rejects.
static void oracle_record_copy(oracle_record_t far *dst, const oracle_record_t far *src)
{
	uint8_t far *d = reinterpret_cast<uint8_t far *>(dst);
	const uint8_t far *s = reinterpret_cast<const uint8_t far *>(src);
	unsigned i;

	for(i = 0; i < ORACLE_RECORD_SIZE; i++) {
		d[i] = s[i];
	}
}

static void oracle_paths_init(void)
{
	if(oracle_paths_ready) {
		return;
	}
	ORACLE_CFG_FN[0] = 'T'; ORACLE_CFG_FN[1] = ORACLE_MAGIC_DIGIT;
	ORACLE_CFG_FN[2] = 'C'; ORACLE_CFG_FN[3] = 'A'; ORACLE_CFG_FN[4] = 'S';
	ORACLE_CFG_FN[5] = 'E'; ORACLE_CFG_FN[6] = '.'; ORACLE_CFG_FN[7] = 'C';
	ORACLE_CFG_FN[8] = 'F'; ORACLE_CFG_FN[9] = 'G'; ORACLE_CFG_FN[10] = '\0';
	ORACLE_BIN_FN[0] = 'T'; ORACLE_BIN_FN[1] = ORACLE_MAGIC_DIGIT;
	ORACLE_BIN_FN[2] = 'C'; ORACLE_BIN_FN[3] = 'A'; ORACLE_BIN_FN[4] = 'S';
	ORACLE_BIN_FN[5] = 'E'; ORACLE_BIN_FN[6] = '.'; ORACLE_BIN_FN[7] = 'B';
	ORACLE_BIN_FN[8] = 'I'; ORACLE_BIN_FN[9] = 'N'; ORACLE_BIN_FN[10] = '\0';
	ORACLE_SPLIT_FN[0] = 'T'; ORACLE_SPLIT_FN[1] = ORACLE_MAGIC_DIGIT;
	ORACLE_SPLIT_FN[2] = 'S'; ORACLE_SPLIT_FN[3] = 'P'; ORACLE_SPLIT_FN[4] = 'L';
	ORACLE_SPLIT_FN[5] = 'I'; ORACLE_SPLIT_FN[6] = 'T'; ORACLE_SPLIT_FN[7] = '.';
	ORACLE_SPLIT_FN[8] = 'B'; ORACLE_SPLIT_FN[9] = 'I'; ORACLE_SPLIT_FN[10] = 'N';
	ORACLE_SPLIT_FN[11] = '\0';
	ORACLE_DONE_FN[0] = 'T'; ORACLE_DONE_FN[1] = ORACLE_MAGIC_DIGIT;
	ORACLE_DONE_FN[2] = 'D'; ORACLE_DONE_FN[3] = 'O'; ORACLE_DONE_FN[4] = 'N';
	ORACLE_DONE_FN[5] = 'E'; ORACLE_DONE_FN[6] = '.'; ORACLE_DONE_FN[7] = 'T';
	ORACLE_DONE_FN[8] = 'X'; ORACLE_DONE_FN[9] = 'T'; ORACLE_DONE_FN[10] = '\0';
	ORACLE_DIAG_FN[0] = 'T'; ORACLE_DIAG_FN[1] = ORACLE_MAGIC_DIGIT;
	ORACLE_DIAG_FN[2] = 'D'; ORACLE_DIAG_FN[3] = 'I'; ORACLE_DIAG_FN[4] = 'A';
	ORACLE_DIAG_FN[5] = 'G'; ORACLE_DIAG_FN[6] = '.'; ORACLE_DIAG_FN[7] = 'T';
	ORACLE_DIAG_FN[8] = 'X'; ORACLE_DIAG_FN[9] = 'T'; ORACLE_DIAG_FN[10] = '\0';
	oracle_paths_ready = true;
}

/// Checksums and hashes
/// --------------------
/// FNV-1a/32, basis 0x811C9DC5, prime 0x01000193. Chosen over CRC32 because it
/// needs no lookup table in a constrained DOS build and is trivial to reproduce
/// on the host.

static uint32_t oracle_fnv1a(uint32_t hash, const void far *buf, unsigned size)
{
	const uint8_t far *p = reinterpret_cast<const uint8_t far *>(buf);

	while(size != 0) {
		hash ^= static_cast<uint32_t>(*p++);
		hash *= ORACLE_FNV1A_PRIME;
		size--;
	}
	return hash;
}

// The 64-bit subsystem hash of `state/port/TXSPLIT_CONTRACT.md` §7: two
// independent FNV-1a/32 passes over the same serialized byte sequence, so an
// 8086 needs only 32-bit arithmetic. Pass B's index XOR makes the two passes
// disagree on transpositions and on runs of equal bytes, which a same-basis
// pair would not. Both passes stream forward and accumulate in one loop.
//
// This is a divergence detector, not a cryptographic primitive. Fields are
// serialized in a fixed declared order with explicit widths; never a struct,
// never a padding byte, never memory directly.
// The accumulator lives in this module's BSS rather than in a caller's stack
// frame. In this memory model a stack object's address is a *far* pointer, so
// the version-1 shape charged every hashed byte two segment-override loads and
// two far stores. Version 2 serializes ~13 KB per row instead of ~300 bytes,
// which makes that overhead the difference between a slow frame and several.
// Only the storage location changes: the construction below is byte-for-byte
// the one in the harness TxSPLIT contract, so v1 and v2 agree on groups 0 and
// 1.
static uint32_t oracle_h_a;
static uint32_t oracle_h_b;
static uint8_t oracle_h_i;

static void oracle_hash_init(void)
{
	oracle_h_a = ORACLE_FNV1A_BASIS_A;
	oracle_h_b = ORACLE_FNV1A_BASIS_B;
	oracle_h_i = 0;
}

static void oracle_hash_u8(uint8_t value)
{
	oracle_h_a = (
		(oracle_h_a ^ static_cast<uint32_t>(value)) * ORACLE_FNV1A_PRIME
	);
	oracle_h_b = ((oracle_h_b ^ static_cast<uint32_t>(
		static_cast<uint8_t>(value ^ oracle_h_i)
	)) * ORACLE_FNV1A_PRIME);
	oracle_h_i++;
}

static void oracle_hash_u16(uint16_t value)
{
	oracle_hash_u8(static_cast<uint8_t>(value & 0xFF));
	oracle_hash_u8(static_cast<uint8_t>(value >> 8));
}

static void oracle_hash_u32(uint32_t value)
{
	oracle_hash_u16(static_cast<uint16_t>(value & 0xFFFFUL));
	oracle_hash_u16(static_cast<uint16_t>(value >> 16));
}

// A `PlayfieldMotion` / `MotionBase<>` is three points of two `Subpixel`s
// (`th04/math/motion.hpp:3-7`, `th01/math/subpixel.hpp:90-108`). Serialized as
// six explicit 16-bit fields in declaration order -- never as the struct, so
// the schema survives the alignment pragmas that the debloated lineage removes
// (`th04/main/player/move.cpp`, `th04/main/bullet/update.cpp`).
// Far parameters, like every other helper in this module: this is the large
// data model, so the address of a global is a far pointer unless that global was
// declared `near` (as `shots[]` is and `player_pos` is not). Near-to-far
// conversion is implicit and lossless, the reverse is not.
static void oracle_hash_motion(const PlayfieldMotion far *m)
{
	oracle_hash_u16(static_cast<uint16_t>(m->cur.x.v));
	oracle_hash_u16(static_cast<uint16_t>(m->cur.y.v));
	oracle_hash_u16(static_cast<uint16_t>(m->prev.x.v));
	oracle_hash_u16(static_cast<uint16_t>(m->prev.y.v));
	oracle_hash_u16(static_cast<uint16_t>(m->velocity.x.v));
	oracle_hash_u16(static_cast<uint16_t>(m->velocity.y.v));
}

static void oracle_hash_sppoint(const SPPoint far *p)
{
	oracle_hash_u16(static_cast<uint16_t>(p->x.v));
	oracle_hash_u16(static_cast<uint16_t>(p->y.v));
}

static void oracle_hash_playfield_point(const PlayfieldPoint far *p)
{
	oracle_hash_u16(static_cast<uint16_t>(p->x.v));
	oracle_hash_u16(static_cast<uint16_t>(p->y.v));
}

// Embedded bullet templates occur in enemies and gather circles as well as in
// the global spawn template. This helper serializes only the template itself;
// the global callback slots and special parameters remain in group 3.
static void oracle_hash_bullet_template(const BulletTemplate far *tmpl)
{
	oracle_hash_u8(tmpl->spawn_type);
	oracle_hash_u8(tmpl->patnum);
	oracle_hash_playfield_point(&tmpl->origin);
	oracle_hash_u8(static_cast<uint8_t>(tmpl->group));
	oracle_hash_u8(static_cast<uint8_t>(tmpl->special_motion));
	oracle_hash_u8(tmpl->angle);
	oracle_hash_u8(tmpl->speed.v);
#if (GAME == 5)
	oracle_hash_u8(tmpl->spread);
	oracle_hash_u8(tmpl->spread_angle_delta);
	oracle_hash_u8(tmpl->stack);
	oracle_hash_u8(tmpl->stack_speed_delta.v);
#else
	oracle_hash_playfield_point(&tmpl->velocity);
	oracle_hash_u8(tmpl->count);
	oracle_hash_u8(tmpl->delta.spread_angle);
#endif
}

// A near function pointer is pointer-shaped state and is NEVER hashed as an
// address (see the harness trace and TH04 deterministic-state contracts). None of
// these slots has a stable cross-lineage enum -- the debloated lineage relinks
// every one of them at a different offset -- so the honest serialization is the
// only lineage-independent fact about them: whether a handler is installed.
static uint8_t oracle_hook_installed(nearfunc_t_near f)
{
	return ((f != 0) ? 1 : 0);
}

static void oracle_hash_store(oracle_split_hash_t far *out)
{
	out->pass_a = oracle_h_a;
	out->pass_b = oracle_h_b;
}

// Group 0 — RNG. `random_seed` (4), `randring[256]`, `randring_p` (2).
static void oracle_hash_group_rng(oracle_split_hash_t far *out)
{
	int i;

	oracle_hash_init();
	oracle_hash_u32(static_cast<uint32_t>(random_seed));
	for(i = 0; i < ORACLE_RANDRING_SIZE; i++) {
		oracle_hash_u8(randring[i]);
	}
	oracle_hash_u16(randring_p);
	oracle_hash_store(out);
}

// `std_ip` is a far pointer into the loaded `.STD` heap segment
// (`th04/formats/std.hpp:6,23`) and must be hashed as an offset from
// `std_seg`, never as a pointer. 0xFFFFFFFF means "no `.STD` loaded", which is
// distinguishable from offset 0.
static uint32_t oracle_std_ip_offset(void)
{
	// `th05/formats/std.cpp:58` establishes this cast for a `__seg` pointer.
	uint16_t seg = reinterpret_cast<uint16_t>(std_seg);

	if(seg == 0) {
		return 0xFFFFFFFFUL;
	}
	return (
		(static_cast<uint32_t>(
			static_cast<uint16_t>(ORACLE_FP_SEG(std_ip) - seg)
		) << 4) +
		static_cast<uint32_t>(ORACLE_FP_OFF(std_ip))
	);
}

// Group 1 — run and scenario counters.
//
// `total_slow_frames` and `resident->slow_frames` are deliberately EXCLUDED.
// They count frames whose rendering exceeded the vsync time
// (`th04/main/frames.h:12-15`), i.e. host performance rather than game state.
// Hashing them would make the trace a stopwatch and every comparison
// host-dependent. They stay recorded once, in the startup block, because there
// they are a scenario input rather than a per-frame measurement.
static void oracle_hash_group_run(oracle_split_hash_t far *out)
{
	oracle_hash_init();
	oracle_hash_u8(stage_id);
	oracle_hash_u8(rank);
	oracle_hash_u16(stage_frame);
	oracle_hash_u8(stage_frame_mod2);
	oracle_hash_u8(stage_frame_mod4);
	oracle_hash_u8(stage_frame_mod8);
	oracle_hash_u8(stage_frame_mod16);
	oracle_hash_u32(total_frames);
	oracle_hash_u16(total_std_frames);
	oracle_hash_u32(frames_unused);
	oracle_hash_u16(resident->std_frames);
	oracle_hash_u16(resident->items_spawned);
	oracle_hash_u16(resident->items_collected);
	oracle_hash_u16(resident->point_items_collected);
	oracle_hash_u16(resident->max_valued_point_items_collected);
	oracle_hash_u16(resident->enemies_gone);
	oracle_hash_u16(resident->enemies_killed);
	oracle_hash_u16(resident->graze);
	oracle_hash_u8(resident->miss_count);
	oracle_hash_u8(resident->bombs_used);
	oracle_hash_u8(resident->end_sequence);
	oracle_hash_u32(resident->frames);
	oracle_hash_u16(stage_graze);
	oracle_hash_u8(extends_gained);
	oracle_hash_u32(score_delta);
	oracle_hash_u32(oracle_std_ip_offset());
#if (GAME == 5)
	// TH05 pre-doubles the section ID so it can be used directly as a byte
	// offset (`th04/formats/std.hpp:8-15`). Serialize the *semantic* ID, or the
	// two games' hashes disagree for identical state.
	oracle_hash_u16(static_cast<uint16_t>(std_map_section_p / 2));
	oracle_hash_u8(static_cast<uint8_t>(dialog_sequence_id));
#else
	oracle_hash_u16(static_cast<uint16_t>(std_map_section_id));
	oracle_hash_u8(0);
#endif
	oracle_hash_u8(static_cast<uint8_t>(quit));
	// The injection hook's identity, as a stable enum. Never its address.
	oracle_hash_u8(ORACLE_HOOK_DEMO);
	oracle_hash_store(out);
}

// Group 2 -- player. The harness TH04 deterministic-state contract names the owners
// (`player.hpp`, `move.hpp`, `shot.hpp`, `bomb.hpp`); this is their field-level
// inventory, which that file listed as `[open]`.
//
// Deliberately EXCLUDED, and each exclusion is a scope-rule call rather than a
// convenience:
//
//   * `shots_alive[SHOT_COUNT]` (`th04/main/player/shot.hpp:100-108`) -- every
//     element carries a raw `Shot near *`, and the array is per-frame
//     collision scratch that `shots_hittest()` rebuilds from `shots[]` before
//     each use. Its *count* is hashed, so a divergence in how many shots were
//     alive still shows up.
//   * `playchar_bomb_func` / `player_bomb_func` are hashed as installed-or-not,
//     never as offsets: the debloated lineage relinks both.
static void oracle_hash_group_player(oracle_split_hash_t far *out)
{
	int i;

	oracle_hash_init();
	oracle_hash_motion(&player_pos);
	oracle_hash_u8(static_cast<uint8_t>(player_is_hit));
	oracle_hash_u8(player_invincibility_time);
	oracle_hash_u8(power);
	oracle_hash_u16(static_cast<uint16_t>(power_overflow));
	oracle_hash_u8(shot_level);
	oracle_hash_u8(shot_time);
	oracle_hash_u8(static_cast<uint8_t>(bombing));
	oracle_hash_u8(static_cast<uint8_t>(bombing_disabled));
	oracle_hash_u8(bomb_frame);
	oracle_hash_u8(oracle_hook_installed(playchar_bomb_func));
#if (GAME == 5)
	// TH04 has the extra `player_bomb_func` indirection and TH05 does not;
	// TH05 has runtime playchar speeds and TH04 compile-time constants. Both
	// games emit both fields so that one field ordering describes both.
	oracle_hash_u8(0);
	oracle_hash_u16(static_cast<uint16_t>(playchar_speed_aligned));
	oracle_hash_u16(static_cast<uint16_t>(playchar_speed_diagonal));
#else
	oracle_hash_u8(oracle_hook_installed(player_bomb_func));
	oracle_hash_u16(static_cast<uint16_t>(playchar_speed_aligned));
	oracle_hash_u16(static_cast<uint16_t>(playchar_speed_diagonal));
#endif
	oracle_hash_u8(static_cast<uint8_t>(shot_last_id));
	// `shot_ptr` points into `shots[]` (`th04/main/player/shot.hpp:86`).
	// Hashed as the element index it denotes, per the pointer rule; 0xFFFF is
	// "none", which is distinguishable from index 0.
	oracle_hash_u16(
		(shot_ptr != 0)
			? static_cast<uint16_t>(shot_ptr - shots)
			: 0xFFFFu
	);
	oracle_hash_u16(shots_alive_count);
	oracle_hash_u8(static_cast<uint8_t>(shots_hittest_against_boss));
	oracle_hash_sppoint(&shot_hitbox_center);
	oracle_hash_sppoint(&shot_hitbox_radius);
	// The option laser. `th04/main/player/shot.hpp:139-141` calls it "unused in
	// TH05, but still present in the code" -- that is true of the *code*, but
	// TH05's MAIN does not link the state: `_shot_laser_time`,
	// `_shot_laser_style`, `_shot_laser_ring_cycle` and
	// `_shot_laser_bottomcenter` are all undefined symbols there. TH05 emits
	// zeroes in their place so that one field ordering still describes both
	// games' group 2.
#if (GAME == 5)
	oracle_hash_u16(0);
	oracle_hash_u8(0);
	oracle_hash_u8(0);
	oracle_hash_u16(0); oracle_hash_u16(0); oracle_hash_u16(0);
	oracle_hash_u16(0); oracle_hash_u16(0); oracle_hash_u16(0);
#else
	oracle_hash_u16(shot_laser_time);
	oracle_hash_u8(static_cast<uint8_t>(shot_laser_style));
	oracle_hash_u8(shot_laser_ring_cycle);
	oracle_hash_motion(&shot_laser_bottomcenter);
#endif
	for(i = 0; i < SHOT_COUNT; i++) {
		oracle_hash_u8(static_cast<uint8_t>(shots[i].flag));
		oracle_hash_u8(static_cast<uint8_t>(shots[i].age));
		oracle_hash_motion(&shots[i].pos);
#if (GAME == 5)
		oracle_hash_u8(static_cast<uint8_t>(shots[i].patnum_base));
		oracle_hash_u8(static_cast<uint8_t>(shots[i].type));
#else
		oracle_hash_u16(static_cast<uint16_t>(shots[i].patnum_base));
#endif
		oracle_hash_u8(static_cast<uint8_t>(shots[i].damage));
		oracle_hash_u8(static_cast<uint8_t>(shots[i].angle));
	}
	oracle_hash_store(out);
}

// Group 3 -- bullets. Returns the number of live entries in `bullets[]`, which
// the row carries plainly: when this hash goes red, "how many bullets were
// alive" is the first thing a human needs, and the count is free here.
//
// `bullet_t` is 26 bytes in TH04 and 32 in TH05 with an inverted
// pellet/bullet16 split (`th04/main/bullet/bullet.hpp:133-199`), so element
// the same element index does not denote the same thing in both games. That is
// fine: the TH04 and TH05 trace schemas are never compared to each
// other. What IS compared is one game across lineages, and for that the
// field-by-field serialization below matters: the debloated lineage deletes
// `#pragma option -a2` from `th04/main/bullet/update.cpp` and
// `th04/main/player/move.cpp`, so a hash over the raw memory image would be
// sensitive to a structure-alignment change that has no semantic content.
//
// Deliberately EXCLUDED:
//
//   * `pellets_render[]` and `pellet_clouds_render[]`
//     (`th04/main/bullet/pellet_r.hpp:17-22`) -- renderer scratch rebuilt every
//     frame, and the latter is 180 raw `bullet_t near *`. `pellets_render`
//     begins on the byte immediately after the last bullet, so the loop below
//     must stop exactly at BULLET_COUNT.
//   * `thicklasers[]` / `thicklaser_template` (TH04) and `lasers[]` /
//     `laser_template` (TH05) -- `[open]`, deferred to schema 3.
//     `th04/main/bullet/laser_t.hpp:16-42` declares a 21-byte `thicklaser_t`,
//     but `th04_main.asm:30046-30062` assembles 24: the header carries one
//     filler byte where the structure has four. Indexing the array through the
//     header would read misaligned elements, i.e. garbage that happens to be
//     reproducible. Fix the header before hashing the family.
static uint16_t oracle_hash_group_bullets(oracle_split_hash_t far *out)
{
	uint16_t alive = 0;
	int i;

	oracle_hash_init();
	for(i = 0; i < BULLET_COUNT; i++) {
		oracle_hash_u8(static_cast<uint8_t>(bullets[i].flag));
		if(bullets[i].flag != F_FREE) {
			alive++;
		}
		oracle_hash_u8(static_cast<uint8_t>(bullets[i].age));
		// `pos.prev` is render-only, but it is never initialized on spawn, so
		// a reused slot carries its previous occupant's value. That makes it a
		// sensitive detector of slot-reuse ordering, which is exactly the kind
		// of divergence this group exists to catch. Hashed deliberately.
		oracle_hash_motion(&bullets[i].pos);
		oracle_hash_u8(bullets[i].from_group);
		oracle_hash_u8(static_cast<uint8_t>(bullets[i].unused));
		oracle_hash_u8(bullets[i].speed_cur.v);
		oracle_hash_u8(bullets[i].angle);
		oracle_hash_u8(static_cast<uint8_t>(bullets[i].spawn_flag));
		oracle_hash_u8(static_cast<uint8_t>(bullets[i].move_flag));
		// `special_motion`'s enumerators are offset by `(GAME - 5) * 0x81`
		// (`th04/main/bullet/bullet.hpp:80-124`), so the same stored byte means
		// a different motion in each game. Stored raw, which is correct within
		// one game's schema and must never be cross-compared between games.
		oracle_hash_u8(static_cast<uint8_t>(bullets[i].special_motion));
		oracle_hash_u8(bullets[i].speed_final.v);
		oracle_hash_u8(bullets[i].u1.decelerate_time);
		oracle_hash_u8(bullets[i].u2.decelerate_speed_delta.v);
		oracle_hash_u16(static_cast<uint16_t>(bullets[i].patnum));
#if (GAME == 5)
		oracle_hash_sppoint(&bullets[i].origin);
		oracle_hash_u16(static_cast<uint16_t>(bullets[i].distance.v));
#endif
	}

	// Spawn parameters. `bullet_template_tune`, `bullets_add_regular` and
	// `bullets_add_special` are `nearfunc_t_near` slots
	// (`th04/main/bullet/bullet.hpp:331-356`) and are hashed as
	// installed-or-not for the same reason as the bomb hooks.
	oracle_hash_u8(bullet_special.turns_max);
	oracle_hash_u8(static_cast<uint8_t>(bullet_template_special_angle.v));
	oracle_hash_u8(bullet_template.spawn_type);
	oracle_hash_u8(bullet_template.patnum);
	oracle_hash_u16(static_cast<uint16_t>(bullet_template.origin.x.v));
	oracle_hash_u16(static_cast<uint16_t>(bullet_template.origin.y.v));
	oracle_hash_u8(static_cast<uint8_t>(bullet_template.group));
	oracle_hash_u8(static_cast<uint8_t>(bullet_template.special_motion));
	oracle_hash_u8(bullet_template.angle);
	oracle_hash_u8(bullet_template.speed.v);
	oracle_hash_u8(oracle_hook_installed(bullet_template_tune));
#if (GAME == 5)
	oracle_hash_u8(bullet_template.spread);
	oracle_hash_u8(bullet_template.spread_angle_delta);
	oracle_hash_u8(bullet_template.stack);
	oracle_hash_u8(bullet_template.stack_speed_delta.v);
	oracle_hash_u8(static_cast<uint8_t>(bullet_zap_drop_point_items));
#else
	oracle_hash_u16(static_cast<uint16_t>(bullet_template.velocity.x.v));
	oracle_hash_u16(static_cast<uint16_t>(bullet_template.velocity.y.v));
	oracle_hash_u8(bullet_template.count);
	oracle_hash_u8(bullet_template.delta.spread_angle);
	oracle_hash_u8(bullet_template.unused_1);
	oracle_hash_u8(bullet_template.unused_2);
	oracle_hash_u8(oracle_hook_installed(bullets_add_regular));
	oracle_hash_u8(oracle_hook_installed(bullets_add_special));
#endif
	oracle_hash_u8(bullet_zap.frame);
	oracle_hash_u8(bullet_clear_time);

	// The custom-entity block. The harness TH05 deterministic-state contract files
	// TH05's
	// cheetos, swords, b4balls and b6balls under this group; all four are
	// `reinterpret_cast` views of the SAME `custom_entities[]` storage
	// (`th05/main/bullet/{cheeto,sword,b4ball,b6ball}.hpp`), as are the stage-2
	// particles and the Stage 3 boss puppets. Hashing the declared `custom_t`
	// fields once covers every view without hashing any of them twice.
	for(i = 0; i < CUSTOM_COUNT; i++) {
		oracle_hash_u8(custom_entities[i].flag);
		oracle_hash_u8(custom_entities[i].angle);
#if (GAME == 5)
		oracle_hash_motion(&custom_entities[i].pos);
		oracle_hash_u16(custom_entities[i].val1);
		oracle_hash_u16(custom_entities[i].val2);
		oracle_hash_u16(static_cast<uint16_t>(custom_entities[i].sprite));
		oracle_hash_u16(static_cast<uint16_t>(custom_entities[i].val3));
		oracle_hash_u16(static_cast<uint16_t>(custom_entities[i].damage));
		oracle_hash_u8(custom_entities[i].speed.v);
		oracle_hash_u8(static_cast<uint8_t>(custom_entities[i].padding));
#else
		oracle_hash_u16(static_cast<uint16_t>(custom_entities[i].center.x));
		oracle_hash_u16(static_cast<uint16_t>(custom_entities[i].center.y));
		oracle_hash_u16(static_cast<uint16_t>(custom_entities[i].val1));
		oracle_hash_u16(static_cast<uint16_t>(custom_entities[i].origin_y.v));
		oracle_hash_u16(static_cast<uint16_t>(custom_entities[i].velocity.x.v));
		oracle_hash_u16(static_cast<uint16_t>(custom_entities[i].velocity.y.v));
		oracle_hash_u16(custom_entities[i].val2);
		oracle_hash_u16(static_cast<uint16_t>(custom_entities[i].distance));
		oracle_hash_u16(static_cast<uint16_t>(custom_entities[i].val3));
		oracle_hash_u16(static_cast<uint16_t>(custom_entities[i].hp));
		oracle_hash_u16(
			static_cast<uint16_t>(custom_entities[i].damage_this_frame)
		);
		oracle_hash_u8(custom_entities[i].val4);
		oracle_hash_u8(custom_entities[i].angle_speed);
#endif
	}
	oracle_hash_store(out);
	return alive;
}

// The enemy script pointer always denotes one of the 32 bases loaded from the
// current `.STD`. Serialize that table index, never the linked near address.
// 0xFE means a non-null value outside the table; 0xFF means no script.
static uint8_t oracle_enemy_script_id(const unsigned char near *script)
{
	int i;

	if(script == 0) {
		return 0xFF;
	}
	for(i = 0; i < STD_ENEMY_SCRIPT_COUNT; i++) {
		if(script == reinterpret_cast<unsigned char near *>(std_enemy_scripts[i])) {
			return static_cast<uint8_t>(i);
		}
	}
	return 0xFE;
}

// Group 4 -- enemies. Declaration order differs between TH04 and TH05, so the
// schema uses one semantic order and emits zero for fields absent in one game.
// Native padding and the three explicitly unused members are excluded.
static void oracle_hash_group_enemies(oracle_split_hash_t far *out)
{
	int i;

	oracle_hash_init();
	for(i = 0; i < ENEMY_COUNT; i++) {
		enemy_t far *enemy = &enemies[i];

		oracle_hash_u8(enemy->flag);
		oracle_hash_u8(enemy->age);
		oracle_hash_motion(&enemy->pos);
		oracle_hash_u16(static_cast<uint16_t>(enemy->hp));
		oracle_hash_u16(static_cast<uint16_t>(enemy->score));
		oracle_hash_u8(oracle_enemy_script_id(enemy->script));
		oracle_hash_u16(static_cast<uint16_t>(enemy->script_ip));
		oracle_hash_u8(enemy->cur_instr_frame);
		oracle_hash_u8(enemy->loop_i);
#if (GAME == 5)
		oracle_hash_u8(enemy->speed.v);
#else
		oracle_hash_u16(static_cast<uint16_t>(enemy->speed.v));
#endif
		oracle_hash_u8(enemy->angle);
		oracle_hash_u8(enemy->angle_delta);
		oracle_hash_u8(enemy->patnum_base);
		oracle_hash_u8(enemy->anim_cels);
		oracle_hash_u8(enemy->anim_frames_per_cel);
		oracle_hash_u8(enemy->anim_cur_cel);
#if (GAME == 5)
		oracle_hash_u8((enemy->clip & ENEMY_CLIP_X) ? 1 : 0);
		oracle_hash_u8((enemy->clip & ENEMY_CLIP_Y) ? 1 : 0);
#else
		oracle_hash_u8(static_cast<uint8_t>(enemy->clip_x));
		oracle_hash_u8(static_cast<uint8_t>(enemy->clip_y));
#endif
		oracle_hash_u8(static_cast<uint8_t>(enemy->item));
		oracle_hash_u8(static_cast<uint8_t>(enemy->damaged_this_frame));
		oracle_hash_u8(static_cast<uint8_t>(enemy->can_be_damaged));
		oracle_hash_u8(static_cast<uint8_t>(enemy->autofire));
		oracle_hash_u8(static_cast<uint8_t>(enemy->kills_player_on_collision));
		oracle_hash_u8(static_cast<uint8_t>(enemy->spawned_in_left_half));
		oracle_hash_u8(enemy->autofire_cur_frame);
		oracle_hash_u8(enemy->autofire_interval);
#if (GAME == 5)
		oracle_hash_u8(enemy->subtype);
#else
		oracle_hash_u8(0);
#endif
		oracle_hash_bullet_template(&enemy->bullet_template);
	}
	// `enemy_cur` is update scratch and always points into `enemies[]` while
	// active. Its normalized index catches loop-order divergence without
	// serializing the linked address.
	oracle_hash_u16(
		(enemy_cur != 0)
			? static_cast<uint16_t>(enemy_cur - enemies)
			: 0xFFFFu
	);
	oracle_hash_store(out);
}

static void oracle_hash_boss(const boss_stuff_t far *actor)
{
	oracle_hash_motion(&actor->pos);
	oracle_hash_u16(static_cast<uint16_t>(actor->hp));
	oracle_hash_u8(actor->sprite);
	oracle_hash_u8(actor->phase);
	oracle_hash_u16(static_cast<uint16_t>(actor->phase_frame));
	oracle_hash_u8(actor->damage_this_frame);
	oracle_hash_u8(actor->mode);
	oracle_hash_u8(actor->angle);
	oracle_hash_u8(actor->phase_state.patterns_seen);
	oracle_hash_u16(static_cast<uint16_t>(actor->phase_end_hp));
}

static void oracle_hash_explosion(const Explosion far *explosion)
{
	oracle_hash_u8(static_cast<uint8_t>(explosion->alive));
	oracle_hash_u8(explosion->age);
	oracle_hash_sppoint(&explosion->center);
	oracle_hash_sppoint(&explosion->radius_cur);
	oracle_hash_sppoint(&explosion->radius_delta);
	oracle_hash_u8(explosion->angle_offset);
}

// Group 5 -- common actor core. Stage-specific boss locals are intentionally
// not claimed by this first actor schema; the boundary catalog and subsequent
// additive schema will enumerate them. This group already catches actor
// lifecycle, phase, HP, position, callback activation, hitbox, and explosion
// divergence across every stage.
static void oracle_hash_group_actors(oracle_split_hash_t far *out)
{
	int i;

	oracle_hash_init();
	oracle_hash_motion(&midboss.pos);
	oracle_hash_u16(midboss.frames_until);
	oracle_hash_u16(static_cast<uint16_t>(midboss.hp));
	oracle_hash_u8(midboss.sprite);
	oracle_hash_u8(midboss.phase);
	oracle_hash_u16(static_cast<uint16_t>(midboss.phase_frame));
	oracle_hash_u8(midboss.damage_this_frame);
	oracle_hash_u8(midboss.angle);
	oracle_hash_u8(oracle_hook_installed(midboss_invalidate));
	oracle_hash_u8((midboss_update != 0) ? 1 : 0);
	oracle_hash_u8(oracle_hook_installed(midboss_render));
	oracle_hash_u8((midboss_update_func != 0) ? 1 : 0);
	oracle_hash_u8(oracle_hook_installed(midboss_render_func));

	oracle_hash_boss(&boss);
	for(i = 0; i < 16; i++) {
		oracle_hash_u8(boss_statebyte[i]);
	}
	oracle_hash_sppoint(&boss_hitbox_radius);
	oracle_hash_u8(static_cast<uint8_t>(boss_phase_timed_out));
	oracle_hash_u8((boss_update != 0) ? 1 : 0);
	oracle_hash_u8(oracle_hook_installed(boss_fg_render));
	oracle_hash_u8((boss_update_func != 0) ? 1 : 0);
	oracle_hash_u8(oracle_hook_installed(boss_bg_render_func));
	oracle_hash_u8(oracle_hook_installed(boss_fg_render_func));
	oracle_hash_u8(oracle_hook_installed(boss_backdrop_colorfill));
#if (GAME == 5)
	oracle_hash_boss(&boss2);
	oracle_hash_u8(oracle_hook_installed(boss_custombullets_render));
	oracle_hash_u16(static_cast<uint16_t>(boss_sprite_left));
	oracle_hash_u16(static_cast<uint16_t>(boss_sprite_right));
	oracle_hash_u16(static_cast<uint16_t>(boss_sprite_stay));
	oracle_hash_u16(static_cast<uint16_t>(boss_flystep_random_clamp.left.v));
	oracle_hash_u16(static_cast<uint16_t>(boss_flystep_random_clamp.right.v));
	oracle_hash_u16(static_cast<uint16_t>(boss_flystep_random_clamp.top.v));
	oracle_hash_u16(static_cast<uint16_t>(boss_flystep_random_clamp.bottom.v));
#endif
	for(i = 0; i < EXPLOSION_SMALL_COUNT; i++) {
		oracle_hash_explosion(&explosions_small[i]);
	}
	oracle_hash_explosion(&explosions_big);
	oracle_hash_store(out);
}

static void oracle_hash_group_items(oracle_split_hash_t far *out)
{
	int i;
	int digit;

	oracle_hash_init();
	for(i = 0; i < ITEM_COUNT; i++) {
		oracle_hash_u8(static_cast<uint8_t>(items[i].flag));
		oracle_hash_motion(&items[i].pos);
		oracle_hash_u8(items[i].type);
		oracle_hash_u8(static_cast<uint8_t>(items[i].unknown));
		oracle_hash_u16(static_cast<uint16_t>(items[i].patnum));
		oracle_hash_u16(static_cast<uint16_t>(items[i].pulled_to_player));
	}
	for(i = 0; i < ITEM_SPLASH_COUNT; i++) {
		oracle_hash_u8(static_cast<uint8_t>(item_splashes[i].flag));
		oracle_hash_u8(static_cast<uint8_t>(item_splashes[i].time));
		oracle_hash_sppoint(&item_splashes[i].center);
		oracle_hash_u16(static_cast<uint16_t>(item_splashes[i].radius_cur.v));
		oracle_hash_u16(static_cast<uint16_t>(item_splashes[i].radius_prev.v));
	}
	oracle_hash_u8(item_splash_last_id);
	oracle_hash_u8(enemy_drop_ring_p);
	oracle_hash_u8(item_playperf_raise);
	oracle_hash_u8(item_playperf_lower);
	oracle_hash_u8(static_cast<uint8_t>(items_pull_to_player));
	oracle_hash_u16(items_spawned);
	oracle_hash_u16(items_collected);
	oracle_hash_u16(total_point_items_collected);
	oracle_hash_u16(total_max_valued_point_items);
#if (GAME == 5)
	oracle_hash_u16(stage_point_items_collected);
	oracle_hash_u16(extend_point_items_collected);
	oracle_hash_u16(item_point_score_at_full_dream);
	oracle_hash_u8(dream);
#else
	oracle_hash_u8(stage_point_items_collected);
	oracle_hash_u16(dream_score);
	// Defined in the original BSS and consumed by item collection and miss
	// logic; declared locally because it has no public owner header yet.
	{
		extern unsigned char dream_items_collected;
		oracle_hash_u8(dream_items_collected);
	}
#endif

	for(i = 0; i < GATHER_COUNT; i++) {
		oracle_hash_u8(static_cast<uint8_t>(gather_circles[i].flag));
		oracle_hash_u8(static_cast<uint8_t>(gather_circles[i].col));
		oracle_hash_motion(&gather_circles[i].center);
		oracle_hash_u16(static_cast<uint16_t>(gather_circles[i].radius_cur.v));
		oracle_hash_u16(static_cast<uint16_t>(gather_circles[i].ring_points));
		oracle_hash_u8(gather_circles[i].angle_cur);
		oracle_hash_u8(gather_circles[i].angle_delta);
		oracle_hash_bullet_template(&gather_circles[i].bullet_template);
		oracle_hash_u16(static_cast<uint16_t>(gather_circles[i].radius_prev.v));
		oracle_hash_u16(static_cast<uint16_t>(gather_circles[i].radius_delta.v));
	}
	oracle_hash_playfield_point(&gather_template.center);
	oracle_hash_playfield_point(&gather_template.velocity);
	oracle_hash_u16(static_cast<uint16_t>(gather_template.radius.v));
	oracle_hash_u16(static_cast<uint16_t>(gather_template.ring_points));
	oracle_hash_u8(static_cast<uint8_t>(gather_template.col));
	oracle_hash_u8(gather_template.angle_delta);

	for(i = 0; i < POINTNUM_COUNT; i++) {
		oracle_hash_u8(static_cast<uint8_t>(pointnums[i].flag));
		oracle_hash_u8(pointnums[i].age);
		oracle_hash_sppoint(&pointnums[i].center_cur);
		oracle_hash_u16(static_cast<uint16_t>(pointnums[i].center_prev_y.v));
		oracle_hash_u16(static_cast<uint16_t>(pointnums[i].width));
		for(digit = 0; digit < POINTNUM_DIGITS; digit++) {
			oracle_hash_u8(pointnums[i].digits_lebcd[digit]);
		}
#if (GAME == 4)
		oracle_hash_u8(static_cast<uint8_t>(pointnums[i].times_2));
#endif
	}
	oracle_hash_u8(pointnum_yellow_p);
	oracle_hash_u8(pointnum_white_p);
	oracle_hash_u8(static_cast<uint8_t>(pointnum_times_2));
	oracle_hash_store(out);
}
/// --------------------

/// Status and diagnostics
/// ----------------------

static void oracle_write_char(int fh, char c)
{
	oracle_dos_write(fh, &c, 1);
}

// One character at a time, so this module contributes no initialized data.
static void oracle_write_text(int fh, oracle_text_id_t text)
{
#define W(c) oracle_write_char(fh, c)
	switch(text) {
	case ORT_OK_RECORD:
		W('o'); W('k'); W(':'); W('r'); W('e'); W('c'); W('o'); W('r'); W('d');
		break;
	case ORT_OK_PLAYBACK:
		W('o'); W('k'); W(':'); W('p'); W('l'); W('a'); W('y'); W('b'); W('a');
		W('c'); W('k');
		break;
	case ORT_OK_INPUT_END:
		W('o'); W('k'); W(':'); W('i'); W('n'); W('p'); W('u'); W('t'); W('-');
		W('e'); W('n'); W('d');
		break;
	case ORT_ERR_CASE_HEADER:
		W('e'); W('r'); W('r'); W('o'); W('r'); W(':'); W('c'); W('a'); W('s');
		W('e'); W('-'); W('h'); W('e'); W('a'); W('d'); W('e'); W('r');
		break;
	case ORT_ERR_CASE_CREATE:
		W('e'); W('r'); W('r'); W('o'); W('r'); W(':'); W('c'); W('a'); W('s');
		W('e'); W('-'); W('c'); W('r'); W('e'); W('a'); W('t'); W('e');
		break;
	case ORT_ERR_CASE_FINALIZE:
		W('e'); W('r'); W('r'); W('o'); W('r'); W(':'); W('c'); W('a'); W('s');
		W('e'); W('-'); W('f'); W('i'); W('n'); W('a'); W('l');
		break;
	case ORT_ERR_FRAME_IO:
		W('e'); W('r'); W('r'); W('o'); W('r'); W(':'); W('f'); W('r'); W('a');
		W('m'); W('e'); W('-'); W('i'); W('o');
		break;
	case ORT_ERR_DESYNC:
		W('e'); W('r'); W('r'); W('o'); W('r'); W(':'); W('d'); W('e'); W('s');
		W('y'); W('n'); W('c');
		break;
	case ORT_ERR_STARTUP:
		W('e'); W('r'); W('r'); W('o'); W('r'); W(':'); W('s'); W('t'); W('a');
		W('r'); W('t'); W('u'); W('p');
		break;
	case ORT_ERR_SPLIT_OPEN:
		W('e'); W('r'); W('r'); W('o'); W('r'); W(':'); W('s'); W('p'); W('l');
		W('i'); W('t'); W('-'); W('o'); W('p'); W('e'); W('n');
		break;
	case ORT_ERR_UNSUPPORTED:
		W('e'); W('r'); W('r'); W('o'); W('r'); W(':'); W('u'); W('n'); W('s');
		W('u'); W('p'); W('p'); W('o'); W('r'); W('t'); W('e'); W('d');
		break;
	}
#undef W
}

static void oracle_done_write(oracle_text_id_t status)
{
	int fh;

	if(oracle_done_written) {
		return;
	}
	oracle_paths_init();
	fh = oracle_dos_create(ORACLE_DONE_FN);
	if(fh >= 0) {
		oracle_write_text(fh, status);
		oracle_write_char(fh, '\r');
		oracle_write_char(fh, '\n');
		oracle_dos_close(fh);
	}
	oracle_done_written = true;
}

static void oracle_diag_hex32(char far *out, uint32_t value)
{
	int i;
	uint8_t nibble;

	for(i = 0; i < 8; i++) {
		nibble = static_cast<uint8_t>(value & 0x0FUL);
		out[7 - i] = static_cast<char>(
			(nibble < 10) ? ('0' + nibble) : ('A' + (nibble - 10))
		);
		value >>= 4;
	}
}

// One fixed-width line per milestone, flushed immediately, so an interrupted
// run still leaves a usable trace. [t0..t2] is the three-character tag.
static void oracle_diag(char t0, char t1, char t2, uint32_t a, uint32_t b)
{
	char line[24];
	int fh;

	oracle_paths_init();
	line[0] = t0;
	line[1] = t1;
	line[2] = t2;
	line[3] = ' ';
	oracle_diag_hex32(&line[4], a);
	line[12] = ' ';
	oracle_diag_hex32(&line[13], b);
	line[21] = '\r';
	line[22] = '\n';
	fh = oracle_dos_open_rw(ORACLE_DIAG_FN);
	if(fh < 0) {
		return;
	}
	oracle_dos_seek(fh, oracle_diag_size);
	if(oracle_dos_write(fh, line, 23) == 23) {
		oracle_diag_size += 23;
	}
	oracle_dos_close(fh);
}
/// ----------------------

/// Split trace
/// -----------

static void oracle_split_write_header(void)
{
	oracle_split_header_t header;
	int fh;

	oracle_memclear(&header, sizeof(header));
	header.magic[0] = 'T';
	header.magic[1] = ORACLE_MAGIC_DIGIT;
	header.magic[2] = 'S';
	header.magic[3] = 'P';
	header.magic[4] = 'L';
	header.magic[5] = 'T';
	header.magic[6] = '1';
	header.version = ORACLE_SPLIT_VERSION;
	header.header_size = sizeof(header);
	header.row_size = sizeof(oracle_split_row_t);
	header.flags = 0;
	fh = oracle_dos_create(ORACLE_SPLIT_FN);
	if(fh < 0) {
		oracle_mode = ORACLE_ERROR;
		oracle_done_write(ORT_ERR_SPLIT_OPEN);
		return;
	}
	oracle_dos_write(fh, &header, sizeof(header));
	oracle_dos_close(fh);
	oracle_split_size = sizeof(header);
}

static void oracle_split_row(uint8_t event, uint16_t input)
{
	oracle_split_row_t row;
	unsigned written;
	int fh;
	int i;

	if((oracle_mode == ORACLE_DISABLED) || (oracle_mode == ORACLE_ERROR)) {
		return;
	}
	oracle_memclear(&row, sizeof(row));
	row.event = event;
	row.process = ORACLE_PROCESS_MAIN;
	row.stage_id = stage_id;
	row.rank = rank;
	row.global_frame = oracle_global_frame;
	// Not monotonic: TH05's Extra splice resets `stage_frame`
	// (`th05/main/dialog/dialog.cpp:271`). A comparator must never sort, diff
	// or interpolate on this column.
	row.scenario_cursor = static_cast<uint32_t>(stage_frame);
	row.input = input;
	row.schema = ORACLE_SPLIT_VERSION;

	row.random_seed = static_cast<uint32_t>(random_seed);
	row.randring_p = randring_p;
	row.stage_graze = stage_graze;
	for(i = 0; i < ORACLE_SCORE_DIGITS; i++) {
		row.score[i] = score.digits[i];
	}
	row.samples_consumed = oracle_sample_count;
#if (GAME == 5)
	row.rem_lives = lives;
	row.rem_bombs = bombs;
	row.dialog_sequence_id = static_cast<uint8_t>(dialog_sequence_id);
#else
	row.rem_lives = resident->rem_lives;
	row.rem_bombs = resident->rem_bombs;
	row.dialog_sequence_id = 0;
#endif
	row.credit_lives = resident->credit_lives;
	row.credit_bombs = resident->credit_bombs;
	row.power = power;
	row.playperf = playperf;
	row.end_sequence = resident->end_sequence;
	row.quit = static_cast<uint8_t>(quit);
	row.bombing = static_cast<uint8_t>(bombing);

	oracle_hash_group_rng(&row.hashes[ORACLE_HASH_GROUP_RNG]);
	oracle_hash_group_run(&row.hashes[ORACLE_HASH_GROUP_RUN]);
	oracle_hash_group_player(&row.hashes[ORACLE_HASH_GROUP_PLAYER]);
	row.bullets_alive = oracle_hash_group_bullets(
		&row.hashes[ORACLE_HASH_GROUP_BULLETS]
	);
	oracle_hash_group_enemies(&row.hashes[ORACLE_HASH_GROUP_ENEMIES]);
	oracle_hash_group_actors(&row.hashes[ORACLE_HASH_GROUP_ACTORS]);
	oracle_hash_group_items(&row.hashes[ORACLE_HASH_GROUP_ITEMS]);

	fh = oracle_dos_open_rw(ORACLE_SPLIT_FN);
	if(fh < 0) {
		oracle_diag('S', 'P', 'O', oracle_split_size, 0xFFFFFFFFUL);
		oracle_mode = ORACLE_ERROR;
		oracle_done_write(ORT_ERR_SPLIT_OPEN);
		return;
	}
	oracle_dos_seek(fh, oracle_split_size);
	written = oracle_dos_write(fh, &row, sizeof(row));
	if(written != sizeof(row)) {
		oracle_dos_close(fh);
		oracle_diag('S', 'P', 'W', oracle_split_size, written);
		oracle_mode = ORACLE_ERROR;
		oracle_done_write(ORT_ERR_SPLIT_OPEN);
		return;
	}
	oracle_dos_close(fh);
	oracle_split_size += sizeof(row);
}
/// -----------

/// Case file
/// ---------

// Control file grammar, deliberately tiny:
//
//   p                       play back T?CASE.BIN
//   r<demo>[<rank>[<stage>]]
//                           record scenario <demo> (1-4, TH05 also 5) at
//                           <rank>, optionally starting at <stage>
//   r<demo>[<rank>[<stage>]] <frames>
//                           ... ending the recording after <frames> input
//                           records, whether or not the run would have ended
//                           on its own
//
// <rank> defaults to 1 (Normal). It has to be selectable because `rank` is a
// required recorded field that the game's own demo path never pins, and a
// corpus is only useful if the difficulty it was taken at is chosen rather
// than inherited from whatever the resident structure happened to hold.
//
// <stage> is the third decimal and defaults to ABSENT, which reproduces
// `start_demo()`'s own table exactly. It exists because ZUN's four TH04 demos
// only ever visit stages 3, 0, 2 and 1 and there is no fifth
// (`th04/op/start.cpp:53-86`), so `stagex_setup()` and everything it arms are
// unreachable by every case a corpus can hold. Three fix-layer rows are
// blocked on precisely that gap.
//
// This costs the FORMAT nothing, which is the whole reason it is done this
// way: `oracle_startup_t` already carries `demo_stage`, `stage` and
// `stage_id`, `oracle_startup_apply()` already writes `demo_stage` back, and
// `oracle_startup_verify()` already compares `stage_id` post-init. So a
// stage-overridden case is self-checking under the UNCHANGED version 1 layout,
// and every previously recorded case stays byte-comparable. TH01 reached the
// same conclusion for the same reason (`state/notes/t1case-boss-corpus.md`).
//
// <frames> is the FRAME BOUND, and it is what makes a stage-overridden case a
// case rather than a prefix. It bounds a RECORDING only; playback ignores it
// entirely and follows the case's own terminal control record, as does the
// corresponding TH02 recorder bound.
//
// [verified-by-emulator 2026-08-15] Why it is needed: the oracle's only
// injection seam is the indirect `_demo_update` call inside the gameplay frame
// loop (`th04_main.asm:352-353`). ZUN's demo tape was recorded against a
// different stage, so on an overridden stage the substituted input loses the
// run; the final miss takes the game OUT of that loop into the
// game-over/continue path, the seam stops being called, and `oracle_finish()`
// never runs. The recording therefore stops at a prefix with no terminal
// boundary, no final payload checksum and no status file. Measured, not
// assumed: three runs stalled, and an 1800 s retry of one produced a
// byte-identical case to its own 420 s attempt, so it is a hang and not
// slowness. Two runs of the SAME stage with DIFFERENT tapes stalled at
// DIFFERENT frames (1345 vs 1537), which is what identifies the cause as
// input-dependent rather than stage-scripted.
//
// Ending the recording at a chosen frame while the run is still alive puts
// `oracle_finish()` back on a path that actually executes.
#if ORACLE_RECORD_SUPPORTED
static uint8_t oracle_cfg_demo_num;
static uint8_t oracle_cfg_rank;

// `ORACLE_CFG_STAGE_NONE` means "no override": the pin runs `start_demo()`'s
// table verbatim. A sentinel rather than a zero, because stage 0 is a real
// stage that demo 2 already starts on.
#define ORACLE_CFG_STAGE_NONE 0xFF
static uint8_t oracle_cfg_start_stage;

// 0 means UNBOUNDED: only the stock end condition applies, and the recorded
// bytes are then bit-for-bit what they were before this field existed.
//
// This is a DELIBERATE divergence from TH02, which defaults its own bound to
// ZUN's end condition (`DEMO_N - 50`) when the field is absent. TH02 can do
// that because its condition is on `demo_frame`, the same counter the bound
// is compared against. TH04/TH05's is on `stage_frame`, which is per-stage,
// while any bound has to count the recorder's own monotonic samples --
// substituting a global default would silently change the length of every
// stock case in the frozen corpus. Justified rather than copied.
#define ORACLE_CFG_FRAMES_NONE 0
static uint32_t oracle_cfg_frames;

// Same parser shape as the TH02 case recorder: leading blanks, then decimal
// digits, with
// `*pos` left one past the last character consumed. Returns 0 when no digit
// was seen, which is the same value as an explicit `0`, and both mean
// no frame bound, so the caller needs no separate presence flag.
static uint32_t oracle_cfg_number(const char *cfg, unsigned len, unsigned *pos)
{
	uint32_t value = 0;
	unsigned i = *pos;

	while((i < len) && ((cfg[i] == ' ') || (cfg[i] == '\t'))) {
		i++;
	}
	while((i < len) && (cfg[i] >= '0') && (cfg[i] <= '9')) {
		value = ((value * 10) + static_cast<uint32_t>(cfg[i] - '0'));
		i++;
	}
	*pos = i;
	return value;
}
#endif

static oracle_mode_t oracle_cfg_mode(void)
{
	char cfg[64];
	int fh;
	unsigned read_len;
	unsigned i;
	unsigned first = 0;
	char mode = '\0';

	// One shot per staged run. `demo_end()` execs OP, whose attract timeout
	// execs MAIN again (`th04/op/m_main.cpp:636-637`), and that second MAIN
	// process starts with freshly-zeroed BSS -- so without this latch it
	// re-reads the same config and TRUNCATES the case and trace that the first
	// one just finished writing.
	//
	// [measured] That is exactly what happened to TH05 on the first run of
	// this harness: T5DONE.TXT was stamped 9:44:26 while T5CASE.BIN and
	// T5SPLIT.BIN were stamped 9:44:28, and the trace ended at global frame
	// 1792 while the diagnostic log recorded the real run finishing at 4997.
	// TH04 escaped it only because its own OP never reached the timeout.
	// The presence of a terminal status file is the run's own record that it
	// is over; the host harness deletes it before each run.
	fh = oracle_dos_open(ORACLE_DONE_FN, ORACLE_ACCESS_READ);
	if(fh >= 0) {
		oracle_dos_close(fh);
		return ORACLE_DISABLED;
	}

	oracle_memclear(cfg, sizeof(cfg));
	fh = oracle_dos_open(ORACLE_CFG_FN, ORACLE_ACCESS_READ);
	if(fh < 0) {
		return ORACLE_DISABLED;
	}
	read_len = oracle_dos_read(fh, cfg, (sizeof(cfg) - 1));
	oracle_dos_close(fh);

	for(i = 0; i < read_len; i++) {
		if(
			(cfg[i] != ' ') && (cfg[i] != '\t') &&
			(cfg[i] != '\r') && (cfg[i] != '\n')
		) {
			mode = cfg[i];
			first = i;
			break;
		}
	}
	if((mode == 'p') || (mode == 'P')) {
		return ORACLE_PLAYBACK;
	}
	if((mode != 'r') && (mode != 'R')) {
		return ORACLE_DISABLED;
	}
#if !ORACLE_RECORD_SUPPORTED
	// A record request reaching a playback-only lineage is a harness mistake,
	// not a case to improvise around: staying DISABLED here would leave no
	// artifact and look like a crashed run. Fail loudly instead.
	oracle_done_write(ORT_ERR_UNSUPPORTED);
	return ORACLE_ERROR;
#else
	unsigned next;

	oracle_cfg_demo_num = 1;
	oracle_cfg_rank = 1;
	oracle_cfg_start_stage = ORACLE_CFG_STAGE_NONE;
	oracle_cfg_frames = ORACLE_CFG_FRAMES_NONE;
	// The three packed digits are consumed through a running cursor rather
	// than through the fixed `first + 1/2/3` offsets they used before, because
	// the frame bound has to start scanning wherever they actually stopped.
	// For every input that has ever been written the two are identical: each
	// optional digit is adjacent to the one before it, so a present digit
	// advances the cursor exactly one and an absent one advances it none.
	next = (first + 1);
	if((next < read_len) && (cfg[next] >= '1') && (cfg[next] <= '5')) {
		oracle_cfg_demo_num = static_cast<uint8_t>(cfg[next] - '0');
		next++;
	}
	if((next < read_len) && (cfg[next] >= '0') && (cfg[next] <= '3')) {
		oracle_cfg_rank = static_cast<uint8_t>(cfg[next] - '0');
		next++;
	}
	// Refuse, never clamp. A digit here is an explicit request for a specific
	// stage; silently narrowing an out-of-range one would record a scenario
	// nobody asked for and label it with the number they did ask for. Note the
	// test is on "is a digit at all", not on "is a digit in range". Treating an
	// invalid multi-digit suffix as an absent override would be coercion by
	// omission.
	if((next < read_len) && (cfg[next] >= '0') && (cfg[next] <= '9')) {
		if(cfg[next] > ('0' + STAGE_EXTRA)) {
			oracle_diag(
				'S', 'T', 'G',
				static_cast<uint32_t>(cfg[next] - '0'), STAGE_EXTRA
			);
			oracle_done_write(ORT_ERR_UNSUPPORTED);
			return ORACLE_ERROR;
		}
		oracle_cfg_start_stage = static_cast<uint8_t>(cfg[next] - '0');
		next++;
	}
	// Whitespace-separated, so it cannot be confused with a fourth packed
	// digit and so it can exceed 9. Absent -> 0 -> unbounded, which is the
	// path every stock scenario takes and the reason the frozen corpus
	// reproduces byte for byte.
	oracle_cfg_frames = oracle_cfg_number(cfg, read_len, &next);
	return ORACLE_RECORD;
#endif /* ORACLE_RECORD_SUPPORTED */
}

static void oracle_header_checksum_set(void)
{
	uint32_t hash;

	oracle_header.header_checksum = 0;
	hash = oracle_fnv1a(
		ORACLE_FNV1A_BASIS_A, &oracle_header, sizeof(oracle_header)
	);
	hash = oracle_fnv1a(hash, &oracle_startup, sizeof(oracle_startup));
	oracle_header.header_checksum = hash;
}

#if ORACLE_RECORD_SUPPORTED
// Rewrites the header/startup prefix. Called once at record start and again at
// every checkpoint, so an interrupted recording still describes exactly the
// samples it actually committed.
static bool oracle_header_write(bool create)
{
	int fh;

	oracle_header.sample_count = oracle_sample_count;
	oracle_header.record_count = oracle_record_count;
	oracle_header.payload_size = (
		oracle_record_count * static_cast<uint32_t>(ORACLE_RECORD_SIZE)
	);
	oracle_header.total_size = (
		oracle_header.payload_offset + oracle_header.payload_size
	);
	oracle_header.payload_checksum = oracle_payload_checksum;
	oracle_header_checksum_set();

	fh = (create
		? oracle_dos_create(ORACLE_BIN_FN)
		: oracle_dos_open_rw(ORACLE_BIN_FN)
	);
	if(fh < 0) {
		return false;
	}
	oracle_dos_seek(fh, 0);
	if(
		oracle_dos_write(fh, &oracle_header, sizeof(oracle_header)) !=
		sizeof(oracle_header)
	) {
		oracle_dos_close(fh);
		return false;
	}
	if(
		oracle_dos_write(fh, &oracle_startup, sizeof(oracle_startup)) !=
		sizeof(oracle_startup)
	) {
		oracle_dos_close(fh);
		return false;
	}
	oracle_dos_close(fh);
	return true;
}
#endif /* ORACLE_RECORD_SUPPORTED */

static bool oracle_header_read(void)
{
	uint32_t stored;
	uint32_t computed;
	unsigned i;
	int fh;

	fh = oracle_dos_open(ORACLE_BIN_FN, ORACLE_ACCESS_READ);
	if(fh < 0) {
		return false;
	}
	if(
		oracle_dos_read(fh, &oracle_header, sizeof(oracle_header)) !=
		sizeof(oracle_header)
	) {
		oracle_dos_close(fh);
		return false;
	}
	if(
		oracle_dos_read(fh, &oracle_startup, sizeof(oracle_startup)) !=
		sizeof(oracle_startup)
	) {
		oracle_dos_close(fh);
		return false;
	}
	oracle_dos_close(fh);

	if(
		(oracle_header.magic[0] != 'T') ||
		(oracle_header.magic[1] != ORACLE_MAGIC_DIGIT) ||
		(oracle_header.magic[2] != 'C') ||
		(oracle_header.magic[3] != 'A') ||
		(oracle_header.magic[4] != 'S') ||
		(oracle_header.magic[5] != 'E') ||
		(oracle_header.magic[6] != '1') ||
		(oracle_header.magic[7] != '\0')
	) {
		return false;
	}
	if(
		(oracle_header.version != ORACLE_VERSION) ||
		(oracle_header.header_size != sizeof(oracle_header)) ||
		(oracle_header.startup_size != sizeof(oracle_startup)) ||
		(oracle_header.record_size != ORACLE_RECORD_SIZE) ||
		(oracle_header.input_semantics != ORACLE_INPUT_SEMANTICS) ||
		(oracle_header.ruleset_id != ORACLE_RULESET_CLASSIC) ||
		(oracle_header.first_process != ORACLE_PROCESS_MAIN)
	) {
		return false;
	}
	if(oracle_header.flags & ~ORACLE_KNOWN_FLAGS) {
		return false;
	}
	if(
		(oracle_header.source_kind != ORACLE_SOURCE_DIRECT) &&
		(oracle_header.source_kind != ORACLE_SOURCE_NORMALIZED)
	) {
		return false;
	}
	if(oracle_header.payload_offset != ORACLE_PREFIX_SIZE) {
		return false;
	}
	if(
		oracle_header.payload_size !=
		(oracle_header.record_count * static_cast<uint32_t>(ORACLE_RECORD_SIZE))
	) {
		return false;
	}
	if(oracle_header.sample_count > oracle_header.record_count) {
		return false;
	}
	if(
		oracle_header.total_size !=
		(oracle_header.payload_offset + oracle_header.payload_size)
	) {
		return false;
	}
	for(i = 0; i < sizeof(oracle_startup.reserved); i++) {
		if(oracle_startup.reserved[i] != 0) {
			return false;
		}
	}
	// An oracle case must not carry a debug flag, and it is REFUSED here
	// rather than coerced into `resident` further down. The harness TxCASE
	// contract requires this of every game; this closes the TH04/TH05 half.
	//
	// It is not a formality in either game:
	//
	// * TH05's `debug` is live, and replay loading would apply it *before* the game
	//   reads it. `oracle_entry()` runs from
	//   `ems_allocate_and_preload_eyecatch()` (`th05_main.asm:344`), while
	//   `th05_main.asm:748-763` overrides `resident->stage` from `debug_stage`
	//   and `_power` from `debug_power` -- and then clears `debug_mode` --
	//   before the demo gate at `:765`. So `oracle_startup_apply()` writing a
	//   set flag back would silently relocate the case to a different stage
	//   and power level while still reporting a successful playback.
	// * TH04's `debug` selects a DIFFERENT MAIN BINARY:
	//   `op_exit_into_main()` execs `BINARY_DEB` rather than `BINARY_MAIN`
	//   (`th04/op/start.hpp:29-33`). A case carrying it does not describe this
	//   executable at all.
	//
	// This is what makes the debug-mode ruling in
	// `kb/conventions/rec98-taxonomy.md` -- "no legitimate recorded run can
	// reach it" -- mechanically true for TH04/TH05 rather than asserted.
	//
	// `debug_stage` / `debug_power` (TH05 only) are deliberately NOT required
	// zero. `th05_main.asm:754` reads them only when `debug_mode` is set, so
	// with the flag refused they are inert; they stay in the startup block
	// because they are recorded state, and requiring them zero would reject
	// legitimate cases for a field that provably cannot act.
	if(oracle_startup.debug != 0) {
		// Attributable: `error:case-header` has ~15 causes, and a control run
		// that cannot tell them apart proves nothing.
		oracle_diag('D', 'B', 'G', oracle_startup.debug, 0);
		return false;
	}

	stored = oracle_header.header_checksum;
	oracle_header_checksum_set();
	computed = oracle_header.header_checksum;
	oracle_header.header_checksum = stored;
	return (stored == computed);
}

#if ORACLE_RECORD_SUPPORTED
// Writes the buffered records at their payload offset and empties the buffer.
static bool oracle_recbuf_flush(void)
{
	uint32_t offset;
	unsigned len;
	unsigned written;
	int fh;

	if(oracle_recbuf_len == 0) {
		return true;
	}
	offset = (
		oracle_header.payload_offset +
		(oracle_recbuf_base * static_cast<uint32_t>(ORACLE_RECORD_SIZE))
	);
	len = (oracle_recbuf_len * ORACLE_RECORD_SIZE);
	fh = oracle_dos_open_rw(ORACLE_BIN_FN);
	if(fh < 0) {
		oracle_diag('F', 'L', 'O', offset, 0xFFFFFFFFUL);
		return false;
	}
	if(!oracle_dos_seek(fh, offset)) {
		oracle_dos_close(fh);
		oracle_diag('F', 'L', 'S', offset, 0);
		return false;
	}
	written = oracle_dos_write(fh, oracle_recbuf, len);
	if(written != len) {
		oracle_dos_close(fh);
		oracle_diag('F', 'L', 'W', offset, written);
		return false;
	}
	oracle_dos_close(fh);
	oracle_recbuf_base += oracle_recbuf_len;
	oracle_recbuf_len = 0;
	return true;
}

static bool oracle_record_append(const oracle_record_t far *rec)
{
	oracle_record_t far *slot;

	if(oracle_recbuf_len >= ORACLE_RECBUF_COUNT) {
		if(!oracle_recbuf_flush()) {
			return false;
		}
	}
	slot = &oracle_recbuf[oracle_recbuf_len];
	oracle_record_copy(slot, rec);
	oracle_recbuf_len++;
	oracle_payload_checksum = oracle_fnv1a(
		oracle_payload_checksum, rec, ORACLE_RECORD_SIZE
	);
	oracle_record_count++;
	return true;
}
#endif /* ORACLE_RECORD_SUPPORTED */

// Sequential read-ahead. The payload checksum is accumulated in file order,
// exactly once per record, so it matches the writer's.
static bool oracle_record_fetch(uint32_t index, oracle_record_t far *rec)
{
	uint32_t offset;
	unsigned want;
	unsigned got;
	uint32_t remaining;
	int fh;

	if(index >= oracle_header.record_count) {
		return false;
	}
	if(
		(oracle_recbuf_len == 0) ||
		(index < oracle_recbuf_base) ||
		(index >= (oracle_recbuf_base + oracle_recbuf_len))
	) {
		remaining = (oracle_header.record_count - index);
		want = ((remaining > ORACLE_RECBUF_COUNT)
			? ORACLE_RECBUF_COUNT
			: static_cast<unsigned>(remaining)
		);
		offset = (
			oracle_header.payload_offset +
			(index * static_cast<uint32_t>(ORACLE_RECORD_SIZE))
		);
		fh = oracle_dos_open(ORACLE_BIN_FN, ORACLE_ACCESS_READ);
		if(fh < 0) {
			return false;
		}
		oracle_dos_seek(fh, offset);
		got = oracle_dos_read(fh, oracle_recbuf, (want * ORACLE_RECORD_SIZE));
		oracle_dos_close(fh);
		if(got != (want * ORACLE_RECORD_SIZE)) {
			return false;
		}
		oracle_recbuf_base = index;
		oracle_recbuf_len = want;
	}
	oracle_record_copy(
		rec, &oracle_recbuf[static_cast<unsigned>(index - oracle_recbuf_base)]
	);
	oracle_payload_checksum = oracle_fnv1a(
		oracle_payload_checksum, rec, ORACLE_RECORD_SIZE
	);
	return true;
}
/// ---------

/// Startup block
/// -------------

#if ORACLE_RECORD_SUPPORTED
static void oracle_startup_capture(void)
{
	int i;
#if (GAME == 5)
	int j;
#endif

	oracle_memclear(&oracle_startup, sizeof(oracle_startup));
	// Captured at MAIN entry, which is *before* `random_seed = 318`
	// (`th04_main.asm:698`, `th05_main.asm:780`) and before stage init's
	// `randring_fill()`. The value the contract asks for is the seed
	// immediately before that fill, so it is written again in
	// `oracle_session_start()`.
	oracle_startup.random_seed = static_cast<int32_t>(random_seed);
	oracle_startup.resident_rand = resident->rand;
	oracle_startup.slow_frames = resident->slow_frames;
	oracle_startup.frames = resident->frames;
	for(i = 0; i < ORACLE_SCORE_DIGITS; i++) {
		oracle_startup.score_last[i] = resident->score_last.digits[i];
	}
	oracle_startup.std_frames = resident->std_frames;
	oracle_startup.items_spawned = resident->items_spawned;
	oracle_startup.items_collected = resident->items_collected;
	oracle_startup.point_items_collected = resident->point_items_collected;
	oracle_startup.max_valued_point_items_collected =
		resident->max_valued_point_items_collected;
	oracle_startup.enemies_gone = resident->enemies_gone;
	oracle_startup.enemies_killed = resident->enemies_killed;
	oracle_startup.graze = resident->graze;
	oracle_startup.credit_lives = resident->credit_lives;
	oracle_startup.credit_bombs = resident->credit_bombs;
	oracle_startup.cfg_lives = resident->cfg_lives;
	oracle_startup.cfg_bombs = resident->cfg_bombs;
	oracle_startup.rank = resident->rank;
	oracle_startup.bgm_mode = resident->bgm_mode;
	oracle_startup.se_mode = resident->se_mode;
	oracle_startup.stage = resident->stage;
	oracle_startup.turbo_mode = resident->turbo_mode;
	oracle_startup.end_sequence = resident->end_sequence;
	oracle_startup.miss_count = resident->miss_count;
	oracle_startup.bombs_used = resident->bombs_used;
	oracle_startup.demo_stage = resident->demo_stage;
	oracle_startup.demo_num = resident->demo_num;
	oracle_startup.zunsoft_shown = resident->zunsoft_shown;
	oracle_startup.stage_id = stage_id;
	oracle_startup.power = power;
	oracle_startup.playperf = playperf;
	oracle_startup.ems_present = ((Ems != 0) ? 1 : 0);
#if (GAME == 5)
	for(i = 0; i < ORACLE_SCORE_DIGITS; i++) {
		oracle_startup.score_highest[i] = resident->score_highest.digits[i];
	}
	for(i = 0; i < 6; i++) {
		for(j = 0; j < ORACLE_SCORE_DIGITS; j++) {
			oracle_startup.stage_score[i][j] =
				resident->stage_score[i].digits[j];
		}
	}
	oracle_startup.playchar = resident->playchar;
	oracle_startup.debug = resident->debug;
	oracle_startup.debug_stage = resident->debug_stage;
	oracle_startup.debug_power = resident->debug_power;
	oracle_startup.unknown = static_cast<uint8_t>(resident->unknown);
#else
	oracle_startup.rem_lives = resident->rem_lives;
	oracle_startup.rem_bombs = resident->rem_bombs;
	oracle_startup.playchar_ascii = resident->playchar_ascii;
	oracle_startup.stage_ascii = static_cast<uint8_t>(resident->stage_ascii);
	oracle_startup.shottype = static_cast<uint8_t>(resident->shottype);
	oracle_startup.end_type_ascii =
		static_cast<uint8_t>(resident->end_type_ascii);
	oracle_startup.debug = resident->debug;
#endif
}
#endif /* ORACLE_RECORD_SUPPORTED */

// Pre-init write. Runs as the first statement of `demo_load()`, i.e. after the
// game has committed to the demo path but before it derives the buffer size and
// the DEMO?.REC file name from `resident->demo_num`, and before it propagates
// `resident->demo_stage` into `resident->stage` / `_stage_id`. Everything the
// game's own code then derives is derived normally.
//
// Only the resident fields are applied. The MAIN-local ones (`stage_id`,
// `power`, `playperf`) are recreated by the game's own initialization and are
// therefore *verified*, not restored — see `oracle_startup_verify()`.
static void oracle_startup_apply(void)
{
	int i;
#if (GAME == 5)
	int j;
#endif

	resident->rand = oracle_startup.resident_rand;
	resident->slow_frames = oracle_startup.slow_frames;
	resident->frames = oracle_startup.frames;
	for(i = 0; i < ORACLE_SCORE_DIGITS; i++) {
		resident->score_last.digits[i] = oracle_startup.score_last[i];
	}
	resident->std_frames = oracle_startup.std_frames;
	resident->items_spawned = oracle_startup.items_spawned;
	resident->items_collected = oracle_startup.items_collected;
	resident->point_items_collected = oracle_startup.point_items_collected;
	resident->max_valued_point_items_collected =
		oracle_startup.max_valued_point_items_collected;
	resident->enemies_gone = oracle_startup.enemies_gone;
	resident->enemies_killed = oracle_startup.enemies_killed;
	resident->graze = oracle_startup.graze;
	resident->credit_lives = oracle_startup.credit_lives;
	resident->credit_bombs = oracle_startup.credit_bombs;
	resident->cfg_lives = oracle_startup.cfg_lives;
	resident->cfg_bombs = oracle_startup.cfg_bombs;
	resident->rank = oracle_startup.rank;
	resident->bgm_mode = oracle_startup.bgm_mode;
	resident->se_mode = oracle_startup.se_mode;
	resident->stage = oracle_startup.stage;
	resident->turbo_mode = oracle_startup.turbo_mode;
	resident->end_sequence = oracle_startup.end_sequence;
	resident->miss_count = oracle_startup.miss_count;
	resident->bombs_used = oracle_startup.bombs_used;
	resident->demo_stage = oracle_startup.demo_stage;
	resident->demo_num = oracle_startup.demo_num;
	resident->zunsoft_shown = oracle_startup.zunsoft_shown;
#if (GAME == 5)
	for(i = 0; i < ORACLE_SCORE_DIGITS; i++) {
		resident->score_highest.digits[i] = oracle_startup.score_highest[i];
	}
	for(i = 0; i < 6; i++) {
		for(j = 0; j < ORACLE_SCORE_DIGITS; j++) {
			resident->stage_score[i].digits[j] =
				oracle_startup.stage_score[i][j];
		}
	}
	resident->playchar = oracle_startup.playchar;
	resident->debug = oracle_startup.debug;
	resident->debug_stage = oracle_startup.debug_stage;
	resident->debug_power = oracle_startup.debug_power;
	resident->unknown = static_cast<char>(oracle_startup.unknown);
#else
	resident->rem_lives = oracle_startup.rem_lives;
	resident->rem_bombs = oracle_startup.rem_bombs;
	resident->playchar_ascii = oracle_startup.playchar_ascii;
	resident->stage_ascii = static_cast<char>(oracle_startup.stage_ascii);
	resident->shottype = static_cast<char>(oracle_startup.shottype);
	resident->end_type_ascii =
		static_cast<char>(oracle_startup.end_type_ascii);
	resident->debug = oracle_startup.debug;
#endif
}

// Post-init verify, NOT post-init restore. Because the scenario start is pinned
// by the game's own code, the correct behavior for a field the normal path
// recreates is to compare and fail. A mismatch is a startup-logic divergence
// and must be reported, never papered over.
static bool oracle_startup_verify(void)
{
	if(oracle_startup.stage_id != stage_id) {
		oracle_diag('S', 'I', 'D', oracle_startup.stage_id, stage_id);
		return false;
	}
	if(oracle_startup.power != power) {
		oracle_diag('P', 'W', 'R', oracle_startup.power, power);
		return false;
	}
	if(oracle_startup.playperf != playperf) {
		oracle_diag('P', 'P', 'F', oracle_startup.playperf, playperf);
		return false;
	}
	if(oracle_startup.rank != rank) {
		oracle_diag('R', 'N', 'K', oracle_startup.rank, rank);
		return false;
	}
	if(oracle_startup.ems_present != ((Ems != 0) ? 1 : 0)) {
		oracle_diag('E', 'M', 'S', oracle_startup.ems_present, (Ems != 0));
		return false;
	}
	if(oracle_startup.random_seed != static_cast<int32_t>(random_seed)) {
		oracle_diag(
			'S', 'E', 'D',
			static_cast<uint32_t>(oracle_startup.random_seed),
			static_cast<uint32_t>(random_seed)
		);
		return false;
	}
	return true;
}
/// -------------

/// Session
/// -------

// Emitted on the first injected frame, which is the first moment at which the
// demo gate, `random_seed = 318` and stage init's `randring_fill()` have all
// completed and no input has been consumed yet. `start` and `round_start`
// coincide there for a single-stage demo case; a multi-stage case would emit
// `round_start` again from a stage hook.
static void oracle_session_start(void)
{
	if(oracle_started) {
		return;
	}
	oracle_started = true;

#if ORACLE_RECORD_SUPPORTED
	if(oracle_mode == ORACLE_RECORD) {
		// The contract's `random_seed` is the value immediately before the
		// case's first `randring_fill()`, i.e. 318 on the demo path. Recapture
		// the three MAIN-local fields at the same boundary.
		oracle_startup.random_seed = static_cast<int32_t>(random_seed);
		oracle_startup.stage_id = stage_id;
		oracle_startup.power = power;
		oracle_startup.playperf = playperf;
		oracle_startup.rank = rank;
		oracle_startup.ems_present = ((Ems != 0) ? 1 : 0);
		if(!oracle_header_write(true)) {
			oracle_mode = ORACLE_ERROR;
			oracle_done_write(ORT_ERR_CASE_CREATE);
			return;
		}
	} else
#endif
	if(!oracle_startup_verify()) {
		oracle_split_write_header();
		oracle_split_row(ORACLE_EVENT_ERROR, 0);
		oracle_mode = ORACLE_ERROR;
		oracle_done_write(ORT_ERR_STARTUP);
		return;
	}

	oracle_split_write_header();
	oracle_split_row(ORACLE_EVENT_START, 0);
	oracle_split_row(ORACLE_EVENT_ROUND_START, 0);
	oracle_diag('S', 'T', 'A', static_cast<uint32_t>(random_seed), stage_id);
}

static void oracle_finish(oracle_text_id_t status)
{
#if ORACLE_RECORD_SUPPORTED
	oracle_record_t rec;
#endif

	if(oracle_finished) {
		return;
	}
	oracle_finished = true;

#if ORACLE_RECORD_SUPPORTED
	if(oracle_mode == ORACLE_RECORD) {
		oracle_memclear(&rec, sizeof(rec));
		rec.kind = ORACLE_RECORD_CONTROL;
		rec.phase = ORACLE_PHASE_CONTROL;
		rec.scenario_cursor = 0xFFFF;
		rec.frame_index = oracle_global_frame;
		rec.control = ORACLE_CONTROL_TERMINAL;
		if(!oracle_record_append(&rec) || !oracle_recbuf_flush()) {
			oracle_mode = ORACLE_ERROR;
			oracle_done_write(ORT_ERR_CASE_FINALIZE);
			return;
		}
		if(!oracle_header_write(false)) {
			oracle_mode = ORACLE_ERROR;
			oracle_done_write(ORT_ERR_CASE_FINALIZE);
			return;
		}
	}
#endif /* ORACLE_RECORD_SUPPORTED */
	oracle_split_row(ORACLE_EVENT_FINISH, 0);
	oracle_diag('F', 'I', 'N', oracle_global_frame, oracle_record_count);
	oracle_done_write(status);
}
/// -------

/// Hooks
/// -----

// Replicates the scenario pinning that OP's `start_demo()` performs
// (`th04/op/start.cpp:53-86`, `th05/op/start.cpp:57-107`), for a chosen
// `demo_num` rather than the cycling one, so that a recording is reproducible
// without going through OP at all.
//
// Only the fields those functions actually write are written here. Everything
// else is left exactly as RES_HUMA.COM / RES_KSO.COM initialized it, and is
// captured verbatim into the startup block.
// `start_stage` is `ORACLE_CFG_STAGE_NONE` for every stock scenario, and that
// path executes exactly the statements this function executed before the start
// stage existed -- which is what keeps every previously recorded case
// reproducible byte for byte.
//
// `resident->stage` is deliberately left at 0 even when a stage IS chosen,
// because `start_demo()` leaves it at 0 too (`th04/op/start.cpp:58`) and MAIN
// re-derives the live stage from `demo_stage` a moment later:
// `stage_id = resident->stage = resident->demo_stage`, unclamped
// (`th04_main.asm:534-545`). Writing `resident->stage` here instead would move
// the eyecatch/rank derivation in `ems_allocate_and_preload_eyecatch()`
// (`th04/main/ems.cpp:64-73`), which runs BEFORE the demo gate -- changing
// which asset a recording loads, and with it the startup block of every stock
// case. The override therefore touches exactly one field.
#if ORACLE_RECORD_SUPPORTED
static void oracle_scenario_pin(
	uint8_t demo_num, uint8_t rank_value, uint8_t start_stage
)
{
	resident->stage = 0;
	resident->credit_lives = 3;
	resident->credit_bombs = 3;
	resident->rank = rank_value;
	resident->demo_num = demo_num;
#if (GAME == 5)
	resident->end_sequence = ES_SCORE;
	switch(demo_num) {
	case 1: resident->playchar = PLAYCHAR_REIMU;  resident->demo_stage = 3; break;
	case 2: resident->playchar = PLAYCHAR_MARISA; resident->demo_stage = 1; break;
	case 3: resident->playchar = PLAYCHAR_MIMA;   resident->demo_stage = 2; break;
	case 4: resident->playchar = PLAYCHAR_YUUKA;  resident->demo_stage = 4; break;
	default:
		resident->playchar = PLAYCHAR_MIMA;
		resident->demo_stage = STAGE_EXTRA;
		break;
	}
	if(start_stage != ORACLE_CFG_STAGE_NONE) {
		resident->demo_stage = start_stage;
	}
#else
	switch(demo_num) {
	case 1: resident->demo_stage = 3; resident->playchar_ascii = ('0' + PLAYCHAR_REIMU);  resident->shottype = SHOTTYPE_A; break;
	case 2: resident->demo_stage = 0; resident->playchar_ascii = ('0' + PLAYCHAR_MARISA); resident->shottype = SHOTTYPE_A; break;
	case 3: resident->demo_stage = 2; resident->playchar_ascii = ('0' + PLAYCHAR_REIMU);  resident->shottype = SHOTTYPE_B; break;
	default: resident->demo_stage = 1; resident->playchar_ascii = ('0' + PLAYCHAR_MARISA); resident->shottype = SHOTTYPE_B; break;
	}
	if(start_stage != ORACLE_CFG_STAGE_NONE) {
		resident->demo_stage = start_stage;
	}
	// Derived from the FINAL `demo_stage`, so the override cannot leave the
	// ASCII copy describing the stage the demo table would have chosen. MAIN
	// recomputes it identically at `th04_main.asm:542-543`.
	resident->stage_ascii = ('0' + resident->demo_stage);
#endif
}
#endif /* ORACLE_RECORD_SUPPORTED */

void oracle_entry(void)
{
	oracle_paths_init();
	if(oracle_mode != ORACLE_DISABLED) {
		return;
	}
	oracle_mode = oracle_cfg_mode();
	if(oracle_mode == ORACLE_DISABLED) {
		return;
	}
	// UNCONDITIONAL since the start-stage digit was added, and that is a
	// correctness fix rather than a tidy-up.
	//
	// It used to be `#if !ORACLE_RECORD_SUPPORTED`, on the reasoning that only
	// a playback-only build could reach ORACLE_ERROR here -- the mode
	// `oracle_cfg_mode()` returned when it refused a record request -- and that
	// making it unconditional would needlessly change the recording lineage's
	// certified MAIN.EXE. Both halves of that reasoning are now void:
	// `oracle_cfg_mode()` also refuses an out-of-range start stage, which only
	// a RECORDING build can ask for, and this parcel rebuilds that binary
	// anyway. Left conditional, a refused stage would have fallen straight
	// through into the playback path and tried to read a case that the run
	// never intended to play.
	if(oracle_mode == ORACLE_ERROR) {
		return;
	}
	oracle_global_frame = 0;
	oracle_sample_count = 0;
	oracle_record_count = 0;
	oracle_payload_checksum = ORACLE_FNV1A_BASIS_A;
	oracle_recbuf_len = 0;
	oracle_recbuf_pos = 0;
	oracle_recbuf_base = 0;

#if ORACLE_RECORD_SUPPORTED
	if(oracle_mode == ORACLE_RECORD) {
#if (GAME == 5)
		// `[measured]` The Extra replay is two files spliced by
		// `dialog_animate()` (`th05/main/dialog/dialog.cpp:235-281`), and
		// `DemoPlay`'s end condition is bypassed entirely for
		// `demo_num > 4` (`th04/main/demo.cpp:54-59`), so a recording of it
		// never reaches a terminal boundary -- an unattended run just gets
		// killed by its timeout after ~12000 frames. Version 1 refuses it on
		// BOTH sides rather than emitting a case that cannot be replayed.
		if(oracle_cfg_demo_num > 4) {
			oracle_mode = ORACLE_ERROR;
			oracle_done_write(ORT_ERR_UNSUPPORTED);
			return;
		}
#endif
		oracle_memclear(&oracle_header, sizeof(oracle_header));
		oracle_header.magic[0] = 'T';
		oracle_header.magic[1] = ORACLE_MAGIC_DIGIT;
		oracle_header.magic[2] = 'C';
		oracle_header.magic[3] = 'A';
		oracle_header.magic[4] = 'S';
		oracle_header.magic[5] = 'E';
		oracle_header.magic[6] = '1';
		oracle_header.magic[7] = '\0';
		oracle_header.version = ORACLE_VERSION;
		oracle_header.header_size = sizeof(oracle_header);
		oracle_header.startup_size = sizeof(oracle_startup);
		oracle_header.record_size = ORACLE_RECORD_SIZE;
		oracle_header.payload_offset = ORACLE_PREFIX_SIZE;
		oracle_header.source_kind = ORACLE_SOURCE_NORMALIZED;
		oracle_header.input_semantics = ORACLE_INPUT_SEMANTICS;
		oracle_header.ruleset_id = ORACLE_RULESET_CLASSIC;
		oracle_header.first_process = ORACLE_PROCESS_MAIN;
		oracle_header.producer = ORACLE_PRODUCER;
		oracle_header.flags = 0;
		oracle_header.case_id = 0;
		oracle_header.source_digest = 0;
		oracle_header.source_commit = ORACLE_SOURCE_COMMIT;
		oracle_scenario_pin(
			oracle_cfg_demo_num, oracle_cfg_rank, oracle_cfg_start_stage
		);
		oracle_header.scenario_id = resident->demo_num;
		oracle_startup_capture();
		oracle_diag('R', 'E', 'C', resident->demo_num, resident->rank);
		// Emitted unconditionally, so a trace can never be silently ambiguous
		// about which stage it was taken on: the second word distinguishes a
		// chosen stage from the demo table's own.
		oracle_diag(
			'S', 'T', 'G', resident->demo_stage,
			((oracle_cfg_start_stage != ORACLE_CFG_STAGE_NONE) ? 1 : 0)
		);
		// Same reasoning as `STG`, one field over: a case that ends early
		// because it was TOLD to and a case that ended early because the run
		// died must never look alike in a trace. First word the bound, second
		// word 1 if a bound is in force.
		oracle_diag(
			'F', 'R', 'B', oracle_cfg_frames,
			((oracle_cfg_frames != ORACLE_CFG_FRAMES_NONE) ? 1 : 0)
		);
		return;
	}
#endif /* ORACLE_RECORD_SUPPORTED */

	// Playback. Read and fully validate the case before applying anything.
	if(!oracle_header_read()) {
		oracle_mode = ORACLE_ERROR;
		oracle_done_write(ORT_ERR_CASE_HEADER);
		return;
	}
	// Version 1 does not represent the TH05 Extra splice: a 20000-frame case
	// exceeds what a single 16-bit payload cursor and one CURSOR_RESET site
	// have been calibrated for, and `state/re/DETERMINISTIC_STATE_TH05.md` §6
	// still lists the second reset site as `[open]`. Fail loudly rather than
	// silently producing an unfaithful trace.
	if(oracle_header.flags & ORACLE_FLAG_SPLICED_SOURCE) {
		oracle_mode = ORACLE_ERROR;
		oracle_done_write(ORT_ERR_UNSUPPORTED);
		return;
	}
	oracle_startup_apply();
	oracle_diag('P', 'L', 'Y', oracle_header.record_count, resident->demo_num);
}

bool oracle_active(void)
{
	return ((oracle_mode == ORACLE_RECORD) || (oracle_mode == ORACLE_PLAYBACK));
}

bool oracle_frame(uint16_t shift_offset)
{
	oracle_record_t rec;
	uint8_t key_replay;
	uint8_t shift;
	bool keep_going;

	oracle_session_start();
	if(oracle_mode == ORACLE_ERROR) {
		return false;
	}
#if !ORACLE_RECORD_SUPPORTED
	// Only the recorder reads `DemoBuf[stage_frame + shift_offset]`. The
	// parameter stays in the signature because it is the shared hook ABI that
	// `th04/main/demo.cpp` calls on every lineage.
	(void)shift_offset;
#endif

#if ORACLE_RECORD_SUPPORTED
	if(oracle_mode == ORACLE_RECORD) {
		// Exactly ZUN's own two reads (`th04/main/demo.cpp:52-53`), minus the
		// abort-on-keypress guard, which must not be reproduced: it would make
		// the run depend on the host keyboard, which is the opposite of an
		// oracle. Recorded as a deviation in the TH04/TH05 delta index.
		key_replay = DemoBuf[stage_frame];
		shift = DemoBuf[stage_frame + shift_offset];

		oracle_memclear(&rec, sizeof(rec));
		rec.kind = ORACLE_RECORD_INPUT;
		rec.phase = ORACLE_PHASE_GAMEPLAY;
		rec.scenario_cursor = stage_frame;
		rec.frame_index = oracle_global_frame;
		rec.key_det_replay = key_replay;
		rec.shiftkey = shift;
		rec.control = 0;
		if(!oracle_record_append(&rec)) {
			oracle_split_row(ORACLE_EVENT_ERROR, 0);
			oracle_mode = ORACLE_ERROR;
			oracle_done_write(ORT_ERR_FRAME_IO);
			return false;
		}
		oracle_sample_count++;

		keep_going = (
#if (GAME == 5)
			(resident->demo_num > 4) ||
#endif
			(stage_frame < (DEMO_N - 4))
		);
		// The frame bound, applied AFTER the append and after the increment,
		// so `oracle_cfg_frames` is exactly the number of input records the
		// case ends up holding -- `oracle_finish()` then adds the one terminal
		// control record, which is what version 1 playback expects.
		//
		// Written as a disjunction on the sentinel rather than as a default
		// value, so that an absent bound adds NO term to the stock condition.
		// That is the whole additivity argument: `r11` executes the same
		// comparison it executed before this field existed, which is why the
		// frozen corpus reproduces by whole-file SHA-256 rather than by
		// column comparison.
		if(
			(oracle_cfg_frames != ORACLE_CFG_FRAMES_NONE) &&
			(oracle_sample_count >= oracle_cfg_frames)
		) {
			keep_going = false;
		}
	} else
#endif /* ORACLE_RECORD_SUPPORTED */
	{
		if(!oracle_record_fetch(oracle_record_count, &rec)) {
			oracle_split_row(ORACLE_EVENT_ERROR, 0);
			oracle_mode = ORACLE_ERROR;
			oracle_done_write(ORT_ERR_FRAME_IO);
			return false;
		}
		oracle_record_count++;
		if(
			(rec.kind != ORACLE_RECORD_INPUT) ||
			(rec.frame_index != oracle_global_frame) ||
			(rec.scenario_cursor != stage_frame)
		) {
			oracle_diag('D', 'S', 'Y', rec.scenario_cursor, stage_frame);
			oracle_split_row(ORACLE_EVENT_ERROR, 0);
			oracle_mode = ORACLE_ERROR;
			oracle_done_write(ORT_ERR_DESYNC);
			return false;
		}
		key_replay = rec.key_det_replay;
		shift = rec.shiftkey;
		oracle_sample_count++;

		// Consume the terminal control record on the SAME frame the recorder
		// stopped on, not on the frame after it. Deferring it by one call
		// would let the game run one extra gameplay frame, which Gate A
		// measured as a divergent `finish` row: score 04569430 -> 04569820 on
		// the very first calibration pair.
		// A version-1 case is a run of input records followed by exactly one
		// terminal control record, so "one record left" identifies the last
		// input frame without any lookahead.
		keep_going = true;
		if(oracle_record_count >= oracle_header.record_count) {
			// The case ended without a terminal control record.
			oracle_split_row(ORACLE_EVENT_ERROR, 0);
			oracle_mode = ORACLE_ERROR;
			oracle_done_write(ORT_ERR_CASE_FINALIZE);
			return false;
		}
		if(oracle_record_count == (oracle_header.record_count - 1)) {
			if(!oracle_record_fetch(oracle_record_count, &rec)) {
				oracle_split_row(ORACLE_EVENT_ERROR, 0);
				oracle_mode = ORACLE_ERROR;
				oracle_done_write(ORT_ERR_FRAME_IO);
				return false;
			}
			oracle_record_count++;
			if(
				(rec.kind != ORACLE_RECORD_CONTROL) ||
				(rec.control != ORACLE_CONTROL_TERMINAL)
			) {
				// Anything else is a case this build cannot play; fail loudly
				// rather than mistrace it.
				oracle_split_row(ORACLE_EVENT_ERROR, 0);
				oracle_mode = ORACLE_ERROR;
				oracle_done_write(ORT_ERR_UNSUPPORTED);
				return false;
			}
			keep_going = false;
		}
	}

	// The 8-bit store into the 16-bit variable clears the high byte every
	// frame, exactly as ZUN's own `DemoPlay` does. Consequence, and it is
	// deliberate: the frame loop's `test _key_det.hi, high INPUT_CANCEL`
	// immediately after this hook can never fire, so pause is unreachable
	// during injected playback. A case that needs a pause requires a format
	// revision, not a workaround.
	key_det = key_replay;
	shiftkey = (shift != 0);

	// AND rather than MOD: the cadence is a power of two, and a 32-bit modulo
	// would call a `mathl.lib` helper once per frame inside the very window
	// this module is supposed to measure without perturbing.
	static_assert(
		(ORACLE_SPLIT_INTERVAL_SAMPLES &
		 (ORACLE_SPLIT_INTERVAL_SAMPLES - 1)) == 0
	);
	if(
		((oracle_global_frame & (ORACLE_SPLIT_INTERVAL_SAMPLES - 1)) == 0) &&
		(oracle_global_frame != 0)
	) {
		oracle_split_row(
			ORACLE_EVENT_CHECKPOINT,
			static_cast<uint16_t>((static_cast<uint16_t>(shift) << 8) | key_replay)
		);
#if ORACLE_RECORD_SUPPORTED
		if(oracle_mode == ORACLE_RECORD) {
			// Rewrite the prefix at every checkpoint, so a run that is killed
			// mid-case still leaves a self-consistent file. Both results are
			// checked: silently ignoring them once produced a TH05 recording
			// that reported ok while its last 3200 records never reached disk.
			if(!oracle_recbuf_flush() || !oracle_header_write(false)) {
				oracle_mode = ORACLE_ERROR;
				oracle_done_write(ORT_ERR_CASE_FINALIZE);
				return false;
			}
		}
#endif
	}
	oracle_global_frame++;

	if(!keep_going) {
#if ORACLE_RECORD_SUPPORTED
		if(oracle_mode == ORACLE_RECORD) {
			oracle_finish(ORT_OK_RECORD);
		} else
#endif
		{
			// Record cursor, sample cursor and incremental payload checksum
			// are tracked separately, and success is refused unless all three
			// agree with the header.
			oracle_finish(
				((oracle_sample_count == oracle_header.sample_count) &&
				 (oracle_record_count == oracle_header.record_count) &&
				 (oracle_payload_checksum == oracle_header.payload_checksum))
					? ORT_OK_PLAYBACK
					: ORT_ERR_CASE_FINALIZE
			);
		}
		return false;
	}
	return true;
}
/// -----
