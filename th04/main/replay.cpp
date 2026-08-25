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
#include "th04/main/oracle.hpp"
#include "th04/main/quit.hpp"
#include "th04/main/replay.hpp"
#include "th04/main/replay_checkpoint.hpp"
#include "th04/replay_format.hpp"
#include "th04/score.h"
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

#define REPLAY_FP_SEG(p) ((unsigned)(((unsigned long)(void far *)(p)) >> 16))
#define REPLAY_FP_OFF(p) ((unsigned)((unsigned long)(void far *)(p)))

#define REPLAY_ACCESS_READ 0
#define REPLAY_ACCESS_RW 2

enum replay_runtime_mode_t {
	RRM_DISABLED = 0,
	RRM_RECORD = 1,
	RRM_PLAYBACK = 2,
};

extern unsigned char stage_id;
extern unsigned char power;
extern unsigned char rank;
extern unsigned char playperf;
extern bool turbo_mode;
extern unsigned char continues_used;
extern unsigned char extends_gained;
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
extern "C" void far player_shot_level_update(void);
#if (GAME == 5)
	extern unsigned char lives;
	extern unsigned char bombs;
	extern unsigned char dream;
	extern unsigned int stage_point_items_collected;
	extern unsigned int extend_point_items_collected;
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
static uint8_t replay_last_stage;
static bool replay_practice_start_pending;
static replay_start_config_t replay_practice_start;
static bool replay_preroll_midboss_active;
static uint8_t replay_preroll_midboss_entries;
static uint8_t replay_preroll_midboss_completions;
static uint8_t replay_preroll_boss_section;
static uint8_t replay_preroll_boss_phase;

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
	uint8_t far *buffer;
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
	buffer = reinterpret_cast<uint8_t far *>(hmem_allocbyte(1024));
	if((buffer == 0) || !replay_dos_seek(replay_fh, offset)) {
		if(buffer != 0) {
			hmem_free(reinterpret_cast<void __seg *>(buffer));
		}
		replay_dos_close(replay_fh);
		replay_dos_close(source_fh);
		return false;
	}
	while(remaining != 0) {
		len = ((remaining > 1024UL)
			? 1024
			: static_cast<unsigned>(remaining)
		);
		if(
			(replay_dos_read(source_fh, buffer, len) != len) ||
			(replay_dos_write(replay_fh, buffer, len) != len)
		) {
			break;
		}
		hash = replay_fnv1a(hash, buffer, len);
		remaining -= len;
	}
	if(
		(remaining == 0) &&
		(hash == replay_header.checkpoint_checksum)
	) {
		replay_header.checkpoint_offset = offset;
		ok = true;
	}
	hmem_free(reinterpret_cast<void __seg *>(buffer));
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
		if(start->phase != 0) {
			return false;
		}
		#if (GAME == 5)
			return (
				(start->section == RCS_CHAPTER_2) &&
				(start->stage != 5)
			);
		#else
			if(start->stage == 3) {
				return (
					(start->section == RCS_CHAPTER_2) ||
					(start->section == RCS_CHAPTER_3)
				);
			}
			return (
				(start->section == RCS_CHAPTER_2) &&
				((start->stage <= 2) || (start->stage == STAGE_EXTRA))
			);
		#endif
	}
	if(start->kind == RSK_MIDBOSS) {
		if(start->phase != 0) {
			return false;
		}
		#if (GAME == 5)
			return (
				(start->section == RCS_MIDBOSS_PRIMARY) &&
				(start->stage != 5)
			);
		#else
			if(start->stage == 3) {
				return (start->section <= RCS_MIDBOSS_SECONDARY);
			}
			return (
				(start->section == RCS_MIDBOSS_PRIMARY) &&
				((start->stage <= 2) || (start->stage == STAGE_EXTRA))
			);
		#endif
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
		!replay_failed &&
		(replay_header.mode == RUM_PRACTICE) &&
		(replay_header.start.kind > RSK_STAGE) &&
		((replay_header.flags & REPLAY_USER_FLAG_CHECKPOINT) == 0)
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

static bool replay_checkpoint_restore(void)
{
	replay_ck_identity_t identity;
	uint32_t validated_digest;
	uint16_t size = static_cast<uint16_t>(replay_header.checkpoint_size);
	uint8_t far *data;
	int fh;
	bool ok = false;

	data = reinterpret_cast<uint8_t far *>(hmem_allocbyte(size));
	if(data == 0) {
		return false;
	}
	fh = replay_dos_open(replay_slot_fn, REPLAY_ACCESS_READ);
	if(fh >= 0) {
		if(
			replay_dos_seek(fh, replay_header.checkpoint_offset) &&
			(replay_dos_read(fh, data, size) == size) &&
			(replay_fnv1a(REPLAY_FNV1A_BASIS, data, size) ==
			 replay_header.checkpoint_checksum)
		) {
			replay_checkpoint_identity_fill(&identity);
			if(
				replay_ck_container_validate(
					&identity, data, size, &validated_digest
				) &&
				(validated_digest == replay_header.state_digest) &&
				replay_ck_container_apply(&identity, data, size)
			) {
				ok = true;
			}
		}
		replay_dos_close(fh);
	}
	hmem_free(reinterpret_cast<void __seg *>(data));
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

	if(replay_mode == RRM_RECORD) {
		if(replay_failed) {
			return;
		}
		if(replay_ck_capture_pending()) {
			key_det = ((phase == REPLAY_PACKET_PHASE_GAMEPLAY)
				? INPUT_SHOT
				: static_cast<input_t>(INPUT_SHOT | INPUT_OK)
			);
			shiftkey = false;
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
	uint8_t far *data;
	uint16_t measured_size;
	uint16_t encoded_size;
	uint32_t measured_digest;
	uint32_t encoded_digest;
	uint32_t checksum;
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
		return false;
	}
	replay_practice_config_apply(&replay_practice_start);
	player_shot_level_update();
	replay_checkpoint_identity_fill(&identity);
	if(
		!replay_start_config_valid(&replay_header.start, true, true) ||
		!replay_ck_container_measure(
			&identity, &measured_size, &measured_digest
		) ||
		(measured_size < REPLAY_CHECKPOINT_HEADER_SIZE) ||
		(measured_size > REPLAY_CHECKPOINT_SIZE_MAX) ||
		(measured_digest == 0)
	) {
		return false;
	}
	data = reinterpret_cast<uint8_t far *>(hmem_allocbyte(measured_size));
	if(data == 0) {
		return false;
	}
	if(
		replay_ck_container_encode(
			&identity, data, measured_size, &encoded_size, &encoded_digest
		) &&
		(encoded_size == measured_size) &&
		(encoded_digest == measured_digest)
	) {
		checksum = replay_fnv1a(REPLAY_FNV1A_BASIS, data, encoded_size);
		replay_checkpoint_fn_set(checkpoint_fn);
		fh = replay_dos_create(checkpoint_fn);
		if(fh >= 0) {
			if(replay_dos_write(fh, data, encoded_size) == encoded_size) {
				ok = true;
			}
			replay_dos_close(fh);
		}
	}
	hmem_free(reinterpret_cast<void __seg *>(data));
	if(!ok) {
		replay_checkpoint_temp_delete();
		return false;
	}
	replay_header.flags |= REPLAY_USER_FLAG_CHECKPOINT;
	replay_header.checkpoint_schema = REPLAY_CHECKPOINT_SCHEMA;
	replay_header.checkpoint_size = encoded_size;
	replay_header.checkpoint_checksum = checksum;
	replay_header.source_fingerprint = REPLAY_CHECKPOINT_SOURCE_FINGERPRINT;
	replay_header.state_digest = encoded_digest;
	if(!replay_record_control(REPLAY_CONTROL_STAGE_START, stage_id, 0)) {
		replay_fail();
		return false;
	}
	return true;
}

bool replay_practice_preroll_active(void)
{
	return replay_ck_capture_pending();
}

bool replay_practice_preroll_boundary(void)
{
	replay_ck_actor_probe_t probe;
	uint8_t section;
	bool reached = false;

	if(!replay_ck_capture_pending()) {
		return false;
	}
	if(!replay_ck_actor_probe(&probe)) {
		replay_fail();
		quit = Q_QUIT_TO_OP;
		return true;
	}
	if(probe.midboss_active && !replay_preroll_midboss_active) {
		section = replay_preroll_midboss_entries;
		replay_preroll_midboss_entries++;
		if(
			(replay_header.start.kind == RSK_MIDBOSS) &&
			(replay_header.start.section == section)
		) {
			reached = true;
		}
	}
	if(
		!probe.midboss_active && probe.midboss_finished &&
		(replay_preroll_midboss_completions <
		 replay_preroll_midboss_entries)
	) {
		replay_preroll_midboss_completions++;
		section = static_cast<uint8_t>(
			RCS_CHAPTER_2 + replay_preroll_midboss_completions - 1
		);
		if(
			(replay_header.start.kind == RSK_CHAPTER) &&
			(replay_header.start.section == section)
		) {
			reached = true;
		}
	}
	replay_preroll_midboss_active = probe.midboss_active;
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
	if(!reached) {
		return false;
	}
	if(!replay_practice_checkpoint_capture()) {
		replay_fail();
		quit = Q_QUIT_TO_OP;
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

void replay_practice_start_apply_after_reset(void)
{
	replay_practice_start_apply();
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
			(command.flags != 0) ||
			!replay_bytes_zero(
				reinterpret_cast<const uint8_t far *>(&command.start),
				sizeof(command.start)
			)
		) {
			return RCM_NONE;
		}
	} else if(command.flags & REPLAY_COMMAND_FLAG_PRACTICE) {
		if(!replay_start_config_valid(
			&command.start, true, (command.start.kind > RSK_STAGE)
		)) {
			return RCM_NONE;
		}
	} else if(!replay_bytes_zero(
		reinterpret_cast<const uint8_t far *>(&command.start),
		sizeof(command.start)
	)) {
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
	replay_practice_start_pending = false;
	replay_preroll_midboss_active = false;
	replay_preroll_midboss_entries = 0;
	replay_preroll_midboss_completions = 0;
	replay_preroll_boss_section = REPLAY_CK_BOSS_SECTION_NONE;
	replay_preroll_boss_phase = 0xFF;

	if(command_mode == RCM_RECORD) {
		replay_checkpoint_temp_delete();
		replay_mode = RRM_RECORD;
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
			replay_header_apply();
		}
		if(!replay_header_write(true)) {
			replay_fail();
			replay_mode = RRM_DISABLED;
		}
		return;
	}
	if(!replay_header_read()) {
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
			replay_fail();
		}
	} else if(
		!replay_playback_control(REPLAY_CONTROL_STAGE_START, &arg, &arg8) ||
		(arg != stage_id) || (arg8 != 0)
	) {
		replay_fail();
		quit = Q_QUIT_TO_OP;
	}
	replay_last_stage = stage_id;
	replay_stage_seen = true;
}

void replay_gameplay_input(void)
{
	input_t host_input;

	if(replay_mode == RRM_DISABLED) {
		return;
	}
	host_input = key_det;
	replay_sample_current(REPLAY_PACKET_PHASE_GAMEPLAY);
	if(replay_mode == RRM_PLAYBACK) {
		key_det &= ~INPUT_CANCEL;
		if(host_input & INPUT_CANCEL) {
			key_det = INPUT_NONE;
			quit = Q_QUIT_TO_OP;
		} else if(
			(replay_header.end_reason == RUER_MENU_RETURN) &&
			(replay_sample_cursor >= replay_header.sample_count)
		) {
			quit = Q_QUIT_TO_OP;
		}
		if(replay_failed) {
			quit = Q_QUIT_TO_OP;
		}
	}
}

void replay_input_reset_sense_tail(void)
{
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

#pragma codeseg
