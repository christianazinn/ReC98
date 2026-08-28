#ifdef T2PD
#pragma codeseg T2M9D_TEXT PATCH
#endif

#include "th02/t2m9diag.hpp"

#ifdef T2PD

// T2M9GO.BIN is the one-shot host trigger. OP consumes it before the title
// loop, writes the normal private Practice command plus this handoff, and
// replaces itself with MAIN. MAIN consumes the handoff exactly once.
static const char t2m9diag_go_fn[] = "T2M9GO.BIN";
static const char t2m9diag_handoff_fn[] = "T2M9AP.BIN";
static const char t2m9diag_result_fn[] = "T2M9P.BIN";

enum t2m9diag_result_t {
	T2M9DR_ARMED = 1,
	T2M9DR_ACTIVE = 2,
	T2M9DR_REJECTED = 3,
};

#if defined(T2M9DIAG_MAIN)
static bool t2m9diag_armed = false;
#endif

static void near t2m9diag_record_magic(uint8_t far *record, char suffix)
{
	record[0] = 'T'; record[1] = '2'; record[2] = 'M'; record[3] = '9';
	record[4] = suffix; record[5] = '1'; record[6] = '\0'; record[7] = '\0';
}

static void near t2m9diag_u32_put(uint8_t far *record, uint32_t value)
{
	record[0] = static_cast<uint8_t>(value);
	record[1] = static_cast<uint8_t>(value >> 8);
	record[2] = static_cast<uint8_t>(value >> 16);
	record[3] = static_cast<uint8_t>(value >> 24);
}

static uint32_t near t2m9diag_u32_get(const uint8_t far *record)
{
	return (
		static_cast<uint32_t>(record[0]) |
		(static_cast<uint32_t>(record[1]) << 8) |
		(static_cast<uint32_t>(record[2]) << 16) |
		(static_cast<uint32_t>(record[3]) << 24)
	);
}

static bool near t2m9diag_magic_matches(
	const uint8_t far *record, char suffix
)
{
	return (
		(record[0] == 'T') && (record[1] == '2') &&
		(record[2] == 'M') && (record[3] == '9') &&
		(record[4] == suffix) && (record[5] == '1') &&
		(record[6] == '\0') && (record[7] == '\0')
	);
}

#if defined(T2M9DIAG_OP)

// OP includes this after op/replay.cpp, so the strict DOS and replay-command
// helpers remain private to this profile and no public OP surface is widened.
static bool near t2m9diag_op_record_write(
	const char far *fn, char suffix, uint32_t resident_frame
)
{
	uint8_t record[12];
	int fd;

	t2m9diag_record_magic(record, suffix);
	t2m9diag_u32_put((record + 8), resident_frame);
	fd = t2op_dos_create(fn);
	if(fd < 0) {
		return false;
	}
	if(t2op_dos_write(fd, record, sizeof(record)) != sizeof(record)) {
		t2op_dos_close(fd);
		t2op_file_delete(fn);
		return false;
	}
	t2op_dos_close(fd);
	t2op_dos_flush();
	return true;
}

void t2m9diag_op_autostart(void)
{
	uint8_t trigger[9];
	char handoff_fn[12];
	char request_fn[11];
	int fd = t2op_dos_open(t2m9diag_go_fn, T2OP_DOS_ACCESS_READ);

	if(fd < 0) {
		return;
	}
	if((t2op_dos_read(fd, trigger, sizeof(trigger)) != 8) ||
		!t2m9diag_magic_matches(trigger, 'G')) {
		t2op_dos_close(fd);
		t2op_file_delete(t2m9diag_go_fn);
		return;
	}
	t2op_dos_close(fd);
	t2op_file_delete(t2m9diag_go_fn);
	t2op_file_delete(t2m9diag_handoff_fn);
	t2op_file_delete(t2m9diag_result_fn);

	t2op_practice_defaults();
	t2op_practice_stage_set(4);
	t2op_practice.shottype = 0;
	t2op_practice.reserved[T2REPLAY_PRACTICE_TARGET_OFFSET] =
		T2RPT_STAGE5_BOSS_PHASE7;
	if(!t2op_start_valid(&t2op_practice)) {
		return;
	}
	cfg_save();
	t2op_save_request_fn_set(request_fn);
	t2op_file_delete(request_fn);
	t2op_temp_set();
	t2op_file_delete(t2op_slot_fn);
	if(!t2op_command_write(
		T2REPLAY_COMMAND_RECORD,
		T2REPLAY_TEMP_SLOT,
		T2REPLAY_COMMAND_FLAG_PRACTICE,
		&t2op_practice
	)) {
		return;
	}
	if(!t2m9diag_op_record_write(
		t2m9diag_handoff_fn, 'A', t2op_practice.resident_frame
	)) {
		t2op_handoff_fn_set(handoff_fn);
		t2op_file_delete(handoff_fn);
		t2op_file_delete(t2op_command_fn);
		return;
	}
	// Match the existing direct diagnostic launch: the full Practice record
	// replaces these transient title fields before MAIN derives gameplay state.
	resident->unused_3 = 0;
	resident->unused_1 = 0;
	t2op_resident_apply(&t2op_practice);
	t2op_main_exec();
}

