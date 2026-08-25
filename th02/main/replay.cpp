/* ReC98 replay mod -- TH02 compact native Story input stream.
 *
 * This translation unit deliberately owns a new trailing code segment. Its
 * state is all BSS, appended after every original contributor, so no original
 * TH02 data or BSS offset moves. The user file contract is in
 * th02/replay_format.hpp and mirrored by tools/replay/th02_user_replay.py.
 */

// This deliberately remains outside MAIN_01: that original group is already
// full. Tupfile.lua links this object after every game contributor and before
// the fixed C runtime tail, leaving every original data/BSS offset untouched.
#pragma option -zCT2REPLAY_TEXT -G-

#include "platform.h"
#include "libs/master.lib/master.hpp"
#include "th01/rank.h"
#include "th02/replay_format.hpp"
#include "th02/resident.hpp"
#include "th02/core/globals.hpp"
#include "th02/hardware/input.hpp"
#include "th02/math/randring.hpp"
#include "th02/snd/snd.h"
#include "th02/main/main.hpp"
#include "th02/main/score.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/bomb.hpp"

#define T2REPLAY_BUFFER_PACKET_COUNT 256
#define T2REPLAY_BUFFER_SIZE (T2REPLAY_BUFFER_PACKET_COUNT * T2REPLAY_PACKET_SIZE)
#define T2REPLAY_INPUT_KNOWN 0xF1FF
#define T2REPLAY_DOS_ACCESS_READ 0
#define T2REPLAY_DOS_ACCESS_RW 2
#define T2REPLAY_FP_SEG(p) ((unsigned)(((unsigned long)(void far *)(p)) >> 16))
#define T2REPLAY_FP_OFF(p) ((unsigned)((unsigned long)(void far *)(p)))

enum t2replay_mode_t {
	T2RM_DISABLED = 0,
	T2RM_RECORD = 1,
	T2RM_PLAYBACK = 2,
};

static char t2replay_command_fn[10];
static char t2replay_slot_fn[11];
static bool t2replay_paths_ready;
static t2replay_mode_t t2replay_mode;
static t2replay_header_t t2replay_header;
static t2replay_packet_t t2replay_buffer[T2REPLAY_BUFFER_PACKET_COUNT];
static uint16_t t2replay_buffer_len;
static uint16_t t2replay_buffer_pos;
static uint32_t t2replay_payload_written;
static uint32_t t2replay_packet_cursor;
static uint32_t t2replay_sample_cursor;
static uint32_t t2replay_payload_checksum;
static t2replay_packet_t t2replay_pending;
static uint8_t t2replay_pending_run;
static uint8_t t2replay_decode_run;
static bool t2replay_pending_valid;
static bool t2replay_failed;
static bool t2replay_finished;
static bool t2replay_playback_exit;
static bool t2replay_stage_seen;
static uint8_t t2replay_last_stage;

static void t2replay_memclear(void far *buf, unsigned size)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(buf);

	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

static void t2replay_paths_init(void)
{
	if(t2replay_paths_ready) {
		return;
	}
	t2replay_command_fn[0] = 'T';
	t2replay_command_fn[1] = '2';
	t2replay_command_fn[2] = 'R';
	t2replay_command_fn[3] = 'P';
	t2replay_command_fn[4] = 'Y';
	t2replay_command_fn[5] = '.';
	t2replay_command_fn[6] = 'C';
	t2replay_command_fn[7] = 'F';
	t2replay_command_fn[8] = 'G';
	t2replay_command_fn[9] = '\0';
	t2replay_slot_fn[0] = 'T';
	t2replay_slot_fn[1] = 'H';
	t2replay_slot_fn[2] = '2';
	t2replay_slot_fn[3] = 'R';
	t2replay_slot_fn[4] = '0';
	t2replay_slot_fn[5] = '0';
	t2replay_slot_fn[6] = '.';
	t2replay_slot_fn[7] = 'R';
	t2replay_slot_fn[8] = 'P';
	t2replay_slot_fn[9] = 'Y';
	t2replay_slot_fn[10] = '\0';
	t2replay_paths_ready = true;
}

static void t2replay_slot_set(uint8_t slot)
{
	t2replay_slot_fn[4] = static_cast<char>('0' + (slot / 10));
	t2replay_slot_fn[5] = static_cast<char>('0' + (slot % 10));
}

