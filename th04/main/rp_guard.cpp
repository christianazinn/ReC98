#pragma option -zCREPLAY_PROTECT_TEXT

// Physical-disk savestate rollback guard shared by TH04 and TH05 MAIN.
// Ported from the proven TH03 implementation through TH01's reduced binding.

#include "platform.h"
#include "x86real.h"
#include "th04/main/rp_guard.hpp"

#define RPG_FP_SEG(p) ((unsigned)(((unsigned long)(void far *)(p)) >> 16))
#define RPG_FP_OFF(p) ((unsigned)((unsigned long)(void far *)(p)))

// Both canonical TH04/TH05 HDIs use 1024-byte FAT sectors. A 4096-byte
// catch-all buffer fails to allocate in TH05's tightest gameplay paths, which
// would disable every legitimate replay. Refuse non-native larger-sector
// media explicitly instead of spending 3 KiB of conventional memory forever.
#define RPG_SECTOR_BUFFER_SIZE_MAX 1024
#define RPG_ABS_READ_PACKET_SIZE 10
#define RPG_SECTOR_ALLOCATION_SIZE \
	(RPG_SECTOR_BUFFER_SIZE_MAX + \
	 RPG_ABS_READ_PACKET_SIZE)

#define RPG_FAT12_CLUSTER_COUNT_MAX 4085UL
#define RPG_FAT16_CLUSTER_COUNT_MAX 65525UL
#define RPG_FAT32_CLUSTER_MAX 0x0FFFFFEFUL
#define RPG_FAT32_ENTRY_MASK 0x0FFFFFFFUL
#define RPG_FAT32_CLUSTER_BAD 0x0FFFFFF7UL
#define RPG_FAT32_CLUSTER_EOC 0x0FFFFFF8UL

#define RPG_FAT_ENTRY_FREE 0x00
#define RPG_FAT_ENTRY_DELETED 0xE5
#define RPG_FAT_ATTR_SKIP 0x18
#define RPG_DIR_ENTRY_SIZE 32
#define RPG_GUARD_BASE_SIZE 1

#define RPG_FAT_UNKNOWN 0
#define RPG_FAT12 12
#define RPG_FAT16 16
#define RPG_FAT32 32

#define RPG_SCAN_CONTINUE 0
#define RPG_SCAN_FOUND 1
#define RPG_SCAN_END 2

#define RPG_FLAGS_INDEX 0
#define RPG_GUARD_SECTOR_INDEX 1
#define RPG_GUARD_OFFSET_INDEX 5
#define RPG_DIAG_CODE_INDEX 7
#define RPG_DIAG_DRIVE_INDEX 8
#define RPG_DIAG_DOS_AX_INDEX 9
#define RPG_DIAG_BPS_INDEX 11
#define RPG_DIAG_ROOT_ENTS_INDEX 13
#define RPG_DIAG_ROOT_START_INDEX 15
#define RPG_DIAG_ROOT_SECS_INDEX 19
#define RPG_DIAG_SECTOR_INDEX 21
#define RPG_DIAG_OFFSET_INDEX 25
#define RPG_DIAG_EXPECTED_INDEX 27
#define RPG_DIAG_ACTUAL_INDEX 31
#define RPG_DIAG_I25_FLAGS_INDEX 35
#define RPG_DIAG_I25_STACK_INDEX 37
#define RPG_STATE_SIZE 39

#define RPG_NONE RPD_NONE
#define RPG_CTX_ALLOC RPD_CTX_ALLOC
#define RPG_BPB_READ RPD_BPB_READ
#define RPG_BPB_UNSUPPORTED RPD_BPB_UNSUPPORTED
#define RPG_ROOT_RANGE RPD_ROOT_RANGE
#define RPG_SHORT_NAME RPD_SHORT_NAME
#define RPG_ROOT_SECTOR_READ RPD_ROOT_SECTOR_READ
#define RPG_ROOT_NOT_FOUND RPD_ROOT_NOT_FOUND
#define RPG_LOCATED_RANGE RPD_LOCATED_RANGE
#define RPG_LOCATED_SECTOR_READ RPD_LOCATED_SECTOR_READ
#define RPG_LOCATED_ENTRY_BAD RPD_LOCATED_ENTRY_BAD
#define RPG_LOCATED_NAME RPD_LOCATED_NAME
#define RPG_VERIFY_MISMATCH RPD_VERIFY_MISMATCH
#define RPG_GUARD_CREATE RPD_GUARD_CREATE
#define RPG_GUARD_CREATE_NONZERO RPD_GUARD_CREATE_NONZERO
#define RPG_CHECKPOINT_APPEND RPD_CHECKPOINT_APPEND
#define RPG_CHECKPOINT_WRITE RPD_CHECKPOINT_WRITE
#define RPG_CHECKPOINT_MISMATCH RPD_CHECKPOINT_MISMATCH
#define RPG_CLOSE_COMMIT RPD_CLOSE_COMMIT
#define RPG_MARKER_CLUSTER RPD_MARKER_CLUSTER
#define RPG_MARKER_VALUE RPD_MARKER_VALUE
#define RPG_MARKER_WRITE RPD_MARKER_WRITE
#define RPG_MARKER_VERIFY RPD_MARKER_VERIFY
#define RPG_ROOT_CHAIN RPD_ROOT_CHAIN
#define RPG_ROOT_FAT_READ RPD_ROOT_FAT_READ
#define RPG_EXTENDED_READ_UNAVAILABLE \
	RPD_EXTENDED_READ_UNAVAILABLE
#define RPG_MARKER_SECTOR_READ RPD_MARKER_SECTOR_READ

#define RPG_FLAG_INVALID REPLAY_PROTECT_FLAG_INVALID
#define RPG_FLAG_LOCATED REPLAY_PROTECT_FLAG_LOCATED
#define RPG_FLAG_ERROR REPLAY_PROTECT_FLAG_ERROR

struct rpg_ctx_t {
	uint8_t far *sector;
	uint8_t far *packet;
	uint8_t drive;
	uint8_t fat_type;
	bool extended_abs_read;
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint32_t total_sectors;
	uint32_t fat_start;
	uint32_t fat_sectors;
	uint32_t root_start;
	uint32_t root_sectors;
	uint16_t root_entry_count;
	uint32_t first_data_sector;
	uint32_t cluster_count;
	uint32_t root_cluster;
};

static uint8_t rpg_state[RPG_STATE_SIZE];
static uint32_t rpg_committed_size;
static uint8_t rpg_last_fat_type;
static char replay_protect_guard_fn[12];
static char replay_protect_diag_fn[12];

static int replay_protect_file_create(const char *fn)
{
	unsigned fn_seg = RPG_FP_SEG(fn);
	unsigned fn_off = RPG_FP_OFF(fn);
	int result;

	asm {
		push ds
		mov  dx, fn_off
		mov  ds, fn_seg
		mov  ah, 3Ch
		xor  cx, cx
		int  21h
		pop  ds
		sbb  dx, dx
		or   ax, dx
		mov  result, ax
	}
	return result;
}

