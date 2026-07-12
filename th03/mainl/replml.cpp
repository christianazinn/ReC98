#pragma option -zCREPLAYL_TEXT

#include "libs/master.lib/master.hpp"
#include "platform.h"
#include "th03/hardware/input.h"
#include "th03/mainl/replay.hpp"
#include "th03/replay_handoff.hpp"
#include "th03/resident.hpp"
#include "th03/replay_protect.hpp"

static char T3_USER_REPLAY_INDEX_FN[16];
static char T3_USER_REPLAY_SLOT_FN[18];
static char T3_USER_REPLAY_FALLBACK_FN[12];

enum mainl_replay_mode_t {
	MR_DISABLED = 0,
	MR_USER_RECORD = 1,
	MR_USER_PLAYBACK = 2,
	MR_ERROR = 3,
};

static mainl_replay_mode_t mainl_replay_mode;
static replay_user_header_t replay_user_header;
static replay_user_index_entry_t replay_user_index_entry;
static const char *replay_user_fn;
static uint8_t replay_user_slot;
static uint32_t replay_sample_count;
static uint32_t replay_global_frame;
static uint32_t replay_input_byte_count;
static uint32_t replay_packet_tag_offset;
static uint8_t replay_rle_phase;
static uint8_t replay_rle_run;
static uint16_t replay_rle_input_mp_p1;
static uint16_t replay_rle_input_mp_p2;
static uint16_t replay_rle_input_sp;
static bool replay_paths_initialized;
static bool replay_rle_packet_open;

static uint8_t mainl_replay_handoff_u8(unsigned index)
{
	return static_cast<uint8_t>(resident->unused_3[index]);
}

static uint32_t mainl_replay_handoff_u32_read(unsigned index)
{
	return (
		static_cast<uint32_t>(mainl_replay_handoff_u8(index)) |
		(static_cast<uint32_t>(mainl_replay_handoff_u8(index + 1)) << 8) |
		(static_cast<uint32_t>(mainl_replay_handoff_u8(index + 2)) << 16) |
		(static_cast<uint32_t>(mainl_replay_handoff_u8(index + 3)) << 24)
	);
}

static void mainl_replay_handoff_u32_write(unsigned index, uint32_t value)
{
	resident->unused_3[index + 0] = static_cast<uint8_t>(value);
	resident->unused_3[index + 1] = static_cast<uint8_t>(value >> 8);
	resident->unused_3[index + 2] = static_cast<uint8_t>(value >> 16);
	resident->unused_3[index + 3] = static_cast<uint8_t>(value >> 24);
}

static void mainl_replay_cursor_store(void)
{
	mainl_replay_handoff_u32_write(
		T3_REPLAY_RES_SAMPLE_COUNT_INDEX, replay_sample_count
	);
	mainl_replay_handoff_u32_write(
		T3_REPLAY_RES_GLOBAL_FRAME_INDEX, replay_global_frame
	);
	mainl_replay_handoff_u32_write(
		T3_REPLAY_RES_INPUT_SIZE_INDEX, replay_input_byte_count
	);
}

