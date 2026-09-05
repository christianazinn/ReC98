/* TH02 patch-owned language preference runtime.
 *
 * This file is textually included by one dedicated trailing translation unit
 * per executable. Direct DOS I/O keeps its loose user files independent from
 * TH02's single active packfile hook.
 */

#pragma option -zCT2LANG_TEXT -G-

#include "th02/language.hpp"

#define T2LANG_CONFIG_SIZE 8
#define T2LANG_CONFIG_VERSION 1
#define T2_SETTINGS_LANGUAGE_MASK 0x01
#define T2_SETTINGS_REPLAY_RECORDING_DISABLED 0x40
#define T2_SETTINGS_AUTOFIRE 0x80
#define T2_SETTINGS_KNOWN_MASK ( \
	T2_SETTINGS_LANGUAGE_MASK | T2_SETTINGS_REPLAY_RECORDING_DISABLED | \
	T2_SETTINGS_AUTOFIRE \
)
#define T2LANG_DOS_ACCESS_READ 0
#define T2LANG_FP_SEG(p) ((unsigned)(((unsigned long)(void far *)(p)) >> 16))
#define T2LANG_FP_OFF(p) ((unsigned)((unsigned long)(void far *)(p)))

struct t2_language_config_t {
	char magic[4];
	uint8_t version;
	uint8_t preference;
	uint8_t checksum;
	uint8_t checksum_inverse;
};

typedef char t2_language_config_size_check[
	(sizeof(t2_language_config_t) == T2LANG_CONFIG_SIZE) ? 1 : -1
];

// Zero-initialization intentionally means Japanese before the first load.
static t2_language_preference_t t2_language_runtime;
static bool t2_autofire_runtime;
static bool t2_replay_recording_runtime;

static void t2_language_config_fn_set(char *fn)
{
	fn[0] = 'T';
	fn[1] = '2';
	fn[2] = 'L';
	fn[3] = 'A';
	fn[4] = 'N';
	fn[5] = 'G';
	fn[6] = '.';
	fn[7] = 'C';
	fn[8] = 'F';
	fn[9] = 'G';
	fn[10] = '\0';
}

static void t2_language_temp_fn_set(char *fn)
{
	fn[0] = 'T';
	fn[1] = '2';
	fn[2] = 'L';
	fn[3] = 'N';
	fn[4] = 'G';
	fn[5] = '.';
	fn[6] = 'T';
	fn[7] = 'M';
	fn[8] = 'P';
	fn[9] = '\0';
}

static void t2_language_backup_fn_set(char *fn)
{
	fn[0] = 'T';
	fn[1] = '2';
	fn[2] = 'L';
	fn[3] = 'N';
	fn[4] = 'G';
	fn[5] = '.';
	fn[6] = 'B';
	fn[7] = 'A';
	fn[8] = 'K';
	fn[9] = '\0';
}

static uint8_t t2_language_config_checksum(const t2_language_config_t *config)
{
	const uint8_t *bytes = reinterpret_cast<const uint8_t *>(config);
	uint8_t sum = 0;
	uint8_t i;

	for(i = 0; i < 6; i++) {
		sum += bytes[i];
	}
	return sum;
}

static int t2_language_dos_open(const char far *fn)
{
	unsigned fn_seg = T2LANG_FP_SEG(fn);
	unsigned fn_off = T2LANG_FP_OFF(fn);
	int result;

	_asm {
		push ds
		mov dx, fn_off
		mov ds, fn_seg
		mov ax, 3D00h
		int 21h
		pop ds
		sbb dx, dx
		or ax, dx
		mov result, ax
	}
	return result;
}

