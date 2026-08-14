/* ReC98 — harness/ORACLE-TH02-MASTER (MOD BRANCH, NOT byte-identical)
 * -------------------------------------------------------------------
 * TH02 oracle case recorder/player and T2SPLIT trace writer, for MAIN.EXE.
 *
 * Ported from `th01/main/t1case.cpp` on harness/ORACLE-TH01-MASTER, which was
 * itself ported from TH03's `th03/main/t3case.cpp`. Structure, naming and
 * idioms follow the TH01 module deliberately. The justified divergences are
 * listed in state/notes/oracle-th02-bringup.md §2; the four that shape this
 * file are:
 *
 *  1. The cross-process carrier is a DISK FILE (T2CASE.ST), not a resident
 *     block. TH02's MAIN.EXE links no master.lib ResData at all — it obtains
 *     `resident` by reading a raw segment word out of huuma.cfg (cfg_load,
 *     th02_main.asm:2082-2131) — and adding `libs/master.lib/resdata.asm` to
 *     th02_main.asm would be adding code to an original segment contribution,
 *     which CLAUDE.md forbids. A file is also TH02-idiomatic (huuma.cfg is the
 *     game's own cross-process carrier) and, decisively, survives processes
 *     that carry no module: a case can span MAIN -> OP -> MAIN without an
 *     OP.EXE injector.
 *  2. The injection seam is ZUN's own `nopcall DemoPlay` operand
 *     (th02_main.asm:1651), redirected to t2case_frame_io(). TH02's input is
 *     one 16-bit [key_det] written by a stateless reader
 *     (input_reset_sense(), th02/hardware/input_rs.cpp — no [input_prev], no
 *     edge detection, no derived double-tap), so overwriting the variable is
 *     exact. TH01 could not do that and had to stub key_sense() instead.
 *  3. The cursor advances once per DemoPlay call = once per gameplay frame.
 *     TH01 had to key on input_sense() calls because six interstitial loops
 *     sense input without advancing the game; TH02's seam is inside the
 *     per-frame gameplay function only, so there is no interstitial to miss.
 *  4. No BGM deviation. TH01 had to force BGM_MODE_OFF because MDRV2 did not
 *     survive `execl`; TH02 re-runs snd_pmd_resident()/snd_mmd_resident() per
 *     process (th02_main.asm:715-734), so the stock audio path is left alone.
 *
 * None of the statics below are initialized data. A `_DATA` contribution from
 * this module would land between the original `_DATA` and `_BSS` inside
 * DGROUP and shift every original BSS offset. TH02 MAIN.EXE is 100%
 * position-independent as of 9fcfef8b and has no raw offsets, so this is not
 * load-bearing here the way it is for TH03 — but the discipline is free.
 * See kb/conventions/th03-mod-layout-verification.md.
 */

#pragma option -zCT2CASE_TEXT

#include <fcntl.h>
#include <io.h>
#include <string.h>
#include <sys/stat.h>
#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "th02/t2case.hpp"
#include "th01/rank.h"
#include "th02/resident.hpp"
#include "th02/core/globals.hpp"
#include "th02/math/randring.hpp"
// th02/hardware/input.hpp has no include guard and th02/main/demo.h opens with
// it, so it must be reached only that way.
#include "th02/main/demo.h"
#include "th02/main/frames.hpp"
#include "th02/main/main.hpp"
#include "th02/main/playperf.hpp"
#include "th02/main/score.hpp"
#include "th02/main/slowdown.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/bomb.hpp"
#include "th02/main/entity.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/item/item.hpp"
#include "th02/main/pointnum/pointnum.hpp"
#include "th02/main/spark.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/boss/bosses.hpp"
#include "th02/main/boss/b3.hpp"
#include "th02/main/midboss/midboss.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/dialog/dialog.hpp"
#include "th02/main/hud/overlay.hpp"
#include "th02/core/initexit.h"

/// State this module has to declare for itself
/// -------------------------------------------
/// Two kinds, both unavoidable:
///
///  1. Symbols TH02 never gave a header. All are `public` in ASM and are
///     documented at the `.cpp` sites cited beside each one; Borland C++ does
///     not decorate variable names, so a plain `extern` resolves to the
///     underscored ASM symbol.
///  2. Structs TH02 declares inside a `.cpp` rather than a header. They are
///     copied VERBATIM here rather than carved out into shared headers,
///     because carving would change the include graph of files that the match
///     branch owns and would make this mod branch far harder to rebase. The
///     size proofs at the bottom of this block are the drift guard: if the
///     original ever changes shape, the build fails here rather than silently
///     hashing a different byte sequence.

extern int player_patnum;      // th02/main/player/player.cpp:25
extern int8_t playchar_speed_aligned_x;  // th02/main/player/speed[bss].asm
extern int8_t playchar_speed_aligned_y;
extern int8_t playchar_speed_diagonal_x;
extern int8_t playchar_speed_diagonal_y;
extern int bomb_frame;                   // th02/main/player/bomb.cpp:10
extern point_t bomb_circle_center;       // th02/main/player/bomb.cpp:11
extern int bomb_circle_frame;            // th02/main/player/bomb.cpp:12
extern bool16 bomb_circle_done;          // th02/main/player/bomb.cpp:13

extern Subpixel8 rank_base_speed;        // th02/main/bullet/bullet.cpp:78
extern uint8_t rank_base_stack;          // th02/main/bullet/bullet.cpp:84
extern uint8_t bullet_stack;             // th02/main/bullet/bullet.cpp:85
extern int8_t easy_slow_skip_cycle;      // th02/main/bullet/bullet.cpp:86

extern screen_x_t stone_left[STONE_COUNT];  // th02/main/boss/b3.cpp:11
extern screen_y_t stone_top[STONE_COUNT];   // th02/main/boss/b3.cpp:12
extern int16_t midboss3_kill_frame[MIDBOSS3_COUNT]; // th02/main/midboss/m3.cpp:9
extern screen_x_t midboss3_left_on_page[STONE_COUNT][PAGE_COUNT]; // m3.cpp:13
extern screen_y_t midboss3_top_on_page[STONE_COUNT][PAGE_COUNT];  // m3.cpp:14

extern uint8_t item_semirandom_ring_p;   // th02/main/item/item.cpp:80
extern uint8_t item_semirandom_cycle;    // th02/main/item/item.cpp:103
extern uint8_t item_drop_cycle;          // th02/main/item/item.cpp:379
extern uint8_t item_collect_skill;       // th02/main/item/item.cpp:262
extern score_t item_score_this_frame;    // th02/main/item/item.cpp:81

// th02/main/tile/tile.hpp and th02/formats/map.hpp are deliberately NOT
// included: neither has an include guard and both pull in
// th02/formats/tile.hpp, which would then be included twice in this
// translation unit. Only four scalars are needed, so they are declared
// directly. `tile_mode` is `tile_mode_t` (th02/main/tile/tile.hpp:68), a
// one-byte enum; it is read here as the byte it is.
extern int8_t tile_line_at_top;                    // th02/main/tile/tile.cpp:23
extern uint8_t tile_mode;                          // th02/main/tile/tile.hpp:68
extern unsigned int map_full_row_at_top_of_screen; // th02/formats/map.hpp:42
extern int map_length;                             // th02/formats/map.hpp:45

// Player shots, exported from th02_main.asm under zero-byte `label` aliases.
// The extents are the ones shots_update_and_render()'s own loop bounds
// evidence (`cmp [bp+var_2], 26h`, `add si, 10h`).
#define T2CASE_SHOT_FLAG_COUNT 39
#define T2CASE_SHOT_SLOT_COUNT 38
#define T2CASE_SHOT_SLOT_SIZE  16
extern uint8_t t2case_shot_scalar;
extern uint8_t t2case_shot_flags[T2CASE_SHOT_FLAG_COUNT];
extern uint8_t t2case_shot_slots[T2CASE_SHOT_SLOT_COUNT * T2CASE_SHOT_SLOT_SIZE];

// Verbatim from th02/main/bullet/bullet.cpp:33-57.
struct bullet_t {
	int8_t flag; // ACTUAL TYPE: entity_flag_t
	int8_t size_type; // ACTUAL TYPE: bullet_size_type_t
	SPPoint screen_topleft[PAGE_COUNT];
	SPPoint velocity;
	main_patnum_t patnum;
	bullet_group_or_special_motion_t group_or_special_motion;
	unsigned char angle;
	SubpixelLength8 speed;
	union {
		uint8_t special_frame;
		uint8_t turns_done;
		uint8_t v;
	} u1;
	int8_t padding;
};
extern bullet_t bullets[BULLET_COUNT];

// Verbatim from th02/main/item/item.cpp:40-52.
struct item_pos_t {
	screen_x_t screen_left;
	Subpixel screen_top;
};
struct item_t {
	entity_flag_t flag;
	item_type_t type;
	item_pos_t pos[PAGE_COUNT];
	Subpixel velocity_y;
	pixel_t velocity_x_during_bounce;
	int age;
};
extern item_t items[ITEM_COUNT];

// Verbatim from th02/main/pointnum/pointnum.cpp:17-31, with `vc_t col` and the
// two `pointnum_cel_t` members read as the single bytes they are, so that this
// module does not have to pull in th02/v_colors.hpp and
// th02/sprites/pointnum.h purely to name two enums it only hashes.
#define T2CASE_POINTNUM_COUNT 20
struct CPointnums {
	uint8_t col;
	int8_t unused;
	screen_x_t left[T2CASE_POINTNUM_COUNT];
	screen_y_t top[T2CASE_POINTNUM_COUNT][PAGE_COUNT];
	uint16_t points[T2CASE_POINTNUM_COUNT];
	entity_flag_t flag[T2CASE_POINTNUM_COUNT];
	uint8_t age[T2CASE_POINTNUM_COUNT];
	uint8_t op;
	uint8_t operand;
};
extern CPointnums pointnums;

