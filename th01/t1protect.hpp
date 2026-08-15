/* ReC98 — harness/ORACLE-TH01-MASTER (MOD BRANCH, NOT byte-identical)
 * -------------------------------------------------------------------
 * TH01 savestate-protection detector. REPLAY_CORE_CONTRACT.md §6.
 *
 * Ported from `th03/replay_protect.hpp` (ReC98 `origin/replays` @ 2dec6338,
 * 1,508 lines, read in full). The full porting dossier — every function, the
 * BPB field list, both interrupt wrappers clause by clause against
 * tools/replay/FORMAT.md:780-785, the root scan, the checkpoint order and the
 * three-way verdict — is `state/notes/t1case-protect.md`. Read that, not this
 * header, to understand WHY any of it is shaped the way it is.
 *
 * WHAT IT DEFENDS. A player takes an emulator savestate mid-run, dies, restores,
 * and keeps recording as though the death never happened. Every ordinary signal
 * is rolled back with the savestate — RAM counters, DOS handles, DOS's cached
 * directory entry. So the detector reads the physical disk in a way DOS cannot
 * fake: a root-relative guard file that grows by exactly one byte per checkpoint,
 * whose size is read by parsing the FAT directly through INT 25h (or DOS 7.1's
 * INT 21h/AX=7305h).
 *
 * The nine deliberate divergences from the reference, each with its dossier
 * reference; every one of them is forced, not stylistic:
 *
 *  1. The carrier is `t1case_res_t::protect[]`, not `resident_t::unused_3[]`.
 *     The reference has 14 `resident->unused_3[...]` sites (dossier §0a); every
 *     one is re-bound here. 39 of the reserved 45 bytes are used.
 *  2. The committed size is NOT duplicated into `protect[]`. It reuses
 *     `t1case_res_t::committed`, which is already the §7 committed/checkpoint
 *     union at carrier offset 40 (dossier §0b, checklist item 2). Duplicating it
 *     would create two copies of the one value the whole detection compares.
 *  3. The 4,106-byte sector buffer is a module `static` far pointer, NOT a
 *     carrier field. The reference keeps its segment in the carrier because
 *     MAINL's BSS is offset-pinned by the accel path's bulk image; TH01 has no
 *     accel layer, and the field must be cleared per process anyway. TH01
 *     self-`execl`s 8-10 times per run against TH03's 2-3, so a stale segment
 *     here would be 8-10 opportunities to free another process's memory
 *     (dossier §2.2, checklist item 6; REPLAY_CORE_CONTRACT.md §7.6).
 *  4. `hmem_allocbyte`/`hmem_free` become `farmalloc`/`farfree`, and master.lib's
 *     `file_*` become the module's own `t1f_*`. Neither `hmem_*`, `file_append`
 *     nor `file_flush` is among the routines th01_reiiden.asm includes, and
 *     adding an include to an original segment contribution is forbidden
 *     (TH01_ORACLE_DELTA_INDEX.md D4 as amended; dossier §0c).
 *  5. The flush step is DELETED. master.lib's `file_write` buffers; Turbo C++'s
 *     `write()` is a direct INT 21h AH=40h with no user-space buffer, so
 *     `replay_protect_flush_current_file` and every `file_ErrorStat` check
 *     collapse into `t1f_write()`'s own return check. AX=5D01h remains, and
 *     becomes the only commit step (checklist item 5).
 *  6. One guard file, `\T1LAST.GRD`, root-relative. The reference's numbered
 *     `TH3Gnn.TMP` branch is dead weight: the oracle lineage has no slots
 *     (`T1CASE_SLOT_NONE`). Root-relative is NOT a style choice — the scan only
 *     ever searches the root directory and matches on the basename, so a
 *     CWD-relative name would find a different file with the same name and
 *     happily read its size (dossier §10.1). TH01's other files are all
 *     CWD-relative, so this genuinely differs from its neighbours.
 *  7. `replay_protect_root_size_read_uncached` is not ported: zero callers
 *     anywhere in th03/ (dossier §1.10).
 *  8. `t1prt_diag_t` is renumbered densely, 26 codes against the reference's 47.
 *     Twelve of those 47 are never assigned and seven more are `RPD_MAIN*_`,
 *     named for TH03 binaries. The old->new mapping is in the dossier §8.
 *  9. The diagnostic sink is T1DIAG.TXT text records (`PRT`, `PSZ`), not a raw
 *     65-byte carrier dump. The gate has to be a program, which is the same
 *     lesson step 2 recorded when `HDR == preceding FIN` turned out to be wrong
 *     (checklist item 15).
 *
 * THE ONE THING NOT TO "SIMPLIFY": `lseek(fd, 0, SEEK_SET)` in the poison path.
 * It is what keeps the marker write inside the guard's already-allocated first
 * cluster, so the write allocates nothing and does not extend the file. An
 * append-based poison would allocate from a FAT the savestate is about to
 * restore, and the poison would vanish with it (dossier §7.2).
 */