static int t2_language_dos_create(const char far *fn)
{
	unsigned fn_seg = T2LANG_FP_SEG(fn);
	unsigned fn_off = T2LANG_FP_OFF(fn);
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

static unsigned t2_language_dos_read(
	int fh, void far *buffer, unsigned size
)
{
	unsigned buffer_seg = T2LANG_FP_SEG(buffer);
	unsigned buffer_off = T2LANG_FP_OFF(buffer);
	unsigned result;

	_asm {
		push ds
		mov bx, fh
		mov cx, size
		mov dx, buffer_off
		mov ds, buffer_seg
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

static unsigned t2_language_dos_write(
	int fh, const void far *buffer, unsigned size
)
{
	unsigned buffer_seg = T2LANG_FP_SEG(buffer);
	unsigned buffer_off = T2LANG_FP_OFF(buffer);
	unsigned result;

	_asm {
		push ds
		mov bx, fh
		mov cx, size
		mov dx, buffer_off
		mov ds, buffer_seg
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

static bool t2_language_dos_close(int fh)
{
	unsigned result;

	_asm {
		mov bx, fh
		mov ah, 3Eh
		int 21h
		sbb ax, ax
		not ax
		mov result, ax
	}
	return (result != 0);
}

static bool t2_language_dos_delete(const char far *fn)
{
	unsigned fn_seg = T2LANG_FP_SEG(fn);
	unsigned fn_off = T2LANG_FP_OFF(fn);
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

static bool t2_language_dos_rename(
	const char far *from, const char far *to
)
{
	unsigned from_seg = T2LANG_FP_SEG(from);
	unsigned from_off = T2LANG_FP_OFF(from);
	unsigned to_seg = T2LANG_FP_SEG(to);
	unsigned to_off = T2LANG_FP_OFF(to);
	unsigned result;

	_asm {
		push ds
		push es
		mov dx, from_off
		mov ds, from_seg
		mov di, to_off
		mov es, to_seg
		mov ah, 56h
		int 21h
		pop es
		pop ds
		sbb ax, ax
		not ax
		mov result, ax
	}
	return (result != 0);
}

static bool t2_language_dos_seek(
	int fh, uint32_t offset, uint8_t origin, uint32_t far *position
)
{
	unsigned offset_hi = static_cast<unsigned>(offset >> 16);
	unsigned offset_lo = static_cast<unsigned>(offset & 0xFFFFUL);
	unsigned result_hi;
	unsigned result_lo;
	unsigned failed;

	_asm {
		mov bx, fh
		mov cx, offset_hi
		mov dx, offset_lo
		mov ah, 42h
		mov al, origin
		int 21h
		mov result_lo, ax
		mov result_hi, dx
		sbb ax, ax
		neg ax
		mov failed, ax
	}
	*position = (
		(static_cast<uint32_t>(result_hi) << 16) |
		static_cast<uint32_t>(result_lo)
	);
	return (failed == 0);
}

static void t2_language_dos_flush(void)
{
	_asm {
		mov ah, 0Dh
		int 21h
	}
}

void far t2_language_load(void)
{
	uint8_t extent[T2LANG_CONFIG_SIZE + 1];
	t2_language_config_t *config =
		reinterpret_cast<t2_language_config_t *>(extent);
	uint8_t checksum;
	char fn[11];
	int fh;
	unsigned read;
	bool closed;

	// A receiver must never retain a stale selection after a missing or invalid
	// file. The all-zero BSS state also deliberately means Japanese.
	t2_language_runtime = T2LANG_JAPANESE;
	t2_autofire_runtime = false;
	t2_replay_recording_runtime = true;
	t2_language_config_fn_set(fn);
	fh = t2_language_dos_open(fn);
	if(fh < 0) {
		return;
	}
	// Reading one byte past the format rejects an overlong file without a seek.
	// DOS read errors return zero from the wrapper and fail the same extent test.
	read = t2_language_dos_read(fh, extent, sizeof(extent));
	closed = t2_language_dos_close(fh);
	if((read != T2LANG_CONFIG_SIZE) || !closed) {
		return;
	}
	checksum = t2_language_config_checksum(config);
	if(
		(config->magic[0] != 'T') ||
		(config->magic[1] != '2') ||
		(config->magic[2] != 'L') ||
		(config->magic[3] != 'G') ||
		(config->version != T2LANG_CONFIG_VERSION) ||
		((config->preference & ~T2_SETTINGS_KNOWN_MASK) != 0) ||
		((config->preference & T2_SETTINGS_LANGUAGE_MASK) > T2LANG_ENGLISH) ||
		(config->checksum != checksum) ||
		(config->checksum_inverse != static_cast<uint8_t>(~checksum))
	) {
		return;
	}
	t2_language_runtime = static_cast<t2_language_preference_t>(
		config->preference & T2_SETTINGS_LANGUAGE_MASK
	);
	t2_autofire_runtime = ((config->preference & T2_SETTINGS_AUTOFIRE) != 0);
	t2_replay_recording_runtime = (
		(config->preference & T2_SETTINGS_REPLAY_RECORDING_DISABLED) == 0
	);
}

t2_language_preference_t far t2_language_get(void)
{
	return t2_language_runtime;
}

static bool t2_settings_set(
	t2_language_preference_t preference, bool autofire, bool replay_recording
)
{
	t2_language_config_t config;
	char fn[11];
	char temporary[10];
	char backup[10];
	int fh;
	bool had_previous = false;
	bool closed;

	if((preference != T2LANG_JAPANESE) && (preference != T2LANG_ENGLISH)) {
		return false;
	}
	t2_language_config_fn_set(fn);
	t2_language_temp_fn_set(temporary);
	t2_language_backup_fn_set(backup);
	config.magic[0] = 'T';
	config.magic[1] = '2';
	config.magic[2] = 'L';
	config.magic[3] = 'G';
	config.version = T2LANG_CONFIG_VERSION;
	config.preference = static_cast<uint8_t>(
		preference |
		(autofire ? T2_SETTINGS_AUTOFIRE : 0) |
		(replay_recording ? 0 : T2_SETTINGS_REPLAY_RECORDING_DISABLED)
	);
	config.checksum = t2_language_config_checksum(&config);
	config.checksum_inverse = static_cast<uint8_t>(~config.checksum);

	// The temporary is complete and durable before either existing visible file
	// moves. Each DOS rename is atomic; an interrupted replacement can leave
	// only a complete old record, a complete new record, or Japanese fallback.
	t2_language_dos_delete(temporary);
	fh = t2_language_dos_create(temporary);
	if(fh < 0) {
		return false;
	}
	if(t2_language_dos_write(fh, &config, sizeof(config)) != sizeof(config)) {
		t2_language_dos_close(fh);
		t2_language_dos_delete(temporary);
		return false;
	}
	closed = t2_language_dos_close(fh);
	if(!closed) {
		t2_language_dos_delete(temporary);
		return false;
	}
	t2_language_dos_flush();

	fh = t2_language_dos_open(fn);
	if(fh >= 0) {
		if(!t2_language_dos_close(fh)) {
			t2_language_dos_delete(temporary);
			return false;
		}
		had_previous = true;
	}
	t2_language_dos_delete(backup);
	if(had_previous && !t2_language_dos_rename(fn, backup)) {
		t2_language_dos_delete(temporary);
		return false;
	}
	if(!t2_language_dos_rename(temporary, fn)) {
		if(had_previous) {
			t2_language_dos_rename(backup, fn);
		}
		t2_language_dos_delete(temporary);
		return false;
	}
	if(had_previous) {
		t2_language_dos_delete(backup);
	}
	t2_language_dos_flush();
	t2_language_runtime = preference;
	t2_autofire_runtime = autofire;
	t2_replay_recording_runtime = replay_recording;
	return true;
}

bool far t2_language_set(t2_language_preference_t preference)
{
	return t2_settings_set(
		preference, t2_autofire_runtime, t2_replay_recording_runtime
	);
}

bool far t2_autofire_get(void)
{
	return t2_autofire_runtime;
}

bool far t2_autofire_set(bool enabled)
{
	return t2_settings_set(
		t2_language_runtime, enabled, t2_replay_recording_runtime
	);
}

bool far t2_replay_recording_enabled(void)
{
	return t2_replay_recording_runtime;
}

bool far t2_replay_recording_set(bool enabled)
{
	return t2_settings_set(t2_language_runtime, t2_autofire_runtime, enabled);
}

// Keep this shared patch segment's growth paragraph-aligned in OP, MAIN, and
// MAINE so every following segment retains its audited phase.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90"

#include "th02/t2langov.inc"