static int replay_protect_file_update(const char *fn)
{
	unsigned fn_seg = RPG_FP_SEG(fn);
	unsigned fn_off = RPG_FP_OFF(fn);
	int result;

	asm {
		push ds
		mov  dx, fn_off
		mov  ds, fn_seg
		mov  ax, 3D01h
		int  21h
		pop  ds
		sbb  dx, dx
		or   ax, dx
		mov  result, ax
	}
	return result;
}

static void replay_protect_file_close(int fd)
{
	asm {
		mov bx, fd
		mov ah, 3Eh
		int 21h
	}
}

static bool replay_protect_file_seek(int fd, uint32_t pos)
{
	unsigned pos_hi = static_cast<unsigned>(pos >> 16);
	unsigned pos_lo = static_cast<unsigned>(pos);
	unsigned failed;

	asm {
		mov bx, fd
		mov cx, pos_hi
		mov dx, pos_lo
		mov ax, 4200h
		int 21h
		sbb ax, ax
		neg ax
		mov failed, ax
	}
	return (failed == 0);
}

static void replay_protect_file_delete(const char *fn)
{
	unsigned fn_seg = RPG_FP_SEG(fn);
	unsigned fn_off = RPG_FP_OFF(fn);

	asm {
		push ds
		mov  dx, fn_off
		mov  ds, fn_seg
		mov  ah, 41h
		int  21h
		pop  ds
	}
}

static bool replay_protect_file_write(
	int fd, const void far *buf, unsigned size
)
{
	unsigned buf_seg = RPG_FP_SEG(buf);
	unsigned buf_off = RPG_FP_OFF(buf);
	unsigned written;

	asm {
		push ds
		mov  bx, fd
		mov  cx, size
		mov  dx, buf_off
		mov  ds, buf_seg
		mov  ah, 40h
		int  21h
		pop  ds
		sbb  cx, cx
		not  cx
		and  ax, cx
		mov  written, ax
	}
	return (written == size);
}

static uint8_t rpg_u8(unsigned index)
{
	return rpg_state[index];
}

static void rpg_u8_write(unsigned index, uint8_t value)
{
	rpg_state[index] = value;
}

static uint16_t rpg_u16(unsigned index)
{
	return static_cast<uint16_t>(
		rpg_state[index] |
		(static_cast<uint16_t>(rpg_state[index + 1]) << 8)
	);
}

static void rpg_u16_write(unsigned index, uint16_t value)
{
	rpg_state[index + 0] = static_cast<uint8_t>(value);
	rpg_state[index + 1] = static_cast<uint8_t>(value >> 8);
}

static uint32_t rpg_u32(unsigned index)
{
	return (
		static_cast<uint32_t>(rpg_state[index + 0]) |
		(static_cast<uint32_t>(rpg_state[index + 1]) << 8) |
		(static_cast<uint32_t>(rpg_state[index + 2]) << 16) |
		(static_cast<uint32_t>(rpg_state[index + 3]) << 24)
	);
}

static void rpg_u32_write(unsigned index, uint32_t value)
{
	rpg_state[index + 0] = static_cast<uint8_t>(value);
	rpg_state[index + 1] = static_cast<uint8_t>(value >> 8);
	rpg_state[index + 2] = static_cast<uint8_t>(value >> 16);
	rpg_state[index + 3] = static_cast<uint8_t>(value >> 24);
}

/// Diagnostics
/// -----------

static void rpg_diag_code_set(uint8_t code)
{
	rpg_u8_write(RPG_DIAG_CODE_INDEX, code);
}

// Never overwrite the FIRST failure with a later, less informative one.
static void rpg_diag_code_set_if_none(uint8_t code)
{
	if(rpg_u8(RPG_DIAG_CODE_INDEX) == RPG_NONE) {
		rpg_diag_code_set(code);
	}
}

static void rpg_diag_sizes_set(uint32_t expected, uint32_t actual)
{
	rpg_u32_write(RPG_DIAG_EXPECTED_INDEX, expected);
	rpg_u32_write(RPG_DIAG_ACTUAL_INDEX, actual);
}

static void rpg_diag_location_set(uint32_t sector, uint16_t offset)
{
	rpg_u32_write(RPG_DIAG_SECTOR_INDEX, sector);
	rpg_u16_write(RPG_DIAG_OFFSET_INDEX, offset);
}

static void rpg_diag_dos_ax_set(uint16_t ax)
{
	rpg_u16_write(RPG_DIAG_DOS_AX_INDEX, ax);
}

static void rpg_diag_int25_flags_set(uint16_t flags, uint16_t stack_flags)
{
	rpg_u16_write(RPG_DIAG_I25_FLAGS_INDEX, flags);
	rpg_u16_write(RPG_DIAG_I25_STACK_INDEX, stack_flags);
}

static void rpg_diag_geometry_set(
	uint8_t drive, uint16_t bytes_per_sector, uint16_t root_entries,
	uint32_t root_start, uint16_t root_sectors
)
{
	rpg_u8_write(RPG_DIAG_DRIVE_INDEX, drive);
	rpg_u16_write(RPG_DIAG_BPS_INDEX, bytes_per_sector);
	rpg_u16_write(RPG_DIAG_ROOT_ENTS_INDEX, root_entries);
	rpg_u32_write(RPG_DIAG_ROOT_START_INDEX, root_start);
	rpg_u16_write(RPG_DIAG_ROOT_SECS_INDEX, root_sectors);
}

/// Flags
/// -----

static uint8_t rpg_flags(void)
{
	return rpg_u8(RPG_FLAGS_INDEX);
}

static void rpg_flag_set(uint8_t bit)
{
	rpg_u8_write(RPG_FLAGS_INDEX, static_cast<uint8_t>(rpg_flags() | bit));
}

static bool rpg_invalid(void)
{
	return ((rpg_flags() & RPG_FLAG_INVALID) != 0);
}

static bool rpg_located(void)
{
	return ((rpg_flags() & RPG_FLAG_LOCATED) != 0);
}

// Recording is disabled by EITHER verdict. Only one of them is an accusation.
static bool rpg_blocked(void)
{
	return ((rpg_flags() & (RPG_FLAG_INVALID | RPG_FLAG_ERROR)) != 0);
}

static void rpg_invalidate(void)
{
	rpg_flag_set(RPG_FLAG_INVALID);
}

static void rpg_detector_error(void)
{
	rpg_flag_set(RPG_FLAG_ERROR);
}

static void rpg_location_set(uint32_t sector, uint16_t offset)
{
	rpg_u32_write(RPG_GUARD_SECTOR_INDEX, sector);
	rpg_u16_write(RPG_GUARD_OFFSET_INDEX, offset);
	rpg_flag_set(RPG_FLAG_LOCATED);
}

// Zeroes the per-recording detector state while retaining the sector buffer.
// Called only when a new recording creates a fresh guard.
static void rpg_state_reset(void)
{
	unsigned i;

	rpg_committed_size = 0;
	rpg_last_fat_type = RPG_FAT_UNKNOWN;
	for(i = 0; i < RPG_STATE_SIZE; i++) {
		rpg_state[i] = 0;
	}
}

