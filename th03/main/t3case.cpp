#pragma option -zCT3CASE_TEXT

// T3CASE1 verifier for TH03 MAIN.
//
// Validation-only mod code. It is NOT part of any bit-identical match branch
// and the binaries it produces are intentionally nonmatching. Nothing here may
// be described as matching an original binary.
//
// Two modes, selected by the first non-blank character of T3CASE.CFG:
//
//   r  record   capture the audited startup description plus every logical
//               input sample into T3CASE.BIN, and emit T3SPLIT.BIN
//   p  playback apply the recorded startup, inject the recorded input stream,
//               verify frame alignment, and emit T3SPLIT.BIN
//
// Absent or unrecognized config leaves the game completely untouched.
//
// Playback deliberately re-derives everything it can instead of restoring a
// memory image. Only the audited startup fields are forced; round
// initialization then runs normally, so a startup-logic divergence shows up as
// a state-hash difference rather than being papered over.

#include "libs/master.lib/master.hpp"
#include "platform.h"
#include "th02/math/randring.hpp"
#include "th02/score.h"
#include "th03/main/defeat.hpp"
#include "th03/main/difficul.hpp"
#include "th03/main/player/chain.hpp"
#include "th03/main/player/combo.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/main/round.hpp"
#include "th03/main/t3case.hpp"
#include "th03/resident.hpp"
#include "th03/t3case.hpp"
#include "th03/t3case_build.hpp"

extern "C" unsigned char score[];
extern "C" unsigned char byte_220FC[PLAYER_COUNT];
extern "C" signed char byte_20E48;
extern "C" bool boss_panic_fired_in_current_combo[PLAYER_COUNT];
extern uint8_t randring_p;
extern uint8_t formation_p[PLAYER_COUNT];
extern uint8_t __seg *formation_type_ring;
extern uint8_t __seg *formation_pos_type_ring;
extern farfunc_t_near farfp_20F24;
extern nearfunc_t_near fp_1FBC0;

extern "C" void pascal far sub_D1E7(void);
extern "C" void pascal far sub_D3F9(void);
extern "C" void pascal near sub_B4A8(void);
extern "C" void pascal near sub_B60A(void);

// The round loop's own exit flag. Logged so a run that produces no frames can
// be told apart from a run that never reached the loop.
extern uint8_t byte_23B00;

// None of these are initialized data. A `_DATA` contribution from this module
// would be placed between the original `_DATA` and `_BSS` inside DGROUP and
// would shift every original BSS offset that TH03's ASM addresses by raw value
// (_players, _resident, _pid_current, ...), corrupting gameplay state. The
// Replay Patch assembles its filenames and status text the same way, for the
// same reason. See kb/conventions/th03-mod-layout-verification.md.
static char T3CASE_CFG_FN[11];
static char T3CASE_BIN_FN[11];
static char T3CASE_SPLIT_FN[12];
static char T3CASE_DONE_FN[11];
static char T3CASE_DIAG_FN[11];
static bool t3case_paths_ready;

enum t3case_text_id_t {
	T3T_OK_RECORD = 0,
	T3T_OK_PLAYBACK,
	T3T_OK_INPUT_END,
	T3T_ERR_CASE_HEADER,
	T3T_ERR_CASE_CREATE,
	T3T_ERR_CASE_FINALIZE,
	T3T_ERR_FRAME_IO,
	T3T_ERR_DESYNC,
	T3T_ERR_SPLIT_OPEN
};

enum t3case_mode_t {
	T3CASE_DISABLED = 0,
	T3CASE_RECORD   = 1,
	T3CASE_PLAYBACK = 2,
	T3CASE_ERROR    = 3,
};

// Mirrors the Replay Patch's background/result callback normalization so that
// the curated hash stays comparable across branches. The callback identity is
// retained by ZUN's native round-retry path but is not reachable through any
// portable field, so it is folded in as a small normalized phase number.
enum t3case_background_phase_t {
	T3CASE_BACKGROUND_INITIAL       = 0,
	T3CASE_BACKGROUND_TYPE_A_STEADY = 1,
	T3CASE_BACKGROUND_TYPE_B_STEADY = 2,
};

enum t3case_result_phase_t {
	T3CASE_RESULT_IDLE    = 0,
	T3CASE_RESULT_OPENING = 1,
	T3CASE_RESULT_CLOSING = 2,
};

static t3case_header_t t3case_header;
static t3case_startup_t t3case_startup;
static t3case_snapshot_t t3case_snapshot;
static t3case_mode_t t3case_mode;
static uint32_t t3case_global_frame;
static uint32_t t3case_sample_count;
static uint32_t t3case_record_count;
static uint32_t t3case_payload_checksum;
static uint8_t t3case_last_route;
static bool t3case_started;
static bool t3case_done_written;

// Stack objects are SS-relative in this memory model, so every helper that a
// caller may hand a local to takes a far pointer.
static void t3case_memclear(void far *buf, unsigned size)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(buf);

	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

static void t3case_paths_init(void)
{
	if(t3case_paths_ready) {
		return;
	}
	T3CASE_CFG_FN[0] = 'T'; T3CASE_CFG_FN[1] = '3'; T3CASE_CFG_FN[2] = 'C';
	T3CASE_CFG_FN[3] = 'A'; T3CASE_CFG_FN[4] = 'S'; T3CASE_CFG_FN[5] = 'E';
	T3CASE_CFG_FN[6] = '.'; T3CASE_CFG_FN[7] = 'C'; T3CASE_CFG_FN[8] = 'F';
	T3CASE_CFG_FN[9] = 'G'; T3CASE_CFG_FN[10] = '\0';
	T3CASE_BIN_FN[0] = 'T'; T3CASE_BIN_FN[1] = '3'; T3CASE_BIN_FN[2] = 'C';
	T3CASE_BIN_FN[3] = 'A'; T3CASE_BIN_FN[4] = 'S'; T3CASE_BIN_FN[5] = 'E';
	T3CASE_BIN_FN[6] = '.'; T3CASE_BIN_FN[7] = 'B'; T3CASE_BIN_FN[8] = 'I';
	T3CASE_BIN_FN[9] = 'N'; T3CASE_BIN_FN[10] = '\0';
	T3CASE_SPLIT_FN[0] = 'T'; T3CASE_SPLIT_FN[1] = '3'; T3CASE_SPLIT_FN[2] = 'S';
	T3CASE_SPLIT_FN[3] = 'P'; T3CASE_SPLIT_FN[4] = 'L'; T3CASE_SPLIT_FN[5] = 'I';
	T3CASE_SPLIT_FN[6] = 'T'; T3CASE_SPLIT_FN[7] = '.'; T3CASE_SPLIT_FN[8] = 'B';
	T3CASE_SPLIT_FN[9] = 'I'; T3CASE_SPLIT_FN[10] = 'N'; T3CASE_SPLIT_FN[11] = '\0';
	T3CASE_DONE_FN[0] = 'T'; T3CASE_DONE_FN[1] = '3'; T3CASE_DONE_FN[2] = 'D';
	T3CASE_DONE_FN[3] = 'O'; T3CASE_DONE_FN[4] = 'N'; T3CASE_DONE_FN[5] = 'E';
	T3CASE_DONE_FN[6] = '.'; T3CASE_DONE_FN[7] = 'T'; T3CASE_DONE_FN[8] = 'X';
	T3CASE_DONE_FN[9] = 'T'; T3CASE_DONE_FN[10] = '\0';
	T3CASE_DIAG_FN[0] = 'T'; T3CASE_DIAG_FN[1] = '3'; T3CASE_DIAG_FN[2] = 'D';
	T3CASE_DIAG_FN[3] = 'I'; T3CASE_DIAG_FN[4] = 'A'; T3CASE_DIAG_FN[5] = 'G';
	T3CASE_DIAG_FN[6] = '.'; T3CASE_DIAG_FN[7] = 'T'; T3CASE_DIAG_FN[8] = 'X';
	T3CASE_DIAG_FN[9] = 'T'; T3CASE_DIAG_FN[10] = '\0';
	t3case_paths_ready = true;
}

