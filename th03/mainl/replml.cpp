#pragma option -zCREPLAYL_TEXT

#include "libs/master.lib/master.hpp"
#include "platform.h"
#include "th03/hardware/input.h"
#include "th03/mainl/replay.hpp"
#include "th03/replay_handoff.hpp"
#include "th03/resident.hpp"

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
static bool replay_paths_initialized;

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
}

static void mainl_replay_memclear(void near *buf, unsigned size)
{
	uint8_t near *p = reinterpret_cast<uint8_t near *>(buf);

	while(size != 0) {
		*p++ = 0;
		size--;
	}
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
		i < (T3_REPLAY_RES_GLOBAL_FRAME_INDEX + 4);
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

static bool mainl_replay_user_header_valid(void)
{
	return (
		(replay_user_header.magic[0] == 'T') &&
		(replay_user_header.magic[1] == '3') &&
		(replay_user_header.magic[2] == 'R') &&
		(replay_user_header.magic[3] == 'P') &&
		(replay_user_header.magic[4] == 'L') &&
		(replay_user_header.magic[5] == 'Y') &&
		(replay_user_header.magic[6] == '2') &&
		(replay_user_header.version == T3_REPLAY_USER_VERSION) &&
		(replay_user_header.header_size == sizeof(replay_user_header)) &&
		(replay_user_header.sample_size == sizeof(replay_user_sample_t)) &&
		(replay_user_header.input_offset == (
			static_cast<uint32_t>(sizeof(replay_user_header)) +
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

static bool mainl_replay_user_header_write(
	replay_user_status_t status, replay_user_end_reason_t end_reason
)
{
	replay_user_header.status = status;
	replay_user_header.end_reason = end_reason;
	replay_user_header.sample_count = replay_sample_count;
	replay_user_header.final_frame_count = replay_global_frame;
	replay_user_header.input_size = (
		replay_sample_count * static_cast<uint32_t>(sizeof(replay_user_sample_t))
	);

	if(!file_append(replay_user_fn)) {
		return false;
	}
	file_seek(0, SEEK_SET);
	file_write(&replay_user_header, sizeof(replay_user_header));
	file_close();
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
	if(!file_append(replay_user_fn)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	file_write(&sample, sizeof(sample));
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
		ok = mainl_replay_record_sample();
	} else {
		ok = mainl_replay_play_sample();
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
	mainl_replay_mode = mainl_replay_resident_mode();
	replay_sample_count = mainl_replay_handoff_u32_read(
		T3_REPLAY_RES_SAMPLE_COUNT_INDEX
	);
	replay_global_frame = mainl_replay_handoff_u32_read(
		T3_REPLAY_RES_GLOBAL_FRAME_INDEX
	);

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
		mainl_replay_user_header_write(RUS_FINALIZED, end_reason);
	} else if(mainl_replay_mode == MR_USER_PLAYBACK) {
		(void)end_reason;
	}
	if(
		(mainl_replay_mode == MR_USER_RECORD) ||
		(mainl_replay_mode == MR_USER_PLAYBACK)
	) {
		mainl_replay_handoff_clear();
	}
	mainl_replay_mode = MR_DISABLED;
}