// Drift guards for the three copies above.
typedef char t2case_bullet_size_check[(sizeof(bullet_t) == 20) ? 1 : -1];
typedef char t2case_item_pos_size_check[(sizeof(item_pos_t) == 4) ? 1 : -1];
typedef char t2case_item_size_check[(sizeof(item_t) == 16) ? 1 : -1];
typedef char t2case_pointnums_size_check[(sizeof(CPointnums) == 204) ? 1 : -1];

/// Cross-process carrier
/// ---------------------
/// Written at every trace row and at every process exit; read at session
/// start. 28 bytes, magic-stamped so a stale file from an unrelated run cannot
/// silently resume a case.

struct t2case_state_t {
	char magic[4]; // "T2ST"
	uint8_t mode;
	uint8_t started;
	uint8_t process_seq;
	uint8_t reserved;
	uint32_t sample_count;
	uint32_t record_count;
	uint32_t global_frame;
	uint32_t payload_checksum;
	uint32_t split_rows;
};

enum t2case_mode_t {
	T2CASE_DISABLED = 0,
	T2CASE_RECORD   = 1,
	T2CASE_PLAYBACK = 2,
	T2CASE_ERROR    = 3
};

enum t2case_text_id_t {
	T2T_OK_RECORD = 0,
	T2T_OK_PLAYBACK,
	T2T_OK_INPUT_END,
	T2T_ERR_CASE_HEADER,
	T2T_ERR_CASE_CREATE,
	T2T_ERR_FRAME_IO,
	T2T_ERR_DESYNC,
	T2T_ERR_SPLIT_OPEN,
	T2T_ERR_VERIFY,
	T2T_ERR_STATE
};

/// State (all BSS)
/// ---------------

static char T2CASE_CFG_FN[11];
static char T2CASE_BIN_FN[11];
static char T2CASE_SPLIT_FN[12];
static char T2CASE_DONE_FN[11];
static char T2CASE_DIAG_FN[11];
static char T2CASE_STATE_FN[10];

static bool t2case_paths_ready;

static t2case_header_t t2case_header;
static t2case_startup_t t2case_startup;

static uint8_t t2case_mode;
static bool t2case_started;
static bool t2case_done_written;
static bool t2case_resumed;
static bool t2case_process_ending;
static bool t2case_process_terminal;
static bool t2case_process_closed;
static uint8_t t2case_process_seq;
static uint32_t t2case_sample_count;
static uint32_t t2case_record_count;
static uint32_t t2case_global_frame;
static uint32_t t2case_payload_checksum;
static uint32_t t2case_split_rows;

// Recording bounds and scenario, parsed out of T2CASE.CFG. Playback ignores
// all of them and follows the case's own control records.
static uint32_t t2case_frames_per_process;
static uint32_t t2case_process_limit;
static uint32_t t2case_frames_this_process;
static uint8_t t2case_cfg_demo_num;
static bool t2case_cfg_live;

/// Small helpers
/// -------------

static void t2case_memclear(void far *buf, unsigned size)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(buf);

	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

// Assembled at runtime; see the module comment on initialized data.
static void t2case_paths_init(void)
{
	if(t2case_paths_ready) {
		return;
	}
	T2CASE_CFG_FN[0] = 'T';
	T2CASE_CFG_FN[1] = '2';
	T2CASE_CFG_FN[2] = 'C';
	T2CASE_CFG_FN[3] = 'A';
	T2CASE_CFG_FN[4] = 'S';
	T2CASE_CFG_FN[5] = 'E';
	T2CASE_CFG_FN[6] = '.';
	T2CASE_CFG_FN[7] = 'C';
	T2CASE_CFG_FN[8] = 'F';
	T2CASE_CFG_FN[9] = 'G';
	T2CASE_CFG_FN[10] = '\0';

	T2CASE_BIN_FN[0] = 'T';
	T2CASE_BIN_FN[1] = '2';
	T2CASE_BIN_FN[2] = 'C';
	T2CASE_BIN_FN[3] = 'A';
	T2CASE_BIN_FN[4] = 'S';
	T2CASE_BIN_FN[5] = 'E';
	T2CASE_BIN_FN[6] = '.';
	T2CASE_BIN_FN[7] = 'B';
	T2CASE_BIN_FN[8] = 'I';
	T2CASE_BIN_FN[9] = 'N';
	T2CASE_BIN_FN[10] = '\0';

	T2CASE_SPLIT_FN[0] = 'T';
	T2CASE_SPLIT_FN[1] = '2';
	T2CASE_SPLIT_FN[2] = 'S';
	T2CASE_SPLIT_FN[3] = 'P';
	T2CASE_SPLIT_FN[4] = 'L';
	T2CASE_SPLIT_FN[5] = 'I';
	T2CASE_SPLIT_FN[6] = 'T';
	T2CASE_SPLIT_FN[7] = '.';
	T2CASE_SPLIT_FN[8] = 'B';
	T2CASE_SPLIT_FN[9] = 'I';
	T2CASE_SPLIT_FN[10] = 'N';
	T2CASE_SPLIT_FN[11] = '\0';

	T2CASE_DONE_FN[0] = 'T';
	T2CASE_DONE_FN[1] = '2';
	T2CASE_DONE_FN[2] = 'D';
	T2CASE_DONE_FN[3] = 'O';
	T2CASE_DONE_FN[4] = 'N';
	T2CASE_DONE_FN[5] = 'E';
	T2CASE_DONE_FN[6] = '.';
	T2CASE_DONE_FN[7] = 'T';
	T2CASE_DONE_FN[8] = 'X';
	T2CASE_DONE_FN[9] = 'T';
	T2CASE_DONE_FN[10] = '\0';

	T2CASE_DIAG_FN[0] = 'T';
	T2CASE_DIAG_FN[1] = '2';
	T2CASE_DIAG_FN[2] = 'D';
	T2CASE_DIAG_FN[3] = 'I';
	T2CASE_DIAG_FN[4] = 'A';
	T2CASE_DIAG_FN[5] = 'G';
	T2CASE_DIAG_FN[6] = '.';
	T2CASE_DIAG_FN[7] = 'T';
	T2CASE_DIAG_FN[8] = 'X';
	T2CASE_DIAG_FN[9] = 'T';
	T2CASE_DIAG_FN[10] = '\0';

	T2CASE_STATE_FN[0] = 'T';
	T2CASE_STATE_FN[1] = '2';
	T2CASE_STATE_FN[2] = 'C';
	T2CASE_STATE_FN[3] = 'A';
	T2CASE_STATE_FN[4] = 'S';
	T2CASE_STATE_FN[5] = 'E';
	T2CASE_STATE_FN[6] = '.';
	T2CASE_STATE_FN[7] = 'S';
	T2CASE_STATE_FN[8] = 'T';
	T2CASE_STATE_FN[9] = '\0';

	t2case_paths_ready = true;
}

/// File access
/// -----------
/// Turbo C++'s low-level I/O rather than master.lib's `file_*` API, exactly as
/// the TH01 module does it, and for the second of its two reasons: master.lib's
/// `file_*` API has a single global handle that the game's own packfile code
/// shares (game_pfopen(), th02/core/initmain.cpp:16), and demo_load() itself
/// uses it (th02_main.asm:2006-2038). Independent descriptors remove that
/// coupling entirely instead of having to reason about it. `lseek` and `write`
/// are already linked into MAIN.EXE from cl.lib; `open`, `read` and `close`
/// come from the same library on demand.

static int t2f_read_open(const char *fn)
{
	return open(fn, (O_RDONLY | O_BINARY));
}

// Truncating create.
static int t2f_create(const char *fn)
{
	return open(fn, (O_WRONLY | O_CREAT | O_TRUNC | O_BINARY), S_IWRITE);
}

// Non-truncating open-or-create, for in-place header rewrites and appends.
static int t2f_update(const char *fn)
{
	return open(fn, (O_WRONLY | O_CREAT | O_BINARY), S_IWRITE);
}

static bool t2f_write(int fd, const void far *buf, unsigned size)
{
	return (write(fd, const_cast<void far *>(buf), size) == static_cast<int>(size));
}

