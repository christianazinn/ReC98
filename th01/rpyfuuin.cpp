/* TH01 replay continuation for FUUIN.EXE.
 *
 * All storage and code is isolated in a trailing module. The original FUUIN
 * modules contain only the receiver, phase, and raw-input call sites.
 */

#pragma option -zCT1REPLAY_FUUIN_TEXT -G-

#include <stddef.h>
#include <stdlib.h>
#include "platform.h"
#include "libs/master.lib/master.hpp"
#include "platform/x86real/pc98/keyboard.hpp"
#include "th01/core/initexit.hpp"
#include "th01/rpyfuuin.hpp"
#include "th01/resident.hpp"
#include "th01/shiftjis/fns.hpp"
#if T1REPLAY_FUUIN_SCORE_PROOF
#include "th01/formats/scoredat.hpp"
#endif

#define T1REPLAY_FUUIN_BUFFER_PACKET_COUNT 128
#define T1REPLAY_DOS_ACCESS_READ 0
#define T1REPLAY_DOS_ACCESS_RW 2
#define T1REPLAY_FP_SEG(p) ((unsigned)(((unsigned long)(void far *)(p)) >> 16))
#define T1REPLAY_FP_OFF(p) ((unsigned)((unsigned long)(void far *)(p)))

struct t1replay_fuuin_stream_state_t {
	uint32_t samples;
	uint32_t processes;
	uint8_t process;
	uint8_t process_seq;
	uint8_t source_process;
	uint8_t fuuin_phase;
	uint8_t terminal_reason;
	bool terminal_seen;
};

static char t1replay_slot_fn[11];
static char t1replay_save_request_fn[11];
#if T1REPLAY_FUUIN_SCORE_PROOF
static char t1replay_score_proof_fn[10];
#endif
static bool t1replay_paths_ready;
static bool t1replay_abort_pending;
static t1replay_mode_t t1replay_mode;
static uint8_t t1replay_phase;
static t1replay_header_t t1replay_header;
static t1replay_res_t far *t1replay_res;
static t1replay_packet_t t1replay_buffer[T1REPLAY_FUUIN_BUFFER_PACKET_COUNT];
static uint16_t t1replay_buffer_len;
static uint16_t t1replay_buffer_pos;
static uint32_t t1replay_payload_written;
static uint32_t t1replay_packet_cursor;
static uint32_t t1replay_sample_cursor;
static uint32_t t1replay_payload_checksum;
static t1replay_packet_t t1replay_pending;
static uint8_t t1replay_pending_run;
static uint8_t t1replay_decode_run;
static bool t1replay_pending_valid;
static uint8_t t1replay_keys[T1REPLAY_INPUT_GROUP_COUNT];
#if T1REPLAY_FUUIN_SCORE_PROOF
enum t1replay_score_observation_t {
	T1RSO_NONE = 0,
	T1RSO_UNAVAILABLE,
	T1RSO_BEFORE,
	T1RSO_AFTER,
};
static t1replay_score_proof_t t1replay_score_proof;
static uint8_t t1replay_score_observation;
static uint32_t t1replay_score_before_digest;
static uint32_t t1replay_score_after_digest;
#endif

static void t1replay_memclear(void far *buf, unsigned size)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(buf);

	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

static void t1replay_paths_init(void)
{
	if(t1replay_paths_ready) {
		return;
	}
	t1replay_slot_fn[0] = 'T'; t1replay_slot_fn[1] = 'H';
	t1replay_slot_fn[2] = '1'; t1replay_slot_fn[3] = 'R';
	t1replay_slot_fn[4] = '0'; t1replay_slot_fn[5] = '0';
	t1replay_slot_fn[6] = '.'; t1replay_slot_fn[7] = 'R';
	t1replay_slot_fn[8] = 'P'; t1replay_slot_fn[9] = 'Y';
	t1replay_slot_fn[10] = '\0';
	t1replay_save_request_fn[0] = 'T';
	t1replay_save_request_fn[1] = '1';
	t1replay_save_request_fn[2] = 'R';
	t1replay_save_request_fn[3] = 'S';
	t1replay_save_request_fn[4] = 'A';
	t1replay_save_request_fn[5] = 'V';
	t1replay_save_request_fn[6] = '.';
	t1replay_save_request_fn[7] = 'C';
	t1replay_save_request_fn[8] = 'F';
	t1replay_save_request_fn[9] = 'G';
	t1replay_save_request_fn[10] = '\0';
#if T1REPLAY_FUUIN_SCORE_PROOF
	t1replay_score_proof_fn[0] = 'T'; t1replay_score_proof_fn[1] = '1';
	t1replay_score_proof_fn[2] = 'S'; t1replay_score_proof_fn[3] = '0';
	t1replay_score_proof_fn[4] = '0'; t1replay_score_proof_fn[5] = '.';
	t1replay_score_proof_fn[6] = 'D'; t1replay_score_proof_fn[7] = 'I';
	t1replay_score_proof_fn[8] = 'G'; t1replay_score_proof_fn[9] = '\0';
#endif
	t1replay_paths_ready = true;
}

static void t1replay_res_id_init(char *id)
{
	id[0] = 'T'; id[1] = '1'; id[2] = 'R'; id[3] = 'e';
	id[4] = 'p'; id[5] = 'l'; id[6] = 'a'; id[7] = 'y';
	id[8] = 'S'; id[9] = 't'; id[10] = 'a'; id[11] = 't';
	id[12] = 'e'; id[13] = '\0';
}

static void t1replay_resident_id_init(char *id)
{
	id[0] = 'R'; id[1] = 'e'; id[2] = 'i'; id[3] = 'i';
	id[4] = 'd'; id[5] = 'e'; id[6] = 'n'; id[7] = 'C';
	id[8] = 'o'; id[9] = 'n'; id[10] = 'f'; id[11] = 'i';
	id[12] = 'g'; id[13] = '\0';
}

static bool t1replay_slot_set(uint8_t slot)
{
	if(t1replay_slot_is_pending(slot)) {
		t1replay_slot_fn[0] = 'T'; t1replay_slot_fn[1] = '1';
		t1replay_slot_fn[2] = 'R'; t1replay_slot_fn[3] = 'P';
		t1replay_slot_fn[4] = 'Y'; t1replay_slot_fn[5] = '.';
		t1replay_slot_fn[6] = 'T'; t1replay_slot_fn[7] = 'M';
		t1replay_slot_fn[8] = 'P'; t1replay_slot_fn[9] = '\0';
#if T1REPLAY_FUUIN_SCORE_PROOF
		// Pending clear evidence must not masquerade as slot 00 while OP
		// decides whether this capture becomes a numbered replay.
		t1replay_score_proof_fn[0] = 'T'; t1replay_score_proof_fn[1] = '1';
		t1replay_score_proof_fn[2] = 'S'; t1replay_score_proof_fn[3] = 'P';
		t1replay_score_proof_fn[4] = '.';
		t1replay_score_proof_fn[5] = 'D'; t1replay_score_proof_fn[6] = 'I';
		t1replay_score_proof_fn[7] = 'G'; t1replay_score_proof_fn[8] = '\0';
#endif
		return true;
	}
	if(!t1replay_slot_is_numbered(slot)) {
		return false;
	}
	t1replay_slot_fn[4] = static_cast<char>('0' + (slot / 10));
	t1replay_slot_fn[5] = static_cast<char>('0' + (slot % 10));
#if T1REPLAY_FUUIN_SCORE_PROOF
	t1replay_score_proof_fn[3] = static_cast<char>('0' + (slot / 10));
	t1replay_score_proof_fn[4] = static_cast<char>('0' + (slot % 10));
#endif
	return true;
}

