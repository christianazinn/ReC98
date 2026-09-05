#ifdef T2PD
#pragma codeseg T2PDIAG_TEXT PATCH
#endif

#include "th02/practice_diag.hpp"

#if T2REPLAY_PRACTICE_DIAGNOSTICS

#include "libs/master.lib/master.hpp"

static const char T2PRACT_DIAG_FN[] = "T2PRACT.BIN";
static const char T2PRACT_DIAG_NO_SOUND_FN[] = "T2NSND.CFG";
static t2practice_diag_record_t t2practice_diag_current;
static bool t2practice_diag_apply_active;

#if defined(T2PRACT_DIAG_OP)
#define T2PRACT_DIAG_ACCESS_READ T2OP_DOS_ACCESS_READ
#define T2PRACT_DIAG_ACCESS_RW T2OP_DOS_ACCESS_RW
#define T2PRACT_DIAG_FP_SEG T2OP_FP_SEG
#define T2PRACT_DIAG_FP_OFF T2OP_FP_OFF
#define t2practice_diag_dos_open t2op_dos_open
#define t2practice_diag_dos_close t2op_dos_close
#define t2practice_diag_dos_flush t2op_dos_flush
#define t2practice_diag_dos_seek t2op_dos_seek
#define t2practice_diag_dos_size t2op_dos_size
#define t2practice_diag_dos_write t2op_dos_write
#define t2practice_diag_dos_delete t2op_file_delete
#elif defined(T2PRACT_DIAG_MAIN)
#define T2PRACT_DIAG_ACCESS_READ T2REPLAY_DOS_ACCESS_READ
#define T2PRACT_DIAG_ACCESS_RW T2REPLAY_DOS_ACCESS_RW
#define T2PRACT_DIAG_FP_SEG T2REPLAY_FP_SEG
#define T2PRACT_DIAG_FP_OFF T2REPLAY_FP_OFF
#define t2practice_diag_dos_open t2replay_dos_open
#define t2practice_diag_dos_close t2replay_dos_close
#define t2practice_diag_dos_flush t2replay_dos_flush
#define t2practice_diag_dos_seek t2replay_dos_seek
#define t2practice_diag_dos_size t2replay_dos_size
#define t2practice_diag_dos_write t2replay_dos_write
#define t2practice_diag_dos_delete t2replay_dos_delete
#else
#error T2PRACT.BIN diagnostics need an OP or MAIN owner
#endif

static int near t2practice_diag_dos_create(const char far *fn)
{
	unsigned fn_seg = T2PRACT_DIAG_FP_SEG(fn);
	unsigned fn_off = T2PRACT_DIAG_FP_OFF(fn);
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

static bool near t2practice_diag_exists(void)
{
	int fd;

	t2practice_diag_io_bypass_begin();
	fd = t2practice_diag_dos_open(
		T2PRACT_DIAG_FN, T2PRACT_DIAG_ACCESS_READ
	);

	if(fd < 0) {
		t2practice_diag_io_bypass_end();
		return false;
	}
	t2practice_diag_dos_close(fd);
	t2practice_diag_io_bypass_end();
	return true;
}

bool16 far t2practice_diag_no_sound(void)
{
	int fd;

	t2practice_diag_io_bypass_begin();
	fd = t2practice_diag_dos_open(
		T2PRACT_DIAG_NO_SOUND_FN, T2PRACT_DIAG_ACCESS_READ
	);
	if(fd >= 0) {
		t2practice_diag_dos_close(fd);
	}
	t2practice_diag_io_bypass_end();
	return (fd >= 0);
}

static void near t2practice_diag_clear_record(
	t2practice_diag_record_t far *record
)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(record);
	unsigned i;

	for(i = 0; i < sizeof(*record); i++) {
		p[i] = 0;
	}
}

static uint16_t near t2practice_diag_checksum(
	const t2practice_diag_record_t far *record
)
{
	const uint8_t far *p = reinterpret_cast<const uint8_t far *>(record);
	uint16_t sum = 0;
	unsigned i;

	for(i = 0; i < sizeof(*record); i++) {
		sum = static_cast<uint16_t>(sum + p[i]);
	}
	return sum;
}