// Stack objects are SS-relative in this memory model, so every helper a caller
// may hand a local to takes a far pointer.
static uint32_t t2case_fnv1a(uint32_t hash, const void far *buf, unsigned size)
{
	const uint8_t far *p = reinterpret_cast<const uint8_t far *>(buf);

	while(size != 0) {
		hash ^= static_cast<uint32_t>(*p++);
		hash *= T2CASE_FNV1A_PRIME;
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

static uint32_t t2h_a;
static uint32_t t2h_b;
static uint16_t t2h_i;

static void t2h_begin(void)
{
	t2h_a = T2CASE_FNV1A_BASIS;
	t2h_b = T2SPLIT_PASSB_BASIS;
	t2h_i = 0;
}

static void t2h_u8(uint8_t value)
{
	t2h_a = ((t2h_a ^ static_cast<uint32_t>(value)) * T2CASE_FNV1A_PRIME);
	t2h_b = ((t2h_b ^ static_cast<uint32_t>(
		static_cast<uint8_t>(value ^ static_cast<uint8_t>(t2h_i & 0xFF))
	)) * T2CASE_FNV1A_PRIME);
	t2h_i++;
}

static void t2h_u16(uint16_t value)
{
	t2h_u8(static_cast<uint8_t>(value));
	t2h_u8(static_cast<uint8_t>(value >> 8));
}

static void t2h_u32(uint32_t value)
{
	t2h_u16(static_cast<uint16_t>(value));
	t2h_u16(static_cast<uint16_t>(value >> 16));
}

static void t2h_commit(t2split_row_t far *row, int group)
{
	row->hash[(group * 2) + 0] = t2h_b;
	row->hash[(group * 2) + 1] = t2h_a;
}

/// Status and diagnostics
/// ----------------------

// Status text is emitted one character at a time so this module contributes no
// initialized data; see the module comment.
static int t2case_text_fd;

static void t2case_write_char(char c)
{
	char buf[1];

	buf[0] = c;
	t2f_write(t2case_text_fd, buf, 1);
}

static void t2case_write_text(uint8_t id)
{
	t2case_write_char((id <= T2T_OK_INPUT_END) ? 'o' : 'e');
	if(id <= T2T_OK_INPUT_END) {
		t2case_write_char('k');
	} else {
		t2case_write_char('r');
		t2case_write_char('r');
		t2case_write_char('o');
		t2case_write_char('r');
	}
	t2case_write_char(':');
	switch(id) {
	case T2T_OK_RECORD:
		t2case_write_char('r'); t2case_write_char('e'); t2case_write_char('c');
		t2case_write_char('o'); t2case_write_char('r'); t2case_write_char('d');
		break;
	case T2T_OK_PLAYBACK:
		t2case_write_char('p'); t2case_write_char('l'); t2case_write_char('a');
		t2case_write_char('y'); t2case_write_char('b'); t2case_write_char('a');
		t2case_write_char('c'); t2case_write_char('k');
		break;
	case T2T_OK_INPUT_END:
		t2case_write_char('i'); t2case_write_char('n'); t2case_write_char('p');
		t2case_write_char('u'); t2case_write_char('t'); t2case_write_char('-');
		t2case_write_char('e'); t2case_write_char('n'); t2case_write_char('d');
		break;
	case T2T_ERR_CASE_HEADER:
		t2case_write_char('h'); t2case_write_char('e'); t2case_write_char('a');
		t2case_write_char('d'); t2case_write_char('e'); t2case_write_char('r');
		break;
	case T2T_ERR_CASE_CREATE:
		t2case_write_char('c'); t2case_write_char('r'); t2case_write_char('e');
		t2case_write_char('a'); t2case_write_char('t'); t2case_write_char('e');
		break;
	case T2T_ERR_FRAME_IO:
		t2case_write_char('f'); t2case_write_char('r'); t2case_write_char('a');
		t2case_write_char('m'); t2case_write_char('e'); t2case_write_char('-');
		t2case_write_char('i'); t2case_write_char('o');
		break;
	case T2T_ERR_DESYNC:
		t2case_write_char('d'); t2case_write_char('e'); t2case_write_char('s');
		t2case_write_char('y'); t2case_write_char('n'); t2case_write_char('c');
		break;
	case T2T_ERR_SPLIT_OPEN:
		t2case_write_char('s'); t2case_write_char('p'); t2case_write_char('l');
		t2case_write_char('i'); t2case_write_char('t');
		break;
	case T2T_ERR_VERIFY:
		t2case_write_char('v'); t2case_write_char('e'); t2case_write_char('r');
		t2case_write_char('i'); t2case_write_char('f'); t2case_write_char('y');
		break;
	default:
		t2case_write_char('s'); t2case_write_char('t'); t2case_write_char('a');
		t2case_write_char('t'); t2case_write_char('e');
		break;
	}
}

static void t2case_done_write(uint8_t status)
{
	if(t2case_done_written) {
		return;
	}
	t2case_paths_init();
	t2case_text_fd = t2f_create(T2CASE_DONE_FN);
	if(t2case_text_fd >= 0) {
		t2case_write_text(status);
		t2case_write_char('\r');
		t2case_write_char('\n');
		close(t2case_text_fd);
	}
	t2case_done_written = true;
}

static char t2case_hex(uint8_t nibble)
{
	return static_cast<char>(
		(nibble < 10) ? ('0' + nibble) : ('A' + (nibble - 10))
	);
}

// One fixed-width line per milestone, flushed immediately, so a run that dies
// still leaves a usable trace. [tag] is exactly three characters.
static void t2case_diag(char t0, char t1, char t2, uint32_t a, uint32_t b)
{
	char line[24];
	int fd;
	int i;

	t2case_paths_init();
	line[0] = t0;
	line[1] = t1;
	line[2] = t2;
	line[3] = ' ';
	for(i = 0; i < 8; i++) {
		line[4 + i] = t2case_hex(static_cast<uint8_t>((a >> ((7 - i) * 4)) & 0xF));
	}
	line[12] = ' ';
	for(i = 0; i < 8; i++) {
		line[13 + i] = t2case_hex(static_cast<uint8_t>((b >> ((7 - i) * 4)) & 0xF));
	}
	line[21] = '\r';
	line[22] = '\n';
	fd = t2f_update(T2CASE_DIAG_FN);
	if(fd < 0) {
		return;
	}
	lseek(fd, 0L, SEEK_END);
	t2f_write(fd, line, 23);
	close(fd);
}

bool16 t2case_active(void)
{
	return ((t2case_mode == T2CASE_RECORD) || (t2case_mode == T2CASE_PLAYBACK));
}

/// Control surface
/// ---------------
/// T2CASE.CFG, following th04/main/oracle.cpp's convention exactly:
///
///   p                              play back T2CASE.BIN
///   r<demo>                        record ZUN's demo <demo> (1-3)
///   r<demo> <frames> <processes>   ... bounded, for cheap Gate A cases
///   l                              record LIVE input (AUTOTYPE or a human)
///
/// The two optional decimals bound a RECORDING only; playback ignores both and
/// follows the case's own control records. They exist because TH02 has a hard
/// process length — ZUN's demo ends at `demo_frame >= DEMO_N - 50` = 6950
/// frames (th02_main.asm:2060) — and a Gate A case does not need all 6950 to
/// prove a handoff. Making the bound explicit and file-driven is honest;
/// hardcoding a shorter one would silently diverge from ZUN's own condition.
///
/// `l` exists because TH02's oracle wants cases beyond the three stock demos.
/// It is not needed for Gate A and produces `source_kind` 1 rather than 2.

static uint32_t t2case_cfg_number(const char *cfg, int len, int *pos)
{
	uint32_t value = 0;
	bool seen = false;
	int i = *pos;

	while((i < len) && ((cfg[i] == ' ') || (cfg[i] == '\t'))) {
		i++;
	}
	while((i < len) && (cfg[i] >= '0') && (cfg[i] <= '9')) {
		value = ((value * 10) + static_cast<uint32_t>(cfg[i] - '0'));
		seen = true;
		i++;
	}
	*pos = i;
	return (seen ? value : 0);
}

static uint8_t t2case_cfg_mode(void)
{
	char cfg[64];
	int read_len;
	int i;
	int fd;
	char mode = '\0';

	t2case_paths_init();
	t2case_memclear(cfg, sizeof(cfg));
	fd = t2f_read_open(T2CASE_CFG_FN);
	if(fd < 0) {
		return T2CASE_DISABLED;
	}
	read_len = read(fd, cfg, (sizeof(cfg) - 1));
	close(fd);
	if(read_len < 0) {
		return T2CASE_DISABLED;
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
	if(i < read_len) {
		i++;
		if((i < read_len) && (cfg[i] >= '1') && (cfg[i] <= '3')) {
			t2case_cfg_demo_num = static_cast<uint8_t>(cfg[i] - '0');
			i++;
		}
		t2case_frames_per_process = t2case_cfg_number(cfg, read_len, &i);
		t2case_process_limit = t2case_cfg_number(cfg, read_len, &i);
	}
	if(t2case_frames_per_process == 0) {
		// ZUN's own end condition, mirrored: DemoPlay stops the demo at
		// `demo_frame >= DEMO_N - 50` (th02_main.asm:2060).
		t2case_frames_per_process = (DEMO_N - 50);
	}
	if(t2case_process_limit == 0) {
		t2case_process_limit = 1;
	}
	if((mode == 'r') || (mode == 'R')) {
		t2case_cfg_live = false;
		return T2CASE_RECORD;
	}
	if((mode == 'l') || (mode == 'L')) {
		t2case_cfg_live = true;
		return T2CASE_RECORD;
	}
	if((mode == 'p') || (mode == 'P')) {
		return T2CASE_PLAYBACK;
	}
	return T2CASE_DISABLED;
}

// Replicates the scenario pinning that OP's start_demo()
// (th02/op_01.cpp:357-370) performs immediately before it exec's MAIN.EXE.
//
// Needed because the oracle launcher runs MAIN.EXE directly rather than
// through OP, exactly as th04/main/oracle.cpp's oracle_scenario_pin() does and
// for the same reason: a run that reached gameplay through the title screen
// would depend on OP's attract timeout and on whatever the previous session
// left in resident_t. `bgm_mode` is deliberately NOT pinned here — start_demo()
// copies OP's own `snd_bgm_mode`, which a direct MAIN launch never establishes,
// so it is left at the value cfg_load() read and is recorded in the startup
// block like any other input.
static void t2case_scenario_pin(uint8_t demo_num)
{
	resident->rem_lives = 2;
	resident->rem_bombs = 3;
	resident->start_lives = 2;
	resident->start_bombs = 3;
	resident->rank = RANK_NORMAL;
	resident->continues_used = 0;
	resident->unused_3 = 0;
	resident->demo_num = static_cast<char>(demo_num);
	resident->shottype = 0;

	// The cfg_load re-mirror (th02_main.asm:2107-2112): cfg_load ran at :701,
	// before us, and derived these from the pre-pin resident.
	lives = static_cast<int8_t>(resident->start_lives);
	bombs = static_cast<int8_t>(resident->start_bombs);
	rank = resident->rank;
}

/// Carrier
/// -------

static bool t2case_state_load(void)
{
	t2case_state_t state;
	int fd;

	t2case_paths_init();
	fd = t2f_read_open(T2CASE_STATE_FN);
	if(fd < 0) {
		return false;
	}
	if(read(fd, &state, sizeof(state)) != static_cast<int>(sizeof(state))) {
		close(fd);
		return false;
	}
	close(fd);
	if(
		(state.magic[0] != 'T') || (state.magic[1] != '2') ||
		(state.magic[2] != 'S') || (state.magic[3] != 'T')
	) {
		return false;
	}
	if((state.mode != T2CASE_RECORD) && (state.mode != T2CASE_PLAYBACK)) {
		return false;
	}
	t2case_mode = state.mode;
	t2case_started = (state.started != 0);
	t2case_process_seq = state.process_seq;
	t2case_sample_count = state.sample_count;
	t2case_record_count = state.record_count;
	t2case_global_frame = state.global_frame;
	t2case_payload_checksum = state.payload_checksum;
	t2case_split_rows = state.split_rows;
	return true;
}

static void t2case_state_store(void)
{
	t2case_state_t state;
	int fd;

	t2case_paths_init();
	t2case_memclear(&state, sizeof(state));
	state.magic[0] = 'T';
	state.magic[1] = '2';
	state.magic[2] = 'S';
	state.magic[3] = 'T';
	state.mode = t2case_mode;
	state.started = (t2case_started ? 1 : 0);
	state.process_seq = t2case_process_seq;
	state.sample_count = t2case_sample_count;
	state.record_count = t2case_record_count;
	state.global_frame = t2case_global_frame;
	state.payload_checksum = t2case_payload_checksum;
	state.split_rows = t2case_split_rows;

	fd = t2f_create(T2CASE_STATE_FN);
	if(fd < 0) {
		return;
	}
	t2f_write(fd, &state, sizeof(state));
	close(fd);
}

// Ends the case: a later MAIN process must not resume a run that is over.
static void t2case_state_clear(void)
{
	t2case_state_t state;
	int fd;

	t2case_paths_init();
	t2case_memclear(&state, sizeof(state));
	fd = t2f_create(T2CASE_STATE_FN);
	if(fd < 0) {
		return;
	}
	t2f_write(fd, &state, sizeof(state));
	close(fd);
}

/// Case file I/O
/// -------------
/// Every access is open -> seek -> read/write -> close and nothing is ever left
/// open across a game call, so the injector can never hold a descriptor that
/// the game's own packfile or replay I/O would be surprised by.

static void t2case_header_checksum_set(void)
{
	uint32_t hash;

	t2case_header.header_checksum = 0;
	hash = t2case_fnv1a(
		T2CASE_FNV1A_BASIS, &t2case_header, sizeof(t2case_header)
	);
	hash = t2case_fnv1a(hash, &t2case_startup, sizeof(t2case_startup));
	t2case_header.header_checksum = hash;
}

static bool t2case_header_write(bool create)
{
	int fd;

	t2case_paths_init();
	t2case_header.record_count = t2case_record_count;
	t2case_header.sample_count = t2case_sample_count;
	t2case_header.payload_size = (
		t2case_record_count * static_cast<uint32_t>(T2CASE_RECORD_SIZE)
	);
	t2case_header.total_size = (
		t2case_header.payload_offset + t2case_header.payload_size
	);
	t2case_header.payload_checksum = t2case_payload_checksum;
	t2case_header_checksum_set();

	fd = (create ? t2f_create(T2CASE_BIN_FN) : t2f_update(T2CASE_BIN_FN));
	if(fd < 0) {
		return false;
	}
	lseek(fd, 0L, SEEK_SET);
	if(!t2f_write(fd, &t2case_header, sizeof(t2case_header))) {
		close(fd);
		return false;
	}
	if(!t2f_write(fd, &t2case_startup, sizeof(t2case_startup))) {
		close(fd);
		return false;
	}
	close(fd);
	return true;
}

static bool t2case_header_read(void)
{
	uint32_t stored;
	uint32_t computed;
	long physical_size;
	int fd;
	int i;

	t2case_paths_init();
	fd = t2f_read_open(T2CASE_BIN_FN);
	if(fd < 0) {
		return false;
	}
	physical_size = lseek(fd, 0L, SEEK_END);
	lseek(fd, 0L, SEEK_SET);
	if(
		read(fd, &t2case_header, sizeof(t2case_header)) !=
		static_cast<int>(sizeof(t2case_header))
	) {
		close(fd);
		return false;
	}
	if(
		read(fd, &t2case_startup, sizeof(t2case_startup)) !=
		static_cast<int>(sizeof(t2case_startup))
	) {
		close(fd);
		return false;
	}
	close(fd);

	if(
		(t2case_header.magic[0] != 'T') || (t2case_header.magic[1] != '2') ||
		(t2case_header.magic[2] != 'C') || (t2case_header.magic[3] != 'A') ||
		(t2case_header.magic[4] != 'S') || (t2case_header.magic[5] != 'E') ||
		(t2case_header.magic[6] != '1') || (t2case_header.magic[7] != '\0')
	) {
		return false;
	}
	if(
		(t2case_header.version != T2CASE_VERSION) ||
		(t2case_header.header_size != T2CASE_HEADER_SIZE) ||
		(t2case_header.startup_size != T2CASE_STARTUP_SIZE) ||
		(t2case_header.record_size != T2CASE_RECORD_SIZE) ||
		(t2case_header.input_semantics != 1) ||
		(t2case_header.ruleset_id != 1) ||
		((t2case_header.source_kind != T2CASE_SOURCE_DIRECT) &&
			(t2case_header.source_kind != T2CASE_SOURCE_NORMALIZED)) ||
		(t2case_header.first_process != T2CASE_PROCESS_MAIN) ||
		(t2case_header.scenario_id > 3) ||
		(t2case_header.flags & ~static_cast<uint16_t>(T2CASE_FLAG_KNOWN)) ||
		(t2case_header.payload_offset !=
			(T2CASE_HEADER_SIZE + T2CASE_STARTUP_SIZE)) ||
		(t2case_header.payload_size !=
			(t2case_header.record_count *
				static_cast<uint32_t>(T2CASE_RECORD_SIZE))) ||
		(t2case_header.sample_count > t2case_header.record_count) ||
		(t2case_header.total_size !=
			(t2case_header.payload_offset + t2case_header.payload_size))
	) {
		return false;
	}
	// Readers reject trailing bytes and truncation, so that a physically short
	// case is attributable here rather than as a read failure mid-run.
	if(
		(physical_size < 0) ||
		(static_cast<uint32_t>(physical_size) != t2case_header.total_size)
	) {
		return false;
	}
	for(i = 0; i < 16; i++) {
		if(t2case_startup.reserved[i] != 0) {
			return false;
		}
	}
	// An oracle case must be a demo-scenario case: the injection seam is only
	// reached when `resident->demo_num != 0` (th02_main.asm:1649-1651).
	if((t2case_startup.demo_num < 1) || (t2case_startup.demo_num > 3)) {
		return false;
	}
	if(t2case_startup.demo_num != static_cast<int8_t>(t2case_header.scenario_id)) {
		return false;
	}
	// `debug` routes OP into "select" instead of MAIN (th02/op_01.cpp:342-346)
	// and is a live gameplay switch; an oracle case must have it off.
	if(t2case_startup.debug != 0) {
		return false;
	}

	stored = t2case_header.header_checksum;
	t2case_header.header_checksum = 0;
	computed = t2case_fnv1a(
		T2CASE_FNV1A_BASIS, &t2case_header, sizeof(t2case_header)
	);
	computed = t2case_fnv1a(computed, &t2case_startup, sizeof(t2case_startup));
	t2case_header.header_checksum = stored;
	return (stored == computed);
}

static bool t2case_record_append(const t2case_record_t far *rec)
{
	uint32_t offset = (
		t2case_header.payload_offset +
		(t2case_record_count * static_cast<uint32_t>(T2CASE_RECORD_SIZE))
	);
	int fd = t2f_update(T2CASE_BIN_FN);

	if(fd < 0) {
		return false;
	}
	lseek(fd, static_cast<long>(offset), SEEK_SET);
	if(!t2f_write(fd, rec, sizeof(*rec))) {
		close(fd);
		return false;
	}
	close(fd);
	t2case_payload_checksum = t2case_fnv1a(
		t2case_payload_checksum, rec, sizeof(*rec)
	);
	t2case_record_count++;
	return true;
}

// Reads record [index] WITHOUT advancing the cursor or the incremental
// checksum, so that frame I/O can look ahead at a control record it must not
// consume. TH01 had no need for this: its process boundaries were all
// injector-initiated. TH02's can also be game-initiated (game over, or the
// player quitting), and the boundary record must be consumed at the same
// logical point in both modes — which is game_exit(), not the frame hook.
static bool t2case_record_peek(uint32_t index, t2case_record_t far *rec)
{
	uint32_t offset = (
		t2case_header.payload_offset +
		(index * static_cast<uint32_t>(T2CASE_RECORD_SIZE))
	);
	int fd;

	if(index >= t2case_header.record_count) {
		return false;
	}
	fd = t2f_read_open(T2CASE_BIN_FN);
	if(fd < 0) {
		return false;
	}
	lseek(fd, static_cast<long>(offset), SEEK_SET);
	if(read(fd, rec, sizeof(*rec)) != static_cast<int>(sizeof(*rec))) {
		close(fd);
		return false;
	}
	close(fd);
	return true;
}

static bool t2case_record_fetch(uint32_t index, t2case_record_t far *rec)
{
	if(!t2case_record_peek(index, rec)) {
		return false;
	}
	t2case_payload_checksum = t2case_fnv1a(
		t2case_payload_checksum, rec, sizeof(*rec)
	);
	return true;
}

static bool t2case_playback_final(void)
{
	return (
		(t2case_sample_count == t2case_header.sample_count) &&
		(t2case_record_count == t2case_header.record_count) &&
		(t2case_payload_checksum == t2case_header.payload_checksum)
	);
}

/// Split trace
/// -----------

static bool t2case_split_write_header(void)
{
	t2split_header_t header;
	int fd;

	t2case_paths_init();
	t2case_memclear(&header, sizeof(header));
	header.magic[0] = 'T';
	header.magic[1] = '2';
	header.magic[2] = 'S';
	header.magic[3] = 'P';
	header.magic[4] = 'L';
	header.magic[5] = 'T';
	header.magic[6] = '1';
	header.magic[7] = '\0';
	header.version = T2SPLIT_VERSION;
	header.header_size = T2SPLIT_HEADER_SIZE;
	header.row_size = T2SPLIT_ROW_SIZE;
	header.flags = T2SPLIT_VERSION;

	fd = t2f_create(T2CASE_SPLIT_FN);
	if(fd < 0) {
		return false;
	}
	if(!t2f_write(fd, &header, sizeof(header))) {
		close(fd);
		return false;
	}
	close(fd);
	return true;
}

/// Subsystem groups
/// ----------------
/// Numbered exactly as state/re/DETERMINISTIC_STATE_TH02.md §4. Every field is
/// a scalar read through its owning header; nothing here is a struct, a
/// pointer, a segment or a padding byte.

static void t2h_group_rng(void)
{
	int i;

	t2h_begin();
	t2h_u32(static_cast<uint32_t>(random_seed));
	for(i = 0; i < RANDRING_SIZE; i++) {
		t2h_u8(randring[i]);
	}
	// DETERMINISTIC_STATE_TH02.md §2: TH02 declares `randring_p` as `db ?`
	// followed by a separate `db ?` pad (th02/math/randring[bss].asm:10-15).
	// Hash the cursor only, never the pad.
	t2h_u8(randring_p);
}

static void t2h_group_run(void)
{
	t2h_begin();
	t2h_u32(static_cast<uint32_t>(resident->frame));
	t2h_u32(stage_frame);
	t2h_u16(static_cast<uint16_t>(resident->continues_used));
	t2h_u16(static_cast<uint16_t>(resident->skill));
	t2h_u8(static_cast<uint8_t>(resident->stage));
	t2h_u8(static_cast<uint8_t>(resident->rank));
	t2h_u8(static_cast<uint8_t>(resident->rem_lives));
	t2h_u8(static_cast<uint8_t>(resident->rem_bombs));
	t2h_u8(resident->start_lives);
	t2h_u8(resident->start_bombs);
	t2h_u8(static_cast<uint8_t>(resident->start_power));
	t2h_u8(resident->shottype);
	t2h_u8(static_cast<uint8_t>(resident->demo_num));
	t2h_u8(static_cast<uint8_t>(resident->reduce_effects ? 1 : 0));
	// The live file-scope copies MAIN actually reads (cfg_load,
	// th02_main.asm:2105-2118), not the resident originals.
	t2h_u8(static_cast<uint8_t>(stage_id));
	t2h_u8(static_cast<uint8_t>(rank));
	t2h_u8(static_cast<uint8_t>(lives));
	t2h_u8(static_cast<uint8_t>(bombs));
	t2h_u8(static_cast<uint8_t>(stage_progression));
	t2h_u8(static_cast<uint8_t>(quit ? 1 : 0));
}

static void t2h_group_scoring(void)
{
	t2h_begin();
	t2h_u32(static_cast<uint32_t>(score));
	t2h_u32(static_cast<uint32_t>(score_delta));
	t2h_u32(static_cast<uint32_t>(hiscore));
	t2h_u32(static_cast<uint32_t>(resident->score_highest));
	t2h_u16(extends_gained);
	t2h_u16(static_cast<uint16_t>(playperf));
	t2h_u16(static_cast<uint16_t>(item_skill));
	t2h_u16(static_cast<uint16_t>(point_items_collected));
	t2h_u8(playperf_max);
	t2h_u8(total_miss_count);
	t2h_u8(total_bombs_used);
	t2h_u8(stage_miss_count);
	t2h_u8(stage_bombs_used);
	t2h_u8(hiscore_continues);
}

static void t2h_group_pacing(void)
{
	t2h_begin();
	t2h_u8(slowdown_factor);
}

static void t2h_group_player(void)
{
	int i;

	t2h_begin();
	t2h_u16(static_cast<uint16_t>(player_topleft.x));
	t2h_u16(static_cast<uint16_t>(player_topleft.y));
	for(i = 0; i < PAGE_COUNT; i++) {
		t2h_u16(static_cast<uint16_t>(player_left_on_page[i]));
		t2h_u16(static_cast<uint16_t>(player_top_on_page[i]));
		t2h_u16(static_cast<uint16_t>(player_option_left_topleft[i].x));
		t2h_u16(static_cast<uint16_t>(player_option_left_topleft[i].y));
	}
	t2h_u16(static_cast<uint16_t>(player_patnum));
	t2h_u8(static_cast<uint8_t>(player_option_patnum));
	t2h_u8(static_cast<uint8_t>(player_is_hit));
	t2h_u8(player_invincibility_time);
	t2h_u8(static_cast<uint8_t>(player_invincible_via_bomb ? 1 : 0));
	t2h_u8(miss_frame);
	t2h_u8(static_cast<uint8_t>(miss_active ? 1 : 0));
	t2h_u8(power);
	t2h_u16(static_cast<uint16_t>(power_overflow));
	t2h_u8(shot_level);
	t2h_u8(static_cast<uint8_t>(playchar_speed_aligned_x));
	t2h_u8(static_cast<uint8_t>(playchar_speed_aligned_y));
	t2h_u8(static_cast<uint8_t>(playchar_speed_diagonal_x));
	t2h_u8(static_cast<uint8_t>(playchar_speed_diagonal_y));
	t2h_u8(static_cast<uint8_t>(bombing ? 1 : 0));
	t2h_u16(static_cast<uint16_t>(bomb_frame));
	t2h_u16(static_cast<uint16_t>(bomb_circle_center.x));
	t2h_u16(static_cast<uint16_t>(bomb_circle_center.y));
	t2h_u16(static_cast<uint16_t>(bomb_circle_frame));
	t2h_u16(static_cast<uint16_t>(bomb_circle_done));

	// Player shots. TH02 never gave these a symbol: shots_update_and_render()
	// is still ASM and walks two anonymous statics in lockstep
	// (th02_main.asm, `byte_20350` / `byte_20351` / `byte_20378`). This branch
	// exports them under zero-byte `label` aliases rather than leaving them
	// out — TXSPLIT_CONTRACT.md, "What is deliberately not inherited", records
	// that omitting shots and entities is exactly what forced TH03 to grow
	// separate semantic diagnostics later. The extents are the ones the
	// function's own loop bounds evidence: 39 flag bytes and 38 slots of 16.
	t2h_u8(t2case_shot_scalar);
	for(i = 0; i < T2CASE_SHOT_FLAG_COUNT; i++) {
		t2h_u8(t2case_shot_flags[i]);
	}
	for(i = 0; i < (T2CASE_SHOT_SLOT_COUNT * T2CASE_SHOT_SLOT_SIZE); i++) {
		t2h_u8(t2case_shot_slots[i]);
	}
}

static void t2h_group_bullets(void)
{
	const bullet_t *b;
	int i;
	int p;

	t2h_begin();
	for(i = 0; i < BULLET_COUNT; i++) {
		b = &bullets[i];
		t2h_u8(static_cast<uint8_t>(b->flag));
		t2h_u8(static_cast<uint8_t>(b->size_type));
		for(p = 0; p < PAGE_COUNT; p++) {
			t2h_u16(static_cast<uint16_t>(b->screen_topleft[p].x.v));
			t2h_u16(static_cast<uint16_t>(b->screen_topleft[p].y.v));
		}
		t2h_u16(static_cast<uint16_t>(b->velocity.x.v));
		t2h_u16(static_cast<uint16_t>(b->velocity.y.v));
		t2h_u8(static_cast<uint8_t>(b->patnum));
		t2h_u8(static_cast<uint8_t>(b->group_or_special_motion));
		t2h_u8(b->angle);
		t2h_u8(static_cast<uint8_t>(b->speed.v));
		t2h_u8(b->u1.v);
		// b->padding is padding. It is never serialized.
	}
	t2h_u16(static_cast<uint16_t>(bullet_special.u1.drift_angle));
	t2h_u16(static_cast<uint16_t>(bullet_special.u2.homing_frames));
	t2h_u16(static_cast<uint16_t>(bullet_special.u3.turns_max));
	t2h_u8(static_cast<uint8_t>(rank_base_speed.v));
	t2h_u8(rank_base_stack);
	t2h_u8(bullet_stack);
	t2h_u8(static_cast<uint8_t>(easy_slow_skip_cycle));
}

static void t2h_group_enemies(void)
{
	int i;
	int p;

	t2h_begin();
	t2h_u8(static_cast<uint8_t>(midboss_active ? 1 : 0));
	t2h_u16(static_cast<uint16_t>(boss_phase_frame));
	t2h_u16(static_cast<uint16_t>(boss_damage));
	for(i = 0; i < PAGE_COUNT; i++) {
		t2h_u16(static_cast<uint16_t>(boss_left_on_page[i]));
		t2h_u16(static_cast<uint16_t>(boss_top_on_page[i]));
	}
	t2h_u32(sigma_frame);
	for(i = 0; i < STONE_COUNT; i++) {
		t2h_u8(static_cast<uint8_t>(stone_flag[i]));
		t2h_u16(static_cast<uint16_t>(stone_damage[i]));
		t2h_u16(static_cast<uint16_t>(stone_left[i]));
		t2h_u16(static_cast<uint16_t>(stone_top[i]));
		for(p = 0; p < PAGE_COUNT; p++) {
			t2h_u16(static_cast<uint16_t>(midboss3_left_on_page[i][p]));
			t2h_u16(static_cast<uint16_t>(midboss3_top_on_page[i][p]));
		}
	}
	for(i = 0; i < MIDBOSS3_COUNT; i++) {
		t2h_u16(static_cast<uint16_t>(midboss3_kill_frame[i]));
	}
}

static void t2h_group_items(void)
{
	const item_t *it;
	int i;
	int p;

	t2h_begin();
	for(i = 0; i < ITEM_COUNT; i++) {
		it = &items[i];
		t2h_u8(static_cast<uint8_t>(it->flag));
		t2h_u8(static_cast<uint8_t>(it->type));
		for(p = 0; p < PAGE_COUNT; p++) {
			t2h_u16(static_cast<uint16_t>(it->pos[p].screen_left));
			t2h_u16(static_cast<uint16_t>(it->pos[p].screen_top.v));
		}
		t2h_u16(static_cast<uint16_t>(it->velocity_y.v));
		t2h_u16(static_cast<uint16_t>(it->velocity_x_during_bounce));
		t2h_u16(static_cast<uint16_t>(it->age));
	}
	t2h_u16(item_bigpower_override);
	t2h_u8(static_cast<uint8_t>(items_miss_add_gameover ? 1 : 0));
	t2h_u8(item_semirandom_ring_p);
	t2h_u8(item_semirandom_cycle);
	t2h_u8(item_drop_cycle);
	t2h_u8(item_collect_skill);
	t2h_u32(static_cast<uint32_t>(item_score_this_frame));

	t2h_u8(pointnums.col);
	for(i = 0; i < T2CASE_POINTNUM_COUNT; i++) {
		t2h_u16(static_cast<uint16_t>(pointnums.left[i]));
		for(p = 0; p < PAGE_COUNT; p++) {
			t2h_u16(static_cast<uint16_t>(pointnums.top[i][p]));
		}
		t2h_u16(pointnums.points[i]);
		t2h_u8(static_cast<uint8_t>(pointnums.flag[i]));
		t2h_u8(pointnums.age[i]);
	}
	t2h_u8(pointnums.op);
	t2h_u8(pointnums.operand);
}

static void t2h_group_field(void)
{
	t2h_begin();
	t2h_u16(static_cast<uint16_t>(scroll_line));
	t2h_u8(scroll_speed);
	t2h_u8(scroll_cycle);
	t2h_u8(scroll_interval);
	t2h_u8(static_cast<uint8_t>(scroll_done ? 1 : 0));
	t2h_u16(map_full_row_at_top_of_screen);
	t2h_u16(static_cast<uint16_t>(map_length));
	t2h_u8(static_cast<uint8_t>(tile_line_at_top));
	t2h_u8(tile_mode);
	t2h_u8(dialog_box_cur);
	// [tile_ring] (25x24 tile image ids) is deliberately excluded: it is a
	// pure function of [scroll_line] plus the loaded map and is only read by
	// the renderer, so it would add 600 bytes per row for no independent
	// signal. [map_section_tiles] and [map] are constant for the whole stage
	// and belong to a round_start snapshot, not to a per-row hash.
}

static void t2h_group_sparks(void)
{
	const spark_t *s;
	int i;
	int p;

	t2h_begin();
	for(i = 0; i < static_cast<int>(SPARK_COUNT); i++) {
		s = &sparks[i];
		t2h_u8(static_cast<uint8_t>(s->flag));
		t2h_u8(s->age);
		for(p = 0; p < PAGE_COUNT; p++) {
			t2h_u16(static_cast<uint16_t>(s->screen_topleft[p].x.v));
			t2h_u16(static_cast<uint16_t>(s->screen_topleft[p].y.v));
		}
		t2h_u16(static_cast<uint16_t>(s->velocity.x.v));
		t2h_u16(static_cast<uint16_t>(s->velocity.y.v));
		t2h_u8(static_cast<uint8_t>(s->render_as));
		// [angle], [speed_base] and [default_render_as] are seeded from
		// master.lib's IRand in bullets_and_sparks_init()
		// (th02/main/bullet/bullet.cpp:110-120), NOT from [randring], which
		// makes them a direct witness of a [random_seed] divergence. They stay
		// in the hash even though they only affect appearance.
		t2h_u8(s->angle);
		t2h_u8(s->speed_base.v);
		t2h_u8(static_cast<uint8_t>(s->default_render_as));
		// unused_1 and unused_2 are padding. They are never serialized.
	}
	t2h_u16(spark_ring_i);
	t2h_u8(spark_sprite_interval);
	t2h_u8(spark_age_max);
	t2h_u16(static_cast<uint16_t>(spark_accel_x.v));
}

static void t2case_split_row(uint8_t event)
{
	t2split_row_t row;
	int fd;

	if((t2case_mode == T2CASE_DISABLED) || (t2case_mode == T2CASE_ERROR)) {
		return;
	}
	t2case_paths_init();
	fd = t2f_update(T2CASE_SPLIT_FN);
	if(fd < 0) {
		t2case_mode = T2CASE_ERROR;
		t2case_done_write(T2T_ERR_SPLIT_OPEN);
		return;
	}
	lseek(fd, 0L, SEEK_END);
	t2case_memclear(&row, sizeof(row));

	row.event = event;
	row.process = T2CASE_PROCESS_MAIN;
	row.stage_id = static_cast<uint8_t>(stage_id);
	row.rank = static_cast<uint8_t>(rank);
	row.global_frame = t2case_global_frame;
	row.scenario_cursor = static_cast<uint32_t>(
		static_cast<uint16_t>(demo_frame)
	);
	row.input = key_det;
	row.schema = T2SPLIT_VERSION;

	row.score = static_cast<uint32_t>(score);
	row.random_seed = static_cast<uint32_t>(random_seed);
	row.samples_consumed = t2case_sample_count;
	row.stage_frame = stage_frame;
	row.resident_frame = static_cast<uint32_t>(resident->frame);
	row.score_highest = static_cast<int32_t>(resident->score_highest);
	row.continues_used = static_cast<uint16_t>(resident->continues_used);
	row.demo_frame = static_cast<uint16_t>(demo_frame);
	row.playperf = static_cast<int16_t>(playperf);
	row.item_skill = static_cast<int16_t>(item_skill);
	row.randring_p = randring_p;
	row.stage_progression = static_cast<uint8_t>(stage_progression);
	row.lives = lives;
	row.bombs = bombs;
	row.power = power;
	row.playperf_max = playperf_max;
	row.total_miss_count = total_miss_count;
	row.total_bombs_used = total_bombs_used;
	row.stage_miss_count = stage_miss_count;
	row.stage_bombs_used = stage_bombs_used;
	row.slowdown_factor = slowdown_factor;
	row.quit = static_cast<uint8_t>(quit);

	t2h_group_rng();      t2h_commit(&row, T2SPLIT_G_RNG);
	t2h_group_run();      t2h_commit(&row, T2SPLIT_G_RUN);
	t2h_group_player();   t2h_commit(&row, T2SPLIT_G_PLAYER);
	t2h_group_bullets();  t2h_commit(&row, T2SPLIT_G_BULLETS);
	t2h_group_enemies();  t2h_commit(&row, T2SPLIT_G_ENEMIES);
	t2h_group_items();    t2h_commit(&row, T2SPLIT_G_ITEMS);
	t2h_group_scoring();  t2h_commit(&row, T2SPLIT_G_SCORING);
	t2h_group_field();    t2h_commit(&row, T2SPLIT_G_FIELD);
	t2h_group_sparks();   t2h_commit(&row, T2SPLIT_G_SPARKS);
	t2h_group_pacing();   t2h_commit(&row, T2SPLIT_G_PACING);

	if(!t2f_write(fd, &row, sizeof(row))) {
		t2case_mode = T2CASE_ERROR;
	}
	close(fd);
	t2case_split_rows++;
	t2case_state_store();
}

static void t2case_input_error(uint8_t status)
{
	t2case_split_row(T2SPLIT_EVENT_ERROR);
	t2case_mode = T2CASE_ERROR;
	t2case_state_clear();
	t2case_done_write(status);
}

/// Startup block
/// -------------

static void t2case_startup_capture(void)
{
	t2case_memclear(&t2case_startup, sizeof(t2case_startup));
	t2case_startup.resident_frame = static_cast<uint32_t>(resident->frame);
	t2case_startup.score = static_cast<int32_t>(resident->score);
	t2case_startup.score_highest = static_cast<int32_t>(resident->score_highest);
	t2case_startup.continues_used =
		static_cast<uint16_t>(resident->continues_used);
	t2case_startup.skill = static_cast<int16_t>(resident->skill);
	t2case_startup.stage = static_cast<int8_t>(resident->stage);
	t2case_startup.rank = static_cast<int8_t>(resident->rank);
	t2case_startup.rem_lives = static_cast<int8_t>(resident->rem_lives);
	t2case_startup.rem_bombs = static_cast<int8_t>(resident->rem_bombs);
	t2case_startup.start_lives = resident->start_lives;
	t2case_startup.start_bombs = resident->start_bombs;
	t2case_startup.start_power = static_cast<int8_t>(resident->start_power);
	t2case_startup.shottype = resident->shottype;
	t2case_startup.bgm_mode = static_cast<int8_t>(resident->bgm_mode);
	t2case_startup.demo_num = static_cast<int8_t>(resident->demo_num);
	t2case_startup.debug = static_cast<int8_t>(resident->debug);
	t2case_startup.reduce_effects =
		static_cast<int8_t>(resident->reduce_effects ? 1 : 0);
	t2case_startup.op_main_retval = resident->op_main_retval;
	t2case_startup.stage_id = static_cast<int8_t>(stage_id);
	t2case_startup.power = power;
	t2case_startup.playperf = static_cast<int8_t>(playperf);
}

// Pre-init apply. TXCASE_CONTRACT.md places this in OP; TH02 puts it at the end
// of game_init_main() instead, which th02_main.asm:704 reaches before
// `resident->demo_num` is read at :738, before demo_load at :740 and before
// `random_seed = resident->frame` at :747. That is early enough for every
// RNG-visible byte, and it keeps the whole injector inside one binary.
//
// The price is the re-mirror below. `cfg_load` (th02_main.asm:2082-2131) runs
// at :701, BEFORE us, and derives five file-scope copies from resident. Those
// copies would otherwise still describe the pre-apply resident. The five lines
// reproduce th02_main.asm:2105-2118 exactly, including the `if(!power) power++`
// clamp. `_playperf = 0` and `_item_bigpower_override = 0` at :2120-2121 are
// unconditional constants, derived from nothing, and so need no re-mirror.
static void t2case_startup_apply(void)
{
	resident->frame = static_cast<long>(t2case_startup.resident_frame);
	resident->score = static_cast<score_t>(t2case_startup.score);
	resident->score_highest = static_cast<long>(t2case_startup.score_highest);
	resident->continues_used = t2case_startup.continues_used;
	resident->skill = t2case_startup.skill;
	resident->stage = static_cast<unsigned char>(t2case_startup.stage);
	resident->rank = static_cast<char>(t2case_startup.rank);
	resident->rem_lives = static_cast<char>(t2case_startup.rem_lives);
	resident->rem_bombs = static_cast<char>(t2case_startup.rem_bombs);
	resident->start_lives = t2case_startup.start_lives;
	resident->start_bombs = t2case_startup.start_bombs;
	resident->start_power = static_cast<char>(t2case_startup.start_power);
	resident->shottype = t2case_startup.shottype;
	resident->bgm_mode = static_cast<char>(t2case_startup.bgm_mode);
	resident->demo_num = static_cast<char>(t2case_startup.demo_num);
	resident->debug = static_cast<char>(t2case_startup.debug);
	resident->reduce_effects = (t2case_startup.reduce_effects != 0);

	// The cfg_load re-mirror (th02_main.asm:2105-2118).
	stage_id = static_cast<char>(resident->stage);
	lives = static_cast<int8_t>(resident->start_lives);
	bombs = static_cast<int8_t>(resident->start_bombs);
	rank = resident->rank;
	power = static_cast<uint8_t>(resident->start_power);
	if(power == 0) {
		power++;
	}
}

// Post-init VERIFY, not restore (TXCASE_CONTRACT.md, "Post-init verify, not
// post-init restore"). The correct behavior is to compare and fail, never to
// overwrite a value the normal path produced.
//
// [random_seed] is deliberately NOT verified: randring_fill()
// (th02/math/randring_fill.asm:1-17) calls master.lib's IRand 256 times inside
// stage init, so by this boundary the seed has already advanced past
// `resident->frame`. `resident->frame` itself is verified instead — it is the
// seed's source (th02_main.asm:747) and is not incremented until :1787.
static bool t2case_startup_verify(void)
{
	return (
		(resident->frame == static_cast<long>(t2case_startup.resident_frame)) &&
		(resident->score == static_cast<score_t>(t2case_startup.score)) &&
		(resident->score_highest ==
			static_cast<long>(t2case_startup.score_highest)) &&
		(resident->continues_used == t2case_startup.continues_used) &&
		(resident->skill == t2case_startup.skill) &&
		(resident->rank == static_cast<char>(t2case_startup.rank)) &&
		(resident->rem_lives == static_cast<char>(t2case_startup.rem_lives)) &&
		(resident->rem_bombs == static_cast<char>(t2case_startup.rem_bombs)) &&
		(resident->start_lives == t2case_startup.start_lives) &&
		(resident->start_bombs == t2case_startup.start_bombs) &&
		(resident->shottype == t2case_startup.shottype) &&
		(resident->demo_num == static_cast<char>(t2case_startup.demo_num)) &&
		(resident->debug == static_cast<char>(t2case_startup.debug)) &&
		(stage_id == static_cast<char>(t2case_startup.stage_id)) &&
		(power == t2case_startup.power) &&
		(playperf == static_cast<int>(t2case_startup.playperf)) &&
		(rank == static_cast<char>(t2case_startup.rank)) &&
		(lives == static_cast<int8_t>(t2case_startup.start_lives)) &&
		(bombs == static_cast<int8_t>(t2case_startup.start_bombs))
	);
}

/// Process lifecycle
/// -----------------

// Mirrors stock DemoPlay's own end-of-demo behavior (th02_main.asm:2063-2067,
// `loc_C20B`): zero [key_det] and raise [quit]. The palette blackout and
// snd_se_reset() are deliberately NOT reproduced — they are presentation and
// audio, which TXSPLIT_CONTRACT.md §7 keeps out of the schema entirely, and
// reproducing them would make the trace depend on renderer state.
//
// [quit] is tested at th02_main.asm:1816, at the end of the per-frame function,
// which makes main()'s call at :792 return 0 and fall through to
// GameExecl("op") at :827 — so the handoff needs no hook of its own.
static void t2case_process_end_request(bool16 terminal)
{
	t2case_process_ending = true;
	t2case_process_terminal = (terminal != false);
	key_det = INPUT_NONE;
	quit = true;
}

// True once T2DONE.TXT exists, i.e. once some earlier process in this run
// already reported a terminal status.
static bool t2case_done_exists(void)
{
	int fd;

	t2case_paths_init();
	fd = t2f_read_open(T2CASE_DONE_FN);
	if(fd < 0) {
		return false;
	}
	close(fd);
	return true;
}

void t2case_session_start(void)
{
	t2case_paths_init();
	t2case_payload_checksum = T2CASE_FNV1A_BASIS;

	// FAIL-CLOSED, and not optional. When a case ends, the game does not:
	// MAIN hands off to OP, OP's attract timeout fires again, and a fresh MAIN
	// starts. With only T2CASE.CFG to go on, that next MAIN would begin a
	// BRAND NEW recording over the top of the finished case — measured on run
	// `th02-rec02`, whose T2DIAG.TXT shows four SES lines for a two-process
	// case and whose T2CASE.BIN was left mid-second-cycle. The host runner does
	// stop on T2DONE.TXT, but it polls, and a Turbo run outruns the poll.
	//
	// T2DONE.TXT is the run's terminal status by contract (TXCASE_CONTRACT.md,
	// "Control surface"), so its mere existence ends every later process. The
	// runner deletes it before each run, and only a terminal or an error
	// creates it.
	if(t2case_done_exists()) {
		t2case_mode = T2CASE_DISABLED;
		return;
	}

	// The carrier wins; T2CASE.CFG is only the first-process fallback. Without
	// this precedence a resumed MAIN would re-apply the startup block and
	// restart the case from record zero.
	t2case_resumed = t2case_state_load();
	if(!t2case_resumed) {
		t2case_mode = t2case_cfg_mode();
	} else {
		// The bounds still have to be read, because a resumed RECORDING needs
		// them for this process too.
		uint8_t cfg_mode = t2case_cfg_mode();

		if(cfg_mode == T2CASE_DISABLED) {
			// A stale carrier next to a removed T2CASE.CFG: refuse to resume.
			t2case_mode = T2CASE_DISABLED;
			t2case_state_clear();
			return;
		}
	}
	if(t2case_mode == T2CASE_DISABLED) {
		return;
	}
	if(!t2case_resumed) {
		t2case_payload_checksum = T2CASE_FNV1A_BASIS;
	}
	t2case_diag(
		'S', 'E', 'S', t2case_mode, (t2case_resumed ? 1UL : 0UL)
	);

	if(t2case_mode == T2CASE_PLAYBACK) {
		if(!t2case_header_read()) {
			t2case_mode = T2CASE_ERROR;
			t2case_state_clear();
			t2case_done_write(T2T_ERR_CASE_HEADER);
			return;
		}
		if(!t2case_resumed) {
			t2case_startup_apply();
		}
	} else if(!t2case_resumed) {
		if(!t2case_cfg_live) {
			// Pin the scenario the way OP's start_demo() would, and select
			// which of ZUN's three demos this case normalizes. Must happen
			// before th02_main.asm:738 tests `resident->demo_num` and before
			// :740 calls demo_load(); t2case_session_start() runs at :704.
			t2case_scenario_pin(
				(t2case_cfg_demo_num != 0) ? t2case_cfg_demo_num : 1
			);
		} else if(resident->demo_num == 0) {
			// A live recording still needs the injection seam, which
			// th02_main.asm:1649-1651 only reaches when demo_num is nonzero.
			t2case_scenario_pin(1);
		}
		t2case_memclear(&t2case_header, sizeof(t2case_header));
		t2case_header.magic[0] = 'T';
		t2case_header.magic[1] = '2';
		t2case_header.magic[2] = 'C';
		t2case_header.magic[3] = 'A';
		t2case_header.magic[4] = 'S';
		t2case_header.magic[5] = 'E';
		t2case_header.magic[6] = '1';
		t2case_header.magic[7] = '\0';
		t2case_header.version = T2CASE_VERSION;
		t2case_header.header_size = T2CASE_HEADER_SIZE;
		t2case_header.startup_size = T2CASE_STARTUP_SIZE;
		t2case_header.record_size = T2CASE_RECORD_SIZE;
		t2case_header.payload_offset = (T2CASE_HEADER_SIZE + T2CASE_STARTUP_SIZE);
		t2case_header.source_kind = static_cast<uint8_t>(
			t2case_cfg_live ? T2CASE_SOURCE_DIRECT : T2CASE_SOURCE_NORMALIZED
		);
		t2case_header.input_semantics = 1;
		t2case_header.ruleset_id = 1;
		t2case_header.scenario_id = static_cast<uint8_t>(resident->demo_num);
		t2case_header.first_process = T2CASE_PROCESS_MAIN;
		t2case_header.producer = T2CASE_PRODUCER_GAME_MOD;
	} else {
		// Resuming a recording in a later MAIN process: keep appending to the
		// case the first process created.
		if(!t2case_header_read()) {
			t2case_mode = T2CASE_ERROR;
			t2case_state_clear();
			t2case_done_write(T2T_ERR_CASE_HEADER);
			return;
		}
	}
	if(t2case_mode == T2CASE_RECORD) {
		if(!t2case_header_write(!t2case_resumed)) {
			t2case_mode = T2CASE_ERROR;
			t2case_state_clear();
			t2case_done_write(T2T_ERR_CASE_CREATE);
			return;
		}
	}
	if(!t2case_resumed) {
		if(!t2case_split_write_header()) {
			t2case_mode = T2CASE_ERROR;
			t2case_done_write(T2T_ERR_SPLIT_OPEN);
			return;
		}
	}
	t2case_process_seq++;
	t2case_frames_this_process = 0;
	t2case_state_store();
	t2case_diag('H', 'D', 'R', t2case_header.record_count, t2case_global_frame);
}

void t2case_stage_enter(void)
{
	t2case_record_t rec;

	if((t2case_mode == T2CASE_DISABLED) || (t2case_mode == T2CASE_ERROR)) {
		overlay_stage_enter_animate();
		return;
	}

	if(!t2case_started) {
		// THE post-init boundary, and the reason the startup block is captured
		// HERE rather than in t2case_session_start(). demo_load()
		// (th02_main.asm:1991-2041) runs at :740, between session start and
		// this point, and pins `resident->frame`, `resident->shottype`,
		// [stage_id], [power] and [playperf]. Capturing before it would store
		// pre-pin values that the post-init verify could never match; capturing
		// here stores exactly the state the case must reproduce, and playback
		// applies it pre-init at :704 where demo_load then re-derives the same
		// values. Nothing is ever restored post-init — only compared.
		if(t2case_mode == T2CASE_PLAYBACK) {
			if(!t2case_startup_verify()) {
				t2case_input_error(T2T_ERR_VERIFY);
				overlay_stage_enter_animate();
				return;
			}
		} else {
			t2case_startup_capture();
			t2case_header.scenario_id = static_cast<uint8_t>(
				resident->demo_num
			);
			if(!t2case_header_write(false)) {
				t2case_input_error(T2T_ERR_CASE_CREATE);
				overlay_stage_enter_animate();
				return;
			}
		}
		t2case_started = true;
	}

	// A STAGE control record at every round start. It is the one place a
	// record and a playback can prove they are still on the same stage of the
	// same process before a single input has been consumed.
	t2case_memclear(&rec, sizeof(rec));
	rec.kind = T2CASE_RECORD_CONTROL;
	rec.phase = T2CASE_PHASE_CONTROL;
	rec.scenario_cursor = 0xFFFF;
	rec.frame_index = t2case_global_frame;
	rec.control = T2CASE_CONTROL_STAGE;

	if(t2case_mode == T2CASE_RECORD) {
		if(!t2case_record_append(&rec)) {
			t2case_input_error(T2T_ERR_FRAME_IO);
			overlay_stage_enter_animate();
			return;
		}
		if(!t2case_header_write(false)) {
			t2case_input_error(T2T_ERR_FRAME_IO);
			overlay_stage_enter_animate();
			return;
		}
	} else {
		t2case_record_t got;

		if(!t2case_record_fetch(t2case_record_count, &got)) {
			t2case_input_error(T2T_ERR_FRAME_IO);
			overlay_stage_enter_animate();
			return;
		}
		t2case_record_count++;
		if(
			(got.kind != T2CASE_RECORD_CONTROL) ||
			(got.phase != T2CASE_PHASE_CONTROL) ||
			(got.scenario_cursor != 0xFFFF) ||
			(got.frame_index != t2case_global_frame) ||
			(got.control != T2CASE_CONTROL_STAGE)
		) {
			t2case_input_error(T2T_ERR_DESYNC);
			overlay_stage_enter_animate();
			return;
		}
	}

	t2case_split_row(
		(t2case_split_rows == 0) ?
		T2SPLIT_EVENT_START : T2SPLIT_EVENT_ROUND_START
	);
	overlay_stage_enter_animate();
}

void t2case_frame_io(void)
{
	t2case_record_t rec;

	if((t2case_mode == T2CASE_DISABLED) || (t2case_mode == T2CASE_ERROR)) {
		// Never touch [key_det] in a stock run. Stock DemoPlay's body is
		// still assembled at th02_main.asm:2048-2073 and still `public`; it is
		// simply no longer called, because :1651 now names this function. A
		// stock run therefore plays the demo with no injection at all, which
		// is the correct behaviour for a build with no T2CASE.CFG present.
		return;
	}
	if(t2case_process_ending) {
		key_det = INPUT_NONE;
		return;
	}

	if(t2case_mode == T2CASE_RECORD) {
		if(
			(t2case_frames_this_process >= t2case_frames_per_process) ||
			(demo_frame >= (DEMO_N - 50))
		) {
			t2case_process_end_request(
				t2case_process_seq >= t2case_process_limit
			);
			return;
		}
		// The whole point of source_kind 2: ZUN's own demo IS the corpus.
		// This is stock DemoPlay's read (th02_main.asm:2053-2058) performed
		// verbatim, so the recording drives the game through exactly the code
		// ZUN's demo would, and the case that comes out is a normalization of
		// DEMO<n>.REC rather than anything we invented. In `l` mode the value
		// input_reset_sense() just read is kept instead, which is how a case
		// beyond the three stock demos gets recorded.
		if(!t2case_cfg_live) {
			key_det = DemoBuf[demo_frame];
		}
		t2case_memclear(&rec, sizeof(rec));
		rec.kind = T2CASE_RECORD_INPUT;
		rec.phase = T2CASE_PHASE_GAMEPLAY;
		rec.scenario_cursor = static_cast<uint16_t>(demo_frame);
		rec.frame_index = t2case_global_frame;
		rec.key_det = key_det;
		if(!t2case_record_append(&rec)) {
			t2case_input_error(T2T_ERR_FRAME_IO);
			return;
		}
		t2case_sample_count++;
	} else {
		if(t2case_record_count >= t2case_header.record_count) {
			t2case_split_row(T2SPLIT_EVENT_INPUT_END);
			t2case_mode = T2CASE_DISABLED;
			t2case_state_clear();
			t2case_done_write(T2T_OK_INPUT_END);
			key_det = INPUT_NONE;
			quit = true;
			return;
		}
		if(!t2case_record_peek(t2case_record_count, &rec)) {
			t2case_input_error(T2T_ERR_FRAME_IO);
			return;
		}
		if(rec.kind == T2CASE_RECORD_CONTROL) {
			// A process boundary. Do NOT consume it here: it is consumed in
			// game_exit(), the one point both an injector-initiated and a
			// game-initiated handoff pass through.
			if(
				(rec.control != T2CASE_CONTROL_PROCESS_END) &&
				(rec.control != T2CASE_CONTROL_TERMINAL)
			) {
				t2case_input_error(T2T_ERR_DESYNC);
				return;
			}
			t2case_process_end_request(
				rec.control == T2CASE_CONTROL_TERMINAL
			);
			return;
		}
		// Consume for real, which folds the record into the incremental
		// payload checksum.
		if(!t2case_record_fetch(t2case_record_count, &rec)) {
			t2case_input_error(T2T_ERR_FRAME_IO);
			return;
		}
		t2case_record_count++;
		if(
			(rec.phase != T2CASE_PHASE_GAMEPLAY) ||
			(rec.frame_index != t2case_global_frame) ||
			(rec.scenario_cursor != static_cast<uint16_t>(demo_frame)) ||
			(rec.control != 0)
		) {
			t2case_input_error(T2T_ERR_DESYNC);
			return;
		}
		key_det = rec.key_det;
		t2case_sample_count++;
	}

	// Exactly where stock DemoPlay advances it (th02_main.asm:2059), so
	// [demo_frame] keeps the meaning the rest of the game gives it.
	demo_frame++;
	t2case_global_frame++;
	t2case_frames_this_process++;
	if((t2case_global_frame & (T2SPLIT_INTERVAL_SAMPLES - 1)) == 0) {
		t2case_split_row(T2SPLIT_EVENT_CHECKPOINT);
		if((t2case_mode == T2CASE_RECORD) && !t2case_header_write(false)) {
			t2case_input_error(T2T_ERR_FRAME_IO);
			return;
		}
	}
}

void t2case_process_exit(void)
{
	t2case_record_t rec;
	bool16 terminal;
	bool final_case;

	if((t2case_mode == T2CASE_DISABLED) || (t2case_mode == T2CASE_ERROR)) {
		return;
	}
	if(t2case_process_closed) {
		return;
	}
	t2case_process_closed = true;

	// A handoff the injector did not ask for — game over, or the player
	// quitting — still ends this process, and the case must record it at the
	// frame it actually happened. Recording and playback therefore agree by
	// construction: both write/consume the boundary record here.
	terminal = (
		t2case_process_ending ?
		(t2case_process_terminal ? true : false) :
		((t2case_mode == T2CASE_RECORD) &&
			(t2case_process_seq >= t2case_process_limit))
	);

	t2case_memclear(&rec, sizeof(rec));
	rec.kind = T2CASE_RECORD_CONTROL;
	rec.phase = T2CASE_PHASE_CONTROL;
	rec.scenario_cursor = 0xFFFF;
	rec.frame_index = t2case_global_frame;
	rec.control = static_cast<uint16_t>(
		terminal ? T2CASE_CONTROL_TERMINAL : T2CASE_CONTROL_PROCESS_END
	);

	if(t2case_mode == T2CASE_RECORD) {
		if(!t2case_record_append(&rec)) {
			t2case_input_error(T2T_ERR_FRAME_IO);
			return;
		}
		if(!t2case_header_write(false)) {
			t2case_input_error(T2T_ERR_FRAME_IO);
			return;
		}
	} else {
		t2case_record_t got;

		if(!t2case_record_fetch(t2case_record_count, &got)) {
			t2case_input_error(T2T_ERR_FRAME_IO);
			return;
		}
		t2case_record_count++;
		if(
			(got.kind != T2CASE_RECORD_CONTROL) ||
			(got.phase != T2CASE_PHASE_CONTROL) ||
			(got.scenario_cursor != 0xFFFF) ||
			(got.frame_index != t2case_global_frame) ||
			(got.control != rec.control)
		) {
			t2case_input_error(T2T_ERR_DESYNC);
			return;
		}
		terminal = (got.control == T2CASE_CONTROL_TERMINAL);
	}

	t2case_split_row(T2SPLIT_EVENT_FINISH);
	t2case_diag('F', 'I', 'N', t2case_record_count, t2case_global_frame);

	final_case = (terminal != false);
	if(final_case) {
		if((t2case_mode == T2CASE_PLAYBACK) && !t2case_playback_final()) {
			t2case_input_error(T2T_ERR_DESYNC);
			return;
		}
		t2case_state_clear();
		t2case_done_write(
			(t2case_mode == T2CASE_RECORD) ? T2T_OK_RECORD : T2T_OK_PLAYBACK
		);
		t2case_mode = T2CASE_DISABLED;
		return;
	}
	t2case_state_store();
}

/// Lifecycle wrappers
/// ------------------
/// Named by th02_main.asm:704 and :2368 in place of the stock symbols. See
/// th02/t2case.hpp for why the seams are here and not inside the shared
/// translation units that define game_init_main() and game_exit().

int t2case_init_main(void)
{
	int ret = game_init_main();

	if(ret == 0) {
		t2case_session_start();
	}
	return ret;
}

void t2case_game_exit(void)
{
	t2case_process_exit();
	game_exit();
}