static void mainl_replay_memclear(void near *buf, unsigned size)
{
	uint8_t near *p = reinterpret_cast<uint8_t near *>(buf);

	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

static bool mainl_replay_write_bytes_checked(const void far *buf, unsigned size)
{
	return (file_write(buf, size) != 0);
}

static bool mainl_replay_write_u16_checked(uint16_t value)
{
	return mainl_replay_write_bytes_checked(&value, sizeof(value));
}

static void mainl_replay_paths_init(void)
{
	if(replay_paths_initialized) {
		return;
	}

	T3_USER_REPLAY_INDEX_FN[0] = 'R';
	T3_USER_REPLAY_INDEX_FN[1] = 'E';
	T3_USER_REPLAY_INDEX_FN[2] = 'P';
	T3_USER_REPLAY_INDEX_FN[3] = 'L';
	T3_USER_REPLAY_INDEX_FN[4] = 'A';
	T3_USER_REPLAY_INDEX_FN[5] = 'Y';
	T3_USER_REPLAY_INDEX_FN[6] = '\\';
	T3_USER_REPLAY_INDEX_FN[7] = 'T';
	T3_USER_REPLAY_INDEX_FN[8] = 'H';
	T3_USER_REPLAY_INDEX_FN[9] = '3';
	T3_USER_REPLAY_INDEX_FN[10] = 'R';
	T3_USER_REPLAY_INDEX_FN[11] = '.';
	T3_USER_REPLAY_INDEX_FN[12] = 'I';
	T3_USER_REPLAY_INDEX_FN[13] = 'D';
	T3_USER_REPLAY_INDEX_FN[14] = 'X';
	T3_USER_REPLAY_INDEX_FN[15] = '\0';

	T3_USER_REPLAY_SLOT_FN[0] = 'R';
	T3_USER_REPLAY_SLOT_FN[1] = 'E';
	T3_USER_REPLAY_SLOT_FN[2] = 'P';
	T3_USER_REPLAY_SLOT_FN[3] = 'L';
	T3_USER_REPLAY_SLOT_FN[4] = 'A';
	T3_USER_REPLAY_SLOT_FN[5] = 'Y';
	T3_USER_REPLAY_SLOT_FN[6] = '\\';
	T3_USER_REPLAY_SLOT_FN[7] = 'T';
	T3_USER_REPLAY_SLOT_FN[8] = 'H';
	T3_USER_REPLAY_SLOT_FN[9] = '3';
	T3_USER_REPLAY_SLOT_FN[10] = 'R';
	T3_USER_REPLAY_SLOT_FN[11] = '0';
	T3_USER_REPLAY_SLOT_FN[12] = '0';
	T3_USER_REPLAY_SLOT_FN[13] = '.';
	T3_USER_REPLAY_SLOT_FN[14] = 'R';
	T3_USER_REPLAY_SLOT_FN[15] = 'P';
	T3_USER_REPLAY_SLOT_FN[16] = 'Y';
	T3_USER_REPLAY_SLOT_FN[17] = '\0';

	T3_USER_REPLAY_FALLBACK_FN[0] = 'T';
	T3_USER_REPLAY_FALLBACK_FN[1] = 'H';
	T3_USER_REPLAY_FALLBACK_FN[2] = '3';
	T3_USER_REPLAY_FALLBACK_FN[3] = 'L';
	T3_USER_REPLAY_FALLBACK_FN[4] = 'A';
	T3_USER_REPLAY_FALLBACK_FN[5] = 'S';
	T3_USER_REPLAY_FALLBACK_FN[6] = 'T';
	T3_USER_REPLAY_FALLBACK_FN[7] = '.';
	T3_USER_REPLAY_FALLBACK_FN[8] = 'R';
	T3_USER_REPLAY_FALLBACK_FN[9] = 'P';
	T3_USER_REPLAY_FALLBACK_FN[10] = 'Y';
	T3_USER_REPLAY_FALLBACK_FN[11] = '\0';

	replay_user_fn = T3_USER_REPLAY_FALLBACK_FN;
	replay_user_slot = T3_REPLAY_USER_SLOT_NONE;
	replay_paths_initialized = true;
}

static mainl_replay_mode_t mainl_replay_resident_mode(void)
{
	if(
		(resident->unused_3[0] != T3_REPLAY_RES_MAGIC_0) ||
		(resident->unused_3[1] != T3_REPLAY_RES_MAGIC_1) ||
		(resident->unused_3[2] != T3_REPLAY_RES_MAGIC_2) ||
		(resident->unused_3[3] != T3_REPLAY_RES_MAGIC_3)
	) {
		return MR_DISABLED;
	}
	if(
		resident->unused_3[T3_REPLAY_RES_MODE_INDEX] ==
		T3_REPLAY_RES_MODE_USER_RECORD
	) {
		return MR_USER_RECORD;
	}
	if(
		resident->unused_3[T3_REPLAY_RES_MODE_INDEX] ==
		T3_REPLAY_RES_MODE_USER_PLAYBACK
	) {
		return MR_USER_PLAYBACK;
	}
	return MR_DISABLED;
}

static void mainl_replay_handoff_clear(void)
{
	int i;

	resident->unused_3[0] = 0;
	resident->unused_3[1] = 0;
	resident->unused_3[2] = 0;
	resident->unused_3[3] = 0;
	resident->unused_3[T3_REPLAY_RES_MODE_INDEX] = 0;
	resident->unused_3[T3_REPLAY_RES_SLOT_INDEX] = T3_REPLAY_USER_SLOT_NONE;
	for(
		i = T3_REPLAY_RES_SAMPLE_COUNT_INDEX;
		i < T3_REPLAY_RES_CURSOR_END_INDEX;
		i++
	) {
		resident->unused_3[i] = 0;
	}
}

static uint8_t mainl_replay_resident_slot(void)
{
	uint8_t slot = mainl_replay_handoff_u8(T3_REPLAY_RES_SLOT_INDEX);

	if(slot < T3_REPLAY_USER_SLOT_COUNT) {
		return slot;
	}
	return T3_REPLAY_USER_SLOT_NONE;
}

static void mainl_replay_user_slot_fn_set(uint8_t slot)
{
	if(slot < T3_REPLAY_USER_SLOT_COUNT) {
		replay_user_slot = slot;
		T3_USER_REPLAY_SLOT_FN[11] = static_cast<char>('0' + (slot / 10));
		T3_USER_REPLAY_SLOT_FN[12] = static_cast<char>('0' + (slot % 10));
		replay_user_fn = T3_USER_REPLAY_SLOT_FN;
	} else {
		replay_user_slot = T3_REPLAY_USER_SLOT_NONE;
		replay_user_fn = T3_USER_REPLAY_FALLBACK_FN;
	}
}

static void mainl_replay_guard_fn_set(char far *fn)
{
	fn[0] = '\\';
	if(replay_user_slot < T3_REPLAY_USER_SLOT_COUNT) {
		fn[1] = 'T';
		fn[2] = 'H';
		fn[3] = '3';
		fn[4] = 'G';
		fn[5] = static_cast<char>('0' + (replay_user_slot / 10));
		fn[6] = static_cast<char>('0' + (replay_user_slot % 10));
		fn[7] = '.';
		fn[8] = 'T';
		fn[9] = 'M';
		fn[10] = 'P';
		fn[11] = '\0';
	} else {
		fn[1] = 'T';
		fn[2] = 'H';
		fn[3] = '3';
		fn[4] = 'L';
		fn[5] = 'A';
		fn[6] = 'S';
		fn[7] = 'T';
		fn[8] = '.';
		fn[9] = 'G';
		fn[10] = 'R';
		fn[11] = 'D';
		fn[12] = '\0';
	}
}

static void mainl_replay_latch_fn_set(char far *fn)
{
	fn[0] = '\\';
	if(replay_user_slot < T3_REPLAY_USER_SLOT_COUNT) {
		fn[1] = 'T';
		fn[2] = 'H';
		fn[3] = '3';
		fn[4] = 'S';
		fn[5] = static_cast<char>('0' + (replay_user_slot / 10));
		fn[6] = static_cast<char>('0' + (replay_user_slot % 10));
		fn[7] = '.';
		fn[8] = 'T';
		fn[9] = 'M';
		fn[10] = 'P';
		fn[11] = '\0';
	} else {
		fn[1] = 'T';
		fn[2] = 'H';
		fn[3] = '3';
		fn[4] = 'L';
		fn[5] = 'A';
		fn[6] = 'S';
		fn[7] = 'T';
		fn[8] = '.';
		fn[9] = 'L';
		fn[10] = 'C';
		fn[11] = 'H';
		fn[12] = '\0';
	}
}

static bool mainl_replay_guard_verify(void)
{
	char guard_fn[13];
	char latch_fn[13];

	mainl_replay_guard_fn_set(guard_fn);
	mainl_replay_latch_fn_set(latch_fn);
	if(!replay_protect_latch_verify(latch_fn)) {
		return false;
	}
	if(!replay_protect_verify(guard_fn)) {
		if(replay_protect_invalid()) {
			replay_protect_latch_set(latch_fn);
		}
		return false;
	}
	return true;
}

static bool mainl_replay_guard_checkpoint(void)
{
	char guard_fn[13];
	char latch_fn[13];

	mainl_replay_guard_fn_set(guard_fn);
	mainl_replay_latch_fn_set(latch_fn);
	if(!replay_protect_checkpoint(guard_fn)) {
		if(replay_protect_invalid()) {
			replay_protect_latch_set(latch_fn);
		}
		return false;
	}
	return true;
}

static void mainl_replay_guard_delete(void)
{
	char guard_fn[13];
	char latch_fn[13];

	mainl_replay_guard_fn_set(guard_fn);
	mainl_replay_latch_fn_set(latch_fn);
	dos_axdx(0x4100, latch_fn);
	dos_axdx(0x4100, guard_fn);
}

static bool mainl_replay_user_header_valid(void)
{
	bool v2 = (
		(replay_user_header.magic[6] == '2') &&
		(replay_user_header.version == T3_REPLAY_USER_VERSION_V2) &&
		(replay_user_header.sample_size == sizeof(replay_user_sample_t))
	);
	bool v3 = (
		(replay_user_header.magic[6] == '3') &&
		(replay_user_header.version == T3_REPLAY_USER_VERSION_V3) &&
		(replay_user_header.sample_size == T3_REPLAY_USER_SAMPLE_SIZE_RLE) &&
		((replay_user_header.flags & T3_REPLAY_USER_FLAG_RLE_INPUT) != 0)
	);
	bool v4 = (
		(replay_user_header.magic[6] == '4') &&
		(replay_user_header.version == T3_REPLAY_USER_VERSION) &&
		(replay_user_header.sample_size == T3_REPLAY_USER_SAMPLE_SIZE_RLE) &&
		((replay_user_header.flags & T3_REPLAY_USER_FLAG_RLE_INPUT) != 0)
	);

	return (
		(replay_user_header.magic[0] == 'T') &&
		(replay_user_header.magic[1] == '3') &&
		(replay_user_header.magic[2] == 'R') &&
		(replay_user_header.magic[3] == 'P') &&
		(replay_user_header.magic[4] == 'L') &&
		(replay_user_header.magic[5] == 'Y') &&
		(v2 || v3 || v4) &&
		(
			(
				(v2 || v3) &&
				(replay_user_header.header_size == sizeof(replay_user_header))
			) ||
			(
				v4 &&
				(replay_user_header.header_size == (
					sizeof(replay_user_header) +
					sizeof(replay_user_summary_ext_t)
				))
			)
		) &&
		(replay_user_header.snapshot_offset == replay_user_header.header_size) &&
		(replay_user_header.snapshot_size == sizeof(replay_user_snapshot_t)) &&
		(replay_user_header.input_offset == (
			static_cast<uint32_t>(replay_user_header.header_size) +
			static_cast<uint32_t>(sizeof(replay_user_snapshot_t))
		))
	);
}

static bool mainl_replay_user_header_read(void)
{
	mainl_replay_user_slot_fn_set(mainl_replay_resident_slot());
	if(!file_ropen(replay_user_fn)) {
		mainl_replay_user_slot_fn_set(T3_REPLAY_USER_SLOT_NONE);
		if(!file_ropen(replay_user_fn)) {
			return false;
		}
	}
	if(
		file_read(&replay_user_header, sizeof(replay_user_header)) !=
		sizeof(replay_user_header)
	) {
		file_close();
		return false;
	}
	file_close();
	return mainl_replay_user_header_valid();
}

static bool mainl_replay_user_index_write(
	replay_user_status_t status, replay_user_end_reason_t end_reason
)
{
	uint32_t offset;
	int i;

	if(replay_user_slot >= T3_REPLAY_USER_SLOT_COUNT) {
		return false;
	}
	if(!file_append(T3_USER_REPLAY_INDEX_FN)) {
		return false;
	}

	mainl_replay_memclear(&replay_user_index_entry, sizeof(replay_user_index_entry));
	replay_user_index_entry.used = true;
	replay_user_index_entry.slot_id = replay_user_slot;
	replay_user_index_entry.status = status;
	replay_user_index_entry.end_reason = end_reason;
	replay_user_index_entry.game_mode = replay_user_header.game_mode;
	replay_user_index_entry.rank = replay_user_header.rank;
	replay_user_index_entry.key_mode = replay_user_header.key_mode;
	replay_user_index_entry.playchar_p1 = replay_user_header.playchar_p1;
	replay_user_index_entry.playchar_p2 = replay_user_header.playchar_p2;
	replay_user_index_entry.story_stage = replay_user_header.story_stage;
	replay_user_index_entry.is_cpu_p1 = replay_user_header.is_cpu_p1;
	replay_user_index_entry.is_cpu_p2 = replay_user_header.is_cpu_p2;
	replay_user_index_entry.sample_count = replay_user_header.sample_count;
	replay_user_index_entry.final_frame_count = (
		replay_user_header.final_frame_count
	);
	replay_user_index_entry.resident_rand = replay_user_header.resident_rand;
	replay_user_index_entry.random_seed_snapshot = (
		replay_user_header.random_seed_snapshot
	);
	replay_user_index_entry.input_crc32 = replay_user_header.input_crc32;
	replay_user_index_entry.snapshot_crc32 = replay_user_header.snapshot_crc32;
	replay_user_index_entry.summary_flags = replay_user_header.summary_flags;
	replay_user_index_entry.final_route = replay_user_header.final_route;
	replay_user_index_entry.final_story_stage = (
		replay_user_header.final_story_stage
	);
	replay_user_index_entry.final_story_lives = (
		replay_user_header.final_story_lives
	);
	replay_user_index_entry.final_misses = replay_user_header.final_misses;
	replay_user_index_entry.stage_reached_count = (
		replay_user_header.stage_reached_count
	);
	for(i = 0; i < T3_REPLAY_USER_PACKED_SCORE_SIZE; i++) {
		replay_user_index_entry.final_score[i] = replay_user_header.final_score[i];
	}
	for(i = 0; i < T3_REPLAY_USER_STAGE_COUNT; i++) {
		replay_user_index_entry.stage_opponents[i] = (
			replay_user_header.stage_opponents[i]
		);
	}

	offset = (
		static_cast<uint32_t>(sizeof(replay_user_index_header_t)) +
		(
			static_cast<uint32_t>(replay_user_slot) *
			static_cast<uint32_t>(sizeof(replay_user_index_entry))
		)
	);
	file_seek(offset, SEEK_SET);
	replay_user_index_entry.status = status;
	replay_user_index_entry.end_reason = end_reason;
	file_write(&replay_user_index_entry, sizeof(replay_user_index_entry));
	file_close();
	return true;
}

static bool mainl_replay_user_index_clear(void)
{
	uint32_t offset;

	if(replay_user_slot >= T3_REPLAY_USER_SLOT_COUNT) {
		return false;
	}
	if(!file_append(T3_USER_REPLAY_INDEX_FN)) {
		return false;
	}

	mainl_replay_memclear(&replay_user_index_entry, sizeof(replay_user_index_entry));
	offset = (
		static_cast<uint32_t>(sizeof(replay_user_index_header_t)) +
		(
			static_cast<uint32_t>(replay_user_slot) *
			static_cast<uint32_t>(sizeof(replay_user_index_entry))
		)
	);
	file_seek(offset, SEEK_SET);
	file_write(&replay_user_index_entry, sizeof(replay_user_index_entry));
	file_close();
	return true;
}

static bool mainl_replay_user_header_write(
	replay_user_status_t status, replay_user_end_reason_t end_reason
)
{
	if(replay_protect_blocked()) {
		return false;
	}
	if(!mainl_replay_guard_verify()) {
		return false;
	}
	replay_user_header.status = status;
	replay_user_header.end_reason = end_reason;
	replay_user_header.sample_count = replay_sample_count;
	replay_user_header.final_frame_count = replay_global_frame;
	if(replay_user_header.sample_size == T3_REPLAY_USER_SAMPLE_SIZE_RLE) {
		replay_user_header.input_size = replay_input_byte_count;
	} else {
		replay_user_header.input_size = (
			replay_sample_count *
			static_cast<uint32_t>(sizeof(replay_user_sample_t))
		);
	}

	if(!file_append(replay_user_fn)) {
		replay_protect_detector_error_set();
		return false;
	}
	file_seek(0, SEEK_SET);
	if(!mainl_replay_write_bytes_checked(
		&replay_user_header, sizeof(replay_user_header)
	)) {
		file_close();
		replay_protect_detector_error_set();
		return false;
	}
	if(!replay_protect_close_current_file(RPD_COMMIT_FLUSH)) {
		return false;
	}
	mainl_replay_user_index_write(status, end_reason);
	return true;
}

static bool mainl_replay_record_sample(void)
{
	replay_user_sample_t sample;
	uint32_t offset;

	sample.frame_index = replay_global_frame;
	sample.input_mp_p1 = input_mp_p1;
	sample.input_mp_p2 = input_mp_p2;
	sample.input_sp = input_sp;
	sample.round_or_result_frame = T3_REPLAY_INTERSTITIAL_ROUND_OR_RESULT_FRAME;
	sample.round_frame = T3_REPLAY_INTERSTITIAL_ROUND_FRAME;

	offset = (
		replay_user_header.input_offset +
		(replay_sample_count * static_cast<uint32_t>(sizeof(sample)))
	);
	if(!mainl_replay_guard_checkpoint()) {
		return true;
	}
	if(!file_append(replay_user_fn)) {
		replay_protect_detector_error_set();
		return true;
	}
	file_seek(offset, SEEK_SET);
	if(!mainl_replay_write_bytes_checked(&sample, sizeof(sample))) {
		file_close();
		replay_protect_detector_error_set();
		return true;
	}
	file_close();
	replay_sample_count++;
	return true;
}

static bool mainl_replay_play_sample(void)
{
	replay_user_sample_t sample;
	uint32_t offset;

	if(replay_sample_count >= replay_user_header.sample_count) {
		return false;
	}

	offset = (
		replay_user_header.input_offset +
		(replay_sample_count * static_cast<uint32_t>(sizeof(sample)))
	);
	if(!file_ropen(replay_user_fn)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	if(file_read(&sample, sizeof(sample)) != sizeof(sample)) {
		file_close();
		return false;
	}
	file_close();

	if(
		(sample.frame_index != replay_global_frame) ||
		(
			sample.round_frame !=
			T3_REPLAY_INTERSTITIAL_ROUND_FRAME
		) ||
		(
			sample.round_or_result_frame !=
			T3_REPLAY_INTERSTITIAL_ROUND_OR_RESULT_FRAME
		)
	) {
		return false;
	}

	input_mp_p1 = sample.input_mp_p1;
	input_mp_p2 = sample.input_mp_p2;
	input_sp = sample.input_sp;
	replay_sample_count++;
	return true;
}

static uint8_t mainl_replay_rle_tag(uint8_t phase, uint8_t run)
{
	return static_cast<uint8_t>(
		(phase << T3_REPLAY_PACKET_PHASE_SHIFT) | (run - 1)
	);
}

static bool mainl_replay_record_rle_sample(void)
{
	uint16_t input_p1 = input_mp_p1;
	uint16_t input_p2 = input_mp_p2;
	uint16_t input_single = input_sp;
	uint8_t change = 0;
	uint8_t tag;
	uint32_t offset;
	uint32_t new_input_byte_count;

	if(replay_protect_blocked()) {
		return true;
	}

	if(
		replay_rle_packet_open &&
		(replay_rle_phase == T3_REPLAY_PACKET_PHASE_INTERSTITIAL) &&
		(replay_rle_input_mp_p1 == input_p1) &&
		(replay_rle_input_mp_p2 == input_p2) &&
		(replay_rle_input_sp == input_single) &&
		(replay_rle_run < T3_REPLAY_PACKET_RUN_MAX)
	) {
		tag = mainl_replay_rle_tag(
			T3_REPLAY_PACKET_PHASE_INTERSTITIAL, replay_rle_run + 1
		);
		if(!mainl_replay_guard_checkpoint()) {
			return true;
		}
		replay_rle_run++;
		if(!file_append(replay_user_fn)) {
			replay_protect_detector_error_set();
			return true;
		}
		file_seek(replay_packet_tag_offset, SEEK_SET);
		if(!mainl_replay_write_bytes_checked(&tag, sizeof(tag))) {
			file_close();
			replay_protect_detector_error_set();
			return true;
		}
		file_close();
		replay_sample_count++;
		return true;
	}

	if(input_p1 != replay_rle_input_mp_p1) {
		change |= T3_REPLAY_PACKET_CHANGE_P1;
	}
	if(input_p2 != replay_rle_input_mp_p2) {
		change |= T3_REPLAY_PACKET_CHANGE_P2;
	}
	if(input_single != replay_rle_input_sp) {
		change |= T3_REPLAY_PACKET_CHANGE_SP;
	}

	offset = (replay_user_header.input_offset + replay_input_byte_count);
	new_input_byte_count = replay_input_byte_count;
	tag = mainl_replay_rle_tag(T3_REPLAY_PACKET_PHASE_INTERSTITIAL, 1);
	if(!mainl_replay_guard_checkpoint()) {
		return true;
	}
	if(!file_append(replay_user_fn)) {
		replay_protect_detector_error_set();
		return true;
	}
	file_seek(offset, SEEK_SET);
	if(
		!mainl_replay_write_bytes_checked(&tag, sizeof(tag)) ||
		!mainl_replay_write_bytes_checked(&change, sizeof(change))
	) {
		file_close();
		replay_protect_detector_error_set();
		return true;
	}
	new_input_byte_count += 2;
	if(change & T3_REPLAY_PACKET_CHANGE_P1) {
		if(!mainl_replay_write_u16_checked(input_p1)) {
			file_close();
			replay_protect_detector_error_set();
			return true;
		}
		new_input_byte_count += sizeof(input_p1);
	}
	if(change & T3_REPLAY_PACKET_CHANGE_P2) {
		if(!mainl_replay_write_u16_checked(input_p2)) {
			file_close();
			replay_protect_detector_error_set();
			return true;
		}
		new_input_byte_count += sizeof(input_p2);
	}
	if(change & T3_REPLAY_PACKET_CHANGE_SP) {
		if(!mainl_replay_write_u16_checked(input_single)) {
			file_close();
			replay_protect_detector_error_set();
			return true;
		}
		new_input_byte_count += sizeof(input_single);
	}
	file_close();

	replay_input_byte_count = new_input_byte_count;
	replay_packet_tag_offset = offset;
	replay_rle_phase = T3_REPLAY_PACKET_PHASE_INTERSTITIAL;
	replay_rle_run = 1;
	replay_rle_input_mp_p1 = input_p1;
	replay_rle_input_mp_p2 = input_p2;
	replay_rle_input_sp = input_single;
	replay_rle_packet_open = true;
	replay_sample_count++;
	return true;
}

static bool mainl_replay_read_rle_packet(void)
{
	uint8_t tag;
	uint8_t change;
	uint32_t offset = (replay_user_header.input_offset + replay_input_byte_count);

	if((replay_input_byte_count + 2) > replay_user_header.input_size) {
		return false;
	}
	if(!file_ropen(replay_user_fn)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	if(
		(file_read(&tag, sizeof(tag)) != sizeof(tag)) ||
		(file_read(&change, sizeof(change)) != sizeof(change))
	) {
		file_close();
		return false;
	}
	replay_input_byte_count += 2;
	replay_rle_phase = static_cast<uint8_t>(
		tag >> T3_REPLAY_PACKET_PHASE_SHIFT
	);
	replay_rle_run = static_cast<uint8_t>(
		(tag & T3_REPLAY_PACKET_RUN_MASK) + 1
	);
	if(replay_rle_phase > T3_REPLAY_PACKET_PHASE_INTERSTITIAL) {
		file_close();
		return false;
	}
	if(change & T3_REPLAY_PACKET_CHANGE_P1) {
		if(
			(replay_input_byte_count + sizeof(replay_rle_input_mp_p1)) >
			replay_user_header.input_size
		) {
			file_close();
			return false;
		}
		if(
			file_read(
				&replay_rle_input_mp_p1, sizeof(replay_rle_input_mp_p1)
			) != sizeof(replay_rle_input_mp_p1)
		) {
			file_close();
			return false;
		}
		replay_input_byte_count += sizeof(replay_rle_input_mp_p1);
	}
	if(change & T3_REPLAY_PACKET_CHANGE_P2) {
		if(
			(replay_input_byte_count + sizeof(replay_rle_input_mp_p2)) >
			replay_user_header.input_size
		) {
			file_close();
			return false;
		}
		if(
			file_read(
				&replay_rle_input_mp_p2, sizeof(replay_rle_input_mp_p2)
			) != sizeof(replay_rle_input_mp_p2)
		) {
			file_close();
			return false;
		}
		replay_input_byte_count += sizeof(replay_rle_input_mp_p2);
	}
	if(change & T3_REPLAY_PACKET_CHANGE_SP) {
		if(
			(replay_input_byte_count + sizeof(replay_rle_input_sp)) >
			replay_user_header.input_size
		) {
			file_close();
			return false;
		}
		if(
			file_read(&replay_rle_input_sp, sizeof(replay_rle_input_sp)) !=
			sizeof(replay_rle_input_sp)
		) {
			file_close();
			return false;
		}
		replay_input_byte_count += sizeof(replay_rle_input_sp);
	}
	if(change & ~(T3_REPLAY_PACKET_CHANGE_P1 | T3_REPLAY_PACKET_CHANGE_P2 | T3_REPLAY_PACKET_CHANGE_SP)) {
		file_close();
		return false;
	}
	file_close();
	return true;
}

static bool mainl_replay_play_rle_sample(void)
{
	if(replay_sample_count >= replay_user_header.sample_count) {
		return false;
	}
	if(replay_rle_run == 0) {
		if(!mainl_replay_read_rle_packet()) {
			return false;
		}
	}
	if(replay_rle_phase != T3_REPLAY_PACKET_PHASE_INTERSTITIAL) {
		return false;
	}
	input_mp_p1 = replay_rle_input_mp_p1;
	input_mp_p2 = replay_rle_input_mp_p2;
	input_sp = replay_rle_input_sp;
	replay_rle_run--;
	replay_sample_count++;
	return true;
}

static void mainl_replay_frame_io(void)
{
	bool ok = true;

	if(
		(mainl_replay_mode != MR_USER_RECORD) &&
		(mainl_replay_mode != MR_USER_PLAYBACK)
	) {
		return;
	}

	if(mainl_replay_mode == MR_USER_RECORD) {
		if(replay_user_header.sample_size == T3_REPLAY_USER_SAMPLE_SIZE_RLE) {
			ok = mainl_replay_record_rle_sample();
		} else {
			ok = mainl_replay_record_sample();
		}
	} else {
		if(replay_user_header.sample_size == T3_REPLAY_USER_SAMPLE_SIZE_RLE) {
			ok = mainl_replay_play_rle_sample();
		} else {
			ok = mainl_replay_play_sample();
		}
	}

	if(!ok) {
		mainl_replay_mode = MR_ERROR;
		input_sp = INPUT_OK;
		return;
	}
	replay_global_frame++;
	mainl_replay_cursor_store();
}

void far mainl_replay_session_start(void)
{
	replay_protect_local_reset();
	mainl_replay_mode = mainl_replay_resident_mode();
	replay_sample_count = mainl_replay_handoff_u32_read(
		T3_REPLAY_RES_SAMPLE_COUNT_INDEX
	);
	replay_global_frame = mainl_replay_handoff_u32_read(
		T3_REPLAY_RES_GLOBAL_FRAME_INDEX
	);
	replay_input_byte_count = mainl_replay_handoff_u32_read(
		T3_REPLAY_RES_INPUT_SIZE_INDEX
	);
	replay_packet_tag_offset = 0;
	replay_rle_phase = T3_REPLAY_PACKET_PHASE_INTERSTITIAL;
	replay_rle_run = 0;
	replay_rle_input_mp_p1 = 0;
	replay_rle_input_mp_p2 = 0;
	replay_rle_input_sp = 0;
	replay_rle_packet_open = false;

	if(
		(mainl_replay_mode == MR_DISABLED) ||
		(replay_sample_count == 0)
	) {
		mainl_replay_mode = MR_DISABLED;
		return;
	}

	mainl_replay_paths_init();
	if(!mainl_replay_user_header_read()) {
		mainl_replay_mode = MR_ERROR;
	} else if(mainl_replay_mode == MR_USER_RECORD) {
		mainl_replay_guard_verify();
	}
}

void far mainl_replay_input_mode_interface(void)
{
	input_mode_interface();
	mainl_replay_frame_io();
}

bool far mainl_replay_initial_stage_splash_skip(void)
{
	return (
		(mainl_replay_resident_mode() == MR_USER_PLAYBACK) &&
		(mainl_replay_handoff_u32_read(T3_REPLAY_RES_SAMPLE_COUNT_INDEX) == 0)
	);
}

void far mainl_replay_finish(replay_user_end_reason_t end_reason)
{
	if(mainl_replay_mode == MR_USER_RECORD) {
		if(replay_protect_invalid()) {
			dos_axdx(0x4100, replay_user_fn);
			mainl_replay_user_index_clear();
		} else {
			mainl_replay_user_header_write(RUS_FINALIZED, end_reason);
		}
		mainl_replay_guard_delete();
	} else if(mainl_replay_mode == MR_USER_PLAYBACK) {
		(void)end_reason;
	}
	if(
		(mainl_replay_mode == MR_USER_RECORD) ||
		(mainl_replay_mode == MR_USER_PLAYBACK)
	) {
		replay_protect_local_free();
		mainl_replay_handoff_clear();
	}
	mainl_replay_mode = MR_DISABLED;
}