static void near t2practice_diag_record_init(
	t2practice_diag_record_t far *record, uint8_t event,
	uint8_t mode, uint8_t flags, const t2replay_start_t far *start
)
{
	t2practice_diag_clear_record(record);
	record->magic[0] = 'T'; record->magic[1] = '2';
	record->magic[2] = 'P'; record->magic[3] = 'R';
	record->magic[4] = 'A'; record->magic[5] = 'C';
	record->magic[6] = 'T'; record->magic[7] = '1';
	record->schema = T2PRACT_DIAG_SCHEMA;
	record->build_id = T2REPLAY_PRACTICE_DIAGNOSTICS_BUILD_ID;
	record->event = event;
	record->command_mode = mode;
	record->command_flags = flags;
	record->stage = -1;
	record->target = 0xFF;
	record->map_length = -1;
	record->target_scroll_step = -1;
	record->top_map_row = -1;
	record->spawn_rows = -1;
	record->spawn_first_trigger = -1;
	record->spawn_upper_bound = -1;
	if(start != 0) {
		record->stage = start->stage;
		record->target = start->reserved[T2REPLAY_PRACTICE_TARGET_OFFSET];
	}
}

static void near t2practice_diag_write(
	const t2practice_diag_record_t far *record, bool reset
)
{
	t2practice_diag_record_t writable = *record;
	uint32_t size;
	int fd;
	bool wrote;

	writable.checksum = 0;
	writable.checksum = t2practice_diag_checksum(&writable);
	t2practice_diag_io_bypass_begin();
	if(reset) {
		fd = t2practice_diag_dos_create(T2PRACT_DIAG_FN);
		if(fd < 0) {
			t2practice_diag_io_bypass_end();
			return;
		}
		wrote = (
			t2practice_diag_dos_write(fd, &writable, sizeof(writable)) ==
			sizeof(writable)
		);
		t2practice_diag_dos_close(fd);
	} else {
		fd = t2practice_diag_dos_open(
			T2PRACT_DIAG_FN, T2PRACT_DIAG_ACCESS_RW
		);
		if(fd < 0) {
			t2practice_diag_io_bypass_end();
			return;
		}
		wrote = (
			t2practice_diag_dos_size(fd, &size) &&
			t2practice_diag_dos_seek(fd, size) &&
			(t2practice_diag_dos_write(fd, &writable, sizeof(writable)) ==
				 sizeof(writable))
		);
		t2practice_diag_dos_close(fd);
	}
	if(wrote) {
		t2practice_diag_dos_flush();
	}
	t2practice_diag_io_bypass_end();
}

static void near t2practice_diag_emit_current(uint8_t event, bool reset)
{
	t2practice_diag_current.event = event;
	t2practice_diag_write(&t2practice_diag_current, reset);
}

void t2practice_diag_clear(void)
{
	t2practice_diag_io_bypass_begin();
	if(t2practice_diag_exists()) {
		t2practice_diag_dos_delete(T2PRACT_DIAG_FN);
	}
	t2practice_diag_io_bypass_end();
	t2practice_diag_apply_active = false;
}

void t2practice_diag_reset(
	uint8_t mode, uint8_t flags, const t2replay_start_t far *start
)
{
	t2practice_diag_record_init(
		&t2practice_diag_current, T2PDE_OP_BEGIN, mode, flags, start
	);
	t2practice_diag_emit_current(T2PDE_OP_BEGIN, true);
	t2practice_diag_apply_active = false;
}

void t2practice_diag_op_command(
	enum t2practice_diag_reason_t reason, uint8_t mode, uint8_t flags,
	const t2replay_start_t far *start
)
{
	t2practice_diag_record_init(
		&t2practice_diag_current, T2PDE_OP_COMMAND_WRITE, mode, flags, start
	);
	t2practice_diag_current.reason = reason;
	t2practice_diag_emit_current(T2PDE_OP_COMMAND_WRITE, false);
}

void t2practice_diag_op_handoff(
	uint8_t mode, uint8_t flags, const t2replay_start_t far *start
)
{
	t2practice_diag_record_init(
		&t2practice_diag_current, T2PDE_OP_HANDOFF, mode, flags, start
	);
	t2practice_diag_emit_current(T2PDE_OP_HANDOFF, false);
}

