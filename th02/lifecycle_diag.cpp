#include "th02/practice_diag.hpp"

#if T2REPLAY_PRACTICE_DIAGNOSTICS

#pragma codeseg T2LIFEDIAG_TEXT

// T2LIFE.BIN is linked into OP, MAIN, and MAINE. It deliberately uses only
// DOS calls local to this tail, so MAINE's shared heap admission is observable
// without coupling the ending executable to MAIN's replay writer.
#define T2LIFE_FP_SEG(p) ((unsigned)(((unsigned long)(void far *)(p)) >> 16))
#define T2LIFE_FP_OFF(p) ((unsigned)((unsigned long)(void far *)(p)))

static const char T2LIFE_DIAG_FN[] = "T2LIFE.BIN";
static uint16_t t2life_chosen_paras;
static uint16_t t2life_min_available_paras;
#if (BINARY == 'M')
extern "C" uint8_t far t2practice_diag_pf_hook;
static uint8_t t2life_pf_hook_saved;
static uint8_t t2life_pf_hook_depth;

void t2practice_diag_io_bypass_begin(void)
{
	if(t2life_pf_hook_depth == 0) {
		t2life_pf_hook_saved = t2practice_diag_pf_hook;
		t2practice_diag_pf_hook = 1;
	}
	t2life_pf_hook_depth++;
}

void t2practice_diag_io_bypass_end(void)
{
	if(t2life_pf_hook_depth == 0) {
		return;
	}
	t2life_pf_hook_depth--;
	if(t2life_pf_hook_depth == 0) {
		t2practice_diag_pf_hook = t2life_pf_hook_saved;
	}
}
#else
void t2practice_diag_io_bypass_begin(void)
{
}

void t2practice_diag_io_bypass_end(void)
{
}
#endif

static int near t2life_dos_open(const char far *fn)
{
	unsigned fn_seg = T2LIFE_FP_SEG(fn);
	unsigned fn_off = T2LIFE_FP_OFF(fn);
	int result;

	_asm {
		push ds
		mov dx, fn_off
		mov ds, fn_seg
		mov ax, 3D02h
		int 21h
		pop ds
		sbb dx, dx
		or ax, dx
		mov result, ax
	}
	return result;
}

static int near t2life_dos_create(const char far *fn)
{
	unsigned fn_seg = T2LIFE_FP_SEG(fn);
	unsigned fn_off = T2LIFE_FP_OFF(fn);
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

static void near t2life_dos_close(int fd)
{
	_asm {
		mov bx, fd
		mov ah, 3Eh
		int 21h
	}
}

static bool near t2life_dos_seek(int fd, uint32_t offset)
{
	unsigned offset_hi = static_cast<unsigned>(offset >> 16);
	unsigned offset_lo = static_cast<unsigned>(offset);
	unsigned failed;

	_asm {
		mov bx, fd
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

static bool near t2life_dos_size(int fd, uint32_t far *size)
{
	unsigned size_hi;
	unsigned size_lo;
	unsigned failed;

	_asm {
		mov bx, fd
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

static bool near t2life_dos_write(
	int fd, const void far *buf, unsigned size
)
{
	unsigned buf_seg = T2LIFE_FP_SEG(buf);
	unsigned buf_off = T2LIFE_FP_OFF(buf);
	unsigned written;

	_asm {
		push ds
		mov bx, fd
		mov cx, size
		mov dx, buf_off
		mov ds, buf_seg
		mov ah, 40h
		int 21h
		pop ds
		sbb cx, cx
		not cx
		and ax, cx
		mov written, ax
	}
	return (written == size);
}

static void near t2life_dos_flush(void)
{
	_asm {
		mov ah, 0Dh
		int 21h
	}
}

static void near t2life_record_clear(t2practice_lifecycle_record_t far *record)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(record);
	unsigned i;

	for(i = 0; i < sizeof(*record); i++) {
		p[i] = 0;
	}
}

static void near t2life_record_init(
	t2practice_lifecycle_record_t far *record,
	enum t2practice_lifecycle_milestone_t milestone
)
{
	t2life_record_clear(record);
	record->magic[0] = 'T'; record->magic[1] = '2';
	record->magic[2] = 'L'; record->magic[3] = 'I';
	record->magic[4] = 'F'; record->magic[5] = 'E';
	record->magic[6] = '1'; record->magic[7] = '\0';
	record->schema = T2LIFE_DIAG_SCHEMA;
	record->milestone = milestone;
}

static void near t2life_append(const t2practice_lifecycle_record_t far *record)
{
	t2practice_lifecycle_record_t writable = *record;
	uint32_t size;
	int fd;
	bool wrote;

	t2practice_diag_io_bypass_begin();
	fd = t2life_dos_open(T2LIFE_DIAG_FN);

	if(fd < 0) {
		fd = t2life_dos_create(T2LIFE_DIAG_FN);
		if(fd < 0) {
			t2practice_diag_io_bypass_end();
			return;
		}
		wrote = t2life_dos_write(fd, &writable, sizeof(writable));
	} else {
		wrote = (
			t2life_dos_size(fd, &size) &&
			t2life_dos_seek(fd, size) &&
			t2life_dos_write(fd, &writable, sizeof(writable))
		);
	}
	t2life_dos_close(fd);
	if(wrote) {
		t2life_dos_flush();
	}
	t2practice_diag_io_bypass_end();
}

void t2practice_diag_lifecycle_op_menu_enter(void)
{
	t2practice_lifecycle_record_t record;

	// The harness removes the file before launch. Do not truncate it here:
	// reaching OP after MAIN or MAINE is part of the acceptance trace.
	t2life_chosen_paras = 0;
	t2life_min_available_paras = 0;
	t2life_record_init(&record, T2PDLM_OP_MENU);
	t2life_append(&record);
}

void t2practice_diag_lifecycle(
	enum t2practice_lifecycle_milestone_t milestone,
	uint16_t largest_paras, uint16_t chosen_paras, uint16_t available_paras
)
{
	t2practice_lifecycle_record_t record;

	if(chosen_paras != 0) {
		t2life_chosen_paras = chosen_paras;
	}
	if(
		(available_paras != 0) &&
		(
			(t2life_min_available_paras == 0) ||
			(available_paras < t2life_min_available_paras)
		)
	) {
		t2life_min_available_paras = available_paras;
	}
	t2life_record_init(&record, milestone);
	record.largest_paras = largest_paras;
	record.chosen_paras = t2life_chosen_paras;
	record.available_paras = available_paras;
	if(record.chosen_paras >= t2life_min_available_paras) {
		record.high_water_paras = static_cast<uint16_t>(
			record.chosen_paras - t2life_min_available_paras
		);
	}
	t2life_append(&record);
}

#endif