static int t1replay_dos_open(const char far *fn, unsigned char access)
{
	unsigned fn_seg = T1REPLAY_FP_SEG(fn);
	unsigned fn_off = T1REPLAY_FP_OFF(fn);
	int result;

	_asm {
		push ds
		mov dx, fn_off
		mov ds, fn_seg
		mov ah, 3Dh
		mov al, access
		int 21h
		pop ds
		sbb dx, dx
		or ax, dx
		mov result, ax
	}
	return result;
}

static int t1replay_dos_create(const char far *fn)
{
	unsigned fn_seg = T1REPLAY_FP_SEG(fn);
	unsigned fn_off = T1REPLAY_FP_OFF(fn);
	int result;

	_asm {
		push ds
		mov dx, fn_off
		mov ds, fn_seg
		xor cx, cx
		mov ah, 3Ch
		int 21h
		pop ds
		sbb dx, dx
		or ax, dx
		mov result, ax
	}
	return result;
}

static void t1replay_dos_close(int fh)
{
	_asm {
		mov bx, fh
		mov ah, 3Eh
		int 21h
	}
}

static bool t1replay_dos_delete(const char far *fn)
{
	unsigned fn_seg = T1REPLAY_FP_SEG(fn);
	unsigned fn_off = T1REPLAY_FP_OFF(fn);
	unsigned result;

	_asm {
		push ds
		mov dx, fn_off
		mov ds, fn_seg
		mov ah, 41h
		int 21h
		pop ds
		sbb ax, ax
		not ax
		mov result, ax
	}
	return (result != 0);
}

static bool t1replay_dos_seek(int fh, uint32_t offset)
{
	unsigned offset_hi = static_cast<unsigned>(offset >> 16);
	unsigned offset_lo = static_cast<unsigned>(offset & 0xFFFFUL);
	unsigned failed;

	_asm {
		mov bx, fh
		mov cx, offset_hi
		mov dx, offset_lo
		mov ax, 4200h
		int 21h
		sbb ax, ax
		neg ax
		mov failed, ax
	}
	return (failed == 0);
}

static bool t1replay_dos_size(int fh, uint32_t far *size)
{
	unsigned size_hi;
	unsigned size_lo;
	unsigned failed;

	_asm {
		mov bx, fh
		xor cx, cx
		xor dx, dx
		mov ax, 4202h
		int 21h
		mov size_lo, ax
		mov size_hi, dx
		sbb ax, ax
		neg ax
		mov failed, ax
	}
	*size = (
		(static_cast<uint32_t>(size_hi) << 16) |
		static_cast<uint32_t>(size_lo)
	);
	return (failed == 0);
}

static unsigned t1replay_dos_read(int fh, void far *buf, unsigned size)
{
	unsigned buf_seg = T1REPLAY_FP_SEG(buf);
	unsigned buf_off = T1REPLAY_FP_OFF(buf);
	unsigned result;

	_asm {
		push ds
		mov bx, fh
		mov cx, size
		mov dx, buf_off
		mov ds, buf_seg
		mov ah, 3Fh
		int 21h
		pop ds
		sbb cx, cx
		not cx
		and ax, cx
		mov result, ax
	}
	return result;
}

static unsigned t1replay_dos_write(int fh, const void far *buf, unsigned size)
{
	unsigned buf_seg = T1REPLAY_FP_SEG(buf);
	unsigned buf_off = T1REPLAY_FP_OFF(buf);
	unsigned result;

	_asm {
		push ds
		mov bx, fh
		mov cx, size
		mov dx, buf_off
		mov ds, buf_seg
		mov ah, 40h
		int 21h
		pop ds
		sbb cx, cx
		not cx
		and ax, cx
		mov result, ax
	}
	return result;
}

static uint32_t t1replay_fnv1a(uint32_t hash, const void far *buf, unsigned size)
{
	const uint8_t far *p = reinterpret_cast<const uint8_t far *>(buf);

	while(size != 0) {
		hash ^= static_cast<uint32_t>(*p++);
		hash *= T1REPLAY_FNV1A_PRIME;
		size--;
	}
	return hash;
}

static bool t1replay_save_request_write(
	t1replay_save_request_source_t source
)
{
	t1replay_save_request_t request;
	int fd;
	bool ok;

	t1replay_memclear(&request, sizeof(request));
	request.magic[0] = 'T'; request.magic[1] = '1';
	request.magic[2] = 'R'; request.magic[3] = 'S';
	request.magic[4] = 'A'; request.magic[5] = 'V';
	request.magic[6] = '1'; request.magic[7] = '\0';
	request.schema = T1REPLAY_SAVE_REQUEST_SCHEMA;
	request.source = source;
	request.replay_header_checksum = t1replay_header.header_checksum;
	request.checksum = t1replay_fnv1a(
		T1REPLAY_FNV1A_BASIS, &request, sizeof(request)
	);
	t1replay_dos_delete(t1replay_save_request_fn);
	fd = t1replay_dos_create(t1replay_save_request_fn);
	if(fd < 0) {
		return false;
	}
	ok = (
		t1replay_dos_write(fd, &request, sizeof(request)) == sizeof(request)
	);
	t1replay_dos_close(fd);
	if(!ok) {
		t1replay_dos_delete(t1replay_save_request_fn);
	}
	return ok;
}

static bool t1replay_bytes_zero(const uint8_t far *p, unsigned size)
{
	while(size != 0) {
		if(*p++ != 0) {
			return false;
		}
		size--;
	}
	return true;
}

#if T1REPLAY_FUUIN_SCORE_PROOF
static uint32_t t1replay_score_table_digest(
	const void far *names,
	const void far *scores,
	const void far *stages,
	const void far *routes
)
{
	uint32_t digest = T1REPLAY_FNV1A_BASIS;

	digest = t1replay_fnv1a(digest, names, SCOREDAT_NAMES_SIZE);
	digest = t1replay_fnv1a(
		digest, scores, (sizeof(score_t) * SCOREDAT_PLACES)
	);
	digest = t1replay_fnv1a(
		digest, stages, (sizeof(int16_t) * SCOREDAT_PLACES)
	);
	return t1replay_fnv1a(
		digest, routes, (SCOREDAT_ROUTE_LEN * SCOREDAT_PLACES)
	);
}

static uint32_t t1replay_score_proof_checksum(
	t1replay_score_proof_t far *proof
)
{
	uint32_t checksum;

	proof->container_checksum = 0;
	checksum = t1replay_fnv1a(
		T1REPLAY_FNV1A_BASIS, proof, sizeof(*proof)
	);
	proof->container_checksum = checksum;
	return checksum;
}

