/* TH01 OP replay and Practice surface.
 *
 * This module deliberately owns only title-side file inspection and transient
 * Practice selection. REIIDEN validates complete replay payloads and remains
 * the only process that can enter playback.
 */

#pragma option -zCT1REPLAY_OP_TEXT -G-

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform/x86real/pc98/keyboard.hpp"
#include "th01/common.h"
#include "th01/hardware/egc.h"
#include "th01/hardware/frmdelay.h"
#include "th01/hardware/graph.h"
#include "th01/hardware/grppsafx.h"
#include "th01/hardware/grp_text.hpp"
#include "th01/replay_op.hpp"
#include "th01/resident.hpp"
#include "th01/rank.h"

static const screen_x_t T1REPLAY_OP_LEFT = 152;
static const pixel_t T1REPLAY_OP_W = 336;
static const screen_y_t T1REPLAY_OP_TOP = 96;
static const pixel_t T1REPLAY_OP_H = 240;
static const screen_x_t T1REPLAY_OP_VALUE_LEFT = 344;
static const screen_y_t T1REPLAY_OP_LINE_H = GLYPH_H;
static const uint8_t T1REPLAY_OP_ROWS_PER_PAGE = 10;
static const vc_t T1REPLAY_OP_COL_LABEL = 5;
static const vc_t T1REPLAY_OP_COL_VALUE = 15;
static const vc_t T1REPLAY_OP_COL_DISABLED = 3;
static const int16_t T1REPLAY_OP_FX = FX_WEIGHT_BLACK;
static const uint8_t T1REPLAY_OP_POINT_VALUE_COUNT = 17;
static const uint16_t T1REPLAY_OP_POINT_CAP = 65530;
static const uint8_t T1REPLAY_OP_PRACTICE_ROW_COUNT = 12;

enum t1replay_op_practice_row_t {
	T1OPR_SCENE,
	T1OPR_ROUTE,
	T1OPR_SECTION,
	T1OPR_CHAPTER,
	T1OPR_SCORE,
	T1OPR_LIVES,
	T1OPR_BOMBS,
	T1OPR_POINT_VALUE,
	T1OPR_PELLET_SPEED,
	T1OPR_RNG_SEED,
	T1OPR_START,
	T1OPR_BACK,
};

struct t1replay_op_slot_t {
	bool exists;
	bool valid;
	t1replay_header_t header;
};

struct t1replay_op_input_t {
	bool up;
	bool down;
	bool left;
	bool right;
	bool ok;
	bool enter;
	bool cancel;
	bool left_held;
	bool right_held;
};

static uint8_t t1replay_op_sel;
static uint8_t t1replay_op_page;
static bool t1replay_op_wait_release;
static t1replay_practice_start_t t1replay_practice_start;
static bool t1replay_op_prev_up;
static bool t1replay_op_prev_down;
static bool t1replay_op_prev_left;
static bool t1replay_op_prev_right;
static bool t1replay_op_prev_ok;
static bool t1replay_op_prev_enter;
static bool t1replay_op_prev_cancel;
static uint8_t t1replay_op_horizontal_hold;
static bool t1replay_op_save_pending;

static uint32_t t1replay_op_fnv1a(
	uint32_t hash, const void *buf, unsigned size
);
static bool t1replay_op_bytes_zero(const uint8_t *p, unsigned size);

static void t1replay_op_slot_fn(char *fn, uint8_t slot)
{
	fn[0] = 'T'; fn[1] = 'H'; fn[2] = '1'; fn[3] = 'R';
	fn[4] = static_cast<char>('0' + (slot / 10));
	fn[5] = static_cast<char>('0' + (slot % 10));
	fn[6] = '.'; fn[7] = 'R'; fn[8] = 'P'; fn[9] = 'Y'; fn[10] = '\0';
}

static void t1replay_op_command_fn(char *fn)
{
	fn[0] = 'T'; fn[1] = '1'; fn[2] = 'R'; fn[3] = 'P'; fn[4] = 'Y';
	fn[5] = '.'; fn[6] = 'C'; fn[7] = 'F'; fn[8] = 'G'; fn[9] = '\0';
}

static void t1replay_op_pending_fn(char *fn)
{
	fn[0] = 'T'; fn[1] = '1'; fn[2] = 'R'; fn[3] = 'P'; fn[4] = 'Y';
	fn[5] = '.'; fn[6] = 'T'; fn[7] = 'M'; fn[8] = 'P'; fn[9] = '\0';
}

static bool t1replay_op_file_exists(const char *fn)
{
	char mode[3];
	FILE *fp;

	mode[0] = 'r'; mode[1] = 'b'; mode[2] = '\0';
	fp = fopen(fn, mode);
	if(!fp) {
		return false;
	}
	fclose(fp);
	return true;
}

#if T1REPLAY_CHECKPOINT_EMIT || T1REPLAY_CHECKPOINT_RESTORE
static bool t1replay_op_checkpoint_fn(
	char *fn, uint8_t slot, uint8_t process_seq
)
{
	if(process_seq > T1REPLAY_CHECKPOINT_PROCESS_MAX) {
		return false;
	}
	fn[0] = 'T'; fn[1] = '1'; fn[2] = 'C';
	if(t1replay_slot_is_pending(slot)) {
		fn[3] = 'P'; fn[4] = 'T';
	} else if(t1replay_slot_is_numbered(slot)) {
		fn[3] = static_cast<char>('0' + (slot / 10));
		fn[4] = static_cast<char>('0' + (slot % 10));
	} else {
		return false;
	}
	fn[5] = static_cast<char>('0' + (process_seq / 10));
	fn[6] = static_cast<char>('0' + (process_seq % 10));
	fn[7] = '.'; fn[8] = 'C'; fn[9] = 'K'; fn[10] = 'P'; fn[11] = '\0';
	return true;
}

static void t1op_ckpt_pending_discard(void)
{
	char fn[12];
	uint8_t process_seq;

	for(process_seq = 0; process_seq <= T1REPLAY_CHECKPOINT_PROCESS_MAX;
		process_seq++) {
		t1replay_op_checkpoint_fn(fn, T1REPLAY_SLOT_PENDING, process_seq);
		remove(fn);
	}
}

static bool t1op_ckpt_destinations_empty(uint8_t slot)
{
	char fn[12];
	uint8_t process_seq;

	for(process_seq = 0; process_seq <= T1REPLAY_CHECKPOINT_PROCESS_MAX;
		process_seq++) {
		t1replay_op_checkpoint_fn(fn, slot, process_seq);
		if(t1replay_op_file_exists(fn)) {
			return false;
		}
	}
	return true;
}

static void t1op_ckpt_pending_rollback(uint8_t slot)
{
	char pending_fn[12];
	char destination_fn[12];
	uint8_t process_seq;

	for(process_seq = 0; process_seq <= T1REPLAY_CHECKPOINT_PROCESS_MAX;
		process_seq++) {
		t1replay_op_checkpoint_fn(
			pending_fn, T1REPLAY_SLOT_PENDING, process_seq
		);
		t1replay_op_checkpoint_fn(destination_fn, slot, process_seq);
		if(!t1replay_op_file_exists(pending_fn) &&
			t1replay_op_file_exists(destination_fn)) {
			rename(destination_fn, pending_fn);
		}
	}
}

static bool t1op_ckpt_pending_stage(uint8_t slot)
{
	char pending_fn[12];
	char destination_fn[12];
	uint8_t process_seq;

	for(process_seq = 0; process_seq <= T1REPLAY_CHECKPOINT_PROCESS_MAX;
		process_seq++) {
		t1replay_op_checkpoint_fn(
			pending_fn, T1REPLAY_SLOT_PENDING, process_seq
		);
		t1replay_op_checkpoint_fn(destination_fn, slot, process_seq);
		if(t1replay_op_file_exists(pending_fn) &&
			(rename(pending_fn, destination_fn) != 0)) {
			t1op_ckpt_pending_rollback(slot);
			return false;
		}
	}
	return true;
}
#endif

#if T1REPLAY_FUUIN_SCORE_PROOF
static void t1replay_op_score_proof_fn(char *fn, uint8_t slot)
{
	fn[0] = 'T'; fn[1] = '1'; fn[2] = 'S';
	if(t1replay_slot_is_pending(slot)) {
		fn[3] = 'P'; fn[4] = '.';
		fn[5] = 'D'; fn[6] = 'I'; fn[7] = 'G'; fn[8] = '\0';
		return;
	}
	fn[3] = static_cast<char>('0' + (slot / 10));
	fn[4] = static_cast<char>('0' + (slot % 10));
	fn[5] = '.'; fn[6] = 'D'; fn[7] = 'I'; fn[8] = 'G'; fn[9] = '\0';
}

