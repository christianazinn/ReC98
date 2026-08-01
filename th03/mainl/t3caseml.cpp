#pragma option -zCT3CASE_MAINL_TEXT

// T3CASE1 adapter for TH03 MAINL. Validation-only, intentionally nonmatching.
//
// MAINL senses physical input repeatedly inside presentation loops. V11 and
// the Replay Patch collapse those polls to one phase-1 logical sample per
// hardware VSync, then drain any remaining samples at the natural process
// transition. Keep this module semantically parallel to mainl/replml.cpp;
// T3CASE is an internal verifier transport, not another user replay format.

#include "libs/master.lib/master.hpp"
#include "platform.h"
#include "th02/hardware/frmdelay.h"
#include "th03/hardware/input.h"
#include "th03/mainl/t3case.hpp"
#include "th03/resident.hpp"
#include "th03/snd/snd.h"
#include "th03/t3case.hpp"
#include "x86real.h"

enum t3case_mainl_mode_t {
	T3CASE_MAINL_DISABLED = 0,
	T3CASE_MAINL_RECORD   = 1,
	T3CASE_MAINL_PLAYBACK = 2,
	T3CASE_MAINL_ERROR    = 3,
};

static char T3CASE_BIN_FN[11];
static char T3CASE_DONE_FN[11];
static char T3CASE_DIAG_FN[11];
static bool t3case_paths_ready;

static t3case_header_t t3case_header;
static t3case_startup_t t3case_startup;
static t3case_snapshot_t t3case_snapshot;
static t3case_mainl_mode_t t3case_mode;
static uint32_t t3case_global_frame;
static uint32_t t3case_sample_count;
static uint32_t t3case_record_count;
static uint32_t t3case_payload_checksum;
static uint16_t t3case_input_vsync;
static input_t t3case_input_mp_p1;
static input_t t3case_input_mp_p2;
static input_t t3case_input_sp;
static bool t3case_control_pending;
static bool t3case_transition_finished;
static bool t3case_done_written;

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
	T3CASE_BIN_FN[0] = 'T'; T3CASE_BIN_FN[1] = '3'; T3CASE_BIN_FN[2] = 'C';
	T3CASE_BIN_FN[3] = 'A'; T3CASE_BIN_FN[4] = 'S'; T3CASE_BIN_FN[5] = 'E';
	T3CASE_BIN_FN[6] = '.'; T3CASE_BIN_FN[7] = 'B'; T3CASE_BIN_FN[8] = 'I';
	T3CASE_BIN_FN[9] = 'N'; T3CASE_BIN_FN[10] = '\0';
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

static void t3case_diag(char t0, char t1, char t2, uint32_t a, uint32_t b)
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

static void t3case_done_write(bool ok)
{
	if(t3case_done_written) {
		return;
	}
	t3case_paths_init();
	if(file_create(T3CASE_DONE_FN)) {
		if(ok) {
			t3case_write_char('o'); t3case_write_char('k');
			t3case_write_char(':');
			if(t3case_mode == T3CASE_MAINL_RECORD) {
				t3case_write_char('r'); t3case_write_char('e');
				t3case_write_char('c'); t3case_write_char('o');
				t3case_write_char('r'); t3case_write_char('d');
			} else {
				t3case_write_char('p'); t3case_write_char('l');
				t3case_write_char('a'); t3case_write_char('y');
				t3case_write_char('b'); t3case_write_char('a');
				t3case_write_char('c'); t3case_write_char('k');
			}
		} else {
			t3case_write_char('e'); t3case_write_char('r');
			t3case_write_char('r'); t3case_write_char('o');
			t3case_write_char('r'); t3case_write_char(':');
			t3case_write_char('m'); t3case_write_char('a');
			t3case_write_char('i'); t3case_write_char('n');
			t3case_write_char('l');
		}
		t3case_write_char('\r');
		t3case_write_char('\n');
		file_close();
	}
	t3case_done_written = true;
}

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

static t3case_mainl_mode_t t3case_resident_mode(void)
{
	if(
		(resident->unused_3[0] != T3CASE_RES_MAGIC_0) ||
		(resident->unused_3[1] != T3CASE_RES_MAGIC_1) ||
		(resident->unused_3[2] != T3CASE_RES_MAGIC_2) ||
		(resident->unused_3[3] != T3CASE_RES_MAGIC_3)
	) {
		return T3CASE_MAINL_DISABLED;
	}
	if(resident->unused_3[T3CASE_RES_MODE_INDEX] == 'r') {
		return T3CASE_MAINL_RECORD;
	}
	if(resident->unused_3[T3CASE_RES_MODE_INDEX] == 'p') {
		return T3CASE_MAINL_PLAYBACK;
	}
	return T3CASE_MAINL_DISABLED;
}