static uint32_t rpg_committed(void)
{
	return rpg_committed_size;
}

static void rpg_committed_set(uint32_t size)
{
	rpg_committed_size = size;
}

/// The sector buffer
/// -----------------
/// The packet shares this allocation at a fixed offset so both far pointers
/// have one segment and the packet cannot straddle a 64 KB boundary.

static uint8_t rpg_buffer[RPG_SECTOR_ALLOCATION_SIZE];

static void rpg_local_free(void)
{
}

static bool rpg_ctx_alloc(rpg_ctx_t far *ctx)
{
	ctx->sector = rpg_buffer;
	ctx->packet = (rpg_buffer + RPG_SECTOR_BUFFER_SIZE_MAX);
	return true;
}

/// DOS environment probes
/// ----------------------

static uint8_t rpg_current_drive(void)
{
	uint8_t drive;

	asm {
		mov ah, 19h
		int 21h
		mov drive, al
	}
	return drive;
}

// AX=3000h returns the major in AL and the minor in AH. The extended absolute
// read is DOS 7.10 and later; below that, FAT32 and any sector above 0xFFFF are
// simply unreachable, which the module reports rather than silently failing.
static bool rpg_extended_abs_read_available(void)
{
	uint16_t version;

	asm {
		mov ax, 3000h
		int 21h
		mov version, ax
	}
	return (
		((version & 0xFF) > 7) ||
		(((version & 0xFF) == 7) && ((version >> 8) >= 10))
	);
}

static void rpg_raw_init(rpg_ctx_t far *ctx)
{
	ctx->drive = rpg_current_drive();
	ctx->fat_type = RPG_FAT_UNKNOWN;
	ctx->extended_abs_read = rpg_extended_abs_read_available();
}

/// Raw sector reads
/// ----------------
/// Both wrappers are transcribed instruction for instruction from the
/// reference, because every clause of tools/replay/FORMAT.md:780-785 is a
/// hard-won note and the reference's code matches it exactly. Do not tidy them.

// Classic INT 25h. THE HAZARD: it returns with an EXTRA FLAGS WORD left on the
// caller's stack, and DOS/driver paths may destroy BP/SI/DI. So: capture FLAGS
// with `pushf` as the very first instruction after the interrupt, pop the extra
// word into DX and keep it only as evidence, and restore BP before touching any
// BP-relative local. The carry test uses the `pushf` word, NEVER the leftover.
static bool rpg_abs_read_small(
	uint8_t drive, uint16_t sector, uint16_t count, void far *buffer
)
{
	uint16_t dos_ax = 0;
	uint16_t dos_flags = 1; // assume carry, so a wrapper that never runs fails
	uint16_t dos_stack_flags = 0;

	_AL = drive;
	_CX = count;
	_DX = sector;
	asm {
		push bp
		push si
		push di
		push es
		push ds
		lds  bx, buffer
		int  25h
		pushf
		push ax
		pop  cx
		pop  ax
		pop  dx
		pop  ds
		pop  es
		pop  di
		pop  si
		pop  bp
		mov  dos_ax, cx
		mov  dos_flags, ax
		mov  dos_stack_flags, dx
	}
	rpg_diag_int25_flags_set(dos_flags, dos_stack_flags);
	if(dos_flags & 1) {
		rpg_diag_dos_ax_set(dos_ax);
		return false;
	}
	rpg_diag_dos_ax_set(0);
	return true;
}

// DOS 7.1 extended absolute read. Normal INT 21h convention, so NO extra stack
// word. The stack-flags diagnostic is explicitly zeroed so a dump distinguishes
// "went through 7305h" from "went through 25h".
static bool rpg_abs_read_extended(
	rpg_ctx_t far *ctx, uint32_t sector, uint16_t count, void far *buffer
)
{
	uint16_t dos_ax = 0;
	uint16_t dos_flags = 1;
	uint8_t far *packet = ctx->packet;
	void far *packet_fp;

	packet[0] = static_cast<uint8_t>(sector);
	packet[1] = static_cast<uint8_t>(sector >> 8);
	packet[2] = static_cast<uint8_t>(sector >> 16);
	packet[3] = static_cast<uint8_t>(sector >> 24);
	packet[4] = static_cast<uint8_t>(count);
	packet[5] = static_cast<uint8_t>(count >> 8);
	packet[6] = static_cast<uint8_t>(FP_OFF(buffer));
	packet[7] = static_cast<uint8_t>(FP_OFF(buffer) >> 8);
	packet[8] = static_cast<uint8_t>(FP_SEG(buffer));
	packet[9] = static_cast<uint8_t>(FP_SEG(buffer) >> 8);
	packet_fp = packet;

	_DX = static_cast<uint16_t>(ctx->drive + 1); // 1-based here: 0 = default
	_CX = 0xFFFF;
	_SI = 0; // bit 0 would select a write

	// Keep AX last. Turbo C++ uses AX to evaluate the drive expression above.
	_AX = 0x7305;
	asm {
		push bp
		push si
		push di
		push es
		push ds
		lds  bx, packet_fp
		int  21h
		pushf
		push ax
		pop  cx
		pop  ax
		pop  ds
		pop  es
		pop  di
		pop  si
		pop  bp
		mov  dos_ax, cx
		mov  dos_flags, ax
	}
	rpg_diag_int25_flags_set(dos_flags, 0);
	if(dos_flags & 1) {
		rpg_diag_dos_ax_set(dos_ax);
		return false;
	}
	rpg_diag_dos_ax_set(0);
	return true;
}

// Preserve the proven FAT12/FAT16 path. FAT32 rejects old-form INT 25h, after
// which DOS 7.1's extended absolute read handles the same request. One sector
// at a time, always.
static bool rpg_sector_read(rpg_ctx_t far *ctx, uint32_t sector)
{
	if((ctx->fat_type == RPG_FAT32) || (sector > 0xFFFFUL)) {
		if(!ctx->extended_abs_read) {
			rpg_diag_location_set(sector, 0);
			rpg_diag_code_set_if_none(RPG_EXTENDED_READ_UNAVAILABLE);
			return false;
		}
		return rpg_abs_read_extended(ctx, sector, 1, ctx->sector);
	}
	if(rpg_abs_read_small(
		ctx->drive, static_cast<uint16_t>(sector), 1, ctx->sector
	)) {
		return true;
	}
	if(!ctx->extended_abs_read) {
		rpg_diag_location_set(sector, 0);
		return false;
	}
	return rpg_abs_read_extended(ctx, sector, 1, ctx->sector); // opportunistic
}

/// Geometry helpers
/// ----------------

static uint16_t rpg_u16_at(const uint8_t far *p, unsigned offset)
{
	return static_cast<uint16_t>(
		p[offset] | (static_cast<uint16_t>(p[offset + 1]) << 8)
	);
}