static bool t1replay_score_proof_read(void)
{
	uint32_t file_size;
	uint32_t stored_checksum;
	uint32_t computed_checksum;
	int fd = t1replay_dos_open(
		t1replay_score_proof_fn, T1REPLAY_DOS_ACCESS_READ
	);

	if(fd < 0 || !t1replay_dos_size(fd, &file_size) ||
		!t1replay_dos_seek(fd, 0) ||
		(file_size != sizeof(t1replay_score_proof)) ||
		(t1replay_dos_read(
			fd, &t1replay_score_proof, sizeof(t1replay_score_proof)
		) != sizeof(t1replay_score_proof))) {
		if(fd >= 0) {
			t1replay_dos_close(fd);
		}
		return false;
	}
	t1replay_dos_close(fd);
	stored_checksum = t1replay_score_proof.container_checksum;
	computed_checksum = t1replay_score_proof_checksum(&t1replay_score_proof);
	if(stored_checksum != computed_checksum) {
		return false;
	}
	return (
		(t1replay_score_proof.magic[0] == 'T') &&
		(t1replay_score_proof.magic[1] == '1') &&
		(t1replay_score_proof.magic[2] == 'S') &&
		(t1replay_score_proof.magic[3] == 'D') &&
		(t1replay_score_proof.magic[4] == 'G') &&
		(t1replay_score_proof.magic[5] == '1') &&
		(t1replay_score_proof.magic[6] == '\0') &&
		(t1replay_score_proof.magic[7] == '\0') &&
		(t1replay_score_proof.schema == T1REPLAY_SCORE_PROOF_SCHEMA) &&
		(t1replay_score_proof.size == T1REPLAY_SCORE_PROOF_SIZE) &&
		(t1replay_score_proof.game_id == 1) &&
		(t1replay_score_proof.slot == t1replay_res->slot) &&
		(t1replay_score_proof.rank == t1replay_header.start.rank) &&
		((t1replay_score_proof.phase == T1REPLAY_FUUIN_PHASE_VERDICT) ||
		 (t1replay_score_proof.phase == T1REPLAY_FUUIN_PHASE_SCORE_NAME) ||
		 (t1replay_score_proof.phase == T1REPLAY_FUUIN_PHASE_SCORE_RELEASE)) &&
		(t1replay_score_proof.replay_start_checksum ==
			t1replay_header.start_checksum) &&
		(t1replay_score_proof.replay_payload_checksum ==
			t1replay_header.payload_checksum) &&
		(t1replay_score_proof.replay_sample_count ==
			t1replay_header.sample_count) &&
		(t1replay_score_proof.replay_packet_count ==
			t1replay_header.packet_count) &&
		(((t1replay_score_proof.phase == T1REPLAY_FUUIN_PHASE_VERDICT) &&
		  (t1replay_score_proof.before_digest == 0) &&
		  (t1replay_score_proof.after_digest == 0)) ||
		 ((t1replay_score_proof.phase != T1REPLAY_FUUIN_PHASE_VERDICT))) &&
		t1replay_bytes_zero(
			t1replay_score_proof.reserved,
			sizeof(t1replay_score_proof.reserved)
		)
	);
}

static bool t1replay_score_proof_write(void)
{
	int fd;

	t1replay_memclear(&t1replay_score_proof, sizeof(t1replay_score_proof));
	t1replay_score_proof.magic[0] = 'T';
	t1replay_score_proof.magic[1] = '1';
	t1replay_score_proof.magic[2] = 'S';
	t1replay_score_proof.magic[3] = 'D';
	t1replay_score_proof.magic[4] = 'G';
	t1replay_score_proof.magic[5] = '1';
	t1replay_score_proof.schema = T1REPLAY_SCORE_PROOF_SCHEMA;
	t1replay_score_proof.size = T1REPLAY_SCORE_PROOF_SIZE;
	t1replay_score_proof.game_id = 1;
	t1replay_score_proof.slot = t1replay_res->slot;
	t1replay_score_proof.rank = t1replay_header.start.rank;
	t1replay_score_proof.phase = t1replay_phase;
	t1replay_score_proof.replay_start_checksum =
		t1replay_header.start_checksum;
	t1replay_score_proof.replay_payload_checksum =
		t1replay_header.payload_checksum;
	t1replay_score_proof.replay_sample_count = t1replay_header.sample_count;
	t1replay_score_proof.replay_packet_count = t1replay_header.packet_count;
	if(t1replay_score_observation == T1RSO_AFTER) {
		t1replay_score_proof.before_digest =
			t1replay_score_before_digest;
		t1replay_score_proof.after_digest = t1replay_score_after_digest;
	}
	t1replay_score_proof_checksum(&t1replay_score_proof);
	fd = t1replay_dos_create(t1replay_score_proof_fn);
	if(fd < 0) {
		return false;
	}
	if(t1replay_dos_write(
		fd, &t1replay_score_proof, sizeof(t1replay_score_proof)
	) != sizeof(t1replay_score_proof)) {
		t1replay_dos_close(fd);
		return false;
	}
	t1replay_dos_close(fd);
	return true;
}

static bool t1replay_score_observation_complete(void)
{
	if(t1replay_score_observation == T1RSO_UNAVAILABLE) {
		return (t1replay_phase == T1REPLAY_FUUIN_PHASE_VERDICT);
	}
	return (
		(t1replay_score_observation == T1RSO_AFTER) &&
		((t1replay_phase == T1REPLAY_FUUIN_PHASE_SCORE_NAME) ||
		 (t1replay_phase == T1REPLAY_FUUIN_PHASE_SCORE_RELEASE))
	);
}
#endif

static bool t1replay_magic_matches(const char far *magic, char last)
{
	return (
		(magic[0] == 'T') && (magic[1] == '1') &&
		(magic[2] == 'R') && (magic[3] == 'P') &&
		(magic[4] == 'Y') && (magic[5] == last) &&
		(magic[6] == '\0') && (magic[7] == '\0')
	);
}

static bool t1replay_start_valid(const t1replay_start_t far *start)
{
	return (
		(start->stage_id < STAGE_COUNT) &&
		(start->rank >= 0) && (start->rank <= 3) &&
		(start->bgm_mode >= 0) && (start->bgm_mode < BGM_MODE_COUNT) &&
		(start->route >= 0) && (start->route < ROUTE_COUNT) &&
		(start->end_flag >= ES_NONE) && (start->end_flag <= ES_JIGOKU) &&
		(start->debug_mode == DM_OFF) &&
		(start->snd_need_init >= 0) && (start->snd_need_init <= 1) &&
		(start->mode_test == 0) &&
		(start->start_binary == T1REPLAY_PROCESS_REIIDEN) &&
		t1replay_bytes_zero(start->reserved, sizeof(start->reserved))
	);
}

