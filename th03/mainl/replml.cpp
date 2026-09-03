#pragma option -zCREPLAYL_TEXT

#include "libs/master.lib/master.hpp"
#include "platform.h"
#include "th03/hardware/input.h"
#include "th03/core/initexit.h"
#include "th03/mainl/replay.hpp"
#include "th03/language.hpp"
#include "th03/lngml.hpp"
#include "th03/keyconfig.hpp"
#include "th03/practice.hpp"
#include "th03/pixel_capture.hpp"
#include "th03/replay_build.hpp"
#include "th03/replay_handoff.hpp"
#include "th03/resident.hpp"
#include "th03/replay_protect.hpp"
#include "th03/scorefile.hpp"
#include "th03/snd/snd.h"

// Pack the pending packet size into unused RLE phase bits.
#define REPLAY_RLE_PHASE_MASK 0x03
#define REPLAY_RLE_PACKET_SIZE_SHIFT 2
#define REPLAY_RLE_PACKET_SIZE_MASK 0x3C
#define REPLAY_RLE_CHARGE_SHIFT 6
#define REPLAY_RLE_CHARGE_MASK 0xC0
#define replay_control_pending \
	resident->unused_3[T3_REPLAY_RES_MAINL_CONTROL_INDEX]
#define mainl_replay_input_vsync (*reinterpret_cast<uint16_t far *>( \
	&resident->unused_3[T3_REPLAY_RES_MAINL_VSYNC_INDEX] \
))

static char T3_USER_REPLAY_INDEX_FN[16];
static char T3_USER_REPLAY_SLOT_FN[18];
static char T3_USER_REPLAY_FALLBACK_FN[12];

void far mainl_staffroll_fade_wait(void)
{
	const uint16_t start = vsync_Count2;

	if(!snd_bgm_active()) {
		return;
	}
	do {
		if(static_cast<uint8_t>(
			snd_kaja_func(KAJA_GET_VOLUME, 0)
		) == 0xFF) {
			return;
		}
	} while(static_cast<uint16_t>(vsync_Count2 - start) < 300);
}

int MASTER_RET mainl_language_file_ropen(const char MASTER_PTR *filename)
{
	(void)language_archive_begin_if_translated(filename);
	return file_ropen(filename);
}

void MASTER_RET mainl_language_file_close(void)
{
	file_close();
	// screens.cpp only streams translated win-quote files through this wrapper.
	language_archive_end(language_is_english());
}

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
static uint16_t replay_write_buffer_size;
static uint16_t replay_write_buffer_seg;
static uint8_t replay_rle_phase;
static uint8_t replay_rle_run;
static uint16_t replay_rle_input_mp_p1;
static uint16_t replay_rle_input_mp_p2;
static uint16_t replay_rle_input_sp;
static bool replay_paths_initialized;
static bool replay_rle_packet_open;
extern "C" unsigned int TextShown;

static bool mainl_replay_preroll_active(void)
{
	return (
		(mainl_replay_mode == MR_USER_PLAYBACK) &&
		(resident->unused_3[T3_REPLAY_RES_PREROLL_TARGET_INDEX] != 0)
	);
}

static void mainl_replay_preroll_se_suppress(bool suppress)
{
	uint8_t far *se_driver_call = (
		reinterpret_cast<uint8_t far *>(snd_se_update) +
		T3_REPLAY_SND_SE_UPDATE_INT_OFFSET
	);

	se_driver_call[0] = (suppress ? 0x90 : 0xCD);
	se_driver_call[1] = (suppress ? 0x90 : PMD);
}

static void mainl_replay_preroll_audio_mask_raw(bool mask)
{
	mainl_replay_preroll_se_suppress(mask);
	if(snd_active || snd_fm_possible) {
		_AL = (mask ? 0xFF : 0);
		_AH = PMD_SET_VOLUME;
		geninterrupt(PMD);
	}
}

static void mainl_replay_preroll_audio_mask(bool mask)
{
	asm { pushf; cli; }
	mainl_replay_preroll_audio_mask_raw(mask);
	asm { popf; }
}

static void mainl_replay_preroll_display_hide(void)
{
	mainl_replay_preroll_audio_mask(true);
	asm {
		mov	ah, 41h
		int	18h
		mov	ah, 0Dh
		int	18h
	}
	TextShown = false;
}