static uint32_t rpg_u32_at(const uint8_t far *p, unsigned offset)
{
	return (
		static_cast<uint32_t>(p[offset + 0]) |
		(static_cast<uint32_t>(p[offset + 1]) << 8) |
		(static_cast<uint32_t>(p[offset + 2]) << 16) |
		(static_cast<uint32_t>(p[offset + 3]) << 24)
	);
}

static bool rpg_sector_size_ok(uint16_t bytes_per_sector)
{
	return (
		(bytes_per_sector == 512) || (bytes_per_sector == 1024)
	);
}

// Hard-coded, never a division: a 32-bit divide would pull LDIV@ into the
// segment.
static uint8_t rpg_power_of_two_shift(uint16_t value)
{
	switch(value) {
	case 1:    return 0;
	case 2:    return 1;
	case 4:    return 2;
	case 8:    return 3;
	case 16:   return 4;
	case 32:   return 5;
	case 64:   return 6;
	case 128:  return 7;
	case 256:  return 8;
	case 512:  return 9;
	case 1024: return 10;
	}
	return 0xFF;
}

static bool rpg_power_of_two(uint16_t value)
{
	return ((value != 0) && ((value & (value - 1)) == 0));
}

/// 8.3 names
/// ---------

static const char *rpg_basename(const char *fn)
{
	const char *p = fn;
	const char *base = fn;

	while(*p) {
		if((*p == '\\') || (*p == '/')) {
			base = (p + 1);
		}
		p++;
	}
	return base;
}

// Expands the guard basename into the 11-byte padded directory form.
static bool rpg_short_name(const char *fn, uint8_t far *out)
{
	const char *p = rpg_basename(fn);
	int i = 0;
	int j;

	for(j = 0; j < 11; j++) {
		out[j] = ' ';
	}
	while(*p && (*p != '.')) {
		if(i >= 8) {
			return false;
		}
		out[i++] = static_cast<uint8_t>(
			((*p >= 'a') && (*p <= 'z')) ? (*p - 'a' + 'A') : *p
		);
		p++;
	}
	if(i == 0) {
		return false;
	}
	if(*p == '.') {
		p++;
		i = 8;
		while(*p) {
			if(i >= 11) {
				return false;
			}
			out[i++] = static_cast<uint8_t>(
				((*p >= 'a') && (*p <= 'z')) ? (*p - 'a' + 'A') : *p
			);
			p++;
		}
	}
	return true;
}

static bool rpg_name_eq(const uint8_t far *entry, const uint8_t far *name)
{
	int i;

	for(i = 0; i < 11; i++) {
		if(entry[i] != name[i]) {
			return false;
		}
	}
	return true;
}

/// Volume geometry — the BPB parse
/// -------------------------------

static bool rpg_volume_init(rpg_ctx_t far *ctx)
{
	const uint8_t far *s;
	uint16_t reserved;
	uint8_t fat_count;
	uint16_t total_16;
	uint16_t fat_16;
	uint32_t fat_span;
	uint32_t root_base;
	uint32_t root_bytes;
	uint32_t data_sectors;
	uint32_t fat_end;
	uint16_t ext_flags;
	uint8_t active_fat = 0;
	uint8_t shift;
	unsigned i;

	if(!rpg_ctx_alloc(ctx)) {
		return false;
	}
	rpg_raw_init(ctx);
	if(!rpg_sector_read(ctx, 0)) {
		rpg_diag_code_set_if_none(RPG_BPB_READ);
		return false;
	}
	s = ctx->sector;
	ctx->bytes_per_sector = rpg_u16_at(s, 0x0B);
	ctx->sectors_per_cluster = s[0x0D];
	reserved = rpg_u16_at(s, 0x0E);
	fat_count = s[0x10];
	ctx->root_entry_count = rpg_u16_at(s, 0x11);
	total_16 = rpg_u16_at(s, 0x13);
	fat_16 = rpg_u16_at(s, 0x16);
	ctx->total_sectors = (total_16 != 0) ? total_16 : rpg_u32_at(s, 0x20);
	ctx->fat_sectors = (fat_16 != 0) ? fat_16 : rpg_u32_at(s, 0x24);

	if(
		!rpg_sector_size_ok(ctx->bytes_per_sector) ||
		!rpg_power_of_two(ctx->sectors_per_cluster) ||
		(reserved == 0) || (fat_count == 0) ||
		(ctx->total_sectors == 0) || (ctx->fat_sectors == 0)
	) {
		rpg_diag_geometry_set(
			ctx->drive, ctx->bytes_per_sector, ctx->root_entry_count, 0, 0
		);
		rpg_diag_code_set_if_none(RPG_BPB_UNSUPPORTED);
		return false;
	}

	// Accumulate the FAT span with an overflow check PER FAT.
	fat_span = 0;
	for(i = 0; i < fat_count; i++) {
		if((fat_span + ctx->fat_sectors) < fat_span) {
			rpg_diag_code_set_if_none(RPG_BPB_UNSUPPORTED);
			return false;
		}
		fat_span += ctx->fat_sectors;
	}
	root_base = (reserved + fat_span);
	if(root_base < fat_span) {
		rpg_diag_code_set_if_none(RPG_BPB_UNSUPPORTED);
		return false;
	}
	root_bytes = (static_cast<uint32_t>(ctx->root_entry_count) << 5);
	ctx->root_sectors = (
		(root_bytes + ctx->bytes_per_sector - 1) / ctx->bytes_per_sector
	);
	ctx->first_data_sector = (root_base + ctx->root_sectors);
	if(
		(ctx->first_data_sector < root_base) ||
		(ctx->first_data_sector >= ctx->total_sectors)
	) {
		rpg_diag_code_set_if_none(RPG_BPB_UNSUPPORTED);
		return false;
	}

	data_sectors = (ctx->total_sectors - ctx->first_data_sector);
	shift = rpg_power_of_two_shift(ctx->sectors_per_cluster);
	ctx->cluster_count = (data_sectors >> shift);
	ctx->fat_start = reserved;
	ctx->root_cluster = 0;

	if(ctx->cluster_count < RPG_FAT12_CLUSTER_COUNT_MAX) {
		ctx->fat_type = RPG_FAT12;
	} else if(ctx->cluster_count < RPG_FAT16_CLUSTER_COUNT_MAX) {
		ctx->fat_type = RPG_FAT16;
	} else {
		ctx->fat_type = RPG_FAT32;
	}
	rpg_last_fat_type = ctx->fat_type;

	if(ctx->fat_type == RPG_FAT32) {
		ext_flags = rpg_u16_at(s, 0x28);
		if(ext_flags & 0x0080) {
			active_fat = static_cast<uint8_t>(ext_flags & 0x000F);
		}
		ctx->root_cluster = (rpg_u32_at(s, 0x2C) & RPG_FAT32_ENTRY_MASK);
		if(
			(ctx->root_entry_count != 0) || (fat_16 != 0) ||
			!ctx->extended_abs_read || // FAT32 needs DOS >= 7.10; no fallback
			(active_fat >= fat_count) ||
			(ctx->cluster_count >= RPG_FAT32_CLUSTER_MAX) ||
			(ctx->root_cluster < 2) ||
			(ctx->root_cluster > (ctx->cluster_count + 1))
		) {
			rpg_diag_code_set_if_none(RPG_BPB_UNSUPPORTED);
			return false;
		}
		ctx->fat_start += (ctx->fat_sectors * active_fat);
		ctx->root_start = (
			ctx->first_data_sector +
			((ctx->root_cluster - 2) * ctx->sectors_per_cluster)
		);
		ctx->root_sectors = 0;
	} else {
		if(
			(ctx->root_entry_count == 0) || (fat_16 == 0) ||
			(ctx->root_sectors == 0)
		) {
			rpg_diag_code_set_if_none(RPG_BPB_UNSUPPORTED);
			return false;
		}
		ctx->root_start = root_base;
	}

	// Set BEFORE the range check, so a range failure still carries numbers.
	rpg_diag_geometry_set(
		ctx->drive, ctx->bytes_per_sector, ctx->root_entry_count,
		ctx->root_start, static_cast<uint16_t>(ctx->root_sectors)
	);
	fat_end = (ctx->fat_start + ctx->fat_sectors);
	if(
		(ctx->root_start >= ctx->total_sectors) ||
		(fat_end > ctx->total_sectors)
	) {
		rpg_diag_code_set_if_none(RPG_ROOT_RANGE);
		return false;
	}
	return true;
}