#ifndef TH01_T1PROTECT_HPP
#define TH01_T1PROTECT_HPP

/// Diagnostic codes
/// ----------------
/// Dense. See the module comment, divergence 8.

enum t1prt_diag_t {
	T1PRT_NONE = 0,
	T1PRT_CTX_ALLOC,
	T1PRT_BPB_READ,
	T1PRT_BPB_UNSUPPORTED,
	T1PRT_ROOT_RANGE,
	T1PRT_SHORT_NAME,
	T1PRT_ROOT_SECTOR_READ,
	T1PRT_ROOT_NOT_FOUND,
	T1PRT_LOCATED_RANGE,
	T1PRT_LOCATED_SECTOR_READ,
	T1PRT_LOCATED_ENTRY_BAD,
	T1PRT_LOCATED_NAME,

	// The rollback signature AND a failed write. They share this code and the
	// same argument order on purpose, mirroring the reference; what separates
	// them is the FLAGS BIT and the SIGN of (actual - expected). Any test that
	// asserts on the code alone reports a cheat and an I/O failure identically.
	// See t1case_protect_verdict_emit() and dossier §6.1.
	T1PRT_VERIFY_MISMATCH,

	T1PRT_GUARD_CREATE,
	T1PRT_GUARD_CREATE_NONZERO,
	T1PRT_CHECKPOINT_APPEND,
	T1PRT_CHECKPOINT_WRITE,
	T1PRT_CHECKPOINT_MISMATCH,
	T1PRT_CLOSE_COMMIT,
	T1PRT_MARKER_CLUSTER,
	T1PRT_MARKER_VALUE,
	T1PRT_MARKER_WRITE,
	T1PRT_MARKER_VERIFY,
	T1PRT_ROOT_CHAIN,
	T1PRT_ROOT_FAT_READ,
	T1PRT_EXTENDED_READ_UNAVAILABLE,
	T1PRT_MARKER_SECTOR_READ
};

#define T1PRT_SECTOR_BUFFER_SIZE_MAX 4096
#define T1PRT_ABS_READ_PACKET_SIZE     10
#define T1PRT_SECTOR_ALLOCATION_SIZE \
	(T1PRT_SECTOR_BUFFER_SIZE_MAX + T1PRT_ABS_READ_PACKET_SIZE)

#define T1PRT_FAT12_CLUSTER_COUNT_MAX  4085UL
#define T1PRT_FAT16_CLUSTER_COUNT_MAX 65525UL
#define T1PRT_FAT32_CLUSTER_MAX  0x0FFFFFEFUL
#define T1PRT_FAT32_ENTRY_MASK   0x0FFFFFFFUL
#define T1PRT_FAT32_CLUSTER_BAD  0x0FFFFFF7UL
#define T1PRT_FAT32_CLUSTER_EOC  0x0FFFFFF8UL

#define T1PRT_FAT_ENTRY_FREE    0x00
#define T1PRT_FAT_ENTRY_DELETED 0xE5
#define T1PRT_FAT_ATTR_SKIP     0x18 // volume label | directory
#define T1PRT_DIR_ENTRY_SIZE      32
#define T1PRT_GUARD_BASE_SIZE      1

// The FAT width IS the enum value, so a diagnostic dump reads as itself.
#define T1PRT_FAT_UNKNOWN 0
#define T1PRT_FAT12      12
#define T1PRT_FAT16      16
#define T1PRT_FAT32      32

// Root-scan outcomes.
#define T1PRT_SCAN_CONTINUE 0
#define T1PRT_SCAN_FOUND    1
#define T1PRT_SCAN_END      2

struct t1prt_ctx_t {
	uint8_t far *sector;
	uint8_t far *packet;
	uint8_t drive;
	uint8_t fat_type;
	bool extended_abs_read;
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint32_t total_sectors;
	uint32_t fat_start;
	uint32_t fat_sectors;
	uint32_t root_start;
	uint32_t root_sectors;
	uint16_t root_entry_count;
	uint32_t first_data_sector;
	uint32_t cluster_count;
	uint32_t root_cluster;
};

#endif /* TH01_T1PROTECT_HPP */