static uint32_t t3case_fnv1a(
	uint32_t hash, const void far *buf, unsigned size
)
{
	const uint8_t far *p = reinterpret_cast<const uint8_t far *>(buf);

	while(size != 0) {
		hash ^= static_cast<uint32_t>(*p++);
		hash *= T3CASE_FNV1A_PRIME;
		size--;
	}
	return hash;
}

// One fixed-width line per milestone, flushed immediately, so a STOP-error
// crash still leaves a usable trace in the image for extraction with
// `tools/replay/extract_hdi_file.py`. [tag] must be exactly three characters.
static void t3case_diag_hex32(char far *out, uint32_t value)
{
	int i;
	uint8_t nibble;

	for(i = 0; i < 8; i++) {
		nibble = static_cast<uint8_t>(value & 0x0F);
		out[7 - i] = static_cast<char>(
			(nibble < 10) ? ('0' + nibble) : ('A' + (nibble - 10))
		);
		value >>= 4;
	}
}

static void t3case_diag(
	char t0, char t1, char t2, uint32_t a, uint32_t b
)
{
	char line[24];

	t3case_paths_init();
	line[0] = t0;
	line[1] = t1;
	line[2] = t2;
	line[3] = ' ';
	t3case_diag_hex32(&line[4], a);
	line[12] = ' ';
	t3case_diag_hex32(&line[13], b);
	line[21] = '\r';
	line[22] = '\n';
	// master.lib's file_append creates the file when it does not exist.
	if(!file_append(T3CASE_DIAG_FN)) {
		return;
	}
	file_write(line, 23);
	file_close();
}

static void t3case_write_char(char c)
{
	file_write(&c, 1);
}

// One character at a time, so this module contributes no initialized data.
static void t3case_write_text(t3case_text_id_t text)
{
#define W(c) t3case_write_char(c)
	switch(text) {
	case T3T_OK_RECORD:
		W('o'); W('k'); W(':'); W('r'); W('e'); W('c'); W('o'); W('r'); W('d');
		break;
	case T3T_OK_PLAYBACK:
		W('o'); W('k'); W(':'); W('p'); W('l'); W('a'); W('y'); W('b'); W('a');
		W('c'); W('k');
		break;
	case T3T_OK_INPUT_END:
		W('o'); W('k'); W(':'); W('i'); W('n'); W('p'); W('u'); W('t'); W('-');
		W('e'); W('n'); W('d');
		break;
	case T3T_ERR_CASE_HEADER:
		W('e'); W('r'); W('r'); W('o'); W('r'); W(':'); W('c'); W('a'); W('s');
		W('e'); W('-'); W('h'); W('e'); W('a'); W('d'); W('e'); W('r');
		break;
	case T3T_ERR_CASE_CREATE:
		W('e'); W('r'); W('r'); W('o'); W('r'); W(':'); W('c'); W('a'); W('s');
		W('e'); W('-'); W('c'); W('r'); W('e'); W('a'); W('t'); W('e');
		break;
	case T3T_ERR_CASE_FINALIZE:
		W('e'); W('r'); W('r'); W('o'); W('r'); W(':'); W('c'); W('a'); W('s');
		W('e'); W('-'); W('f'); W('i'); W('n'); W('a'); W('l');
		break;
	case T3T_ERR_FRAME_IO:
		W('e'); W('r'); W('r'); W('o'); W('r'); W(':'); W('f'); W('r'); W('a');
		W('m'); W('e'); W('-'); W('i'); W('o');
		break;
	case T3T_ERR_DESYNC:
		W('e'); W('r'); W('r'); W('o'); W('r'); W(':'); W('d'); W('e'); W('s');
		W('y'); W('n'); W('c');
		break;
	case T3T_ERR_SPLIT_OPEN:
		W('e'); W('r'); W('r'); W('o'); W('r'); W(':'); W('s'); W('p'); W('l');
		W('i'); W('t'); W('-'); W('o'); W('p'); W('e'); W('n');
		break;
	}
#undef W
}

static void t3case_done_write(t3case_text_id_t status)
{
	if(t3case_done_written) {
		return;
	}
	t3case_paths_init();
	if(file_create(T3CASE_DONE_FN)) {
		t3case_write_text(status);
		t3case_write_char('\r');
		t3case_write_char('\n');
		file_close();
	}
	t3case_done_written = true;
}

// ---------------------------------------------------------------- curated
// state hash. Field order and helpers are byte-for-byte the same as the
// Replay Patch's `replay_state_hash()`, so a Replays trace and a trace from
// these branches remain directly comparable. Never hash native structs,
// padding, pointers, segment values, heap state, or renderer/audio state.

static uint32_t t3case_hash_u8(uint32_t hash, uint8_t value)
{
	return (((hash << 5) + hash) ^ value);
}

static uint32_t t3case_hash_u16(uint32_t hash, uint16_t value)
{
	hash = t3case_hash_u8(hash, static_cast<uint8_t>(value));
	hash = t3case_hash_u8(hash, static_cast<uint8_t>(value >> 8));
	return hash;
}