static uint8_t t1replay_reiiden_group_mask(uint8_t group)
{
	switch(group) {
	case T1RIG_0: return T1REPLAY_INPUT_MASK_0;
	case T1RIG_3: return T1REPLAY_INPUT_MASK_3;
	case T1RIG_5: return T1REPLAY_INPUT_MASK_5;
	case T1RIG_6: return T1REPLAY_INPUT_MASK_6;
	case T1RIG_7: return T1REPLAY_INPUT_MASK_7;
	case T1RIG_8: return T1REPLAY_INPUT_MASK_8;
	case T1RIG_9: return T1REPLAY_INPUT_MASK_9;
	}
	return 0;
}

static bool t1replay_fuuin_keys_valid(const uint8_t far *keys)
{
	return (
		!(keys[T1RFIG_0] & ~T1REPLAY_INPUT_MASK_0) &&
		!(keys[T1RFIG_3] & ~T1REPLAY_INPUT_MASK_3) &&
		!(keys[T1RFIG_5] & ~T1REPLAY_INPUT_MASK_5) &&
		!(keys[T1RFIG_7] & ~T1REPLAY_INPUT_MASK_7) &&
		(keys[4] == 0) && (keys[5] == 0) && (keys[6] == 0)
	);
}

static void t1replay_stream_state_reset(
	t1replay_fuuin_stream_state_t far *state
)
{
	t1replay_memclear(state, sizeof(*state));
	state->process = T1REPLAY_PROCESS_REIIDEN;
}

static bool t1replay_packet_is_valid(
	const t1replay_packet_t far *packet,
	t1replay_fuuin_stream_state_t far *state
)
{
	uint8_t i;
	uint8_t control;
	uint8_t value;

	if(state->terminal_seen) {
		return false;
	}
	if(!(packet->tag & T1REPLAY_PACKET_CONTROL)) {
		if(state->process == T1REPLAY_PROCESS_REIIDEN) {
			for(i = 0; i < T1REPLAY_INPUT_GROUP_COUNT; i++) {
				if(packet->keys[i] & ~t1replay_reiiden_group_mask(i)) {
					return false;
				}
			}
		} else if(
			(state->process != T1REPLAY_PROCESS_FUUIN) ||
			(state->fuuin_phase == T1REPLAY_FUUIN_PHASE_NONE) ||
			!t1replay_fuuin_keys_valid(packet->keys)
		) {
			return false;
		}
		value = static_cast<uint8_t>(
			(packet->tag & T1REPLAY_PACKET_RUN_MASK) + 1
		);
		if(state->samples > (0xFFFFFFFFUL - static_cast<uint32_t>(value))) {
			return false;
		}
		state->samples += value;
		return true;
	}
	control = static_cast<uint8_t>(packet->tag & T1REPLAY_PACKET_RUN_MASK);
	if(
		(packet->keys[0] != state->process) ||
		(packet->keys[1] != state->process_seq) ||
		(packet->keys[3] != 0) || (packet->keys[4] != 0) ||
		(packet->keys[5] != 0) || (packet->keys[6] != 0)
	) {
		return false;
	}
	if(control == T1REPLAY_CONTROL_PROCESS_END) {
		if(
			(state->process != T1REPLAY_PROCESS_REIIDEN) ||
			((packet->keys[2] != T1REPLAY_PROCESS_REIIDEN) &&
			 (packet->keys[2] != T1REPLAY_PROCESS_FUUIN)) ||
			(state->process_seq == 0xFF)
		) {
			return false;
		}
		state->source_process = state->process;
		state->process = packet->keys[2];
		state->process_seq++;
		state->fuuin_phase = T1REPLAY_FUUIN_PHASE_NONE;
	} else if(control == T1REPLAY_CONTROL_PHASE) {
		value = packet->keys[2];
		if(
			(state->process != T1REPLAY_PROCESS_FUUIN) ||
			!(((state->fuuin_phase == T1REPLAY_FUUIN_PHASE_NONE) &&
			   (value == T1REPLAY_FUUIN_PHASE_VERDICT)) ||
			  ((state->fuuin_phase == T1REPLAY_FUUIN_PHASE_VERDICT) &&
			   ((value == T1REPLAY_FUUIN_PHASE_SCORE_NAME) ||
				(value == T1REPLAY_FUUIN_PHASE_SCORE_RELEASE))))
		) {
			return false;
		}
		state->fuuin_phase = value;
	} else if(control == T1REPLAY_CONTROL_TERMINAL) {
		value = packet->keys[2];
		if(
			!(((state->process == T1REPLAY_PROCESS_REIIDEN) &&
			   (value == T1REPLAY_END_MENU)) ||
			  ((state->process == T1REPLAY_PROCESS_FUUIN) &&
			   (state->fuuin_phase != T1REPLAY_FUUIN_PHASE_NONE) &&
			   (value == T1REPLAY_END_CLEAR)))
		) {
			return false;
		}
		state->terminal_seen = true;
		state->terminal_reason = value;
	} else {
		return false;
	}
	if(control != T1REPLAY_CONTROL_PHASE) {
		if(state->processes == 0xFFFFFFFFUL) {
			return false;
		}
		state->processes++;
	}
	return true;
}

static bool t1replay_stream_validate(int fd, bool finalized)
{
	uint32_t packets_seen = 0;
	uint32_t hash = T1REPLAY_FNV1A_BASIS;
	t1replay_fuuin_stream_state_t state;
	unsigned want;
	unsigned len;
	unsigned i;

	t1replay_stream_state_reset(&state);
	if(!t1replay_dos_seek(fd, t1replay_header.input_offset)) {
		return false;
	}
	while(packets_seen < t1replay_header.packet_count) {
		want = static_cast<unsigned>(
			((t1replay_header.packet_count - packets_seen) >
			 T1REPLAY_FUUIN_BUFFER_PACKET_COUNT) ?
				T1REPLAY_FUUIN_BUFFER_PACKET_COUNT :
				(t1replay_header.packet_count - packets_seen)
		);
		len = (want * T1REPLAY_PACKET_SIZE);
		if(t1replay_dos_read(fd, t1replay_buffer, len) != len) {
			return false;
		}
		hash = t1replay_fnv1a(hash, t1replay_buffer, len);
		for(i = 0; i < want; i++) {
			if(!t1replay_packet_is_valid(&t1replay_buffer[i], &state)) {
				return false;
			}
		}
		packets_seen += want;
	}
	return (
		(hash == t1replay_header.payload_checksum) &&
		(state.samples == t1replay_header.sample_count) &&
		(state.processes == t1replay_header.process_count) &&
		(finalized ?
			(state.terminal_seen &&
			 (state.terminal_reason == t1replay_header.end_reason)) :
			!state.terminal_seen)
	);
}