static uint32_t t1replay_op_score_proof_checksum(t1replay_score_proof_t *proof)
{
	uint32_t checksum;

	proof->container_checksum = 0;
	checksum = t1replay_op_fnv1a(
		T1REPLAY_FNV1A_BASIS, proof, sizeof(*proof)
	);
	proof->container_checksum = checksum;
	return checksum;
}

static bool t1replay_op_score_proof_valid(
	t1replay_score_proof_t *proof, const t1replay_header_t *header
)
{
	uint32_t stored_checksum = proof->container_checksum;

	if(stored_checksum != t1replay_op_score_proof_checksum(proof)) {
		return false;
	}
	return (
		(proof->magic[0] == 'T') && (proof->magic[1] == '1') &&
		(proof->magic[2] == 'S') && (proof->magic[3] == 'D') &&
		(proof->magic[4] == 'G') && (proof->magic[5] == '1') &&
		(proof->magic[6] == '\0') && (proof->magic[7] == '\0') &&
		(proof->schema == T1REPLAY_SCORE_PROOF_SCHEMA) &&
		(proof->size == T1REPLAY_SCORE_PROOF_SIZE) &&
		(proof->game_id == 1) &&
		(proof->slot == T1REPLAY_SLOT_PENDING) &&
		(proof->rank == header->start.rank) &&
		((proof->phase == T1REPLAY_FUUIN_PHASE_VERDICT) ||
		 (proof->phase == T1REPLAY_FUUIN_PHASE_SCORE_NAME) ||
		 (proof->phase == T1REPLAY_FUUIN_PHASE_SCORE_RELEASE)) &&
		(proof->replay_start_checksum == header->start_checksum) &&
		(proof->replay_payload_checksum == header->payload_checksum) &&
		(proof->replay_sample_count == header->sample_count) &&
		(proof->replay_packet_count == header->packet_count) &&
		(((proof->phase == T1REPLAY_FUUIN_PHASE_VERDICT) &&
		  (proof->before_digest == 0) && (proof->after_digest == 0)) ||
		 ((proof->phase != T1REPLAY_FUUIN_PHASE_VERDICT))) &&
		t1replay_op_bytes_zero(proof->reserved, sizeof(proof->reserved))
	);
}

static bool t1replay_op_score_proof_read(
	t1replay_score_proof_t *proof, const t1replay_header_t *header
)
{
	char fn[10];
	char mode[3];
	FILE *fp;
	long size;

	t1replay_op_score_proof_fn(fn, T1REPLAY_SLOT_PENDING);
	mode[0] = 'r'; mode[1] = 'b'; mode[2] = '\0';
	fp = fopen(fn, mode);
	if(!fp || (fseek(fp, 0L, SEEK_END) != 0) ||
		((size = ftell(fp)) != sizeof(*proof)) ||
		(fseek(fp, 0L, SEEK_SET) != 0) ||
		(fread(proof, 1, sizeof(*proof), fp) != sizeof(*proof))) {
		if(fp) {
			fclose(fp);
		}
		return false;
	}
	fclose(fp);
	return t1replay_op_score_proof_valid(proof, header);
}

static bool t1replay_op_score_proof_write(
	const t1replay_score_proof_t *proof, uint8_t slot
)
{
	char fn[10];
	char mode[3];
	FILE *fp;
	bool ok;

	t1replay_op_score_proof_fn(fn, slot);
	mode[0] = 'w'; mode[1] = 'b'; mode[2] = '\0';
	fp = fopen(fn, mode);
	if(!fp) {
		return false;
	}
	ok = (fwrite(proof, 1, sizeof(*proof), fp) == sizeof(*proof));
	if(fclose(fp) != 0) {
		ok = false;
	}
	if(!ok) {
		remove(fn);
	}
	return ok;
}

static void t1replay_op_pending_score_proof_discard(void)
{
	char fn[10];

	t1replay_op_score_proof_fn(fn, T1REPLAY_SLOT_PENDING);
	remove(fn);
}

static bool t1replay_op_score_proof_destination_empty(uint8_t slot)
{
	char fn[10];

	t1replay_op_score_proof_fn(fn, slot);
	return !t1replay_op_file_exists(fn);
}

static bool t1replay_op_pending_score_proof_valid(
	const t1replay_op_slot_t& pending
)
{
	t1replay_score_proof_t proof;

	if(pending.header.end_reason != T1REPLAY_END_CLEAR) {
		return true;
	}
	return t1replay_op_score_proof_read(&proof, &pending.header);
}
#endif

static uint32_t t1replay_op_fnv1a(uint32_t hash, const void *buf, unsigned size)
{
	const uint8_t *p = reinterpret_cast<const uint8_t *>(buf);

	while(size != 0) {
		hash ^= *p++;
		hash *= T1REPLAY_FNV1A_PRIME;
		size--;
	}
	return hash;
}

static bool t1replay_op_bytes_zero(const uint8_t *p, unsigned size)
{
	while(size != 0) {
		if(*p++ != 0) {
			return false;
		}
		size--;
	}
	return true;
}

static bool t1replay_op_magic_matches(const char *magic, char last)
{
	return (
		(magic[0] == 'T') && (magic[1] == '1') &&
		(magic[2] == 'R') && (magic[3] == 'P') &&
		(magic[4] == 'Y') && (magic[5] == last) &&
		(magic[6] == '\0') && (magic[7] == '\0')
	);
}

static bool t1replay_op_start_valid(const t1replay_start_t *start)
{
	return (
		(start->stage_id < STAGE_COUNT) &&
		(start->rank >= 0) && (start->rank <= RANK_LUNATIC) &&
		(start->bgm_mode >= BGM_MODE_OFF) && (start->bgm_mode < BGM_MODE_COUNT) &&
		(start->route >= ROUTE_MAKAI) && (start->route < ROUTE_COUNT) &&
		(start->end_flag >= ES_NONE) && (start->end_flag <= ES_JIGOKU) &&
		(start->debug_mode == DM_OFF) &&
		(start->snd_need_init >= 0) && (start->snd_need_init <= 1) &&
		(start->mode_test == 0) &&
		(start->start_binary == T1REPLAY_PROCESS_REIIDEN) &&
		t1replay_op_bytes_zero(start->reserved, sizeof(start->reserved))
	);
}

static bool t1replay_op_header_valid(t1replay_header_t *header)
{
	uint32_t stored_header_checksum = header->header_checksum;
	uint32_t header_checksum;
	uint32_t start_checksum;

	if(
		!t1replay_op_magic_matches(header->magic, '2') ||
		(header->version != T1REPLAY_VERSION) ||
		(header->header_size != T1REPLAY_HEADER_SIZE) ||
		(header->packet_size != T1REPLAY_PACKET_SIZE) ||
		(header->flags != T1REPLAY_FLAGS_KNOWN) ||
		(header->status != T1REPLAY_STATUS_FINALIZED) ||
		((header->end_reason != T1REPLAY_END_MENU) &&
		 (header->end_reason != T1REPLAY_END_CLEAR)) ||
		(header->game_id != 1) ||
		(header->input_semantics != T1REPLAY_INPUT_SEMANTICS_LATCHED_GROUPS) ||
		(header->process_count == 0) ||
		(header->reserved_0 != 0) ||
		(header->input_offset != T1REPLAY_HEADER_SIZE) ||
		(header->packet_count > (T1REPLAY_INPUT_SIZE_MAX / T1REPLAY_PACKET_SIZE)) ||
		(header->input_size != (header->packet_count * T1REPLAY_PACKET_SIZE)) ||
		(header->input_size > T1REPLAY_INPUT_SIZE_MAX) ||
		!t1replay_op_bytes_zero(header->reserved, sizeof(header->reserved)) ||
		!t1replay_op_start_valid(&header->start)
	) {
		return false;
	}
	start_checksum = t1replay_op_fnv1a(
		T1REPLAY_FNV1A_BASIS, &header->start, sizeof(header->start)
	);
	if(start_checksum != header->start_checksum) {
		return false;
	}
	header->header_checksum = 0;
	header_checksum = t1replay_op_fnv1a(
		T1REPLAY_FNV1A_BASIS, header, sizeof(*header)
	);
	header->header_checksum = stored_header_checksum;
	return (header_checksum == stored_header_checksum);
}

static uint8_t t1replay_op_group_mask(uint8_t group)
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

static bool t1replay_op_fuuin_keys_valid(const uint8_t *keys)
{
	return (
		!(keys[T1RFIG_0] & ~T1REPLAY_INPUT_MASK_0) &&
		!(keys[T1RFIG_3] & ~T1REPLAY_INPUT_MASK_3) &&
		!(keys[T1RFIG_5] & ~T1REPLAY_INPUT_MASK_5) &&
		!(keys[T1RFIG_7] & ~T1REPLAY_INPUT_MASK_7) &&
		(keys[4] == 0) && (keys[5] == 0) && (keys[6] == 0)
	);
}