#elif defined(T2M9DIAG_MAIN)

#include "th02/main/later_boss_practice.hpp"

#define T2M9DIAG_FP_SEG(p) ((unsigned)(((unsigned long)(void far *)(p)) >> 16))
#define T2M9DIAG_FP_OFF(p) ((unsigned)((unsigned long)(void far *)(p)))

static int near t2m9diag_dos_open(const char far *fn)
{
	unsigned fn_seg = T2M9DIAG_FP_SEG(fn);
	unsigned fn_off = T2M9DIAG_FP_OFF(fn);
	int result;

	_asm {
		push	ds
		mov	dx, fn_off
		mov	ds, fn_seg
		mov	ax, 3D00h
		int	21h
		pop	ds
		sbb	dx, dx
		or	ax, dx
		mov	result, ax
	}
	return result;
}

static unsigned near t2m9diag_dos_read(
	int fd, void far *buffer, unsigned size
)
{
	unsigned buffer_seg = T2M9DIAG_FP_SEG(buffer);
	unsigned buffer_off = T2M9DIAG_FP_OFF(buffer);
	unsigned result;

	_asm {
		push	ds
		mov	bx, fd
		mov	cx, size
		mov	dx, buffer_off
		mov	ds, buffer_seg
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

static int near t2m9diag_dos_create(const char far *fn)
{
	unsigned fn_seg = T2M9DIAG_FP_SEG(fn);
	unsigned fn_off = T2M9DIAG_FP_OFF(fn);
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

static unsigned near t2m9diag_dos_write(
	int fd, const void far *buffer, unsigned size
)
{
	unsigned buffer_seg = T2M9DIAG_FP_SEG(buffer);
	unsigned buffer_off = T2M9DIAG_FP_OFF(buffer);
	unsigned result;

	_asm {
		push	ds
		mov	bx, fd
		mov	cx, size
		mov	dx, buffer_off
		mov	ds, buffer_seg
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

static void near t2m9diag_dos_close(int fd)
{
	_asm {
		mov	bx, fd
		mov	ah, 3Eh
		int	21h
	}
}

static void near t2m9diag_dos_delete(const char far *fn)
{
	unsigned fn_seg = T2M9DIAG_FP_SEG(fn);
	unsigned fn_off = T2M9DIAG_FP_OFF(fn);

	_asm {
		push	ds
		mov	dx, fn_off
		mov	ds, fn_seg
		mov	ah, 41h
		int	21h
		pop	ds
	}
}

static void near t2m9diag_dos_flush(void)
{
	_asm {
		mov	ah, 0Dh
		int	21h
	}
}

static void near t2m9diag_result_write(
	enum t2m9diag_result_t result, uint32_t resident_frame
)
{
	uint8_t record[14];
	int fd;

	t2m9diag_record_magic(record, 'P');
	record[8] = static_cast<uint8_t>(result);
	record[9] = 4;
	t2m9diag_u32_put((record + 10), resident_frame);
	fd = t2m9diag_dos_create(t2m9diag_result_fn);
	if(fd < 0) {
		return;
	}
	if(t2m9diag_dos_write(fd, record, sizeof(record)) == sizeof(record)) {
		t2m9diag_dos_flush();
	}
	t2m9diag_dos_close(fd);
}

void far t2m9diag_main_entry_arm(void)
{
	uint8_t handoff[13];
	int fd = t2m9diag_dos_open(t2m9diag_handoff_fn);
	bool valid;

	if(fd < 0) {
		return;
	}
	valid = (
		(t2m9diag_dos_read(fd, handoff, sizeof(handoff)) == 12) &&
		t2m9diag_magic_matches(handoff, 'A') &&
		(t2m9diag_u32_get(handoff + 8) == resident->frame)
	);
	t2m9diag_dos_close(fd);
	t2m9diag_dos_delete(t2m9diag_handoff_fn);
	if(!valid) {
		t2m9diag_result_write(T2M9DR_REJECTED, resident->frame);
		return;
	}
	t2m9diag_armed = true;
	t2m9diag_result_write(T2M9DR_ARMED, resident->frame);
}

bool16 far t2m9diag_practice_target_apply(void)
{
	if(!t2m9diag_armed) {
		return true;
	}
	t2m9diag_armed = false;
	if(stage_id != 4) {
		t2m9diag_result_write(T2M9DR_REJECTED, resident->frame);
		return false;
	}
	// replay_practice_target_apply() has already completed the proven public
	// Phase 7 terminal-field and callback setup. Replace only the actor and
	// presentation payload before stage_loop() reaches its first gameplay frame.
	if(!th02_later_boss_clean_init(T2LBPT_MIMA_PHASE9)) {
		t2m9diag_result_write(T2M9DR_REJECTED, resident->frame);
		return false;
	}
	t2m9diag_result_write(T2M9DR_ACTIVE, resident->frame);
	return true;
}

#endif /* T2M9DIAG_OP / T2M9DIAG_MAIN */
#endif /* T2PD */
