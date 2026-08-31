/* TH01 compact user replay core for REIIDEN.EXE.
 *
 * This is intentionally a trailing, isolated module. It owns no original
 * storage and redirects only the measured key_sense call sites through the
 * input wrapper. Its format is mirrored by the harness-side host parser.
 */

#pragma option -zCT1REPLAY_TEXT -G-

#include "platform.h"
#include "libs/master.lib/master.hpp"
#include "th01/replay.hpp"
#include "th01/replay_format.hpp"
#if defined(T1RB)
#include "th01/replay_milestone.hpp"
#endif
#include "th01/hardware/vsync.hpp"
#include "th01/rp_guard.hpp"
#include "th01/savestate_acceptance.hpp"
#include "th01/resident.hpp"
#include "th01/hiscore/regist.hpp"
#include "th01/math/dir.hpp"
#include "th01/main/stage/timer.hpp"
#include "th01/main/extend.hpp"
#include "th01/main/player/player.hpp"
#include "th01/main/player/bomb.hpp"
#include "th01/main/player/orb.hpp"
#include "th01/main/player/shot.hpp"
#include "th01/main/bullet/pellet.hpp"
#include "th01/main/bullet/missile.hpp"
#include "th01/main/bullet/laser_s.hpp"
#include "th01/main/stage/card.hpp"
#include "th01/main/stage/stageobj.hpp"
#include "th01/main/stage/item.hpp"
#include "th01/main/particle.hpp"
#include "th01/hardware/input.hpp"
#include "th01/main/boss/boss.hpp"
#include "th01/main/boss/b05.hpp"
#include "th01/main/boss/b10j.hpp"
#include "th01/main/boss/b15j.hpp"
#include "th01/main/boss/b15m.hpp"
#include "th01/main/boss/b20m.hpp"
#include "th01/main/boss/b20j.hpp"
#include "th01/rboss.hpp"
#include "th01/rpypixel.hpp"
#include "th01/t1ymx.hpp"
#include "th01/t1elx.hpp"
#include "th01/t1kik.hpp"
#include "th01/t1sar.hpp"
#include "th01/snd/mdrv2.h"
#include "platform/x86real/pc98/keyboard.hpp"

#define T1REPLAY_BUFFER_PACKET_COUNT 128
#define T1REPLAY_FAST_FORWARD_RATE 4
#define T1REPLAY_DOS_ACCESS_READ 0
#define T1REPLAY_DOS_ACCESS_RW 2
#define T1REPLAY_FP_SEG(p) ((unsigned)(((unsigned long)(void far *)(p)) >> 16))
#define T1REPLAY_FP_OFF(p) ((unsigned)((unsigned long)(void far *)(p)))
#if T1REPLAY_EXACT_TRACE
	#define T1REPLAY_EXACT_TRACE_HEADER_SIZE 16
	#define T1REPLAY_EXACT_TRACE_ROW_SIZE 92
	#define T1REPLAY_EXACT_TRACE_VERSION 1
	#define T1REPLAY_EXACT_TRACE_BUFFER_ROWS 32
	#define T1REPLAY_EXACT_ROW_PRE_INPUT 1
	#define T1REPLAY_EXACT_ROW_RESTORE_APPLIED 2
	#define T1REPLAY_EXACT_ROW_TERMINAL 3

	struct t1replay_exact_trace_header_t {
		char magic[8];
		uint16_t version;
		uint16_t header_size;
		uint16_t row_size;
		uint8_t group_count;
		uint8_t reserved;
	};

	struct t1replay_exact_trace_row_t {
		uint8_t row_kind;
		uint8_t process_seq;
		uint8_t source_process;
		uint8_t target_process;
		uint32_t sample_cursor;
		uint32_t packet_cursor;
		uint32_t input_cursor;
		uint16_t stage_id;
		int8_t route;
		int8_t boss_id;
		uint32_t frame_rand;
		uint32_t score;
		int16_t rem_lives;
		int16_t rem_bombs;
		int16_t pellet_speed_raise_cycle;
		uint8_t terminal_reason;
		uint8_t reserved;
		uint32_t group_digest[T1REPLAY_CHECKPOINT_GROUP_COUNT];
	};

	typedef char t1replay_exact_trace_header_size_check[
		(sizeof(t1replay_exact_trace_header_t) ==
		 T1REPLAY_EXACT_TRACE_HEADER_SIZE) ? 1 : -1
	];
	typedef char t1replay_exact_trace_row_size_check[
		(sizeof(t1replay_exact_trace_row_t) ==
		 T1REPLAY_EXACT_TRACE_ROW_SIZE) ? 1 : -1
	];
#endif
extern bool timer_initialized;
extern bool first_stage_in_scene;
extern bool stage_wait_for_shot_to_begin;
extern bool16 mode_test;
extern uint32_t bomb_frame;
extern uint32_t frame_since_start_of_binary;
extern int8_t boss_id;

typedef char t1replay_res_id_size_check[
	(sizeof(T1REPLAY_RES_ID) == sizeof(reinterpret_cast<t1replay_res_t *>(0)->id)) ? 1 : -1
];

static char t1replay_command_fn[10];
static char t1replay_slot_fn[11];
static char t1replay_save_request_fn[11];
static char t1replay_save_request_commit_fn[11];
static char t1replay_restart_request_fn[11];
static char t1replay_restart_request_commit_fn[11];
static char t1replay_checkpoint_fn[12];
static bool t1replay_paths_ready;
static bool t1replay_abort_pending;
static uint8_t t1replay_fast_forward_phase;
static bool t1replay_gameplay_input_armed;
static bool t1replay_gameplay_wait_skip_pending;
static bool t1replay_timing_frame_armed;
static bool t1replay_timing_first_frame;
static bool t1replay_command_delete_failed;
static bool t1replay_terminal_pending;
static t1replay_pause_action_t t1replay_pause_action;
static t1replay_mode_t t1replay_mode;
static t1replay_header_t t1replay_header;
static t1replay_res_t far *t1replay_res;
static t1replay_packet_t t1replay_buffer[T1REPLAY_BUFFER_PACKET_COUNT];
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
static bool t1replay_checkpoint_capture_attempted;
static t1replay_checkpoint_t t1replay_checkpoint;
static bool t1replay_checkpoint_restore_is_pending;
#if T1REPLAY_WORLD_CAPTURE
	static t1replay_checkpoint_t t1replay_exact_snapshot;
#endif
#if T1REPLAY_EXACT_TRACE
	static t1replay_exact_trace_row_t
		t1replay_exact_trace_buffer[T1REPLAY_EXACT_TRACE_BUFFER_ROWS];
	static t1replay_exact_trace_row_t t1replay_exact_terminal_row;
	static uint8_t t1replay_exact_trace_buffer_count;
	static bool t1replay_exact_trace_ready;
	static bool t1replay_exact_trace_failed;
	static bool t1replay_exact_terminal_pending;
	static uint32_t t1replay_exact_last_sample;
	static uint8_t t1replay_exact_last_kind;
	static int t1replay_exact_pellet_speed_raise_cycle;

	static bool t1replay_exact_trace_emit(
		uint8_t kind, uint8_t target_process, uint8_t terminal_reason,
		int pellet_speed_raise_cycle
	);
	static bool t1replay_exact_trace_row_capture(
		t1replay_exact_trace_row_t far *row, uint8_t kind,
		uint8_t target_process, uint8_t terminal_reason,
		int pellet_speed_raise_cycle
	);
	static bool t1replay_exact_trace_flush(void);
#endif

static void t1replay_fast_forward_boundary_reset(void)
{
	// The fourth held-Z frame normally reaches frame_delay(1), which clears the
	// VSync counter. If a stage/process boundary preempts one of the preceding
	// skipped waits, normalize that pacing-only counter before stock transition
	// code observes it.
	if(
		(t1replay_fast_forward_phase != 0) ||
		t1replay_gameplay_wait_skip_pending
	) {
		z_vsync_Count1 = 0;
	}
	t1replay_fast_forward_phase = 0;
	t1replay_gameplay_input_armed = false;
	t1replay_gameplay_wait_skip_pending = false;
	t1replay_timing_frame_armed = false;
	t1replay_timing_first_frame = true;
}

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
	t1replay_command_fn[0] = 'T'; t1replay_command_fn[1] = '1';
	t1replay_command_fn[2] = 'R'; t1replay_command_fn[3] = 'P';
	t1replay_command_fn[4] = 'Y'; t1replay_command_fn[5] = '.';
	t1replay_command_fn[6] = 'C'; t1replay_command_fn[7] = 'F';
	t1replay_command_fn[8] = 'G'; t1replay_command_fn[9] = '\0';
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
	t1replay_save_request_commit_fn[0] = 'T';
	t1replay_save_request_commit_fn[1] = '1';
	t1replay_save_request_commit_fn[2] = 'R';
	t1replay_save_request_commit_fn[3] = 'S';
	t1replay_save_request_commit_fn[4] = 'A';
	t1replay_save_request_commit_fn[5] = 'V';
	t1replay_save_request_commit_fn[6] = '.';
	t1replay_save_request_commit_fn[7] = 'C';
	t1replay_save_request_commit_fn[8] = 'M';
	t1replay_save_request_commit_fn[9] = 'T';
	t1replay_save_request_commit_fn[10] = '\0';
	t1replay_restart_request_fn[0] = 'T';
	t1replay_restart_request_fn[1] = '1';
	t1replay_restart_request_fn[2] = 'R';
	t1replay_restart_request_fn[3] = 'R';
	t1replay_restart_request_fn[4] = 'S';
	t1replay_restart_request_fn[5] = 'T';
	t1replay_restart_request_fn[6] = '.';
	t1replay_restart_request_fn[7] = 'C';
	t1replay_restart_request_fn[8] = 'F';
	t1replay_restart_request_fn[9] = 'G';
	t1replay_restart_request_fn[10] = '\0';
	t1replay_restart_request_commit_fn[0] = 'T';
	t1replay_restart_request_commit_fn[1] = '1';
	t1replay_restart_request_commit_fn[2] = 'R';
	t1replay_restart_request_commit_fn[3] = 'R';
	t1replay_restart_request_commit_fn[4] = 'S';
	t1replay_restart_request_commit_fn[5] = 'T';
	t1replay_restart_request_commit_fn[6] = '.';
	t1replay_restart_request_commit_fn[7] = 'C';
	t1replay_restart_request_commit_fn[8] = 'M';
	t1replay_restart_request_commit_fn[9] = 'T';
	t1replay_restart_request_commit_fn[10] = '\0';
	t1replay_checkpoint_fn[0] = 'T'; t1replay_checkpoint_fn[1] = '1';
	t1replay_checkpoint_fn[2] = 'C'; t1replay_checkpoint_fn[3] = '0';
	t1replay_checkpoint_fn[4] = '0'; t1replay_checkpoint_fn[5] = '0';
	t1replay_checkpoint_fn[6] = '0'; t1replay_checkpoint_fn[7] = '.';
	t1replay_checkpoint_fn[8] = 'C'; t1replay_checkpoint_fn[9] = 'K';
	t1replay_checkpoint_fn[10] = 'P'; t1replay_checkpoint_fn[11] = '\0';
	t1replay_paths_ready = true;
}

static void t1replay_command_witness_fn_init(char *fn)
{
	fn[0] = 'T'; fn[1] = '1'; fn[2] = 'R'; fn[3] = 'P'; fn[4] = 'Y';
	fn[5] = '.'; fn[6] = 'C'; fn[7] = 'M'; fn[8] = 'T'; fn[9] = '\0';
}

// Keep this automatic rather than static. Turbo C++ otherwise emits this
// resident ID as initialized _DATA, which would shift every original BSS
// location in REIIDEN.EXE.
static void t1replay_res_id_init(char *id)
{
	id[0] = 'T'; id[1] = '1'; id[2] = 'R'; id[3] = 'e';
	id[4] = 'p'; id[5] = 'l'; id[6] = 'a'; id[7] = 'y';
	id[8] = 'S'; id[9] = 't'; id[10] = 'a'; id[11] = 't';
	id[12] = 'e'; id[13] = '\0';
}

static void t1replay_restart_res_id_init(char *id)
{
	id[0] = 'T'; id[1] = '1'; id[2] = 'R'; id[3] = 'e';
	id[4] = 'p'; id[5] = 'l'; id[6] = 'a'; id[7] = 'y';
	id[8] = 'R'; id[9] = 'e'; id[10] = 's'; id[11] = 't';
	id[12] = 'a'; id[13] = 'r'; id[14] = 't'; id[15] = '\0';
}

// RES_ID would otherwise materialize its string literal in this TU's _DATA
// through ResData<resident_t>::exist(). Build the identifier on the stack for
// the same reason as the replay carrier ID above.
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
		return true;
	}
	if(!t1replay_slot_is_numbered(slot)) {
		return false;
	}
	t1replay_slot_fn[4] = static_cast<char>('0' + (slot / 10));
	t1replay_slot_fn[5] = static_cast<char>('0' + (slot % 10));
	return true;
}