static uint32_t t3case_hash_u32(uint32_t hash, uint32_t value)
{
	hash = t3case_hash_u16(hash, static_cast<uint16_t>(value));
	hash = t3case_hash_u16(hash, static_cast<uint16_t>(value >> 16));
	return hash;
}

static uint32_t t3case_hash_score(
	uint32_t hash, const unsigned char near *digits
)
{
	int digit;

	for(digit = 0; digit < SCORE_DIGITS; digit++) {
		hash = t3case_hash_u8(hash, digits[digit]);
	}
	return hash;
}

static uint32_t t3case_hash_bytes(
	uint32_t hash, const void near *buf, unsigned size
)
{
	const uint8_t near *p = reinterpret_cast<const uint8_t near *>(buf);

	while(size != 0) {
		hash = t3case_hash_u8(hash, *p++);
		size--;
	}
	return hash;
}

static uint32_t t3case_hash_player(
	uint32_t hash, const player_stuff_t near *player
)
{
	hash = t3case_hash_u16(hash, player->center.x.v);
	hash = t3case_hash_u16(hash, player->center.y.v);
	hash = t3case_hash_u8(hash, player->halfhearts);
	hash = t3case_hash_u8(hash, player->invincibility_time);
	hash = t3case_hash_u8(hash, player->shot_mode);
	hash = t3case_hash_u8(hash, player->knockback_time);
	hash = t3case_hash_u8(hash, player->move_lock_time);
	hash = t3case_hash_u16(hash, player->gauge_charged);
	hash = t3case_hash_u16(hash, player->gauge_avail);
	hash = t3case_hash_u8(hash, player->bombs);
	hash = t3case_hash_u8(hash, player->rounds_won);
	hash = t3case_hash_u16(hash, player->cpu_frame);
	hash = t3case_hash_u8(hash, player->hit_damage_next);
	hash = t3case_hash_u8(hash, player->shot_active);
	hash = t3case_hash_u8(hash, player->cpu_dodge_strategy);
	hash = t3case_hash_u16(hash, player->human_movement_last);
	hash = t3case_hash_u8(hash, player->spell_ready_frames);
	hash = t3case_hash_u16(hash, player->combo_bonus_max);
	hash = t3case_hash_u8(hash, player->combo_hits_max);
	hash = t3case_hash_u8(hash, player->gauge_attacks_fired);
	hash = t3case_hash_u8(hash, player->boss_attacks_fired);
	hash = t3case_hash_u8(hash, player->boss_attacks_reversed);
	hash = t3case_hash_u8(hash, player->boss_panics_fired);
	return hash;
}

static uint8_t t3case_background_phase(void)
{
	if(farfp_20F24 == sub_D1E7) {
		return T3CASE_BACKGROUND_TYPE_A_STEADY;
	}
	if(farfp_20F24 == sub_D3F9) {
		return T3CASE_BACKGROUND_TYPE_B_STEADY;
	}
	return T3CASE_BACKGROUND_INITIAL;
}

static uint8_t t3case_result_phase(void)
{
	if(fp_1FBC0 == sub_B4A8) {
		return T3CASE_RESULT_OPENING;
	}
	if(fp_1FBC0 == sub_B60A) {
		return T3CASE_RESULT_CLOSING;
	}
	return T3CASE_RESULT_IDLE;
}

static uint32_t t3case_state_hash(void)
{
	uint32_t hash = 5381;

	hash = t3case_hash_u32(hash, round_frame);
	hash = t3case_hash_u16(hash, round_or_result_frame);
	hash = t3case_hash_u8(hash, round_speed);
	hash = t3case_hash_u8(hash, defeat_flag);
	hash = t3case_hash_u8(hash, resident->pid_winner);
	hash = t3case_hash_u16(hash, input_mp_p1);
	hash = t3case_hash_u16(hash, input_mp_p2);
	hash = t3case_hash_u16(hash, input_sp);
	hash = t3case_hash_score(hash, score);
	hash = t3case_hash_score(hash, (score + SCORE_DIGITS));
	hash = t3case_hash_player(hash, &players[0]);
	hash = t3case_hash_player(hash, &players[1]);
	hash = t3case_hash_u8(hash, randring_p);
	hash = t3case_hash_u8(hash, byte_220FC[0]);
	hash = t3case_hash_u8(hash, byte_220FC[1]);
	hash = t3case_hash_u8(hash, byte_20E48);
	hash = t3case_hash_u8(hash, boss_panic_fired_in_current_combo[0]);
	hash = t3case_hash_u8(hash, boss_panic_fired_in_current_combo[1]);
	hash = t3case_hash_u8(hash, t3case_background_phase());
	hash = t3case_hash_u8(hash, t3case_result_phase());
	hash = t3case_hash_bytes(hash, combos, sizeof(combos));
	hash = t3case_hash_bytes(hash, chain_ring_p, sizeof(chain_ring_p));
	hash = t3case_hash_bytes(hash, &chains, sizeof(chains));
	hash = t3case_hash_u32(hash, random_seed);
	return hash;
}

// ------------------------------------------------------------------- split

static void t3case_split_write_header(void)
{
	t3case_split_header_t header;

	t3case_memclear(&header, sizeof(header));
	header.magic[0] = 'T';
	header.magic[1] = '3';
	header.magic[2] = 'S';
	header.magic[3] = 'P';
	header.magic[4] = 'L';
	header.magic[5] = 'T';
	header.magic[6] = '1';
	header.version = T3CASE_SPLIT_VERSION;
	header.header_size = sizeof(header);
	header.row_size = sizeof(t3case_split_row_t);
	if(!file_create(T3CASE_SPLIT_FN)) {
		return;
	}
	file_write(&header, sizeof(header));
	file_close();
}

static void t3case_split_row(uint8_t event, uint8_t route)
{
	t3case_split_row_t row;
	int i;

	if((t3case_mode == T3CASE_DISABLED) || (t3case_mode == T3CASE_ERROR)) {
		return;
	}
	if(!file_append(T3CASE_SPLIT_FN)) {
		t3case_mode = T3CASE_ERROR;
		t3case_done_write(T3T_ERR_SPLIT_OPEN);
		return;
	}
	t3case_memclear(&row, sizeof(row));
	row.event = event;
	row.route = route;
	row.game_mode = resident->game_mode;
	row.story_stage = resident->story_stage;
	row.round_id = round_id;
	row.winner = resident->pid_winner;
	row.round_speed = round_speed;
	row.global_frame = t3case_global_frame;
	row.round_frame = round_frame;
	row.round_or_result_frame = round_or_result_frame;
	for(i = 0; i < T3CASE_SPLIT_PACKED_SCORE_SIZE; i++) {
		row.score_p1[i] = static_cast<uint8_t>(
			(score[(i * 2) + 0] % 10) | ((score[(i * 2) + 1] % 10) << 4)
		);
		row.score_p2[i] = static_cast<uint8_t>(
			(score[SCORE_DIGITS + (i * 2) + 0] % 10) |
			((score[SCORE_DIGITS + (i * 2) + 1] % 10) << 4)
		);
	}
	row.resident_rand = resident->rand;
	row.state_hash = t3case_state_hash();
	if(!file_write(&row, sizeof(row))) {
		t3case_mode = T3CASE_ERROR;
	}
	file_close();
}