struct t1replay_op_stream_state_t {
	uint32_t samples;
	uint32_t processes;
	uint8_t process;
	uint8_t process_seq;
	uint8_t source_process;
	uint8_t fuuin_phase;
	uint8_t terminal_reason;
	bool terminal_seen;
};

static void t1replay_op_stream_state_reset(t1replay_op_stream_state_t *state)
{
	memset(state, 0, sizeof(*state));
	state->process = T1REPLAY_PROCESS_REIIDEN;
}

static bool t1replay_op_packet_valid(
	const t1replay_packet_t *packet, t1replay_op_stream_state_t *state
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
				if(packet->keys[i] & ~t1replay_op_group_mask(i)) {
					return false;
				}
			}
		} else if(
			(state->process != T1REPLAY_PROCESS_FUUIN) ||
			(state->fuuin_phase == T1REPLAY_FUUIN_PHASE_NONE) ||
			!t1replay_op_fuuin_keys_valid(packet->keys)
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

static bool t1replay_op_stream_valid(FILE *fp, const t1replay_header_t *header)
{
	uint32_t packets_seen = 0;
	uint32_t hash = T1REPLAY_FNV1A_BASIS;
	t1replay_op_stream_state_t state;
	t1replay_packet_t packet;
	long file_size;

	if(
		(fseek(fp, 0L, SEEK_END) != 0) ||
		((file_size = ftell(fp)) < 0) ||
		(static_cast<uint32_t>(file_size) !=
			(header->input_offset + header->input_size)) ||
		(fseek(fp, static_cast<long>(header->input_offset), SEEK_SET) != 0)
	) {
		return false;
	}
	t1replay_op_stream_state_reset(&state);
	while(packets_seen < header->packet_count) {
		if(fread(&packet, 1, sizeof(packet), fp) != sizeof(packet)) {
			return false;
		}
		hash = t1replay_op_fnv1a(hash, &packet, sizeof(packet));
		if(!t1replay_op_packet_valid(&packet, &state)) {
			return false;
		}
		packets_seen++;
	}
	return (
		(hash == header->payload_checksum) &&
		(state.samples == header->sample_count) &&
		(state.processes == header->process_count) &&
		state.terminal_seen &&
		(state.terminal_reason == header->end_reason)
	);
}

static void t1replay_op_file_read(
	const char *fn, t1replay_op_slot_t& result, bool deep
)
{
	char mode[3];
	FILE *fp;

	memset(&result, 0, sizeof(result));
	mode[0] = 'r'; mode[1] = 'b'; mode[2] = '\0';
	fp = fopen(fn, mode);
	if(!fp) {
		return;
	}
	result.exists = true;
	if(fread(&result.header, 1, sizeof(result.header), fp) == sizeof(result.header)) {
		result.valid = t1replay_op_header_valid(&result.header);
		if(result.valid && deep) {
			result.valid = t1replay_op_stream_valid(fp, &result.header);
		}
	}
	fclose(fp);
}

static void t1replay_op_slot_read(uint8_t slot, t1replay_op_slot_t& result)
{
	char fn[11];

	memset(&result, 0, sizeof(result));
	if(!t1replay_slot_is_numbered(slot)) {
		return;
	}
	t1replay_op_slot_fn(fn, slot);
	t1replay_op_file_read(fn, result, false);
}

static void t1replay_op_pending_read(t1replay_op_slot_t& result)
{
	char fn[10];

	t1replay_op_pending_fn(fn);
	t1replay_op_file_read(fn, result, true);
}

static void t1replay_op_pending_discard(void)
{
	char fn[10];

	t1replay_op_pending_fn(fn);
	remove(fn);
#if T1REPLAY_CHECKPOINT_EMIT || T1REPLAY_CHECKPOINT_RESTORE
	t1op_ckpt_pending_discard();
#endif
#if T1REPLAY_FUUIN_SCORE_PROOF
	t1replay_op_pending_score_proof_discard();
#endif
}

static bool t1replay_op_command_write(uint8_t mode, uint8_t slot)
{
	t1replay_command_t command;
	char fn[10];
	char fopen_mode[3];
	FILE *fp;
	bool ok;

	if(
		((mode != T1REPLAY_COMMAND_RECORD) &&
		 (mode != T1REPLAY_COMMAND_PLAYBACK)) ||
		!t1replay_slot_valid_for_mode(mode, slot)
	) {
		return false;
	}
	memset(&command, 0, sizeof(command));
	command.magic[0] = 'T'; command.magic[1] = '1';
	command.magic[2] = 'R'; command.magic[3] = 'P';
	command.magic[4] = 'Y'; command.magic[5] = 'C';
	command.mode = mode;
	command.slot = slot;
	t1replay_op_command_fn(fn);
	fopen_mode[0] = 'w'; fopen_mode[1] = 'b'; fopen_mode[2] = '\0';
	fp = fopen(fn, fopen_mode);
	if(!fp) {
		return false;
	}
	ok = (fwrite(&command, 1, sizeof(command), fp) == sizeof(command));
	if(fclose(fp) != 0) {
		ok = false;
	}
	if(!ok) {
		remove(fn);
	}
	return ok;
}

#if T1REPLAY_EXACT_TRACE
bool t1replay_op_exact_bootstrap(void)
{
	uint8_t config[8];
	uint8_t checksum = 0;
	uint8_t extra;
	char fn[12];
	char mode[3];
	FILE *fp;
	bool valid;
	int i;

	fn[0] = 'T'; fn[1] = '1'; fn[2] = 'E'; fn[3] = 'X';
	fn[4] = 'A'; fn[5] = 'C'; fn[6] = 'T'; fn[7] = '.';
	fn[8] = 'C'; fn[9] = 'F'; fn[10] = 'G'; fn[11] = '\0';
	mode[0] = 'r'; mode[1] = 'b'; mode[2] = '\0';
	fp = fopen(fn, mode);
	if(!fp) {
		return false;
	}
	valid = (
		(fread(config, 1, sizeof(config), fp) == sizeof(config)) &&
		(fread(&extra, 1, 1, fp) == 0)
	);
	fclose(fp);
	remove(fn);
	if(!valid) {
		return false;
	}
	for(i = 0; i < 7; i++) {
		checksum ^= config[i];
	}
	if(
		(config[0] != 'T') || (config[1] != '1') ||
		(config[2] != 'E') || (config[3] != 'X') ||
		(config[4] != 1) ||
		(config[5] != T1REPLAY_COMMAND_PLAYBACK) ||
		(config[6] >= T1REPLAY_SLOT_COUNT) ||
		(config[7] != checksum)
	) {
		return false;
	}
	return t1replay_op_command_write(config[5], config[6]);
}
#endif

bool t1replay_op_record_prepare(void)
{
	// The command is one-shot process control, never durable replay state. A
	// new capture always supersedes an abandoned temporary, while the full
	// numbered slot set remains available to the post-run picker.
	t1replay_op_command_clear();
	t1replay_op_pending_discard();
	if(!t1replay_op_command_write(
		T1REPLAY_COMMAND_RECORD, T1REPLAY_SLOT_PENDING
	)) {
		t1replay_op_command_clear();
		return false;
	}
	return true;
}

static bool t1replay_op_pending_commit(uint8_t slot)
{
	t1replay_op_slot_t pending;
	t1replay_op_slot_t destination;
	char pending_fn[10];
	char destination_fn[11];
#if T1REPLAY_FUUIN_SCORE_PROOF
	t1replay_score_proof_t proof;
	bool proof_staged = false;
#endif

	if(!t1replay_slot_is_numbered(slot)) {
		return false;
	}
	// The same complete validation gate applies at picker entry and commit.
	// This prevents a corrupt or externally replaced temporary from becoming
	// a numbered replay between those two title frames.
	t1replay_op_pending_read(pending);
	if(!pending.exists || !pending.valid) {
		return false;
	}
#if T1REPLAY_FUUIN_SCORE_PROOF
	if(!t1replay_op_pending_score_proof_valid(pending)) {
		return false;
	}
#endif
	t1replay_op_slot_read(slot, destination);
	if(
		destination.exists ||
#if T1REPLAY_CHECKPOINT_EMIT || T1REPLAY_CHECKPOINT_RESTORE
		!t1op_ckpt_destinations_empty(slot) ||
#endif
#if T1REPLAY_FUUIN_SCORE_PROOF
		!t1replay_op_score_proof_destination_empty(slot) ||
#endif
		false
	) {
		return false;
	}
#if T1REPLAY_FUUIN_SCORE_PROOF
	if(pending.header.end_reason == T1REPLAY_END_CLEAR) {
		// A private score-proof build accepts a clear only if the temporary
		// sidecar binds to this exact finalized pending stream. Rewrite its
		// slot field before committing the replay so later playback cannot
		// consume evidence for the pending sentinel.
		if(!t1replay_op_score_proof_read(&proof, &pending.header)) {
			return false;
		}
		proof.slot = slot;
		t1replay_op_score_proof_checksum(&proof);
		if(!t1replay_op_score_proof_write(&proof, slot)) {
			return false;
		}
		proof_staged = true;
	} else {
		// A non-clear run cannot legitimately own a score-table proof.
		t1replay_op_pending_score_proof_discard();
	}
#endif
#if T1REPLAY_CHECKPOINT_EMIT || T1REPLAY_CHECKPOINT_RESTORE
	if(!t1op_ckpt_pending_stage(slot)) {
#if T1REPLAY_FUUIN_SCORE_PROOF
		if(proof_staged) {
			t1replay_op_score_proof_fn(destination_fn, slot);
			remove(destination_fn);
		}
#endif
		return false;
	}
#endif
	t1replay_op_pending_fn(pending_fn);
	t1replay_op_slot_fn(destination_fn, slot);
	if(rename(pending_fn, destination_fn) != 0) {
#if T1REPLAY_CHECKPOINT_EMIT || T1REPLAY_CHECKPOINT_RESTORE
		t1op_ckpt_pending_rollback(slot);
#endif
#if T1REPLAY_FUUIN_SCORE_PROOF
		if(proof_staged) {
			t1replay_op_score_proof_fn(destination_fn, slot);
			remove(destination_fn);
		}
#endif
		return false;
	}
#if T1REPLAY_FUUIN_SCORE_PROOF
	if(proof_staged) {
		t1replay_op_pending_score_proof_discard();
	}
#endif
	return true;
}

static void t1replay_op_backing_restore(void)
{
	egc_copy_rect_1_to_0_16(
		T1REPLAY_OP_LEFT, T1REPLAY_OP_TOP, T1REPLAY_OP_W, T1REPLAY_OP_H
	);
}

/* Keep renderer strings out of initialized DGROUP.  Aside from preserving the
 * original OP data layout, this keeps every new title-surface string in this
 * module's private code path. */
enum t1replay_op_word_t {
	T1ROW_REPLAY_BROWSER,
	T1ROW_SAVE_REPLAY,
	T1ROW_SLOT_STATUS_START_STAGE_RANK,
	T1ROW_EMPTY,
	T1ROW_INVALID,
	T1ROW_CLEAR,
	T1ROW_MENU,
	T1ROW_PAGE,
	T1ROW_PRACTICE,
	T1ROW_REPLAY,
	T1ROW_SCENE,
	T1ROW_ROUTE,
	T1ROW_SECTION,
	T1ROW_CHAPTER,
	T1ROW_RANK,
	T1ROW_SCORE,
	T1ROW_LIVES,
	T1ROW_BOMBS,
	T1ROW_POINT_VALUE,
	T1ROW_PELLET_SPEED,
	T1ROW_RNG_SEED,
	T1ROW_START,
	T1ROW_BACK,
	T1ROW_SHRINE,
	T1ROW_SCENE_II,
	T1ROW_SCENE_III,
	T1ROW_SCENE_IV,
	T1ROW_MAKAI,
	T1ROW_JIGOKU,
	T1ROW_STAGE_START,
	T1ROW_BOSS_START,
	T1ROW_EASY,
	T1ROW_NORMAL,
	T1ROW_HARD,
	T1ROW_LUNATIC,
};

static char t1replay_op_text[64];

static char *t1replay_op_word_append(char *p, t1replay_op_word_t word)
{
	#define T1ROW_PUTC(c) *p++ = (c)
	#define T1ROW_SPACE() T1ROW_PUTC(' ')
	#define T1ROW_WORD1(a) T1ROW_PUTC(a)
	#define T1ROW_WORD2(a, b) T1ROW_WORD1(a); T1ROW_WORD1(b)
	#define T1ROW_WORD3(a, b, c) T1ROW_WORD2(a, b); T1ROW_WORD1(c)
	#define T1ROW_WORD4(a, b, c, d) T1ROW_WORD3(a, b, c); T1ROW_WORD1(d)
	switch(word) {
	case T1ROW_REPLAY_BROWSER:
		T1ROW_WORD4('R', 'E', 'P', 'L'); T1ROW_WORD2('A', 'Y'); T1ROW_SPACE(); T1ROW_WORD3('B', 'R', 'O'); T1ROW_WORD3('W', 'S', 'E'); T1ROW_WORD1('R'); break;
	case T1ROW_SAVE_REPLAY:
		T1ROW_WORD4('S', 'A', 'V', 'E'); T1ROW_SPACE(); T1ROW_WORD4('R', 'E', 'P', 'L'); T1ROW_WORD2('A', 'Y'); break;
	case T1ROW_SLOT_STATUS_START_STAGE_RANK:
		T1ROW_WORD4('S', 'L', 'O', 'T'); T1ROW_SPACE(); T1ROW_SPACE(); T1ROW_WORD4('S', 'T', 'A', 'T'); T1ROW_WORD2('U', 'S'); T1ROW_SPACE(); T1ROW_SPACE(); T1ROW_WORD4('S', 'T', 'A', 'R'); T1ROW_WORD1('T'); T1ROW_SPACE(); T1ROW_SPACE(); T1ROW_WORD4('S', 'T', 'A', 'G'); T1ROW_WORD1('E'); T1ROW_SPACE(); T1ROW_SPACE(); T1ROW_WORD4('R', 'A', 'N', 'K'); break;
	case T1ROW_EMPTY: T1ROW_WORD4('E', 'M', 'P', 'T'); T1ROW_WORD1('Y'); break;
	case T1ROW_INVALID: T1ROW_WORD4('I', 'N', 'V', 'A'); T1ROW_WORD3('L', 'I', 'D'); break;
	case T1ROW_CLEAR: T1ROW_WORD4('C', 'L', 'E', 'A'); T1ROW_WORD1('R'); break;
	case T1ROW_MENU: T1ROW_WORD4('M', 'E', 'N', 'U'); break;
	case T1ROW_PAGE: T1ROW_WORD4('P', 'A', 'G', 'E'); break;
	case T1ROW_PRACTICE: T1ROW_WORD4('P', 'R', 'A', 'C'); T1ROW_WORD4('T', 'I', 'C', 'E'); break;
	case T1ROW_REPLAY: T1ROW_WORD4('R', 'E', 'P', 'L'); T1ROW_WORD2('A', 'Y'); break;
	case T1ROW_SCENE: T1ROW_WORD4('S', 'C', 'E', 'N'); T1ROW_WORD1('E'); break;
	case T1ROW_ROUTE: T1ROW_WORD4('R', 'O', 'U', 'T'); T1ROW_WORD1('E'); break;
	case T1ROW_SECTION: T1ROW_WORD4('S', 'E', 'C', 'T'); T1ROW_WORD3('I', 'O', 'N'); break;
	case T1ROW_CHAPTER: T1ROW_WORD4('C', 'H', 'A', 'P'); T1ROW_WORD3('T', 'E', 'R'); break;
	case T1ROW_RANK: T1ROW_WORD4('R', 'A', 'N', 'K'); break;
	case T1ROW_SCORE: T1ROW_WORD4('S', 'C', 'O', 'R'); T1ROW_WORD1('E'); break;
	case T1ROW_LIVES: T1ROW_WORD4('L', 'I', 'V', 'E'); T1ROW_WORD1('S'); break;
	case T1ROW_BOMBS: T1ROW_WORD4('B', 'O', 'M', 'B'); T1ROW_WORD1('S'); break;
	case T1ROW_POINT_VALUE: T1ROW_WORD4('P', 'O', 'I', 'N'); T1ROW_WORD1('T'); T1ROW_SPACE(); T1ROW_WORD4('V', 'A', 'L', 'U'); T1ROW_WORD1('E'); break;
	case T1ROW_PELLET_SPEED: T1ROW_WORD4('P', 'E', 'L', 'L'); T1ROW_WORD2('E', 'T'); T1ROW_SPACE(); T1ROW_WORD4('S', 'P', 'E', 'E'); T1ROW_WORD1('D'); break;
	case T1ROW_RNG_SEED: T1ROW_WORD3('R', 'N', 'G'); T1ROW_SPACE(); T1ROW_WORD4('S', 'E', 'E', 'D'); break;
	case T1ROW_START: T1ROW_WORD4('S', 'T', 'A', 'R'); T1ROW_WORD1('T'); break;
	case T1ROW_BACK: T1ROW_WORD4('B', 'A', 'C', 'K'); break;
	case T1ROW_SHRINE: T1ROW_WORD4('S', 'H', 'R', 'I'); T1ROW_WORD2('N', 'E'); break;
	case T1ROW_SCENE_II: T1ROW_WORD4('S', 'C', 'E', 'N'); T1ROW_WORD1('E'); T1ROW_SPACE(); T1ROW_WORD2('I', 'I'); break;
	case T1ROW_SCENE_III: T1ROW_WORD4('S', 'C', 'E', 'N'); T1ROW_WORD1('E'); T1ROW_SPACE(); T1ROW_WORD3('I', 'I', 'I'); break;
	case T1ROW_SCENE_IV: T1ROW_WORD4('S', 'C', 'E', 'N'); T1ROW_WORD1('E'); T1ROW_SPACE(); T1ROW_WORD2('I', 'V'); break;
	case T1ROW_MAKAI: T1ROW_WORD4('M', 'A', 'K', 'A'); T1ROW_WORD1('I'); break;
	case T1ROW_JIGOKU: T1ROW_WORD4('J', 'I', 'G', 'O'); T1ROW_WORD2('K', 'U'); break;
	case T1ROW_STAGE_START: T1ROW_WORD4('S', 'T', 'A', 'G'); T1ROW_WORD1('E'); T1ROW_SPACE(); T1ROW_WORD4('S', 'T', 'A', 'R'); T1ROW_WORD1('T'); break;
	case T1ROW_BOSS_START: T1ROW_WORD4('B', 'O', 'S', 'S'); T1ROW_SPACE(); T1ROW_WORD4('S', 'T', 'A', 'R'); T1ROW_WORD1('T'); break;
	case T1ROW_EASY: T1ROW_WORD4('E', 'A', 'S', 'Y'); break;
	case T1ROW_NORMAL: T1ROW_WORD4('N', 'O', 'R', 'M'); T1ROW_WORD2('A', 'L'); break;
	case T1ROW_HARD: T1ROW_WORD4('H', 'A', 'R', 'D'); break;
	case T1ROW_LUNATIC: T1ROW_WORD4('L', 'U', 'N', 'A'); T1ROW_WORD3('T', 'I', 'C'); break;
	}
	#undef T1ROW_WORD4
	#undef T1ROW_WORD3
	#undef T1ROW_WORD2
	#undef T1ROW_WORD1
	#undef T1ROW_SPACE
	#undef T1ROW_PUTC
	return p;
}

static char *t1replay_op_uint_append(char *p, uint32_t value, uint8_t width)
{
	char digits[10];
	uint8_t count = 0;

	do {
		digits[count++] = static_cast<char>('0' + (value % 10));
		value /= 10;
	} while(value != 0);
	while(count < width) {
		digits[count++] = '0';
	}
	while(count != 0) {
		*p++ = digits[--count];
	}
	return p;
}

static char *t1replay_op_pellet_speed_append(char *p, pellet_speed_t speed)
{
	uint16_t magnitude;

	if(speed < 0) {
		*p++ = '-';
		magnitude = static_cast<uint16_t>(-speed);
	} else {
		magnitude = speed;
	}
	p = t1replay_op_uint_append(p, magnitude / PELLET_SPEED_MULTIPLIER, 1);
	*p++ = '.';
	return t1replay_op_uint_append(
		p, ((magnitude % PELLET_SPEED_MULTIPLIER) * 100) /
			PELLET_SPEED_MULTIPLIER, 2
	);
}

// The native collection sequence has 17 user-selectable values: 0, 1000 to
// 9000, 10000 to 60000, and the overflow-safe POINT_CAP from stage/item.cpp.
// Keep the mapping in code instead of a static table so this title-only parcel
// still contributes no initialized DGROUP data.
static uint8_t t1replay_op_point_value_index(uint16_t point_value)
{
	if(point_value <= 9000) {
		return static_cast<uint8_t>(point_value / 1000);
	}
	if(point_value <= 60000) {
		return static_cast<uint8_t>(9 + (point_value / 10000));
	}
	return (T1REPLAY_OP_POINT_VALUE_COUNT - 1);
}

static uint16_t t1replay_op_point_value_from_index(uint8_t index)
{
	if(index <= 9) {
		return static_cast<uint16_t>(index * 1000);
	}
	if(index < (T1REPLAY_OP_POINT_VALUE_COUNT - 1)) {
		return static_cast<uint16_t>((index - 9) * 10000);
	}
	return T1REPLAY_OP_POINT_CAP;
}

static uint16_t t1replay_op_point_value_change(uint16_t point_value, int delta)
{
	int index = t1replay_op_point_value_index(point_value) + delta;

	while(index < 0) {
		index += T1REPLAY_OP_POINT_VALUE_COUNT;
	}
	while(index >= T1REPLAY_OP_POINT_VALUE_COUNT) {
		index -= T1REPLAY_OP_POINT_VALUE_COUNT;
	}
	return t1replay_op_point_value_from_index(static_cast<uint8_t>(index));
}

static void t1replay_op_text_left(screen_y_t y, vc_t col, char *end)
{
	*end = '\0';
	graph_putsa_fx(T1REPLAY_OP_LEFT, y, (col | T1REPLAY_OP_FX), t1replay_op_text);
}

static void t1replay_op_text_value(screen_y_t y, vc_t col, char *end)
{
	*end = '\0';
	graph_putsa_fx(
		T1REPLAY_OP_VALUE_LEFT, y, (col | T1REPLAY_OP_FX), t1replay_op_text
	);
}

void t1replay_op_main_choice_put(
	int choice, int center_x, int top, int col, int fx
)
{
	char *p = t1replay_op_text;

	if(choice == 3) {
		*p++ = ' ';
		p = t1replay_op_word_append(p, T1ROW_PRACTICE);
		*p++ = ' ';
	} else {
		*p++ = ' '; *p++ = ' ';
		p = t1replay_op_word_append(p, T1ROW_REPLAY);
		*p++ = ' '; *p++ = ' ';
	}
	*p = '\0';
	graph_putsa_fx(
		static_cast<screen_x_t>(center_x - (shiftjis_w(t1replay_op_text) / 2)),
		static_cast<screen_y_t>(top), (col | fx), t1replay_op_text
	);
}

static char *t1replay_op_rank_append(char *p, int8_t rank)
{
	t1replay_op_word_t word = T1ROW_EASY;

	if(rank == RANK_NORMAL) {
		word = T1ROW_NORMAL;
	} else if(rank == RANK_HARD) {
		word = T1ROW_HARD;
	} else if(rank == RANK_LUNATIC) {
		word = T1ROW_LUNATIC;
	}
	return t1replay_op_word_append(p, word);
}

static char *t1replay_op_scene_append(char *p, uint8_t scene)
{
	t1replay_op_word_t word = T1ROW_SHRINE;

	if(scene == 1) {
		word = T1ROW_SCENE_II;
	} else if(scene == 2) {
		word = T1ROW_SCENE_III;
	} else if(scene == 3) {
		word = T1ROW_SCENE_IV;
	}
	return t1replay_op_word_append(p, word);
}

static char *t1replay_op_route_append(char *p, uint8_t route)
{
	return t1replay_op_word_append(
		p, (route == ROUTE_JIGOKU) ? T1ROW_JIGOKU : T1ROW_MAKAI
	);
}

static char *t1replay_op_section_append(char *p, uint8_t section)
{
	t1replay_op_word_t word = T1ROW_STAGE_START;

	if(section == T1RPS_CHAPTER) {
		word = T1ROW_CHAPTER;
	} else if(section == T1RPS_BOSS_START) {
		word = T1ROW_BOSS_START;
	}
	return t1replay_op_word_append(p, word);
}

static void t1replay_op_input_read(t1replay_op_input_t& input)
{
	REGS in;
	REGS out7;
	REGS out8;
	REGS out9;
	REGS out0;
	REGS out3;
	REGS out5;
	bool up;
	bool down;
	bool left;
	bool right;
	bool enter;
	bool ok;
	bool cancel;

	in.h.ah = 0x04;
	in.h.al = 7; int86(0x18, &in, &out7);
	in.h.al = 8; int86(0x18, &in, &out8);
	in.h.al = 9; int86(0x18, &in, &out9);
	in.h.al = 0; int86(0x18, &in, &out0);
	in.h.al = 3; int86(0x18, &in, &out3);
	in.h.al = 5; int86(0x18, &in, &out5);
	up = ((out7.h.ah & K7_ARROW_UP) || (out8.h.ah & K8_NUM_8));
	down = ((out7.h.ah & K7_ARROW_DOWN) || (out9.h.ah & K9_NUM_2));
	left = ((out7.h.ah & K7_ARROW_LEFT) || (out8.h.ah & K8_NUM_4));
	right = ((out7.h.ah & K7_ARROW_RIGHT) || (out9.h.ah & K9_NUM_6));
	enter = (out3.h.ah & K3_RETURN);
	ok = (enter || (out5.h.ah & K5_Z));
	cancel = (out0.h.ah & K0_ESC);
	if(t1replay_op_wait_release) {
		input.up = input.down = input.left = input.right = input.ok = input.enter = input.cancel = false;
		input.left_held = input.right_held = false;
		if(!up && !down && !left && !right && !ok && !cancel) {
			t1replay_op_wait_release = false;
		}
	} else {
		input.up = (up && !t1replay_op_prev_up);
		input.down = (down && !t1replay_op_prev_down);
		input.left = (left && !t1replay_op_prev_left);
		input.right = (right && !t1replay_op_prev_right);
		input.ok = (ok && !t1replay_op_prev_ok);
		input.enter = (enter && !t1replay_op_prev_enter);
		input.cancel = (cancel && !t1replay_op_prev_cancel);
		input.left_held = left;
		input.right_held = right;
	}
	t1replay_op_prev_up = up;
	t1replay_op_prev_down = down;
	t1replay_op_prev_left = left;
	t1replay_op_prev_right = right;
	t1replay_op_prev_ok = ok;
	t1replay_op_prev_enter = enter;
	t1replay_op_prev_cancel = cancel;
}

static void t1replay_op_input_reset(void)
{
	t1replay_op_prev_up = false;
	t1replay_op_prev_down = false;
	t1replay_op_prev_left = false;
	t1replay_op_prev_right = false;
	t1replay_op_prev_ok = false;
	t1replay_op_prev_enter = false;
	t1replay_op_prev_cancel = false;
	t1replay_op_horizontal_hold = 0;
	t1replay_op_wait_release = true;
}

static void t1replay_op_replay_render(void)
{
	t1replay_op_slot_t slot;
	char *p;
	screen_y_t y = T1REPLAY_OP_TOP;
	uint8_t row;
	uint8_t slot_id;
	vc_t col;

	t1replay_op_backing_restore();
	p = t1replay_op_word_append(
		t1replay_op_text,
		t1replay_op_save_pending ? T1ROW_SAVE_REPLAY : T1ROW_REPLAY_BROWSER
	);
	t1replay_op_text_left(y, T1REPLAY_OP_COL_VALUE, p);
	y += (T1REPLAY_OP_LINE_H * 2);
	p = t1replay_op_word_append(t1replay_op_text, T1ROW_SLOT_STATUS_START_STAGE_RANK);
	t1replay_op_text_left(y, T1REPLAY_OP_COL_LABEL, p);
	y += T1REPLAY_OP_LINE_H;
	for(row = 0; row < T1REPLAY_OP_ROWS_PER_PAGE; row++) {
		slot_id = static_cast<uint8_t>((t1replay_op_page * T1REPLAY_OP_ROWS_PER_PAGE) + row);
		t1replay_op_slot_read(slot_id, slot);
		col = ((t1replay_op_sel == row) ? T1REPLAY_OP_COL_VALUE : T1REPLAY_OP_COL_LABEL);
		p = t1replay_op_uint_append(t1replay_op_text, slot_id, 2);
		*p++ = ' '; *p++ = ' '; *p++ = ' '; *p++ = ' ';
		if(!slot.exists) {
			p = t1replay_op_word_append(p, T1ROW_EMPTY);
		} else if(!slot.valid) {
			p = t1replay_op_word_append(p, T1ROW_INVALID);
			col = T1REPLAY_OP_COL_DISABLED;
		} else {
			p = t1replay_op_word_append(
				p, (slot.header.end_reason == T1REPLAY_END_CLEAR) ?
					T1ROW_CLEAR : T1ROW_MENU
			);
			*p++ = ' '; *p++ = ' ';
			p = t1replay_op_uint_append(p, slot.header.start.score, 9);
			*p++ = ' '; *p++ = ' ';
			p = t1replay_op_uint_append(p, slot.header.start.stage_id + 1, 2);
			*p++ = ' '; *p++ = ' '; *p++ = ' '; *p++ = ' '; *p++ = ' ';
			p = t1replay_op_rank_append(p, slot.header.start.rank);
		}
		t1replay_op_text_left(y, col, p);
		y += T1REPLAY_OP_LINE_H;
	}
	p = t1replay_op_word_append(t1replay_op_text, T1ROW_PAGE);
	*p++ = ' ';
	p = t1replay_op_uint_append(p, t1replay_op_page + 1, 2);
	*p++ = '/'; *p++ = '1'; *p++ = '0';
	t1replay_op_text_left(y, T1REPLAY_OP_COL_LABEL, p);
}

static void t1replay_op_practice_render(void)
{
	char *p;
	screen_y_t y = T1REPLAY_OP_TOP;
	uint8_t row = 0;
	vc_t label_col;
	vc_t value_col;

	t1replay_op_backing_restore();
	p = t1replay_op_word_append(t1replay_op_text, T1ROW_PRACTICE);
	t1replay_op_text_left(y, T1REPLAY_OP_COL_VALUE, p);
	y += (T1REPLAY_OP_LINE_H * 2);
	#define T1REPLAY_OP_PRACTICE_LINE(label_word, value_append) \
		label_col = ((t1replay_op_sel == row) ? T1REPLAY_OP_COL_VALUE : T1REPLAY_OP_COL_LABEL); \
		value_col = label_col; \
		p = t1replay_op_word_append(t1replay_op_text, label_word); \
		t1replay_op_text_left(y, label_col, p); \
		p = (value_append); \
		t1replay_op_text_value(y, value_col, p); \
		y += T1REPLAY_OP_LINE_H; row++
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_SCENE,
		t1replay_op_scene_append(t1replay_op_text, t1replay_practice_start.scene)
	);
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_ROUTE,
		t1replay_op_word_append(
			t1replay_op_text,
			(t1replay_practice_start.scene == 0) ? T1ROW_SHRINE :
				((t1replay_practice_start.route == ROUTE_JIGOKU) ?
					T1ROW_JIGOKU : T1ROW_MAKAI)
		)
	);
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_SECTION,
		t1replay_op_section_append(t1replay_op_text, t1replay_practice_start.section)
	);
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_CHAPTER,
		(t1replay_practice_start.section == T1RPS_CHAPTER) ?
			t1replay_op_uint_append(t1replay_op_text, t1replay_practice_start.chapter + 1, 1) :
			(t1replay_op_text[0] = '-', t1replay_op_text + 1)
	);
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_SCORE,
		t1replay_op_uint_append(t1replay_op_text, t1replay_practice_start.score, 1)
	);
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_LIVES,
		t1replay_op_uint_append(t1replay_op_text, t1replay_practice_start.lives, 1)
	);
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_BOMBS,
		t1replay_op_uint_append(t1replay_op_text, t1replay_practice_start.bombs, 1)
	);
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_POINT_VALUE,
		t1replay_op_uint_append(t1replay_op_text, t1replay_practice_start.point_value, 1)
	);
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_PELLET_SPEED,
		t1replay_op_pellet_speed_append(t1replay_op_text, t1replay_practice_start.pellet_speed)
	);
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_RNG_SEED,
		t1replay_op_uint_append(t1replay_op_text, t1replay_practice_start.rand, 1)
	);
	T1REPLAY_OP_PRACTICE_LINE(T1ROW_START, t1replay_op_text);
	T1REPLAY_OP_PRACTICE_LINE(T1ROW_BACK, t1replay_op_text);
	#undef T1REPLAY_OP_PRACTICE_LINE
}