static bool t1replay_header_read(bool finalized)
{
	uint32_t file_size;
	uint32_t stored_checksum;
	uint32_t computed_checksum;
	int fd = t1replay_dos_open(t1replay_slot_fn, T1REPLAY_DOS_ACCESS_READ);

	if(fd < 0) {
		return false;
	}
	if((t1replay_dos_read(fd, &t1replay_header, sizeof(t1replay_header)) !=
		 sizeof(t1replay_header)) || !t1replay_dos_size(fd, &file_size)) {
		t1replay_dos_close(fd);
		return false;
	}
	stored_checksum = t1replay_header.header_checksum;
	t1replay_header.header_checksum = 0;
	computed_checksum = t1replay_fnv1a(
		T1REPLAY_FNV1A_BASIS, &t1replay_header, sizeof(t1replay_header)
	);
	t1replay_header.header_checksum = stored_checksum;
	if(
		!t1replay_magic_matches(t1replay_header.magic, '4') ||
		(t1replay_header.version != T1REPLAY_VERSION) ||
		(t1replay_header.header_size != T1REPLAY_HEADER_SIZE) ||
		(t1replay_header.packet_size != T1REPLAY_PACKET_SIZE) ||
		(t1replay_header.flags != T1REPLAY_FLAGS_KNOWN) ||
		(t1replay_header.status !=
			(finalized ? T1REPLAY_STATUS_FINALIZED : T1REPLAY_STATUS_RECORDING)) ||
		(t1replay_header.game_id != 1) ||
		(t1replay_header.input_semantics !=
			T1REPLAY_INPUT_SEMANTICS_LATCHED_GROUPS) ||
		(t1replay_header.input_offset != T1REPLAY_HEADER_SIZE) ||
		(t1replay_header.input_size > T1REPLAY_INPUT_SIZE_MAX) ||
		(t1replay_header.packet_count >
			(T1REPLAY_INPUT_SIZE_MAX / T1REPLAY_PACKET_SIZE)) ||
		(t1replay_header.input_size !=
			(t1replay_header.packet_count * T1REPLAY_PACKET_SIZE)) ||
		(file_size !=
			(t1replay_header.input_offset + t1replay_header.input_size)) ||
		(stored_checksum != computed_checksum) ||
		(t1replay_header.start_checksum != t1replay_fnv1a(
			T1REPLAY_FNV1A_BASIS, &t1replay_header.start,
			sizeof(t1replay_header.start)
		)) ||
		!t1replay_start_valid(&t1replay_header.start) ||
		!t1replay_name_valid(t1replay_header.name) ||
		!t1replay_summary_valid(
			&t1replay_header.summary, &t1replay_header.start,
			finalized, t1replay_header.end_reason
		) ||
		(t1replay_header.reserved_0 != 0) ||
		(finalized ?
			(t1replay_header.end_reason != T1REPLAY_END_CLEAR) :
			(t1replay_header.end_reason != 0)) ||
		!t1replay_stream_validate(fd, finalized)
	) {
		t1replay_dos_close(fd);
		return false;
	}
	t1replay_dos_close(fd);
	return true;
}

static bool t1replay_payload_prefix_valid(
	uint32_t length, uint32_t expected,
	t1replay_fuuin_stream_state_t far *state
)
{
	uint32_t hash = T1REPLAY_FNV1A_BASIS;
	uint32_t left = length;
	unsigned want;
	unsigned packets;
	unsigned i;
	int fd;

	if((length > t1replay_header.input_size) ||
		((length % T1REPLAY_PACKET_SIZE) != 0)) {
		return false;
	}
	t1replay_stream_state_reset(state);
	fd = t1replay_dos_open(t1replay_slot_fn, T1REPLAY_DOS_ACCESS_READ);
	if(fd < 0 || !t1replay_dos_seek(fd, t1replay_header.input_offset)) {
		if(fd >= 0) {
			t1replay_dos_close(fd);
		}
		return false;
	}
	while(left != 0) {
		want = static_cast<unsigned>(
			(left > sizeof(t1replay_buffer)) ? sizeof(t1replay_buffer) : left
		);
		if(t1replay_dos_read(fd, t1replay_buffer, want) != want) {
			t1replay_dos_close(fd);
			return false;
		}
		hash = t1replay_fnv1a(hash, t1replay_buffer, want);
		packets = (want / T1REPLAY_PACKET_SIZE);
		for(i = 0; i < packets; i++) {
			if(!t1replay_packet_is_valid(&t1replay_buffer[i], state)) {
				t1replay_dos_close(fd);
				return false;
			}
		}
		left -= want;
	}
	t1replay_dos_close(fd);
	return (hash == expected);
}

static uint32_t t1replay_res_checksum(void)
{
	uint32_t checksum;

	t1replay_res->checksum = 0;
	checksum = t1replay_fnv1a(
		T1REPLAY_FNV1A_BASIS, &t1replay_res->magic,
		(sizeof(*t1replay_res) - offsetof(t1replay_res_t, magic))
	);
	t1replay_res->checksum = checksum;
	return checksum;
}

static bool t1replay_res_valid(void)
{
	uint32_t stored;
	uint32_t computed;

	if(
		!t1replay_res ||
		(t1replay_res->magic[0] != 'T') ||
		(t1replay_res->magic[1] != '1') ||
		(t1replay_res->magic[2] != 'R') ||
		(t1replay_res->magic[3] != 'S') ||
		(t1replay_res->version != T1REPLAY_RES_VERSION) ||
		((t1replay_res->mode != T1RM_RECORD) &&
		 (t1replay_res->mode != T1RM_PLAYBACK)) ||
		!t1replay_slot_valid_for_mode(
			t1replay_res->mode, t1replay_res->slot
		) ||
		(t1replay_res->source_process != T1REPLAY_PROCESS_REIIDEN) ||
		(t1replay_res->target_process != T1REPLAY_PROCESS_FUUIN) ||
		(t1replay_res->process_seq == 0) ||
		!t1replay_bytes_zero(
			t1replay_res->reserved, sizeof(t1replay_res->reserved)
		)
	) {
		return false;
	}
	stored = t1replay_res->checksum;
	computed = t1replay_res_checksum();
	t1replay_res->checksum = stored;
	return (stored == computed);
}

static uint32_t t1replay_fuuin_handoff_checksum(
	const resident_t far *resident
)
{
	t1replay_fuuin_handoff_t handoff;
	int i;

	t1replay_memclear(&handoff, sizeof(handoff));
	handoff.resident_rand = resident->rand;
	handoff.score = resident->score;
	handoff.score_highest = resident->score_highest;
	handoff.continues_total = resident->continues_total;
	for(i = 0; i < SCENE_COUNT; i++) {
		handoff.continues_per_scene[i] = resident->continues_per_scene[i];
	}
	handoff.rank = resident->rank;
	handoff.credit_lives_extra = resident->credit_lives_extra;
	handoff.end_flag = static_cast<int8_t>(resident->end_flag);
	return t1replay_fnv1a(
		T1REPLAY_FNV1A_BASIS, &handoff, sizeof(handoff)
	);
}