void t2practice_diag_main_command(
	enum t2practice_diag_reason_t reason, const t2replay_command_t far *command
)
{
	if(!t2practice_diag_exists()) {
		return;
	}
	t2practice_diag_record_init(
		&t2practice_diag_current,
		T2PDE_MAIN_COMMAND_ADMISSION,
		(command != 0) ? command->mode : 0,
		(command != 0) ? command->flags : 0,
		(command != 0) ? &command->start : 0
	);
	t2practice_diag_current.reason = reason;
	t2practice_diag_emit_current(T2PDE_MAIN_COMMAND_ADMISSION, false);
}

void t2practice_diag_main_progress(
	enum t2practice_diag_main_progress_t progress, int8_t stage
)
{
	if(!t2practice_diag_exists()) {
		return;
	}
	t2practice_diag_record_init(
		&t2practice_diag_current, T2PDE_MAIN_PROGRESS, 0, 0, 0
	);
	t2practice_diag_current.stage = stage;
	t2practice_diag_current.target = static_cast<uint8_t>(progress);
	t2practice_diag_emit_current(T2PDE_MAIN_PROGRESS, false);
}

void t2practice_diag_apply_begin(
	int8_t stage, uint8_t target, int map_length, int spawn_rows
)
{
	if(!t2practice_diag_exists()) {
		t2practice_diag_apply_active = false;
		return;
	}
	t2practice_diag_record_init(
		&t2practice_diag_current, T2PDE_TARGET_APPLY_BEGIN, 0, 0, 0
	);
	t2practice_diag_current.stage = stage;
	t2practice_diag_current.target = target;
	t2practice_diag_current.map_length = map_length;
	t2practice_diag_current.spawn_rows = spawn_rows;
	t2practice_diag_emit_current(T2PDE_TARGET_APPLY_BEGIN, false);
	t2practice_diag_apply_active = true;
}

void t2practice_diag_target_scroll(int target_scroll_step)
{
	if(t2practice_diag_apply_active) {
		t2practice_diag_current.target_scroll_step = target_scroll_step;
	}
}

void t2practice_diag_top_map_row(int top_map_row)
{
	if(t2practice_diag_apply_active) {
		t2practice_diag_current.top_map_row = top_map_row;
	}
}

void t2practice_diag_spawn_first_trigger(int trigger)
{
	if(t2practice_diag_apply_active) {
		t2practice_diag_current.spawn_first_trigger = trigger;
	}
}

void t2practice_diag_spawn_upper_bound(int spawn_upper_bound)
{
	if(t2practice_diag_apply_active) {
		t2practice_diag_current.spawn_upper_bound = spawn_upper_bound;
	}
}

void t2practice_diag_failure(enum t2practice_diag_reason_t reason)
{
	if(t2practice_diag_apply_active &&
		(t2practice_diag_current.reason == T2PDR_NONE)) {
		t2practice_diag_current.reason = reason;
	}
}

void t2practice_diag_constructor_result(bool16 result)
{
	if(t2practice_diag_apply_active) {
		t2practice_diag_current.constructor_result = (
			result ? T2PDCR_SUCCESS : T2PDCR_FAILURE
		);
		if(!result) {
			t2practice_diag_failure(T2PDR_CONSTRUCTOR);
		}
	}
}

void t2practice_diag_apply_end(bool16 result)
{
	if(!t2practice_diag_apply_active) {
		return;
	}
	if(!result && (t2practice_diag_current.reason == T2PDR_NONE)) {
		t2practice_diag_current.reason = T2PDR_TARGET_APPLY;
	}
	t2practice_diag_emit_current(T2PDE_TARGET_APPLY_RESULT, false);
	t2practice_diag_apply_active = false;
}

#undef T2PRACT_DIAG_ACCESS_READ
#undef T2PRACT_DIAG_ACCESS_RW
#undef T2PRACT_DIAG_FP_SEG
#undef T2PRACT_DIAG_FP_OFF
#undef t2practice_diag_dos_open
#undef t2practice_diag_dos_close
#undef t2practice_diag_dos_flush
#undef t2practice_diag_dos_seek
#undef t2practice_diag_dos_size
#undef t2practice_diag_dos_write
#undef t2practice_diag_dos_delete

#endif