static bool rpg_cluster_sector(
	rpg_ctx_t far *ctx, uint32_t cluster, uint32_t far *sector
)
{
	uint32_t cluster_end;

	if((cluster < 2) || (cluster > (ctx->cluster_count + 1))) {
		return false;
	}
	*sector = (
		ctx->first_data_sector + ((cluster - 2) * ctx->sectors_per_cluster)
	);
	cluster_end = (*sector + ctx->sectors_per_cluster);
	return (
		(*sector >= ctx->first_data_sector) && (cluster_end >= *sector) &&
		(cluster_end <= ctx->total_sectors)
	);
}

/// The root-directory scan
/// -----------------------
/// NOTE on the attribute mask: an LFN entry has attr 0x0F, which sets both the
/// volume-label and the directory bit, so the mask skips LFN entries as a side
/// effect. That is correct by accident rather than by intent — do not "improve"
/// the mask.

static int rpg_root_sector_scan(
	rpg_ctx_t far *ctx, uint32_t sector_index, const uint8_t far *name,
	uint32_t far *size, bool cache_location
)
{
	uint16_t offset;
	const uint8_t far *e;

	for(
		offset = 0;
		offset < ctx->bytes_per_sector;
		offset = static_cast<uint16_t>(offset + RPG_DIR_ENTRY_SIZE)
	) {
		e = (ctx->sector + offset);
		if(e[0] == RPG_FAT_ENTRY_FREE) {
			rpg_diag_location_set(sector_index, offset);
			return RPG_SCAN_END; // end of directory: it is not here
		}
		if(e[0] == RPG_FAT_ENTRY_DELETED) {
			continue;
		}
		if(e[0x0B] & RPG_FAT_ATTR_SKIP) {
			continue;
		}
		if(rpg_name_eq(e, name)) {
			*size = rpg_u32_at(e, 0x1C);
			rpg_diag_location_set(sector_index, offset);
			if(cache_location) {
				rpg_location_set(sector_index, offset);
			}
			return RPG_SCAN_FOUND;
		}
	}
	return RPG_SCAN_CONTINUE;
}

static bool rpg_fat32_next_cluster(
	rpg_ctx_t far *ctx, uint32_t cluster, uint32_t far *next, bool far *eoc
)
{
	uint32_t fat_offset = (cluster << 2);
	uint32_t fat_sector;
	uint16_t offset;

	fat_sector = (
		ctx->fat_start +
		(fat_offset >> rpg_power_of_two_shift(ctx->bytes_per_sector))
	);
	offset = static_cast<uint16_t>(fat_offset & (ctx->bytes_per_sector - 1));
	if(
		(fat_sector < ctx->fat_start) ||
		(fat_sector >= (ctx->fat_start + ctx->fat_sectors)) ||
		(offset > (ctx->bytes_per_sector - 4))
	) {
		rpg_diag_location_set(fat_sector, offset);
		rpg_diag_code_set_if_none(RPG_ROOT_CHAIN);
		return false;
	}
	if(!rpg_sector_read(ctx, fat_sector)) {
		rpg_diag_location_set(fat_sector, offset);
		rpg_diag_code_set_if_none(RPG_ROOT_FAT_READ);
		return false;
	}
	*next = (rpg_u32_at(ctx->sector, offset) & RPG_FAT32_ENTRY_MASK);
	*eoc = (*next >= RPG_FAT32_CLUSTER_EOC);
	if(*eoc) {
		return true;
	}
	if(
		(*next < 2) || (*next == RPG_FAT32_CLUSTER_BAD) ||
		(*next > (ctx->cluster_count + 1))
	) {
		rpg_diag_sizes_set(cluster, *next);
		rpg_diag_code_set_if_none(RPG_ROOT_CHAIN);
		return false;
	}
	return true;
}

static bool rpg_root_size_read(const char *fn, uint32_t far *size)
{
	rpg_ctx_t ctx;
	uint8_t name[11];
	uint32_t sector;
	uint32_t cluster;
	uint32_t next;
	uint32_t clusters_walked = 0;
	uint32_t i;
	bool eoc;
	int scan;

	if(!rpg_short_name(fn, name)) {
		rpg_diag_code_set_if_none(RPG_SHORT_NAME);
		return false;
	}
	if(!rpg_volume_init(&ctx)) {
		return false;
	}
	if(ctx.fat_type != RPG_FAT32) {
		for(i = 0; i < ctx.root_sectors; i++) {
			if(!rpg_sector_read(&ctx, ctx.root_start + i)) {
				rpg_diag_code_set_if_none(RPG_ROOT_SECTOR_READ);
				return false;
			}
			scan = rpg_root_sector_scan(
				&ctx, (ctx.root_start + i), name, size, true
			);
			if(scan == RPG_SCAN_FOUND) {
				return true;
			}
			if(scan == RPG_SCAN_END) {
				rpg_diag_code_set_if_none(RPG_ROOT_NOT_FOUND);
				return false;
			}
		}
		rpg_diag_code_set_if_none(RPG_ROOT_NOT_FOUND);
		return false;
	}

	cluster = ctx.root_cluster;
	for(;;) {
		if(!rpg_cluster_sector(&ctx, cluster, &sector)) {
			rpg_diag_sizes_set(2, cluster);
			rpg_diag_code_set_if_none(RPG_ROOT_CHAIN);
			return false;
		}
		for(i = 0; i < ctx.sectors_per_cluster; i++) {
			if(!rpg_sector_read(&ctx, (sector + i))) {
				rpg_diag_location_set((sector + i), 0);
				rpg_diag_code_set_if_none(RPG_ROOT_SECTOR_READ);
				return false;
			}
			scan = rpg_root_sector_scan(
				&ctx, (sector + i), name, size, true
			);
			if(scan == RPG_SCAN_FOUND) {
				return true;
			}
			if(scan == RPG_SCAN_END) {
				rpg_diag_code_set_if_none(RPG_ROOT_NOT_FOUND);
				return false;
			}
		}
		clusters_walked++;

		// THE LOOP GUARD, and it is checked BEFORE the FAT read. A chain longer
		// than the volume's cluster count is a cycle or a corrupt FAT. This is
		// the module's only unbounded loop and this is what bounds it.
		if(clusters_walked >= ctx.cluster_count) {
			rpg_diag_sizes_set(ctx.cluster_count, clusters_walked);
			rpg_diag_code_set_if_none(RPG_ROOT_CHAIN);
			return false;
		}
		if(!rpg_fat32_next_cluster(&ctx, cluster, &next, &eoc)) {
			return false;
		}
		if(eoc) {
			break;
		}
		cluster = next;
	}
	rpg_diag_code_set_if_none(RPG_ROOT_NOT_FOUND);
	return false;
}