static bool t1replay_res_matches_header(void)
{
	t1replay_fuuin_stream_state_t state;

	if(
		(t1replay_res->packet_count >
			(T1REPLAY_INPUT_SIZE_MAX / T1REPLAY_PACKET_SIZE)) ||
		(t1replay_res->input_size !=
			(t1replay_res->packet_count * T1REPLAY_PACKET_SIZE)) ||
		(t1replay_res->start_checksum != t1replay_header.start_checksum)
	) {
		return false;
	}
	if(t1replay_mode == T1RM_RECORD) {
		if(
			(t1replay_res->sample_count != t1replay_header.sample_count) ||
			(t1replay_res->packet_count != t1replay_header.packet_count) ||
			(t1replay_res->input_size != t1replay_header.input_size)
		) {
			return false;
		}
	} else if(
		(t1replay_res->sample_count > t1replay_header.sample_count) ||
		(t1replay_res->packet_count > t1replay_header.packet_count) ||
		(t1replay_res->input_size > t1replay_header.input_size)
	) {
		return false;
	}
	return (
		t1replay_payload_prefix_valid(
			t1replay_res->input_size, t1replay_res->payload_checksum, &state
		) &&
		!state.terminal_seen &&
		(state.samples == t1replay_res->sample_count) &&
		(state.process == T1REPLAY_PROCESS_FUUIN) &&
		(state.process_seq == t1replay_res->process_seq) &&
		(state.source_process == T1REPLAY_PROCESS_REIIDEN) &&
		(state.fuuin_phase == T1REPLAY_FUUIN_PHASE_NONE)
	);
}

static void t1replay_res_clear(void)
{
	if(t1replay_res) {
		unsigned segment = T1REPLAY_FP_SEG(t1replay_res);

		asm {
			push es
			mov es, segment
			mov ah, 49h
			int 21h
			pop es
		}
		t1replay_res = 0;
	}
}

static void t1replay_state_reset(void)
{
	t1replay_buffer_len = 0;
	t1replay_buffer_pos = 0;
	t1replay_payload_written = 0;
	t1replay_packet_cursor = 0;
	t1replay_sample_cursor = 0;
	t1replay_payload_checksum = T1REPLAY_FNV1A_BASIS;
	t1replay_pending_run = 0;
	t1replay_decode_run = 0;
	t1replay_pending_valid = false;
	t1replay_phase = T1REPLAY_FUUIN_PHASE_NONE;
	t1replay_memclear(t1replay_keys, sizeof(t1replay_keys));
#if T1REPLAY_FUUIN_SCORE_PROOF
	t1replay_memclear(&t1replay_score_proof, sizeof(t1replay_score_proof));
	t1replay_score_observation = T1RSO_NONE;
	t1replay_score_before_digest = 0;
	t1replay_score_after_digest = 0;
#endif
}

static void t1replay_header_checksum_set(void)
{
	t1replay_header.header_checksum = 0;
	t1replay_header.header_checksum = t1replay_fnv1a(
		T1REPLAY_FNV1A_BASIS, &t1replay_header, sizeof(t1replay_header)
	);
}

static bool t1replay_header_write(void)
{
	int fd = t1replay_dos_open(t1replay_slot_fn, T1REPLAY_DOS_ACCESS_RW);

	if(fd < 0) {
		return false;
	}
	t1replay_header.payload_checksum = t1replay_payload_checksum;
	t1replay_header_checksum_set();
	if(!t1replay_dos_seek(fd, 0) ||
		(t1replay_dos_write(fd, &t1replay_header, sizeof(t1replay_header)) !=
		 sizeof(t1replay_header))) {
		t1replay_dos_close(fd);
		return false;
	}
	t1replay_dos_close(fd);
	return true;
}

static bool t1replay_buffer_flush(void)
{
	unsigned len;
	int fd;

	if(t1replay_buffer_len == 0) {
		return true;
	}
	len = (t1replay_buffer_len * T1REPLAY_PACKET_SIZE);
	fd = t1replay_dos_open(t1replay_slot_fn, T1REPLAY_DOS_ACCESS_RW);
	if(fd < 0) {
		return false;
	}
	if(!t1replay_dos_seek(fd, t1replay_header.input_offset +
		t1replay_payload_written) ||
		(t1replay_dos_write(fd, t1replay_buffer, len) != len)) {
		t1replay_dos_close(fd);
		return false;
	}
	t1replay_dos_close(fd);
	t1replay_payload_written += len;
	t1replay_buffer_len = 0;
	return t1replay_header_write();
}

static bool t1replay_packet_commit(const t1replay_packet_t far *packet)
{
	if(t1replay_header.packet_count >=
		(T1REPLAY_INPUT_SIZE_MAX / T1REPLAY_PACKET_SIZE)) {
		return false;
	}
	t1replay_buffer[t1replay_buffer_len] = *packet;
	t1replay_buffer_len++;
	t1replay_header.packet_count++;
	t1replay_header.input_size += T1REPLAY_PACKET_SIZE;
	t1replay_payload_checksum = t1replay_fnv1a(
		t1replay_payload_checksum, packet, T1REPLAY_PACKET_SIZE
	);
	if(t1replay_buffer_len >= T1REPLAY_FUUIN_BUFFER_PACKET_COUNT) {
		return t1replay_buffer_flush();
	}
	return true;
}

static bool t1replay_pending_commit(void)
{
	if(!t1replay_pending_valid) {
		return true;
	}
	t1replay_pending.tag = static_cast<uint8_t>(t1replay_pending_run - 1);
	if(t1replay_header.sample_count >
		(0xFFFFFFFFUL - static_cast<uint32_t>(t1replay_pending_run))) {
		return false;
	}
	t1replay_header.sample_count += t1replay_pending_run;
	if(!t1replay_packet_commit(&t1replay_pending)) {
		return false;
	}
	t1replay_pending_valid = false;
	return true;
}

static bool t1replay_record_sample(void)
{
	uint8_t i;

	if(t1replay_pending_valid &&
		(t1replay_pending_run < T1REPLAY_PACKET_RUN_MAX)) {
		for(i = 0; i < T1REPLAY_INPUT_GROUP_COUNT; i++) {
			if(t1replay_pending.keys[i] != t1replay_keys[i]) {
				break;
			}
		}
		if(i == T1REPLAY_INPUT_GROUP_COUNT) {
			t1replay_pending_run++;
			return true;
		}
	}
	if(!t1replay_pending_commit()) {
		return false;
	}
	t1replay_pending_valid = true;
	t1replay_pending_run = 1;
	for(i = 0; i < T1REPLAY_INPUT_GROUP_COUNT; i++) {
		t1replay_pending.keys[i] = t1replay_keys[i];
	}
	return true;
}

static bool t1replay_packet_fetch(t1replay_packet_t far *packet)
{
	uint32_t remaining;
	unsigned want;
	unsigned len;
	int fd;

	if(t1replay_packet_cursor >= t1replay_header.packet_count) {
		return false;
	}
	if(t1replay_buffer_pos >= t1replay_buffer_len) {
		remaining = (t1replay_header.packet_count - t1replay_packet_cursor);
		want = static_cast<unsigned>(
			(remaining > T1REPLAY_FUUIN_BUFFER_PACKET_COUNT) ?
				T1REPLAY_FUUIN_BUFFER_PACKET_COUNT : remaining
		);
		len = (want * T1REPLAY_PACKET_SIZE);
		fd = t1replay_dos_open(t1replay_slot_fn, T1REPLAY_DOS_ACCESS_READ);
		if(fd < 0 || !t1replay_dos_seek(fd, t1replay_header.input_offset +
			(t1replay_packet_cursor * T1REPLAY_PACKET_SIZE)) ||
			(t1replay_dos_read(fd, t1replay_buffer, len) != len)) {
			if(fd >= 0) {
				t1replay_dos_close(fd);
			}
			return false;
		}
		t1replay_dos_close(fd);
		t1replay_buffer_len = want;
		t1replay_buffer_pos = 0;
	}
	*packet = t1replay_buffer[t1replay_buffer_pos++];
	t1replay_packet_cursor++;
	t1replay_payload_written += T1REPLAY_PACKET_SIZE;
	t1replay_payload_checksum = t1replay_fnv1a(
		t1replay_payload_checksum, packet, T1REPLAY_PACKET_SIZE
	);
	return true;
}