static void t3case_handoff_load(void)
{
	t3case_sample_count = t3case_handoff_u32_read(T3CASE_RES_SAMPLES_INDEX);
	t3case_global_frame = t3case_handoff_u32_read(T3CASE_RES_FRAME_INDEX);
	t3case_record_count = t3case_handoff_u32_read(T3CASE_RES_RECORDS_INDEX);
	t3case_payload_checksum = t3case_handoff_u32_read(
		T3CASE_RES_CHECKSUM_INDEX
	);
}

static void t3case_handoff_store(void)
{
	resident->unused_3[0] = T3CASE_RES_MAGIC_0;
	resident->unused_3[1] = T3CASE_RES_MAGIC_1;
	resident->unused_3[2] = T3CASE_RES_MAGIC_2;
	resident->unused_3[3] = T3CASE_RES_MAGIC_3;
	resident->unused_3[T3CASE_RES_MODE_INDEX] = (
		(t3case_mode == T3CASE_MAINL_RECORD) ? 'r' : 'p'
	);
	t3case_handoff_u32_write(T3CASE_RES_SAMPLES_INDEX, t3case_sample_count);
	t3case_handoff_u32_write(T3CASE_RES_FRAME_INDEX, t3case_global_frame);
	t3case_handoff_u32_write(T3CASE_RES_RECORDS_INDEX, t3case_record_count);
	t3case_handoff_u32_write(
		T3CASE_RES_CHECKSUM_INDEX, t3case_payload_checksum
	);
}