static bool t1replay_checkpoint_path_set(uint8_t slot, uint8_t process_seq)
{
	if(process_seq > T1REPLAY_CHECKPOINT_PROCESS_MAX) {
		return false;
	}
	if(t1replay_slot_is_pending(slot)) {
		// A private exact-capture run still has to survive REIIDEN's process
		// boundary before OP has chosen a numbered destination. Keep that
		// sidecar process-local and distinct from every T1CxxYY.CKP slot.
		t1replay_checkpoint_fn[3] = 'P';
		t1replay_checkpoint_fn[4] = 'T';
	} else if(t1replay_slot_is_numbered(slot)) {
		t1replay_checkpoint_fn[3] = static_cast<char>('0' + (slot / 10));
		t1replay_checkpoint_fn[4] = static_cast<char>('0' + (slot % 10));
	} else {
		return false;
	}
	t1replay_checkpoint_fn[5] = static_cast<char>('0' + (process_seq / 10));
	t1replay_checkpoint_fn[6] = static_cast<char>('0' + (process_seq % 10));
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
		mov ah, 3Ch
		xor cx, cx
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

static void t1replay_dos_flush(void)
{
	_asm {
		mov ah, 0Dh
		int 21h
	}
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

#if T1REPLAY_SAVESTATE_ACCEPTANCE
static const char T1SAVESTATE_ACCEPTANCE_FN[] = "T1SGA.BIN";

static void t1savestate_acceptance_clear(
	t1savestate_acceptance_record_t *record
)
{
	uint8_t *p = reinterpret_cast<uint8_t *>(record);
	unsigned i;

	for(i = 0; i < sizeof(*record); i++) {
		p[i] = 0;
	}
}

static uint16_t t1savestate_acceptance_checksum(
	const t1savestate_acceptance_record_t *record
)
{
	const uint8_t *p = reinterpret_cast<const uint8_t *>(record);
	uint16_t sum = 0;
	unsigned i;

	for(i = 0; i < sizeof(*record); i++) {
		sum = static_cast<uint16_t>(sum + p[i]);
	}
	return sum;
}

static void t1savestate_acceptance_emit(
	uint8_t event, uint8_t checkpoint_result, t1replay_guard_t far *guard
)
{
	t1savestate_acceptance_record_t record;
	uint8_t marker = 0;
	uint32_t disk_size = 0;
	int fd;

	t1savestate_acceptance_clear(&record);
	record.magic[0] = 'T'; record.magic[1] = '1';
	record.magic[2] = 'S'; record.magic[3] = 'G';
	record.magic[4] = 'A'; record.magic[5] = '0';
	record.magic[6] = '0'; record.magic[7] = '1';
	record.schema = T1SAVESTATE_ACCEPTANCE_SCHEMA;
	record.event = event;
	record.checkpoint_result = checkpoint_result;
	record.guard_flags = guard->flags;
	record.committed_size = guard->committed_size;
	if(t1rpg_marker_read(guard, &marker, &disk_size)) {
		record.raw_ok = 1;
		record.marker = marker;
		record.expected_size = guard->committed_size;
		record.actual_size = disk_size;
	}
	record.checksum = 0;
	record.checksum = t1savestate_acceptance_checksum(&record);
	fd = t1replay_dos_create(T1SAVESTATE_ACCEPTANCE_FN);
	if(fd < 0) {
		return;
	}
	if(t1replay_dos_write(fd, &record, sizeof(record)) != sizeof(record)) {
		t1replay_dos_close(fd);
		return;
	}
	t1replay_dos_close(fd);
	t1replay_dos_flush();
	(void)t1rpg_commit_process();
}

static bool t1savestate_acceptance_begin(t1replay_guard_t far *guard)
{
	bool ok = t1rpg_begin(guard);

	t1savestate_acceptance_emit(T1SAE_BEGIN, ok, guard);
	return ok;
}

static bool t1savestate_acceptance_checkpoint(
	t1replay_guard_t far *guard, uint8_t event
)
{
	bool ok = t1rpg_checkpoint(guard);

	t1savestate_acceptance_emit(event, ok, guard);
	return ok;
}

static void t1savestate_acceptance_end(t1replay_guard_t far *guard)
{
	t1savestate_acceptance_emit(T1SAE_END, 0xFF, guard);
	t1rpg_end(guard);
}
#endif

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

static void t1replay_request_pair_discard(
	const char far *request_fn, const char far *commit_fn
)
{
	t1replay_dos_delete(request_fn);
	t1replay_dos_delete(commit_fn);
}

// A freshly created request is consumed immediately after an executable
// handoff. Some target DOS implementations do not make its directory entry
// visible across execl() after close plus AH=0Dh alone. The mandatory second
// identical witness is the following durable directory operation. OP accepts
// the deeply validated primary alone and merely cleans up the witness, because
// the witness itself may still be the entry hidden by the same cache. This
// never depends on a diagnostic build or a diagnostic write.
static bool t1replay_request_pair_write(
	const char far *request_fn,
	const char far *commit_fn,
	const void far *request,
	unsigned request_size
)
{
	int fd;
	bool ok;

	t1replay_request_pair_discard(request_fn, commit_fn);
	fd = t1replay_dos_create(request_fn);
	if(fd < 0) {
		return false;
	}
	ok = (t1replay_dos_write(fd, request, request_size) == request_size);
	t1replay_dos_close(fd);
	if(!ok) {
		t1replay_request_pair_discard(request_fn, commit_fn);
		return false;
	}
	t1replay_dos_flush();
	fd = t1replay_dos_create(commit_fn);
	if(fd < 0) {
		t1replay_request_pair_discard(request_fn, commit_fn);
		return false;
	}
	ok = (t1replay_dos_write(fd, request, request_size) == request_size);
	t1replay_dos_close(fd);
	if(!ok) {
		t1replay_request_pair_discard(request_fn, commit_fn);
	}
	return ok;
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

static uint32_t t1replay_restart_state_checksum(
	t1replay_restart_state_t far *state
)
{
	uint32_t checksum;

	state->checksum = 0;
	checksum = t1replay_fnv1a(
		T1REPLAY_FNV1A_BASIS,
		&state->magic,
		(sizeof(*state) - offsetof(t1replay_restart_state_t, magic))
	);
	state->checksum = checksum;
	return checksum;
}

static bool t1replay_restart_bytes_zero(const uint8_t far *p, unsigned size)
{
	while(size != 0) {
		if(*p++ != 0) {
			return false;
		}
		size--;
	}
	return true;
}

static bool t1replay_restart_practice_start_valid(
	const t1replay_practice_start_t far *start
)
{
	return (
		start &&
		(start->scene < SCENE_COUNT) &&
		(start->route < ROUTE_COUNT) &&
		(start->section <= T1RPS_BOSS_PHASE) &&
		(start->chapter <= BOSS_STAGE) &&
		(start->rank >= 0) && (start->rank <= 3) &&
		(start->score >= 0) && (start->score <= 99990000L) &&
		(start->lives >= 1) && (start->lives <= LIVES_MAX) &&
		(start->bombs >= 0) && (start->bombs <= BOMBS_MAX) &&
		(start->point_value <= 65530) &&
		(start->pellet_speed >= PELLET_SPEED_LOWER_MIN) &&
		(start->pellet_speed <= PELLET_SPEED_RAISE_MAX) &&
		(
			(start->section != T1RPS_BOSS_PHASE) ?
			true :
			(
				(start->scene == 0) && (start->route == ROUTE_MAKAI) &&
				(start->chapter == BOSS_STAGE)
			)
		)
	);
}

static bool t1replay_restart_state_valid(
	t1replay_restart_state_t far *state
)
{
	uint32_t stored;
	uint32_t computed;

	if(!state ||
		(state->magic[0] != 'T') || (state->magic[1] != '1') ||
		(state->magic[2] != 'R') || (state->magic[3] != 'R') ||
		(state->version != T1REPLAY_RESTART_RES_VERSION) ||
		((state->kind != T1RRK_NORMAL) &&
		 (state->kind != T1RRK_PRACTICE)) ||
		!t1replay_restart_bytes_zero(
			state->reserved, sizeof(state->reserved)
		) ||
		((state->kind == T1RRK_PRACTICE) &&
		 !t1replay_restart_practice_start_valid(&state->practice))) {
		return false;
	}
	stored = state->checksum;
	computed = t1replay_restart_state_checksum(state);
	state->checksum = stored;
	return (stored == computed);
}

static bool t1replay_restart_request_write(void)
{
	t1replay_restart_state_t far *state;
	t1replay_restart_request_t request;
	char id[sizeof(T1REPLAY_RESTART_RES_ID)];

	t1replay_restart_res_id_init(id);
	state = ResData<t1replay_restart_state_t>::exist(id);
	if(!t1replay_restart_state_valid(state)) {
		return false;
	}
	t1replay_memclear(&request, sizeof(request));
	request.magic[0] = 'T'; request.magic[1] = '1';
	request.magic[2] = 'R'; request.magic[3] = 'R';
	request.magic[4] = 'S'; request.magic[5] = 'T';
	request.magic[6] = '1'; request.magic[7] = '\0';
	request.schema = T1REPLAY_RESTART_REQUEST_SCHEMA;
	request.restart_state_checksum = state->checksum;
	request.checksum = t1replay_fnv1a(
		T1REPLAY_FNV1A_BASIS, &request, sizeof(request)
	);
	return t1replay_request_pair_write(
		t1replay_restart_request_fn,
		t1replay_restart_request_commit_fn,
		&request,
		sizeof(request)
	);
}

static bool t1replay_save_request_write(
	t1replay_save_request_source_t source
)
{
	t1replay_save_request_t request;

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
	return t1replay_request_pair_write(
		t1replay_save_request_fn,
		t1replay_save_request_commit_fn,
		&request,
		sizeof(request)
	);
}

static void t1replay_pending_files_discard(void)
{
	uint8_t process_seq;

	t1replay_request_pair_discard(
		t1replay_save_request_fn, t1replay_save_request_commit_fn
	);
	if(t1replay_slot_set(T1REPLAY_SLOT_PENDING)) {
		t1replay_dos_delete(t1replay_slot_fn);
	}
#if T1REPLAY_CHECKPOINT_EMIT || T1REPLAY_CHECKPOINT_RESTORE
	for(process_seq = 0; process_seq <= T1REPLAY_CHECKPOINT_PROCESS_MAX;
		process_seq++) {
		if(t1replay_checkpoint_path_set(
			T1REPLAY_SLOT_PENDING, process_seq
		)) {
			t1replay_dos_delete(t1replay_checkpoint_fn);
		}
	}
#else
	(process_seq);
#endif
}

static uint32_t t1replay_fuuin_handoff_checksum(void)
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

static uint32_t t1replay_checkpoint_group_digest(
	uint32_t digest, uint8_t id, const void far *data, unsigned size
)
{
	digest = t1replay_fnv1a(digest, &id, sizeof(id));
	return t1replay_fnv1a(digest, data, size);
}

static void t1replay_checkpoint_group_set(
	t1replay_checkpoint_group_t far *group, uint8_t id, uint32_t offset,
	uint16_t size, const void far *data
)
{
	group->id = id;
	group->schema = T1REPLAY_CHECKPOINT_SCHEMA;
	group->codec = T1REPLAY_CHECKPOINT_CODEC_RAW;
	group->flags = 0;
	group->offset = offset;
	group->stored_size = size;
	group->decoded_size = size;
	group->checksum = t1replay_fnv1a(T1REPLAY_FNV1A_BASIS, data, size);
}

static bool t1replay_checkpoint_group_valid(
	const t1replay_checkpoint_group_t far *group, uint8_t id, uint32_t offset,
	uint16_t size, const void far *data
)
{
	return (
		(group->id == id) &&
		(group->schema == T1REPLAY_CHECKPOINT_SCHEMA) &&
		(group->codec == T1REPLAY_CHECKPOINT_CODEC_RAW) &&
		(group->flags == 0) &&
		(group->offset == offset) &&
		(group->stored_size == size) &&
		(group->decoded_size == size) &&
		(group->checksum == t1replay_fnv1a(T1REPLAY_FNV1A_BASIS, data, size))
	);
}

static uint32_t t1replay_checkpoint_checksum(
	const t1replay_checkpoint_t far *checkpoint
)
{
	uint32_t zero = 0;
	uint32_t checksum = t1replay_fnv1a(
		T1REPLAY_FNV1A_BASIS, checkpoint,
		offsetof(t1replay_checkpoint_header_t, container_checksum)
	);

	checksum = t1replay_fnv1a(checksum, &zero, sizeof(zero));
	return t1replay_fnv1a(
		checksum, checkpoint->groups,
		(sizeof(*checkpoint) - offsetof(t1replay_checkpoint_t, groups))
	);
}

static bool t1replay_checkpoint_scenario_valid(
	const t1replay_checkpoint_scenario_t far *scenario
)
{
	return (
		(scenario->resident_stage_id < STAGE_COUNT) &&
		(scenario->resident_rank >= 0) && (scenario->resident_rank <= 3) &&
		(scenario->resident_bgm_mode >= 0) &&
			(scenario->resident_bgm_mode < BGM_MODE_COUNT) &&
		(scenario->resident_route >= 0) &&
			(scenario->resident_route < ROUTE_COUNT) &&
		(scenario->resident_end_flag >= ES_NONE) &&
			(scenario->resident_end_flag <= ES_JIGOKU) &&
		(scenario->resident_debug_mode == DM_OFF) &&
		((scenario->resident_snd_need_init == 0) ||
			(scenario->resident_snd_need_init == 1)) &&
		(scenario->game_rank >= 0) && (scenario->game_rank <= 3) &&
		(scenario->game_bgm_mode >= 0) &&
			(scenario->game_bgm_mode < BGM_MODE_COUNT) &&
		(scenario->game_route >= 0) &&
			(scenario->game_route < ROUTE_COUNT) &&
		(scenario->mode_test == 0) &&
		(scenario->reserved_0 == 0) &&
		t1replay_bytes_zero(scenario->reserved, sizeof(scenario->reserved))
	);
}

static bool t1replay_checkpoint_input_valid(
	const t1replay_checkpoint_input_t far *input
)
{
	uint8_t i;

	for(i = 0; i < 12; i++) {
		uint8_t pressed_mask;

		switch(i) {
		case 0:  pressed_mask = K7_ARROW_UP;    break;
		case 1:  pressed_mask = K7_ARROW_DOWN;  break;
		case 2:  pressed_mask = K7_ARROW_LEFT;  break;
		case 3:  pressed_mask = K7_ARROW_RIGHT; break;
		case 4:  pressed_mask = K5_Z;           break;
		case 5:  pressed_mask = K5_X;           break;
		case 6:  pressed_mask = K0_ESC;         break;
		case 7:  pressed_mask = K3_RETURN;      break;
		case 8:  pressed_mask = K8_NUM_8;       break;
		case 9:  pressed_mask = K9_NUM_2;       break;
		case 10: pressed_mask = K8_NUM_4;       break;
		default: pressed_mask = K9_NUM_6;       break;
		}
		if((input->input_history[i] != 0) &&
			(input->input_history[i] != pressed_mask)) {
			return false;
		}
	}
	// Slots 12 and 13 are counters. After the native signed frame counter
	// wraps, its `< BOMB_DOUBLETAP_WINDOW` test permits the entire uint8 range.
	// Slots 14 and 15 belong only to the unsupported test-mode debug input.
	return (
		(input->input_history[14] == 0) &&
		(input->input_history[15] == 0) &&
		(input->input_lr <= 3) &&
		(input->input_shot <= 1) &&
		(input->input_ok <= 1) &&
		(input->input_strike <= 1) &&
		(input->input_up <= 1) &&
		(input->input_down <= 1) &&
		(input->input_bomb <= 1) &&
		(input->paused <= 1) &&
		(input->player_is_hit <= 1) &&
		(input->input_mem_enter <= 1) &&
		(input->input_mem_leave <= 1) &&
		(input->reserved_0 == 0)
	);
}

static bool t1replay_checkpoint_pacing_valid(
	const t1replay_checkpoint_pacing_t far *pacing
)
{
	return (
		(pacing->replay_packet_anchor <=
			(T1REPLAY_INPUT_SIZE_MAX / T1REPLAY_PACKET_SIZE)) &&
		(pacing->replay_input_anchor ==
			(pacing->replay_packet_anchor * T1REPLAY_PACKET_SIZE)) &&
		(pacing->pellet_speed_raise_cycle > 0) &&
		(pacing->process_seq <= T1REPLAY_CHECKPOINT_PROCESS_MAX) &&
		(pacing->timer_initialized <= 1) &&
		(pacing->first_stage_in_scene <= 1) &&
		(pacing->stage_wait_for_shot_to_begin <= 1) &&
		(pacing->reserved == 0)
	);
}

static bool t1replay_checkpoint_player_valid(
	const t1replay_checkpoint_player_t far *player
)
{
	return (
		(player->mode >= 0) && (player->mode <= 5) &&
		(player->dash_direction >= X_RIGHT) &&
			(player->dash_direction <= X_LEFT) &&
		((player->bomb_flag == 0) ||
		 (player->bomb_flag == 2) ||
		 (player->bomb_flag == 3)) &&
		(player->bombing <= 1) &&
		(player->combo_enabled <= 1) &&
		(player->player_deflecting <= 1) &&
		(player->player_sliding <= 1) &&
		(player->player_is_hit <= 1) &&
		(player->player_invincible <= 1) &&
		(player->player_invincible_against_orb <= 1) &&
		(player->bomb_damaging <= 1) &&
		t1replay_bytes_zero(player->reserved, sizeof(player->reserved))
	);
}

static bool t1replay_checkpoint_orb_valid(
	const t1replay_checkpoint_orb_t far *orb
)
{
	return (
		(orb->velocity_x >= OVX_0) && (orb->velocity_x < OVX_COUNT) &&
		(orb->in_portal <= 1) &&
		t1replay_bytes_zero(orb->reserved, sizeof(orb->reserved))
	);
}

static bool t1replay_checkpoint_stage_valid(
	const t1replay_checkpoint_stage_t far *stage
)
{
	uint16_t i;

	if(
		(stage->cards_count > T1REPLAY_CHECKPOINT_CARD_COUNT_MAX) ||
		(stage->obstacles_count > T1REPLAY_CHECKPOINT_OBSTACLE_COUNT_MAX) ||
		(stage->vertical_bars_blocked > 1) ||
		(stage->portals_blocked > 1) ||
		(stage->card_flip_cycle >= CARD_FLIP_CYCLE_MAX) ||
		(stage->reserved != 0)
	) {
		return false;
	}
	for(i = 0; i < stage->cards_count; i++) {
		if(
			(stage->cards[i].hp < 0) || (stage->cards[i].hp > 3) ||
			(stage->cards[i].flag > CARD_REMOVED) ||
			(stage->cards[i].flip_frame < 0) ||
			(stage->cards[i].flip_frame >= card_first_frame_of(CARD_CELS)) ||
			((stage->cards[i].flag != CARD_FLIPPING) &&
			 ((stage->cards[i].flip_frame != 0) ||
			  (stage->cards[i].score != 0))) ||
			((stage->cards[i].flag == CARD_FLIPPING) &&
			 (stage->cards[i].score > CARD_SCORE_CAP))
		) {
			return false;
		}
	}
	for(i = 0; i < stage->obstacles_count; i++) {
		uint8_t type = stage->obstacles[i].type;
		bool16 is_turret = (
			(type >= OT_TURRET_SLOW_1_AIMED) &&
			(type <= OT_TURRET_QUICK_5_SPREAD_WIDE_AIMED)
		);

		if(
			(type < OT_BUMPER) || (type > OT_BAR_RIGHT) ||
			((type >= OT_ACTUALLY_A_CARD) && (type <= OT_ACTUALLY_A_4FLIP_CARD)) ||
			(stage->obstacles[i].frame < 0) ||
			(stage->obstacles[i].turret_flag > 9) ||
			(!is_turret && (stage->obstacles[i].turret_flag != 0)) ||
			(((type == OT_BUMPER) || (type >= OT_BAR_TOP)) &&
			 (stage->obstacles[i].frame >= 8)) ||
			((type == OT_PORTAL) && (stage->obstacles[i].frame >= 60))
		) {
			return false;
		}
	}
	return true;
}

static bool t1replay_checkpoint_item_valid(
	const t1replay_checkpoint_item_t far *item
)
{
	return (
		(item->flag == 0) || (item->flag == 1) || (item->flag == 2) ||
		(item->flag == 3) || (item->flag == 99) || (item->flag == 100)
	);
}

static bool t1replay_checkpoint_items_valid(
	const t1replay_checkpoint_items_t far *items
)
{
	uint8_t i;

	for(i = 0; i < T1REPLAY_CHECKPOINT_ITEM_BOMB_COUNT; i++) {
		if(!t1replay_checkpoint_item_valid(&items->bombs[i])) {
			return false;
		}
	}
	for(i = 0; i < T1REPLAY_CHECKPOINT_ITEM_POINT_COUNT; i++) {
		if(!t1replay_checkpoint_item_valid(&items->points[i])) {
			return false;
		}
	}
	return true;
}

static bool t1replay_checkpoint_pellets_valid(
	const t1replay_checkpoint_pellets_t far *pellets
)
{
	uint16_t i;

	if(
		(pellets->interlace_field > 1) ||
		(pellets->spawn_with_cloud > 1) ||
		(pellets->reserved != 0)
	) {
		return false;
	}
	for(i = 0; i < T1REPLAY_CHECKPOINT_PELLET_COUNT; i++) {
		const t1replay_checkpoint_pellet_t far *pellet = &pellets->pellets[i];

		if(
			(pellet->moving > 1) ||
			(pellet->motion_type > PM_CHASE) ||
			(pellet->not_rendered > 1) ||
			(pellet->from_group < PG_NONE) ||
			(pellet->from_group > PG_1_RANDOM_WIDE) ||
			(pellet->sling_direction < PSD_NONE) ||
			(pellet->sling_direction > PSD_COUNTERCLOCKWISE)
		) {
			return false;
		}
	}
	return true;
}

static bool t1replay_checkpoint_shots_valid(
	const t1replay_checkpoint_shots_t far *shots
)
{
	uint8_t i;

	for(i = 0; i < T1REPLAY_CHECKPOINT_SHOT_COUNT; i++) {
		if(
			(shots->shots[i].moving > 1) ||
			(shots->shots[i].decay_frame > 7)
		) {
			return false;
		}
	}
	return true;
}

static bool t1replay_checkpoint_missiles_valid(
	const t1replay_checkpoint_missiles_t far *missiles
)
{
	uint8_t i;

	if(!t1replay_bytes_zero(missiles->reserved, sizeof(missiles->reserved))) {
		return false;
	}
	for(i = 0; i < T1REPLAY_CHECKPOINT_MISSILE_COUNT; i++) {
		if(missiles->missiles[i].flag > MF_HIT_last) {
			return false;
		}
	}
	return true;
}

static bool t1replay_checkpoint_lasers_valid(
	const t1replay_checkpoint_lasers_t far *lasers
)
{
	uint8_t i;

	for(i = 0; i < T1REPLAY_CHECKPOINT_LASER_COUNT; i++) {
		const t1replay_checkpoint_laser_t far *laser = &lasers->lasers[i];

		if(
			(laser->alive > 1) || (laser->damaging > 1) ||
			(laser->put_flag > 1) || (laser->reserved != 0)
		) {
			return false;
		}
	}
	return true;
}

static bool t1replay_checkpoint_particles_valid(
	const t1replay_checkpoint_particles_t far *particles
)
{
	uint8_t i;

	if(!t1replay_bytes_zero(particles->reserved, sizeof(particles->reserved))) {
		return false;
	}
	for(i = 0; i < T1REPLAY_CHECKPOINT_PARTICLE_COUNT; i++) {
		if(
			(particles->particles[i].alive > 1) ||
			!t1replay_bytes_zero(
				particles->particles[i].reserved,
				sizeof(particles->particles[i].reserved)
			)
		) {
			return false;
		}
	}
	return true;
}

static bool t1replay_checkpoint_valid(
	const t1replay_checkpoint_t far *checkpoint
)
{
	uint32_t digest = T1REPLAY_FNV1A_BASIS;

	if(
		(checkpoint->header.magic[0] != 'T') ||
		(checkpoint->header.magic[1] != '1') ||
		(checkpoint->header.magic[2] != 'C') ||
		(checkpoint->header.magic[3] != 'K') ||
		(checkpoint->header.magic[4] != 'P') ||
		(checkpoint->header.magic[5] != '1') ||
		(checkpoint->header.magic[6] != '\0') ||
		(checkpoint->header.magic[7] != '\0') ||
		(checkpoint->header.schema != T1REPLAY_CHECKPOINT_SCHEMA) ||
		(checkpoint->header.header_size != T1REPLAY_CHECKPOINT_HEADER_SIZE) ||
		(checkpoint->header.game_id != 1) ||
		(checkpoint->header.group_count != T1REPLAY_CHECKPOINT_GROUP_COUNT) ||
		(checkpoint->header.flags != T1REPLAY_CHECKPOINT_FLAGS_KNOWN) ||
		(checkpoint->header.total_size != T1REPLAY_CHECKPOINT_SIZE) ||
		(checkpoint->header.replay_start_checksum !=
			t1replay_header.start_checksum) ||
		!t1replay_checkpoint_scenario_valid(&checkpoint->scenario) ||
		!t1replay_checkpoint_input_valid(&checkpoint->input) ||
		!t1replay_checkpoint_pacing_valid(&checkpoint->pacing) ||
		!t1replay_checkpoint_player_valid(&checkpoint->player) ||
		!t1replay_checkpoint_orb_valid(&checkpoint->orb) ||
		!t1replay_checkpoint_stage_valid(&checkpoint->stage) ||
		!t1replay_checkpoint_items_valid(&checkpoint->items) ||
		!t1replay_checkpoint_pellets_valid(&checkpoint->pellets) ||
		!t1replay_checkpoint_shots_valid(&checkpoint->shots) ||
		!t1replay_checkpoint_missiles_valid(&checkpoint->missiles) ||
		!t1replay_checkpoint_lasers_valid(&checkpoint->lasers) ||
		!t1replay_checkpoint_particles_valid(&checkpoint->particles) ||
		!t1replay_checkpoint_boss_valid(&checkpoint->boss) ||
		!t1replay_checkpoint_world_valid(checkpoint)
	) {
		return false;
	}
	#define checkpoint_group_valid(id, field) \
		!t1replay_checkpoint_group_valid( \
			&checkpoint->groups[id], id, offsetof(t1replay_checkpoint_t, field), \
			sizeof(checkpoint->field), &checkpoint->field \
		)
	if(
		checkpoint_group_valid(T1RCGI_SCENARIO, scenario) ||
		checkpoint_group_valid(T1RCGI_RNG, rng) ||
		checkpoint_group_valid(T1RCGI_INPUT, input) ||
		checkpoint_group_valid(T1RCGI_PACING, pacing) ||
		checkpoint_group_valid(T1RCGI_PLAYER, player) ||
		checkpoint_group_valid(T1RCGI_ORB, orb) ||
		checkpoint_group_valid(T1RCGI_STAGE, stage) ||
		checkpoint_group_valid(T1RCGI_ITEMS, items) ||
		checkpoint_group_valid(T1RCGI_PELLETS, pellets) ||
		checkpoint_group_valid(T1RCGI_SHOTS, shots) ||
		checkpoint_group_valid(T1RCGI_MISSILES, missiles) ||
		checkpoint_group_valid(T1RCGI_LASERS, lasers) ||
		checkpoint_group_valid(T1RCGI_PARTICLES, particles) ||
		checkpoint_group_valid(T1RCGI_BOSS, boss)
	) {
		return false;
	}
	#undef checkpoint_group_valid
	#define checkpoint_digest(id, field) \
		digest = t1replay_checkpoint_group_digest( \
			digest, id, &checkpoint->field, sizeof(checkpoint->field) \
		)
	checkpoint_digest(T1RCGI_SCENARIO, scenario);
	checkpoint_digest(T1RCGI_RNG, rng);
	checkpoint_digest(T1RCGI_INPUT, input);
	checkpoint_digest(T1RCGI_PACING, pacing);
	checkpoint_digest(T1RCGI_PLAYER, player);
	checkpoint_digest(T1RCGI_ORB, orb);
	checkpoint_digest(T1RCGI_STAGE, stage);
	checkpoint_digest(T1RCGI_ITEMS, items);
	checkpoint_digest(T1RCGI_PELLETS, pellets);
	checkpoint_digest(T1RCGI_SHOTS, shots);
	checkpoint_digest(T1RCGI_MISSILES, missiles);
	checkpoint_digest(T1RCGI_LASERS, lasers);
	checkpoint_digest(T1RCGI_PARTICLES, particles);
	checkpoint_digest(T1RCGI_BOSS, boss);
	#undef checkpoint_digest
	return (
		(checkpoint->header.state_digest == digest) &&
		(checkpoint->header.container_checksum ==
			t1replay_checkpoint_checksum(checkpoint))
	);
}

static bool t1replay_checkpoint_cross_groups_valid(
	const t1replay_checkpoint_t far *checkpoint
)
{
	const t1replay_checkpoint_scenario_t far *scenario = &checkpoint->scenario;
	const t1replay_checkpoint_input_t far *input = &checkpoint->input;
	const t1replay_checkpoint_pacing_t far *pacing = &checkpoint->pacing;

	if(
		(scenario->resident_rank != scenario->game_rank) ||
		(scenario->resident_bgm_mode != scenario->game_bgm_mode) ||
		(scenario->resident_rem_bombs != scenario->game_rem_bombs) ||
		(scenario->resident_credit_lives_extra !=
			scenario->game_credit_lives_extra) ||
		(scenario->resident_route != scenario->game_route) ||
		(scenario->resident_rem_lives != scenario->game_rem_lives) ||
		(scenario->resident_continues_total !=
			scenario->game_continues_total) ||
		input->paused || input->player_is_hit ||
		checkpoint->player.player_is_hit ||
		!pacing->timer_initialized || pacing->first_stage_in_scene ||
		pacing->stage_wait_for_shot_to_begin
	) {
		return false;
	}
	if(checkpoint->orb.in_portal) {
		int slot = checkpoint->stage.entered_portal_slot;

		if(
			!checkpoint->stage.portals_blocked ||
			(slot < 0) || (slot >= checkpoint->stage.obstacles_count) ||
			(checkpoint->stage.obstacles[slot].type != OT_PORTAL) ||
			(checkpoint->stage.obstacles[slot].frame == 0)
		) {
			return false;
		}
	}
	return true;
}

static bool t1replay_magic_matches(const char far *magic, char last)
{
	return (
		(magic[0] == 'T') && (magic[1] == '1') &&
		(magic[2] == 'R') && (magic[3] == 'P') &&
		(magic[4] == 'Y') && (magic[5] == last) &&
		(magic[6] == '\0') && (magic[7] == '\0')
	);
}

static uint8_t t1replay_group_mask(uint8_t group)
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

static uint8_t t1replay_group_number(uint8_t group)
{
	switch(group) {
	case T1RIG_0: return 0;
	case T1RIG_3: return 3;
	case T1RIG_5: return 5;
	case T1RIG_6: return 6;
	case T1RIG_7: return 7;
	case T1RIG_8: return 8;
	case T1RIG_9: return 9;
	}
	return 0;
}

#if T1REPLAY_KONNGARA_PHASE1_DIRECT_TRACE
#define T1REPLAY_PRIVATE_KONNGARA_PHASE1_TARGET 2
#endif
#if T1YMX_DIRECT_TRACE
#define T1REPLAY_PRIVATE_YUUGENMAGAN_FIRST_COMBAT_TARGET 3
#endif
#if T1ELX_DIRECT_TRACE
#define T1REPLAY_PRIVATE_ELIS_FIRST_COMBAT_TARGET 4
#endif
#if T1KIK_DIRECT_TRACE
#define T1REPLAY_PRIVATE_KIKURI_FIRST_COMBAT_TARGET 5
#endif
#if T1SAR_DIRECT_TRACE
#define T1REPLAY_PRIVATE_SARIEL_FIRST_COMBAT_TARGET 6
#endif

static bool t1replay_practice_boss_phase_start_valid(
	const t1replay_start_t far *start
)
{
	if(
		(start->practice_boss_phase == T1RPBPT_NONE) ||
		(
			(start->practice_boss_phase ==
				T1RPBPT_SINGYOKU_FIRST_COMBAT) &&
			(start->stage_id == BOSS_STAGE) &&
			(start->route == ROUTE_MAKAI)
		) ||
		(
			(start->practice_boss_phase ==
				T1RPBPT_MIMA_FIRST_COMBAT) &&
			(start->stage_id == ((1 * STAGES_PER_SCENE) + BOSS_STAGE)) &&
			(start->route == ROUTE_JIGOKU)
		)
	) {
		return true;
	}
#if T1REPLAY_KONNGARA_PHASE1_DIRECT_TRACE
	return (
		(start->practice_boss_phase == T1REPLAY_PRIVATE_KONNGARA_PHASE1_TARGET) &&
		(start->stage_id == ((STAGES_PER_SCENE * 3) + BOSS_STAGE)) &&
		(start->route == ROUTE_JIGOKU)
	);
#elif T1YMX_DIRECT_TRACE
	return (
		(start->practice_boss_phase ==
			T1REPLAY_PRIVATE_YUUGENMAGAN_FIRST_COMBAT_TARGET) &&
		(start->stage_id == ((STAGES_PER_SCENE * 1) + BOSS_STAGE)) &&
		(start->route == ROUTE_MAKAI)
	);
#elif T1ELX_DIRECT_TRACE
	return (
		(start->practice_boss_phase ==
			T1REPLAY_PRIVATE_ELIS_FIRST_COMBAT_TARGET) &&
		(start->stage_id == ((STAGES_PER_SCENE * 2) + BOSS_STAGE)) &&
		(start->route == ROUTE_MAKAI)
	);
#elif T1KIK_DIRECT_TRACE
	return (
		(start->practice_boss_phase ==
			T1REPLAY_PRIVATE_KIKURI_FIRST_COMBAT_TARGET) &&
		(start->stage_id == ((STAGES_PER_SCENE * 2) + BOSS_STAGE)) &&
		(start->route == ROUTE_JIGOKU)
	);
#elif T1SAR_DIRECT_TRACE
	return (
		(start->practice_boss_phase ==
			T1REPLAY_PRIVATE_SARIEL_FIRST_COMBAT_TARGET) &&
		(start->stage_id == ((STAGES_PER_SCENE * 3) + BOSS_STAGE)) &&
		(start->route == ROUTE_MAKAI)
	);
#else
	return false;
#endif
}

static bool t1replay_start_valid(const t1replay_start_t far *start)
{
	return (
		(start->stage_id < STAGE_COUNT) &&
		(start->rank >= 0) && (start->rank <= 3) &&
		(start->bgm_mode >= 0) && (start->bgm_mode <= 1) &&
		(start->route >= 0) && (start->route <= 1) &&
		(start->end_flag >= 0) && (start->end_flag <= 2) &&
		(start->debug_mode == 0) &&
		(start->snd_need_init >= 0) && (start->snd_need_init <= 1) &&
		(start->mode_test == 0) &&
		(start->start_binary == T1REPLAY_PROCESS_REIIDEN) &&
		t1replay_practice_boss_phase_start_valid(start) &&
		t1replay_bytes_zero(start->reserved, sizeof(start->reserved))
	);
}

static void t1replay_header_checksum_set(void)
{
	t1replay_header.header_checksum = 0;
	t1replay_header.header_checksum = t1replay_fnv1a(
		T1REPLAY_FNV1A_BASIS, &t1replay_header, sizeof(t1replay_header)
	);
}

static unsigned t1replay_header_wire_size(void)
{
	if(
		t1replay_magic_matches(t1replay_header.magic, '4') &&
		(t1replay_header.version == T1REPLAY_VERSION_LEGACY) &&
		(t1replay_header.header_size == T1REPLAY_HEADER_SIZE_LEGACY)
	) {
		return T1REPLAY_HEADER_SIZE_LEGACY;
	}
	if(
		t1replay_magic_matches(t1replay_header.magic, '5') &&
		(t1replay_header.version == T1REPLAY_VERSION) &&
		(t1replay_header.header_size == T1REPLAY_HEADER_SIZE)
	) {
		return T1REPLAY_HEADER_SIZE;
	}
	return 0;
}

static bool t1replay_header_write(bool create)
{
	int fd = (create ?
		t1replay_dos_create(t1replay_slot_fn) :
		t1replay_dos_open(t1replay_slot_fn, T1REPLAY_DOS_ACCESS_RW)
	);

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

struct t1replay_stream_state_t {
	uint32_t samples;
	uint32_t processes;
	uint8_t process;
	uint8_t process_seq;
	uint8_t source_process;
	uint8_t fuuin_phase;
	uint8_t terminal_reason;
	bool terminal_seen;
};

static void t1replay_stream_state_reset(t1replay_stream_state_t far *state)
{
	t1replay_memclear(state, sizeof(*state));
	state->process = T1REPLAY_PROCESS_REIIDEN;
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

static bool t1replay_packet_is_valid(
	const t1replay_packet_t far *packet, t1replay_stream_state_t far *state
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
				if(packet->keys[i] & ~t1replay_group_mask(i)) {
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
			   ((value == T1REPLAY_END_MENU) ||
				(value == T1REPLAY_END_GAME_OVER))) ||
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
	t1replay_stream_state_t state;
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
			 T1REPLAY_BUFFER_PACKET_COUNT) ?
				T1REPLAY_BUFFER_PACKET_COUNT :
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
	unsigned header_size;
	int fd = t1replay_dos_open(t1replay_slot_fn, T1REPLAY_DOS_ACCESS_READ);

	if(fd < 0) {
		return false;
	}
	t1replay_memclear(&t1replay_header, sizeof(t1replay_header));
	if(
		(t1replay_dos_read(
			fd, &t1replay_header, T1REPLAY_HEADER_SIZE_LEGACY
		) != T1REPLAY_HEADER_SIZE_LEGACY) ||
		((header_size = t1replay_header_wire_size()) == 0) ||
		((header_size > T1REPLAY_HEADER_SIZE_LEGACY) &&
		 (t1replay_dos_read(
			fd,
			(reinterpret_cast<uint8_t far *>(&t1replay_header) +
			 T1REPLAY_HEADER_SIZE_LEGACY),
			(header_size - T1REPLAY_HEADER_SIZE_LEGACY)
		 ) != (header_size - T1REPLAY_HEADER_SIZE_LEGACY))) ||
		!t1replay_dos_size(fd, &file_size)
	) {
		t1replay_dos_close(fd);
		return false;
	}
	stored_checksum = t1replay_header.header_checksum;
	t1replay_header.header_checksum = 0;
	computed_checksum = t1replay_fnv1a(
		T1REPLAY_FNV1A_BASIS, &t1replay_header, header_size
	);
	t1replay_header.header_checksum = stored_checksum;
	if(
		(t1replay_header.packet_size != T1REPLAY_PACKET_SIZE) ||
		(t1replay_header.flags != T1REPLAY_FLAGS_KNOWN) ||
		(t1replay_header.status !=
			(finalized ? T1REPLAY_STATUS_FINALIZED : T1REPLAY_STATUS_RECORDING)) ||
		(t1replay_header.game_id != 1) ||
		(t1replay_header.input_semantics != T1REPLAY_INPUT_SEMANTICS_LATCHED_GROUPS) ||
		(t1replay_header.input_offset != header_size) ||
		(t1replay_header.input_size > T1REPLAY_INPUT_SIZE_MAX) ||
		(t1replay_header.packet_count >
			(T1REPLAY_INPUT_SIZE_MAX / T1REPLAY_PACKET_SIZE)) ||
		(t1replay_header.input_size !=
			(t1replay_header.packet_count * T1REPLAY_PACKET_SIZE)) ||
		(file_size != (t1replay_header.input_offset + t1replay_header.input_size)) ||
		(stored_checksum != computed_checksum) ||
		(t1replay_header.start_checksum != t1replay_fnv1a(
			T1REPLAY_FNV1A_BASIS, &t1replay_header.start, sizeof(t1replay_header.start)
		)) ||
		!t1replay_start_valid(&t1replay_header.start) ||
		!t1replay_name_valid(t1replay_header.name) ||
		!t1replay_summary_valid(
			&t1replay_header.summary, &t1replay_header.start,
			finalized, t1replay_header.end_reason
		) ||
		(t1replay_header.reserved_0 != 0) ||
		(t1replay_header.slow_frames > t1replay_header.timed_frames) ||
		((t1replay_header.status == T1REPLAY_STATUS_FINALIZED) ?
			((t1replay_header.end_reason != T1REPLAY_END_MENU) &&
			 (t1replay_header.end_reason != T1REPLAY_END_CLEAR) &&
			 (t1replay_header.end_reason != T1REPLAY_END_GAME_OVER)) :
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
	uint32_t length, uint32_t expected, t1replay_stream_state_t far *state
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

static bool t1replay_checkpoint_read(uint8_t slot, uint8_t process_seq)
{
	uint32_t size;
	int fd;
	bool valid;

	if(!t1replay_checkpoint_path_set(slot, process_seq)) {
		return false;
	}
	fd = t1replay_dos_open(t1replay_checkpoint_fn, T1REPLAY_DOS_ACCESS_READ);
	if(fd < 0) {
		return false;
	}
	valid = (
		t1replay_dos_size(fd, &size) && t1replay_dos_seek(fd, 0) &&
		(size == sizeof(t1replay_checkpoint)) &&
		(t1replay_dos_read(
			fd, &t1replay_checkpoint, sizeof(t1replay_checkpoint)
		) == sizeof(t1replay_checkpoint))
	);
	t1replay_dos_close(fd);
	return valid;
}

#if T1REPLAY_PIXEL_TRACE && !T1REPLAY_CHECKPOINT_RESTORE
static bool t1replay_checkpoint_probe_prepare(
	resident_t far *start_resident, uint8_t slot, uint8_t process_seq,
	uint8_t source_process
)
{
	t1replay_stream_state_t state;
	const t1replay_checkpoint_pacing_t far *pacing =
		&t1replay_checkpoint.pacing;
	const t1replay_checkpoint_scenario_t far *scenario =
		&t1replay_checkpoint.scenario;

	if(
		!start_resident ||
		!t1replay_checkpoint_read(slot, process_seq) ||
		!t1replay_checkpoint_valid(&t1replay_checkpoint) ||
		!t1replay_checkpoint_cross_groups_valid(&t1replay_checkpoint) ||
		!t1replay_ckpt_present_valid(&t1replay_checkpoint) ||
		(pacing->process_seq != process_seq) ||
		(pacing->replay_packet_anchor > t1replay_header.packet_count) ||
		(pacing->replay_sample_anchor > t1replay_header.sample_count) ||
		(pacing->replay_input_anchor > t1replay_header.input_size) ||
		(start_resident->stage_id != scenario->resident_stage_id) ||
		(start_resident->route != scenario->resident_route) ||
		!t1replay_payload_prefix_valid(
			pacing->replay_input_anchor, pacing->replay_prefix_checksum, &state
		) ||
		state.terminal_seen ||
		(state.samples != pacing->replay_sample_anchor) ||
		(state.process != T1REPLAY_PROCESS_REIIDEN) ||
		(state.process_seq != process_seq) ||
		(state.source_process != source_process)
	) {
		return false;
	}
	return t1replay_pixel_probe_arm(&t1replay_checkpoint);
}
#endif

static bool t1replay_checkpoint_restore_prepare(
	resident_t far *start_resident, uint8_t slot, uint8_t process_seq,
	uint8_t source_process
)
{
	t1replay_stream_state_t state;
	const t1replay_checkpoint_pacing_t far *pacing =
		&t1replay_checkpoint.pacing;
	const t1replay_checkpoint_scenario_t far *scenario =
		&t1replay_checkpoint.scenario;

	if(
		!start_resident ||
		!t1replay_checkpoint_read(slot, process_seq) ||
		!t1replay_checkpoint_valid(&t1replay_checkpoint) ||
		!t1replay_checkpoint_cross_groups_valid(&t1replay_checkpoint) ||
#if T1REPLAY_CHECKPOINT_RESTORE
		!t1replay_ckpt_present_valid(&t1replay_checkpoint) ||
#endif
		(pacing->process_seq != process_seq) ||
		(pacing->replay_packet_anchor > t1replay_header.packet_count) ||
		(pacing->replay_sample_anchor > t1replay_header.sample_count) ||
		(pacing->replay_input_anchor > t1replay_header.input_size) ||
		(start_resident->stage_id != scenario->resident_stage_id) ||
		(start_resident->route != scenario->resident_route) ||
		!t1replay_payload_prefix_valid(
			pacing->replay_input_anchor, pacing->replay_prefix_checksum, &state
		) ||
		state.terminal_seen ||
		(state.samples != pacing->replay_sample_anchor) ||
		(state.process != T1REPLAY_PROCESS_REIIDEN) ||
		(state.process_seq != process_seq) ||
		(state.source_process != source_process)
	) {
		return false;
	}

	// Only the resident scenario is needed before native scene and owner
	// loaders rebuild their allocations. All remaining mutation is deferred to
	// the top-of-gameplay-loop restore seam.
	start_resident->rand = scenario->resident_rand;
	start_resident->score = scenario->resident_score;
	start_resident->continues_total = scenario->resident_continues_total;
	start_resident->hiscore = scenario->resident_hiscore;
	start_resident->score_highest = scenario->resident_score_highest;
	for(int i = 0; i < (STAGES_PER_SCENE - 1); i++) {
		start_resident->bonus_per_stage[i] = scenario->resident_bonus_per_stage[i];
		start_resident->continues_per_scene[i] =
			scenario->resident_continues_per_scene[i];
	}
	start_resident->stage_id = scenario->resident_stage_id;
	start_resident->point_value = scenario->resident_point_value;
	start_resident->pellet_speed = scenario->resident_pellet_speed;
	start_resident->rank = scenario->resident_rank;
	start_resident->bgm_mode = static_cast<bgm_mode_t>(
		scenario->resident_bgm_mode
	);
	start_resident->rem_bombs = scenario->resident_rem_bombs;
	start_resident->credit_lives_extra =
		scenario->resident_credit_lives_extra;
	start_resident->end_flag = static_cast<end_sequence_t>(
		scenario->resident_end_flag
	);
	start_resident->route = scenario->resident_route;
	start_resident->rem_lives = scenario->resident_rem_lives;
	start_resident->snd_need_init = scenario->resident_snd_need_init;
	start_resident->debug_mode = DM_OFF;

	// Prefix validation used the shared I/O buffer. Resume from an empty decode
	// buffer at the exact packet boundary recorded by the sidecar.
	t1replay_buffer_len = 0;
	t1replay_buffer_pos = 0;
	t1replay_payload_written = pacing->replay_input_anchor;
	t1replay_packet_cursor = pacing->replay_packet_anchor;
	t1replay_sample_cursor = pacing->replay_sample_anchor;
	t1replay_payload_checksum = pacing->replay_prefix_checksum;
	t1replay_pending_run = 0;
	t1replay_decode_run = 0;
	t1replay_pending_valid = false;
	t1replay_memclear(t1replay_keys, sizeof(t1replay_keys));
#if T1REPLAY_PIXEL_TRACE
	if(!t1replay_pixel_probe_arm(&t1replay_checkpoint)) {
		return false;
	}
#endif
	t1replay_checkpoint_restore_is_pending = true;
	return true;
}

static uint8_t t1replay_practice_boss_phase_from_restart(
	const t1replay_start_t far *start
)
{
	t1replay_restart_state_t far *state;
	char id[sizeof(T1REPLAY_RESTART_RES_ID)];

	if(!start) {
		return T1RPBPT_NONE;
	}
	t1replay_restart_res_id_init(id);
	state = ResData<t1replay_restart_state_t>::exist(id);
	if(
		!t1replay_restart_state_valid(state) ||
		(state->kind != T1RRK_PRACTICE) ||
		(start->rank != state->practice.rank) ||
		(start->score != state->practice.score) ||
		(start->rem_lives != state->practice.lives) ||
		(start->rem_bombs != state->practice.bombs) ||
		(start->point_value != state->practice.point_value) ||
		(start->pellet_speed != state->practice.pellet_speed) ||
		(start->resident_rand != state->practice.rand)
	) {
		return T1RPBPT_NONE;
	}

#if T1REPLAY_KONNGARA_PHASE1_DIRECT_TRACE
	if(
		(state->practice.section == T1RPS_BOSS_START) &&
		(state->practice.scene == 3) &&
		(state->practice.route == ROUTE_JIGOKU) &&
		(state->practice.chapter == BOSS_STAGE) &&
		(start->stage_id == ((STAGES_PER_SCENE * 3) + BOSS_STAGE)) &&
		(start->route == ROUTE_JIGOKU)
	) {
		return T1REPLAY_PRIVATE_KONNGARA_PHASE1_TARGET;
}
#endif

#if T1YMX_DIRECT_TRACE
	if(
		(state->practice.section == T1RPS_BOSS_START) &&
		(state->practice.scene == 1) &&
		(state->practice.route == ROUTE_MAKAI) &&
		(state->practice.chapter == BOSS_STAGE) &&
		(start->stage_id == ((STAGES_PER_SCENE * 1) + BOSS_STAGE)) &&
		(start->route == ROUTE_MAKAI)
	) {
		return T1REPLAY_PRIVATE_YUUGENMAGAN_FIRST_COMBAT_TARGET;
	}
#endif

#if T1ELX_DIRECT_TRACE
	if(
		(state->practice.section == T1RPS_BOSS_START) &&
		(state->practice.scene == 2) &&
		(state->practice.route == ROUTE_MAKAI) &&
		(state->practice.chapter == BOSS_STAGE) &&
		(start->stage_id == ((STAGES_PER_SCENE * 2) + BOSS_STAGE)) &&
		(start->route == ROUTE_MAKAI)
	) {
		return T1REPLAY_PRIVATE_ELIS_FIRST_COMBAT_TARGET;
	}
#endif

#if T1KIK_DIRECT_TRACE
	if(
		(state->practice.section == T1RPS_BOSS_START) &&
		(state->practice.scene == 2) &&
		(state->practice.route == ROUTE_JIGOKU) &&
		(state->practice.chapter == BOSS_STAGE) &&
		(start->stage_id == ((STAGES_PER_SCENE * 2) + BOSS_STAGE)) &&
		(start->route == ROUTE_JIGOKU)
	) {
		return T1REPLAY_PRIVATE_KIKURI_FIRST_COMBAT_TARGET;
	}
#endif

#if T1SAR_DIRECT_TRACE
	if(
		(state->practice.section == T1RPS_BOSS_START) &&
		(state->practice.scene == 3) &&
		(state->practice.route == ROUTE_MAKAI) &&
		(state->practice.chapter == BOSS_STAGE) &&
		(start->stage_id == ((STAGES_PER_SCENE * 3) + BOSS_STAGE)) &&
		(start->route == ROUTE_MAKAI)
	) {
		return T1REPLAY_PRIVATE_SARIEL_FIRST_COMBAT_TARGET;
	}
#endif

	if(
		(state->practice.section != T1RPS_BOSS_PHASE) ||
		(state->practice.chapter != BOSS_STAGE)
	) {
		return T1RPBPT_NONE;
	}
	if(
		(state->practice.scene == 0) &&
		(state->practice.route == ROUTE_MAKAI) &&
		(start->stage_id == BOSS_STAGE) &&
		(start->route == ROUTE_MAKAI)
	) {
		return T1RPBPT_SINGYOKU_FIRST_COMBAT;
	}
	if(
		(state->practice.scene == 1) &&
		(state->practice.route == ROUTE_JIGOKU) &&
		(start->stage_id == ((1 * STAGES_PER_SCENE) + BOSS_STAGE)) &&
		(start->route == ROUTE_JIGOKU)
	) {
		return T1RPBPT_MIMA_FIRST_COMBAT;
	}
	return T1RPBPT_NONE;
}

static void t1replay_start_capture(void)
{
	int i;
	t1replay_start_t far *start = &t1replay_header.start;

	t1replay_memclear(start, sizeof(*start));
	start->resident_rand = resident->rand;
	start->score = resident->score;
	start->continues_total = resident->continues_total;
	start->hiscore = resident->hiscore;
	start->score_highest = resident->score_highest;
	for(i = 0; i < (STAGES_PER_SCENE - 1); i++) {
		start->bonus_per_stage[i] = resident->bonus_per_stage[i];
		start->continues_per_scene[i] = resident->continues_per_scene[i];
	}
	start->stage_id = resident->stage_id;
	start->point_value = resident->point_value;
	start->pellet_speed = resident->pellet_speed;
	start->rank = resident->rank;
	start->bgm_mode = static_cast<int8_t>(resident->bgm_mode);
	start->rem_bombs = resident->rem_bombs;
	start->credit_lives_extra = resident->credit_lives_extra;
	start->rem_lives = resident->rem_lives;
	start->route = resident->route;
	start->end_flag = static_cast<int8_t>(resident->end_flag);
	start->debug_mode = static_cast<int8_t>(resident->debug_mode);
	start->snd_need_init = resident->snd_need_init;
	start->start_binary = T1REPLAY_PROCESS_REIIDEN;
	start->practice_boss_phase = t1replay_practice_boss_phase_from_restart(start);
#if T1ELX_NATURAL_TRACE
	if(
		(t1replay_mode == T1RM_RECORD) &&
		(start->stage_id == ((STAGES_PER_SCENE * 2) + BOSS_STAGE)) &&
		(start->route == ROUTE_MAKAI) &&
		(start->practice_boss_phase == T1RPBPT_NONE)
	) {
		t1elx_natural_prepare();
	}
#endif
#if T1KIK_NATURAL_TRACE
	if(
		(t1replay_mode == T1RM_RECORD) &&
		(start->stage_id == ((STAGES_PER_SCENE * 2) + BOSS_STAGE)) &&
		(start->route == ROUTE_JIGOKU) &&
		(start->practice_boss_phase == T1RPBPT_NONE)
	) {
		t1kik_natural_prepare();
	}
#endif
#if T1SAR_NATURAL_TRACE
	if(
		(t1replay_mode == T1RM_RECORD) &&
		(start->stage_id == ((STAGES_PER_SCENE * 3) + BOSS_STAGE)) &&
		(start->route == ROUTE_MAKAI) &&
		(start->practice_boss_phase == T1RPBPT_NONE)
	) {
		t1sar_natural_prepare();
	}
#endif
}

static void t1replay_start_apply(void)
{
	int i;
	const t1replay_start_t far *start = &t1replay_header.start;

	resident->rand = start->resident_rand;
	resident->score = start->score;
	resident->continues_total = start->continues_total;
	resident->hiscore = start->hiscore;
	resident->score_highest = start->score_highest;
	for(i = 0; i < (STAGES_PER_SCENE - 1); i++) {
		resident->bonus_per_stage[i] = start->bonus_per_stage[i];
		resident->continues_per_scene[i] = start->continues_per_scene[i];
	}
	resident->stage_id = start->stage_id;
	resident->point_value = start->point_value;
	resident->pellet_speed = start->pellet_speed;
	resident->rank = start->rank;
	resident->bgm_mode = static_cast<bgm_mode_t>(start->bgm_mode);
	resident->rem_bombs = start->rem_bombs;
	resident->credit_lives_extra = start->credit_lives_extra;
	resident->rem_lives = start->rem_lives;
	resident->route = start->route;
	resident->end_flag = static_cast<end_sequence_t>(start->end_flag);
	resident->debug_mode = DM_OFF;
	resident->snd_need_init = start->snd_need_init;
}

static void t1replay_header_capture(void)
{
	int i;

	t1replay_memclear(&t1replay_header, sizeof(t1replay_header));
	t1replay_header.magic[0] = 'T'; t1replay_header.magic[1] = '1';
	t1replay_header.magic[2] = 'R'; t1replay_header.magic[3] = 'P';
	t1replay_header.magic[4] = 'Y'; t1replay_header.magic[5] = '5';
	t1replay_header.version = T1REPLAY_VERSION;
	t1replay_header.header_size = T1REPLAY_HEADER_SIZE;
	t1replay_header.packet_size = T1REPLAY_PACKET_SIZE;
	t1replay_header.flags = T1REPLAY_FLAGS_KNOWN;
	t1replay_header.status = T1REPLAY_STATUS_RECORDING;
	t1replay_header.game_id = 1;
	t1replay_header.input_semantics = T1REPLAY_INPUT_SEMANTICS_LATCHED_GROUPS;
	t1replay_header.input_offset = T1REPLAY_HEADER_SIZE;
	for(i = 0; i < T1REPLAY_NAME_BYTES; i++) {
		t1replay_header.name[i] = ' ';
	}
	t1replay_header.summary.final_stage_id = T1REPLAY_FINAL_STAGE_NONE;
	t1replay_start_capture();
	t1replay_header.start_checksum = t1replay_fnv1a(
		T1REPLAY_FNV1A_BASIS, &t1replay_header.start,
		sizeof(t1replay_header.start)
	);
}

static void t1replay_checkpoint_scenario_capture(
	t1replay_checkpoint_scenario_t far *scenario
)
{
	int i;

	scenario->resident_rand = resident->rand;
	scenario->resident_score = resident->score;
	scenario->resident_continues_total = resident->continues_total;
	scenario->resident_hiscore = resident->hiscore;
	scenario->resident_score_highest = resident->score_highest;
	for(i = 0; i < (STAGES_PER_SCENE - 1); i++) {
		scenario->resident_bonus_per_stage[i] = resident->bonus_per_stage[i];
		scenario->resident_continues_per_scene[i] =
			resident->continues_per_scene[i];
	}
	scenario->game_score = score;
	scenario->game_continues_total = continues_total;
	scenario->reserved_0 = 0;
	scenario->resident_stage_id = resident->stage_id;
	scenario->resident_point_value = resident->point_value;
	scenario->resident_pellet_speed = resident->pellet_speed;
	scenario->resident_rank = resident->rank;
	scenario->resident_bgm_mode = static_cast<int8_t>(resident->bgm_mode);
	scenario->resident_rem_bombs = resident->rem_bombs;
	scenario->resident_credit_lives_extra = resident->credit_lives_extra;
	scenario->resident_end_flag = static_cast<int8_t>(resident->end_flag);
	scenario->resident_route = resident->route;
	scenario->resident_rem_lives = resident->rem_lives;
	scenario->resident_snd_need_init = resident->snd_need_init;
	scenario->resident_debug_mode = resident->debug_mode;
	scenario->game_rank = rank;
	scenario->game_bgm_mode = static_cast<int8_t>(bgm_mode);
	scenario->game_rem_bombs = rem_bombs;
	scenario->game_credit_lives_extra = credit_lives_extra;
	scenario->game_route = route;
	scenario->game_rem_lives = static_cast<int8_t>(rem_lives);
	scenario->mode_test = static_cast<int8_t>(mode_test);
}

#if T1REPLAY_CHECKPOINT_EMIT
static bool t1replay_checkpoint_write(void)
{
	int fd = t1replay_dos_create(t1replay_checkpoint_fn);

	if(fd < 0) {
		return false;
	}
	if(t1replay_dos_write(fd, &t1replay_checkpoint,
		sizeof(t1replay_checkpoint)) != sizeof(t1replay_checkpoint)) {
		t1replay_dos_close(fd);
		return false;
	}
	t1replay_dos_close(fd);
	return true;
}

// This optional write is deliberately process-end-only. First-frame capture is
// BSS-only so release recording has no sidecar I/O or startup disk stutter.
static void t1replay_checkpoint_flush_if_enabled(void)
{
	if(
		!t1replay_checkpoint_capture_attempted ||
		!t1replay_res ||
		!t1replay_checkpoint_valid(&t1replay_checkpoint) ||
	#if T1REPLAY_CHECKPOINT_RELEASE
		// Public direct starts are available only for the presentation state
		// covered by the fresh-process two-page oracle. All other captures stay
		// BSS-only and therefore cannot advertise a sidecar to OP.
		!t1replay_ckpt_present_valid(&t1replay_checkpoint) ||
	#endif
		!t1replay_checkpoint_path_set(
			t1replay_res->slot, t1replay_checkpoint.pacing.process_seq
		)
	) {
		return;
	}
	// The base replay has already committed and flushed. Sidecar failure must
	// not change its carrier, status, crash prefix, or executable handoff.
	t1replay_checkpoint_write();
}
#endif

static uint32_t t1replay_res_checksum(void)
{
	uint32_t checksum;

	t1replay_res->checksum = 0;
	checksum = t1replay_fnv1a(
		T1REPLAY_FNV1A_BASIS,
		&t1replay_res->magic,
		(sizeof(*t1replay_res) - offsetof(t1replay_res_t, magic))
	);
	t1replay_res->checksum = checksum;
	return checksum;
}

static bool t1replay_res_valid(void)
{
	uint32_t stored;
	uint32_t computed;

	if(!t1replay_res ||
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
		!t1replay_bytes_zero(
			t1replay_res->reserved, sizeof(t1replay_res->reserved)
		) ||
		((t1replay_res->source_process != T1REPLAY_PROCESS_NONE) &&
		 (t1replay_res->source_process != T1REPLAY_PROCESS_REIIDEN)) ||
		((t1replay_res->target_process != T1REPLAY_PROCESS_REIIDEN) &&
		 (t1replay_res->target_process != T1REPLAY_PROCESS_FUUIN)) ||
		((t1replay_res->mode == T1RM_RECORD) &&
		 !t1rpg_state_valid(&t1replay_res->guard)) ||
		((t1replay_res->mode == T1RM_RECORD) &&
		 (t1replay_res->guard.sample_count != t1replay_res->sample_count)) ||
		((t1replay_res->mode == T1RM_PLAYBACK) &&
		 !t1rpg_state_empty(&t1replay_res->guard)) ||
		(t1replay_res->slow_frames > t1replay_res->timed_frames)) {
		return false;
	}
	stored = t1replay_res->checksum;
	computed = t1replay_res_checksum();
	t1replay_res->checksum = stored;
	return (stored == computed);
}

static bool t1replay_res_open(const char *id, bool create)
{
	t1replay_res = ResData<t1replay_res_t>::exist(id);
	if(!t1replay_res && create) {
		t1replay_res = ResData<t1replay_res_t>::create(id);
	}
	return (t1replay_res != 0);
}

static void t1replay_res_clear(void)
{
	if(t1replay_res) {
		resdata_free(reinterpret_cast<void __seg *>(t1replay_res));
		t1replay_res = 0;
	}
}

static void t1replay_res_store(void)
{
	t1replay_res->mode = static_cast<uint8_t>(t1replay_mode);
	if(t1replay_mode == T1RM_RECORD) {
		// A record-mode handoff is immediately after buffer_flush(). The file
		// header, not the unused playback cursors, is the durable total.
		t1replay_res->sample_count = t1replay_header.sample_count;
		t1replay_res->packet_count = t1replay_header.packet_count;
		t1replay_res->input_size = t1replay_header.input_size;
	} else {
		t1replay_res->sample_count = t1replay_sample_cursor;
		t1replay_res->packet_count = t1replay_packet_cursor;
		t1replay_res->input_size = t1replay_payload_written;
	}
	t1replay_res->payload_checksum = t1replay_payload_checksum;
	t1replay_res->start_checksum = t1replay_header.start_checksum;
	t1replay_res->timed_frames = t1replay_header.timed_frames;
	t1replay_res->slow_frames = t1replay_header.slow_frames;
	t1replay_res_checksum();
}

static bool t1replay_res_matches_header(void)
{
	t1replay_stream_state_t state;

	if(
		(t1replay_res->packet_count >
			(T1REPLAY_INPUT_SIZE_MAX / T1REPLAY_PACKET_SIZE)) ||
		(t1replay_res->input_size !=
			(t1replay_res->packet_count * T1REPLAY_PACKET_SIZE)) ||
		(t1replay_res->start_checksum != t1replay_header.start_checksum) ||
		(t1replay_res->timed_frames != t1replay_header.timed_frames) ||
		(t1replay_res->slow_frames != t1replay_header.slow_frames) ||
		(t1replay_res->target_process != T1REPLAY_PROCESS_REIIDEN) ||
		(t1replay_res->handoff_checksum != 0)
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
		(state.process == t1replay_res->target_process) &&
		(state.process_seq == t1replay_res->process_seq) &&
		(state.source_process == t1replay_res->source_process)
	);
}

static t1replay_mode_t t1replay_command_load(
	uint8_t far *slot, bool *checkpoint_direct
)
{
	t1replay_command_t command;
	char witness_fn[10];
	uint32_t size;
	int fd;
	bool valid;
	bool direct;

	*checkpoint_direct = false;

	// T1RPY.CFG is authoritative. The second copy only forced another DOS
	// directory mutation before execl(), so its absence never rejects a valid
	// primary and its contents are never parsed.
	t1replay_command_witness_fn_init(witness_fn);
	t1replay_dos_delete(witness_fn);
	fd = t1replay_dos_open(t1replay_command_fn, T1REPLAY_DOS_ACCESS_READ);
	if(fd < 0) {
		return T1RM_DISABLED;
	}
	valid = (
		t1replay_dos_size(fd, &size) && t1replay_dos_seek(fd, 0) &&
		(size == sizeof(command)) &&
		(t1replay_dos_read(fd, &command, sizeof(command)) == sizeof(command))
	);
	t1replay_dos_close(fd);
	// A command is a one-shot request, not durable replay state. Consume every
	// opened command, including malformed ones, before considering its fields.
	if(!t1replay_dos_delete(t1replay_command_fn)) {
		t1replay_command_delete_failed = true;
		return T1RM_DISABLED;
	}
	if(!valid) {
		return T1RM_DISABLED;
	}
	direct = (
		(command.reserved[0] == T1REPLAY_COMMAND_DIRECT_CHECKPOINT) &&
		t1replay_bytes_zero(&command.reserved[1],
			(sizeof(command.reserved) - 1))
	);
	if(
		!t1replay_magic_matches(command.magic, 'C') ||
		((command.mode != T1REPLAY_COMMAND_RECORD) &&
		 (command.mode != T1REPLAY_COMMAND_PLAYBACK)) ||
		!t1replay_slot_valid_for_mode(command.mode, command.slot) ||
		(!t1replay_bytes_zero(command.reserved, sizeof(command.reserved)) &&
			(!direct || (command.mode != T1REPLAY_COMMAND_PLAYBACK)))
	) {
		return T1RM_DISABLED;
	}
	*slot = command.slot;
	*checkpoint_direct = direct;
	return static_cast<t1replay_mode_t>(command.mode);
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
	t1replay_command_delete_failed = false;
	t1replay_terminal_pending = false;
	t1replay_fast_forward_boundary_reset();
	t1replay_pause_action = T1RPA_RESUME;
	t1replay_checkpoint_capture_attempted = false;
	t1replay_checkpoint_restore_is_pending = false;
	t1replay_memclear(t1replay_keys, sizeof(t1replay_keys));
#if T1REPLAY_PRIVATE_PIXEL_TRACE
	t1replay_pixel_probe_reset();
#endif
#if T1ELX_TRACE
	t1elx_trace_reset();
#endif
#if T1KIK_TRACE
	t1kik_trace_reset();
#endif
#if T1SAR_TRACE
	t1sar_trace_reset();
#endif
#if T1REPLAY_EXACT_TRACE
	t1replay_exact_trace_ready = false;
	t1replay_exact_trace_failed = false;
	t1replay_exact_terminal_pending = false;
	t1replay_exact_last_sample = 0xFFFFFFFFUL;
	t1replay_exact_last_kind = 0;
	t1replay_exact_pellet_speed_raise_cycle = 0;
	t1replay_exact_trace_buffer_count = 0;
#endif
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
	return t1replay_header_write(false);
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
	if(t1replay_buffer_len >= T1REPLAY_BUFFER_PACKET_COUNT) {
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

	if(t1replay_pending_valid && (t1replay_pending_run < T1REPLAY_PACKET_RUN_MAX)) {
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
			(remaining > T1REPLAY_BUFFER_PACKET_COUNT) ?
				T1REPLAY_BUFFER_PACKET_COUNT : remaining
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

	if(t1replay_header.process_count == 0xFF) {
		return false;
	}
	t1replay_memclear(&packet, sizeof(packet));
	packet.tag = static_cast<uint8_t>(T1REPLAY_PACKET_CONTROL | control);
	packet.keys[0] = T1REPLAY_PROCESS_REIIDEN;
	packet.keys[1] = t1replay_res->process_seq;
	packet.keys[2] = value;
	if(!t1replay_packet_commit(&packet)) {
		return false;
	}
	t1replay_header.process_count++;
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
		(packet.keys[0] == T1REPLAY_PROCESS_REIIDEN) &&
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
		t1replay_header_write(false);
	}
	t1replay_res_clear();
	t1replay_mode = T1RM_DISABLED;
}

static void t1replay_fail_and_abort_if_playback(void)
{
	t1replay_fail();
	if(t1replay_abort_pending) {
		t1replay_abort_to_op();
	}
}

// A malformed carrier cannot establish whether its previous owner recorded or
// played back. Its presence nevertheless proves that this REIIDEN launch is a
// replay handoff, so it must fail closed rather than reach native input.
static void t1replay_abort_before_start(void)
{
	t1replay_res_clear();
	t1replay_mode = T1RM_DISABLED;
	t1replay_abort_pending = true;
}

void far t1replay_entry(void)
{
	uint8_t slot;
	t1replay_mode_t command_mode;
	resident_t far *start_resident;
	char res_id[sizeof(T1REPLAY_RES_ID)];
	char resident_id[sizeof(RES_ID)];
	bool resumed = false;
	bool checkpoint_direct = false;

	t1replay_paths_init();
	t1replay_res_id_init(res_id);
	t1replay_resident_id_init(resident_id);
	t1replay_state_reset();
	t1replay_abort_pending = false;
	t1replay_mode = T1RM_DISABLED;
	t1replay_res = 0;

	if(t1replay_res_open(res_id, false)) {
		if(t1replay_res_valid()) {
			resumed = true;
			t1replay_mode = static_cast<t1replay_mode_t>(t1replay_res->mode);
			t1replay_slot_set(t1replay_res->slot);
			if(!t1replay_header_read(t1replay_mode == T1RM_PLAYBACK) ||
				!t1replay_res_matches_header()) {
				t1replay_fail();
				return;
			}
			if((t1replay_mode == T1RM_RECORD) &&
				!t1rpg_verify(&t1replay_res->guard)) {
				if(t1replay_res->guard.flags & T1REPLAY_GUARD_FLAG_INVALID) {
					t1rpg_poison(&t1replay_res->guard);
				}
				t1replay_fail();
				return;
			}
			t1replay_payload_written = t1replay_res->input_size;
			t1replay_packet_cursor = t1replay_res->packet_count;
			t1replay_sample_cursor = t1replay_res->sample_count;
			t1replay_payload_checksum = t1replay_res->payload_checksum;
			#if T1REPLAY_CHECKPOINT_PRIVATE_RESTORE
			if(t1replay_mode == T1RM_PLAYBACK) {
				start_resident = ResData<resident_t>::exist(resident_id);
				resident = start_resident;
				if(!t1replay_checkpoint_restore_prepare(
					start_resident, t1replay_res->slot,
					t1replay_res->process_seq, t1replay_res->source_process
				)) {
					t1replay_fail();
					return;
				}
				t1replay_res_store();
			}
#elif T1REPLAY_PIXEL_TRACE
			if(t1replay_mode == T1RM_PLAYBACK) {
				start_resident = ResData<resident_t>::exist(resident_id);
				resident = start_resident;
				if(!t1replay_checkpoint_probe_prepare(
					start_resident, t1replay_res->slot,
					t1replay_res->process_seq, t1replay_res->source_process
				)) {
					t1replay_fail();
					return;
				}
			}
#endif
		} else {
			t1replay_abort_before_start();
			return;
		}
	}
	#if defined(T1RB)
	if(resumed) {
		t1replay_process_milestone(T1RPM_CARRIER_ACCEPTED);
	}
	#endif
	if(!resumed) {
		if(t1replay_res) {
			t1replay_res_clear();
		}
		command_mode = t1replay_command_load(
			&slot, &checkpoint_direct
		);
		if(command_mode == T1RM_DISABLED) {
			#if T1REPLAY_PROCESS_MILESTONES
			t1replay_process_milestone(T1RPM_COMMAND_REJECTED);
			#endif
			if(t1replay_command_delete_failed) {
				t1replay_abort_before_start();
			}
			return;
		}
		#if T1REPLAY_PROCESS_MILESTONES
		t1replay_process_milestone(T1RPM_COMMAND_ACCEPTED);
		#endif
		t1replay_slot_set(slot);
		start_resident = ResData<resident_t>::exist(resident_id);
		if(!start_resident) {
			#if T1REPLAY_PROCESS_MILESTONES
			t1replay_process_milestone(T1RPM_RESIDENT_MISSING);
			#endif
			if(command_mode == T1RM_PLAYBACK) {
				t1replay_mode = T1RM_PLAYBACK;
				t1replay_fail();
			}
			return;
		}
		#if T1REPLAY_PROCESS_MILESTONES
		t1replay_process_milestone(T1RPM_RESIDENT_FOUND);
		#endif
		resident = start_resident;
		if(command_mode == T1RM_RECORD) {
			t1replay_mode = T1RM_RECORD;
			t1replay_header_capture();
			#if T1REPLAY_PROCESS_MILESTONES
			t1replay_process_milestone(T1RPM_HEADER_CAPTURED);
			#endif
			if(!t1replay_start_valid(&t1replay_header.start) ||
				!t1replay_header_write(true)) {
				#if T1REPLAY_PROCESS_MILESTONES
				t1replay_process_milestone(T1RPM_HEADER_WRITE_FAILED);
				#endif
				t1replay_mode = T1RM_DISABLED;
				return;
			}
			#if T1REPLAY_PROCESS_MILESTONES
			t1replay_process_milestone(T1RPM_HEADER_WRITTEN);
			#endif
		} else {
			t1replay_mode = T1RM_PLAYBACK;
			if(!t1replay_header_read(true)) {
				t1replay_fail();
				return;
			}
		#if T1REPLAY_CHECKPOINT_RESTORE
			t1replay_start_apply();
			if(
				(t1replay_header.start.practice_boss_phase == T1RPBPT_NONE) &&
				(T1REPLAY_CHECKPOINT_PRIVATE_RESTORE || checkpoint_direct)
			) {
				if(!t1replay_checkpoint_restore_prepare(
					start_resident, slot, 0, T1REPLAY_PROCESS_NONE
				)) {
					t1replay_fail();
					return;
				}
			}
		#else
			t1replay_start_apply();
#if T1REPLAY_PIXEL_TRACE
			if(!t1replay_checkpoint_probe_prepare(
				start_resident, slot, 0, T1REPLAY_PROCESS_NONE
			)) {
				t1replay_fail();
				return;
			}
#endif
#endif
		}
		if(t1replay_header.start.practice_boss_phase != T1RPBPT_NONE) {
			if(checkpoint_direct) {
				t1replay_fail();
				return;
			}
			t1replay_checkpoint_restore_is_pending = true;
		}
		if(!t1replay_res_open(res_id, true)) {
			#if T1REPLAY_PROCESS_MILESTONES
			t1replay_process_milestone(T1RPM_REPLAY_RES_CREATE_FAILED);
			#endif
			if(t1replay_mode == T1RM_PLAYBACK) {
				t1replay_fail();
			} else {
				t1replay_mode = T1RM_DISABLED;
			}
			return;
		}
		#if T1REPLAY_PROCESS_MILESTONES
		t1replay_process_milestone(T1RPM_REPLAY_RES_CREATED);
		#endif
		t1replay_memclear(&t1replay_res->magic,
			(sizeof(*t1replay_res) - offsetof(t1replay_res_t, magic)));
		t1replay_res->magic[0] = 'T'; t1replay_res->magic[1] = '1';
		t1replay_res->magic[2] = 'R'; t1replay_res->magic[3] = 'S';
		t1replay_res->version = T1REPLAY_RES_VERSION;
		t1replay_res->slot = slot;
		t1replay_res->source_process = T1REPLAY_PROCESS_NONE;
		t1replay_res->target_process = T1REPLAY_PROCESS_REIIDEN;
		if((t1replay_mode == T1RM_RECORD) &&
			!t1replay_guard_begin(&t1replay_res->guard)) {
			#if T1REPLAY_PROCESS_MILESTONES
			t1replay_process_milestone(T1RPM_GUARD_BEGIN_FAILED);
			#endif
			t1replay_fail();
			return;
		}
		#if T1REPLAY_PROCESS_MILESTONES
		if(t1replay_mode == T1RM_RECORD) {
			t1replay_process_milestone(T1RPM_GUARD_READY);
		}
		#endif
		t1replay_res_store();
		#if defined(T1RB)
		t1replay_process_milestone(T1RPM_CARRIER_ACCEPTED);
		#endif
	}
}

bool16 far t1replay_checkpoint_restore_pending(void)
{
	return t1replay_checkpoint_restore_is_pending;
}

static void t1replay_checkpoint_scenario_apply(
	const t1replay_checkpoint_scenario_t far *scenario
)
{
	int i;

	resident->rand = scenario->resident_rand;
	resident->score = scenario->resident_score;
	resident->continues_total = scenario->resident_continues_total;
	resident->hiscore = scenario->resident_hiscore;
	resident->score_highest = scenario->resident_score_highest;
	for(i = 0; i < (STAGES_PER_SCENE - 1); i++) {
		resident->bonus_per_stage[i] = scenario->resident_bonus_per_stage[i];
		resident->continues_per_scene[i] =
			scenario->resident_continues_per_scene[i];
	}
	resident->stage_id = scenario->resident_stage_id;
	resident->point_value = scenario->resident_point_value;
	resident->pellet_speed = scenario->resident_pellet_speed;
	resident->rank = scenario->resident_rank;
	resident->bgm_mode = static_cast<bgm_mode_t>(scenario->resident_bgm_mode);
	resident->rem_bombs = scenario->resident_rem_bombs;
	resident->credit_lives_extra = scenario->resident_credit_lives_extra;
	resident->end_flag = static_cast<end_sequence_t>(scenario->resident_end_flag);
	resident->route = scenario->resident_route;
	resident->rem_lives = scenario->resident_rem_lives;
	resident->snd_need_init = scenario->resident_snd_need_init;
	resident->debug_mode = DM_OFF;

	score = scenario->game_score;
	continues_total = scenario->game_continues_total;
	rank = scenario->game_rank;
	bgm_mode = static_cast<bgm_mode_t>(scenario->game_bgm_mode);
	rem_bombs = scenario->game_rem_bombs;
	credit_lives_extra = scenario->game_credit_lives_extra;
	route = scenario->game_route;
	rem_lives = scenario->game_rem_lives;
	mode_test = false;
	extend_next = ((score / SCORE_PER_EXTEND) + 1);
}

#if T1REPLAY_KONNGARA_PHASE1_DIRECT_TRACE
static bool t1replay_practice_konngara_phase1_restore_apply(
	int *pellet_speed_raise_cycle
)
{
	const t1replay_start_t far *start = &t1replay_header.start;

	if(
		!t1replay_checkpoint_restore_is_pending || !pellet_speed_raise_cycle ||
		((t1replay_mode != T1RM_RECORD) &&
		 (t1replay_mode != T1RM_PLAYBACK)) ||
		!resident || !t1replay_res ||
		(t1replay_res->process_seq != 0) ||
		(t1replay_res->source_process != T1REPLAY_PROCESS_NONE) ||
		!t1replay_practice_boss_phase_start_valid(start) ||
		(start->practice_boss_phase != T1REPLAY_PRIVATE_KONNGARA_PHASE1_TARGET) ||
		(resident->stage_id != ((STAGES_PER_SCENE * 3) + BOSS_STAGE)) ||
		(resident->route != ROUTE_JIGOKU) ||
		(boss_id != BID_KONNGARA) ||
		!t1boss_konngara_phase1_direct_construct()
	) {
		t1replay_checkpoint_restore_is_pending = false;
		t1replay_fail();
		return false;
	}

	// This is the native post-phase-0 first-input seam, with no checkpoint.
	timer_initialized = true;
	irand_init(frame_rand);
	bomb_doubletap_frame = BOMB_DOUBLETAP_WINDOW;
	first_stage_in_scene = false;
	frame_rand++;
	*pellet_speed_raise_cycle = (
		1800 - (rem_lives * 200) - (rem_bombs * 50)
	);
	if((frame_rand % *pellet_speed_raise_cycle) == 0) {
		pellet_speed_raise(0.025f);
	}
	t1replay_checkpoint_restore_is_pending = false;
	return true;
}
#endif

#if T1YMX_DIRECT_TRACE
static bool t1replay_practice_yuugenmagan_first_combat_restore_apply(
	int *pellet_speed_raise_cycle
)
{
	const t1replay_start_t far *start = &t1replay_header.start;

	if(
		!t1replay_checkpoint_restore_is_pending || !pellet_speed_raise_cycle ||
		((t1replay_mode != T1RM_RECORD) &&
		 (t1replay_mode != T1RM_PLAYBACK)) ||
		!resident || !t1replay_res ||
		(t1replay_res->process_seq != 0) ||
		(t1replay_res->source_process != T1REPLAY_PROCESS_NONE) ||
		!t1replay_practice_boss_phase_start_valid(start) ||
		(start->practice_boss_phase !=
			T1REPLAY_PRIVATE_YUUGENMAGAN_FIRST_COMBAT_TARGET) ||
		(resident->stage_id != ((STAGES_PER_SCENE * 1) + BOSS_STAGE)) ||
		(resident->route != ROUTE_MAKAI) ||
		(boss_id != BID_YUUGENMAGAN) ||
		!t1boss_yuugenmagan_first_combat_direct_construct()
	) {
		t1replay_checkpoint_restore_is_pending = false;
		t1replay_fail();
		return false;
	}

	// This is the native post-entrance first-input branch. It does not replay
	// phase 0 or fabricate its paint, palette, input, or random activity.
	timer_initialized = true;
	irand_init(frame_rand);
	bomb_doubletap_frame = BOMB_DOUBLETAP_WINDOW;
	first_stage_in_scene = false;
	frame_rand++;
	*pellet_speed_raise_cycle = (
		1800 - (rem_lives * 200) - (rem_bombs * 50)
	);
	if((frame_rand % *pellet_speed_raise_cycle) == 0) {
		pellet_speed_raise(0.025f);
	}
	t1replay_checkpoint_restore_is_pending = false;
	return true;
}
#endif

#if T1ELX_DIRECT_TRACE
static bool t1replay_practice_elis_first_combat_restore_apply(
	int *pellet_speed_raise_cycle
)
{
	const t1replay_start_t far *start = &t1replay_header.start;

	if(
		!t1replay_checkpoint_restore_is_pending || !pellet_speed_raise_cycle ||
		((t1replay_mode != T1RM_RECORD) &&
		 (t1replay_mode != T1RM_PLAYBACK)) ||
		!resident || !t1replay_res ||
		(t1replay_res->process_seq != 0) ||
		(t1replay_res->source_process != T1REPLAY_PROCESS_NONE) ||
		!t1replay_practice_boss_phase_start_valid(start) ||
		(start->practice_boss_phase !=
			T1REPLAY_PRIVATE_ELIS_FIRST_COMBAT_TARGET) ||
		(resident->stage_id != ((STAGES_PER_SCENE * 2) + BOSS_STAGE)) ||
		(resident->route != ROUTE_MAKAI) ||
		(boss_id != BID_ELIS) ||
		!t1boss_elis_practice_first_combat_apply() ||
		!t1elx_direct_prepare()
	) {
		t1replay_checkpoint_restore_is_pending = false;
		t1replay_fail();
		return false;
	}

	// The owner leaves a carrier for elis_main() to consume. This follows the
	// same native post-entrance input branch as a sequential run.
	timer_initialized = true;
	irand_init(frame_rand);
	bomb_doubletap_frame = BOMB_DOUBLETAP_WINDOW;
	first_stage_in_scene = false;
	frame_rand++;
	*pellet_speed_raise_cycle = (
		1800 - (rem_lives * 200) - (rem_bombs * 50)
	);
	if((frame_rand % *pellet_speed_raise_cycle) == 0) {
		pellet_speed_raise(0.025f);
	}
	t1replay_checkpoint_restore_is_pending = false;
	return true;
}
#endif

#if T1KIK_DIRECT_TRACE
static bool t1replay_practice_kikuri_first_combat_restore_apply(
	int *pellet_speed_raise_cycle
)
{
	const t1replay_start_t far *start = &t1replay_header.start;

	if(
		!t1replay_checkpoint_restore_is_pending || !pellet_speed_raise_cycle ||
		((t1replay_mode != T1RM_RECORD) &&
		 (t1replay_mode != T1RM_PLAYBACK)) ||
		!resident || !t1replay_res ||
		(t1replay_res->process_seq != 0) ||
		(t1replay_res->source_process != T1REPLAY_PROCESS_NONE) ||
		!t1replay_practice_boss_phase_start_valid(start) ||
		(start->practice_boss_phase !=
			T1REPLAY_PRIVATE_KIKURI_FIRST_COMBAT_TARGET) ||
		(resident->stage_id != ((STAGES_PER_SCENE * 2) + BOSS_STAGE)) ||
		(resident->route != ROUTE_JIGOKU) ||
		(boss_id != BID_KIKURI) ||
		!t1boss_kikuri_first_combat_profile_apply_loaded() ||
		!t1kik_direct_prepare()
	) {
		t1replay_checkpoint_restore_is_pending = false;
		t1replay_fail();
		return false;
	}

	// Resume through the same native post-entrance branch as sequential
	// Phase 2. The owner already restored the source-owned page and palette.
	timer_initialized = true;
	irand_init(frame_rand);
	bomb_doubletap_frame = BOMB_DOUBLETAP_WINDOW;
	first_stage_in_scene = false;
	frame_rand++;
	*pellet_speed_raise_cycle = (
		1800 - (rem_lives * 200) - (rem_bombs * 50)
	);
	if((frame_rand % *pellet_speed_raise_cycle) == 0) {
		pellet_speed_raise(0.025f);
	}
	t1replay_checkpoint_restore_is_pending = false;
	return true;
}
#endif

#if T1SAR_DIRECT_TRACE
static bool t1replay_practice_sariel_first_combat_restore_apply(
	int *pellet_speed_raise_cycle
)
{
	const t1replay_start_t far *start = &t1replay_header.start;

	if(
		!t1replay_checkpoint_restore_is_pending || !pellet_speed_raise_cycle ||
		((t1replay_mode != T1RM_RECORD) &&
		 (t1replay_mode != T1RM_PLAYBACK)) ||
		!resident || !t1replay_res ||
		(t1replay_res->process_seq != 0) ||
		(t1replay_res->source_process != T1REPLAY_PROCESS_NONE) ||
		!t1replay_practice_boss_phase_start_valid(start) ||
		(start->practice_boss_phase !=
			T1REPLAY_PRIVATE_SARIEL_FIRST_COMBAT_TARGET) ||
		(resident->stage_id != ((STAGES_PER_SCENE * 3) + BOSS_STAGE)) ||
		(resident->route != ROUTE_MAKAI) ||
		(boss_id != BID_SARIEL) ||
		!t1boss_sariel_first_combat_direct_construct()
	) {
		t1replay_checkpoint_restore_is_pending = false;
		t1replay_fail();
		return false;
	}

	// Resume at the source-owned post-entrance seam. This profile neither
	// substitutes a checkpoint nor replays Sariel's blocking entrance.
	timer_initialized = true;
	irand_init(frame_rand);
	bomb_doubletap_frame = BOMB_DOUBLETAP_WINDOW;
	first_stage_in_scene = false;
	frame_rand++;
	*pellet_speed_raise_cycle = (
		1800 - (rem_lives * 200) - (rem_bombs * 50)
	);
	if((frame_rand % *pellet_speed_raise_cycle) == 0) {
		pellet_speed_raise(0.025f);
	}
	t1replay_checkpoint_restore_is_pending = false;
	return true;
}
#endif

static bool t1replay_practice_boss_phase_restore_apply(
	int *pellet_speed_raise_cycle
)
{
	const t1replay_start_t far *start = &t1replay_header.start;
	bool constructed = false;

	if(
		!t1replay_checkpoint_restore_is_pending || !pellet_speed_raise_cycle ||
		((t1replay_mode != T1RM_RECORD) &&
		 (t1replay_mode != T1RM_PLAYBACK)) ||
		!resident || !t1replay_res ||
		(t1replay_res->process_seq != 0) ||
		(t1replay_res->source_process != T1REPLAY_PROCESS_NONE) ||
		!t1replay_practice_boss_phase_start_valid(start) ||
		(start->practice_boss_phase == T1RPBPT_NONE)
	) {
		t1replay_checkpoint_restore_is_pending = false;
		t1replay_fail();
		return false;
	}
	if(start->practice_boss_phase == T1RPBPT_SINGYOKU_FIRST_COMBAT) {
		constructed = (
			(resident->stage_id == BOSS_STAGE) &&
			(resident->route == ROUTE_MAKAI) &&
			(boss_id == BID_SINGYOKU) &&
			t1boss_singyoku_practice_boss_phase_apply(
				start->practice_boss_phase
			)
		);
	} else if(start->practice_boss_phase == T1RPBPT_MIMA_FIRST_COMBAT) {
		constructed = (
			(resident->stage_id == ((1 * STAGES_PER_SCENE) + BOSS_STAGE)) &&
			(resident->route == ROUTE_JIGOKU) &&
			(boss_id == BID_MIMA) &&
			t1boss_mima_practice_first_combat_construct()
		);
	}
	if(!constructed) {
		t1replay_checkpoint_restore_is_pending = false;
		t1replay_fail();
		return false;
	}
	// Match the native post-entrance branch and advance to its first input
	// boundary. This target owns no captured player, bullet, or VRAM state.
	timer_initialized = true;
	irand_init(frame_rand);
	bomb_doubletap_frame = BOMB_DOUBLETAP_WINDOW;
	first_stage_in_scene = false;
	frame_rand++;
	*pellet_speed_raise_cycle = (
		1800 - (rem_lives * 200) - (rem_bombs * 50)
	);
	if((frame_rand % *pellet_speed_raise_cycle) == 0) {
		pellet_speed_raise(0.025f);
	}
	t1replay_checkpoint_restore_is_pending = false;
	return true;
}

bool16 far t1replay_checkpoint_restore_apply(int *pellet_speed_raise_cycle)
{
	const t1replay_checkpoint_t far *checkpoint = &t1replay_checkpoint;

	if(t1replay_header.start.practice_boss_phase != T1RPBPT_NONE) {
#if T1SAR_DIRECT_TRACE
		if(
			t1replay_header.start.practice_boss_phase ==
			T1REPLAY_PRIVATE_SARIEL_FIRST_COMBAT_TARGET
		) {
			return t1replay_practice_sariel_first_combat_restore_apply(
				pellet_speed_raise_cycle
			);
		}
#endif
#if T1REPLAY_KONNGARA_PHASE1_DIRECT_TRACE
		if(
			t1replay_header.start.practice_boss_phase ==
			T1REPLAY_PRIVATE_KONNGARA_PHASE1_TARGET
		) {
			return t1replay_practice_konngara_phase1_restore_apply(
				pellet_speed_raise_cycle
			);
		}
#endif
		#if T1YMX_DIRECT_TRACE
		if(
			t1replay_header.start.practice_boss_phase ==
			T1REPLAY_PRIVATE_YUUGENMAGAN_FIRST_COMBAT_TARGET
		) {
			return t1replay_practice_yuugenmagan_first_combat_restore_apply(
				pellet_speed_raise_cycle
			);
		}
		#endif
		#if T1ELX_DIRECT_TRACE
		if(
			t1replay_header.start.practice_boss_phase ==
			T1REPLAY_PRIVATE_ELIS_FIRST_COMBAT_TARGET
		) {
			return t1replay_practice_elis_first_combat_restore_apply(
				pellet_speed_raise_cycle
			);
		}
		#endif
		#if T1KIK_DIRECT_TRACE
		if(
			t1replay_header.start.practice_boss_phase ==
			T1REPLAY_PRIVATE_KIKURI_FIRST_COMBAT_TARGET
		) {
			return t1replay_practice_kikuri_first_combat_restore_apply(
				pellet_speed_raise_cycle
			);
		}
		#endif
		return t1replay_practice_boss_phase_restore_apply(
			pellet_speed_raise_cycle
		);
	}

	if(
		!t1replay_checkpoint_restore_is_pending || !pellet_speed_raise_cycle ||
		(t1replay_mode != T1RM_PLAYBACK) || !resident ||
		!t1replay_checkpoint_valid(checkpoint) ||
		!t1replay_checkpoint_cross_groups_valid(checkpoint) ||
		(resident->stage_id != checkpoint->scenario.resident_stage_id) ||
		(resident->route != checkpoint->scenario.resident_route) ||
		(boss_id != checkpoint->boss.boss_id)
	) {
		t1replay_checkpoint_restore_is_pending = false;
		t1replay_fail();
		return false;
	}
	if((boss_id == BID_NONE) &&
		!t1replay_stage_checkpoint_import(&checkpoint->stage)) {
		t1replay_checkpoint_restore_is_pending = false;
		t1replay_fail();
		return false;
	}

	t1replay_checkpoint_scenario_apply(&checkpoint->scenario);
	frame_rand = checkpoint->rng.frame_rand;
	random_seed = static_cast<long>(checkpoint->rng.random_seed);
	t1replay_player_checkpoint_import(&checkpoint->player);
	t1replay_orb_checkpoint_import(&checkpoint->orb);
	t1replay_items_checkpoint_import(&checkpoint->items);
	t1replay_pellets_checkpoint_import(&checkpoint->pellets);
	t1replay_shots_checkpoint_import(&checkpoint->shots);
	t1replay_missiles_checkpoint_import(&checkpoint->missiles);
	t1replay_lasers_checkpoint_import(&checkpoint->lasers);
	t1replay_particles_checkpoint_import(&checkpoint->particles);
	if(!t1replay_checkpoint_boss_apply(&checkpoint->boss)) {
		t1replay_checkpoint_restore_is_pending = false;
		t1replay_fail();
		return false;
	}

	frame_since_start_of_binary = checkpoint->pacing.frame_since_start_of_binary;
	bomb_frame = checkpoint->pacing.bomb_frame;
	timer_initialized = checkpoint->pacing.timer_initialized;
	first_stage_in_scene = checkpoint->pacing.first_stage_in_scene;
	stage_wait_for_shot_to_begin =
		checkpoint->pacing.stage_wait_for_shot_to_begin;
	t1replay_timer_checkpoint_import(&checkpoint->pacing);
	*pellet_speed_raise_cycle = checkpoint->pacing.pellet_speed_raise_cycle;
#if T1REPLAY_CHECKPOINT_RESTORE
	if(!t1replay_ckpt_present_apply(checkpoint)) {
		t1replay_checkpoint_restore_is_pending = false;
		t1replay_fail();
		return false;
	}
#endif
	t1replay_input_checkpoint_import(&checkpoint->input);
#if T1REPLAY_EXACT_TRACE
	t1replay_exact_pellet_speed_raise_cycle = *pellet_speed_raise_cycle;
	if(t1replay_exact_trace_emit(
		T1REPLAY_EXACT_ROW_RESTORE_APPLIED, T1REPLAY_PROCESS_REIIDEN, 0,
		*pellet_speed_raise_cycle
	)) {
#if T1REPLAY_PIXEL_TRACE
		t1replay_pixel_probe_restored(
			t1replay_exact_snapshot.pacing.process_seq,
			t1replay_exact_snapshot.pacing.replay_sample_anchor,
			t1replay_exact_snapshot.pacing.replay_packet_anchor,
			t1replay_exact_snapshot.pacing.replay_input_anchor,
			t1replay_exact_snapshot.header.state_digest
		);
#endif
	}
#endif
	t1replay_checkpoint_restore_is_pending = false;
	return true;
}

#if T1REPLAY_WORLD_CAPTURE
static bool t1replay_checkpoint_world_snapshot_capture(
	t1replay_checkpoint_t far *checkpoint, int pellet_speed_raise_cycle,
	uint32_t sample_anchor, uint32_t packet_anchor, uint32_t input_anchor,
	uint32_t prefix_checksum, uint8_t process_seq
)
{
	t1replay_memclear(checkpoint, sizeof(*checkpoint));
	checkpoint->header.magic[0] = 'T';
	checkpoint->header.magic[1] = '1';
	checkpoint->header.magic[2] = 'C';
	checkpoint->header.magic[3] = 'K';
	checkpoint->header.magic[4] = 'P';
	checkpoint->header.magic[5] = '1';
	checkpoint->header.schema = T1REPLAY_CHECKPOINT_SCHEMA;
	checkpoint->header.header_size = T1REPLAY_CHECKPOINT_HEADER_SIZE;
	checkpoint->header.game_id = 1;
	checkpoint->header.group_count = T1REPLAY_CHECKPOINT_GROUP_COUNT;
	checkpoint->header.flags = T1REPLAY_CHECKPOINT_FLAG_CAPTURE_ONLY;
	checkpoint->header.total_size = T1REPLAY_CHECKPOINT_SIZE;
	checkpoint->header.replay_start_checksum = t1replay_header.start_checksum;
	t1replay_checkpoint_scenario_capture(&checkpoint->scenario);
	checkpoint->rng.frame_rand = frame_rand;
	checkpoint->rng.random_seed = static_cast<uint32_t>(random_seed);
	t1replay_input_checkpoint_export(&checkpoint->input);
	checkpoint->pacing.frame_since_start_of_binary = frame_since_start_of_binary;
	checkpoint->pacing.bomb_frame = bomb_frame;
	checkpoint->pacing.replay_sample_anchor = sample_anchor;
	checkpoint->pacing.replay_packet_anchor = packet_anchor;
	checkpoint->pacing.replay_input_anchor = input_anchor;
	checkpoint->pacing.replay_prefix_checksum = prefix_checksum;
	checkpoint->pacing.pellet_speed_raise_cycle = pellet_speed_raise_cycle;
	checkpoint->pacing.process_seq = process_seq;
	checkpoint->pacing.timer_initialized = timer_initialized;
	checkpoint->pacing.first_stage_in_scene = first_stage_in_scene;
	checkpoint->pacing.stage_wait_for_shot_to_begin =
		stage_wait_for_shot_to_begin;
	t1replay_timer_checkpoint_export(&checkpoint->pacing);
	t1replay_player_checkpoint_export(&checkpoint->player);
	t1replay_orb_checkpoint_export(&checkpoint->orb);
	if(!t1replay_stage_checkpoint_export(&checkpoint->stage)) {
		return false;
	}
	t1replay_items_checkpoint_export(&checkpoint->items);
	t1replay_pellets_checkpoint_export(&checkpoint->pellets);
	t1replay_shots_checkpoint_export(&checkpoint->shots);
	t1replay_missiles_checkpoint_export(&checkpoint->missiles);
	t1replay_lasers_checkpoint_export(&checkpoint->lasers);
	t1replay_particles_checkpoint_export(&checkpoint->particles);
	return true;
}

static uint32_t t1replay_checkpoint_world_digest(
	const t1replay_checkpoint_t far *checkpoint
)
{
	uint32_t digest = T1REPLAY_FNV1A_BASIS;

	#define checkpoint_world_digest(id, field) \
		digest = t1replay_checkpoint_group_digest( \
			digest, id, &checkpoint->field, sizeof(checkpoint->field) \
		)
	checkpoint_world_digest(T1RCGI_SCENARIO, scenario);
	checkpoint_world_digest(T1RCGI_RNG, rng);
	checkpoint_world_digest(T1RCGI_INPUT, input);
	checkpoint_world_digest(T1RCGI_PACING, pacing);
	checkpoint_world_digest(T1RCGI_PLAYER, player);
	checkpoint_world_digest(T1RCGI_ORB, orb);
	checkpoint_world_digest(T1RCGI_STAGE, stage);
	checkpoint_world_digest(T1RCGI_ITEMS, items);
	checkpoint_world_digest(T1RCGI_PELLETS, pellets);
	checkpoint_world_digest(T1RCGI_SHOTS, shots);
	checkpoint_world_digest(T1RCGI_MISSILES, missiles);
	checkpoint_world_digest(T1RCGI_LASERS, lasers);
	checkpoint_world_digest(T1RCGI_PARTICLES, particles);
	#undef checkpoint_world_digest
	return digest;
}

#endif

#if T1REPLAY_EXACT_TRACE
static bool t1replay_checkpoint_snapshot_capture(
	t1replay_checkpoint_t far *checkpoint, int pellet_speed_raise_cycle,
	uint32_t sample_anchor, uint32_t packet_anchor, uint32_t input_anchor,
	uint32_t prefix_checksum, uint8_t process_seq
)
{
	uint32_t digest = T1REPLAY_FNV1A_BASIS;

	if(!t1replay_checkpoint_world_snapshot_capture(
		checkpoint, pellet_speed_raise_cycle, sample_anchor, packet_anchor,
		input_anchor, prefix_checksum, process_seq
	)) {
		return false;
	}
	if(!t1replay_checkpoint_boss_capture(&checkpoint->boss)) {
		return false;
	}

	#define checkpoint_group_set_and_digest(id, field) { \
		t1replay_checkpoint_group_set( \
			&checkpoint->groups[id], id, offsetof(t1replay_checkpoint_t, field), \
			sizeof(checkpoint->field), &checkpoint->field \
		); \
		digest = t1replay_checkpoint_group_digest( \
			digest, id, &checkpoint->field, sizeof(checkpoint->field) \
		); \
	}
	checkpoint_group_set_and_digest(T1RCGI_SCENARIO, scenario);
	checkpoint_group_set_and_digest(T1RCGI_RNG, rng);
	checkpoint_group_set_and_digest(T1RCGI_INPUT, input);
	checkpoint_group_set_and_digest(T1RCGI_PACING, pacing);
	checkpoint_group_set_and_digest(T1RCGI_PLAYER, player);
	checkpoint_group_set_and_digest(T1RCGI_ORB, orb);
	checkpoint_group_set_and_digest(T1RCGI_STAGE, stage);
	checkpoint_group_set_and_digest(T1RCGI_ITEMS, items);
	checkpoint_group_set_and_digest(T1RCGI_PELLETS, pellets);
	checkpoint_group_set_and_digest(T1RCGI_SHOTS, shots);
	checkpoint_group_set_and_digest(T1RCGI_MISSILES, missiles);
	checkpoint_group_set_and_digest(T1RCGI_LASERS, lasers);
	checkpoint_group_set_and_digest(T1RCGI_PARTICLES, particles);
	checkpoint_group_set_and_digest(T1RCGI_BOSS, boss);
	#undef checkpoint_group_set_and_digest
	checkpoint->header.state_digest = digest;
	checkpoint->header.container_checksum = t1replay_checkpoint_checksum(
		checkpoint
	);
	return (
		t1replay_checkpoint_valid(checkpoint) &&
		t1replay_checkpoint_cross_groups_valid(checkpoint)
	);
}

void far t1replay_checkpoint_capture(int pellet_speed_raise_cycle)
{
#if T1KIK_TRACE
	if(t1replay_mode == T1RM_RECORD) {
		t1kik_pre_input(t1replay_res ? t1replay_res->process_seq : 0);
	}
#endif
#if T1SAR_TRACE
	if(t1replay_mode == T1RM_RECORD) {
		t1sar_pre_input(t1replay_res ? t1replay_res->process_seq : 0);
	}
#endif
#if T1REPLAY_EXACT_TRACE
	t1replay_exact_pellet_speed_raise_cycle = pellet_speed_raise_cycle;
	if(t1replay_mode == T1RM_PLAYBACK) {
		if(t1replay_exact_trace_emit(
			T1REPLAY_EXACT_ROW_PRE_INPUT, T1REPLAY_PROCESS_REIIDEN, 0,
			pellet_speed_raise_cycle
		)) {
#if T1REPLAY_PIXEL_TRACE
			t1replay_pixel_probe_pre_input(
				t1replay_exact_snapshot.pacing.process_seq,
				t1replay_exact_snapshot.pacing.replay_sample_anchor,
				t1replay_exact_snapshot.pacing.replay_packet_anchor,
				t1replay_exact_snapshot.pacing.replay_input_anchor,
				t1replay_exact_snapshot.header.state_digest
			);
#endif
		}
	}
#endif
#if T1REPLAY_KONNGARA_PHASE1_TRACE
	if(t1replay_mode == T1RM_RECORD) {
		t1replay_pixel_probe_konngara_phase1_pre_input(
			t1replay_res ? t1replay_res->process_seq : 0,
			t1replay_header.sample_count, t1replay_header.packet_count,
			t1replay_header.input_size, pellet_speed_raise_cycle,
			t1boss_konngara_phase1_resource_digest()
		);
	}
#endif
#if T1REPLAY_YUUGENMAGAN_FIRST_COMBAT_TRACE
	if(t1replay_mode == T1RM_RECORD) {
		t1ymx_pre_input();
	}
#endif

#if T1REPLAY_EXACT_TRACE
	if(
		(t1replay_mode != T1RM_RECORD) ||
		t1replay_checkpoint_capture_attempted ||
		!t1replay_res
	) {
		return;
	}
	// Capture exactly once. Release only flushes a presentation-eligible
	// snapshot at process end; this input seam remains BSS-only.
	t1replay_checkpoint_capture_attempted = true;
#if T1REPLAY_CHECKPOINT_EMIT
	// Exact restore starts at a packet boundary. Splitting an RLE run is a
	// private-capture format cost and never affects release packetization.
	if(!t1replay_pending_commit()) {
		return;
	}
#else
	if(t1replay_pending_valid) {
		return;
	}
#endif
	if(!t1replay_checkpoint_snapshot_capture(
		&t1replay_checkpoint, pellet_speed_raise_cycle,
		t1replay_header.sample_count, t1replay_header.packet_count,
		t1replay_header.input_size, t1replay_payload_checksum,
		t1replay_res->process_seq
	)) {
		return;
	}
#if T1REPLAY_EXACT_TRACE
	t1replay_exact_trace_emit(
		T1REPLAY_EXACT_ROW_PRE_INPUT, T1REPLAY_PROCESS_REIIDEN, 0,
		pellet_speed_raise_cycle
	);
#endif
#endif
}

static void t1replay_exact_trace_path(char *fn)
{
	fn[0] = 'T'; fn[1] = '1'; fn[2] = 'E'; fn[3] = 'X';
	fn[4] = 'A'; fn[5] = 'C'; fn[6] = 'T'; fn[7] = '.';
	fn[8] = 'B'; fn[9] = 'I'; fn[10] = 'N'; fn[11] = '\0';
}

static bool t1replay_exact_trace_header_valid(
	const t1replay_exact_trace_header_t far *header
)
{
	return (
		(header->magic[0] == 'T') && (header->magic[1] == '1') &&
		(header->magic[2] == 'E') && (header->magic[3] == 'R') &&
		(header->magic[4] == '1') && (header->magic[5] == '\0') &&
		(header->magic[6] == '\0') && (header->magic[7] == '\0') &&
		(header->version == T1REPLAY_EXACT_TRACE_VERSION) &&
		(header->header_size == T1REPLAY_EXACT_TRACE_HEADER_SIZE) &&
		(header->row_size == T1REPLAY_EXACT_TRACE_ROW_SIZE) &&
		(header->group_count == T1REPLAY_CHECKPOINT_GROUP_COUNT) &&
		(header->reserved == 0)
	);
}

static bool t1replay_exact_trace_prepare(void)
{
	t1replay_exact_trace_header_t header;
	char fn[12];
	uint32_t size;
	int fd;

	if(t1replay_exact_trace_ready) {
		return true;
	}
	if(t1replay_exact_trace_failed) {
		return false;
	}
	t1replay_exact_trace_path(fn);
	fd = t1replay_dos_open(fn, T1REPLAY_DOS_ACCESS_READ);
	if(fd >= 0) {
		if(
			!t1replay_dos_size(fd, &size) ||
			(size < T1REPLAY_EXACT_TRACE_HEADER_SIZE) ||
			(((size - T1REPLAY_EXACT_TRACE_HEADER_SIZE) %
			  T1REPLAY_EXACT_TRACE_ROW_SIZE) != 0) ||
			!t1replay_dos_seek(fd, 0) ||
			(t1replay_dos_read(fd, &header, sizeof(header)) != sizeof(header)) ||
			!t1replay_exact_trace_header_valid(&header)
		) {
			t1replay_dos_close(fd);
			t1replay_exact_trace_failed = true;
			return false;
		}
		t1replay_dos_close(fd);
		t1replay_exact_trace_ready = true;
		return true;
	}

	t1replay_memclear(&header, sizeof(header));
	header.magic[0] = 'T'; header.magic[1] = '1';
	header.magic[2] = 'E'; header.magic[3] = 'R';
	header.magic[4] = '1';
	header.version = T1REPLAY_EXACT_TRACE_VERSION;
	header.header_size = T1REPLAY_EXACT_TRACE_HEADER_SIZE;
	header.row_size = T1REPLAY_EXACT_TRACE_ROW_SIZE;
	header.group_count = T1REPLAY_CHECKPOINT_GROUP_COUNT;
	fd = t1replay_dos_create(fn);
	if(fd < 0) {
		t1replay_exact_trace_failed = true;
		return false;
	}
	if(t1replay_dos_write(fd, &header, sizeof(header)) != sizeof(header)) {
		t1replay_dos_close(fd);
		t1replay_exact_trace_failed = true;
		return false;
	}
	t1replay_dos_close(fd);
	t1replay_exact_trace_ready = true;
	return true;
}

static bool t1replay_exact_trace_flush(void)
{
	char fn[12];
	uint32_t size;
	unsigned len;
	int fd;
	bool ok;

	if(t1replay_exact_trace_failed) {
		return false;
	}
	if(t1replay_exact_trace_buffer_count == 0) {
		return true;
	}
	if(!t1replay_exact_trace_prepare()) {
		return false;
	}
	len = static_cast<unsigned>(
		t1replay_exact_trace_buffer_count * sizeof(t1replay_exact_trace_buffer[0])
	);
	t1replay_exact_trace_path(fn);
	fd = t1replay_dos_open(fn, T1REPLAY_DOS_ACCESS_RW);
	if(fd < 0) {
		t1replay_exact_trace_failed = true;
		return false;
	}
	ok = (
		t1replay_dos_size(fd, &size) &&
		(size >= T1REPLAY_EXACT_TRACE_HEADER_SIZE) &&
		(((size - T1REPLAY_EXACT_TRACE_HEADER_SIZE) %
		  T1REPLAY_EXACT_TRACE_ROW_SIZE) == 0) &&
		t1replay_dos_seek(fd, size) &&
		(t1replay_dos_write(fd, t1replay_exact_trace_buffer, len) == len)
	);
	t1replay_dos_close(fd);
	if(!ok) {
		t1replay_exact_trace_failed = true;
	}
	if(ok) {
		t1replay_exact_trace_buffer_count = 0;
	}
	return ok;
}

static bool t1replay_exact_trace_row_write(
	const t1replay_exact_trace_row_t far *row
)
{
	if(
		(t1replay_exact_trace_buffer_count >=
		 T1REPLAY_EXACT_TRACE_BUFFER_ROWS) &&
		!t1replay_exact_trace_flush()
	) {
		return false;
	}
	t1replay_exact_trace_buffer[t1replay_exact_trace_buffer_count] = *row;
	t1replay_exact_trace_buffer_count++;
	if(row->row_kind == T1REPLAY_EXACT_ROW_TERMINAL) {
		return t1replay_exact_trace_flush();
	}
	return true;
}

static bool t1replay_exact_trace_emit(
	uint8_t kind, uint8_t target_process, uint8_t terminal_reason,
	int pellet_speed_raise_cycle
)
{
	t1replay_exact_trace_row_t row;

	if(!t1replay_exact_trace_row_capture(
		&row, kind, target_process, terminal_reason,
		pellet_speed_raise_cycle
	)) {
		return false;
	}
	if(
		(t1replay_exact_last_kind == kind) &&
		(t1replay_exact_last_sample == row.sample_cursor)
	) {
		return true;
	}
	if(!t1replay_exact_trace_row_write(&row)) {
		t1replay_exact_trace_failed = true;
		return false;
	}
	t1replay_exact_last_kind = kind;
	t1replay_exact_last_sample = row.sample_cursor;
	return true;
}

static bool t1replay_exact_trace_row_capture(
	t1replay_exact_trace_row_t far *row, uint8_t kind,
	uint8_t target_process, uint8_t terminal_reason,
	int pellet_speed_raise_cycle
)
{
	t1replay_checkpoint_t far *snapshot = &t1replay_exact_snapshot;
	uint32_t sample_anchor;
	uint32_t packet_anchor;
	uint32_t input_anchor;
	uint8_t i;

	if(!t1replay_res || t1replay_exact_trace_failed) {
		return false;
	}
	if(t1replay_mode == T1RM_RECORD) {
		sample_anchor = t1replay_header.sample_count;
		packet_anchor = t1replay_header.packet_count;
		input_anchor = t1replay_header.input_size;
	} else {
		sample_anchor = t1replay_sample_cursor;
		packet_anchor = t1replay_packet_cursor;
		input_anchor = t1replay_payload_written;
	}
	if(!t1replay_checkpoint_snapshot_capture(
		snapshot, pellet_speed_raise_cycle, sample_anchor, packet_anchor,
		input_anchor, t1replay_payload_checksum, t1replay_res->process_seq
	)) {
		t1replay_exact_trace_failed = true;
		return false;
	}

	t1replay_memclear(row, sizeof(*row));
	row->row_kind = kind;
	row->process_seq = t1replay_res->process_seq;
	row->source_process = t1replay_res->source_process;
	row->target_process = target_process;
	row->sample_cursor = sample_anchor;
	row->packet_cursor = packet_anchor;
	row->input_cursor = input_anchor;
	row->stage_id = snapshot->scenario.resident_stage_id;
	row->route = snapshot->scenario.game_route;
	row->boss_id = snapshot->boss.boss_id;
	row->frame_rand = snapshot->rng.frame_rand;
	row->score = static_cast<uint32_t>(snapshot->scenario.game_score);
	row->rem_lives = snapshot->scenario.game_rem_lives;
	row->rem_bombs = snapshot->scenario.game_rem_bombs;
	row->pellet_speed_raise_cycle = static_cast<int16_t>(
		pellet_speed_raise_cycle
	);
	row->terminal_reason = terminal_reason;
	for(i = 0; i < T1REPLAY_CHECKPOINT_GROUP_COUNT; i++) {
		row->group_digest[i] = snapshot->groups[i].checksum;
	}
	return true;
}

void far t1replay_exact_terminal_capture(uint8_t end_reason)
{
	if(t1replay_mode == T1RM_DISABLED) {
		return;
	}
	t1replay_exact_terminal_pending = t1replay_exact_trace_row_capture(
		&t1replay_exact_terminal_row, T1REPLAY_EXACT_ROW_TERMINAL,
		T1REPLAY_PROCESS_NONE, end_reason,
		t1replay_exact_pellet_speed_raise_cycle
	);
}
#else
void far t1replay_checkpoint_capture(int pellet_speed_raise_cycle)
{
	t1replay_checkpoint_t far *checkpoint = &t1replay_checkpoint;
	uint32_t digest = T1REPLAY_FNV1A_BASIS;

#if T1KIK_TRACE
	if(t1replay_mode == T1RM_RECORD) {
		t1kik_pre_input(t1replay_res ? t1replay_res->process_seq : 0);
	}
#endif
#if T1SAR_TRACE
	if(t1replay_mode == T1RM_RECORD) {
		t1sar_pre_input(t1replay_res ? t1replay_res->process_seq : 0);
	}
#endif

#if T1REPLAY_KONNGARA_PHASE1_TRACE
	if(t1replay_mode == T1RM_RECORD) {
		t1replay_pixel_probe_konngara_phase1_pre_input(
			t1replay_res ? t1replay_res->process_seq : 0,
			t1replay_header.sample_count, t1replay_header.packet_count,
			t1replay_header.input_size, pellet_speed_raise_cycle,
			t1boss_konngara_phase1_resource_digest()
		);
	}
#endif
#if T1REPLAY_YUUGENMAGAN_FIRST_COMBAT_TRACE
	if(t1replay_mode == T1RM_RECORD) {
		t1ymx_pre_input();
	}
#endif
	if(
		(t1replay_mode != T1RM_RECORD) ||
		t1replay_checkpoint_capture_attempted ||
		!t1replay_res
	) {
		return;
	}
	// Capture exactly once. Release only flushes a presentation-eligible
	// snapshot at process end; this input seam remains BSS-only.
	t1replay_checkpoint_capture_attempted = true;
#if T1REPLAY_CHECKPOINT_EMIT
	// Exact restore starts at a packet boundary. Splitting an RLE run is a
	// private-capture format cost and never affects release packetization.
	if(!t1replay_pending_commit()) {
		return;
	}
#else
	if(t1replay_pending_valid) {
		return;
	}
#endif
	t1replay_memclear(checkpoint, sizeof(*checkpoint));
	checkpoint->header.magic[0] = 'T';
	checkpoint->header.magic[1] = '1';
	checkpoint->header.magic[2] = 'C';
	checkpoint->header.magic[3] = 'K';
	checkpoint->header.magic[4] = 'P';
	checkpoint->header.magic[5] = '1';
	checkpoint->header.schema = T1REPLAY_CHECKPOINT_SCHEMA;
	checkpoint->header.header_size = T1REPLAY_CHECKPOINT_HEADER_SIZE;
	checkpoint->header.game_id = 1;
	checkpoint->header.group_count = T1REPLAY_CHECKPOINT_GROUP_COUNT;
	checkpoint->header.flags = T1REPLAY_CHECKPOINT_FLAG_CAPTURE_ONLY;
	checkpoint->header.total_size = T1REPLAY_CHECKPOINT_SIZE;
	checkpoint->header.replay_start_checksum = t1replay_header.start_checksum;
	t1replay_checkpoint_scenario_capture(&checkpoint->scenario);
	checkpoint->rng.frame_rand = frame_rand;
	checkpoint->rng.random_seed = static_cast<uint32_t>(random_seed);
	t1replay_input_checkpoint_export(&checkpoint->input);
	checkpoint->pacing.frame_since_start_of_binary = frame_since_start_of_binary;
	checkpoint->pacing.bomb_frame = bomb_frame;
	checkpoint->pacing.replay_sample_anchor = t1replay_header.sample_count;
	checkpoint->pacing.replay_packet_anchor = t1replay_header.packet_count;
	checkpoint->pacing.replay_input_anchor = t1replay_header.input_size;
	checkpoint->pacing.replay_prefix_checksum = t1replay_payload_checksum;
	checkpoint->pacing.pellet_speed_raise_cycle = pellet_speed_raise_cycle;
	checkpoint->pacing.process_seq = t1replay_res->process_seq;
	checkpoint->pacing.timer_initialized = timer_initialized;
	checkpoint->pacing.first_stage_in_scene = first_stage_in_scene;
	checkpoint->pacing.stage_wait_for_shot_to_begin =
		stage_wait_for_shot_to_begin;
	t1replay_timer_checkpoint_export(&checkpoint->pacing);
	t1replay_player_checkpoint_export(&checkpoint->player);
	t1replay_orb_checkpoint_export(&checkpoint->orb);
	if(!t1replay_stage_checkpoint_export(&checkpoint->stage)) {
		return;
	}
	t1replay_items_checkpoint_export(&checkpoint->items);
	t1replay_pellets_checkpoint_export(&checkpoint->pellets);
	t1replay_shots_checkpoint_export(&checkpoint->shots);
	t1replay_missiles_checkpoint_export(&checkpoint->missiles);
	t1replay_lasers_checkpoint_export(&checkpoint->lasers);
	t1replay_particles_checkpoint_export(&checkpoint->particles);
	if(!t1replay_checkpoint_boss_capture(&checkpoint->boss)) {
		return;
	}

	#define checkpoint_group_set_and_digest(id, field) { \
		t1replay_checkpoint_group_set( \
			&checkpoint->groups[id], id, offsetof(t1replay_checkpoint_t, field), \
			sizeof(checkpoint->field), &checkpoint->field \
		); \
		digest = t1replay_checkpoint_group_digest( \
			digest, id, &checkpoint->field, sizeof(checkpoint->field) \
		); \
	}
	checkpoint_group_set_and_digest(T1RCGI_SCENARIO, scenario);
	checkpoint_group_set_and_digest(T1RCGI_RNG, rng);
	checkpoint_group_set_and_digest(T1RCGI_INPUT, input);
	checkpoint_group_set_and_digest(T1RCGI_PACING, pacing);
	checkpoint_group_set_and_digest(T1RCGI_PLAYER, player);
	checkpoint_group_set_and_digest(T1RCGI_ORB, orb);
	checkpoint_group_set_and_digest(T1RCGI_STAGE, stage);
	checkpoint_group_set_and_digest(T1RCGI_ITEMS, items);
	checkpoint_group_set_and_digest(T1RCGI_PELLETS, pellets);
	checkpoint_group_set_and_digest(T1RCGI_SHOTS, shots);
	checkpoint_group_set_and_digest(T1RCGI_MISSILES, missiles);
	checkpoint_group_set_and_digest(T1RCGI_LASERS, lasers);
	checkpoint_group_set_and_digest(T1RCGI_PARTICLES, particles);
	checkpoint_group_set_and_digest(T1RCGI_BOSS, boss);
	#undef checkpoint_group_set_and_digest
	checkpoint->header.state_digest = digest;
	checkpoint->header.container_checksum = t1replay_checkpoint_checksum(
		checkpoint
	);
}
#endif

#if T1REPLAY_KONNGARA_PHASE1_TRACE
bool16 far t1replay_pixel_probe_world_capture(
	t1replay_pixel_world_t far *world, int pellet_speed_raise_cycle
)
{
	t1replay_checkpoint_t far *snapshot = &t1replay_exact_snapshot;
	uint16_t i;

	if(
		!world || !t1replay_res ||
		!t1replay_checkpoint_world_snapshot_capture(
			snapshot, pellet_speed_raise_cycle,
			t1replay_header.sample_count, t1replay_header.packet_count,
			t1replay_header.input_size, t1replay_payload_checksum,
			t1replay_res->process_seq
		)
	) {
		return false;
	}
	t1replay_memclear(world, sizeof(*world));
	world->semantic_digest = t1replay_checkpoint_world_digest(snapshot);
	world->cards = snapshot->stage.cards_count;
	world->obstacles = snapshot->stage.obstacles_count;
	for(i = 0; i < T1REPLAY_CHECKPOINT_ITEM_BOMB_COUNT; i++) {
		if(snapshot->items.bombs[i].flag != 0) {
			world->bomb_items++;
		}
	}
	for(i = 0; i < T1REPLAY_CHECKPOINT_ITEM_POINT_COUNT; i++) {
		if(snapshot->items.points[i].flag != 0) {
			world->point_items++;
		}
	}
	for(i = 0; i < T1REPLAY_CHECKPOINT_PELLET_COUNT; i++) {
		const t1replay_checkpoint_pellet_t far *pellet =
			&snapshot->pellets.pellets[i];
		if(pellet->moving || pellet->cloud_frame || pellet->decay_frame) {
			world->pellets++;
		}
	}
	for(i = 0; i < T1REPLAY_CHECKPOINT_SHOT_COUNT; i++) {
		const t1replay_checkpoint_shot_t far *shot = &snapshot->shots.shots[i];
		if(shot->moving || shot->decay_frame) {
			world->shots++;
		}
	}
	for(i = 0; i < T1REPLAY_CHECKPOINT_MISSILE_COUNT; i++) {
		if(snapshot->missiles.missiles[i].flag != 0) {
			world->missiles++;
		}
	}
	for(i = 0; i < T1REPLAY_CHECKPOINT_LASER_COUNT; i++) {
		const t1replay_checkpoint_laser_t far *laser =
			&snapshot->lasers.lasers[i];
		if(laser->alive || laser->damaging || laser->put_flag) {
			world->lasers++;
		}
	}
	for(i = 0; i < T1REPLAY_CHECKPOINT_PARTICLE_COUNT; i++) {
		if(snapshot->particles.particles[i].alive) {
			world->particles++;
		}
	}
	return true;
}
#endif

void far t1replay_frame_io(void)
{
	uint8_t i;
	int group;

	if(t1replay_mode == T1RM_DISABLED) {
		return;
	}
	// Escape is deliberately physical and checked before packet decoding. It
	// cancels the current playback transaction without consuming, recording, or
	// synthesizing an input sample.
	if(t1replay_playback_abort_requested()) {
		t1replay_abort_to_op();
		return;
	}
	if(t1replay_mode == T1RM_RECORD) {
		for(i = 0; i < T1REPLAY_INPUT_GROUP_COUNT; i++) {
			group = t1replay_group_number(i);
			t1replay_keys[i] = static_cast<uint8_t>(
				(key_sense(group) | key_sense(group)) & t1replay_group_mask(i)
			);
		}
		if(!t1replay_record_sample() ||
			!t1rpg_sample(&t1replay_res->guard)) {
			t1replay_fail();
			return;
		}
	} else if(!t1replay_playback_sample()) {
		t1replay_fail_and_abort_if_playback();
	} else if(t1replay_gameplay_input_armed) {
		// input_sense(false) can recurse through the stock debug surface. Only
		// its first pass is paired with the normal orbital gameplay wait.
		t1replay_gameplay_input_armed = false;
		if(peekb(0, KEYGROUP_5) & K5_Z) {
			t1replay_fast_forward_phase++;
			if(t1replay_fast_forward_phase >= T1REPLAY_FAST_FORWARD_RATE) {
				t1replay_fast_forward_phase = 0;
			} else {
				t1replay_gameplay_wait_skip_pending = true;
			}
		} else {
			t1replay_fast_forward_phase = 0;
		}
	}
}

void far t1replay_gameplay_input_begin(void)
{
	// A previous frame can terminate before it reaches the orbital wait. Do not
	// allow that private pacing state to cross into this frame or a transition.
	t1replay_gameplay_input_armed = true;
	t1replay_gameplay_wait_skip_pending = false;
}

void far t1replay_gameplay_input_end(void)
{
	t1replay_gameplay_input_armed = false;
#if T1KIK_TRACE
	if(t1replay_mode == T1RM_RECORD) {
		t1kik_post_input(t1replay_res ? t1replay_res->process_seq : 0);
	}
#endif
	if(t1replay_mode == T1RM_RECORD) {
		// The stock life-loss path exposes no replay hook. Its first gameplay
		// input seam is nevertheless source-identifiable by the native miss
		// invincibility value, so loading/animation time is never telemetry.
		if(player_invincibility_time == PLAYER_MISS_INVINCIBILITY_FRAMES) {
			t1replay_timing_first_frame = true;
		}
		t1replay_timing_frame_armed = true;
	}
}

bool16 far t1replay_gameplay_wait_skip(void)
{
	bool16 skip = t1replay_gameplay_wait_skip_pending;

	// This wrapper already sits directly before the native frame_delay(1).
	// Observe its counter without resetting, waiting, or changing the stock
	// pacing path. Exactly one elapsed VSync is on time; only larger values are
	// classified as slow. Pause-opening frames are excluded entirely.
	if((t1replay_mode == T1RM_RECORD) && t1replay_timing_frame_armed) {
		t1replay_timing_frame_armed = false;
		if(paused) {
			// Pause polling and playfield restoration are outside gameplay
			// telemetry. Keep this marker for the first resumed frame so none
			// of that deferred work is classified there either.
			t1replay_timing_first_frame = true;
		} else if(t1replay_timing_first_frame) {
			t1replay_timing_first_frame = false;
		} else {
			if(t1replay_header.timed_frames == 0xFFFFFFFFUL) {
				t1replay_fail();
			} else {
				t1replay_header.timed_frames++;
				if(z_vsync_Count1 > 1) {
					t1replay_header.slow_frames++;
				}
			}
		}
	}

	t1replay_gameplay_wait_skip_pending = false;
	return skip;
}

int far t1replay_key_sense(int keygroup)
{
	if(t1replay_abort_pending) {
		return 0;
	}
	if(t1replay_mode == T1RM_DISABLED) {
		return key_sense(keygroup);
	}
	switch(keygroup) {
	case 0: return t1replay_keys[T1RIG_0];
	case 3: return t1replay_keys[T1RIG_3];
	case 5: return t1replay_keys[T1RIG_5];
	case 6: return t1replay_keys[T1RIG_6];
	case 7: return t1replay_keys[T1RIG_7];
	case 8: return t1replay_keys[T1RIG_8];
	case 9: return t1replay_keys[T1RIG_9];
	}
	return key_sense(keygroup);
}

static bool t1replay_process_control(uint8_t control, uint8_t value)
{
	if(t1replay_mode == T1RM_DISABLED) {
		return false;
	}
	if(t1replay_mode == T1RM_RECORD) {
		if(!t1replay_pending_commit() ||
			!t1replay_control_commit(control, value) ||
			!t1replay_buffer_flush()) {
			t1replay_fail();
			return false;
		}
	} else if(!t1replay_control_playback(control, value)) {
		t1replay_fail_and_abort_if_playback();
		return false;
	}
	return true;
}

static bool t1replay_stage_complete_apply(uint8_t stage_id, score_t stage_score)
{
	t1replay_summary_t far *summary = &t1replay_header.summary;
	t1replay_stage_summary_t far *split;
	uint8_t index;

	if(t1replay_mode == T1RM_RECORD) {
		index = summary->split_count;
		if(
			(index >= STAGE_COUNT) ||
			(stage_id != (t1replay_header.start.stage_id + index))
		) {
			return false;
		}
		split = &summary->splits[index];
		split->score = stage_score;
		split->stage_id = stage_id;
		split->flags = T1REPLAY_STAGE_FLAGS_KNOWN;
		summary->split_count++;
		return t1replay_summary_valid(
			summary, &t1replay_header.start, false, 0
		);
	}
	if(
		(stage_id < t1replay_header.start.stage_id) ||
		((index = static_cast<uint8_t>(
			stage_id - t1replay_header.start.stage_id
		)) >= summary->split_count)
	) {
		return false;
	}
	split = &summary->splits[index];
	return (
		(split->stage_id == stage_id) &&
		(split->flags == T1REPLAY_STAGE_FLAGS_KNOWN) &&
		(split->score == stage_score)
	);
}

void far t1replay_stage_complete(uint8_t stage_id, score_t stage_score)
{
	t1replay_fast_forward_boundary_reset();
	if(t1replay_mode == T1RM_DISABLED) {
		return;
	}
	if(!t1replay_stage_complete_apply(stage_id, stage_score)) {
		t1replay_fail_and_abort_if_playback();
	}
}

static bool t1replay_nonclear_summary_finalize(uint8_t end_reason)
{
	t1replay_summary_t far *summary = &t1replay_header.summary;
	t1replay_stage_summary_t far *split;
	uint8_t stage_id = static_cast<uint8_t>(resident->stage_id);

	if(t1replay_mode == T1RM_RECORD) {
		if(
			(summary->split_count >= STAGE_COUNT) ||
			(stage_id !=
			 (t1replay_header.start.stage_id + summary->split_count))
		) {
			return false;
		}
		split = &summary->splits[summary->split_count];
		split->score = score;
		split->stage_id = stage_id;
		split->flags = T1REPLAY_STAGE_FLAG_REACHED;
		summary->split_count++;
		summary->final_score = score;
		summary->final_stage_id = stage_id;
		summary->terminal_reason = end_reason;
		return t1replay_summary_valid(
			summary, &t1replay_header.start, true, end_reason
		);
	}
	if(
		(summary->split_count == 0) ||
		(summary->final_stage_id != stage_id) ||
		(summary->final_score != score)
	) {
		return false;
	}
	return t1replay_summary_valid(
		summary, &t1replay_header.start, true, end_reason
	);
}

bool16 far t1replay_process_handoff(uint8_t target_process)
{
	t1replay_fast_forward_boundary_reset();
	if(
		(target_process != T1REPLAY_PROCESS_REIIDEN) &&
		(target_process != T1REPLAY_PROCESS_FUUIN)
	) {
		return false;
	}
	if(t1replay_mode == T1RM_DISABLED) {
		// continue_menu() reaches this path only for Yes. The first credit has
		// already terminated, so remove its pending transaction before native
		// unrecorded execution replaces this process.
		if(
			t1replay_terminal_pending &&
			(target_process == T1REPLAY_PROCESS_REIIDEN)
		) {
			t1replay_pending_files_discard();
			t1replay_terminal_pending = false;
		}
		return false;
	}
	if((t1replay_mode == T1RM_RECORD) &&
		!t1replay_guard_checkpoint(
			&t1replay_res->guard, T1SAE_HANDOFF
		)) {
		t1replay_fail();
		return false;
	}
	if(
		!t1replay_process_control(T1REPLAY_CONTROL_PROCESS_END, target_process)
	) {
		return false;
	}
	t1replay_res->process_seq++;
	t1replay_res->source_process = T1REPLAY_PROCESS_REIIDEN;
	t1replay_res->target_process = target_process;
	t1replay_res->handoff_checksum = (
		(target_process == T1REPLAY_PROCESS_FUUIN) ?
		t1replay_fuuin_handoff_checksum() : 0
	);
	t1replay_res_store();
#if T1REPLAY_CHECKPOINT_EMIT
	t1replay_checkpoint_flush_if_enabled();
#endif
#if T1REPLAY_EXACT_TRACE
	if(!t1replay_exact_trace_flush()) {
		return false;
	}
#endif
	return true;
}

static void t1replay_terminal_request_pending(
	t1replay_save_request_source_t source
)
{
	if(!t1replay_terminal_pending) {
		return;
	}
	t1replay_terminal_pending = false;
	if(
		(t1replay_header.status != T1REPLAY_STATUS_FINALIZED) ||
		((t1replay_header.end_reason != T1REPLAY_END_MENU) &&
		 (t1replay_header.end_reason != T1REPLAY_END_GAME_OVER)) ||
		(t1replay_header.header_checksum == 0) ||
		!t1replay_save_request_write(source)
	) {
		t1replay_pending_files_discard();
	}
}

static void t1replay_pause_terminal_apply(bool playback)
{
	t1replay_pause_action_t action = t1replay_pause_action;

	t1replay_pause_action = T1RPA_RESUME;
	if(playback || (action == T1RPA_RESUME)) {
		return;
	}
	if(action == T1RPA_SAVE_EXIT) {
		t1replay_terminal_request_pending(T1RSRS_PAUSE);
		return;
	}
	t1replay_pending_files_discard();
	if(action == T1RPA_RESTART) {
		t1replay_restart_request_write();
	}
}

void far t1replay_terminal(uint8_t end_reason)
{
	bool playback = (t1replay_mode == T1RM_PLAYBACK);

	t1replay_fast_forward_boundary_reset();

	if(t1replay_mode == T1RM_DISABLED) {
		// Esc leaves Continue through a separate stock branch that has no BGM
		// stop call to wrap. Its existing late terminal call reaches this path.
		t1replay_terminal_request_pending(T1RSRS_POSTGAME);
		return;
	}
	if(
		((end_reason != T1REPLAY_END_MENU) &&
		 (end_reason != T1REPLAY_END_GAME_OVER)) ||
		!t1replay_nonclear_summary_finalize(end_reason)
	) {
		t1replay_fail_and_abort_if_playback();
		return;
	}
	if((t1replay_mode == T1RM_RECORD) &&
		!t1replay_guard_checkpoint(
			&t1replay_res->guard, T1SAE_FINALIZE
		)) {
		t1replay_fail();
		return;
	}
	if(!t1replay_process_control(T1REPLAY_CONTROL_TERMINAL, end_reason)) {
		return;
	}
	if(t1replay_mode == T1RM_RECORD) {
		t1replay_header.status = T1REPLAY_STATUS_FINALIZED;
		t1replay_header.end_reason = end_reason;
		if(!t1replay_header_write(false)) {
			t1replay_fail();
			return;
		}
		t1replay_terminal_pending = t1replay_slot_is_pending(
			t1replay_res->slot
		);
#if T1REPLAY_CHECKPOINT_EMIT
	t1replay_checkpoint_flush_if_enabled();
#endif
	} else if(
		(t1replay_packet_cursor != t1replay_header.packet_count) ||
		(t1replay_sample_cursor != t1replay_header.sample_count) ||
		(t1replay_payload_written != t1replay_header.input_size) ||
		(t1replay_payload_checksum != t1replay_header.payload_checksum)
	) {
		t1replay_fail_and_abort_if_playback();
		return;
	}
#if T1REPLAY_EXACT_TRACE
	if(
		!t1replay_exact_terminal_pending ||
		(t1replay_exact_terminal_row.terminal_reason != end_reason)
	) {
		t1replay_fail_and_abort_if_playback();
		return;
	}
	t1replay_exact_terminal_row.sample_cursor = t1replay_sample_cursor;
	t1replay_exact_terminal_row.packet_cursor = t1replay_packet_cursor;
	t1replay_exact_terminal_row.input_cursor = t1replay_payload_written;
	if(!t1replay_exact_trace_row_write(&t1replay_exact_terminal_row)) {
		t1replay_exact_trace_failed = true;
		t1replay_fail_and_abort_if_playback();
		return;
	}
	t1replay_exact_terminal_pending = false;
#endif
	if(!playback) {
		t1replay_guard_end(&t1replay_res->guard);
	}
	t1replay_res_clear();
	t1replay_mode = T1RM_DISABLED;
	t1replay_pause_terminal_apply(playback);
	if(playback) {
		t1replay_abort_to_op();
	}
}

void far t1replay_gameover_regist_menu(
	score_t score, int16_t stage_num,
	sshiftjis_t route[SCOREDAT_ROUTE_LEN + 1]
)
{
#if T1REPLAY_EXACT_TRACE
	t1replay_exact_terminal_capture(T1REPLAY_END_GAME_OVER);
#endif
	t1replay_terminal(T1REPLAY_END_GAME_OVER);
	regist_menu(score, stage_num, route);
}

void far t1replay_terminal_save_request(void)
{
	t1replay_terminal_request_pending(T1RSRS_POSTGAME);
	mdrv2_bgm_stop();
}

bool16 far t1replay_active(void)
{
	return (t1replay_mode != T1RM_DISABLED);
}

bool16 far t1replay_playback_active(void)
{
	return (t1replay_mode == T1RM_PLAYBACK);
}

bool16 far t1replay_pause_save_available(void)
{
	// Playback is a read-only transaction. Leaving it still reaches OP through
	// the ordinary terminal path, but it must not advertise a save action that
	// cannot create a numbered replay.
	return (
		(t1replay_mode == T1RM_RECORD) && t1replay_res &&
		!t1rpg_blocked(&t1replay_res->guard)
	);
}

bool16 far t1replay_pause_save_refresh(void)
{
	bool16 was_available = t1replay_pause_save_available();

	if(was_available) {
		t1replay_guard_pause_check();
	}
	return (was_available && !t1replay_pause_save_available());
}

void far t1replay_guard_pause_check(void)
{
	if((t1replay_mode == T1RM_RECORD) && t1replay_res &&
		!t1replay_guard_checkpoint(&t1replay_res->guard, T1SAE_PAUSE)) {
		t1replay_fail();
	}
}

bool16 far t1replay_pause_restart_available(void)
{
	t1replay_restart_state_t far *state;
	char id[sizeof(T1REPLAY_RESTART_RES_ID)];

	if(t1replay_mode == T1RM_PLAYBACK) {
		return false;
	}
	t1replay_restart_res_id_init(id);
	state = ResData<t1replay_restart_state_t>::exist(id);
	return t1replay_restart_state_valid(state);
}

bool16 far t1replay_playback_abort_requested(void)
{
	// This is intentionally outside the canonical input stream. A physical
	// cancellation must not decode one more recorded input sample.
	return (
		(t1replay_mode == T1RM_PLAYBACK) &&
		((peekb(0, KEYGROUP_0) & K0_ESC) != 0)
	);
}

void far t1replay_pause_action_set(t1replay_pause_action_t action)
{
	if((action < T1RPA_RESUME) || (action > T1RPA_DISCARD_EXIT)) {
		return;
	}
	if((action == T1RPA_SAVE_EXIT) && !t1replay_pause_save_available()) {
		return;
	}
	if((action == T1RPA_RESTART) && !t1replay_pause_restart_available()) {
		return;
	}
	t1replay_pause_action = action;
}

bool16 far t1replay_abort_requested(void)
{
	return t1replay_abort_pending;
}

#pragma codeseg
