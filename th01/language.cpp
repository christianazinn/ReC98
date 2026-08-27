/* TH01 patch-owned language preference runtime.
 *
 * Each process reads this compact, versioned configuration independently. It
 * is intentionally not a replay, resident, or score-file carrier.
 */

#pragma option -zCT1LANG_TEXT -G-

#include "th01/language.hpp"

#define T1LANG_CONFIG_SIZE 8
#define T1LANG_CONFIG_VERSION 1
#define T1LANG_FP_SEG(p) ((unsigned)(((unsigned long)(void far *)(p)) >> 16))
#define T1LANG_FP_OFF(p) ((unsigned)((unsigned long)(void far *)(p)))

struct t1_language_config_t {
	char magic[4];
	uint8_t version;
	uint8_t preference;
	uint8_t checksum;
	uint8_t checksum_inverse;
};

typedef char t1_language_config_size_check[
	(sizeof(t1_language_config_t) == T1LANG_CONFIG_SIZE) ? 1 : -1
];

struct t1_language_config_extent_t {
	t1_language_config_t config;
	uint8_t extra;
};

typedef char t1_language_config_extent_size_check[
	(sizeof(t1_language_config_extent_t) == (T1LANG_CONFIG_SIZE + 1)) ? 1 : -1
];

// Zero-initialization intentionally means Japanese before the first load.
static t1_language_preference_t t1_language_runtime;

static void t1_language_config_fn_set(char *fn)
{
	fn[0] = 'T';
	fn[1] = '1';
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

static uint8_t t1_language_config_checksum(const t1_language_config_t *config)
{
	const uint8_t *bytes = reinterpret_cast<const uint8_t *>(config);
	uint8_t sum = 0;
	uint8_t i;

	for(i = 0; i < 6; i++) {
		sum += bytes[i];
	}
	return sum;
}

static int t1_language_dos_open(const char far *fn)
{
	unsigned fn_seg = T1LANG_FP_SEG(fn);
	unsigned fn_off = T1LANG_FP_OFF(fn);
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

static unsigned t1_language_dos_read(
	int fh, void far *buffer, unsigned size
)
{
	unsigned buffer_seg = T1LANG_FP_SEG(buffer);
	unsigned buffer_off = T1LANG_FP_OFF(buffer);
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

static bool t1_language_dos_close(int fh)
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

void far t1_language_load(void)
{
	t1_language_config_extent_t extent;
	t1_language_config_t *config = &extent.config;
	uint8_t checksum;
	char fn[11];
	int fh;
	unsigned read;
	bool closed;

	// A receiver must never retain a stale selection after a missing or invalid
	// file. The all-zero BSS state also deliberately means Japanese.
	t1_language_runtime = T1LANG_JAPANESE;
	t1_language_config_fn_set(fn);
	fh = t1_language_dos_open(fn);
	if(fh < 0) {
		return;
	}
	// Reading one byte past the format rejects an overlong file without a seek.
	// DOS read errors return zero from the wrapper and fail the same extent test.
	read = t1_language_dos_read(fh, &extent, sizeof(extent));
	closed = t1_language_dos_close(fh);
	if((read != sizeof(extent.config)) || !closed) {
		return;
	}
	checksum = t1_language_config_checksum(config);
	if(
		(config->magic[0] != 'T') ||
		(config->magic[1] != '1') ||
		(config->magic[2] != 'L') ||
		(config->magic[3] != 'G') ||
		(config->version != T1LANG_CONFIG_VERSION) ||
		(config->preference > T1LANG_ENGLISH) ||
		(config->checksum != checksum) ||
		(config->checksum_inverse != static_cast<uint8_t>(~checksum))
	) {
		return;
	}
	t1_language_runtime = static_cast<t1_language_preference_t>(
		config->preference
	);
}

t1_language_preference_t far t1_language_get(void)
{
	return t1_language_runtime;
}

#pragma codeseg
