#pragma option -zCREPLAY_TEXT

// Production user replay stream for TH04 and TH05 MAIN. This module owns all
// format and DOS I/O logic in a new segment; original code only calls the
// narrow hooks declared in replay.hpp.

#include "platform.h"
#include "libs/master.lib/master.hpp"
#include "th01/rank.h"
#include "th02/hardware/frmdelay.h"
#include "th04/common.h"
#include "th04/end/end.h"
#include "th04/main/ems.hpp"
#include "th04/main/frames.h"
#include "th04/main/oracle.hpp"
#include "th04/main/quit.hpp"
#include "th04/main/replay.hpp"
#include "th04/main/replay_checkpoint.hpp"
#include "th04/main/slowdown.hpp"
#include "th04/replay_format.hpp"
#include "th04/replay_targets.hpp"
#include "th04/score.h"
#include "th03/core/initexit.h"
#if (GAME == 5)
	#include "th05/hardware/input.h"
	#include "th05/resident.hpp"
#else
	#include "th04/hardware/input.h"
	#include "th04/resident.hpp"
#endif

#define REPLAY_BUFFER_PACKET_COUNT 256
#define REPLAY_BUFFER_SIZE \
	(REPLAY_BUFFER_PACKET_COUNT * REPLAY_USER_PACKET_SIZE)
#if (GAME == 5)
	#define REPLAY_CHECKPOINT_GROUP_COUNT REPLAY_CKPT_GROUPS_TH05
#else
	#define REPLAY_CHECKPOINT_GROUP_COUNT REPLAY_CKPT_GROUPS_TH04
#endif
#define REPLAY_CHECKPOINT_PREFIX_SIZE ( \
	REPLAY_CHECKPOINT_HEADER_SIZE + \
	(REPLAY_CHECKPOINT_GROUP_COUNT * REPLAY_CHECKPOINT_GROUP_SIZE) \
)

#define REPLAY_FP_SEG(p) ((unsigned)(((unsigned long)(void far *)(p)) >> 16))
#define REPLAY_FP_OFF(p) ((unsigned)((unsigned long)(void far *)(p)))

#define REPLAY_ACCESS_READ 0
#define REPLAY_ACCESS_RW 2

enum replay_runtime_mode_t {
	RRM_DISABLED = 0,
	RRM_RECORD = 1,
	RRM_PLAYBACK = 2,
	RRM_PRACTICE = 3,
};

extern unsigned char stage_id;
extern unsigned char power;
extern unsigned char rank;
extern unsigned char playperf;
extern bool turbo_mode;
extern unsigned char continues_used;
extern unsigned char extends_gained;
extern unsigned int stage_frame;
extern unsigned long score_delta;
extern unsigned long score_delta_frame;
extern unsigned int stage_graze;
extern int power_overflow;
extern unsigned int total_std_frames;
extern unsigned int items_spawned;
extern unsigned int items_collected;
extern unsigned int total_point_items_collected;
// TLINK truncates identifiers past 32 characters. Use the published short
// alias for total_max_valued_point_items_collected (kb/codegen/0060).
extern unsigned int total_max_valued_point_items;
extern unsigned int enemies_gone;
extern unsigned int enemies_killed;
extern bool player_is_hit;
extern char *eyename;
extern bool scroll_active;
extern bool (near* std_update)(void);
bool near std_update_frames_then_animate_dialog_and_activate_boss_if_done(void);
extern "C" void far player_shot_level_update(void);
#if (GAME == 5)
	extern unsigned char lives;
	extern unsigned char bombs;
	extern unsigned char dream;
	extern unsigned int stage_point_items_collected;
	extern unsigned int extend_point_items_collected;
	extern bool debug_mode_active;
	extern unsigned char debug_fast_forward;
#else
	extern unsigned char dream_items_collected;
	extern unsigned int dream_score;
	extern unsigned char stage_point_items_collected;
	extern "C" const unsigned int DREAM_SCORE_PER_ITEMS[];
#endif

static char replay_cfg_fn[11];
static char replay_slot_fn[11];
static bool replay_paths_ready;
static replay_runtime_mode_t replay_mode;
static replay_user_header_t replay_header;
static replay_user_packet_t replay_buffer[REPLAY_BUFFER_PACKET_COUNT];
static uint16_t replay_buffer_len;
static uint16_t replay_buffer_pos;
static uint32_t replay_payload_written;
static uint32_t replay_packet_cursor;
static uint32_t replay_sample_cursor;
static uint32_t replay_payload_checksum;
static replay_user_packet_t replay_pending;
static uint8_t replay_pending_run;
static uint8_t replay_decode_run;
static bool replay_pending_valid;
static bool replay_failed;
static bool replay_finished;
static bool replay_stage_seen;
static bool replay_private_test;
static uint32_t replay_private_diagnostic;
static uint8_t replay_checkpoint_prefix[REPLAY_CHECKPOINT_PREFIX_SIZE];
#define REPLAY_PRIVATE_SAMPLE_LIMIT 600UL
uint8_t replay_ck_failure_group_value;
uint16_t replay_ck_failure_field_value;
static uint8_t replay_last_stage;
static bool replay_practice_start_pending;
static replay_start_config_t replay_practice_start;
static uint8_t replay_preroll_boss_section;
static uint8_t replay_preroll_boss_phase;
static uint8_t replay_preroll_interstitial_cycle;
static bool replay_practice_preroll_pending;
static bool replay_practice_direct_redraw_pending;

#define REPLAY_DOS_RESERVE_PARAS (4096 >> 4)
#if (GAME == 5)
	#define REPLAY_MAIN_HEAP_TARGET_PARAS (291200 >> 4)
#else
	// Correct the original 320,000-byte no-EMS heap landmine.
	#define REPLAY_MAIN_HEAP_TARGET_PARAS (324000 >> 4)
#endif
#define REPLAY_MAIN_HEAP_EMS_MIN_PARAS (245760 >> 4)

static uint16_t replay_dos_largest_free_block(void)
{
	uint16_t largest;

	_asm {
		mov bx, 0FFFFh
		mov ah, 48h
		int 21h
		mov largest, bx
	}
	return largest;
}

static void replay_dos_terminate_failure(void)
{
	_asm {
		mov ax, 4C01h
		int 21h
	}
}

void replay_game_init_main_or_exit(const unsigned char far *pf_fn)
{
	uint16_t largest = replay_dos_largest_free_block();
	uint16_t minimum = (
		(ems_exist() && (ems_space() >= EMSSIZE))
		? REPLAY_MAIN_HEAP_EMS_MIN_PARAS
		: REPLAY_MAIN_HEAP_TARGET_PARAS
	);

	if(largest < static_cast<uint16_t>(minimum + REPLAY_DOS_RESERVE_PARAS)) {
		replay_dos_terminate_failure();
	}
	largest = static_cast<uint16_t>(largest - REPLAY_DOS_RESERVE_PARAS);
	mem_assign_paras = (
		(largest < REPLAY_MAIN_HEAP_TARGET_PARAS)
		? largest
		: REPLAY_MAIN_HEAP_TARGET_PARAS
	);
	if(game_init_main(pf_fn)) {
		replay_dos_terminate_failure();
	}
}

static int replay_dos_open(const char far *fn, unsigned char access)
{
	unsigned fn_seg = REPLAY_FP_SEG(fn);
	unsigned fn_off = REPLAY_FP_OFF(fn);
	int result;

	_asm {
		push	ds
		mov	dx, fn_off
		mov	ds, fn_seg
		mov	ah, 3Dh
		mov	al, access
		int	21h
		pop	ds
		sbb	dx, dx
		or	ax, dx
		mov	result, ax
	}
	return result;
}

static int replay_dos_create(const char far *fn)
{
	unsigned fn_seg = REPLAY_FP_SEG(fn);
	unsigned fn_off = REPLAY_FP_OFF(fn);
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

static void replay_dos_close(int fh)
{
	_asm {
		mov	bx, fh
		mov	ah, 3Eh
		int	21h
	}
}

static unsigned replay_dos_read(int fh, void far *buf, unsigned len)
{
	unsigned buf_seg = REPLAY_FP_SEG(buf);
	unsigned buf_off = REPLAY_FP_OFF(buf);
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
		not	cx
		and	ax, cx
		mov	result, ax
	}
	return result;
}