static bool t1replay_playback_sample(void)
{
	t1replay_packet_t packet;
	uint8_t i;

	if(t1replay_decode_run == 0) {
		if(!t1replay_packet_fetch(&packet) ||
			(packet.tag & T1REPLAY_PACKET_CONTROL)) {
			return false;
		}
		if(!t1replay_fuuin_keys_valid(packet.keys)) {
			return false;
		}
		for(i = 0; i < T1REPLAY_INPUT_GROUP_COUNT; i++) {
			t1replay_keys[i] = packet.keys[i];
		}
		t1replay_decode_run = static_cast<uint8_t>(
			(packet.tag & T1REPLAY_PACKET_RUN_MASK) + 1
		);
	}
	t1replay_decode_run--;
	t1replay_sample_cursor++;
	return true;
}

static bool t1replay_control_commit(uint8_t control, uint8_t value)
{
	t1replay_packet_t packet;

	if((control != T1REPLAY_CONTROL_PHASE) &&
		(t1replay_header.process_count == 0xFF)) {
		return false;
	}
	t1replay_memclear(&packet, sizeof(packet));
	packet.tag = static_cast<uint8_t>(T1REPLAY_PACKET_CONTROL | control);
	packet.keys[0] = T1REPLAY_PROCESS_FUUIN;
	packet.keys[1] = t1replay_res->process_seq;
	packet.keys[2] = value;
	if(!t1replay_packet_commit(&packet)) {
		return false;
	}
	if(control != T1REPLAY_CONTROL_PHASE) {
		t1replay_header.process_count++;
	}
	return true;
}

static bool t1replay_control_playback(uint8_t control, uint8_t value)
{
	t1replay_packet_t packet;

	if((t1replay_decode_run != 0) || !t1replay_packet_fetch(&packet)) {
		return false;
	}
	return (
		(packet.tag == static_cast<uint8_t>(T1REPLAY_PACKET_CONTROL | control)) &&
		(packet.keys[0] == T1REPLAY_PROCESS_FUUIN) &&
		(packet.keys[1] == t1replay_res->process_seq) &&
		(packet.keys[2] == value) &&
		(packet.keys[3] == 0) && (packet.keys[4] == 0) &&
		(packet.keys[5] == 0) && (packet.keys[6] == 0)
	);
}

static void t1replay_fail(void)
{
	if(t1replay_mode == T1RM_PLAYBACK) {
		t1replay_abort_pending = true;
	}
	if(t1replay_mode == T1RM_RECORD) {
		t1replay_header.status = T1REPLAY_STATUS_ERROR;
		t1replay_header_write();
	}
	t1replay_res_clear();
	t1replay_mode = T1RM_DISABLED;
}

static void t1replay_fail_and_abort_if_playback(void)
{
	t1replay_fail();
	if(t1replay_abort_pending) {
		t1replay_fuuin_abort_to_op();
		exit(1);
	}
}

#if T1REPLAY_FUUIN_SCORE_PROOF
void far t1replay_fuuin_score_table_unavailable(void)
{
	if(t1replay_mode == T1RM_DISABLED) {
		return;
	}
	if(
		(t1replay_phase != T1REPLAY_FUUIN_PHASE_VERDICT) ||
		(t1replay_score_observation != T1RSO_NONE) ||
		((t1replay_mode == T1RM_PLAYBACK) &&
		 (t1replay_score_proof.phase != T1REPLAY_FUUIN_PHASE_VERDICT))
	) {
		t1replay_fail_and_abort_if_playback();
		return;
	}
	t1replay_score_observation = T1RSO_UNAVAILABLE;
}

void far t1replay_fuuin_score_table_before(
	const void far *names,
	const void far *scores,
	const void far *stages,
	const void far *routes
)
{
	uint32_t digest;

	if(t1replay_mode == T1RM_DISABLED) {
		return;
	}
	digest = t1replay_score_table_digest(names, scores, stages, routes);
	if(
		(t1replay_phase != T1REPLAY_FUUIN_PHASE_VERDICT) ||
		(t1replay_score_observation != T1RSO_NONE) ||
		((t1replay_mode == T1RM_PLAYBACK) &&
		 ((t1replay_score_proof.phase == T1REPLAY_FUUIN_PHASE_VERDICT) ||
		  (t1replay_score_proof.before_digest != digest)))
	) {
		t1replay_fail_and_abort_if_playback();
		return;
	}
	t1replay_score_before_digest = digest;
	t1replay_score_observation = T1RSO_BEFORE;
}

void far t1replay_fuuin_score_table_after(
	const void far *names,
	const void far *scores,
	const void far *stages,
	const void far *routes
)
{
	uint32_t digest;

	if(t1replay_mode == T1RM_DISABLED) {
		return;
	}
	digest = t1replay_score_table_digest(names, scores, stages, routes);
	if(
		(t1replay_score_observation != T1RSO_BEFORE) ||
		((t1replay_phase != T1REPLAY_FUUIN_PHASE_SCORE_NAME) &&
		 (t1replay_phase != T1REPLAY_FUUIN_PHASE_SCORE_RELEASE)) ||
		((t1replay_mode == T1RM_PLAYBACK) &&
		 ((t1replay_score_proof.phase != t1replay_phase) ||
		  (t1replay_score_proof.after_digest != digest)))
	) {
		t1replay_fail_and_abort_if_playback();
		return;
	}
	t1replay_score_after_digest = digest;
	t1replay_score_observation = T1RSO_AFTER;
}
#endif

bool16 far t1replay_fuuin_entry(bool16 continuation_expected)
{
	char res_id[sizeof(T1REPLAY_RES_ID)];
	char resident_id[sizeof(RES_ID)];
	resident_t far *resident;

	t1replay_paths_init();
	t1replay_res_id_init(res_id);
	t1replay_resident_id_init(resident_id);
	t1replay_state_reset();
	t1replay_abort_pending = false;
	t1replay_mode = T1RM_DISABLED;
	t1replay_res = ResData<t1replay_res_t>::exist(res_id);
	if(!t1replay_res) {
		return !continuation_expected;
	}
	if(!continuation_expected) {
		t1replay_res_clear();
		return false;
	}
	resident = ResData<resident_t>::exist(resident_id);
	if(
		!resident || !t1replay_res_valid() ||
		(t1replay_res->handoff_checksum !=
			t1replay_fuuin_handoff_checksum(resident))
	) {
		t1replay_res_clear();
		return false;
	}
	t1replay_mode = static_cast<t1replay_mode_t>(t1replay_res->mode);
	t1replay_slot_set(t1replay_res->slot);
	if(
		!t1replay_header_read(t1replay_mode == T1RM_PLAYBACK) ||
		!t1replay_res_matches_header()
	) {
		t1replay_fail();
		return false;
	}
#if T1REPLAY_FUUIN_SCORE_PROOF
	if((t1replay_mode == T1RM_PLAYBACK) && !t1replay_score_proof_read()) {
		t1replay_fail();
		return false;
	}
#endif
	t1replay_payload_written = t1replay_res->input_size;
	t1replay_packet_cursor = t1replay_res->packet_count;
	t1replay_sample_cursor = t1replay_res->sample_count;
	t1replay_payload_checksum = t1replay_res->payload_checksum;
	return true;
}