// -------------------------------------------------------------- case file

static t3case_mode_t t3case_cfg_mode(void)
{
	char cfg[64];
	unsigned read_len;
	unsigned i;
	char mode = '\0';

	t3case_memclear(cfg, sizeof(cfg));
	if(!file_ropen(T3CASE_CFG_FN)) {
		return T3CASE_DISABLED;
	}
	read_len = file_read(cfg, (sizeof(cfg) - 1));
	file_close();

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
		return T3CASE_RECORD;
	}
	if((mode == 'p') || (mode == 'P')) {
		return T3CASE_PLAYBACK;
	}
	return T3CASE_DISABLED;
}

static void t3case_header_checksum_set(void)
{
	uint32_t hash;

	t3case_header.header_checksum = 0;
	hash = t3case_fnv1a(
		T3CASE_FNV1A_BASIS, &t3case_header, sizeof(t3case_header)
	);
	hash = t3case_fnv1a(hash, &t3case_startup, sizeof(t3case_startup));
	if(t3case_header.flags & T3CASE_FLAG_SNAPSHOT) {
		hash = t3case_fnv1a(hash, &t3case_snapshot, sizeof(t3case_snapshot));
	}
	t3case_header.header_checksum = hash;
}

// Rewrites the 128-byte header/startup prefix. Called once at record start and
// again at every checkpoint, so an interrupted recording still describes the
// samples it actually committed.
static bool t3case_header_write(bool create)
{
	t3case_header.sample_count = t3case_sample_count;
	t3case_header.record_count = t3case_record_count;
	t3case_header.payload_size = (
		t3case_record_count * static_cast<uint32_t>(T3CASE_RECORD_SIZE)
	);
	t3case_header.total_size = t3case_header.payload_offset +
		t3case_header.payload_size;
	t3case_header.payload_checksum = t3case_payload_checksum;
	t3case_header_checksum_set();

	if(create) {
		if(!file_create(T3CASE_BIN_FN)) {
			return false;
		}
	} else {
		if(!file_append(T3CASE_BIN_FN)) {
			return false;
		}
		file_seek(0, SEEK_SET);
	}
	if(!file_write(&t3case_header, sizeof(t3case_header))) {
		file_close();
		return false;
	}
	if(!file_write(&t3case_startup, sizeof(t3case_startup))) {
		file_close();
		return false;
	}
	if(
		(t3case_header.flags & T3CASE_FLAG_SNAPSHOT) &&
		!file_write(&t3case_snapshot, sizeof(t3case_snapshot))
	) {
		file_close();
		return false;
	}
	file_close();
	return true;
}

static bool t3case_header_read(void)
{
	uint32_t stored;
	uint32_t computed;
	unsigned i;

	if(!file_ropen(T3CASE_BIN_FN)) {
		return false;
	}
	if(
		file_read(&t3case_header, sizeof(t3case_header)) !=
		sizeof(t3case_header)
	) {
		file_close();
		return false;
	}
	if(
		file_read(&t3case_startup, sizeof(t3case_startup)) !=
		sizeof(t3case_startup)
	) {
		file_close();
		return false;
	}
	t3case_memclear(&t3case_snapshot, sizeof(t3case_snapshot));
	if(
		(t3case_header.flags & T3CASE_FLAG_SNAPSHOT) &&
		(
			file_read(&t3case_snapshot, sizeof(t3case_snapshot)) !=
			sizeof(t3case_snapshot)
		)
	) {
		file_close();
		return false;
	}
	file_close();

	if(
		(t3case_header.magic[0] != 'T') ||
		(t3case_header.magic[1] != '3') ||
		(t3case_header.magic[2] != 'C') ||
		(t3case_header.magic[3] != 'A') ||
		(t3case_header.magic[4] != 'S') ||
		(t3case_header.magic[5] != 'E') ||
		(t3case_header.magic[6] != '1') ||
		(t3case_header.magic[7] != '\0')
	) {
		return false;
	}
	if(
		(t3case_header.version != T3CASE_VERSION) ||
		(t3case_header.header_size != sizeof(t3case_header)) ||
		(t3case_header.startup_size != sizeof(t3case_startup)) ||
		(t3case_header.record_size != T3CASE_RECORD_SIZE) ||
		(t3case_header.input_semantics != T3CASE_INPUT_SEMANTICS) ||
		(t3case_header.ruleset_id != T3CASE_RULESET_CLASSIC) ||
		(t3case_header.first_process != T3CASE_PROCESS_MAIN)
	) {
		return false;
	}
	if(t3case_header.flags & ~T3CASE_KNOWN_FLAGS) {
		return false;
	}
	if(t3case_header.flags & T3CASE_FLAG_SNAPSHOT) {
		if(
			t3case_header.payload_offset !=
			(static_cast<uint32_t>(T3CASE_PREFIX_SIZE) + T3CASE_SNAPSHOT_SIZE)
		) {
			return false;
		}
	} else if(t3case_header.payload_offset != T3CASE_PREFIX_SIZE) {
		return false;
	}
	if(t3case_header.source_kind == T3CASE_SOURCE_MASTER_DIRECT) {
		if(t3case_header.flags & T3CASE_FLAG_SNAPSHOT) {
			return false;
		}
	} else if(t3case_header.source_kind == T3CASE_SOURCE_NORMALIZED_V11) {
		if(!(t3case_header.flags & T3CASE_FLAG_SNAPSHOT)) {
			return false;
		}
	} else {
		return false;
	}
	if(
		t3case_header.payload_size !=
		(t3case_header.record_count * static_cast<uint32_t>(T3CASE_RECORD_SIZE))
	) {
		return false;
	}
	if(t3case_header.sample_count > t3case_header.record_count) {
		return false;
	}
	if(
		t3case_header.total_size !=
		(t3case_header.payload_offset + t3case_header.payload_size)
	) {
		return false;
	}
	if(
		(t3case_startup.post_init_flags != 0) ||
		(t3case_startup.post_init_randring_p != 0) ||
		(t3case_startup.autofire & 0xFC)
	) {
		return false;
	}
	for(i = 0; i < sizeof(t3case_startup.reserved); i++) {
		if(t3case_startup.reserved[i] != 0) {
			return false;
		}
	}

	stored = t3case_header.header_checksum;
	t3case_header_checksum_set();
	computed = t3case_header.header_checksum;
	t3case_header.header_checksum = stored;
	return (stored == computed);
}