void t1replay_op_replay_enter(void)
{
	t1replay_op_save_pending = false;
	t1replay_op_sel = 0;
	t1replay_op_page = 0;
	t1replay_op_input_reset();
	t1replay_op_replay_render();
}

bool t1replay_op_pending_enter(void)
{
	t1replay_op_slot_t pending;

	// A stale temporary may be left behind by a reset or power loss. It is
	// never user-visible unless its finalized stream passes the complete OP
	// grammar check, including physical file extent and terminal control.
	t1replay_op_pending_read(pending);
	if(!pending.exists) {
		return false;
	}
	if(
		!pending.valid ||
#if T1REPLAY_FUUIN_SCORE_PROOF
		!t1replay_op_pending_score_proof_valid(pending) ||
#endif
		false
	) {
		t1replay_op_pending_discard();
		return false;
	}
	t1replay_op_save_pending = true;
	t1replay_op_sel = 0;
	t1replay_op_page = 0;
	t1replay_op_input_reset();
	t1replay_op_replay_render();
	return true;
}

void t1replay_op_practice_enter(int8_t rank, int8_t lives, int8_t bombs, uint32_t rand)
{
	memset(&t1replay_practice_start, 0, sizeof(t1replay_practice_start));
	t1replay_practice_start.scene = 0;
	t1replay_practice_start.route = ROUTE_MAKAI;
	t1replay_practice_start.section = T1RPS_STAGE_START;
	t1replay_practice_start.rank = rank;
	t1replay_practice_start.lives = lives;
	t1replay_practice_start.bombs = bombs;
	t1replay_practice_start.pellet_speed = to_pellet_speed(-0.1f);
	t1replay_practice_start.rand = rand;
	t1replay_op_sel = 0;
	t1replay_op_input_reset();
	t1replay_op_practice_render();
}