static unsigned replay_dos_write(int fh, const void far *buf, unsigned len)
{
	unsigned buf_seg = REPLAY_FP_SEG(buf);
	unsigned buf_off = REPLAY_FP_OFF(buf);
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

bool pascal far replay_checkpoint_dos_read(
	uint16_t context, void far *data, uint16_t size
)
{
	return (replay_dos_read(context, data, size) == size);
}

bool pascal far replay_checkpoint_dos_write(
	uint16_t context, void far *data, uint16_t size
)
{
	return (replay_dos_write(context, data, size) == size);
}

static bool replay_dos_seek(int fh, uint32_t pos)
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

static bool replay_dos_size(int fh, uint32_t *size)
{
	unsigned pos_hi;
	unsigned pos_lo;
	unsigned failed;

	_asm {
		mov	bx, fh
		xor	cx, cx
		xor	dx, dx
		mov	ax, 4202h
		int	21h
		mov	pos_lo, ax
		mov	pos_hi, dx
		sbb	ax, ax
		neg	ax
		mov	failed, ax
	}
	*size = (
		(static_cast<uint32_t>(pos_hi) << 16) |
		static_cast<uint32_t>(pos_lo)
	);
	return (failed == 0);
}

static void replay_dos_delete(const char far *fn)
{
	unsigned fn_seg = REPLAY_FP_SEG(fn);
	unsigned fn_off = REPLAY_FP_OFF(fn);

	_asm {
		push	ds
		mov	dx, fn_off
		mov	ds, fn_seg
		mov	ah, 41h
		int	21h
		pop	ds
	}
}

static void replay_memclear(void far *buf, unsigned size)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(buf);

	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

static void replay_copy(
	void far *dst, const void far *src, unsigned size
)
{
	uint8_t far *d = reinterpret_cast<uint8_t far *>(dst);
	const uint8_t far *s = reinterpret_cast<const uint8_t far *>(src);

	while(size != 0) {
		*d++ = *s++;
		size--;
	}
}

static uint32_t replay_fnv1a(
	uint32_t hash, const void far *buf, unsigned size
)
{
	const uint8_t far *p = reinterpret_cast<const uint8_t far *>(buf);

	while(size != 0) {
		hash ^= static_cast<uint32_t>(*p++);
		hash *= REPLAY_FNV1A_PRIME;
		size--;
	}
	return hash;
}

static void replay_paths_init(void)
{
	if(replay_paths_ready) {
		return;
	}
	replay_cfg_fn[0] = 'T'; replay_cfg_fn[1] = ('0' + GAME);
	replay_cfg_fn[2] = 'R'; replay_cfg_fn[3] = 'P'; replay_cfg_fn[4] = 'Y';
	replay_cfg_fn[5] = '.'; replay_cfg_fn[6] = 'C'; replay_cfg_fn[7] = 'F';
	replay_cfg_fn[8] = 'G'; replay_cfg_fn[9] = '\0';

	replay_slot_fn[0] = 'T'; replay_slot_fn[1] = 'H';
	replay_slot_fn[2] = ('0' + GAME); replay_slot_fn[3] = 'R';
	replay_slot_fn[4] = '0'; replay_slot_fn[5] = '0';
	replay_slot_fn[6] = '.'; replay_slot_fn[7] = 'R';
	replay_slot_fn[8] = 'P'; replay_slot_fn[9] = 'Y';
	replay_slot_fn[10] = '\0';
	replay_paths_ready = true;
}

static void replay_slot_set(uint8_t slot)
{
	replay_slot_fn[4] = ('0' + (slot / 10));
	replay_slot_fn[5] = ('0' + (slot % 10));
}

static void replay_checkpoint_fn_set(char far *fn)
{
	fn[0] = 'T'; fn[1] = ('0' + GAME); fn[2] = 'R'; fn[3] = 'C';
	fn[4] = 'K'; fn[5] = '.'; fn[6] = 'T'; fn[7] = 'M'; fn[8] = 'P';
	fn[9] = '\0';
}

static void replay_checkpoint_temp_delete(void)
{
	char fn[10];

	replay_checkpoint_fn_set(fn);
	replay_dos_delete(fn);
}

static bool replay_dos_hash_continue(
	int fh,
	uint32_t offset,
	uint32_t size,
	uint32_t hash,
	uint32_t far *hash_out
)
{
	unsigned len;

	if((hash_out == 0) || !replay_dos_seek(fh, offset)) {
		return false;
	}
	while(size != 0) {
		len = ((size > REPLAY_BUFFER_SIZE)
			? REPLAY_BUFFER_SIZE
			: static_cast<unsigned>(size)
		);
		if(replay_dos_read(fh, replay_buffer, len) != len) {
			return false;
		}
		hash = replay_fnv1a(hash, replay_buffer, len);
		size -= len;
	}
	*hash_out = hash;
	return true;
}

static bool replay_dos_hash(
	int fh, uint32_t offset, uint32_t size, uint32_t far *hash_out
)
{
	return replay_dos_hash_continue(
		fh, offset, size, REPLAY_FNV1A_BASIS, hash_out
	);
}

static bool replay_dos_hash_three_continue(
	int fh,
	uint32_t offset,
	uint32_t size,
	uint32_t far *hash_a,
	uint32_t far *hash_b,
	uint32_t far *hash_c
)
{
	unsigned len;

	if(
		(hash_a == 0) || (hash_b == 0) || (hash_c == 0) ||
		!replay_dos_seek(fh, offset)
	) {
		return false;
	}
	while(size != 0) {
		len = ((size > REPLAY_BUFFER_SIZE)
			? REPLAY_BUFFER_SIZE
			: static_cast<unsigned>(size)
		);
		if(replay_dos_read(fh, replay_buffer, len) != len) {
			return false;
		}
		*hash_a = replay_fnv1a(*hash_a, replay_buffer, len);
		*hash_b = replay_fnv1a(*hash_b, replay_buffer, len);
		*hash_c = replay_fnv1a(*hash_c, replay_buffer, len);
		size -= len;
	}
	return true;
}

struct replay_private_result_t {
	char magic[8];
	uint8_t mode;
	uint8_t ok;
	uint8_t end_reason;
	uint8_t end_sequence;
	uint32_t sample_count;
	uint32_t packet_count;
	uint32_t payload_checksum;
	uint32_t state_digest;
	uint32_t diagnostic;
};

typedef char replay_private_result_size_check[
	(sizeof(replay_private_result_t) == 32) ? 1 : -1
];

static void replay_private_result_write(uint8_t end_reason)
{
	replay_private_result_t result;
	char fn[11];
	int fh;

	if(!replay_private_test) {
		return;
	}
	replay_memclear(&result, sizeof(result));
	result.magic[0] = 'T'; result.magic[1] = ('0' + GAME);
	result.magic[2] = 'R'; result.magic[3] = 'S';
	result.magic[4] = 'L'; result.magic[5] = 'T';
	result.magic[6] = '1'; result.magic[7] = '\0';
	result.mode = replay_mode;
	result.ok = static_cast<uint8_t>(!replay_failed);
	result.end_reason = end_reason;
	result.end_sequence = resident->end_sequence;
	result.sample_count = (
		(replay_mode == RRM_RECORD)
		? replay_header.sample_count
		: replay_sample_cursor
	);
	result.packet_count = (
		(replay_mode == RRM_RECORD)
		? replay_header.packet_count
		: replay_packet_cursor
	);
	result.payload_checksum = replay_payload_checksum;
	result.state_digest = replay_header.state_digest;
	result.diagnostic = replay_private_diagnostic;
	fn[0] = 'T'; fn[1] = ('0' + GAME); fn[2] = 'R'; fn[3] = 'S';
	fn[4] = 'L'; fn[5] = 'T'; fn[6] = '.'; fn[7] = 'B';
	fn[8] = 'I'; fn[9] = 'N'; fn[10] = '\0';
	fh = replay_dos_create(fn);
	if(fh >= 0) {
		replay_dos_write(fh, &result, sizeof(result));
		replay_dos_close(fh);
	}
}

static uint32_t replay_score_points(void)
{
	uint32_t value = 0;
	int i;

	for(i = (SCORE_DIGITS - 1); i >= 1; i--) {
		value = ((value * 10UL) + score.digits[i]);
	}
	return (value * 10UL);
}

static uint8_t replay_lives(void)
{
	#if (GAME == 5)
		return lives;
	#else
		return resident->rem_lives;
	#endif
}

static uint8_t replay_bombs(void)
{
	#if (GAME == 5)
		return bombs;
	#else
		return resident->rem_bombs;
	#endif
}

static uint8_t replay_dream(void)
{
	#if (GAME == 5)
		return dream;
	#else
		return dream_items_collected;
	#endif
}

static void replay_header_checksum_set(void)
{
	replay_header.header_checksum = 0;
	replay_header.header_checksum = replay_fnv1a(
		REPLAY_FNV1A_BASIS, &replay_header, sizeof(replay_header)
	);
}

static bool replay_header_write(bool create)
{
	int fh;

	replay_header.payload_checksum = replay_payload_checksum;
	replay_header_checksum_set();
	fh = (create
		? replay_dos_create(replay_slot_fn)
		: replay_dos_open(replay_slot_fn, REPLAY_ACCESS_RW)
	);
	if(fh < 0) {
		return false;
	}
	if(
		!replay_dos_seek(fh, 0) ||
		(replay_dos_write(fh, &replay_header, sizeof(replay_header)) !=
		 sizeof(replay_header))
	) {
		replay_dos_close(fh);
		return false;
	}
	replay_dos_close(fh);
	return true;
}

static bool replay_buffer_flush(void)
{
	unsigned len;
	int fh;

	if(replay_buffer_len == 0) {
		return true;
	}
	len = (replay_buffer_len * REPLAY_USER_PACKET_SIZE);
	fh = replay_dos_open(replay_slot_fn, REPLAY_ACCESS_RW);
	if(fh < 0) {
		return false;
	}
	if(
		!replay_dos_seek(
			fh, (replay_header.input_offset + replay_payload_written)
		) ||
		(replay_dos_write(fh, replay_buffer, len) != len)
	) {
		replay_dos_close(fh);
		return false;
	}
	replay_dos_close(fh);
	replay_payload_written += len;
	replay_buffer_len = 0;
	return replay_header_write(false);
}

static bool replay_checkpoint_append(void)
{
	char checkpoint_fn[10];
	uint32_t hash = REPLAY_FNV1A_BASIS;
	uint32_t remaining = replay_header.checkpoint_size;
	uint32_t file_size;
	uint32_t offset = (
		replay_header.input_offset + replay_header.input_size
	);
	unsigned len;
	int source_fh;
	int replay_fh;
	bool ok = false;

	if((replay_header.flags & REPLAY_USER_FLAG_CHECKPOINT) == 0) {
		return true;
	}
	if(
		(replay_header.checkpoint_schema != REPLAY_CHECKPOINT_SCHEMA) ||
		(replay_header.checkpoint_offset != 0) ||
		(replay_header.checkpoint_size < REPLAY_CHECKPOINT_HEADER_SIZE) ||
		(replay_header.checkpoint_size > REPLAY_CHECKPOINT_SIZE_MAX) ||
		(replay_header.checkpoint_checksum == 0) ||
		(replay_header.source_fingerprint !=
		 REPLAY_CHECKPOINT_SOURCE_FINGERPRINT) ||
		(replay_header.state_digest == 0)
	) {
		return false;
	}
	replay_checkpoint_fn_set(checkpoint_fn);
	source_fh = replay_dos_open(checkpoint_fn, REPLAY_ACCESS_READ);
	if(source_fh < 0) {
		return false;
	}
	if(
		!replay_dos_size(source_fh, &file_size) ||
		(file_size != replay_header.checkpoint_size) ||
		!replay_dos_seek(source_fh, 0)
	) {
		replay_dos_close(source_fh);
		return false;
	}
	replay_fh = replay_dos_open(replay_slot_fn, REPLAY_ACCESS_RW);
	if(replay_fh < 0) {
		replay_dos_close(source_fh);
		return false;
	}
	if(!replay_dos_seek(replay_fh, offset)) {
		replay_dos_close(replay_fh);
		replay_dos_close(source_fh);
		return false;
	}
	while(remaining != 0) {
		len = ((remaining > REPLAY_BUFFER_SIZE)
			? REPLAY_BUFFER_SIZE
			: static_cast<unsigned>(remaining)
		);
		if(
			(replay_dos_read(source_fh, replay_buffer, len) != len) ||
			(replay_dos_write(replay_fh, replay_buffer, len) != len)
		) {
			break;
		}
		hash = replay_fnv1a(hash, replay_buffer, len);
		remaining -= len;
	}
	if(
		(remaining == 0) &&
		(hash == replay_header.checkpoint_checksum)
	) {
		replay_header.checkpoint_offset = offset;
		ok = true;
	}
	replay_dos_close(replay_fh);
	replay_dos_close(source_fh);
	return ok;
}

static bool replay_packet_commit(const replay_user_packet_t far *packet)
{
	replay_copy(
		&replay_buffer[replay_buffer_len], packet, REPLAY_USER_PACKET_SIZE
	);
	replay_buffer_len++;
	replay_header.packet_count++;
	replay_header.input_size += REPLAY_USER_PACKET_SIZE;
	replay_payload_checksum = replay_fnv1a(
		replay_payload_checksum, packet, REPLAY_USER_PACKET_SIZE
	);
	if(replay_buffer_len >= REPLAY_BUFFER_PACKET_COUNT) {
		return replay_buffer_flush();
	}
	return true;
}

static bool replay_pending_commit(void)
{
	if(!replay_pending_valid) {
		return true;
	}
	replay_pending.tag = static_cast<uint8_t>(
		(replay_pending.tag & 0xC0) | (replay_pending_run - 1)
	);
	replay_header.sample_count += replay_pending_run;
	if(!replay_packet_commit(&replay_pending)) {
		return false;
	}
	replay_pending_valid = false;
	replay_pending_run = 0;
	return true;
}

static bool replay_record_sample(uint8_t phase, input_t input, bool shift)
{
	uint8_t low = static_cast<uint8_t>(input & 0xFF);
	uint8_t high = static_cast<uint8_t>(input >> 8);

	if(
		replay_pending_valid &&
		((replay_pending.tag >> REPLAY_PACKET_PHASE_SHIFT) == phase) &&
		(replay_pending.input_low == low) &&
		(replay_pending.input_high == high) &&
		(replay_pending.shift == static_cast<uint8_t>(shift != false)) &&
		(replay_pending_run < REPLAY_PACKET_RUN_MAX)
	) {
		replay_pending_run++;
		return true;
	}
	if(!replay_pending_commit()) {
		return false;
	}
	replay_pending.tag = static_cast<uint8_t>(
		phase << REPLAY_PACKET_PHASE_SHIFT
	);
	replay_pending.input_low = low;
	replay_pending.input_high = high;
	replay_pending.shift = static_cast<uint8_t>(shift != false);
	replay_pending_run = 1;
	replay_pending_valid = true;
	return true;
}

static bool replay_record_control(uint8_t opcode, uint16_t arg, uint8_t arg8)
{
	replay_user_packet_t packet;

	if(!replay_pending_commit()) {
		return false;
	}
	packet.tag = static_cast<uint8_t>(
		(REPLAY_PACKET_PHASE_CONTROL << REPLAY_PACKET_PHASE_SHIFT) | opcode
	);
	packet.input_low = static_cast<uint8_t>(arg & 0xFF);
	packet.input_high = static_cast<uint8_t>(arg >> 8);
	packet.shift = arg8;
	return replay_packet_commit(&packet);
}

static bool replay_bytes_zero(const uint8_t far *p, unsigned size)
{
	while(size != 0) {
		if(*p++ != 0) {
			return false;
		}
		size--;
	}
	return true;
}

bool replay_checkpoint_identity_valid(
	const replay_start_config_t far *start
)
{
	if(start->kind == RSK_CHAPTER) {
		return (
			(start->phase == 0) &&
			replay_practice_chapter_valid(start->stage, start->section)
		);
	}
	if(start->kind == RSK_MIDBOSS) {
		return (
			(start->phase == 0) &&
			replay_practice_midboss_valid(start->stage, start->section)
		);
	}
	if(start->kind == RSK_BOSS_PHASE) {
		#if (GAME == 5)
			switch(start->stage) {
			case 0: return ((start->section == 0) && (start->phase <= 4));
			case 1: return ((start->section == 0) && (start->phase <= 7));
			case 2: return ((start->section == 0) && (start->phase <= 14));
			case 3:
				if(start->section == RCS_TH05_PAIR) {
					return (start->phase <= 2);
				}
				return (
					(start->section <= RCS_TH05_YUKI) &&
					(start->phase <= 9)
				);
			case 4: return ((start->section == 0) && (start->phase <= 10));
			case 5: return ((start->section == 0) && (start->phase <= 12));
			case STAGE_EXTRA:
				return ((start->section == 0) && (start->phase <= 17));
			}
		#else
			switch(start->stage) {
			case 0: return ((start->section == 0) && (start->phase <= 5));
			case 1: return ((start->section == 0) && (start->phase <= 6));
			case 2: return ((start->section == 0) && (start->phase <= 4));
			case 3:
				return (
					(start->section == 0) &&
					(start->phase <= ((start->playchar == 0) ? 3 : 12))
				);
			case 4: return ((start->section == 0) && (start->phase <= 18));
			case 5: return ((start->section == 0) && (start->phase <= 17));
			case STAGE_EXTRA:
				if(start->section == RCS_TH04_MUGETSU) {
					return (start->phase <= 7);
				}
				return (
					(start->section == RCS_TH04_GENGETSU) &&
					(start->phase <= 9)
				);
			}
		#endif
	}
	return false;
}

static bool replay_ck_capture_pending(void)
{
	return (
		(replay_mode == RRM_RECORD) &&
		replay_practice_preroll_pending
	);
}

static uint8_t replay_native_playperf(uint8_t start_rank)
{
	#if (GAME == 5)
		if(start_rank == RANK_HARD) {
			return 44;
		}
		if(start_rank == RANK_LUNATIC) {
			return 48;
		}
		return 32;
	#else
		if(start_rank == RANK_HARD) {
			return 20;
		}
		if(start_rank == RANK_LUNATIC) {
			return 22;
		}
		return 16;
	#endif
}

static bool replay_playperf_valid(uint8_t start_rank, uint8_t value)
{
	uint8_t min;
	uint8_t max;

	#if (GAME == 5)
		switch(start_rank) {
		case RANK_EASY: min = 16; max = 32; break;
		case RANK_NORMAL: min = 24; max = 40; break;
		case RANK_HARD: min = 44; max = 54; break;
		case RANK_LUNATIC: min = 48; max = 58; break;
		default: min = 32; max = 36; break;
		}
	#else
		switch(start_rank) {
		case RANK_EASY: min = 4; max = 16; break;
		case RANK_NORMAL: min = 11; max = 24; break;
		case RANK_HARD: min = 20; max = 32; break;
		case RANK_LUNATIC: min = 22; max = 34; break;
		default: min = 16; max = 20; break;
		}
	#endif
	return ((value >= min) && (value <= max));
}

static bool replay_start_config_valid(
	const replay_start_config_t far *start, bool practice, bool checkpoint
)
{
	bool kind_valid = (
		practice
			? (checkpoint
				? replay_checkpoint_identity_valid(start)
				: (start->kind == RSK_STAGE))
			: (start->kind == RSK_NATIVE)
	);
	if(
		(start->schema != REPLAY_START_SCHEMA) ||
		!kind_valid ||
		(start->stage > STAGE_EXTRA) ||
		((!checkpoint) && ((start->section != 0) || (start->phase != 0))) ||
		(start->rank > RANK_EXTRA) ||
		((start->stage == STAGE_EXTRA) != (start->rank == RANK_EXTRA)) ||
		(start->lives > 9) ||
		(start->bombs > 9) ||
		(start->power < 1) ||
		(start->power > 128) ||
		(start->continues_used > 9) ||
		(start->extends_gained > 10) ||
		(start->turbo_mode > 1) ||
		((start->stage == STAGE_EXTRA) && !start->turbo_mode) ||
		(start->score > 99999990UL) ||
		((start->score % 10UL) != 0) ||
		(start->credit_lives < 1) ||
		(start->credit_lives > 6) ||
		(start->credit_bombs > ((GAME == 5) ? 3 : 2)) ||
		(start->stage_graze > 999) ||
		(start->power_overflow > 42) ||
		!replay_playperf_valid(start->rank, start->playperf) ||
		!replay_bytes_zero(start->reserved, sizeof(start->reserved))
	) {
		return false;
	}
	#if (GAME == 5)
		if(
			(start->playchar > 3) || (start->shottype != 0) ||
			(start->dream > 128) ||
			(start->stage_point_items_collected > 999)
		) {
			return false;
		}
	#else
		if(
			(start->playchar > 1) || (start->shottype > 1) ||
			(start->dream > 7) ||
			(start->stage_point_items_collected > 255)
		) {
			return false;
		}
	#endif
	if(!practice && (
		((start->stage != 0) && (start->stage != STAGE_EXTRA)) ||
		(start->score != 0) ||
		(start->lives != start->credit_lives) ||
		(start->bombs != start->credit_bombs) ||
		(start->power != 1) ||
		(start->dream != ((GAME == 5) ? 1 : 0)) ||
		(start->continues_used != 0) ||
		(start->extends_gained != 0) ||
		(start->graze != 0) ||
		(start->std_frames != 0) ||
		(start->items_spawned != 0) ||
		(start->items_collected != 0) ||
		(start->point_items_collected != 0) ||
		(start->max_valued_point_items_collected != 0) ||
		(start->enemies_gone != 0) ||
		(start->enemies_killed != 0) ||
		(start->miss_count != 0) ||
		(start->bombs_used != 0) ||
		(start->stage_point_items_collected != 0) ||
		(start->stage_graze != 0) ||
		(start->power_overflow != 0) ||
		(start->playperf != replay_native_playperf(start->rank))
	)) {
		return false;
	}
	return true;
}

static bool replay_header_read(void)
{
	uint32_t stored;
	uint32_t computed;
	uint32_t file_size;
	uint32_t input_end;
	uint32_t expected_file_size;
	bool checkpoint;
	unsigned i;
	int fh;

	fh = replay_dos_open(replay_slot_fn, REPLAY_ACCESS_READ);
	if(fh < 0) {
		return false;
	}
	if(
		(replay_dos_read(fh, &replay_header, sizeof(replay_header)) !=
		 sizeof(replay_header)) ||
		!replay_dos_size(fh, &file_size)
	) {
		replay_dos_close(fh);
		return false;
	}
	replay_dos_close(fh);
	if(
		(replay_header.magic[0] != 'T') ||
		(replay_header.magic[1] != ('0' + GAME)) ||
		(replay_header.magic[2] != 'R') ||
		(replay_header.magic[3] != 'P') ||
		(replay_header.magic[4] != 'Y') ||
		(replay_header.magic[5] != ('0' + REPLAY_USER_VERSION)) ||
		(replay_header.magic[6] != '\0') ||
		(replay_header.magic[7] != '\0')
	) {
		return false;
	}
	checkpoint = (
		(replay_header.flags & REPLAY_USER_FLAG_CHECKPOINT) != 0
	);
	input_end = (replay_header.input_offset + replay_header.input_size);
	expected_file_size = input_end;
	if(checkpoint) {
		if(
			(replay_header.checkpoint_schema != REPLAY_CHECKPOINT_SCHEMA) ||
			(replay_header.checkpoint_offset != input_end) ||
			(replay_header.checkpoint_size < REPLAY_CHECKPOINT_HEADER_SIZE) ||
			(replay_header.checkpoint_size > REPLAY_CHECKPOINT_SIZE_MAX) ||
			(replay_header.checkpoint_checksum == 0) ||
			(replay_header.source_fingerprint !=
			 REPLAY_CHECKPOINT_SOURCE_FINGERPRINT) ||
			(replay_header.state_digest == 0)
		) {
			return false;
		}
		expected_file_size += replay_header.checkpoint_size;
	} else if(
		(replay_header.checkpoint_schema != 0) ||
		(replay_header.checkpoint_offset != 0) ||
		(replay_header.checkpoint_size != 0) ||
		(replay_header.checkpoint_checksum != 0) ||
		(replay_header.source_fingerprint != 0) ||
		(replay_header.state_digest != 0)
	) {
		return false;
	}
	if(
		(replay_header.version != REPLAY_USER_VERSION) ||
		(replay_header.header_size != REPLAY_USER_HEADER_SIZE) ||
		(replay_header.packet_size != REPLAY_USER_PACKET_SIZE) ||
		((replay_header.flags & ~REPLAY_USER_KNOWN_FLAGS) != 0) ||
		((replay_header.flags & (REPLAY_USER_FLAG_RLE_INPUT |
		 REPLAY_USER_FLAG_SHIFT_INPUT)) !=
		 (REPLAY_USER_FLAG_RLE_INPUT | REPLAY_USER_FLAG_SHIFT_INPUT)) ||
		(replay_header.status != RUS_FINALIZED) ||
		(replay_header.game_id != GAME) ||
		(replay_header.ruleset != REPLAY_USER_RULESET_STOCK) ||
		(replay_header.mode > RUM_PRACTICE) ||
		((replay_header.mode == RUM_PRACTICE) !=
		 ((replay_header.flags & REPLAY_USER_FLAG_PRACTICE) != 0)) ||
		(replay_header.input_semantics != REPLAY_USER_INPUT_SEMANTICS) ||
		(replay_header.input_offset != REPLAY_USER_HEADER_SIZE) ||
		(replay_header.input_size > REPLAY_USER_INPUT_SIZE_MAX) ||
		(replay_header.packet_count >
		 (REPLAY_USER_INPUT_SIZE_MAX / REPLAY_USER_PACKET_SIZE)) ||
		(replay_header.input_size !=
			(replay_header.packet_count * REPLAY_USER_PACKET_SIZE)) ||
		(file_size != expected_file_size) ||
		(replay_header.stage_reached > STAGE_EXTRA) ||
		!replay_start_config_valid(
			&replay_header.start, (replay_header.mode == RUM_PRACTICE),
			((replay_header.flags & REPLAY_USER_FLAG_CHECKPOINT) != 0)
		)
	) {
		return false;
	}
	for(i = 0; i < sizeof(replay_header.reserved); i++) {
		if(replay_header.reserved[i] != 0) {
			return false;
		}
	}
	stored = replay_header.header_checksum;
	replay_header_checksum_set();
	computed = replay_header.header_checksum;
	replay_header.header_checksum = stored;
	return (stored == computed);
}

static void replay_checkpoint_identity_fill(
	replay_ck_identity_t far *identity
)
{
	identity->start_kind = replay_header.start.kind;
	identity->stage = replay_header.start.stage;
	identity->section = replay_header.start.section;
	identity->phase = replay_header.start.phase;
	identity->source_fingerprint = REPLAY_CHECKPOINT_SOURCE_FINGERPRINT;
}

static uint32_t replay_checkpoint_group_offset(
	const replay_ck_plan_t far *plan, uint8_t group_id
)
{
	uint32_t offset = (
		replay_header.checkpoint_offset + plan->prefix_size
	);
	uint8_t i;

	for(i = 0; i < group_id; i++) {
		offset += plan->group_sizes[i];
	}
	return offset;
}

static uint8_t replay_checkpoint_apply_group_id(uint8_t index)
{
	switch(index) {
	case 0: return RCGI_RUN;
	case 1: return RCGI_STAGE_VM;
	case 2: return RCGI_PLAYER;
	case 3: return RCGI_BULLETS;
	case 4: return RCGI_ENEMIES;
	case 5: return RCGI_ACTORS;
	case 6: return RCGI_ITEMS;
	case 7: return RCGI_SCORING;
	case 8: return RCGI_FIELD;
	case 9: return RCGI_EFFECTS;
	case 10: return RCGI_PACING;
	#if (GAME == 5)
		case 11: return RCGI_DIALOG;
	#endif
	}
	return RCGI_RNG;
}

static bool replay_checkpoint_restore(void)
{
	replay_ck_identity_t identity;
	replay_ck_plan_t plan;
	uint32_t checkpoint_hash;
	uint32_t container_hash;
	uint32_t state_digest = REPLAY_FNV1A_BASIS;
	uint32_t stored_container_hash;
	uint32_t group_offset;
	uint32_t live_digest;
	uint16_t live_size;
	uint16_t size = static_cast<uint16_t>(replay_header.checkpoint_size);
	uint8_t group_id;
	uint8_t apply_index;
	int fh;
	bool ok = true;

	if(size < REPLAY_CHECKPOINT_PREFIX_SIZE) {
		return false;
	}
	fh = replay_dos_open(replay_slot_fn, REPLAY_ACCESS_READ);
	if(fh < 0) {
		return false;
	}
	replay_checkpoint_identity_fill(&identity);
	if(
		!replay_dos_seek(fh, replay_header.checkpoint_offset) ||
		(replay_dos_read(
			fh, replay_checkpoint_prefix, REPLAY_CHECKPOINT_PREFIX_SIZE
		) != REPLAY_CHECKPOINT_PREFIX_SIZE) ||
		!replay_ck_container_prefix_validate(
			&identity, replay_checkpoint_prefix,
			REPLAY_CHECKPOINT_PREFIX_SIZE, size, &plan
		)
	) {
		replay_dos_close(fh);
		return false;
	}
	checkpoint_hash = replay_fnv1a(
		REPLAY_FNV1A_BASIS, replay_checkpoint_prefix,
		REPLAY_CHECKPOINT_PREFIX_SIZE
	);
	stored_container_hash = plan.container_checksum;
	replay_ck_container_prefix_checksum_set(replay_checkpoint_prefix, 0);
	container_hash = replay_fnv1a(
		REPLAY_FNV1A_BASIS, replay_checkpoint_prefix,
		REPLAY_CHECKPOINT_PREFIX_SIZE
	);
	replay_ck_container_prefix_checksum_set(
		replay_checkpoint_prefix, stored_container_hash
	);

	// First pass: validate every group without mutating gameplay state.
	for(group_id = 0; group_id < REPLAY_CHECKPOINT_GROUP_COUNT; group_id++) {
		group_offset = replay_checkpoint_group_offset(&plan, group_id);
		state_digest = replay_ck_group_digest_begin(
			state_digest, group_id, plan.group_sizes[group_id]
		);
		if(
			(state_digest == 0) ||
			!replay_dos_hash_three_continue(
				fh, group_offset, plan.group_sizes[group_id],
				&checkpoint_hash, &container_hash, &state_digest
			) ||
			!replay_dos_seek(fh, group_offset) ||
			!replay_ck_group_validate_stream(
				group_id, replay_buffer, REPLAY_BUFFER_SIZE,
				plan.group_sizes[group_id],
				plan.group_checksums[group_id],
				replay_checkpoint_dos_read,
				static_cast<uint16_t>(fh)
			)
		) {
			ok = false;
		}
		if(!ok) {
			break;
		}
	}
	if(
		!ok || (checkpoint_hash != replay_header.checkpoint_checksum) ||
		(container_hash != stored_container_hash) ||
		(state_digest != plan.state_digest) ||
		(plan.state_digest != replay_header.state_digest)
	) {
		replay_dos_close(fh);
		return false;
	}

	// Second pass: apply only after the complete container has validated.
	for(
		apply_index = 0;
		apply_index < REPLAY_CHECKPOINT_GROUP_COUNT;
		apply_index++
	) {
		group_id = replay_checkpoint_apply_group_id(apply_index);
		group_offset = replay_checkpoint_group_offset(&plan, group_id);
		if(
			!replay_dos_seek(fh, group_offset) ||
			!replay_ck_group_apply_stream(
				group_id, replay_buffer, REPLAY_BUFFER_SIZE,
				plan.group_sizes[group_id],
				plan.group_checksums[group_id],
				replay_checkpoint_dos_read,
				static_cast<uint16_t>(fh)
			)
		) {
			ok = false;
		}
		if(!ok) {
			break;
		}
	}
	if(
		ok && replay_ck_container_measure(&identity, &live_size, &live_digest) &&
		(live_size == plan.total_size) && (live_digest == plan.state_digest)
	) {
		ok = true;
	} else {
		ok = false;
	}
	replay_dos_close(fh);
	return ok;
}

static bool replay_packet_read(replay_user_packet_t far *packet)
{
	uint32_t remaining;
	unsigned want;
	unsigned len;
	int fh;

	if(replay_packet_cursor >= replay_header.packet_count) {
		return false;
	}
	if(replay_buffer_pos >= replay_buffer_len) {
		remaining = (replay_header.packet_count - replay_packet_cursor);
		want = ((remaining > REPLAY_BUFFER_PACKET_COUNT)
			? REPLAY_BUFFER_PACKET_COUNT
			: static_cast<unsigned>(remaining)
		);
		len = (want * REPLAY_USER_PACKET_SIZE);
		fh = replay_dos_open(replay_slot_fn, REPLAY_ACCESS_READ);
		if(fh < 0) {
			return false;
		}
		if(
			!replay_dos_seek(
				fh,
				(replay_header.input_offset +
				 (replay_packet_cursor * REPLAY_USER_PACKET_SIZE))
			) ||
			(replay_dos_read(fh, replay_buffer, len) != len)
		) {
			replay_dos_close(fh);
			return false;
		}
		replay_dos_close(fh);
		replay_buffer_len = want;
		replay_buffer_pos = 0;
	}
	replay_copy(
		packet, &replay_buffer[replay_buffer_pos], REPLAY_USER_PACKET_SIZE
	);
	replay_buffer_pos++;
	replay_packet_cursor++;
	replay_payload_checksum = replay_fnv1a(
		replay_payload_checksum, packet, REPLAY_USER_PACKET_SIZE
	);
	return true;
}

static bool replay_playback_sample(
	uint8_t phase, input_t far *input, bool far *shift
)
{
	if(replay_decode_run == 0) {
		if(!replay_packet_read(&replay_pending)) {
			return false;
		}
		if((replay_pending.tag >> REPLAY_PACKET_PHASE_SHIFT) != phase) {
			return false;
		}
		replay_decode_run = static_cast<uint8_t>(
			(replay_pending.tag & REPLAY_PACKET_RUN_MASK) + 1
		);
	}
	*input = static_cast<input_t>(
		replay_pending.input_low |
		(static_cast<uint16_t>(replay_pending.input_high) << 8)
	);
	*shift = (replay_pending.shift != 0);
	replay_decode_run--;
	replay_sample_cursor++;
	return true;
}

static bool replay_playback_control(
	uint8_t opcode, uint16_t far *arg, uint8_t far *arg8
)
{
	replay_user_packet_t packet;

	if(replay_decode_run != 0) {
		return false;
	}
	if(!replay_packet_read(&packet)) {
		return false;
	}
	if(
		((packet.tag >> REPLAY_PACKET_PHASE_SHIFT) !=
		 REPLAY_PACKET_PHASE_CONTROL) ||
		((packet.tag & REPLAY_PACKET_RUN_MASK) != opcode)
	) {
		return false;
	}
	*arg = static_cast<uint16_t>(
		packet.input_low |
		(static_cast<uint16_t>(packet.input_high) << 8)
	);
	*arg8 = packet.shift;
	return true;
}

static void replay_fail(void)
{
	replay_failed = true;
}

static void replay_sample_current(uint8_t phase)
{
	input_t input;
	bool shift;

	if(replay_practice_preroll_pending) {
		if(phase == REPLAY_PACKET_PHASE_GAMEPLAY) {
			key_det = static_cast<input_t>(
				INPUT_SHOT |
				((stage_frame & 0x80) ? INPUT_LEFT : INPUT_RIGHT)
			);
		} else {
			replay_preroll_interstitial_cycle++;
			key_det = ((replay_preroll_interstitial_cycle & 1) != 0)
				? INPUT_NONE
				: static_cast<input_t>(INPUT_SHOT | INPUT_OK);
		}
		shiftkey = false;
		return;
	}
	if(replay_mode == RRM_RECORD) {
		if(replay_failed) {
			return;
		}
		if(!replay_record_sample(phase, key_det, shiftkey)) {
			replay_fail();
		}
	} else if(replay_mode == RRM_PLAYBACK) {
		if(!replay_playback_sample(phase, &input, &shift)) {
			replay_fail();
			key_det = ((phase == REPLAY_PACKET_PHASE_GAMEPLAY)
				? INPUT_NONE
				: static_cast<input_t>(INPUT_SHOT | INPUT_OK)
			);
			shiftkey = false;
			return;
		}
		key_det = input;
		shiftkey = shift;
	}
}

static void replay_header_capture(void)
{
	unsigned i;
	uint16_t date_year;
	uint8_t date_month;
	uint8_t date_day;
	uint8_t time_hour;
	uint8_t time_minute;
	uint8_t time_second;

	replay_memclear(&replay_header, sizeof(replay_header));
	replay_header.magic[0] = 'T';
	replay_header.magic[1] = ('0' + GAME);
	replay_header.magic[2] = 'R';
	replay_header.magic[3] = 'P';
	replay_header.magic[4] = 'Y';
	replay_header.magic[5] = ('0' + REPLAY_USER_VERSION);
	replay_header.version = REPLAY_USER_VERSION;
	replay_header.header_size = REPLAY_USER_HEADER_SIZE;
	replay_header.packet_size = REPLAY_USER_PACKET_SIZE;
	replay_header.flags = (
		REPLAY_USER_FLAG_RLE_INPUT | REPLAY_USER_FLAG_SHIFT_INPUT
	);
	replay_header.status = RUS_RECORDING;
	replay_header.game_id = GAME;
	replay_header.ruleset = REPLAY_USER_RULESET_STOCK;
	replay_header.mode = RUM_STORY;
	replay_header.input_semantics = REPLAY_USER_INPUT_SEMANTICS;
	replay_header.input_offset = REPLAY_USER_HEADER_SIZE;
	replay_header.start.schema = REPLAY_START_SCHEMA;
	replay_header.start.kind = RSK_NATIVE;
	replay_header.start.stage = resident->stage;
	replay_header.start.rank = (
		(resident->stage == STAGE_EXTRA) ? RANK_EXTRA : resident->rank
	);
	#if (GAME == 5)
		replay_header.start.playchar = resident->playchar;
		replay_header.start.shottype = 0;
	#else
		replay_header.start.playchar = (resident->playchar_ascii - '0');
		replay_header.start.shottype = resident->shottype;
	#endif
	replay_header.stage_reached = resident->stage;
	replay_header.start.resident_rand = resident->rand;
	replay_header.start.random_seed = random_seed;
	replay_header.start.credit_lives = resident->credit_lives;
	replay_header.start.credit_bombs = resident->credit_bombs;
	replay_header.start.lives = resident->credit_lives;
	replay_header.start.bombs = resident->credit_bombs;
	replay_header.start.power = 1;
	replay_header.start.dream = ((GAME == 5) ? 1 : 0);
	replay_header.start.turbo_mode = static_cast<uint8_t>(resident->turbo_mode);
	for(i = 0; i < REPLAY_USER_NAME_LEN; i++) {
		replay_header.name[i] = ' ';
	}
	_asm {
		mov	ah, 2Ah
		int	21h
		mov	date_year, cx
		mov	date_month, dh
		mov	date_day, dl
		mov	ah, 2Ch
		int	21h
		mov	time_hour, ch
		mov	time_minute, cl
		mov	time_second, dh
	}
	if(date_year >= 1980) {
		replay_header.dos_date = static_cast<uint16_t>(
			((date_year - 1980) << 9) |
			(static_cast<uint16_t>(date_month) << 5) | date_day
		);
	}
	replay_header.dos_time = static_cast<uint16_t>(
		(static_cast<uint16_t>(time_hour) << 11) |
		(static_cast<uint16_t>(time_minute) << 5) |
		(time_second >> 1)
	);
}

static void replay_start_capture_live(void)
{
	replay_start_config_t far *start = &replay_header.start;

	start->stage = stage_id;
	start->rank = rank;
	start->lives = replay_lives();
	start->bombs = replay_bombs();
	start->power = power;
	start->dream = replay_dream();
	start->playperf = playperf;
	start->turbo_mode = static_cast<uint8_t>(turbo_mode);
	start->continues_used = continues_used;
	start->extends_gained = extends_gained;
	start->score = replay_score_points();
	start->graze = resident->graze;
	start->std_frames = total_std_frames;
	start->items_spawned = items_spawned;
	start->items_collected = items_collected;
	start->point_items_collected = total_point_items_collected;
	start->max_valued_point_items_collected =
		total_max_valued_point_items;
	start->enemies_gone = enemies_gone;
	start->enemies_killed = enemies_killed;
	start->miss_count = resident->miss_count;
	start->bombs_used = resident->bombs_used;
	start->stage_point_items_collected = stage_point_items_collected;
	start->stage_graze = stage_graze;
	start->power_overflow = static_cast<uint16_t>(power_overflow);
}

static void replay_practice_config_apply(
	const replay_start_config_t far *start
);

bool replay_practice_checkpoint_capture(void)
{
	char checkpoint_fn[10];
	replay_ck_identity_t identity;
	replay_ck_plan_t plan;
	uint32_t container_checksum;
	uint32_t checksum;
	uint8_t group_id;
	int fh;
	bool ok = false;

	if(
		(replay_mode != RRM_RECORD) || replay_failed ||
		(replay_header.mode != RUM_PRACTICE) ||
		(replay_header.start.kind <= RSK_STAGE) ||
		((replay_header.flags & REPLAY_USER_FLAG_CHECKPOINT) != 0) ||
		!replay_stage_seen || (stage_id != replay_header.start.stage) ||
		(replay_header.packet_count != 0) ||
		(replay_header.sample_count != 0) ||
		(replay_buffer_len != 0) || (replay_payload_written != 0)
	) {
		replay_private_diagnostic = (0x20UL << 24);
		return false;
	}
	replay_practice_config_apply(&replay_practice_start);
	player_shot_level_update();
	replay_checkpoint_identity_fill(&identity);
	if(!replay_start_config_valid(&replay_header.start, true, true)) {
		replay_private_diagnostic = (0x21UL << 24);
		return false;
	}
	if(!replay_ck_container_plan(&identity, &plan)) {
		replay_private_diagnostic = (
			(0x22UL << 24) |
			(static_cast<uint32_t>(replay_ck_failure_group()) << 16) |
			replay_ck_failure_field()
		);
		return false;
	}
	if(
		(plan.total_size < REPLAY_CHECKPOINT_PREFIX_SIZE) ||
		(plan.total_size > REPLAY_CHECKPOINT_SIZE_MAX) ||
		(plan.prefix_size != REPLAY_CHECKPOINT_PREFIX_SIZE) ||
		(plan.state_digest == 0)
	) {
		replay_private_diagnostic = (0x23UL << 24) | plan.total_size;
		return false;
	}
	if(
		!replay_ck_container_prefix_encode(
			&identity, &plan, replay_checkpoint_prefix,
			REPLAY_CHECKPOINT_PREFIX_SIZE
		)
	) {
		replay_private_diagnostic = (0x25UL << 24);
		return false;
	}
	replay_checkpoint_fn_set(checkpoint_fn);
	fh = replay_dos_create(checkpoint_fn);
	if(
		(fh >= 0) &&
		(replay_dos_write(
			fh, replay_checkpoint_prefix, REPLAY_CHECKPOINT_PREFIX_SIZE
		) == REPLAY_CHECKPOINT_PREFIX_SIZE)
	) {
		ok = true;
		for(
			group_id = 0;
			group_id < REPLAY_CHECKPOINT_GROUP_COUNT;
			group_id++
		) {
			if(
				!replay_ck_group_encode_stream(
					group_id, replay_buffer, REPLAY_BUFFER_SIZE,
					plan.group_sizes[group_id],
					plan.group_checksums[group_id],
					replay_checkpoint_dos_write,
					static_cast<uint16_t>(fh)
				)) {
				ok = false;
				break;
			}
		}
		if(ok) {
			if(!replay_dos_hash(
				fh, 0, plan.total_size, &container_checksum
			)) {
				ok = false;
			}
		}
		if(ok) {
			replay_ck_container_prefix_checksum_set(
				replay_checkpoint_prefix, container_checksum
			);
			if(
				!replay_dos_seek(fh, 0) ||
				(replay_dos_write(
					fh, replay_checkpoint_prefix,
					REPLAY_CHECKPOINT_PREFIX_SIZE
				) != REPLAY_CHECKPOINT_PREFIX_SIZE) ||
				!replay_dos_hash(fh, 0, plan.total_size, &checksum)
			) {
				ok = false;
			}
		}
	}
	if(fh >= 0) {
		replay_dos_close(fh);
	}
	if(!ok) {
		if(replay_private_diagnostic == 0) {
			replay_private_diagnostic = (
				(0x25UL << 24) |
				(static_cast<uint32_t>(replay_ck_failure_group()) << 16)
			);
		}
		replay_checkpoint_temp_delete();
		return false;
	}
	replay_header.flags |= REPLAY_USER_FLAG_CHECKPOINT;
	replay_header.checkpoint_schema = REPLAY_CHECKPOINT_SCHEMA;
	replay_header.checkpoint_size = plan.total_size;
	replay_header.checkpoint_checksum = checksum;
	replay_header.source_fingerprint = REPLAY_CHECKPOINT_SOURCE_FINGERPRINT;
	replay_header.state_digest = plan.state_digest;
	if(!replay_record_control(REPLAY_CONTROL_STAGE_START, stage_id, 0)) {
		replay_private_diagnostic = (0x27UL << 24);
		replay_fail();
		return false;
	}
	replay_practice_preroll_pending = false;
	return true;
}

bool replay_practice_preroll_active(void)
{
	return replay_practice_preroll_pending;
}

bool replay_practice_direct_redraw_take(void)
{
	bool pending = replay_practice_direct_redraw_pending;

	replay_practice_direct_redraw_pending = false;
	return pending;
}

bool replay_private_test_active(void)
{
	return replay_private_test;
}

bool replay_practice_preroll_boundary(void)
{
	replay_ck_actor_probe_t probe;
	bool reached = false;

	if(!replay_practice_preroll_pending) {
		return false;
	}
	if(
		(replay_header.start.kind == RSK_CHAPTER) ||
		(replay_header.start.kind == RSK_MIDBOSS)
	) {
		if(!replay_ck_practice_direct_seek(&replay_header.start)) {
			replay_private_diagnostic = (0x04UL << 24) | stage_frame;
			replay_fail();
			quit = Q_QUIT_TO_OP;
			return true;
		}
		replay_practice_direct_redraw_pending = true;
		reached = true;
	} else {
		if(!replay_ck_actor_probe(&probe)) {
			replay_private_diagnostic = (
				(0x01UL << 24) | replay_sample_cursor
			);
			replay_fail();
			quit = Q_QUIT_TO_OP;
			return true;
		}
		if(
			(probe.boss_section != REPLAY_CK_BOSS_SECTION_NONE) &&
			((replay_preroll_boss_section != probe.boss_section) ||
			 (replay_preroll_boss_phase != probe.boss_phase))
		) {
			if(
				(replay_header.start.kind == RSK_BOSS_PHASE) &&
				(replay_header.start.section == probe.boss_section) &&
				(replay_header.start.phase == probe.boss_phase)
			) {
				reached = true;
			}
		}
		replay_preroll_boss_section = probe.boss_section;
		replay_preroll_boss_phase = probe.boss_phase;
	}
	if(!reached) {
		return false;
	}
	if(replay_mode == RRM_RECORD) {
		if(!replay_practice_checkpoint_capture()) {
			if(replay_private_diagnostic == 0) {
				replay_private_diagnostic = (0x02UL << 24);
			}
			replay_fail();
			quit = Q_QUIT_TO_OP;
		}
	} else {
		replay_practice_config_apply(&replay_practice_start);
		player_shot_level_update();
		replay_practice_preroll_pending = false;
	}
	return true;
}

static void replay_score_apply(uint32_t points, uint8_t continues)
{
	unsigned i;

	points /= 10UL;
	score.digits[0] = continues;
	for(i = 1; i < SCORE_DIGITS; i++) {
		score.digits[i] = static_cast<uint8_t>(points % 10UL);
		points /= 10UL;
	}
	score_delta = 0;
	score_delta_frame = 0;
}

static void replay_practice_config_apply(
	const replay_start_config_t far *start
)
{
	replay_score_apply(start->score, start->continues_used);
	playperf = start->playperf;
	continues_used = start->continues_used;
	extends_gained = start->extends_gained;
	power = start->power;
	power_overflow = start->power_overflow;
	stage_graze = start->stage_graze;
	stage_point_items_collected = start->stage_point_items_collected;
	total_std_frames = start->std_frames;
	items_spawned = start->items_spawned;
	items_collected = start->items_collected;
	total_point_items_collected = start->point_items_collected;
	total_max_valued_point_items =
		start->max_valued_point_items_collected;
	enemies_gone = start->enemies_gone;
	enemies_killed = start->enemies_killed;
	resident->graze = start->graze;
	resident->std_frames = start->std_frames;
	resident->items_spawned = start->items_spawned;
	resident->items_collected = start->items_collected;
	resident->point_items_collected = start->point_items_collected;
	resident->max_valued_point_items_collected =
		start->max_valued_point_items_collected;
	resident->enemies_gone = start->enemies_gone;
	resident->enemies_killed = start->enemies_killed;
	resident->miss_count = start->miss_count;
	resident->bombs_used = start->bombs_used;
	#if (GAME == 5)
		lives = start->lives;
		bombs = start->bombs;
		dream = start->dream;
		extend_point_items_collected = start->point_items_collected;
	#else
		resident->rem_lives = start->lives;
		resident->rem_bombs = start->bombs;
		dream_items_collected = start->dream;
		dream_score = DREAM_SCORE_PER_ITEMS[dream_items_collected];
	#endif
}

static void replay_practice_start_apply(void)
{
	if(!replay_practice_start_pending) {
		return;
	}
	replay_practice_config_apply(&replay_practice_start);
}

bool replay_practice_run_start_requested(void)
{
	return replay_practice_start_pending;
}

bool replay_stage_is_first(uint8_t stage)
{
	return (
		(stage == 0) ||
		(stage == 6) ||
		replay_practice_run_start_requested()
	);
}

void replay_practice_start_apply_after_reset(void)
{
	replay_practice_start_apply();
}

void replay_practice_start_apply_and_stage_activate(void)
{
	replay_practice_start_apply();
	std_update = std_update_frames_then_animate_dialog_and_activate_boss_if_done;
	scroll_active = true;
}

void replay_practice_items_ready(void)
{
	if(!replay_practice_start_pending) {
		return;
	}
	#if (GAME == 4)
		dream_score = DREAM_SCORE_PER_ITEMS[dream_items_collected];
	#endif
	replay_practice_start_pending = false;
}

static void replay_header_apply(void)
{
	const replay_start_config_t far *start = &replay_header.start;

	resident->rand = start->resident_rand;
	random_seed = start->random_seed;
	// Extra is selected through [resident->stage]. [resident->rank] remains the
	// configured Story rank and must never receive the semantic Extra rank.
	if(start->stage != STAGE_EXTRA) {
		resident->rank = start->rank;
	}
	resident->stage = start->stage;
	resident->credit_lives = start->credit_lives;
	resident->credit_bombs = start->credit_bombs;
	resident->cfg_lives = start->credit_lives;
	resident->cfg_bombs = start->credit_bombs;
	resident->turbo_mode = (start->turbo_mode != 0);
	resident->demo_num = 0;
	resident->debug = false;
	#if (GAME == 5)
		resident->playchar = start->playchar;
		resident->debug_stage = 0;
		resident->debug_power = 0;
	#else
		resident->playchar_ascii = ('0' + start->playchar);
		resident->stage_ascii = ('0' + start->stage);
		resident->shottype = start->shottype;
	#endif
	if(replay_header.mode == RUM_PRACTICE) {
		replay_copy(&replay_practice_start, start, sizeof(*start));
		replay_practice_start_pending = true;
	}
}

static replay_command_mode_t replay_command_load(
	uint8_t far *slot, uint8_t far *flags, replay_start_config_t far *start
)
{
	replay_command_t command;
	uint32_t file_size;
	int fh;
	unsigned i;

	fh = replay_dos_open(replay_cfg_fn, REPLAY_ACCESS_READ);
	if(fh < 0) {
		return RCM_NONE;
	}
	replay_memclear(&command, sizeof(command));
	i = replay_dos_read(fh, &command, sizeof(command));
	if(!replay_dos_size(fh, &file_size)) {
		file_size = 0;
	}
	replay_dos_close(fh);
	replay_dos_delete(replay_cfg_fn);
	if((i != sizeof(command)) || (file_size != sizeof(command))) {
		return RCM_NONE;
	}
	if(
		(command.magic[0] != 'T') ||
		(command.magic[1] != ('0' + GAME)) ||
		(command.magic[2] != 'R') ||
		(command.magic[3] != 'C') ||
		(command.magic[4] != 'F') ||
		(command.magic[5] != 'G') ||
		(command.magic[6] != '2') ||
		(command.magic[7] != '\0') ||
		(command.slot >= REPLAY_USER_SLOT_COUNT) ||
		((command.flags & ~REPLAY_COMMAND_KNOWN_FLAGS) != 0) ||
		(command.reserved_0 != 0)
	) {
		return RCM_NONE;
	}
	for(i = 0; i < sizeof(command.reserved); i++) {
		if(command.reserved[i] != 0) {
			return RCM_NONE;
		}
	}
	if((command.mode != RCM_RECORD) && (command.mode != RCM_PLAYBACK)) {
		return RCM_NONE;
	}
	if(command.mode == RCM_PLAYBACK) {
		if(
			((command.flags & ~REPLAY_COMMAND_FLAG_PRIVATE_TEST) != 0) ||
			!replay_bytes_zero(
				reinterpret_cast<const uint8_t far *>(&command.start),
				sizeof(command.start)
			)
		) {
			return RCM_NONE;
		}
	} else if(command.flags & REPLAY_COMMAND_FLAG_PRACTICE) {
		if(
			((command.flags & REPLAY_COMMAND_FLAG_NO_RECORD) != 0) &&
			((command.flags & REPLAY_COMMAND_FLAG_PRIVATE_TEST) != 0)
		) {
			return RCM_NONE;
		}
		if(!replay_start_config_valid(
			&command.start, true, (command.start.kind > RSK_STAGE)
		)) {
			return RCM_NONE;
		}
	} else if(
		(command.flags != 0) ||
		!replay_bytes_zero(
			reinterpret_cast<const uint8_t far *>(&command.start),
			sizeof(command.start)
		)
	) {
		return RCM_NONE;
	}
	*slot = command.slot;
	*flags = command.flags;
	replay_copy(start, &command.start, sizeof(command.start));
	return static_cast<replay_command_mode_t>(command.mode);
}

void replay_entry(void)
{
	replay_command_mode_t command_mode;
	uint8_t slot;
	uint8_t command_flags;
	replay_start_config_t command_start;

	if(replay_mode != RRM_DISABLED) {
		return;
	}
	replay_paths_init();
	if(oracle_active()) {
		// Both command files are one-shot. The validation oracle takes
		// precedence without leaving a user replay command for the next run.
		replay_dos_delete(replay_cfg_fn);
		return;
	}
	command_mode = replay_command_load(&slot, &command_flags, &command_start);
	if(command_mode == RCM_NONE) {
		return;
	}
	replay_slot_set(slot);
	replay_payload_checksum = REPLAY_FNV1A_BASIS;
	replay_buffer_len = 0;
	replay_buffer_pos = 0;
	replay_payload_written = 0;
	replay_packet_cursor = 0;
	replay_sample_cursor = 0;
	replay_pending_run = 0;
	replay_decode_run = 0;
	replay_pending_valid = false;
	replay_failed = false;
	replay_finished = false;
	replay_stage_seen = false;
	replay_private_test = (
		(command_flags & REPLAY_COMMAND_FLAG_PRIVATE_TEST) != 0
	);
	replay_private_diagnostic = 0;
	replay_practice_start_pending = false;
	replay_preroll_boss_section = REPLAY_CK_BOSS_SECTION_NONE;
	replay_preroll_boss_phase = 0xFF;
	replay_preroll_interstitial_cycle = 0;
	replay_practice_preroll_pending = false;
	replay_practice_direct_redraw_pending = false;

	if(command_mode == RCM_RECORD) {
		replay_checkpoint_temp_delete();
		replay_mode = ((command_flags & REPLAY_COMMAND_FLAG_NO_RECORD)
			? RRM_PRACTICE
			: RRM_RECORD
		);
		replay_header_capture();
		if(command_flags & REPLAY_COMMAND_FLAG_PRACTICE) {
			replay_header.flags |= REPLAY_USER_FLAG_PRACTICE;
			replay_header.mode = RUM_PRACTICE;
			replay_copy(
				&replay_header.start, &command_start, sizeof(command_start)
			);
			replay_header.stage_reached = command_start.stage;
			replay_copy(
				&replay_practice_start, &command_start, sizeof(command_start)
			);
			replay_practice_start_pending = true;
			replay_practice_preroll_pending = (command_start.kind > RSK_STAGE);
			replay_header_apply();
		}
		if(replay_mode == RRM_PRACTICE) {
			return;
		}
		if(!replay_header_write(true)) {
			replay_fail();
			replay_mode = RRM_DISABLED;
		}
		return;
	}
	if(!replay_header_read()) {
		replay_mode = RRM_PLAYBACK;
		replay_fail();
		return;
	}
	replay_mode = RRM_PLAYBACK;
	replay_header_apply();
}

void replay_stage_start(void)
{
	uint16_t arg;
	uint8_t arg8;

	if(replay_mode == RRM_DISABLED) {
		return;
	}
	if(replay_failed) {
		quit = Q_QUIT_TO_OP;
		return;
	}
	if(
		(replay_mode == RRM_PLAYBACK) && !replay_stage_seen &&
		((replay_header.flags & REPLAY_USER_FLAG_CHECKPOINT) != 0) &&
		!replay_checkpoint_restore()
	) {
		replay_fail();
		quit = Q_QUIT_TO_OP;
		return;
	}
	if(replay_mode == RRM_RECORD) {
		if(replay_stage_seen && (replay_last_stage < REPLAY_USER_STAGE_COUNT)) {
			replay_header.stage_scores[replay_last_stage] =
				replay_score_points();
		}
		if(!replay_stage_seen && !replay_ck_capture_pending()) {
			replay_start_capture_live();
		}
		replay_header.stage_reached = stage_id;
		if(
			!replay_ck_capture_pending() &&
			!replay_failed &&
			!replay_record_control(REPLAY_CONTROL_STAGE_START, stage_id, 0)
		) {
			replay_private_diagnostic = (0x03UL << 24) | replay_sample_cursor;
			replay_fail();
		}
	} else if((replay_mode == RRM_PLAYBACK) && (
		!replay_playback_control(REPLAY_CONTROL_STAGE_START, &arg, &arg8) ||
		(arg != stage_id) || (arg8 != 0)
	)) {
		replay_fail();
		quit = Q_QUIT_TO_OP;
	}
	replay_last_stage = stage_id;
	replay_stage_seen = true;
}

void replay_main_entry_setup(void)
{
	oracle_entry();
	replay_entry();

	stage_id = resident->stage;
	if(stage_id == STAGE_EXTRA) {
		rank = RANK_EXTRA;
	} else {
		rank = resident->rank;
	}
	#if (GAME == 4)
		eyename[3] = ('0' + rank);
	#endif
}

bool replay_frame_pacing_should_delay(void)
{
	total_slow_frames += (
		#if (GAME == 5)
			slowdown_caused_by_bullets |
		#endif
		(vsync_Count1 >= slowdown_factor)
	);
	total_frames++;

	#if (GAME == 5)
		#define DEBUG_FF_OFF 0x00
		#define DEBUG_FF_TURNING_ON 0xFE
		#define DEBUG_FF_ON 0xFF
		#define DEBUG_FF_TURNING_OFF 0x01

		if(replay_practice_preroll_active() || replay_private_test_active()) {
			player_is_hit = false;
		} else if(debug_mode_active) {
			if(key_det & INPUT_Q) {
				if(debug_fast_forward == DEBUG_FF_OFF) {
					debug_fast_forward = DEBUG_FF_TURNING_ON;
				} else if(debug_fast_forward == DEBUG_FF_ON) {
					debug_fast_forward = DEBUG_FF_TURNING_OFF;
				}
			} else {
				if(debug_fast_forward == DEBUG_FF_TURNING_OFF) {
					debug_fast_forward = DEBUG_FF_OFF;
				} else if(debug_fast_forward == DEBUG_FF_TURNING_ON) {
					debug_fast_forward = DEBUG_FF_ON;
				}
			}
		}
		if(
			!replay_practice_preroll_active() &&
			(debug_fast_forward == DEBUG_FF_OFF)
		) {
			return true;
		}
		if(debug_fast_forward != DEBUG_FF_OFF) {
			player_is_hit = false;
		}
		return false;
	#else
		if(replay_practice_preroll_active() || replay_private_test_active()) {
			player_is_hit = false;
			return false;
		}
		return true;
	#endif
}

bool replay_stage_frame_advance_should_raise(void)
{
	unsigned int playperf_interval;

	frames_unused++;
	stage_frame++;
	stage_frame_mod16 = (stage_frame & 0x0F);
	stage_frame_mod8 = (stage_frame & 0x07);
	stage_frame_mod4 = (stage_frame & 0x03);
	stage_frame_mod2 = (stage_frame & 0x01);

	#if (GAME == 5)
		playperf_interval = 4096;
	#else
		playperf_interval = resident->rem_lives;
		playperf_interval = ((playperf_interval >= 10)
			? 1000
			: (6000 - (playperf_interval * 500))
		);
	#endif
	return ((stage_frame % playperf_interval) == 0);
}

void replay_metrics_commit(void)
{
	resident->std_frames = total_std_frames;
	resident->items_spawned = items_spawned;
	resident->items_collected = items_collected;
	resident->point_items_collected = total_point_items_collected;
	resident->max_valued_point_items_collected = total_max_valued_point_items;
	resident->enemies_gone = enemies_gone;
	resident->enemies_killed = enemies_killed;
	resident->slow_frames = total_slow_frames;
	resident->frames = total_frames;
}

void replay_gameplay_input(void)
{
	input_t host_input;

	if(replay_mode == RRM_DISABLED) {
		return;
	}
	if(replay_practice_preroll_pending) {
		replay_sample_current(REPLAY_PACKET_PHASE_GAMEPLAY);
		return;
	}
	if(replay_mode != RRM_PLAYBACK) {
		return;
	}
	host_input = key_det;
	if(
		(replay_header.end_reason == RUER_MENU_RETURN) &&
		(replay_sample_cursor >= replay_header.sample_count)
	) {
		key_det = INPUT_NONE;
		shiftkey = false;
		quit = Q_QUIT_TO_OP;
		return;
	}
	replay_sample_current(REPLAY_PACKET_PHASE_GAMEPLAY);
	key_det &= ~INPUT_CANCEL;
	if(host_input & INPUT_CANCEL) {
		key_det = INPUT_NONE;
		quit = Q_QUIT_TO_OP;
	}
	if(replay_failed) {
		quit = Q_QUIT_TO_OP;
	}
}

void replay_input_reset_sense_tail(void)
{
	if(replay_mode == RRM_RECORD) {
		replay_sample_current(REPLAY_PACKET_PHASE_GAMEPLAY);
		if(
			replay_private_test &&
			(replay_header.sample_count >= REPLAY_PRIVATE_SAMPLE_LIMIT)
		) {
			quit = Q_QUIT_TO_OP;
		}
	}
	input_reset_sense();
	if(replay_mode == RRM_PLAYBACK) {
		key_det = INPUT_NONE;
		shiftkey = false;
	}
}

void replay_input_reset_sense_interstitial(void)
{
	input_reset_sense();
	replay_sample_current(REPLAY_PACKET_PHASE_INTERSTITIAL);
}

void replay_input_sense_interstitial(void)
{
	input_sense();
	replay_sample_current(REPLAY_PACKET_PHASE_INTERSTITIAL);
}

int16_t replay_input_reset_sense_held_interstitial(void)
{
	#if (GAME == 5)
		input_reset_sense_held();
	#else
		input_reset_sense();
	#endif
	replay_sample_current(REPLAY_PACKET_PHASE_INTERSTITIAL);
	return static_cast<int16_t>(key_det);
}

void pascal replay_input_wait_for_change(int frames)
{
	#if (GAME == 5)
		int time;

		while(replay_input_reset_sense_held_interstitial()) {
		}
		time = frames;
		do {
			if(replay_input_reset_sense_held_interstitial()) {
				return;
			}
			_AX = vsync_Count1;
			while(_AX == vsync_Count1) {
			}
			if(time) {
				time--;
				if(time == 0) {
					break;
				}
			}
		} while(1);
	#else
		int frame = 0;

		do {
			input_reset_sense();
			frame_delay(1);
			replay_input_sense_interstitial();
		} while(key_det != INPUT_NONE);
		if(!frames) {
			frames = 9999;
		}
		while(frame < frames) {
			input_reset_sense();
			frame_delay(1);
			replay_input_sense_interstitial();
			if(key_det != INPUT_NONE) {
				break;
			}
			frame++;
			if(frames == 9999) {
				frame = 0;
			}
		}
	#endif
}

static uint8_t replay_end_reason(void)
{
	if(resident->end_sequence == ES_SCORE) {
		return RUER_GAME_OVER;
	}
	if(
		(resident->end_sequence == ES_GOOD) ||
		(resident->end_sequence == ES_BAD) ||
		(resident->end_sequence == ES_EXTRA)
	) {
		return RUER_COMPLETE;
	}
	return RUER_MENU_RETURN;
}

bool replay_process_end(void)
{
	uint16_t arg;
	uint8_t arg8;
	uint8_t end_reason;

	if(replay_finished) {
		return (replay_mode == RRM_PLAYBACK);
	}
	replay_finished = true;
	if(replay_mode == RRM_DISABLED) {
		return false;
	}
	if(replay_mode == RRM_PRACTICE) {
		return false;
	}
	end_reason = replay_end_reason();
	if(replay_mode == RRM_RECORD) {
		if(replay_stage_seen && (replay_last_stage < REPLAY_USER_STAGE_COUNT)) {
			replay_header.stage_scores[replay_last_stage] =
				replay_score_points();
		}
		replay_header.end_reason = end_reason;
		replay_header.score_final = replay_score_points();
		replay_header.lives_final = replay_lives();
		replay_header.bombs_final = replay_bombs();
		replay_header.power_final = power;
		replay_header.dream_final = replay_dream();
		if(!replay_failed && replay_ck_capture_pending()) {
			replay_private_diagnostic = (0x04UL << 24) | replay_sample_cursor;
			replay_fail();
		}
		if(!replay_failed) {
			if(
				!replay_record_control(
					REPLAY_CONTROL_TERMINAL,
					end_reason,
					resident->end_sequence
				) ||
				!replay_buffer_flush() ||
				!replay_checkpoint_append()
			) {
				replay_fail();
			}
		}
		replay_header.status = (replay_failed ? RUS_ERROR : RUS_FINALIZED);
		if(!replay_header_write(false)) {
			replay_failed = true;
		}
		replay_checkpoint_temp_delete();
		replay_private_result_write(end_reason);
		return false;
	}
	if(
		replay_failed ||
		!replay_playback_control(REPLAY_CONTROL_TERMINAL, &arg, &arg8) ||
		(arg != end_reason) ||
		(arg8 != resident->end_sequence) ||
		(replay_decode_run != 0) ||
		(replay_packet_cursor != replay_header.packet_count) ||
		(replay_sample_cursor != replay_header.sample_count) ||
		(replay_payload_checksum != replay_header.payload_checksum)
	) {
		replay_fail();
	}
	replay_private_result_write(end_reason);
	return true;
}

bool replay_active(void)
{
	return (replay_mode != RRM_DISABLED);
}

bool replay_playback_active(void)
{
	return (replay_mode == RRM_PLAYBACK);
}

// Preserve the paragraph phase of every following stock CODE segment.
#if (GAME == 4)
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#else
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90"
#endif

#pragma codeseg