static void t3case_handoff_clear(void)
{
	resident->unused_3[0] = 0;
	resident->unused_3[T3CASE_RES_MODE_INDEX] = 0;
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

static bool t3case_header_read(void)
{
	uint32_t stored;
	uint32_t computed;
	unsigned i;

	if(!file_ropen(T3CASE_BIN_FN)) {
		return false;
	}
	if(file_read(&t3case_header, sizeof(t3case_header)) != sizeof(t3case_header)) {
		file_close();
		return false;
	}
	if(file_read(&t3case_startup, sizeof(t3case_startup)) != sizeof(t3case_startup)) {
		file_close();
		return false;
	}
	t3case_memclear(&t3case_snapshot, sizeof(t3case_snapshot));
	if(
		(t3case_header.flags & T3CASE_FLAG_SNAPSHOT) &&
		(file_read(&t3case_snapshot, sizeof(t3case_snapshot)) != sizeof(t3case_snapshot))
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
		(t3case_header.first_process != T3CASE_PROCESS_MAIN) ||
		(t3case_header.flags & ~T3CASE_KNOWN_FLAGS)
	) {
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
	if(
		(t3case_header.payload_size != (
			t3case_header.record_count * static_cast<uint32_t>(T3CASE_RECORD_SIZE)
		)) ||
		(t3case_header.sample_count > t3case_header.record_count) ||
		(t3case_header.total_size != (
			t3case_header.payload_offset + t3case_header.payload_size
		)) ||
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

static bool t3case_header_write(void)
{
	t3case_header.sample_count = t3case_sample_count;
	t3case_header.record_count = t3case_record_count;
	t3case_header.payload_size = (
		t3case_record_count * static_cast<uint32_t>(T3CASE_RECORD_SIZE)
	);
	t3case_header.total_size = (
		t3case_header.payload_offset + t3case_header.payload_size
	);
	t3case_header.payload_checksum = t3case_payload_checksum;
	t3case_header_checksum_set();

	if(!file_append(T3CASE_BIN_FN)) {
		return false;
	}
	file_seek(0, SEEK_SET);
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

static bool t3case_record_read_raw(
	uint32_t index, t3case_record_t far *rec
)
{
	uint32_t offset;

	if(index >= t3case_header.record_count) {
		return false;
	}
	offset = (
		t3case_header.payload_offset +
		(index * static_cast<uint32_t>(T3CASE_RECORD_SIZE))
	);
	if(!file_ropen(T3CASE_BIN_FN)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	if(file_read(rec, sizeof(*rec)) != sizeof(*rec)) {
		file_close();
		return false;
	}
	file_close();
	return true;
}

static bool t3case_record_consume(t3case_record_t far *rec)
{
	if(!t3case_record_read_raw(t3case_record_count, rec)) {
		return false;
	}
	t3case_payload_checksum = t3case_fnv1a(
		t3case_payload_checksum, rec, sizeof(*rec)
	);
	t3case_record_count++;
	return true;
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

static bool t3case_control_peek(void)
{
	t3case_record_t rec;

	if(!t3case_record_read_raw(t3case_record_count, &rec)) {
		return false;
	}
	return (
		(rec.kind == T3CASE_RECORD_CONTROL) &&
		(rec.phase == T3CASE_PHASE_CONTROL) &&
		(rec.frame_index == t3case_global_frame) &&
		(rec.control == T3CASE_CONTROL_MAINL_END)
	);
}

static bool t3case_control_consume(void)
{
	t3case_record_t rec;

	if(!t3case_record_consume(&rec)) {
		return false;
	}
	return (
		(rec.kind == T3CASE_RECORD_CONTROL) &&
		(rec.phase == T3CASE_PHASE_CONTROL) &&
		(rec.frame_index == t3case_global_frame) &&
		(rec.control == T3CASE_CONTROL_MAINL_END)
	);
}

static bool t3case_payload_final(void)
{
	return (
		(t3case_sample_count == t3case_header.sample_count) &&
		(t3case_record_count == t3case_header.record_count) &&
		(t3case_payload_checksum == t3case_header.payload_checksum)
	);
}

static void t3case_error(void)
{
	t3case_diag('M', 'E', 'R', t3case_record_count, t3case_global_frame);
	t3case_mode = T3CASE_MAINL_ERROR;
	t3case_handoff_clear();
	input_mp_p1 = INPUT_NONE;
	input_mp_p2 = INPUT_NONE;
	input_sp = INPUT_OK;
	t3case_done_write(false);
}

static bool t3case_play_sample(void)
{
	t3case_record_t rec;

	if(!t3case_record_consume(&rec)) {
		return false;
	}
	if(
		(rec.kind != T3CASE_RECORD_INPUT) ||
		(rec.phase != T3CASE_PHASE_INTERSTITIAL) ||
		(rec.frame_index != t3case_global_frame) ||
		(rec.round_frame != 0xFFFFFFFFUL) ||
		(rec.round_or_result_frame != 0xFFFF) ||
		(rec.control & ~0x0003)
	) {
		return false;
	}
	t3case_input_mp_p1 = rec.input_mp_p1;
	t3case_input_mp_p2 = rec.input_mp_p2;
	t3case_input_sp = rec.input_sp;
	t3case_sample_count++;
	t3case_global_frame++;
	t3case_handoff_store();
	return true;
}

static bool t3case_record_sample(void)
{
	t3case_record_t rec;

	t3case_memclear(&rec, sizeof(rec));
	rec.kind = T3CASE_RECORD_INPUT;
	rec.phase = T3CASE_PHASE_INTERSTITIAL;
	rec.round_or_result_frame = 0xFFFF;
	rec.frame_index = t3case_global_frame;
	rec.round_frame = 0xFFFFFFFFUL;
	rec.input_mp_p1 = input_mp_p1;
	rec.input_mp_p2 = input_mp_p2;
	rec.input_sp = input_sp;
	if(!t3case_record_append(&rec)) {
		return false;
	}
	t3case_input_mp_p1 = input_mp_p1;
	t3case_input_mp_p2 = input_mp_p2;
	t3case_input_sp = input_sp;
	t3case_sample_count++;
	t3case_global_frame++;
	t3case_handoff_store();
	return true;
}

static void t3case_frame_io(void)
{
	bool ok;

	if(t3case_mode == T3CASE_MAINL_RECORD) {
		ok = t3case_record_sample();
	} else if(t3case_mode == T3CASE_MAINL_PLAYBACK) {
		if(t3case_control_pending || t3case_control_peek()) {
			t3case_control_pending = true;
			return;
		}
		ok = t3case_play_sample();
	} else {
		return;
	}
	if(!ok) {
		t3case_error();
	}
}

void far t3case_mainl_session_start(void)
{
	t3case_mode = t3case_resident_mode();
	t3case_input_mp_p1 = INPUT_NONE;
	t3case_input_mp_p2 = INPUT_NONE;
	t3case_input_sp = INPUT_NONE;
	t3case_input_vsync = (vsync_Count2 - 1);
	t3case_control_pending = false;
	t3case_transition_finished = false;
	t3case_done_written = false;

	if(t3case_mode == T3CASE_MAINL_DISABLED) {
		return;
	}
	t3case_paths_init();
	t3case_handoff_load();
	if(
		!t3case_header_read() ||
		(t3case_sample_count > t3case_header.sample_count) ||
		(t3case_record_count > t3case_header.record_count)
	) {
		t3case_error();
		return;
	}
	t3case_diag('M', 'S', 'L', t3case_record_count, t3case_global_frame);
}

void far t3case_mainl_input_mode_interface(void)
{
	uint16_t physical_input_sp;

	if(t3case_mode == T3CASE_MAINL_ERROR) {
		input_mp_p1 = INPUT_NONE;
		input_mp_p2 = INPUT_NONE;
		input_sp = INPUT_OK;
		return;
	}

	input_mode_interface();
	physical_input_sp = input_sp;
	if(
		(t3case_mode == T3CASE_MAINL_PLAYBACK) &&
		(physical_input_sp & INPUT_CANCEL)
	) {
		t3case_error();
		return;
	}
	if(
		((t3case_mode == T3CASE_MAINL_RECORD) ||
		 (t3case_mode == T3CASE_MAINL_PLAYBACK)) &&
		(t3case_input_vsync != vsync_Count2)
	) {
		t3case_frame_io();
		// Start the next interval after synchronous verifier file access.
		t3case_input_vsync = vsync_Count2;
	}
	if(t3case_mode == T3CASE_MAINL_PLAYBACK) {
		input_mp_p1 = t3case_input_mp_p1;
		input_mp_p2 = t3case_input_mp_p2;
		input_sp = t3case_input_sp;
	}
}

void far t3case_mainl_input_wait_for_change(int frames_to_wait)
{
	int frames_waited = 0;

	while(1) {
		t3case_mainl_input_mode_interface();
		if(input_sp == INPUT_NONE) {
			break;
		}
		frame_delay(1);
	}

	if(!frames_to_wait) {
		frames_to_wait = 9999;
	}

	while(frames_waited < frames_to_wait) {
		t3case_mainl_input_mode_interface();
		if(input_sp != INPUT_NONE) {
			break;
		}
		frames_waited++;
		frame_delay(1);
		if(frames_to_wait == 9999) {
			frames_waited = 0;
		}
	}
}

bool16 far t3case_mainl_input_wait_for_ok(unsigned int frames)
{
	vsync_Count1 = 0;
	do {
		t3case_mainl_input_mode_interface();
		if((input_sp & INPUT_SHOT) || (input_sp & INPUT_OK)) {
			return true;
		}
	} while(vsync_Count1 < frames);
	return false;
}

static inline uint16_t t3case_snd_get_song_measure(void)
{
	_AH = KAJA_GET_SONG_MEASURE;
	if(snd_bgm_is_fm()) {
		geninterrupt(PMD);
	} else {
		_DX = (MMD_TICKS_PER_QUARTER_NOTE * 4);
		geninterrupt(MMD);
	}
	return _AX;
}

bool16 far t3case_mainl_input_wait_for_ok_or_measure(
	int measure, unsigned int frames
)
{
	if(!snd_active) {
		return t3case_mainl_input_wait_for_ok(frames);
	}
	do {
		_AX = t3case_snd_get_song_measure();
		t3case_mainl_input_mode_interface();
		if((input_sp & INPUT_SHOT) || (input_sp & INPUT_OK)) {
			return true;
		}
	} while(_AX < measure);
	return false;
}

static void t3case_transition_finish(bool terminal)
{
	bool ok = true;
	t3case_mainl_mode_t mode = t3case_mode;

	if(t3case_transition_finished) {
		return;
	}
	if(
		(mode != T3CASE_MAINL_RECORD) &&
		(mode != T3CASE_MAINL_PLAYBACK)
	) {
		return;
	}
	t3case_transition_finished = true;

	if(mode == T3CASE_MAINL_RECORD) {
		t3case_record_t rec;

		t3case_memclear(&rec, sizeof(rec));
		rec.kind = T3CASE_RECORD_CONTROL;
		rec.phase = T3CASE_PHASE_CONTROL;
		rec.frame_index = t3case_global_frame;
		rec.round_frame = 0xFFFFFFFFUL;
		rec.round_or_result_frame = 0xFFFF;
		rec.control = T3CASE_CONTROL_MAINL_END;
		ok = t3case_record_append(&rec);
		if(ok) {
			ok = t3case_header_write();
		}
	} else {
		while(!t3case_control_pending && !t3case_control_peek()) {
			if(!t3case_play_sample()) {
				ok = false;
				break;
			}
		}
		if(ok) {
			ok = t3case_control_consume();
		}
	}

	if(!ok) {
		t3case_error();
		return;
	}
	t3case_diag('M', 'E', 'N', t3case_record_count, t3case_global_frame);
	if(
		(mode == T3CASE_MAINL_PLAYBACK) &&
		(terminal != t3case_payload_final())
	) {
		// A terminal route with remaining records or a nonterminal route with an
		// exhausted full case both contradict the source process topology.
		t3case_error();
		return;
	}
	if(terminal || (
		(mode == T3CASE_MAINL_PLAYBACK) && t3case_payload_final()
	)) {
		t3case_handoff_clear();
		t3case_done_write(true);
		t3case_mode = T3CASE_MAINL_DISABLED;
		return;
	}
	t3case_handoff_store();
}

void far t3case_mainl_transition_finish(void)
{
	t3case_transition_finish(false);
}

void far t3case_mainl_terminal_finish(void)
{
	t3case_transition_finish(true);
}

// Keep the isolated adapter at a whole-paragraph length so _TEXTC and every
// later original segment retain their baseline paragraph phase.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90"