void t1replay_op_restore(void)
{
	t1replay_op_backing_restore();
}

void t1replay_op_command_clear(void)
{
	char fn[10];

	t1replay_op_command_fn(fn);
	remove(fn);
}

t1replay_op_result_t t1replay_op_replay_update(void)
{
	t1replay_op_input_t input;
	t1replay_op_slot_t slot;
	t1replay_op_result_t result;

	result.action = T1ROA_NONE;
	result.slot = 0;

	t1replay_op_input_read(input);
	if(input.cancel) {
		if(t1replay_op_save_pending) {
			t1replay_op_pending_discard();
			t1replay_op_save_pending = false;
		}
		result.action = T1ROA_RETURN;
		return result;
	}
	if(input.up) {
		if(t1replay_op_sel == 0) {
			t1replay_op_sel = (T1REPLAY_OP_ROWS_PER_PAGE - 1);
		} else {
			t1replay_op_sel--;
		}
		t1replay_op_replay_render();
	}
	if(input.down) {
		t1replay_op_sel = static_cast<uint8_t>(
			(t1replay_op_sel + 1) % T1REPLAY_OP_ROWS_PER_PAGE
		);
		t1replay_op_replay_render();
	}
	if(input.left) {
		if(t1replay_op_page == 0) {
			t1replay_op_page = 9;
		} else {
			t1replay_op_page--;
		}
		t1replay_op_replay_render();
	}
	if(input.right) {
		t1replay_op_page = static_cast<uint8_t>((t1replay_op_page + 1) % 10);
		t1replay_op_replay_render();
	}
	if(input.ok) {
		result.slot = static_cast<uint8_t>(
			(t1replay_op_page * T1REPLAY_OP_ROWS_PER_PAGE) + t1replay_op_sel
		);
		if(t1replay_op_save_pending) {
			if(t1replay_op_pending_commit(result.slot)) {
				t1replay_op_save_pending = false;
				result.action = T1ROA_RETURN;
			} else {
				t1replay_op_pending_read(slot);
				if(!slot.exists || !slot.valid) {
					t1replay_op_pending_discard();
					t1replay_op_save_pending = false;
					result.action = T1ROA_RETURN;
				}
			}
		} else {
			t1replay_op_slot_read(result.slot, slot);
			if(
				slot.valid &&
				t1replay_op_command_write(T1REPLAY_COMMAND_PLAYBACK, result.slot)
			) {
				result.action = T1ROA_PLAYBACK;
			}
		}
	}
	return result;
}