static int t2replay_dos_open(const char far *fn, unsigned char access)
{
	unsigned fn_seg = T2REPLAY_FP_SEG(fn);
	unsigned fn_off = T2REPLAY_FP_OFF(fn);
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

static int t2replay_dos_create(const char far *fn)
{
	unsigned fn_seg = T2REPLAY_FP_SEG(fn);
	unsigned fn_off = T2REPLAY_FP_OFF(fn);
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

static void t2replay_dos_close(int fh)
{
	_asm {
		mov	bx, fh
		mov	ah, 3Eh
		int	21h
	}
}

static bool t2replay_dos_seek(int fh, uint32_t offset)
{
	unsigned offset_hi = static_cast<unsigned>(offset >> 16);
	unsigned offset_lo = static_cast<unsigned>(offset & 0xFFFFUL);
	unsigned failed;

	_asm {
		mov	bx, fh
		mov	cx, offset_hi
		mov	dx, offset_lo
		mov	ax, 4200h
		int	21h
		sbb	ax, ax
		neg	ax
		mov	failed, ax
	}
	return (failed == 0);
}

static bool t2replay_dos_size(int fh, uint32_t far *size)
{
	unsigned size_hi;
	unsigned size_lo;
	unsigned failed;

	_asm {
		mov	bx, fh
		xor	cx, cx
		xor	dx, dx
		mov	ax, 4202h
		int	21h
		mov	size_lo, ax
		mov	size_hi, dx
		sbb	ax, ax
		neg	ax
		mov	failed, ax
	}
	*size = (
		(static_cast<uint32_t>(size_hi) << 16) |
		static_cast<uint32_t>(size_lo)
	);
	return (failed == 0);
}

static unsigned t2replay_dos_read(int fh, void far *buf, unsigned size)
{
	unsigned buf_seg = T2REPLAY_FP_SEG(buf);
	unsigned buf_off = T2REPLAY_FP_OFF(buf);
	unsigned result;

	_asm {
		push	ds
		mov	bx, fh
		mov	cx, size
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

static unsigned t2replay_dos_write(int fh, const void far *buf, unsigned size)
{
	unsigned buf_seg = T2REPLAY_FP_SEG(buf);
	unsigned buf_off = T2REPLAY_FP_OFF(buf);
	unsigned result;

	_asm {
		push	ds
		mov	bx, fh
		mov	cx, size
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

static void t2replay_dos_delete(const char far *fn)
{
	unsigned fn_seg = T2REPLAY_FP_SEG(fn);
	unsigned fn_off = T2REPLAY_FP_OFF(fn);

	_asm {
		push	ds
		mov	dx, fn_off
		mov	ds, fn_seg
		mov	ah, 41h
		int	21h
		pop	ds
	}
}

static uint32_t t2replay_fnv1a(
	uint32_t hash, const void far *buf, unsigned size
)
{
	const uint8_t far *p = reinterpret_cast<const uint8_t far *>(buf);

	while(size != 0) {
		hash ^= static_cast<uint32_t>(*p++);
		hash *= T2REPLAY_FNV1A_PRIME;
		size--;
	}
	return hash;
}

static bool t2replay_bytes_zero(const uint8_t far *p, unsigned size)
{
	while(size != 0) {
		if(*p++ != 0) {
			return false;
		}
		size--;
	}
	return true;
}

static bool t2replay_magic_matches(const char far *magic, char last)
{
	return (
		(magic[0] == 'T') &&
		(magic[1] == '2') &&
		(magic[2] == 'R') &&
		(magic[3] == 'P') &&
		(magic[4] == 'Y') &&
		(magic[5] == last) &&
		(magic[6] == '\0') &&
		(magic[7] == '\0')
	);
}

static void t2replay_header_checksum_set(void)
{
	t2replay_header.header_checksum = 0;
	t2replay_header.header_checksum = t2replay_fnv1a(
		T2REPLAY_FNV1A_BASIS, &t2replay_header, sizeof(t2replay_header)
	);
}

static bool t2replay_header_write(bool create)
{
	int fd = (create
		? t2replay_dos_create(t2replay_slot_fn)
		: t2replay_dos_open(t2replay_slot_fn, T2REPLAY_DOS_ACCESS_RW)
	);

	if(fd < 0) {
		return false;
	}
	t2replay_header.payload_checksum = t2replay_payload_checksum;
	t2replay_header_checksum_set();
	if(!t2replay_dos_seek(fd, 0) ||
		(t2replay_dos_write(fd, &t2replay_header, sizeof(t2replay_header)) !=
		 sizeof(t2replay_header))) {
		t2replay_dos_close(fd);
		return false;
	}
	t2replay_dos_close(fd);
	return true;
}

static bool t2replay_buffer_flush(void)
{
	unsigned len;
	int fd;

	if(t2replay_buffer_len == 0) {
		return true;
	}
	len = (t2replay_buffer_len * T2REPLAY_PACKET_SIZE);
	fd = t2replay_dos_open(t2replay_slot_fn, T2REPLAY_DOS_ACCESS_RW);
	if(fd < 0) {
		return false;
	}
	if(!t2replay_dos_seek(fd, t2replay_header.input_offset + t2replay_payload_written) ||
		(t2replay_dos_write(fd, t2replay_buffer, len) != len)) {
		t2replay_dos_close(fd);
		return false;
	}
	t2replay_dos_close(fd);
	t2replay_payload_written += len;
	t2replay_buffer_len = 0;
	return t2replay_header_write(false);
}

static bool t2replay_packet_commit(const t2replay_packet_t far *packet)
{
	t2replay_buffer[t2replay_buffer_len] = *packet;
	t2replay_buffer_len++;
	t2replay_header.packet_count++;
	t2replay_header.input_size += T2REPLAY_PACKET_SIZE;
	t2replay_payload_checksum = t2replay_fnv1a(
		t2replay_payload_checksum, packet, T2REPLAY_PACKET_SIZE
	);
	if(t2replay_buffer_len >= T2REPLAY_BUFFER_PACKET_COUNT) {
		return t2replay_buffer_flush();
	}
	return true;
}

static bool t2replay_pending_commit(void)
{
	if(!t2replay_pending_valid) {
		return true;
	}
	t2replay_pending.tag = static_cast<uint8_t>(
		(t2replay_pending.tag & 0xC0) | (t2replay_pending_run - 1)
	);
	t2replay_header.sample_count += t2replay_pending_run;
	if(!t2replay_packet_commit(&t2replay_pending)) {
		return false;
	}
	t2replay_pending_valid = false;
	t2replay_pending_run = 0;
	return true;
}

static bool t2replay_record_sample(uint8_t phase)
{
	uint8_t low = static_cast<uint8_t>(key_det & 0xFF);
	uint8_t high = static_cast<uint8_t>(key_det >> 8);

	if(
		t2replay_pending_valid &&
		((t2replay_pending.tag >> T2REPLAY_PACKET_PHASE_SHIFT) == phase) &&
		(t2replay_pending.input_low == low) &&
		(t2replay_pending.input_high == high) &&
		(t2replay_pending_run < T2REPLAY_PACKET_RUN_MAX)
	) {
		t2replay_pending_run++;
		return true;
	}
	if(!t2replay_pending_commit()) {
		return false;
	}
	t2replay_pending.tag = static_cast<uint8_t>(
		phase << T2REPLAY_PACKET_PHASE_SHIFT
	);
	t2replay_pending.input_low = low;
	t2replay_pending.input_high = high;
	t2replay_pending.arg = 0;
	t2replay_pending_run = 1;
	t2replay_pending_valid = true;
	return true;
}

static bool t2replay_record_control(uint8_t opcode, uint16_t value, uint8_t arg)
{
	t2replay_packet_t packet;

	if(!t2replay_pending_commit()) {
		return false;
	}
	packet.tag = static_cast<uint8_t>(
		(T2REPLAY_PHASE_CONTROL << T2REPLAY_PACKET_PHASE_SHIFT) | opcode
	);
	packet.input_low = static_cast<uint8_t>(value & 0xFF);
	packet.input_high = static_cast<uint8_t>(value >> 8);
	packet.arg = arg;
	return t2replay_packet_commit(&packet);
}

static bool t2replay_start_valid(const t2replay_start_t far *start)
{
	if(
		(start->stage < 0) ||
		(start->stage >= T2REPLAY_STAGE_COUNT) ||
		(start->rank > RANK_EXTRA) ||
		((start->stage == (T2REPLAY_STAGE_COUNT - 1)) !=
		 (start->rank == RANK_EXTRA)) ||
		(start->rem_lives < 0) ||
		(start->rem_lives > 5) ||
		(start->rem_bombs < 0) ||
		(start->rem_bombs > 5) ||
		(start->start_lives < 1) ||
		(start->start_lives > 5) ||
		(start->start_bombs < 1) ||
		(start->start_bombs > 5) ||
		(start->start_power < 0) ||
		(start->start_power > 80) ||
		(start->random_seed != start->resident_frame) ||
		(start->shottype > 3) ||
		(start->bgm_mode > SND_BGM_MIDI) ||
		(start->reduce_effects > 1) ||
		(start->debug != 0) ||
		!t2replay_bytes_zero(start->reserved, sizeof(start->reserved))
	) {
		return false;
	}
	return true;
}

static bool t2replay_packet_is_valid(
	const t2replay_packet_t far *packet, uint32_t far *samples,
	bool far *terminal_seen
)
{
	uint8_t phase = static_cast<uint8_t>(
		packet->tag >> T2REPLAY_PACKET_PHASE_SHIFT
	);
	uint8_t low = static_cast<uint8_t>(packet->tag & T2REPLAY_PACKET_RUN_MASK);
	input_t input;

	if(*terminal_seen) {
		return false;
	}
	if(phase < T2REPLAY_PHASE_CONTROL) {
		input = static_cast<input_t>(
			packet->input_low | (static_cast<uint16_t>(packet->input_high) << 8)
		);
		if((packet->arg != 0) || (input & ~T2REPLAY_INPUT_KNOWN)) {
			return false;
		}
		*samples += static_cast<uint32_t>(low + 1);
		return (*samples >= static_cast<uint32_t>(low + 1));
	}
	if(phase != T2REPLAY_PHASE_CONTROL) {
		return false;
	}
	if((low == T2REPLAY_CONTROL_STAGE_START) && (packet->arg == 0)) {
		return (
			(packet->input_high == 0) &&
			(packet->input_low < T2REPLAY_STAGE_COUNT) &&
			!(*terminal_seen)
		);
	}
	if(low == T2REPLAY_CONTROL_TERMINAL) {
		if(
			(packet->input_high != 0) ||
			((packet->input_low != T2REPLAY_END_GAME_OVER) &&
			 (packet->input_low != T2REPLAY_END_CLEAR)) ||
			(packet->arg >= T2REPLAY_STAGE_COUNT) ||
			*terminal_seen
		) {
			return false;
		}
		*terminal_seen = true;
		return true;
	}
	return false;
}

static bool t2replay_payload_validate(int fd, uint32_t file_size)
{
	uint32_t hash = T2REPLAY_FNV1A_BASIS;
	uint32_t samples = 0;
	uint32_t packets_seen = 0;
	unsigned want;
	unsigned len;
	unsigned i;
	bool terminal_seen = false;
	bool stage_seen = false;
	uint8_t expected_stage = static_cast<uint8_t>(t2replay_header.start.stage);
	uint8_t terminal_reason = 0;
	uint8_t terminal_stage = 0;

	if(file_size != (t2replay_header.input_offset + t2replay_header.input_size)) {
		return false;
	}
	if(!t2replay_dos_seek(fd, t2replay_header.input_offset)) {
		return false;
	}
	while(packets_seen < t2replay_header.packet_count) {
		want = static_cast<unsigned>(
			((t2replay_header.packet_count - packets_seen) >
			 T2REPLAY_BUFFER_PACKET_COUNT)
				? T2REPLAY_BUFFER_PACKET_COUNT
				: (t2replay_header.packet_count - packets_seen)
		);
		len = (want * T2REPLAY_PACKET_SIZE);
		if(t2replay_dos_read(fd, t2replay_buffer, len) != len) {
			return false;
		}
		hash = t2replay_fnv1a(hash, t2replay_buffer, len);
		for(i = 0; i < want; i++) {
			if(!t2replay_packet_is_valid(
				&t2replay_buffer[i], &samples, &terminal_seen
			)) {
				return false;
			}
			if((t2replay_buffer[i].tag == static_cast<uint8_t>(
				(T2REPLAY_PHASE_CONTROL << T2REPLAY_PACKET_PHASE_SHIFT) |
				T2REPLAY_CONTROL_TERMINAL
			))) {
				if(
					!stage_seen ||
					(t2replay_buffer[i].arg != (expected_stage - 1))
				) {
					return false;
				}
				terminal_reason = t2replay_buffer[i].input_low;
				terminal_stage = t2replay_buffer[i].arg;
			} else if((t2replay_buffer[i].tag == static_cast<uint8_t>(
				(T2REPLAY_PHASE_CONTROL << T2REPLAY_PACKET_PHASE_SHIFT) |
				T2REPLAY_CONTROL_STAGE_START
			))) {
				if(
					(expected_stage >= T2REPLAY_STAGE_COUNT) ||
					(t2replay_buffer[i].input_low != expected_stage)
				) {
					return false;
				}
				stage_seen = true;
				expected_stage++;
			}
		}
		packets_seen += want;
	}
	return (
		(hash == t2replay_header.payload_checksum) &&
		(samples == t2replay_header.sample_count) &&
		terminal_seen &&
		(t2replay_header.stage_reached == (expected_stage - 1)) &&
		(terminal_reason == t2replay_header.end_reason) &&
		(terminal_stage == t2replay_header.terminal_stage)
	);
}

static bool t2replay_header_read(void)
{
	uint32_t file_size;
	uint32_t stored_checksum;
	uint32_t computed_checksum;
	int fd;

	fd = t2replay_dos_open(t2replay_slot_fn, T2REPLAY_DOS_ACCESS_READ);
	if(fd < 0) {
		return false;
	}
	if((t2replay_dos_read(fd, &t2replay_header, sizeof(t2replay_header)) !=
		 sizeof(t2replay_header)) || !t2replay_dos_size(fd, &file_size)) {
		t2replay_dos_close(fd);
		return false;
	}
	stored_checksum = t2replay_header.header_checksum;
	t2replay_header.header_checksum = 0;
	computed_checksum = t2replay_fnv1a(
		T2REPLAY_FNV1A_BASIS, &t2replay_header, sizeof(t2replay_header)
	);
	t2replay_header.header_checksum = stored_checksum;
	if(
		!t2replay_magic_matches(t2replay_header.magic, '1') ||
		(t2replay_header.version != T2REPLAY_VERSION) ||
		(t2replay_header.header_size != T2REPLAY_HEADER_SIZE) ||
		(t2replay_header.packet_size != T2REPLAY_PACKET_SIZE) ||
		(t2replay_header.flags != T2REPLAY_KNOWN_FLAGS) ||
		(t2replay_header.status != T2REPLAY_STATUS_FINALIZED) ||
		(t2replay_header.game_id != 2) ||
		(t2replay_header.ruleset != T2REPLAY_RULESET_STOCK) ||
		(t2replay_header.input_semantics != T2REPLAY_INPUT_SEMANTICS_KEY_DET) ||
		(t2replay_header.stage_count != T2REPLAY_STAGE_COUNT) ||
		(t2replay_header.stage_reached >= T2REPLAY_STAGE_COUNT) ||
		(t2replay_header.terminal_stage >= T2REPLAY_STAGE_COUNT) ||
		(t2replay_header.end_reason < T2REPLAY_END_GAME_OVER) ||
		(t2replay_header.end_reason > T2REPLAY_END_CLEAR) ||
		(t2replay_header.input_offset != T2REPLAY_HEADER_SIZE) ||
		(t2replay_header.input_size > T2REPLAY_INPUT_SIZE_MAX) ||
		(t2replay_header.packet_count >
		 (T2REPLAY_INPUT_SIZE_MAX / T2REPLAY_PACKET_SIZE)) ||
		(t2replay_header.input_size !=
		 (t2replay_header.packet_count * T2REPLAY_PACKET_SIZE)) ||
		(stored_checksum != computed_checksum) ||
		!t2replay_start_valid(&t2replay_header.start) ||
		!t2replay_bytes_zero(t2replay_header.reserved, sizeof(t2replay_header.reserved))
	) {
		t2replay_dos_close(fd);
		return false;
	}
	if(!t2replay_payload_validate(fd, file_size)) {
		t2replay_dos_close(fd);
		return false;
	}
	t2replay_dos_close(fd);
	t2replay_buffer_len = 0;
	t2replay_buffer_pos = 0;
	return true;
}

static bool t2replay_packet_read(t2replay_packet_t far *packet)
{
	uint32_t remaining;
	unsigned want;
	unsigned len;
	int fd;

	if(t2replay_packet_cursor >= t2replay_header.packet_count) {
		return false;
	}
	if(t2replay_buffer_pos >= t2replay_buffer_len) {
		remaining = (t2replay_header.packet_count - t2replay_packet_cursor);
		want = static_cast<unsigned>(
			(remaining > T2REPLAY_BUFFER_PACKET_COUNT)
				? T2REPLAY_BUFFER_PACKET_COUNT : remaining
		);
		len = (want * T2REPLAY_PACKET_SIZE);
		fd = t2replay_dos_open(t2replay_slot_fn, T2REPLAY_DOS_ACCESS_READ);
		if(fd < 0) {
			return false;
		}
		if(!t2replay_dos_seek(fd, t2replay_header.input_offset +
			(t2replay_packet_cursor * T2REPLAY_PACKET_SIZE)) ||
			(t2replay_dos_read(fd, t2replay_buffer, len) != len)) {
			t2replay_dos_close(fd);
			return false;
		}
		t2replay_dos_close(fd);
		t2replay_buffer_len = want;
		t2replay_buffer_pos = 0;
	}
	*packet = t2replay_buffer[t2replay_buffer_pos++];
	t2replay_packet_cursor++;
	return true;
}

static bool t2replay_playback_sample(uint8_t phase)
{
	input_t input;

	if(t2replay_decode_run == 0) {
		if(!t2replay_packet_read(&t2replay_pending) ||
			((t2replay_pending.tag >> T2REPLAY_PACKET_PHASE_SHIFT) != phase) ||
			(t2replay_pending.arg != 0)) {
			return false;
		}
		t2replay_decode_run = static_cast<uint8_t>(
			(t2replay_pending.tag & T2REPLAY_PACKET_RUN_MASK) + 1
		);
	}
	input = static_cast<input_t>(
		t2replay_pending.input_low |
		(static_cast<uint16_t>(t2replay_pending.input_high) << 8)
	);
	if(input & ~T2REPLAY_INPUT_KNOWN) {
		return false;
	}
	key_det = input;
	t2replay_decode_run--;
	t2replay_sample_cursor++;
	return true;
}

static bool t2replay_playback_control(uint8_t opcode, uint16_t value, uint8_t arg)
{
	t2replay_packet_t packet;

	if((t2replay_decode_run != 0) || !t2replay_packet_read(&packet)) {
		return false;
	}
	return (
		(packet.tag == static_cast<uint8_t>(
			(T2REPLAY_PHASE_CONTROL << T2REPLAY_PACKET_PHASE_SHIFT) | opcode
		)) &&
		(packet.input_low == static_cast<uint8_t>(value & 0xFF)) &&
		(packet.input_high == static_cast<uint8_t>(value >> 8)) &&
		(packet.arg == arg)
	);
}

static void t2replay_fail(void)
{
	t2replay_failed = true;
	key_det = INPUT_NONE;
	quit = true;
}

static void t2replay_header_capture(void)
{
	t2replay_memclear(&t2replay_header, sizeof(t2replay_header));
	t2replay_header.magic[0] = 'T';
	t2replay_header.magic[1] = '2';
	t2replay_header.magic[2] = 'R';
	t2replay_header.magic[3] = 'P';
	t2replay_header.magic[4] = 'Y';
	t2replay_header.magic[5] = '1';
	t2replay_header.version = T2REPLAY_VERSION;
	t2replay_header.header_size = T2REPLAY_HEADER_SIZE;
	t2replay_header.packet_size = T2REPLAY_PACKET_SIZE;
	t2replay_header.flags = T2REPLAY_KNOWN_FLAGS;
	t2replay_header.status = T2REPLAY_STATUS_RECORDING;
	t2replay_header.game_id = 2;
	t2replay_header.ruleset = T2REPLAY_RULESET_STOCK;
	t2replay_header.input_semantics = T2REPLAY_INPUT_SEMANTICS_KEY_DET;
	t2replay_header.stage_count = T2REPLAY_STAGE_COUNT;
	t2replay_header.input_offset = T2REPLAY_HEADER_SIZE;
	t2replay_header.start.resident_frame = static_cast<uint32_t>(resident->frame);
	// main_entry() assigns this exact resident value to [random_seed] immediately
	// before stage_init(). stage_init() then consumes it to fill [randring].
	t2replay_header.start.random_seed = t2replay_header.start.resident_frame;
	t2replay_header.start.score = resident->score;
	t2replay_header.start.score_highest = resident->score_highest;
	t2replay_header.start.continues_used = resident->continues_used;
	t2replay_header.start.skill = resident->skill;
	t2replay_header.start.stage = resident->stage;
	t2replay_header.start.rank = resident->rank;
	t2replay_header.start.rem_lives = resident->rem_lives;
	t2replay_header.start.rem_bombs = resident->rem_bombs;
	t2replay_header.start.start_lives = resident->start_lives;
	t2replay_header.start.start_bombs = resident->start_bombs;
	t2replay_header.start.start_power = resident->start_power;
	t2replay_header.start.shottype = resident->shottype;
	t2replay_header.start.bgm_mode = resident->bgm_mode;
	t2replay_header.start.reduce_effects = (resident->reduce_effects ? 1 : 0);
}

static void t2replay_header_apply(void)
{
	const t2replay_start_t far *start = &t2replay_header.start;

	resident->frame = static_cast<long>(start->resident_frame);
	resident->score = start->score;
	resident->score_highest = start->score_highest;
	resident->continues_used = start->continues_used;
	resident->skill = start->skill;
	resident->stage = static_cast<unsigned char>(start->stage);
	resident->rank = start->rank;
	resident->rem_lives = start->rem_lives;
	resident->rem_bombs = start->rem_bombs;
	resident->start_lives = start->start_lives;
	resident->start_bombs = start->start_bombs;
	resident->start_power = start->start_power;
	resident->shottype = start->shottype;
	resident->bgm_mode = start->bgm_mode;
	resident->reduce_effects = (start->reduce_effects != 0);
	resident->debug = false;
	resident->demo_num = 0;
}

static bool t2replay_command_valid(const t2replay_command_t far *command)
{
	unsigned i;

	if(!t2replay_magic_matches(command->magic, 'C') ||
		((command->mode != T2REPLAY_COMMAND_RECORD) &&
		 (command->mode != T2REPLAY_COMMAND_PLAYBACK)) ||
		(command->slot >= T2REPLAY_SLOT_COUNT)) {
		return false;
	}
	for(i = 0; i < sizeof(command->reserved); i++) {
		if(command->reserved[i] != 0) {
			return false;
		}
	}
	return true;
}

static t2replay_mode_t t2replay_command_load(uint8_t far *slot)
{
	t2replay_command_t command;
	uint32_t size;
	int fd;

	fd = t2replay_dos_open(t2replay_command_fn, T2REPLAY_DOS_ACCESS_READ);
	if(fd < 0) {
		return T2RM_DISABLED;
	}
	if((t2replay_dos_read(fd, &command, sizeof(command)) != sizeof(command)) ||
		!t2replay_dos_size(fd, &size)) {
		t2replay_dos_close(fd);
		t2replay_dos_delete(t2replay_command_fn);
		return T2RM_DISABLED;
	}
	t2replay_dos_close(fd);
	t2replay_dos_delete(t2replay_command_fn);
	if((size != sizeof(command)) || !t2replay_command_valid(&command)) {
		return T2RM_DISABLED;
	}
	*slot = command.slot;
	return static_cast<t2replay_mode_t>(command.mode);
}

static void t2replay_final_score_capture(void)
{
	if(t2replay_stage_seen && (t2replay_last_stage < T2REPLAY_STAGE_COUNT)) {
		t2replay_header.stage_scores[t2replay_last_stage] =
			static_cast<uint32_t>(score);
	}
	t2replay_header.score_final = score;
	t2replay_header.lives_final = lives;
	t2replay_header.bombs_final = bombs;
	t2replay_header.power_final = power;
	t2replay_header.terminal_stage = static_cast<uint8_t>(stage_id);
}

static void t2replay_finalize(uint8_t end_reason)
{
	if(t2replay_finished || (t2replay_mode == T2RM_DISABLED)) {
		return;
	}
	t2replay_finished = true;
	if(t2replay_mode == T2RM_RECORD) {
		t2replay_final_score_capture();
		t2replay_header.end_reason = end_reason;
		if(!t2replay_failed &&
			(!t2replay_record_control(
				T2REPLAY_CONTROL_TERMINAL,
				end_reason,
				t2replay_header.terminal_stage
			) || !t2replay_buffer_flush())) {
			t2replay_failed = true;
		}
		t2replay_header.status = (
			t2replay_failed ? T2REPLAY_STATUS_ERROR : T2REPLAY_STATUS_FINALIZED
		);
		if(!t2replay_header_write(false)) {
			t2replay_failed = true;
		}
		t2replay_mode = T2RM_DISABLED;
	} else {
		if(
			t2replay_failed ||
			!t2replay_playback_control(
				T2REPLAY_CONTROL_TERMINAL,
				end_reason,
				static_cast<uint8_t>(stage_id)
			) ||
			(t2replay_decode_run != 0) ||
			(t2replay_packet_cursor != t2replay_header.packet_count) ||
			(t2replay_sample_cursor != t2replay_header.sample_count)
		) {
			t2replay_failed = true;
		}
		t2replay_playback_exit = true;
	}
}

void replay_entry(void)
{
	uint8_t slot;
	t2replay_mode_t command_mode;

	if(t2replay_mode != T2RM_DISABLED) {
		return;
	}
	t2replay_paths_init();
	command_mode = t2replay_command_load(&slot);
	if(command_mode == T2RM_DISABLED) {
		return;
	}
	t2replay_slot_set(slot);
	t2replay_payload_checksum = T2REPLAY_FNV1A_BASIS;
	t2replay_buffer_len = 0;
	t2replay_buffer_pos = 0;
	t2replay_payload_written = 0;
	t2replay_packet_cursor = 0;
	t2replay_sample_cursor = 0;
	t2replay_pending_run = 0;
	t2replay_decode_run = 0;
	t2replay_pending_valid = false;
	t2replay_failed = false;
	t2replay_finished = false;
	t2replay_playback_exit = false;
	t2replay_stage_seen = false;
	if(command_mode == T2RM_RECORD) {
		t2replay_mode = T2RM_RECORD;
		t2replay_header_capture();
		if(!t2replay_start_valid(&t2replay_header.start) ||
			!t2replay_header_write(true)) {
			t2replay_mode = T2RM_DISABLED;
		}
	} else if(t2replay_header_read()) {
		t2replay_mode = T2RM_PLAYBACK;
		t2replay_payload_checksum = T2REPLAY_FNV1A_BASIS;
		t2replay_header_apply();
	}
}

void replay_stage_start(void)
{
	if(t2replay_mode == T2RM_DISABLED) {
		return;
	}
	if(t2replay_mode == T2RM_RECORD) {
		if(t2replay_stage_seen && (t2replay_last_stage < T2REPLAY_STAGE_COUNT)) {
			t2replay_header.stage_scores[t2replay_last_stage] =
				static_cast<uint32_t>(score);
		}
		t2replay_header.stage_reached = static_cast<uint8_t>(stage_id);
		if(!t2replay_record_control(
			T2REPLAY_CONTROL_STAGE_START, stage_id, 0
		)) {
			t2replay_failed = true;
		}
	} else if(!t2replay_playback_control(
		T2REPLAY_CONTROL_STAGE_START, stage_id, 0
	)) {
		t2replay_fail();
	}
	t2replay_last_stage = static_cast<uint8_t>(stage_id);
	t2replay_stage_seen = true;
}

void replay_input_sample(uint8_t phase)
{
	input_t host_input;

	if(t2replay_mode == T2RM_DISABLED) {
		return;
	}
	host_input = key_det;
	if(t2replay_mode == T2RM_RECORD) {
		if(!t2replay_failed && !t2replay_record_sample(phase)) {
			t2replay_failed = true;
		}
	} else {
		if(!t2replay_playback_sample(phase)) {
			t2replay_fail();
			return;
		}
		if(host_input & INPUT_CANCEL) {
			t2replay_fail();
		}
	}
}

bool replay_gameover(void)
{
	if(t2replay_mode == T2RM_DISABLED) {
		return false;
	}
	t2replay_finalize(T2REPLAY_END_GAME_OVER);
	return t2replay_playback_exit;
}

bool replay_process_end(const char *binary_fn)
{
	if(!t2replay_finished && (t2replay_mode != T2RM_DISABLED)) {
		t2replay_finalize(
			(binary_fn[0] == 'm') ? T2REPLAY_END_CLEAR : T2REPLAY_END_GAME_OVER
		);
	}
	return t2replay_playback_exit;
}

bool replay_playback_active(void)
{
	return (t2replay_mode == T2RM_PLAYBACK);
}

#pragma codeseg