// The fast path. Fully revalidates the cache before trusting it, and does NOT
// run volume_init — no BPB parse, so [fat_type] stays UNKNOWN and sector_read
// takes the INT 25h path unless the sector is above 0xFFFF. The offset bound is
// the compile-time buffer maximum, not the volume's sector size, because the
// geometry has deliberately not been read.
static bool rpg_located_size_read(const char *fn, uint32_t far *size)
{
	rpg_ctx_t ctx;
	uint8_t name[11];
	uint32_t sector = rpg_u32(RPG_GUARD_SECTOR_INDEX);
	uint16_t offset = rpg_u16(RPG_GUARD_OFFSET_INDEX);
	const uint8_t far *e;

	if(
		!rpg_located() || (sector == 0) ||
		(offset > (RPG_SECTOR_BUFFER_SIZE_MAX - RPG_DIR_ENTRY_SIZE)) ||
		((offset & 0x1F) != 0)
	) {
		rpg_diag_code_set_if_none(RPG_LOCATED_RANGE);
		return false;
	}
	if(!rpg_short_name(fn, name)) {
		rpg_diag_code_set_if_none(RPG_SHORT_NAME);
		return false;
	}
	if(!rpg_ctx_alloc(&ctx)) {
		return false;
	}
	rpg_raw_init(&ctx);
	if(!rpg_sector_read(&ctx, sector)) {
		rpg_diag_code_set_if_none(RPG_LOCATED_SECTOR_READ);
		return false;
	}
	e = (ctx.sector + offset);
	if(
		(e[0] == RPG_FAT_ENTRY_FREE) || (e[0] == RPG_FAT_ENTRY_DELETED) ||
		(e[0x0B] & RPG_FAT_ATTR_SKIP)
	) {
		rpg_diag_code_set_if_none(RPG_LOCATED_ENTRY_BAD);
		return false;
	}
	if(!rpg_name_eq(e, name)) {
		rpg_diag_code_set_if_none(RPG_LOCATED_NAME);
		return false;
	}
	*size = rpg_u32_at(e, 0x1C);
	rpg_diag_location_set(sector, offset);
	return true;
}

static bool rpg_size_read(const char *fn, uint32_t far *size)
{
	if(rpg_located() && rpg_located_size_read(fn, size)) {
		return true;
	}
	if(!rpg_root_size_read(fn, size)) {
		return false;
	}

	// A cached probe that missed is RECOVERABLE. Without this reset the
// RPG_LOCATED_* code stays in the state block and the next _set_if_none
	// declines to overwrite it, mislabelling a later, real failure.
	rpg_diag_code_set(RPG_NONE);
	return true;
}

// Reads the guard's marker byte — byte 0 of its first data cluster — and its
// directory size, both RAW. This is the read DOS cannot fake, and it is the
// whole detector.
static bool rpg_guard_marker_read(
	const char *fn, uint8_t far *marker, uint32_t far *disk_size
)
{
	rpg_ctx_t ctx;
	uint8_t name[11];
	uint32_t sector;
	uint32_t cluster;
	uint32_t data_sector;
	uint16_t offset;
	const uint8_t far *e;

	if(!rpg_located()) {
		if(!rpg_root_size_read(fn, disk_size)) { // primes the cache
			return false;
		}
	}
	if(!rpg_short_name(fn, name)) {
		rpg_diag_code_set_if_none(RPG_SHORT_NAME);
		return false;
	}
	if(!rpg_volume_init(&ctx)) { // needs the geometry to map cluster->sector
		return false;
	}
	sector = rpg_u32(RPG_GUARD_SECTOR_INDEX);
	offset = rpg_u16(RPG_GUARD_OFFSET_INDEX);
	if(
		!rpg_located() || (sector == 0) ||
		(offset > (ctx.bytes_per_sector - RPG_DIR_ENTRY_SIZE))
	) {
		rpg_diag_code_set_if_none(RPG_LOCATED_RANGE);
		return false;
	}
	if(!rpg_sector_read(&ctx, sector)) {
		rpg_diag_code_set_if_none(RPG_LOCATED_SECTOR_READ);
		return false;
	}
	e = (ctx.sector + offset);
	if(
		(e[0] == RPG_FAT_ENTRY_FREE) || (e[0] == RPG_FAT_ENTRY_DELETED) ||
		(e[0x0B] & RPG_FAT_ATTR_SKIP)
	) {
		rpg_diag_code_set_if_none(RPG_LOCATED_ENTRY_BAD);
		return false;
	}
	if(!rpg_name_eq(e, name)) {
		rpg_diag_code_set_if_none(RPG_LOCATED_NAME);
		return false;
	}
	*disk_size = rpg_u32_at(e, 0x1C);
	if(*disk_size < RPG_GUARD_BASE_SIZE) {
		rpg_diag_code_set_if_none(RPG_MARKER_CLUSTER);
		return false;
	}
	cluster = rpg_u16_at(e, 0x1A);
	if(ctx.fat_type == RPG_FAT32) {
		cluster |= (static_cast<uint32_t>(rpg_u16_at(e, 0x14)) << 16);
		cluster &= RPG_FAT32_ENTRY_MASK;
	}
	if(!rpg_cluster_sector(&ctx, cluster, &data_sector)) {
		rpg_diag_sizes_set(2, cluster);
		rpg_diag_code_set_if_none(RPG_MARKER_CLUSTER);
		return false;
	}
	if(!rpg_sector_read(&ctx, data_sector)) {
		rpg_diag_code_set_if_none(RPG_MARKER_SECTOR_READ);
		return false;
	}
	*marker = ctx.sector[0];
	return true;
}

/// The DOS commit
/// --------------
/// INT 21h AX=5D01h, "commit all files for this process": it flushes DOS's
/// internal buffers AND updates the directory entry for every handle owned by
/// this PSP, WITHOUT closing the handle. That is what makes a freshly written
/// size visible to a raw sector read while the file is still open — the reason
/// the whole detector works.