static bool t1replay_op_shift_pressed(void)
{
	REGS in;
	REGS out;

	in.h.ah = 0x02;
	int86(0x18, &in, &out);
	return ((out.h.al & 1) != 0);
}

static void t1replay_op_practice_change(int delta, bool fast)
{
	switch(t1replay_op_sel) {
	case T1OPR_SCENE:
		t1replay_practice_start.scene = static_cast<uint8_t>(
			(t1replay_practice_start.scene + SCENE_COUNT + delta) % SCENE_COUNT
		);
		if(t1replay_practice_start.scene == 0) {
			t1replay_practice_start.route = ROUTE_MAKAI;
		}
		break;
	case T1OPR_ROUTE:
		if(t1replay_practice_start.scene != 0) {
			t1replay_practice_start.route = static_cast<uint8_t>(
				(t1replay_practice_start.route + ROUTE_COUNT + delta) % ROUTE_COUNT
			);
		}
		break;
	case T1OPR_SECTION:
		t1replay_practice_start.section = static_cast<uint8_t>(
			(t1replay_practice_start.section + 3 + delta) % 3
		);
		break;
	case T1OPR_CHAPTER:
		if(t1replay_practice_start.section == T1RPS_CHAPTER) {
			t1replay_practice_start.chapter = static_cast<uint8_t>(
				(t1replay_practice_start.chapter + BOSS_STAGE + delta) % BOSS_STAGE
			);
		}
		break;
	case T1OPR_SCORE:
		t1replay_practice_start.score += (delta * (fast ? 1000000L : 10000L));
		if(t1replay_practice_start.score < 0) {
			t1replay_practice_start.score = 99990000L;
		} else if(t1replay_practice_start.score > 99990000L) {
			t1replay_practice_start.score = 0;
		}
		break;
	case T1OPR_LIVES:
		t1replay_practice_start.lives += delta;
		if(t1replay_practice_start.lives < 1) {
			t1replay_practice_start.lives = LIVES_MAX;
		} else if(t1replay_practice_start.lives > LIVES_MAX) {
			t1replay_practice_start.lives = 1;
		}
		break;
	case T1OPR_BOMBS:
		t1replay_practice_start.bombs += delta;
		if(t1replay_practice_start.bombs < 0) {
			t1replay_practice_start.bombs = BOMBS_MAX;
		} else if(t1replay_practice_start.bombs > BOMBS_MAX) {
			t1replay_practice_start.bombs = 0;
		}
		break;
	case T1OPR_POINT_VALUE:
		t1replay_practice_start.point_value = t1replay_op_point_value_change(
			t1replay_practice_start.point_value, (delta * (fast ? 4 : 1))
		);
		break;
	case T1OPR_PELLET_SPEED:
		t1replay_practice_start.pellet_speed += (delta * (fast ? 4 : 1));
		if(t1replay_practice_start.pellet_speed < PELLET_SPEED_LOWER_MIN) {
			t1replay_practice_start.pellet_speed = PELLET_SPEED_RAISE_MAX;
		} else if(t1replay_practice_start.pellet_speed > PELLET_SPEED_RAISE_MAX) {
			t1replay_practice_start.pellet_speed = PELLET_SPEED_LOWER_MIN;
		}
		break;
	case T1OPR_RNG_SEED:
		t1replay_practice_start.rand += (delta * (fast ? 256L : 1L));
		break;
	}
}