static void mainl_replay_preroll_display_show(void)
{
	asm {
		mov	ah, 40h
		int	18h
		mov	ah, 0Ch
		int	18h
	}
	TextShown = true;
	mainl_replay_preroll_audio_mask(false);
}

static void mainl_replay_preroll_audio_refresh(void)
{
	if(
		!mainl_replay_preroll_active() ||
		(!snd_active && !snd_fm_possible)
	) {
		return;
	}
	_AH = KAJA_GET_VOLUME;
	geninterrupt(PMD);
	if(_AL != 0xFF) {
		// Song starts reset PMD's volume. It therefore doubles as a cheap
		// once-per-MAINL-frame mute sentinel.
		mainl_replay_preroll_audio_mask(true);
	}
}

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
	// The terminal Game Over handoff is the uppercase playback marker.
	if(
		(resident->unused_3[T3_REPLAY_RES_MODE_INDEX] | 0x20) ==
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

static void mainl_replay_handoff_mode_set(uint8_t mode)
{
	resident->unused_3[T3_REPLAY_RES_MODE_INDEX] = mode;
}

uint8_t far mainl_replay_resume_take(void)
{
	uint8_t mode;

	if(
		(resident->unused_3[0] != T3_REPLAY_RES_MAGIC_0) ||
		(resident->unused_3[1] != T3_REPLAY_RES_MAGIC_1) ||
		(resident->unused_3[2] != T3_REPLAY_RES_MAGIC_2) ||
		(resident->unused_3[3] != T3_REPLAY_RES_MAGIC_3)
	) {
		return 0;
	}
	mode = mainl_replay_handoff_u8(T3_REPLAY_RES_MODE_INDEX);
	if(mode == T3R_RES_MODE_USER_GAME_OVER) {
		return mode;
	}
	if(
		(mode != T3_REPLAY_RES_MODE_RESUME_GAME_OVER) &&
		(mode != T3_REPLAY_RES_MODE_RESUME_CLEAR)
	) {
		return 0;
	}
	mainl_replay_handoff_clear();
	return mode;
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

static bool mainl_replay_guard_checkpoint(void)
{
	char guard_fn[13];

	mainl_replay_guard_fn_set(guard_fn);
	if(!replay_protect_checkpoint(guard_fn)) {
		if(replay_protect_invalid()) {
			replay_protect_guard_marker_set(guard_fn);
		}
		return false;
	}
	return true;
}

static void mainl_replay_diag_code_set_if_none(uint8_t code)
{
	if(mainl_replay_handoff_u8(T3R_DIAG_CODE_INDEX) == RPD_NONE) {
		replay_protect_diag_code_set(code);
	}
}

static void mainl_replay_guard_buffer_prepare(void)
{
	replay_protect_ctx_t ctx;

	if(!replay_protect_ctx_alloc(&ctx)) {
		replay_protect_detector_error_set();
	}
}

static bool mainl_replay_guard_create(void)
{
	char guard_fn[13];

	mainl_replay_guard_fn_set(guard_fn);
	replay_protect_file_delete_commit(guard_fn);
	return replay_protect_guard_create(guard_fn);
}

static bool mainl_replay_record_prepare(void)
{
	mainl_replay_user_slot_fn_set(mainl_replay_resident_slot());
	if(!file_create(replay_user_fn)) {
		replay_protect_detector_error_set();
		return false;
	}
	if(!replay_protect_close_current_file(RPD_COMMIT_FLUSH)) {
		return false;
	}
	return mainl_replay_guard_create();
}

static void mainl_replay_guard_delete(void)
{
	char guard_fn[13];

	mainl_replay_guard_fn_set(guard_fn);
	replay_protect_file_delete_commit(guard_fn);
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
		(replay_user_header.magic[6] == '1') &&
		(
			replay_user_header.magic[7] == static_cast<char>(
				'0' + (replay_user_header.version - 10)
			)
		) &&
		replay_user_version_supported(replay_user_header.version) &&
		(replay_user_header.sample_size == T3_REPLAY_USER_SAMPLE_SIZE_RLE) &&
		((replay_user_header.flags & T3_REPLAY_USER_FLAG_RLE_INPUT) != 0) &&
		((replay_user_header.flags & T3_REPLAY_USER_FLAG_CHARGE_INPUT) != 0) &&
		(
			(
				(replay_user_header.flags & T3_REPLAY_USER_FLAG_PRACTICE) &&
				(replay_user_header.game_mode == GM_VS_1P_CPU) &&
				practice_replay_config_valid(
					replay_user_header.scenario.practice.config
				)
			) ||
			((replay_user_header.flags & T3_REPLAY_USER_FLAG_PRACTICE) == 0)
		) &&
		(replay_user_header.autofire <= 0x03) &&
		(replay_user_header.header_size == replay_user_header_size(
			replay_user_header.version
		)) &&
		(replay_user_header.snapshot_offset == replay_user_header.header_size) &&
		(replay_user_header.snapshot_size == replay_user_checkpoint_size(
			replay_user_header.version
		)) &&
		(replay_user_header.input_offset == replay_user_input_offset(
			replay_user_header.version,
			replay_user_header.game_mode,
			replay_user_header.flags
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
	for(i = 0; i < T3_REPLAY_USER_NAME_LEN; i++) {
		replay_user_index_entry.name[i] = replay_user_header.name[i];
	}
	replay_user_index_entry.dos_date = replay_user_header.dos_date;
	replay_user_index_entry.autofire = replay_user_header.autofire;
	replay_user_index_entry.replay_flags = static_cast<uint8_t>(
		replay_user_header.flags & T3_REPLAY_USER_FLAG_PRACTICE
	);
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
	if((replay_user_header.flags & T3_REPLAY_USER_FLAG_PRACTICE) == 0) {
		for(i = 0; i < T3_REPLAY_USER_STAGE_COUNT; i++) {
			replay_user_index_entry.stage_opponents[i] = (
				replay_user_header.scenario.story.stage_opponents[i]
			);
		}
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
	uint8_t far *write_buffer = reinterpret_cast<uint8_t far *>(
		MK_FP(replay_write_buffer_seg, 0)
	);

	if(replay_protect_blocked()) {
		return false;
	}
	if(!mainl_replay_guard_checkpoint()) {
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
	if(replay_write_buffer_size != 0) {
		file_seek(
			(replay_user_header.input_offset + replay_input_byte_count -
			 replay_write_buffer_size),
			SEEK_SET
		);
		if(!mainl_replay_write_bytes_checked(
			write_buffer, replay_write_buffer_size
		)) {
			file_close();
			replay_protect_detector_error_set();
			return false;
		}
	}
	if(!replay_protect_close_current_file(RPD_COMMIT_FLUSH)) {
		return false;
	}
	replay_write_buffer_size = 0;
	replay_rle_packet_open = false;
	mainl_replay_user_index_write(status, end_reason);
	return true;
}

static bool mainl_replay_periodic_flush(void)
{
	if(!mainl_replay_user_header_write(RUS_RECORDING, RUER_PARTIAL)) {
		mainl_replay_diag_code_set_if_none(RPD_MAINL_PERIODIC_WRITE);
		return false;
	}
	return true;
}

static bool mainl_replay_buffer_reserve(unsigned size)
{
	if((replay_write_buffer_size + size) <= T3_REPLAY_WRITE_BUFFER_SIZE) {
		return true;
	}
	if(
		mainl_replay_periodic_flush() &&
		((replay_write_buffer_size + size) <= T3_REPLAY_WRITE_BUFFER_SIZE)
	) {
		return true;
	}
	mainl_replay_diag_code_set_if_none(RPD_MAINL_BUFFER_FULL);
	replay_protect_detector_error_set();
	return false;
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

static bool mainl_replay_buffer_u8(uint8_t value)
{
	uint8_t far *write_buffer = reinterpret_cast<uint8_t far *>(
		MK_FP(replay_write_buffer_seg, 0)
	);

	if(replay_write_buffer_size >= T3_REPLAY_WRITE_BUFFER_SIZE) {
		return false;
	}
	write_buffer[replay_write_buffer_size++] = value;
	replay_input_byte_count++;
	return true;
}

static bool mainl_replay_buffer_u16(uint16_t value)
{
	uint8_t far *write_buffer = reinterpret_cast<uint8_t far *>(
		MK_FP(replay_write_buffer_seg, 0)
	);

	if(
		(replay_write_buffer_size + sizeof(value)) >
		T3_REPLAY_WRITE_BUFFER_SIZE
	) {
		return false;
	}
	write_buffer[replay_write_buffer_size++] = static_cast<uint8_t>(value);
	write_buffer[replay_write_buffer_size++] = static_cast<uint8_t>(
		value >> 8
	);
	replay_input_byte_count += sizeof(value);
	return true;
}

static bool mainl_replay_control_write(uint8_t control)
{
	uint8_t tag = static_cast<uint8_t>(
		(T3_REPLAY_PACKET_PHASE_CONTROL << T3_REPLAY_PACKET_PHASE_SHIFT) |
		control
	);
	uint8_t marker = T3_REPLAY_PACKET_CONTROL_MARKER;
	if(replay_protect_blocked()) {
		return true;
	}
	if(!mainl_replay_buffer_reserve(2)) {
		return false;
	}
	if(!mainl_replay_buffer_u8(tag) || !mainl_replay_buffer_u8(marker)) {
		replay_protect_detector_error_set();
		return false;
	}
	replay_rle_packet_open = false;
	return true;
}

static bool mainl_replay_control_peek(uint8_t control)
{
	uint8_t tag;
	uint8_t marker;
	uint32_t offset = (replay_user_header.input_offset + replay_input_byte_count);

	if(
		(replay_rle_run != 0) ||
		((replay_input_byte_count + 2) > replay_user_header.input_size)
	) {
		return false;
	}
	if(!file_ropen(replay_user_fn)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	if(
		(file_read(&tag, sizeof(tag)) != sizeof(tag)) ||
		(file_read(&marker, sizeof(marker)) != sizeof(marker))
	) {
		file_close();
		return false;
	}
	file_close();
	return (
		(tag == static_cast<uint8_t>(
			(T3_REPLAY_PACKET_PHASE_CONTROL << T3_REPLAY_PACKET_PHASE_SHIFT) |
			control
		)) &&
		(marker == T3_REPLAY_PACKET_CONTROL_MARKER)
	);
}

static bool mainl_replay_control_consume(uint8_t control)
{
	if(!mainl_replay_control_peek(control)) {
		return false;
	}
	replay_input_byte_count += 2;
	replay_control_pending = false;
	return true;
}

static bool mainl_replay_record_rle_sample(void)
{
	uint16_t input_p1 = input_mp_p1;
	uint16_t input_p2 = input_mp_p2;
	uint16_t input_single = input_sp;
	uint8_t input_charge = (resident->input_charge & 0x03);
	uint8_t previous_charge = static_cast<uint8_t>(
		(replay_rle_phase & REPLAY_RLE_CHARGE_MASK) >>
		REPLAY_RLE_CHARGE_SHIFT
	);
	uint8_t packet_size = static_cast<uint8_t>(
		(replay_rle_phase & REPLAY_RLE_PACKET_SIZE_MASK) >>
		REPLAY_RLE_PACKET_SIZE_SHIFT
	);
	uint8_t far *write_buffer = reinterpret_cast<uint8_t far *>(
		MK_FP(replay_write_buffer_seg, 0)
	);
	uint8_t change = 0;
	uint8_t tag;

	if(replay_protect_blocked()) {
		return true;
	}
	if(!mainl_replay_buffer_reserve(T3_REPLAY_PACKET_SIZE_MAX)) {
		return true;
	}

	if(
		replay_rle_packet_open &&
		(
			(replay_rle_phase & REPLAY_RLE_PHASE_MASK) ==
			T3_REPLAY_PACKET_PHASE_INTERSTITIAL
		) &&
		(replay_rle_input_mp_p1 == input_p1) &&
		(replay_rle_input_mp_p2 == input_p2) &&
		(replay_rle_input_sp == input_single) &&
		(previous_charge == input_charge) &&
		(replay_rle_run < T3_REPLAY_PACKET_RUN_MAX)
	) {
		tag = mainl_replay_rle_tag(
			T3_REPLAY_PACKET_PHASE_INTERSTITIAL, replay_rle_run + 1
		);
		replay_rle_run++;
		write_buffer[replay_write_buffer_size - packet_size] = tag;
		replay_sample_count++;
		return true;
	}

	if(!replay_rle_packet_open) {
		change = (
			T3_REPLAY_PACKET_CHANGE_P1 |
			T3_REPLAY_PACKET_CHANGE_P2 |
			T3_REPLAY_PACKET_CHANGE_SP |
			T3_REPLAY_PACKET_CHANGE_CHARGE
		);
	} else {
		if(input_p1 != replay_rle_input_mp_p1) {
			change |= T3_REPLAY_PACKET_CHANGE_P1;
		}
		if(input_p2 != replay_rle_input_mp_p2) {
			change |= T3_REPLAY_PACKET_CHANGE_P2;
		}
		if(input_single != replay_rle_input_sp) {
			change |= T3_REPLAY_PACKET_CHANGE_SP;
		}
		if(input_charge != previous_charge) {
			change |= T3_REPLAY_PACKET_CHANGE_CHARGE;
		}
	}

	tag = mainl_replay_rle_tag(T3_REPLAY_PACKET_PHASE_INTERSTITIAL, 1);
	packet_size = replay_write_buffer_size;
	if(!mainl_replay_buffer_u8(tag) || !mainl_replay_buffer_u8(change)) {
		replay_protect_detector_error_set();
		return true;
	}
	if(change & T3_REPLAY_PACKET_CHANGE_P1) {
		if(!mainl_replay_buffer_u16(input_p1)) {
			replay_protect_detector_error_set();
			return true;
		}
	}
	if(change & T3_REPLAY_PACKET_CHANGE_P2) {
		if(!mainl_replay_buffer_u16(input_p2)) {
			replay_protect_detector_error_set();
			return true;
		}
	}
	if(change & T3_REPLAY_PACKET_CHANGE_SP) {
		if(!mainl_replay_buffer_u16(input_single)) {
			replay_protect_detector_error_set();
			return true;
		}
	}
	if(change & T3_REPLAY_PACKET_CHANGE_CHARGE) {
		if(!mainl_replay_buffer_u8(input_charge)) {
			replay_protect_detector_error_set();
			return true;
		}
	}
	packet_size = static_cast<uint8_t>(replay_write_buffer_size - packet_size);
	replay_rle_phase = static_cast<uint8_t>(
		T3_REPLAY_PACKET_PHASE_INTERSTITIAL |
		(packet_size << REPLAY_RLE_PACKET_SIZE_SHIFT) |
		(input_charge << REPLAY_RLE_CHARGE_SHIFT)
	);
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
	uint8_t phase;
	uint8_t charge_input = static_cast<uint8_t>(
		(replay_rle_phase & REPLAY_RLE_CHARGE_MASK) >>
		REPLAY_RLE_CHARGE_SHIFT
	);
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
	phase = static_cast<uint8_t>(tag >> T3_REPLAY_PACKET_PHASE_SHIFT);
	replay_rle_phase = static_cast<uint8_t>(
		(replay_rle_phase & REPLAY_RLE_CHARGE_MASK) | phase
	);
	replay_rle_run = static_cast<uint8_t>(
		(tag & T3_REPLAY_PACKET_RUN_MASK) + 1
	);
	if(phase > T3_REPLAY_PACKET_PHASE_INTERSTITIAL) {
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
	if(change & T3_REPLAY_PACKET_CHANGE_CHARGE) {
		if(
			(replay_input_byte_count + sizeof(charge_input)) >
			replay_user_header.input_size
		) {
			file_close();
			return false;
		}
		if(
			file_read(
				&charge_input, sizeof(charge_input)
			) != sizeof(charge_input)
		) {
			file_close();
			return false;
		}
		replay_input_byte_count += sizeof(charge_input);
	}
	if(charge_input > 0x03) {
		file_close();
		return false;
	}
	replay_rle_phase = static_cast<uint8_t>(
		phase | (charge_input << REPLAY_RLE_CHARGE_SHIFT)
	);
	if(change & ~(
		T3_REPLAY_PACKET_CHANGE_P1 |
		T3_REPLAY_PACKET_CHANGE_P2 |
		T3_REPLAY_PACKET_CHANGE_SP |
		T3_REPLAY_PACKET_CHANGE_CHARGE
	)) {
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
	if(
		(replay_rle_phase & REPLAY_RLE_PHASE_MASK) !=
		T3_REPLAY_PACKET_PHASE_INTERSTITIAL
	) {
		return false;
	}
	input_mp_p1 = replay_rle_input_mp_p1;
	input_mp_p2 = replay_rle_input_mp_p2;
	input_sp = replay_rle_input_sp;
	resident->input_charge = static_cast<uint8_t>(
		(replay_rle_phase & REPLAY_RLE_CHARGE_MASK) >>
		REPLAY_RLE_CHARGE_SHIFT
	);
	replay_rle_run--;
	replay_sample_count++;
	return true;
}

static void mainl_replay_frame_io(void)
{
	bool ok = true;
	scorestat_process_sync();
	t3pix_logical_identity_set(
		replay_sample_count, replay_global_frame, T3PIX_ID_NONE
	);

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
		if(
			(replay_user_header.sample_size == T3_REPLAY_USER_SAMPLE_SIZE_RLE) &&
			(
				replay_control_pending ||
				(
					(replay_rle_run == 0) &&
					mainl_replay_control_peek(
						T3_REPLAY_PACKET_CONTROL_MAINL_END
					)
				)
			)
		) {
			// MAINL can reach a blocking "release, then press" input loop one
			// VSync later than it did while recording. Alternate a synthetic
			// release with Shot after the payload ends.
			replay_control_pending ^= INPUT_SHOT;
			replay_rle_input_sp = (
				replay_control_pending ^ INPUT_SHOT
			);
			return;
		}
		if(replay_user_header.sample_size == T3_REPLAY_USER_SAMPLE_SIZE_RLE) {
			ok = mainl_replay_play_rle_sample();
		} else {
			ok = mainl_replay_play_sample();
		}
	}

	if(!ok) {
		if(mainl_replay_mode == MR_USER_PLAYBACK) {
			if(mainl_replay_preroll_active()) {
				mainl_replay_preroll_display_show();
			}
			resident->game_mode = GM_NONE;
		}
		mainl_replay_mode = MR_ERROR;
		input_sp = INPUT_OK;
		return;
	}
	if(
		(mainl_replay_mode == MR_USER_RECORD) &&
		(
			(replay_global_frame & (T3_REPLAY_DISK_INTERVAL_SAMPLES - 1)) ==
			(T3_REPLAY_DISK_INTERVAL_SAMPLES - 1)
		)
	) {
		mainl_replay_periodic_flush();
	}
	replay_global_frame++;
	t3pix_logical_identity_set(
		replay_sample_count, replay_global_frame, T3PIX_ID_NONE
	);
	mainl_replay_cursor_store();
}

void far mainl_replay_session_start(void)
{
	scorestat_process_enter();
	language_mainl_apply();
	replay_protect_local_reset();
	mainl_replay_mode = mainl_replay_resident_mode();
	if(
		(resident->game_mode == GM_STORY) &&
		(mainl_replay_mode == MR_USER_RECORD) &&
		!scorestat_active()
	) {
		scorestat_run_begin();
	}
	replay_sample_count = mainl_replay_handoff_u32_read(
		T3_REPLAY_RES_SAMPLE_COUNT_INDEX
	);
	replay_global_frame = mainl_replay_handoff_u32_read(
		T3_REPLAY_RES_GLOBAL_FRAME_INDEX
	);
	t3pix_logical_identity_set(
		replay_sample_count, replay_global_frame, T3PIX_ID_NONE
	);
	replay_input_byte_count = mainl_replay_handoff_u32_read(
		T3_REPLAY_RES_INPUT_SIZE_INDEX
	);
	replay_write_buffer_size = 0;
	replay_write_buffer_seg = 0;
	replay_rle_phase = T3_REPLAY_PACKET_PHASE_INTERSTITIAL;
	replay_rle_run = 0;
	replay_rle_input_mp_p1 = 0;
	replay_rle_input_mp_p2 = 0;
	replay_rle_input_sp = 0;
	mainl_replay_input_vsync = (vsync_Count2 - 1);
	resident->input_charge = 0;
	replay_rle_packet_open = false;
	replay_control_pending = false;

	if(
		(mainl_replay_mode == MR_USER_RECORD) &&
		(replay_sample_count == 0)
	) {
		mainl_replay_paths_init();
		mainl_replay_record_prepare();
		replay_protect_local_free();
		mainl_replay_mode = MR_DISABLED;
		return;
	}
	if(
		(mainl_replay_mode == MR_DISABLED) ||
		(replay_sample_count == 0)
	) {
		mainl_replay_mode = MR_DISABLED;
		return;
	}
	if(mainl_replay_mode == MR_USER_RECORD) {
		mainl_replay_guard_buffer_prepare();
		replay_write_buffer_seg = reinterpret_cast<uint16_t>(
			hmem_allocbyte(T3_REPLAY_WRITE_BUFFER_SIZE)
		);
		if(replay_write_buffer_seg == 0) {
			mainl_replay_diag_code_set_if_none(RPD_MAINL_BUFFER_ALLOC);
			replay_protect_detector_error_set();
		}
	}

	mainl_replay_paths_init();
	if(!mainl_replay_user_header_read()) {
		if(mainl_replay_preroll_active()) {
			mainl_replay_preroll_display_show();
		}
		mainl_replay_mode = MR_ERROR;
	} else if(mainl_replay_mode == MR_USER_PLAYBACK) {
		resident->autofire = replay_user_header.autofire;
		if(mainl_replay_preroll_active()) {
			mainl_replay_preroll_display_hide();
		}
	}
}

void far mainl_replay_input_mode_interface(void)
{
	uint16_t physical_input_sp;

	if(mainl_replay_mode == MR_ERROR) {
		input_mp_p1 = INPUT_NONE;
		input_mp_p2 = INPUT_NONE;
		input_sp = INPUT_OK;
		return;
	}

	input_mode_interface();
	keyconfig_charge_mask_human();
	physical_input_sp = input_sp;
	if(mainl_replay_mode == MR_USER_PLAYBACK) {
		if(physical_input_sp & INPUT_CANCEL) {
			if(mainl_replay_preroll_active()) {
				mainl_replay_preroll_display_show();
			}
			resident->game_mode = GM_NONE;
			mainl_replay_handoff_clear();
			mainl_replay_mode = MR_ERROR;
			input_mp_p1 = INPUT_NONE;
			input_mp_p2 = INPUT_NONE;
			input_sp = INPUT_OK;
			return;
		}
	}
	if(
		(
			(mainl_replay_mode == MR_USER_RECORD) ||
			(mainl_replay_mode == MR_USER_PLAYBACK)
		) &&
		(mainl_replay_input_vsync != vsync_Count2)
	) {
		mainl_replay_preroll_audio_refresh();
		// Match MAIN's temporal input granularity while retaining responsive
		// physical polling in the cutscene interpreter's tight loops.
		mainl_replay_frame_io();
		// Start the next interval after any synchronous replay file access.
		mainl_replay_input_vsync = vsync_Count2;
	}
	if(mainl_replay_mode == MR_USER_PLAYBACK) {
		input_mp_p1 = replay_rle_input_mp_p1;
		input_mp_p2 = replay_rle_input_mp_p2;
		input_sp = replay_rle_input_sp;
		resident->input_charge = static_cast<uint8_t>(
			(replay_rle_phase & REPLAY_RLE_CHARGE_MASK) >>
			REPLAY_RLE_CHARGE_SHIFT
		);
	}
}

static void mainl_replay_transition_finish_impl(bool persist_partial)
{
	bool ok = true;
	mainl_replay_mode_t mode = mainl_replay_mode;

	if((mode != MR_USER_RECORD) && (mode != MR_USER_PLAYBACK)) {
		return;
	}

	if(mode == MR_USER_RECORD) {
		ok = mainl_replay_control_write(T3_REPLAY_PACKET_CONTROL_MAINL_END);
		if(ok && persist_partial) {
			ok = mainl_replay_user_header_write(RUS_RECORDING, RUER_PARTIAL);
			if(!ok) {
				mainl_replay_diag_code_set_if_none(RPD_MAINL_HEADER_WRITE);
			}
		}
	} else {
		while(
			!replay_control_pending &&
			!mainl_replay_control_peek(T3_REPLAY_PACKET_CONTROL_MAINL_END)
		) {
			if(!mainl_replay_play_rle_sample()) {
				ok = false;
				break;
			}
			replay_global_frame++;
		}
		if(ok) {
			ok = mainl_replay_control_consume(
				T3_REPLAY_PACKET_CONTROL_MAINL_END
			);
		}
	}
	if(!ok) {
		if(mode == MR_USER_PLAYBACK) {
			if(mainl_replay_preroll_active()) {
				mainl_replay_preroll_display_show();
			}
			resident->game_mode = GM_NONE;
			mainl_replay_handoff_clear();
		} else {
			mainl_replay_cursor_store();
		}
		mainl_replay_mode = MR_ERROR;
		input_mp_p1 = INPUT_NONE;
		input_mp_p2 = INPUT_NONE;
		input_sp = INPUT_OK;
		return;
	}
	mainl_replay_cursor_store();
}

void far mainl_replay_transition_finish(void)
{
	mainl_replay_transition_finish_impl(true);
}

bool far mainl_replay_initial_stage_splash_skip(void)
{
	return (
		(mainl_replay_resident_mode() == MR_USER_PLAYBACK) &&
		(
			(mainl_replay_handoff_u32_read(T3_REPLAY_RES_SAMPLE_COUNT_INDEX) == 0) ||
			(mainl_replay_handoff_u8(T3_REPLAY_RES_PLAYBACK_STAGE_INDEX) != 0)
		)
	);
}

bool far mainl_replay_stage_start_selected(void)
{
	uint8_t checkpoint = mainl_replay_handoff_u8(
		T3_REPLAY_RES_PLAYBACK_CHECKPOINT_INDEX
	);

	return (
		(mainl_replay_resident_mode() == MR_USER_PLAYBACK) &&
		(checkpoint != 0) &&
		(checkpoint <= T3R_CKPT_COUNT_MAX)
	);
}

int far mainl_replay_stage_transition_needed(void)
{
	if(practice_game_active() && practice_initial_stage_take()) {
		return false;
	}
#if defined(TH03_REPLAY_DEV_STAGE_SELECT)
	if(resident->unused_3[T3_REPLAY_RES_DEBUG_STAGE_START_INDEX] != 0) {
		resident->unused_3[T3_REPLAY_RES_DEBUG_STAGE_START_INDEX] = 0;
		return false;
	}
#endif
	return (
		(resident->story_stage != 0) &&
		!mainl_replay_stage_start_selected()
	);
}

bool far mainl_replay_finish(
	replay_user_end_reason_t end_reason, uint8_t save_prompt_mode
)
{
	scorestat_exit_checkpoint();
	bool save_pending = false;

	// A terminal transition writes the authoritative finalized header below.
	// Avoid a redundant guard checkpoint, process commit, and partial index write.
	mainl_replay_transition_finish_impl(false);

	if(mainl_replay_mode == MR_USER_RECORD) {
		if(replay_protect_invalid()) {
			replay_protect_file_delete_commit(replay_user_fn);
			mainl_replay_user_index_clear();
		} else if(mainl_replay_user_header_write(RUS_FINALIZED, end_reason)) {
			save_pending = true;
		}
		mainl_replay_guard_delete();
	} else if(mainl_replay_mode == MR_USER_PLAYBACK) {
		(void)end_reason;
		if(mainl_replay_preroll_active()) {
			mainl_replay_preroll_display_show();
		}
		resident->game_mode = GM_NONE;
	}
	if(
		(mainl_replay_mode == MR_USER_RECORD) ||
		(mainl_replay_mode == MR_USER_PLAYBACK)
	) {
		replay_protect_local_free();
		if(save_pending) {
			mainl_replay_handoff_mode_set(save_prompt_mode);
		} else if(mainl_replay_mode == MR_USER_RECORD) {
			mainl_replay_handoff_mode_set(T3_REPLAY_RES_MODE_ACCEL_CLEANUP);
		} else {
			mainl_replay_handoff_clear();
		}
	}
	mainl_replay_mode = MR_DISABLED;
	return save_pending;
}

bool far mainl_replay_clear_playback_finish(void)
{
	if(mainl_replay_mode != MR_USER_PLAYBACK) {
		return false;
	}
	mainl_replay_finish(
		RUER_COMPLETE, T3_REPLAY_RES_MODE_SAVE_PROMPT_CLEAR
	);
	return true;
}

void far mainl_replay_exit_to_main(void)
{
	scorestat_exit_checkpoint();
	if(
		(resident->game_mode == GM_STORY) &&
		(resident->rem_credits < 3) &&
		!scorestat_active()
	) {
		scorestat_continue_accept();
	}
	if(!mainl_replay_stage_start_selected()) {
		mainl_replay_transition_finish();
	}
	game_exit_from_mainl_to_main();
}

// Keep the following shared segment at its accepted paragraph phase.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90"