void far t1replay_fuuin_input_reset(void)
{
	if(t1replay_mode != T1RM_DISABLED) {
		t1replay_memclear(t1replay_keys, sizeof(t1replay_keys));
	}
}

void far t1replay_fuuin_phase_begin(uint8_t phase)
{
	bool valid = (
		((t1replay_phase == T1REPLAY_FUUIN_PHASE_NONE) &&
		 (phase == T1REPLAY_FUUIN_PHASE_VERDICT)) ||
		((t1replay_phase == T1REPLAY_FUUIN_PHASE_VERDICT) &&
		 ((phase == T1REPLAY_FUUIN_PHASE_SCORE_NAME) ||
		  (phase == T1REPLAY_FUUIN_PHASE_SCORE_RELEASE)))
	);

	if(t1replay_mode == T1RM_DISABLED) {
		return;
	}
	if(!valid) {
		t1replay_fail_and_abort_if_playback();
		return;
	}
	if(t1replay_mode == T1RM_RECORD) {
		if(!t1replay_pending_commit() ||
			!t1replay_control_commit(T1REPLAY_CONTROL_PHASE, phase)) {
			t1replay_fail();
			return;
		}
	} else if(!t1replay_control_playback(T1REPLAY_CONTROL_PHASE, phase)) {
		t1replay_fail_and_abort_if_playback();
		return;
	}
	t1replay_phase = phase;
}

void far t1replay_fuuin_frame_io(void)
{
	if(t1replay_mode == T1RM_DISABLED) {
		return;
	}
	if(t1replay_phase == T1REPLAY_FUUIN_PHASE_NONE) {
		t1replay_fail_and_abort_if_playback();
		return;
	}
	if(t1replay_mode == T1RM_RECORD) {
		t1replay_keys[T1RFIG_0] = static_cast<uint8_t>(
			(key_sense(0) | key_sense(0)) & T1REPLAY_INPUT_MASK_0
		);
		t1replay_keys[T1RFIG_3] = static_cast<uint8_t>(
			(key_sense(3) | key_sense(3)) & T1REPLAY_INPUT_MASK_3
		);
		t1replay_keys[T1RFIG_5] = static_cast<uint8_t>(
			(key_sense(5) | key_sense(5)) & T1REPLAY_INPUT_MASK_5
		);
		t1replay_keys[T1RFIG_7] = static_cast<uint8_t>(
			(key_sense(7) | key_sense(7)) & T1REPLAY_INPUT_MASK_7
		);
		if(!t1replay_record_sample()) {
			t1replay_fail();
		}
	} else if(!t1replay_playback_sample()) {
		t1replay_fail_and_abort_if_playback();
	}
}

int far t1replay_fuuin_key_sense(int keygroup)
{
	if(t1replay_abort_pending) {
		return 0;
	}
	if(t1replay_mode == T1RM_DISABLED) {
		return key_sense(keygroup);
	}
	switch(keygroup) {
	case 0: return t1replay_keys[T1RFIG_0];
	case 3: return t1replay_keys[T1RFIG_3];
	case 5: return t1replay_keys[T1RFIG_5];
	case 7: return t1replay_keys[T1RFIG_7];
	}
	// Playback must never fall through to a physical group.
	return (t1replay_mode == T1RM_PLAYBACK) ? 0 : key_sense(keygroup);
}

static bool t1replay_clear_summary_finalize(void)
{
	t1replay_summary_t far *summary = &t1replay_header.summary;

	if(t1replay_mode == T1RM_RECORD) {
		if(
			(summary->split_count == 0) ||
			(summary->splits[summary->split_count - 1].stage_id !=
			 (STAGE_COUNT - 1)) ||
			(summary->splits[summary->split_count - 1].score != score) ||
			(summary->splits[summary->split_count - 1].flags !=
			 T1REPLAY_STAGE_FLAGS_KNOWN)
		) {
			return false;
		}
		summary->final_score = score;
		summary->final_stage_id = (STAGE_COUNT - 1);
		summary->terminal_reason = T1REPLAY_END_CLEAR;
	}
	return (
		(summary->final_score == score) &&
		t1replay_summary_valid(
			summary, &t1replay_header.start, true, T1REPLAY_END_CLEAR
		)
	);
}

void far t1replay_fuuin_terminal(void)
{
	if(t1replay_mode == T1RM_DISABLED) {
		return;
	}
#if T1REPLAY_FUUIN_SCORE_PROOF
	if(!t1replay_score_observation_complete()) {
		t1replay_fail_and_abort_if_playback();
		return;
	}
#endif
	if(!t1replay_clear_summary_finalize()) {
		t1replay_fail_and_abort_if_playback();
		return;
	}
	if(t1replay_mode == T1RM_RECORD) {
		if(!t1replay_pending_commit() ||
			!t1replay_control_commit(
				T1REPLAY_CONTROL_TERMINAL, T1REPLAY_END_CLEAR
			) ||
			!t1replay_buffer_flush()) {
			t1replay_fail();
			return;
		}
		t1replay_header.status = T1REPLAY_STATUS_FINALIZED;
		t1replay_header.end_reason = T1REPLAY_END_CLEAR;
		if(!t1replay_header_write()) {
			t1replay_fail();
			return;
		}
#if T1REPLAY_FUUIN_SCORE_PROOF
		if(!t1replay_score_proof_write()) {
			t1replay_fail();
			return;
		}
#endif
		if(
			t1replay_slot_is_pending(t1replay_res->slot) &&
			!t1replay_save_request_write(T1RSRS_POSTGAME)
		) {
			t1replay_fail();
			return;
		}
	} else if(
		!t1replay_control_playback(
			T1REPLAY_CONTROL_TERMINAL, T1REPLAY_END_CLEAR
		) ||
		(t1replay_packet_cursor != t1replay_header.packet_count) ||
		(t1replay_sample_cursor != t1replay_header.sample_count) ||
		(t1replay_payload_written != t1replay_header.input_size) ||
		(t1replay_payload_checksum != t1replay_header.payload_checksum)
	) {
		t1replay_fail_and_abort_if_playback();
		return;
	}
	t1replay_res_clear();
	t1replay_mode = T1RM_DISABLED;
}

bool16 far t1replay_fuuin_active(void)
{
	return (t1replay_mode != T1RM_DISABLED);
}

bool16 far t1replay_fuuin_playback(void)
{
	return (t1replay_mode == T1RM_PLAYBACK);
}

#pragma codeseg