static bool t1replay_op_practice_field_is_numeric(uint8_t field)
{
	return (
		(field == T1OPR_SCORE) || (field == T1OPR_LIVES) ||
		(field == T1OPR_BOMBS) || (field == T1OPR_POINT_VALUE) ||
		(field == T1OPR_PELLET_SPEED) || (field == T1OPR_RNG_SEED)
	);
}

static uint32_t t1replay_op_practice_numeric_get(uint8_t field)
{
	switch(field) {
	case T1OPR_SCORE: return static_cast<uint32_t>(t1replay_practice_start.score);
	case T1OPR_LIVES: return t1replay_practice_start.lives;
	case T1OPR_BOMBS: return t1replay_practice_start.bombs;
	case T1OPR_POINT_VALUE: return t1replay_practice_start.point_value;
	case T1OPR_RNG_SEED: return t1replay_practice_start.rand;
	default: return 0;
	}
}

static uint32_t t1replay_op_practice_numeric_min(uint8_t field)
{
	return ((field == T1OPR_LIVES) ? 1 : 0);
}

static uint32_t t1replay_op_practice_numeric_max(uint8_t field)
{
	switch(field) {
	case T1OPR_SCORE: return 99990000UL;
	case T1OPR_LIVES: return LIVES_MAX;
	case T1OPR_BOMBS: return BOMBS_MAX;
	case T1OPR_POINT_VALUE: return T1REPLAY_OP_POINT_CAP;
	case T1OPR_RNG_SEED: return 0xFFFFFFFFUL;
	default: return 0;
	}
}

static void t1replay_op_practice_numeric_set(uint8_t field, uint32_t value)
{
	switch(field) {
	case T1OPR_SCORE:
		t1replay_practice_start.score = static_cast<int32_t>(value);
		break;
	case T1OPR_LIVES:
		t1replay_practice_start.lives = static_cast<int8_t>(value);
		break;
	case T1OPR_BOMBS:
		t1replay_practice_start.bombs = static_cast<int8_t>(value);
		break;
	case T1OPR_POINT_VALUE:
		t1replay_practice_start.point_value = static_cast<uint16_t>(value);
		break;
	case T1OPR_RNG_SEED:
		t1replay_practice_start.rand = value;
		break;
	}
}

static int t1replay_op_practice_digit_edge(
	uint8_t now0, uint8_t prev0, uint8_t now1, uint8_t prev1
)
{
	#define T1OPR_PRESSED(now, prev, bit) (((now) & (bit)) && !((prev) & (bit)))
	if(T1OPR_PRESSED(now0, prev0, K0_1)) return 1;
	if(T1OPR_PRESSED(now0, prev0, K0_2)) return 2;
	if(T1OPR_PRESSED(now0, prev0, K0_3)) return 3;
	if(T1OPR_PRESSED(now0, prev0, K0_4)) return 4;
	if(T1OPR_PRESSED(now0, prev0, K0_5)) return 5;
	if(T1OPR_PRESSED(now0, prev0, K0_6)) return 6;
	if(T1OPR_PRESSED(now0, prev0, K0_7)) return 7;
	if(T1OPR_PRESSED(now1, prev1, K1_8)) return 8;
	if(T1OPR_PRESSED(now1, prev1, K1_9)) return 9;
	if(T1OPR_PRESSED(now1, prev1, K1_0)) return 0;
	#undef T1OPR_PRESSED
	return -1;
}