static uint16_t rpg_current_psp(void)
{
	uint16_t psp;

	asm {
		mov ah, 51h // undocumented, as the reference uses
		int 21h
		mov psp, bx
	}
	return psp;
}

static bool rpg_commit_process(void)
{
	uint16_t dpl[11];
	uint16_t dos_ax = 0;
	uint16_t dos_flags = 1;
	int i;

	for(i = 0; i < 11; i++) {
		dpl[i] = 0;
	}
	dpl[10] = rpg_current_psp();

	// The DPL is a stack local and this memory model has DS != SS, so DS:DX
	// must be made SS:offset before the call. Without `push ss / pop ds` the
	// `lea dx` offset would be interpreted against the wrong segment.
	asm {
		push bp
		push si
		push di
		push es
		push ds
		push ss
		pop  ds
		lea  dx, dpl
		mov  ax, 5D01h
		int  21h
		pushf
		push ax
		pop  cx
		pop  ax
		pop  ds
		pop  es
		pop  di
		pop  si
		pop  bp
		mov  dos_ax, cx
		mov  dos_flags, ax
	}
	if(dos_flags & 1) {
		rpg_diag_dos_ax_set(dos_ax);
		return false;
	}
	return true;
}

// close() + a second commit, so directory-entry updates made by the close are
// not left only in DOS's buffers.
static bool rpg_close_and_commit(int fd, uint8_t code)
{
	replay_protect_file_close(fd);
	if(!rpg_commit_process()) {
		rpg_diag_code_set_if_none(RPG_CLOSE_COMMIT);
		rpg_detector_error();
		return false;
	}
	(void)code;
	return true;
}

/// The guard file
/// --------------

static bool rpg_guard_create(const char *fn)
{
	uint8_t marker = 0;
	uint8_t disk_marker;
	uint32_t disk_size;
	int fd;

	rpg_state_reset();
	fd = replay_protect_file_create(fn);
	if(fd < 0) {
		rpg_diag_code_set_if_none(RPG_GUARD_CREATE);
		rpg_detector_error();
		return false;
	}
	if(!replay_protect_file_write(fd, &marker, 1)) {
		replay_protect_file_close(fd);
		rpg_diag_code_set_if_none(RPG_MARKER_WRITE);
		rpg_detector_error();
		return false;
	}
	(void)rpg_commit_process();
	if(!rpg_close_and_commit(fd, RPG_MARKER_WRITE)) {
		return false;
	}
	if(!rpg_guard_marker_read(fn, &disk_marker, &disk_size)) {
		rpg_detector_error();
		return false;
	}
	if(disk_size != RPG_GUARD_BASE_SIZE) {
		rpg_diag_sizes_set(RPG_GUARD_BASE_SIZE, disk_size);
		rpg_diag_code_set_if_none(RPG_GUARD_CREATE_NONZERO);
		rpg_detector_error();
		return false;
	}
	if(disk_marker != 0) {
		rpg_diag_sizes_set(0, disk_marker);
		rpg_diag_code_set_if_none(RPG_MARKER_VERIFY);
		rpg_detector_error();
		return false;
	}
	rpg_committed_set(RPG_GUARD_BASE_SIZE);
	return true;
}

/// THE THREE-WAY VERDICT
/// ---------------------
/// Rows 3 and 4 share RPG_VERIFY_MISMATCH and the same argument order. What
/// separates an accusation from an I/O failure is the FLAG BIT and the SIGN of
/// (actual - expected). The acceptance procedure records both values and the
/// INVALID/ERROR distinction.

// The ladder itself, factored out of the read so it can be driven directly.
// That is not a testing convenience bolted on: rows 3 and 4 differ only by a
// flag bit and the sign of (actual - expected), and a verdict that can only be
// reached through a successful raw disk read cannot be exercised at all on a
// host that has no FAT image.
static bool rpg_verdict(uint8_t marker, uint32_t disk_size, uint32_t committed)
{
	if(marker != 0) {
		// Row 2: a previous run already poisoned this guard — a repeated load.
		rpg_diag_sizes_set(0, marker);
		rpg_diag_code_set(RPG_MARKER_VALUE);
		rpg_invalidate();
		return false;
	}
	if(disk_size > committed) {
		// Row 3: THE DISK IS AHEAD OF RAM, so RAM was rolled back.
		rpg_diag_sizes_set(committed, disk_size);
		rpg_diag_code_set(RPG_VERIFY_MISMATCH);
		rpg_invalidate();
		return false;
	}
	if(disk_size < committed) {
		// Row 4: a write failed. Same code, same order — but ERROR, not
		// INVALID, and actual < expected. NOT an accusation.
		rpg_diag_sizes_set(committed, disk_size);
		rpg_diag_code_set(RPG_VERIFY_MISMATCH);
		rpg_detector_error();
		return false;
	}
	return true; // row 5: OK
}

static bool rpg_guard_marker_verify(const char *fn)
{
	uint8_t marker;
	uint32_t disk_size;

	if(rpg_blocked()) {
		return false; // already blocked; short-circuit, no new evidence
	}
	if(!rpg_guard_marker_read(fn, &marker, &disk_size)) {
		rpg_detector_error(); // row 1: I/O or parse failure
		return false;
	}
	return rpg_verdict(marker, disk_size, rpg_committed());
}

// THE POISON. Writes marker byte 1 into the guard's first DATA byte.
//
// The lseek(0) is what makes it durable and it must not be "simplified" into an
// append: seeking to 0 and overwriting neither extends the file nor allocates,
// so the write stays inside the already-allocated first cluster and touches no
// FAT sector. An append would allocate from a FAT the savestate is about to
// restore, and the poison would vanish with it.
static void rpg_guard_marker_set(const char *fn)
{
	uint8_t marker = 1;
	uint8_t disk_marker;
	uint32_t disk_size;
	int fd;

	fd = replay_protect_file_update(fn);
	if(fd < 0) {
		rpg_diag_code_set_if_none(RPG_MARKER_WRITE);
		rpg_detector_error();
		return;
	}
	if(!replay_protect_file_seek(fd, 0)) {
		replay_protect_file_close(fd);
		rpg_diag_code_set_if_none(RPG_MARKER_WRITE);
		rpg_detector_error();
		return;
	}
	if(!replay_protect_file_write(fd, &marker, 1)) {
		replay_protect_file_close(fd);
		rpg_diag_code_set_if_none(RPG_MARKER_WRITE);
		rpg_detector_error();
		return;
	}
	(void)rpg_commit_process();
	if(!rpg_close_and_commit(fd, RPG_MARKER_WRITE)) {
		return;
	}
	if(!rpg_guard_marker_read(fn, &disk_marker, &disk_size)) {
		rpg_detector_error();
		return;
	}
	if(disk_marker != 1) {
		rpg_diag_sizes_set(1, disk_marker);
		rpg_diag_code_set_if_none(RPG_MARKER_VERIFY);
		rpg_detector_error();
		return;
	}

	// NOT bookkeeping — this is the accusation. Leaving the diagnostic that
	// reads as a detection, rather than one that reads as "poison written
	// successfully", is what makes the evidence legible afterwards.
	rpg_diag_sizes_set(0, disk_marker);
	rpg_diag_code_set(RPG_MARKER_VALUE);
}

