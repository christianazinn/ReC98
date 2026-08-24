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
extern unsigned char continues_used;
#if (GAME == 5)
	extern unsigned char lives;
	extern unsigned char bombs;
	extern unsigned char dream;
#else
	extern unsigned char dream_items_collected;
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
			fh, (REPLAY_USER_HEADER_SIZE + replay_payload_written)
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

static bool replay_header_read(void)
{
	uint32_t stored;
	uint32_t computed;
	uint32_t file_size;
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
		(replay_header.magic[5] != '1') ||
		(replay_header.magic[6] != '\0') ||
		(replay_header.magic[7] != '\0')
	) {
		return false;
	}
	if(
		(replay_header.version != REPLAY_USER_VERSION) ||
		(replay_header.header_size != REPLAY_USER_HEADER_SIZE) ||
		(replay_header.packet_size != REPLAY_USER_PACKET_SIZE) ||
		(replay_header.flags !=
			(REPLAY_USER_FLAG_RLE_INPUT | REPLAY_USER_FLAG_SHIFT_INPUT)) ||
		(replay_header.status != RUS_FINALIZED) ||
		(replay_header.game_id != GAME) ||
		(replay_header.ruleset != REPLAY_USER_RULESET_STOCK) ||
		(replay_header.mode != RUM_STORY) ||
		(replay_header.input_semantics != REPLAY_USER_INPUT_SEMANTICS) ||
		(replay_header.input_offset != REPLAY_USER_HEADER_SIZE) ||
		(replay_header.input_size > REPLAY_USER_INPUT_SIZE_MAX) ||
		(replay_header.packet_count >
		 (REPLAY_USER_INPUT_SIZE_MAX / REPLAY_USER_PACKET_SIZE)) ||
		(replay_header.input_size !=
			(replay_header.packet_count * REPLAY_USER_PACKET_SIZE)) ||
		(file_size != (REPLAY_USER_HEADER_SIZE + replay_header.input_size)) ||
		(replay_header.checkpoint_offset != 0) ||
		(replay_header.checkpoint_size != 0) ||
		((replay_header.start_stage != 0) &&
		 (replay_header.start_stage != STAGE_EXTRA)) ||
		(replay_header.start_section != 0) ||
		(replay_header.rank > RANK_LUNATIC) ||
		(replay_header.score_start != 0) ||
		(replay_header.lives_start != replay_header.credit_lives) ||
		(replay_header.bombs_start != replay_header.credit_bombs) ||
		(replay_header.power_start != 1) ||
		(replay_header.dream_start != ((GAME == 5) ? 1 : 0)) ||
		(replay_header.turbo_mode > 1)
	) {
		return false;
	}
	#if (GAME == 5)
		if((replay_header.playchar > 3) || (replay_header.shottype != 0)) {
			return false;
		}
	#else
		if((replay_header.playchar > 1) || (replay_header.shottype > 1)) {
			return false;
		}
	#endif
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
				(REPLAY_USER_HEADER_SIZE +
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
	replay_header.magic[5] = '1';
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
	replay_header.rank = resident->rank;
	#if (GAME == 5)
		replay_header.playchar = resident->playchar;
		replay_header.shottype = 0;
	#else
		replay_header.playchar = (resident->playchar_ascii - '0');
		replay_header.shottype = resident->shottype;
	#endif
	replay_header.start_stage = resident->stage;
	replay_header.stage_reached = resident->stage;
	replay_header.input_semantics = REPLAY_USER_INPUT_SEMANTICS;
	replay_header.input_offset = REPLAY_USER_HEADER_SIZE;
	replay_header.resident_rand = resident->rand;
	replay_header.random_seed = random_seed;
	replay_header.score_start = 0;
	replay_header.credit_lives = resident->credit_lives;
	replay_header.credit_bombs = resident->credit_bombs;
	replay_header.lives_start = resident->credit_lives;
	replay_header.bombs_start = resident->credit_bombs;
	replay_header.power_start = 1;
	replay_header.dream_start = ((GAME == 5) ? 1 : 0);
	replay_header.turbo_mode = static_cast<uint8_t>(resident->turbo_mode);
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

static void replay_header_apply(void)
{
	resident->rand = replay_header.resident_rand;
	random_seed = replay_header.random_seed;
	resident->rank = replay_header.rank;
	resident->stage = replay_header.start_stage;
	resident->credit_lives = replay_header.credit_lives;
	resident->credit_bombs = replay_header.credit_bombs;
	resident->cfg_lives = replay_header.credit_lives;
	resident->cfg_bombs = replay_header.credit_bombs;
	resident->turbo_mode = (replay_header.turbo_mode != 0);
	resident->demo_num = 0;
	resident->debug = false;
	#if (GAME == 5)
		resident->playchar = replay_header.playchar;
		resident->debug_stage = 0;
		resident->debug_power = 0;
	#else
		resident->playchar_ascii = ('0' + replay_header.playchar);
		resident->stage_ascii = ('0' + replay_header.start_stage);
		resident->shottype = replay_header.shottype;
	#endif
}

static replay_command_mode_t replay_command_load(uint8_t far *slot)
{
	replay_command_t command;
	int fh;
	unsigned i;

	fh = replay_dos_open(replay_cfg_fn, REPLAY_ACCESS_READ);
	if(fh < 0) {
		return RCM_NONE;
	}
	replay_memclear(&command, sizeof(command));
	i = replay_dos_read(fh, &command, sizeof(command));
	replay_dos_close(fh);
	replay_dos_delete(replay_cfg_fn);
	if(i != sizeof(command)) {
		return RCM_NONE;
	}
	if(
		(command.magic[0] != 'T') ||
		(command.magic[1] != ('0' + GAME)) ||
		(command.magic[2] != 'R') ||
		(command.magic[3] != 'C') ||
		(command.magic[4] != 'F') ||
		(command.magic[5] != 'G') ||
		(command.magic[6] != '1') ||
		(command.magic[7] != '\0') ||
		(command.slot >= REPLAY_USER_SLOT_COUNT)
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
	*slot = command.slot;
	return static_cast<replay_command_mode_t>(command.mode);
}

void replay_entry(void)
{
	replay_command_mode_t command_mode;
	uint8_t slot;

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
	command_mode = replay_command_load(&slot);
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

	if(command_mode == RCM_RECORD) {
		replay_mode = RRM_RECORD;
		replay_header_capture();
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
	if(replay_mode == RRM_RECORD) {
		if(replay_stage_seen && (replay_last_stage < REPLAY_USER_STAGE_COUNT)) {
			replay_header.stage_scores[replay_last_stage] =
				replay_score_points();
		}
		if(!replay_stage_seen) {
			replay_header.start_stage = stage_id;
			replay_header.lives_start = replay_lives();
			replay_header.bombs_start = replay_bombs();
			replay_header.power_start = power;
			replay_header.dream_start = replay_dream();
		}
		replay_header.stage_reached = stage_id;
		if(
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
		if(!replay_failed) {
			if(
				!replay_record_control(
					REPLAY_CONTROL_TERMINAL,
					end_reason,
					resident->end_sequence
				) ||
				!replay_buffer_flush()
			) {
				replay_fail();
			}
		}
		replay_header.status = (replay_failed ? RUS_ERROR : RUS_FINALIZED);
		if(!replay_header_write(false)) {
			replay_failed = true;
		}
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