static bool t3case_record_append(const t3case_record_t far *rec)
{
	uint32_t offset = (
		t3case_header.payload_offset +
		(t3case_record_count * static_cast<uint32_t>(T3CASE_RECORD_SIZE))
	);

	if(!file_append(T3CASE_BIN_FN)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	if(!file_write(rec, sizeof(*rec))) {
		file_close();
		return false;
	}
	file_close();
	t3case_payload_checksum = t3case_fnv1a(
		t3case_payload_checksum, rec, sizeof(*rec)
	);
	t3case_record_count++;
	return true;
}

static bool t3case_record_fetch(uint32_t index, t3case_record_t far *rec)
{
	uint32_t offset = (
		t3case_header.payload_offset +
		(index * static_cast<uint32_t>(T3CASE_RECORD_SIZE))
	);

	if(index >= t3case_header.record_count) {
		return false;
	}
	if(!file_ropen(T3CASE_BIN_FN)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	if(file_read(rec, sizeof(*rec)) != sizeof(*rec)) {
		file_close();
		return false;
	}
	file_close();
	t3case_payload_checksum = t3case_fnv1a(
		t3case_payload_checksum, rec, sizeof(*rec)
	);
	return true;
}

static bool t3case_playback_control_consume(uint16_t expected)
{
	t3case_record_t rec;

	if(!t3case_record_fetch(t3case_record_count, &rec)) {
		return false;
	}
	t3case_record_count++;
	return (
		(rec.kind == T3CASE_RECORD_CONTROL) &&
		(rec.phase == T3CASE_PHASE_CONTROL) &&
		(rec.frame_index == t3case_global_frame) &&
		(rec.control == expected)
	);
}

static bool t3case_playback_payload_final(void)
{
	return (
		(t3case_sample_count == t3case_header.sample_count) &&
		(t3case_record_count == t3case_header.record_count) &&
		(t3case_payload_checksum == t3case_header.payload_checksum)
	);
}

// ------------------------------------------------------------------ startup

static void t3case_startup_capture(void)
{
	int i;
	int digit;

	t3case_memclear(&t3case_startup, sizeof(t3case_startup));
	t3case_startup.resident_rand = resident->rand;
	t3case_startup.random_seed = random_seed;
	t3case_startup.game_mode = resident->game_mode;
	t3case_startup.rank = resident->rank;
	t3case_startup.key_mode = resident->key_mode;
	t3case_startup.story_stage = resident->story_stage;
	t3case_startup.story_lives = resident->story_lives;
	t3case_startup.rem_credits = resident->rem_credits;
	t3case_startup.skill = resident->skill;
	t3case_startup.demo_num = resident->demo_num;
	t3case_startup.pid_winner = resident->pid_winner;
	t3case_startup.show_score_menu = (resident->show_score_menu != 0);
	t3case_startup.op_animation_fast = (resident->op_animation_fast != 0);
	for(i = 0; i < T3CASE_PLAYER_COUNT; i++) {
		t3case_startup.is_cpu[i] = (resident->is_cpu[i] != 0);
		t3case_startup.playchar_paletted[i] = (
			resident->playchar_paletted[i].v
		);
		for(digit = 0; digit < T3CASE_SCORE_DIGITS; digit++) {
			t3case_startup.score_last[i][digit] = (
				resident->score_last[i].digits[digit]
			);
		}
	}
	for(i = 0; i < T3CASE_STAGE_COUNT; i++) {
		t3case_startup.story_opponents[i] = resident->story_opponents[i].v;
	}
	// No post-init field is restored yet. Setting a bit here requires
	// documenting the source symbol, why normal initialization cannot recreate
	// the value, and the evidence that it is deterministic.
	t3case_startup.post_init_flags = 0;
}

static void t3case_startup_apply(void)
{
	int i;
	int digit;

	resident->rand = t3case_startup.resident_rand;
	random_seed = t3case_startup.random_seed;
	resident->game_mode = t3case_startup.game_mode;
	resident->rank = t3case_startup.rank;
	resident->key_mode = t3case_startup.key_mode;
	resident->story_stage = t3case_startup.story_stage;
	resident->story_lives = t3case_startup.story_lives;
	resident->rem_credits = t3case_startup.rem_credits;
	resident->skill = t3case_startup.skill;
	resident->demo_num = t3case_startup.demo_num;
	resident->pid_winner = t3case_startup.pid_winner;
	resident->show_score_menu = (t3case_startup.show_score_menu != 0);
	resident->op_animation_fast = (t3case_startup.op_animation_fast != 0);
	for(i = 0; i < T3CASE_PLAYER_COUNT; i++) {
		resident->is_cpu[i] = (t3case_startup.is_cpu[i] != 0);
		resident->playchar_paletted[i].v = (
			t3case_startup.playchar_paletted[i]
		);
		for(digit = 0; digit < T3CASE_SCORE_DIGITS; digit++) {
			resident->score_last[i].digits[digit] = (
				t3case_startup.score_last[i][digit]
			);
		}
	}
	for(i = 0; i < T3CASE_STAGE_COUNT; i++) {
		resident->story_opponents[i].v = t3case_startup.story_opponents[i];
	}
}

static void t3case_snapshot_apply(void)
{
	int i;
	int ring_i;

	if(!(t3case_header.flags & T3CASE_FLAG_SNAPSHOT)) {
		return;
	}
	random_seed = t3case_snapshot.random_seed;
	randring_p = t3case_snapshot.randring_p;
	for(i = 0; i < RANDRING_SIZE; i++) {
		randring[i] = t3case_snapshot.randring[i];
		formation_type_ring[i] = t3case_snapshot.formation_type_ring[i];
		formation_pos_type_ring[i] = t3case_snapshot.formation_pos_type_ring[i];
	}
	for(i = 0; i < PLAYER_COUNT; i++) {
		formation_p[i] = t3case_snapshot.formation_p[i];
		players[i].cpu_charge_at_avail_ring_p =
			t3case_snapshot.cpu_charge_at_avail_ring_p[i];
		for(ring_i = 0; ring_i < CHARGE_AT_AVAIL_RING_SIZE; ring_i++) {
			players[i].cpu_charge_at_avail_ring[ring_i] =
				t3case_snapshot.cpu_charge_at_avail_ring[i][ring_i];
		}
		players[i].center.x.v = t3case_snapshot.player_center_x[i];
		players[i].center.y.v = t3case_snapshot.player_center_y[i];
		players[i].halfhearts = t3case_snapshot.player_halfhearts[i];
		players[i].invincibility_time =
			t3case_snapshot.player_invincibility_time[i];
		players[i].gauge_charge_speed =
			t3case_snapshot.player_gauge_charge_speed[i];
		players[i].gauge_charged = t3case_snapshot.player_gauge_charged[i];
		players[i].gauge_avail = t3case_snapshot.player_gauge_avail[i];
		players[i].bombs = t3case_snapshot.player_bombs[i];
		players[i].shot_active = static_cast<shot_active_t>(
			t3case_snapshot.player_shot_active[i]
		);
		players[i].cpu_frame = t3case_snapshot.player_cpu_frame[i];
	}
}

// ---------------------------------------------------------- resident handoff
// TH03 re-execs MAIN for every round and stage, so the case cursor has to
// survive the process boundary. `resident->unused_3[]` is the same scratch area
// the Replay Patch uses for this (see `th03/replay_handoff.hpp`); always reach
// it through the symbol, because the array starts two bytes later on
// Anniversary, which inserts `pmd_fn` ahead of it.

#define T3CASE_RES_MAGIC_0 'T'
#define T3CASE_RES_MAGIC_1 '3'
#define T3CASE_RES_MAGIC_2 'C'
#define T3CASE_RES_MAGIC_3 '1'
#define T3CASE_RES_MODE_INDEX     4
#define T3CASE_RES_SAMPLES_INDEX  5
#define T3CASE_RES_FRAME_INDEX    9
#define T3CASE_RES_RECORDS_INDEX  13
#define T3CASE_RES_CHECKSUM_INDEX 17
#define T3CASE_RES_STARTED_INDEX  21

static uint32_t t3case_handoff_u32_read(unsigned index)
{
	return (
		static_cast<uint32_t>(static_cast<uint8_t>(resident->unused_3[index])) |
		(static_cast<uint32_t>(
			static_cast<uint8_t>(resident->unused_3[index + 1])
		) << 8) |
		(static_cast<uint32_t>(
			static_cast<uint8_t>(resident->unused_3[index + 2])
		) << 16) |
		(static_cast<uint32_t>(
			static_cast<uint8_t>(resident->unused_3[index + 3])
		) << 24)
	);
}

static void t3case_handoff_u32_write(unsigned index, uint32_t value)
{
	resident->unused_3[index + 0] = static_cast<int8_t>(value);
	resident->unused_3[index + 1] = static_cast<int8_t>(value >> 8);
	resident->unused_3[index + 2] = static_cast<int8_t>(value >> 16);
	resident->unused_3[index + 3] = static_cast<int8_t>(value >> 24);
}

static t3case_mode_t t3case_resident_mode(void)
{
	if(
		(resident->unused_3[0] != T3CASE_RES_MAGIC_0) ||
		(resident->unused_3[1] != T3CASE_RES_MAGIC_1) ||
		(resident->unused_3[2] != T3CASE_RES_MAGIC_2) ||
		(resident->unused_3[3] != T3CASE_RES_MAGIC_3)
	) {
		return T3CASE_DISABLED;
	}
	if(resident->unused_3[T3CASE_RES_MODE_INDEX] == 'r') {
		return T3CASE_RECORD;
	}
	if(resident->unused_3[T3CASE_RES_MODE_INDEX] == 'p') {
		return T3CASE_PLAYBACK;
	}
	return T3CASE_DISABLED;
}

static void t3case_handoff_cursor_load(void)
{
	t3case_sample_count = t3case_handoff_u32_read(T3CASE_RES_SAMPLES_INDEX);
	t3case_global_frame = t3case_handoff_u32_read(T3CASE_RES_FRAME_INDEX);
	t3case_record_count = t3case_handoff_u32_read(T3CASE_RES_RECORDS_INDEX);
	t3case_payload_checksum = t3case_handoff_u32_read(
		T3CASE_RES_CHECKSUM_INDEX
	);
	t3case_started = (resident->unused_3[T3CASE_RES_STARTED_INDEX] != 0);
}

static void t3case_handoff_store(void)
{
	resident->unused_3[0] = T3CASE_RES_MAGIC_0;
	resident->unused_3[1] = T3CASE_RES_MAGIC_1;
	resident->unused_3[2] = T3CASE_RES_MAGIC_2;
	resident->unused_3[3] = T3CASE_RES_MAGIC_3;
	resident->unused_3[T3CASE_RES_MODE_INDEX] = (
		(t3case_mode == T3CASE_RECORD) ? 'r' : 'p'
	);
	t3case_handoff_u32_write(T3CASE_RES_SAMPLES_INDEX, t3case_sample_count);
	t3case_handoff_u32_write(T3CASE_RES_FRAME_INDEX, t3case_global_frame);
	t3case_handoff_u32_write(T3CASE_RES_RECORDS_INDEX, t3case_record_count);
	t3case_handoff_u32_write(
		T3CASE_RES_CHECKSUM_INDEX, t3case_payload_checksum
	);
	resident->unused_3[T3CASE_RES_STARTED_INDEX] = (t3case_started ? 1 : 0);
}

// Ends the case: a later MAIN process must not resume a run that is over.
static void t3case_handoff_clear(void)
{
	resident->unused_3[0] = 0;
	resident->unused_3[T3CASE_RES_MODE_INDEX] = 0;
}

// ------------------------------------------------------------------- hooks

// The one and only session hook, called after `round_startup()` and
// `farfp_20F20()` — byte-for-byte the position and role of the Replay Patch's
// `replay_session_start()`. Everything the verifier does, reads included,
// happens at or after this point. Earlier windows were tried and are unsafe:
// before `game_init_main()` master.lib and the game are not initialized at all,
// and between `cfg_load_resident_ptr()` and `round_startup()` the packfile is
// open with master.lib's single-handle file state shared with `pfint21`.
//
// Capture and apply both happen here, so a recording and its playback see the
// identical point in the process and stay self-consistent.
void far t3case_scenario_apply(void)
{
	t3case_paths_init();

	// A resumed process already had its scenario applied by the process that
	// started the case, and its round is rebuilt from the handoff instead.
	if(t3case_resident_mode() != T3CASE_DISABLED) {
		return;
	}
	if(t3case_cfg_mode() != T3CASE_PLAYBACK) {
		return;
	}

	// Deliberately silent on failure: t3case_session_start() runs a few lines
	// later, reads the same header, and owns all error reporting. Leaving
	// t3case_mode alone here keeps that single point of truth.
	if(!t3case_header_read()) {
		return;
	}
	t3case_startup_apply();
}

void far t3case_session_start(void)
{
	bool resumed;

	t3case_global_frame = 0;
	t3case_sample_count = 0;
	t3case_record_count = 0;
	t3case_payload_checksum = T3CASE_FNV1A_BASIS;
	t3case_last_route = 0;
	t3case_started = false;
	t3case_done_written = false;

	// Resident handoff first, config file only as a fallback — the same
	// precedence `replay_session_start()` uses. TH03 re-execs MAIN per round and
	// stage, so the cursor has to survive the process boundary or a case would
	// silently restart.
	t3case_paths_init();
	t3case_mode = t3case_resident_mode();
	resumed = (t3case_mode != T3CASE_DISABLED);
	if(!resumed) {
		t3case_mode = t3case_cfg_mode();
	}
	if(t3case_mode == T3CASE_DISABLED) {
		return;
	}
	if(resumed) {
		t3case_handoff_cursor_load();
	}
	t3case_diag('S', 'E', 'S', t3case_mode, (resumed ? 1 : 0));

	if(t3case_mode == T3CASE_PLAYBACK) {
		if(!t3case_header_read()) {
			t3case_mode = T3CASE_ERROR;
			t3case_done_write(T3T_ERR_CASE_HEADER);
			return;
		}
		if(!resumed) {
			t3case_startup_apply();
			t3case_snapshot_apply();
		}
		t3case_diag('H', 'D', 'R', t3case_header.sample_count, t3case_sample_count);
	} else if(!resumed) {
		t3case_startup_capture();
		t3case_memclear(&t3case_header, sizeof(t3case_header));
		t3case_header.magic[0] = 'T';
		t3case_header.magic[1] = '3';
		t3case_header.magic[2] = 'C';
		t3case_header.magic[3] = 'A';
		t3case_header.magic[4] = 'S';
		t3case_header.magic[5] = 'E';
		t3case_header.magic[6] = '1';
		t3case_header.magic[7] = '\0';
		t3case_header.version = T3CASE_VERSION;
		t3case_header.header_size = sizeof(t3case_header);
		t3case_header.startup_size = sizeof(t3case_startup);
		t3case_header.record_size = T3CASE_RECORD_SIZE;
		t3case_header.payload_offset = T3CASE_PREFIX_SIZE;
		t3case_header.source_kind = T3CASE_SOURCE_MASTER_DIRECT;
		t3case_header.input_semantics = T3CASE_INPUT_SEMANTICS;
		t3case_header.ruleset_id = T3CASE_RULESET_CLASSIC;
		t3case_header.scenario_id = resident->game_mode;
		t3case_header.first_process = T3CASE_PROCESS_MAIN;
		t3case_header.producer = T3CASE_PRODUCER;
	} else {
		// Resuming a recording in a later MAIN process: keep appending to the
		// case that the first process created.
		if(!t3case_header_read()) {
			t3case_mode = T3CASE_ERROR;
			t3case_done_write(T3T_ERR_CASE_HEADER);
			return;
		}
	}

	if(t3case_mode == T3CASE_RECORD) {
		if(!t3case_header_write(!resumed)) {
			t3case_mode = T3CASE_ERROR;
			t3case_diag('C', 'R', 'E', 0, 0);
			t3case_done_write(T3T_ERR_CASE_CREATE);
			return;
		}
	}
	if(!resumed) {
		t3case_split_write_header();
	}
	t3case_handoff_store();
	t3case_diag('S', 'P', '0', t3case_mode, t3case_record_count);
}

void far t3case_round_start(void)
{
	if((t3case_mode == T3CASE_DISABLED) || (t3case_mode == T3CASE_ERROR)) {
		return;
	}
	if(!t3case_started) {
		t3case_started = true;
		t3case_diag('R', 'S', '0', byte_23B00, t3case_startup.resident_rand);
		t3case_split_row(T3CASE_EVENT_START, t3case_last_route);
		return;
	}
	t3case_diag('R', 'S', 'N', byte_23B00, t3case_global_frame);
	t3case_split_row(T3CASE_EVENT_ROUND_START, t3case_last_route);
}

// Autofire lives in the Replay Patch, not in TH03. A V11 stream stores the
// mapped input *before* the transform, while this hook is exactly where the game
// consumes it *after* — so a normalized case carries the per-player Charge mask in
// [control] and the Autofire mask in the startup block, and the transform is
// finished here. This is the only place it can be finished: the shot gate below is
// TH03's live `byte_220FC`, which no host normalizer can know, and which
// `player_update()` has not yet advanced when this runs.
//
// Mirrors `replay_autofire_apply_player()` in th03/main/replay.cpp on `replays`.
static input_t t3case_autofire_apply(input_t input, uint8_t player, uint8_t charge)
{
	if(t3case_startup.is_cpu[player]) {
		return input; // a CPU side ignores both Autofire and Charge
	}
	if(!(t3case_startup.autofire & (1 << player))) {
		return input;
	}
	if(charge & (1 << player)) {
		return (input | INPUT_SHOT);
	}
	if((input & INPUT_SHOT) && (byte_220FC[player] <= 3)) {
		return (input & ~INPUT_SHOT);
	}
	return input;
}

// Terminating a case must never go through INPUT_CANCEL. rndloop.cpp:126 sends
// that to sub_C7A5(), TH03's pause handler, which returns immediately only when
// BOTH players are CPU; with a human player it spins in `input_wait:` on
// frame_delay(1) waiting for a physical keypress, which nothing injects. Every
// case before the first normalized one was a demo, which is why this never showed.
// replay.cpp:3708 on `replays` does the right thing -- clear input_sp and raise
// byte_23B00, the round-exit flag rndloop.cpp:307 actually loops on -- so do that.
void far t3case_frame_io(void)
{
	t3case_record_t rec;

	if((t3case_mode == T3CASE_DISABLED) || (t3case_mode == T3CASE_ERROR)) {
		return;
	}
	if(t3case_global_frame == 0) {
		t3case_diag('F', 'R', '0', round_frame, t3case_header.sample_count);
	}

	if(t3case_mode == T3CASE_RECORD) {
		rec.kind = T3CASE_RECORD_INPUT;
		rec.phase = T3CASE_PHASE_GAMEPLAY;
		rec.round_or_result_frame = round_or_result_frame;
		rec.frame_index = t3case_global_frame;
		rec.round_frame = round_frame;
		rec.input_mp_p1 = input_mp_p1;
		rec.input_mp_p2 = input_mp_p2;
		rec.input_sp = input_sp;
		rec.control = 0;
		if(!t3case_record_append(&rec)) {
			t3case_split_row(T3CASE_EVENT_ERROR, t3case_last_route);
			t3case_mode = T3CASE_ERROR;
			input_sp = 0;
			byte_23B00 = 1;
			t3case_handoff_clear();
			t3case_done_write(T3T_ERR_FRAME_IO);
			return;
		}
		t3case_sample_count++;
	} else {
		if(t3case_sample_count >= t3case_header.sample_count) {
			if(
				!t3case_playback_control_consume(T3CASE_CONTROL_MAIN_END) ||
				!t3case_playback_payload_final()
			) {
				t3case_split_row(T3CASE_EVENT_ERROR, t3case_last_route);
				t3case_mode = T3CASE_ERROR;
				input_sp = 0;
				byte_23B00 = 1;
				t3case_handoff_clear();
				t3case_done_write(T3T_ERR_FRAME_IO);
				return;
			}
			t3case_split_row(T3CASE_EVENT_INPUT_END, t3case_last_route);
			input_sp = 0;
			byte_23B00 = 1;
			t3case_handoff_clear();
			t3case_done_write(T3T_OK_INPUT_END);
			t3case_mode = T3CASE_DISABLED;
			return;
		}
		if(!t3case_record_fetch(t3case_record_count, &rec)) {
			t3case_split_row(T3CASE_EVENT_ERROR, t3case_last_route);
			t3case_mode = T3CASE_ERROR;
			input_sp = 0;
			byte_23B00 = 1;
			t3case_handoff_clear();
			t3case_done_write(T3T_ERR_FRAME_IO);
			return;
		}
		t3case_record_count++;
		// The verification that makes this a verifier: a recorded position
		// that stops agreeing with the live one means the two branches have
		// already diverged, so refuse to keep injecting into a different run.
		// [frame_index] stays authoritative and dense in every case. The two
		// TH03 counters are only enforced when the case actually recorded them:
		// a normalized case marks them advisory, and enforcing them there would
		// report a spurious desync on the very first frame.
		if(
			(rec.kind != T3CASE_RECORD_INPUT) ||
			(rec.phase != T3CASE_PHASE_GAMEPLAY) ||
			(rec.frame_index != t3case_global_frame) ||
			(
				!(t3case_header.flags & T3CASE_FLAG_ADVISORY_POSITIONS) && (
					(rec.round_frame != round_frame) ||
					(rec.round_or_result_frame != round_or_result_frame)
				)
			)
		) {
			t3case_split_row(T3CASE_EVENT_ERROR, t3case_last_route);
			t3case_mode = T3CASE_ERROR;
			input_sp = 0;
			byte_23B00 = 1;
			t3case_handoff_clear();
			t3case_done_write(T3T_ERR_DESYNC);
			return;
		}
		if(t3case_header.flags & T3CASE_FLAG_CHARGE_IN_CONTROL) {
			input_mp_p1 = t3case_autofire_apply(
				rec.input_mp_p1, 0, (uint8_t)rec.control
			);
			input_mp_p2 = t3case_autofire_apply(
				rec.input_mp_p2, 1, (uint8_t)rec.control
			);
		} else {
			input_mp_p1 = rec.input_mp_p1;
			input_mp_p2 = rec.input_mp_p2;
		}
		input_sp = rec.input_sp;
		t3case_sample_count++;
	}

	t3case_global_frame++;
	if((t3case_global_frame & (T3CASE_SPLIT_INTERVAL_SAMPLES - 1)) == 0) {
		t3case_split_row(T3CASE_EVENT_CHECKPOINT, t3case_last_route);
		if(t3case_mode == T3CASE_RECORD) {
			t3case_header_write(false);
		}
		t3case_handoff_store();
	}
}

void far t3case_route(uint8_t route)
{
	if((t3case_mode == T3CASE_DISABLED) || (t3case_mode == T3CASE_ERROR)) {
		return;
	}
	t3case_last_route = route;
	t3case_diag('R', 'T', 'E', route, t3case_global_frame);
	t3case_split_row(T3CASE_EVENT_ROUTE, route);
}

void far t3case_finish(void)
{
	uint8_t route = t3case_last_route;

	if((t3case_mode == T3CASE_DISABLED) || (t3case_mode == T3CASE_ERROR)) {
		return;
	}
	t3case_diag('F', 'I', 'N', route, t3case_sample_count);
	if(t3case_mode == T3CASE_RECORD) {
		t3case_record_t rec;

		t3case_memclear(&rec, sizeof(rec));
		rec.kind = T3CASE_RECORD_CONTROL;
		rec.phase = T3CASE_PHASE_CONTROL;
		rec.frame_index = t3case_global_frame;
		rec.round_frame = round_frame;
		rec.round_or_result_frame = round_or_result_frame;
		rec.control = T3CASE_CONTROL_MAIN_END;
		t3case_record_append(&rec);
		if(!t3case_header_write(false)) {
			t3case_split_row(T3CASE_EVENT_ERROR, route);
			t3case_mode = T3CASE_ERROR;
			t3case_handoff_clear();
			t3case_done_write(T3T_ERR_CASE_FINALIZE);
			return;
		}
	} else if(
		!t3case_playback_control_consume(T3CASE_CONTROL_MAIN_END) ||
		((route == 0) && !t3case_playback_payload_final())
	) {
		t3case_split_row(T3CASE_EVENT_ERROR, route);
		t3case_mode = T3CASE_ERROR;
		t3case_handoff_clear();
		t3case_done_write(T3T_ERR_FRAME_IO);
		return;
	}
	t3case_split_row(T3CASE_EVENT_FINISH, route);

	// `route == 0` is MAIN's exit to OP, so the run is over. Anything else exits
	// to MAINL and another MAIN process follows, which must resume this cursor
	// rather than start a new case.
	if(route == 0) {
		t3case_handoff_clear();
		t3case_done_write(
			(t3case_mode == T3CASE_RECORD) ? T3T_OK_RECORD : T3T_OK_PLAYBACK
		);
		return;
	}
	t3case_handoff_store();
}
