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
 *  2. The injection seam is `key_sense()`, redirected at its six call sites,
 *     not an `fp_*` callback slot. TH01 has no such slot, and overwriting the
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
#include <string.h>
#include <sys/stat.h>
#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "th01/t1case.hpp"
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

#define T1CASE_RES_ID "T1CaseState"

struct t1case_res_t {
	char id[sizeof(T1CASE_RES_ID)];
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

enum t1case_mode_t {
	T1CASE_DISABLED = 0,
	T1CASE_RECORD   = 1,
	T1CASE_PLAYBACK = 2,
	T1CASE_ERROR    = 3
};

enum t1case_text_id_t {
	T1T_OK_RECORD = 0,
	T1T_OK_PLAYBACK,
	T1T_OK_INPUT_END,
	T1T_ERR_CASE_HEADER,
	T1T_ERR_CASE_CREATE,
	T1T_ERR_FRAME_IO,
	T1T_ERR_DESYNC,
	T1T_ERR_SPLIT_OPEN,
	T1T_ERR_VERIFY,
	T1T_ERR_RESIDENT
};

/// State (all BSS)
/// ---------------

static char T1CASE_CFG_FN[11];
static char T1CASE_BIN_FN[11];
static char T1CASE_SPLIT_FN[12];
static char T1CASE_DONE_FN[11];
static char T1CASE_DIAG_FN[11];

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
static bool t1case_done_written;
static uint32_t t1case_sample_count;
static uint32_t t1case_record_count;
static uint32_t t1case_global_frame;
static uint32_t t1case_payload_checksum;
static uint32_t t1case_split_rows;

// The seven group bytes latched for the current input_sense() pass, and a
// pointer to input_sense()'s function-local [input_prev].
static uint8_t t1case_keys[T1CASE_GROUP_COUNT];
static uint8_t near *t1case_input_prev;

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

/// Control surface
/// ---------------

static uint8_t t1case_cfg_mode(void)
{
	char cfg[64];
	int read_len;
	int i;
	int fd;
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
	if((mode == 'r') || (mode == 'R')) {
		return T1CASE_RECORD;
	}
	if((mode == 'p') || (mode == 'P')) {
		return T1CASE_PLAYBACK;
	}
	return T1CASE_DISABLED;
}

/// Carrier
/// -------

static bool t1case_res_open(bool create)
{
	t1case_res = ResData<t1case_res_t>::exist(T1CASE_RES_ID_BUF);
	if(t1case_res) {
		return true;
	}
	if(!create) {
		return false;
	}
	t1case_res = ResData<t1case_res_t>::create(T1CASE_RES_ID_BUF);
	if(!t1case_res) {
		return false;
	}
	t1case_res->mode = T1CASE_DISABLED;
	t1case_res->started = 0;
	t1case_res->process_seq = 0;
	t1case_res->reserved = 0;
	t1case_res->sample_count = 0;
	t1case_res->record_count = 0;
	t1case_res->global_frame = 0;
	t1case_res->payload_checksum = T1CASE_FNV1A_BASIS;
	t1case_res->split_rows = 0;
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
		t1case_memclear(resident, sizeof(resident_t));
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
	t1case_payload_checksum = t1case_res->payload_checksum;
	t1case_split_rows = t1case_res->split_rows;
	t1case_started = (t1case_res->started != 0);
}

static void t1case_handoff_store(void)
{
	if(!t1case_res) {
		return;
	}
	t1case_res->mode = t1case_mode;
	t1case_res->started = (t1case_started ? 1 : 0);
	t1case_res->sample_count = t1case_sample_count;
	t1case_res->record_count = t1case_record_count;
	t1case_res->global_frame = t1case_global_frame;
	t1case_res->payload_checksum = t1case_payload_checksum;
	t1case_res->split_rows = t1case_split_rows;
}

// Ends the case: a later REIIDEN process must not resume a run that is over.
static void t1case_handoff_clear(void)
{
	if(t1case_res) {
		t1case_res->mode = T1CASE_DISABLED;
		t1case_res->id[0] = '\0';
	}
}

/// Case file I/O
/// -------------
/// master.lib has a single global file handle, so every access is
/// open -> seek -> read/write -> close and nothing is ever left open across a
/// game call. All of these routines are already linked into REIIDEN.EXE by
/// th01_reiiden.asm, so no master.lib include had to be added to an original
/// segment contribution.

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

static bool t1case_header_write(bool create)
{
	int fd;

	t1case_paths_init();
	t1case_header.record_count = t1case_record_count;
	t1case_header.sample_count = t1case_sample_count;
	t1case_header.payload_size = (
		t1case_record_count * static_cast<uint32_t>(T1CASE_RECORD_SIZE)
	);
	t1case_header.total_size = (
		t1case_header.payload_offset + t1case_header.payload_size
	);
	t1case_header.payload_checksum = t1case_payload_checksum;
	t1case_header_checksum_set();

	fd = (create ? t1f_create(T1CASE_BIN_FN) : t1f_update(T1CASE_BIN_FN));
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
		(t1case_header.record_size != T1CASE_RECORD_SIZE) ||
		(t1case_header.input_semantics != 1) ||
		(t1case_header.ruleset_id != 1) ||
		(t1case_header.source_kind != T1CASE_SOURCE_DIRECT) ||
		(t1case_header.first_process != T1CASE_PROCESS_REIIDEN) ||
		(t1case_header.flags & ~static_cast<uint16_t>(T1CASE_FLAG_KNOWN)) ||
		(t1case_header.payload_offset !=
			(T1CASE_HEADER_SIZE + T1CASE_STARTUP_SIZE)) ||
		(t1case_header.payload_size !=
			(t1case_header.record_count *
				static_cast<uint32_t>(T1CASE_RECORD_SIZE))) ||
		(t1case_header.sample_count > t1case_header.record_count) ||
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

static bool t1case_record_append(const t1case_record_t far *rec)
{
	uint32_t offset = (
		t1case_header.payload_offset +
		(t1case_record_count * static_cast<uint32_t>(T1CASE_RECORD_SIZE))
	);
	int fd = t1f_update(T1CASE_BIN_FN);

	if(fd < 0) {
		return false;
	}
	lseek(fd, static_cast<long>(offset), SEEK_SET);
	if(!t1f_write(fd, rec, sizeof(*rec))) {
		close(fd);
		return false;
	}
	close(fd);
	t1case_payload_checksum = t1case_fnv1a(
		t1case_payload_checksum, rec, sizeof(*rec)
	);
	t1case_record_count++;
	return true;
}

static bool t1case_record_fetch(uint32_t index, t1case_record_t far *rec)
{
	uint32_t offset = (
		t1case_header.payload_offset +
		(index * static_cast<uint32_t>(T1CASE_RECORD_SIZE))
	);
	int fd;

	if(index >= t1case_header.record_count) {
		return false;
	}
	fd = t1f_read_open(T1CASE_BIN_FN);
	if(fd < 0) {
		return false;
	}
	lseek(fd, static_cast<long>(offset), SEEK_SET);
	if(read(fd, rec, sizeof(*rec)) != static_cast<int>(sizeof(*rec))) {
		close(fd);
		return false;
	}
	close(fd);
	t1case_payload_checksum = t1case_fnv1a(
		t1case_payload_checksum, rec, sizeof(*rec)
	);
	return true;
}

static bool t1case_playback_final(void)
{
	return (
		(t1case_sample_count == t1case_header.sample_count) &&
		(t1case_record_count == t1case_header.record_count) &&
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

static void t1case_startup_capture(void)
{
	int i;

	t1case_memclear(&t1case_startup, sizeof(t1case_startup));
	t1case_startup.resident_rand = resident->rand;
	t1case_startup.score = resident->score;
	t1case_startup.continues_total = resident->continues_total;
	t1case_startup.hiscore = resident->hiscore;
	t1case_startup.score_highest = resident->score_highest;
	for(i = 0; i < (STAGES_PER_SCENE - 1); i++) {
		t1case_startup.bonus_per_stage[i] = resident->bonus_per_stage[i];
	}
	for(i = 0; i < SCENE_COUNT; i++) {
		t1case_startup.continues_per_scene[i] = resident->continues_per_scene[i];
	}
	t1case_startup.stage_id = resident->stage_id;
	t1case_startup.point_value = resident->point_value;
	t1case_startup.pellet_speed = static_cast<int16_t>(resident->pellet_speed);
	t1case_startup.rank = resident->rank;
	t1case_startup.bgm_mode = static_cast<int8_t>(resident->bgm_mode);
	t1case_startup.rem_bombs = resident->rem_bombs;
	t1case_startup.credit_lives_extra = resident->credit_lives_extra;
	t1case_startup.rem_lives = resident->rem_lives;
	t1case_startup.route = resident->route;
	t1case_startup.end_flag = static_cast<int8_t>(resident->end_flag);
	t1case_startup.debug_mode = resident->debug_mode;
	t1case_startup.snd_need_init = resident->snd_need_init;
	t1case_startup.mode_test = 0;
	t1case_startup.start_binary = T1CASE_PROCESS_REIIDEN;
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
	t1case_record_t rec;
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

		t1case_memclear(&rec, sizeof(rec));
		rec.kind = T1CASE_RECORD_INPUT;
		rec.phase = phase;
		rec.scenario_cursor = static_cast<uint16_t>(frame_rand);
		rec.frame_index = t1case_global_frame;
		memcpy(rec.keys, t1case_keys, T1CASE_GROUP_COUNT);
		if(!t1case_record_append(&rec)) {
			t1case_input_error(T1T_ERR_FRAME_IO);
			return;
		}
		t1case_sample_count++;
	} else {
		if(t1case_record_count >= t1case_header.record_count) {
			t1case_split_row(T1SPLIT_EVENT_INPUT_END);
			t1case_mode = T1CASE_DISABLED;
			t1case_handoff_clear();
			t1case_done_write(T1T_OK_INPUT_END);
			return;
		}
		if(!t1case_record_fetch(t1case_record_count, &rec)) {
			t1case_input_error(T1T_ERR_FRAME_IO);
			return;
		}
		t1case_record_count++;
		if(
			(rec.kind != T1CASE_RECORD_INPUT) ||
			(rec.phase != phase) ||
			(rec.frame_index != t1case_global_frame) ||
			(rec.reserved != 0)
		) {
			t1case_input_error(T1T_ERR_DESYNC);
			return;
		}
		memcpy(t1case_keys, rec.keys, T1CASE_GROUP_COUNT);
		t1case_sample_count++;
	}

	t1case_global_frame++;
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

	t1case_paths_init();
	t1case_payload_checksum = T1CASE_FNV1A_BASIS;

	// The resident handoff wins; T1CASE.CFG is only the first-process
	// fallback. Without this precedence a self-restarted REIIDEN would
	// re-apply the startup block and restart the case from record zero.
	if(t1case_res_open(false)) {
		t1case_mode = t1case_res->mode;
		if(
			(t1case_res->id[0] != 'T') ||
			((t1case_mode != T1CASE_RECORD) && (t1case_mode != T1CASE_PLAYBACK))
		) {
			t1case_mode = T1CASE_DISABLED;
		}
	} else {
		t1case_mode = T1CASE_DISABLED;
	}
	resumed = (t1case_mode != T1CASE_DISABLED);
	if(!resumed) {
		t1case_mode = t1case_cfg_mode();
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
	if(resumed) {
		t1case_handoff_load();
	}
	t1case_diag(
		'S', 'E', 'S', t1case_mode, (resumed ? 1UL : 0UL)
	);

	if(t1case_mode == T1CASE_PLAYBACK) {
		if(!t1case_header_read()) {
			t1case_mode = T1CASE_ERROR;
			t1case_handoff_clear();
			t1case_done_write(T1T_ERR_CASE_HEADER);
			return;
		}
		if(!resumed) {
			t1case_startup_apply();
		}
	} else if(!resumed) {
		t1case_startup_capture();
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
		t1case_header.record_size = T1CASE_RECORD_SIZE;
		t1case_header.payload_offset = (T1CASE_HEADER_SIZE + T1CASE_STARTUP_SIZE);
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
	if(t1case_mode == T1CASE_RECORD) {
		if(!t1case_header_write(!resumed)) {
			t1case_mode = T1CASE_ERROR;
			t1case_handoff_clear();
			t1case_done_write(T1T_ERR_CASE_CREATE);
			return;
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
	if(!t1case_started) {
		if((t1case_mode == T1CASE_PLAYBACK) && !t1case_startup_verify()) {
			t1case_input_error(T1T_ERR_VERIFY);
			return;
		}
		t1case_started = true;
		t1case_split_row(T1SPLIT_EVENT_START);
	} else {
		t1case_split_row(T1SPLIT_EVENT_ROUND_START);
	}
	t1case_handoff_store();
}

void far t1case_finish(bool16 terminal)
{
	t1case_record_t rec;
	bool final_case;

	if((t1case_mode == T1CASE_DISABLED) || (t1case_mode == T1CASE_ERROR)) {
		return;
	}
	t1case_memclear(&rec, sizeof(rec));
	rec.kind = T1CASE_RECORD_CONTROL;
	rec.phase = T1CASE_PHASE_CONTROL;
	rec.scenario_cursor = 0xFFFF;
	rec.frame_index = t1case_global_frame;
	rec.keys[0] = static_cast<uint8_t>(
		terminal ? T1CASE_CONTROL_TERMINAL : T1CASE_CONTROL_PROCESS_END
	);
	rec.keys[1] = 0;

	if(t1case_mode == T1CASE_RECORD) {
		if(!t1case_record_append(&rec)) {
			t1case_input_error(T1T_ERR_FRAME_IO);
			return;
		}
		if(!t1case_header_write(false)) {
			t1case_input_error(T1T_ERR_FRAME_IO);
			return;
		}
	} else {
		t1case_record_t got;

		if(!t1case_record_fetch(t1case_record_count, &got)) {
			t1case_input_error(T1T_ERR_FRAME_IO);
			return;
		}
		t1case_record_count++;
		if(
			(got.kind != T1CASE_RECORD_CONTROL) ||
			(got.phase != T1CASE_PHASE_CONTROL) ||
			(got.scenario_cursor != 0xFFFF) ||
			(got.frame_index != t1case_global_frame) ||
			(got.keys[0] != rec.keys[0])
		) {
			t1case_input_error(T1T_ERR_DESYNC);
			return;
		}
	}

	t1case_split_row(T1SPLIT_EVENT_FINISH);
	t1case_diag('F', 'I', 'N', t1case_record_count, t1case_global_frame);

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
		t1case_done_write(
			(t1case_mode == T1CASE_RECORD) ? T1T_OK_RECORD : T1T_OK_PLAYBACK
		);
		t1case_mode = T1CASE_DISABLED;
		return;
	}
	t1case_handoff_store();
}