// One checkpoint: verify FIRST, then append exactly one byte and prove the disk
// agrees.
static bool rpg_checkpoint(const char *fn)
{
	uint32_t expected;
	uint32_t disk_size;
	bool committed_open;
	uint8_t byte = 0;
	int fd;

	if(!rpg_guard_marker_verify(fn)) {
		return false; // flags and diagnostics already set by the verdict
	}
	expected = (rpg_committed() + 1);
	fd = replay_protect_file_update(fn);
	if(fd < 0) {
		rpg_diag_code_set_if_none(RPG_CHECKPOINT_APPEND);
		rpg_detector_error();
		return false;
	}
	if(!replay_protect_file_seek(fd, rpg_committed())) {
		replay_protect_file_close(fd);
		rpg_diag_code_set_if_none(RPG_CHECKPOINT_APPEND);
		rpg_detector_error();
		return false;
	}
	if(!replay_protect_file_write(fd, &byte, 1)) {
		replay_protect_file_close(fd);
		rpg_diag_code_set_if_none(RPG_CHECKPOINT_WRITE);
		rpg_detector_error();
		return false;
	}

	// Commit WHILE THE HANDLE IS STILL OPEN, then read the size raw.
	committed_open = false;
	if(rpg_commit_process()) {
		if(rpg_size_read(fn, &disk_size)) {
			committed_open = (disk_size == expected);
		}
	}
	if(!rpg_close_and_commit(fd, RPG_CHECKPOINT_WRITE)) {
		return false;
	}
	if(!committed_open) {
		if(!rpg_size_read(fn, &disk_size)) {
			rpg_detector_error();
			return false;
		}
		if(disk_size != expected) {
			rpg_diag_sizes_set(expected, disk_size);
			rpg_diag_code_set_if_none(RPG_CHECKPOINT_MISMATCH);
			rpg_detector_error();
			return false;
		}
	}
	rpg_committed_set(expected);
	rpg_diag_dos_ax_set(0);
	return true;
}

static void replay_protect_paths_init(void)
{
	replay_protect_guard_fn[0] = '\\';
	replay_protect_guard_fn[1] = 'T';
	replay_protect_guard_fn[2] = static_cast<char>('0' + GAME);
	replay_protect_guard_fn[3] = 'L';
	replay_protect_guard_fn[4] = 'A';
	replay_protect_guard_fn[5] = 'S';
	replay_protect_guard_fn[6] = 'T';
	replay_protect_guard_fn[7] = '.';
	replay_protect_guard_fn[8] = 'G';
	replay_protect_guard_fn[9] = 'R';
	replay_protect_guard_fn[10] = 'D';
	replay_protect_guard_fn[11] = '\0';

	replay_protect_diag_fn[0] = 'T';
	replay_protect_diag_fn[1] = static_cast<char>('0' + GAME);
	replay_protect_diag_fn[2] = 'G';
	replay_protect_diag_fn[3] = 'D';
	replay_protect_diag_fn[4] = 'I';
	replay_protect_diag_fn[5] = 'A';
	replay_protect_diag_fn[6] = 'G';
	replay_protect_diag_fn[7] = '.';
	replay_protect_diag_fn[8] = 'B';
	replay_protect_diag_fn[9] = 'I';
	replay_protect_diag_fn[10] = 'N';
	replay_protect_diag_fn[11] = '\0';
}

static void replay_protect_diag_write(void)
{
	replay_protect_diag_record_t diag;
	int fd;

	diag.magic[0] = 'T';
	diag.magic[1] = static_cast<char>('0' + GAME);
	diag.magic[2] = 'G';
	diag.magic[3] = 'D';
	diag.magic[4] = '1';
	diag.magic[5] = '\0';
	diag.magic[6] = '\0';
	diag.magic[7] = '\0';
	diag.code = rpg_u8(
		RPG_DIAG_CODE_INDEX
	);
	diag.flags = rpg_flags();
	diag.drive = rpg_u8(
		RPG_DIAG_DRIVE_INDEX
	);
	diag.fat_type = rpg_last_fat_type;
	diag.dos_ax = rpg_u16(
		RPG_DIAG_DOS_AX_INDEX
	);
	diag.bytes_per_sector = rpg_u16(
		RPG_DIAG_BPS_INDEX
	);
	diag.root_entries = rpg_u16(
		RPG_DIAG_ROOT_ENTS_INDEX
	);
	diag.root_start = rpg_u32(
		RPG_DIAG_ROOT_START_INDEX
	);
	diag.root_sectors = rpg_u16(
		RPG_DIAG_ROOT_SECS_INDEX
	);
	diag.sector = rpg_u32(
		RPG_DIAG_SECTOR_INDEX
	);
	diag.offset = rpg_u16(
		RPG_DIAG_OFFSET_INDEX
	);
	diag.expected = rpg_u32(
		RPG_DIAG_EXPECTED_INDEX
	);
	diag.actual = rpg_u32(
		RPG_DIAG_ACTUAL_INDEX
	);
	diag.int25_flags = rpg_u16(
		RPG_DIAG_I25_FLAGS_INDEX
	);
	diag.int25_stack_flags = rpg_u16(
		RPG_DIAG_I25_STACK_INDEX
	);

	fd = replay_protect_file_create(replay_protect_diag_fn);
	if(fd < 0) {
		return;
	}
	(void)replay_protect_file_write(fd, &diag, sizeof(diag));
	(void)rpg_commit_process();
	replay_protect_file_close(fd);
	(void)rpg_commit_process();
}

bool replay_protect_begin(void)
{
	replay_protect_paths_init();
	rpg_local_free();
	rpg_state_reset();
	replay_protect_file_delete(replay_protect_diag_fn);
	replay_protect_file_delete(replay_protect_guard_fn);
	(void)rpg_commit_process();
	if(!rpg_guard_create(replay_protect_guard_fn)) {
		replay_protect_diag_write();
		return false;
	}
	return true;
}

bool replay_protect_checkpoint(void)
{
	if(rpg_blocked()) {
		return false;
	}
	if(!rpg_checkpoint(replay_protect_guard_fn)) {
		if(rpg_invalid()) {
			rpg_guard_marker_set(replay_protect_guard_fn);
		}
		replay_protect_diag_write();
		return false;
	}
	return true;
}

bool replay_protect_blocked(void)
{
	return rpg_blocked();
}

void replay_protect_end(void)
{
	if(rpg_blocked()) {
		replay_protect_diag_write();
	}
	replay_protect_file_delete(replay_protect_guard_fn);
	(void)rpg_commit_process();
	rpg_local_free();
}

// Keep the following CRT segment at its accepted paragraph phase.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