static void t1replay_op_practice_numeric_entry(uint8_t field)
{
	uint32_t original = t1replay_op_practice_numeric_get(field);
	uint32_t value = 0;
	uint32_t min = t1replay_op_practice_numeric_min(field);
	uint32_t max = t1replay_op_practice_numeric_max(field);
	uint8_t now0;
	uint8_t now1;
	uint8_t now3;
	uint8_t prev0;
	uint8_t prev1;
	uint8_t prev3;
	int digit;
	bool entered = false;

	do {
		prev3 = static_cast<uint8_t>(peekb(0, KEYGROUP_3));
		frame_delay(1);
	} while(prev3 & K3_RETURN);
	prev0 = static_cast<uint8_t>(peekb(0, KEYGROUP_0));
	prev1 = static_cast<uint8_t>(peekb(0, KEYGROUP_1));
	while(1) {
		now0 = static_cast<uint8_t>(peekb(0, KEYGROUP_0));
		now1 = static_cast<uint8_t>(peekb(0, KEYGROUP_1));
		now3 = static_cast<uint8_t>(peekb(0, KEYGROUP_3));
		if((now0 & K0_ESC) && !(prev0 & K0_ESC)) {
			t1replay_op_practice_numeric_set(field, original);
			t1replay_op_practice_render();
			t1replay_op_input_reset();
			return;
		}
		if((now3 & K3_RETURN) && !(prev3 & K3_RETURN)) {
			if(entered) {
				if(value < min) {
					value = min;
				}
				if(field == T1OPR_POINT_VALUE) {
					value = t1replay_op_point_value_from_index(
						t1replay_op_point_value_index(static_cast<uint16_t>(value))
					);
				}
				t1replay_op_practice_numeric_set(field, value);
			}
			t1replay_op_practice_render();
			t1replay_op_input_reset();
			return;
		}
		if((now1 & K1_BACKSPACE) && !(prev1 & K1_BACKSPACE)) {
			value /= 10UL;
			t1replay_op_practice_numeric_set(field, ((value < min) ? min : value));
			entered = true;
			t1replay_op_practice_render();
		} else {
			digit = t1replay_op_practice_digit_edge(now0, prev0, now1, prev1);
			if(digit >= 0) {
				if(
					(static_cast<uint32_t>(digit) > max) ||
					(value > ((max - digit) / 10UL))
				) {
					value = max;
				} else {
					value = ((value * 10UL) + digit);
				}
				t1replay_op_practice_numeric_set(
					field, ((value < min) ? min : value)
				);
				entered = true;
				t1replay_op_practice_render();
			}
		}
		prev0 = now0;
		prev1 = now1;
		prev3 = now3;
		frame_delay(1);
	}
}

static pellet_speed_t t1replay_op_pellet_speed_from_magnitude(
	uint16_t magnitude, bool negative
)
{
	return static_cast<pellet_speed_t>(
		negative ? -static_cast<int>(magnitude) : magnitude
	);
}

static uint16_t t1replay_op_pellet_speed_magnitude_max(bool negative)
{
	return static_cast<uint16_t>(
		negative ? -PELLET_SPEED_LOWER_MIN : PELLET_SPEED_RAISE_MAX
	);
}

static void t1replay_op_practice_pellet_speed_entry(void)
{
	pellet_speed_t original = t1replay_practice_start.pellet_speed;
	uint16_t magnitude = 0;
	uint16_t max;
	uint8_t now0;
	uint8_t now1;
	uint8_t now3;
	uint8_t prev0;
	uint8_t prev1;
	uint8_t prev3;
	int digit;
	bool negative = false;
	bool entered = false;

	do {
		prev3 = static_cast<uint8_t>(peekb(0, KEYGROUP_3));
		frame_delay(1);
	} while(prev3 & K3_RETURN);
	prev0 = static_cast<uint8_t>(peekb(0, KEYGROUP_0));
	prev1 = static_cast<uint8_t>(peekb(0, KEYGROUP_1));
	while(1) {
		now0 = static_cast<uint8_t>(peekb(0, KEYGROUP_0));
		now1 = static_cast<uint8_t>(peekb(0, KEYGROUP_1));
		now3 = static_cast<uint8_t>(peekb(0, KEYGROUP_3));
		if((now0 & K0_ESC) && !(prev0 & K0_ESC)) {
			t1replay_practice_start.pellet_speed = original;
			t1replay_op_practice_render();
			t1replay_op_input_reset();
			return;
		}
		if((now3 & K3_RETURN) && !(prev3 & K3_RETURN)) {
			if(entered) {
				t1replay_practice_start.pellet_speed =
					t1replay_op_pellet_speed_from_magnitude(magnitude, negative);
			}
			t1replay_op_practice_render();
			t1replay_op_input_reset();
			return;
		}
		if((now1 & K1_MINUS) && !(prev1 & K1_MINUS)) {
			negative = !negative;
			t1replay_practice_start.pellet_speed =
				t1replay_op_pellet_speed_from_magnitude(magnitude, negative);
			entered = true;
			t1replay_op_practice_render();
		} else if((now1 & K1_BACKSPACE) && !(prev1 & K1_BACKSPACE)) {
			magnitude /= 10;
			t1replay_practice_start.pellet_speed =
				t1replay_op_pellet_speed_from_magnitude(magnitude, negative);
			entered = true;
			t1replay_op_practice_render();
		} else {
			digit = t1replay_op_practice_digit_edge(now0, prev0, now1, prev1);
			if(digit >= 0) {
				max = t1replay_op_pellet_speed_magnitude_max(negative);
				if(
					(static_cast<uint16_t>(digit) > max) ||
					(magnitude > ((max - digit) / 10))
				) {
					magnitude = max;
				} else {
					magnitude = static_cast<uint16_t>((magnitude * 10) + digit);
				}
				t1replay_practice_start.pellet_speed =
					t1replay_op_pellet_speed_from_magnitude(magnitude, negative);
				entered = true;
				t1replay_op_practice_render();
			}
		}
		prev0 = now0;
		prev1 = now1;
		prev3 = now3;
		frame_delay(1);
	}
}

t1replay_op_result_t t1replay_op_practice_update(void)
{
	t1replay_op_input_t input;
	t1replay_op_result_t result;
	bool horizontal_trigger = false;
	result.action = T1ROA_NONE;
	result.slot = 0;

	t1replay_op_input_read(input);
	if(input.cancel) {
		result.action = T1ROA_RETURN;
		return result;
	}
	if(input.up) {
		if(t1replay_op_sel == 0) {
			t1replay_op_sel = (T1REPLAY_OP_PRACTICE_ROW_COUNT - 1);
		} else {
			t1replay_op_sel--;
		}
		t1replay_op_practice_render();
	}
	if(input.down) {
		t1replay_op_sel = static_cast<uint8_t>(
			(t1replay_op_sel + 1) % T1REPLAY_OP_PRACTICE_ROW_COUNT
		);
		t1replay_op_practice_render();
	}
	if(!input.left_held && !input.right_held) {
		t1replay_op_horizontal_hold = 0;
	} else {
		horizontal_trigger = (
			input.left || input.right ||
			((t1replay_op_horizontal_hold >= 12) &&
			 ((t1replay_op_horizontal_hold & 1) == 0))
		);
		if(t1replay_op_horizontal_hold != 255) {
			t1replay_op_horizontal_hold++;
		}
	}
	if(horizontal_trigger) {
		t1replay_op_practice_change(
			(input.right_held ? 1 : -1), t1replay_op_shift_pressed()
		);
		t1replay_op_practice_render();
	}
	if(input.enter && t1replay_op_practice_field_is_numeric(t1replay_op_sel)) {
		if(t1replay_op_sel == T1OPR_PELLET_SPEED) {
			t1replay_op_practice_pellet_speed_entry();
		} else {
			t1replay_op_practice_numeric_entry(t1replay_op_sel);
		}
	} else if(input.ok) {
		if(t1replay_op_sel == T1OPR_START) {
			if(t1replay_op_record_prepare()) {
				result.action = T1ROA_PRACTICE_RECORD;
			} else {
				result.action = T1ROA_PRACTICE_UNRECORDED;
			}
		} else if(t1replay_op_sel == T1OPR_BACK) {
			result.action = T1ROA_RETURN;
		}
	}
	return result;
}

void t1replay_op_practice_start_get(t1replay_practice_start_t& start)
{
	start = t1replay_practice_start;
	start.route = (start.scene == 0) ? ROUTE_MAKAI : start.route;
	if(start.section == T1RPS_BOSS_START) {
		start.chapter = BOSS_STAGE;
	} else if(start.section == T1RPS_STAGE_START) {
		start.chapter = 0;
	}
}
