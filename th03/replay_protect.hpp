#ifndef TH03_REPLAY_PROTECT_HPP
#define TH03_REPLAY_PROTECT_HPP

#include "libs/master.lib/master.hpp"
#include "platform.h"
#include "th03/replay_handoff.hpp"
#include "x86real.h"

extern "C" void MASTER_RET file_flush(void);
extern "C" int __cdecl file_ErrorStat;

#define RP_FAT_ENTRY_FREE 0x00
#define RP_FAT_ENTRY_DELETED 0xE5
#define RP_FAT_ATTR_VOLUME 0x08
#define RP_FAT_ATTR_DIR 0x10
#define RP_SECTOR_BUFFER_SIZE_MAX 4096
#define RP_ABS_READ_PACKET_SIZE 10
#define RP_SECTOR_ALLOCATION_SIZE \
	(RP_SECTOR_BUFFER_SIZE_MAX + RP_ABS_READ_PACKET_SIZE)
#define RP_DOS_PARAMETER_LIST_WORDS 11
#define RP_DOS_PARAMETER_LIST_PROCESS_ID 10
#define RP_GUARD_BASE_SIZE 1UL
#define RP_GUARD_MARKER_CLEAR 0
#define RP_GUARD_MARKER_SET 1
#define RP_FAT12_CLUSTER_COUNT_MAX 4085UL
#define RP_FAT16_CLUSTER_COUNT_MAX 65525UL
#define RP_FAT32_CLUSTER_MAX 0x0FFFFFEFUL
#define RP_FAT32_ENTRY_MASK 0x0FFFFFFFUL
#define RP_FAT32_CLUSTER_BAD 0x0FFFFFF7UL
#define RP_FAT32_CLUSTER_EOC 0x0FFFFFF8UL

enum replay_protect_fat_t {
	RPF_UNKNOWN = 0,
	RPF_FAT12 = 12,
	RPF_FAT16 = 16,
	RPF_FAT32 = 32,
};

enum replay_protect_diag_t {
	RPD_NONE = 0,
	RPD_CTX_ALLOC = 1,
	RPD_BPB_READ = 2,
	RPD_BPB_UNSUPPORTED = 3,
	RPD_ROOT_RANGE = 4,
	RPD_SHORT_NAME = 5,
	RPD_ROOT_SECTOR_READ = 6,
	RPD_ROOT_NOT_FOUND = 7,
	RPD_LOCATED_RANGE = 8,
	RPD_LOCATED_SECTOR_READ = 9,
	RPD_LOCATED_ENTRY_BAD = 10,
	RPD_LOCATED_NAME = 11,
	RPD_VERIFY_SIZE_READ = 12,
	RPD_VERIFY_MISMATCH = 13,
	RPD_COMMIT_FLUSH = 14,
	RPD_COMMIT_DOS = 15,
	RPD_GUARD_CREATE = 16,
	RPD_GUARD_CREATE_SIZE_READ = 17,
	RPD_GUARD_CREATE_NONZERO = 18,
	RPD_CHECKPOINT_APPEND = 19,
	RPD_CHECKPOINT_WRITE = 20,
	RPD_CHECKPOINT_SIZE_READ = 21,
	RPD_CHECKPOINT_MISMATCH = 22,
	RPD_SECTOR_TOO_LARGE = 23,
	RPD_LATCH_CREATE = 24,
	RPD_LATCH_CREATE_SIZE_READ = 25,
	RPD_LATCH_CREATE_MISMATCH = 26,
	RPD_LATCH_SIZE_READ = 27,
	RPD_LATCH_SET = 28,
	RPD_LATCH_WRITE = 29,
	RPD_LATCH_MISMATCH = 30,
	RPD_CLOSE_COMMIT = 31,
	RPD_MAINL_HEADER_WRITE = 32,
	RPD_MAINL_BUFFER_ALLOC = 33,
	RPD_MAINL_BUFFER_FULL = 34,
	RPD_MAIN_USER_READ = 35,
	RPD_MAIN_RECORD_IO = 36,
	RPD_MAIN_PAUSE_FLUSH = 37,
	RPD_MAINL_PERIODIC_WRITE = 38,
	RPD_MARKER_CLUSTER = 39,
	RPD_MARKER_SECTOR_READ = 40,
	RPD_MARKER_VALUE = 41,
	RPD_MARKER_WRITE = 42,
	RPD_MARKER_VERIFY = 43,
	RPD_EXTENDED_READ_UNAVAILABLE = 44,
	RPD_ROOT_CHAIN = 45,
	RPD_ROOT_FAT_READ = 46,
};

struct replay_protect_ctx_t {
	uint8_t far *sector;
	uint8_t far *packet;
	uint16_t bytes_per_sector;
	uint16_t root_entry_count;
	uint32_t root_start;
	uint16_t root_sectors;
	uint32_t fat_start;
	uint32_t fat_sectors;
	uint32_t first_data_sector;
	uint32_t total_sectors;
	uint32_t cluster_count;
	uint32_t root_cluster;
	uint8_t drive;
	uint8_t sectors_per_cluster;
	uint8_t fat_type;
	uint8_t extended_abs_read;
};

static uint16_t replay_protect_u16(const uint8_t far *p)
{
	return static_cast<uint16_t>(
		p[0] | (static_cast<uint16_t>(p[1]) << 8)
	);
}

static uint32_t replay_protect_u32(const uint8_t far *p)
{
	return (
		static_cast<uint32_t>(replay_protect_u16(p)) |
		(static_cast<uint32_t>(replay_protect_u16(p + 2)) << 16)
	);
}

static void replay_protect_u16_write(uint8_t far *p, uint16_t value)
{
	p[0] = static_cast<uint8_t>(value);
	p[1] = static_cast<uint8_t>(value >> 8);
}

static void replay_protect_u32_write(uint8_t far *p, uint32_t value)
{
	p[0] = static_cast<uint8_t>(value);
	p[1] = static_cast<uint8_t>(value >> 8);
	p[2] = static_cast<uint8_t>(value >> 16);
	p[3] = static_cast<uint8_t>(value >> 24);
}

static uint32_t replay_protect_mul_u32_u8(uint32_t a, uint8_t b)
{
	uint32_t ret = 0;

	while(b != 0) {
		if(b & 1) {
			ret += a;
		}
		a <<= 1;
		b >>= 1;
	}
	return ret;
}

static uint8_t replay_protect_handoff_u8(unsigned index)
{
	return static_cast<uint8_t>(resident->unused_3[index]);
}

static uint32_t replay_protect_handoff_u32_read(unsigned index)
{
	return (
		static_cast<uint32_t>(replay_protect_handoff_u8(index)) |
		(static_cast<uint32_t>(replay_protect_handoff_u8(index + 1)) << 8) |
		(static_cast<uint32_t>(replay_protect_handoff_u8(index + 2)) << 16) |
		(static_cast<uint32_t>(replay_protect_handoff_u8(index + 3)) << 24)
	);
}

static void replay_protect_handoff_u32_write(unsigned index, uint32_t value)
{
	resident->unused_3[index + 0] = static_cast<uint8_t>(value);
	resident->unused_3[index + 1] = static_cast<uint8_t>(value >> 8);
	resident->unused_3[index + 2] = static_cast<uint8_t>(value >> 16);
	resident->unused_3[index + 3] = static_cast<uint8_t>(value >> 24);
}

static uint16_t replay_protect_handoff_u16_read(unsigned index)
{
	return static_cast<uint16_t>(
		replay_protect_handoff_u8(index) |
		(static_cast<uint16_t>(replay_protect_handoff_u8(index + 1)) << 8)
	);
}

static void replay_protect_handoff_u16_write(unsigned index, uint16_t value)
{
	resident->unused_3[index + 0] = static_cast<uint8_t>(value);
	resident->unused_3[index + 1] = static_cast<uint8_t>(value >> 8);
}

static void replay_protect_diag_code_set(uint8_t code)
{
	resident->unused_3[T3R_DIAG_CODE_INDEX] = code;
}

static void replay_protect_diag_dos_ax_set(uint16_t value)
{
	replay_protect_handoff_u16_write(
		T3R_DIAG_DOS_AX_INDEX, value
	);
}

static void replay_protect_diag_int25_flags_set(
	uint16_t flags, uint16_t stack_flags
)
{
	replay_protect_handoff_u16_write(T3R_DIAG_INT25_FLAGS_INDEX, flags);
	replay_protect_handoff_u16_write(
		T3R_DIAG_INT25_STACK_FLAGS_INDEX, stack_flags
	);
}

static void replay_protect_diag_geometry_set(
	uint8_t drive, uint16_t bytes_per_sector, uint16_t root_entry_count,
	uint32_t root_start, uint16_t root_sectors
)
{
	resident->unused_3[T3R_DIAG_DRIVE_INDEX] = drive;
	replay_protect_handoff_u16_write(
		T3R_DIAG_BYTES_SECTOR_INDEX, bytes_per_sector
	);
	replay_protect_handoff_u16_write(
		T3R_DIAG_ROOT_ENTRIES_INDEX, root_entry_count
	);
	replay_protect_handoff_u32_write(
		T3R_DIAG_ROOT_START_INDEX, root_start
	);
	replay_protect_handoff_u16_write(
		T3R_DIAG_ROOT_SECTORS_INDEX, root_sectors
	);
}

static void replay_protect_diag_location_set(uint32_t sector, uint16_t offset)
{
	replay_protect_handoff_u32_write(
		T3R_DIAG_SECTOR_INDEX, sector
	);
	replay_protect_handoff_u16_write(
		T3R_DIAG_OFFSET_INDEX, offset
	);
}

static void replay_protect_diag_sizes_set(uint32_t expected, uint32_t actual)
{
	replay_protect_handoff_u32_write(
		T3R_DIAG_EXPECTED_INDEX, expected
	);
	replay_protect_handoff_u32_write(
		T3R_DIAG_ACTUAL_INDEX, actual
	);
}

static bool replay_protect_invalid(void)
{
	return (
		(replay_protect_handoff_u8(T3_REPLAY_RES_PROTECT_FLAGS_INDEX) &
		 T3_REPLAY_RES_PROTECT_INVALID) != 0
	);
}

static bool replay_protect_located(void)
{
	return (
		(replay_protect_handoff_u8(T3_REPLAY_RES_PROTECT_FLAGS_INDEX) &
		 T3_REPLAY_RES_PROTECT_LOCATED) != 0
	);
}

static bool replay_protect_detector_error(void)
{
	return (
		(replay_protect_handoff_u8(T3_REPLAY_RES_PROTECT_FLAGS_INDEX) &
		 T3_REPLAY_RES_PROTECT_ERROR) != 0
	);
}

static bool replay_protect_blocked(void)
{
	return (replay_protect_invalid() || replay_protect_detector_error());
}

static void replay_protect_invalidate(void)
{
	resident->unused_3[T3_REPLAY_RES_PROTECT_FLAGS_INDEX] |= (
		T3_REPLAY_RES_PROTECT_INVALID
	);
}

static void replay_protect_detector_error_set(void)
{
	resident->unused_3[T3_REPLAY_RES_PROTECT_FLAGS_INDEX] |= (
		T3_REPLAY_RES_PROTECT_ERROR
	);
}

static void replay_protect_location_set(uint32_t sector, uint16_t offset)
{
	replay_protect_handoff_u32_write(T3_REPLAY_RES_GUARD_SECTOR_INDEX, sector);
	replay_protect_handoff_u16_write(T3_REPLAY_RES_GUARD_OFFSET_INDEX, offset);
	resident->unused_3[T3_REPLAY_RES_PROTECT_FLAGS_INDEX] |= (
		T3_REPLAY_RES_PROTECT_LOCATED
	);
}

static void replay_protect_committed_size_set(uint32_t size)
{
	replay_protect_handoff_u32_write(T3_REPLAY_RES_COMMITTED_SIZE_INDEX, size);
}

static uint32_t replay_protect_committed_size(void)
{
	return replay_protect_handoff_u32_read(
		T3_REPLAY_RES_COMMITTED_SIZE_INDEX
	);
}

static void replay_protect_state_reset(void)
{
	int i;

	replay_protect_committed_size_set(0);
	resident->unused_3[T3_REPLAY_RES_PROTECT_FLAGS_INDEX] = 0;
	replay_protect_handoff_u32_write(T3_REPLAY_RES_GUARD_SECTOR_INDEX, 0);
	replay_protect_handoff_u16_write(T3_REPLAY_RES_GUARD_OFFSET_INDEX, 0);
	for(i = T3R_DIAG_CODE_INDEX; i < T3R_DIAG_END_INDEX; i++) {
		resident->unused_3[i] = 0;
	}
}

static void replay_protect_local_reset(void)
{
	replay_protect_handoff_u16_write(T3_REPLAY_RES_PROTECT_BUF_SEG_INDEX, 0);
}

static bool replay_protect_ctx_alloc(replay_protect_ctx_t _ss *ctx)
{
	uint16_t seg = replay_protect_handoff_u16_read(
		T3_REPLAY_RES_PROTECT_BUF_SEG_INDEX
	);

	ctx->sector = nullptr;
	ctx->packet = nullptr;
	if(seg == 0) {
		seg = reinterpret_cast<uint16_t>(
			hmem_allocbyte(RP_SECTOR_ALLOCATION_SIZE)
		);
		if(seg == 0) {
			replay_protect_diag_code_set(RPD_CTX_ALLOC);
			return false;
		}
		replay_protect_handoff_u16_write(
			T3_REPLAY_RES_PROTECT_BUF_SEG_INDEX, seg
		);
	}
	ctx->sector = reinterpret_cast<uint8_t far *>(MK_FP(seg, 0));
	ctx->packet = (ctx->sector + RP_SECTOR_BUFFER_SIZE_MAX);
	return ((ctx->sector != nullptr) && (ctx->packet != nullptr));
}

static void replay_protect_ctx_free(replay_protect_ctx_t _ss *ctx)
{
	(void)ctx;
}

static void replay_protect_local_free(void)
{
	uint16_t seg = replay_protect_handoff_u16_read(
		T3_REPLAY_RES_PROTECT_BUF_SEG_INDEX
	);

	if(seg != 0) {
		hmem_free(reinterpret_cast<void __seg *>(seg));
		replay_protect_handoff_u16_write(T3_REPLAY_RES_PROTECT_BUF_SEG_INDEX, 0);
	}
}

static uint8_t replay_protect_current_drive(void)
{
	_AH = 0x19;
	geninterrupt(0x21);
	return _AL;
}

static bool replay_protect_extended_abs_read_available(void)
{
	uint8_t major;
	uint8_t minor;

	_AX = 0x3000;
	geninterrupt(0x21);
	major = _AL;
	minor = _AH;
	return ((major > 7) || ((major == 7) && (minor >= 10)));
}

static void replay_protect_raw_init(replay_protect_ctx_t _ss *ctx)
{
	ctx->drive = replay_protect_current_drive();
	ctx->fat_type = RPF_UNKNOWN;
	ctx->extended_abs_read = replay_protect_extended_abs_read_available();
}

static bool replay_protect_abs_read_small(
	uint8_t drive, uint16_t sector, uint16_t count, void far *buffer
)
{
	uint16_t dos_ax = 0;
	uint16_t dos_flags = 1;
	uint16_t dos_stack_flags = 0;

	_AL = drive;
	_CX = count;
	_DX = sector;
	asm {
		push	bp
		push	si
		push	di
		push	es
		push	ds
		lds	bx, buffer
		int	25h
		pushf
		push	ax
		pop	cx
		pop	ax
		pop	dx
		pop	ds
		pop	es
		pop	di
		pop	si
		pop	bp
		mov	dos_ax, cx
		mov	dos_flags, ax
		mov	dos_stack_flags, dx
	}
	replay_protect_diag_int25_flags_set(dos_flags, dos_stack_flags);
	if(dos_flags & 1) {
		replay_protect_diag_dos_ax_set(dos_ax);
		return false;
	}
	replay_protect_diag_dos_ax_set(0);
	return true;
}

static bool replay_protect_abs_read_extended(
	uint8_t drive, uint32_t sector, uint16_t count, void far *buffer,
	uint8_t far *packet
)
{
	uint16_t dos_ax = 0;
	uint16_t dos_flags = 1;

	replay_protect_u32_write(packet + 0, sector);
	replay_protect_u16_write(packet + 4, count);
	replay_protect_u16_write(packet + 6, FP_OFF(buffer));
	replay_protect_u16_write(packet + 8, FP_SEG(buffer));
	_DX = static_cast<uint16_t>(drive + 1);
	_CX = 0xFFFF;
	_SI = 0;
	// Keep AX last. Turbo C++ uses AX to evaluate the drive expression above.
	_AX = 0x7305;
	asm {
		push	bp
		push	si
		push	di
		push	es
		push	ds
		lds	bx, packet
		int	21h
		pushf
		push	ax
		pop	cx
		pop	ax
		pop	ds
		pop	es
		pop	di
		pop	si
		pop	bp
		mov	dos_ax, cx
		mov	dos_flags, ax
	}
	replay_protect_diag_int25_flags_set(dos_flags, 0);
	if(dos_flags & 1) {
		replay_protect_diag_dos_ax_set(dos_ax);
		return false;
	}
	replay_protect_diag_dos_ax_set(0);
	return true;
}

static bool replay_protect_sector_read(
	replay_protect_ctx_t _ss *ctx, uint32_t sector
)
{
	// Preserve the proven FAT12/FAT16 path. FAT32 rejects old-form INT 25h,
	// after which DOS 7.1's extended absolute read handles the same request.
	if((ctx->fat_type == RPF_FAT32) || (sector > 0xFFFFUL)) {
		if(!ctx->extended_abs_read) {
			replay_protect_diag_location_set(sector, 0);
			replay_protect_diag_code_set(RPD_EXTENDED_READ_UNAVAILABLE);
			return false;
		}
		return replay_protect_abs_read_extended(
			ctx->drive, sector, 1, ctx->sector, ctx->packet
		);
	}
	if(replay_protect_abs_read_small(
		ctx->drive, static_cast<uint16_t>(sector), 1, ctx->sector
	)) {
		return true;
	}
	if(!ctx->extended_abs_read) {
		replay_protect_diag_location_set(sector, 0);
		return false;
	}
	return replay_protect_abs_read_extended(
		ctx->drive, sector, 1, ctx->sector, ctx->packet
	);
}

static bool replay_protect_sector_size_supported(uint16_t bytes_per_sector)
{
	return (
		(bytes_per_sector == 512) ||
		(bytes_per_sector == 1024) ||
		(bytes_per_sector == 2048) ||
		(bytes_per_sector == 4096)
	);
}

static bool replay_protect_sectors_per_cluster_supported(uint8_t value)
{
	return ((value != 0) && ((value & (value - 1)) == 0));
}

static uint8_t replay_protect_power_of_two_shift(uint16_t value)
{
	uint8_t shift = 0;

	while(value > 1) {
		value >>= 1;
		shift++;
	}
	return shift;
}

static uint16_t replay_protect_root_sector_count(
	uint32_t root_bytes, uint16_t bytes_per_sector
)
{
	if(bytes_per_sector == 512) {
		return static_cast<uint16_t>((root_bytes + 511) >> 9);
	}
	if(bytes_per_sector == 1024) {
		return static_cast<uint16_t>((root_bytes + 1023) >> 10);
	}
	if(bytes_per_sector == 2048) {
		return static_cast<uint16_t>((root_bytes + 2047) >> 11);
	}
	return static_cast<uint16_t>((root_bytes + 4095) >> 12);
}

static bool replay_protect_volume_init(replay_protect_ctx_t _ss *ctx)
{
	uint16_t reserved;
	uint8_t fat_count;
	uint8_t active_fat = 0;
	uint8_t i;
	uint16_t sectors_per_fat_16;
	uint16_t total_sectors_16;
	uint16_t fat32_flags;
	uint32_t root_bytes;
	uint32_t fat_span = 0;
	uint32_t root_base;
	uint32_t data_sectors;
	uint32_t root_delta;

	if(!replay_protect_ctx_alloc(ctx)) {
		return false;
	}

	replay_protect_raw_init(ctx);
	if(!replay_protect_sector_read(ctx, 0)) {
		replay_protect_diag_code_set(RPD_BPB_READ);
		replay_protect_ctx_free(ctx);
		return false;
	}

	ctx->bytes_per_sector = replay_protect_u16(ctx->sector + 0x0B);
	ctx->sectors_per_cluster = ctx->sector[0x0D];
	reserved = replay_protect_u16(ctx->sector + 0x0E);
	fat_count = ctx->sector[0x10];
	ctx->root_entry_count = replay_protect_u16(ctx->sector + 0x11);
	total_sectors_16 = replay_protect_u16(ctx->sector + 0x13);
	sectors_per_fat_16 = replay_protect_u16(ctx->sector + 0x16);
	ctx->total_sectors = total_sectors_16;
	if(ctx->total_sectors == 0) {
		ctx->total_sectors = replay_protect_u32(ctx->sector + 0x20);
	}
	ctx->fat_sectors = sectors_per_fat_16;
	if(ctx->fat_sectors == 0) {
		ctx->fat_sectors = replay_protect_u32(ctx->sector + 0x24);
	}

	if(
		!replay_protect_sector_size_supported(ctx->bytes_per_sector) ||
		!replay_protect_sectors_per_cluster_supported(
			ctx->sectors_per_cluster
		) ||
		(reserved == 0) ||
		(fat_count == 0) ||
		(ctx->total_sectors == 0) ||
		(ctx->fat_sectors == 0)
	) {
		replay_protect_diag_geometry_set(
			ctx->drive, ctx->bytes_per_sector, ctx->root_entry_count, 0, 0
		);
		replay_protect_diag_code_set(RPD_BPB_UNSUPPORTED);
		replay_protect_ctx_free(ctx);
		return false;
	}

	for(i = 0; i < fat_count; i++) {
		root_base = fat_span;
		fat_span += ctx->fat_sectors;
		if(fat_span < root_base) {
			replay_protect_diag_code_set(RPD_BPB_UNSUPPORTED);
			replay_protect_ctx_free(ctx);
			return false;
		}
	}
	root_base = static_cast<uint32_t>(reserved) + fat_span;
	if(root_base < reserved) {
		replay_protect_diag_code_set(RPD_BPB_UNSUPPORTED);
		replay_protect_ctx_free(ctx);
		return false;
	}
	root_bytes = (static_cast<uint32_t>(ctx->root_entry_count) << 5);
	ctx->root_sectors = replay_protect_root_sector_count(
		root_bytes, ctx->bytes_per_sector
	);
	ctx->first_data_sector = root_base + ctx->root_sectors;
	if(
		(ctx->first_data_sector < root_base) ||
		(ctx->first_data_sector >= ctx->total_sectors)
	) {
		replay_protect_diag_code_set(RPD_BPB_UNSUPPORTED);
		replay_protect_ctx_free(ctx);
		return false;
	}
	data_sectors = ctx->total_sectors - ctx->first_data_sector;
	ctx->cluster_count = (
		data_sectors >>
		replay_protect_power_of_two_shift(ctx->sectors_per_cluster)
	);
	ctx->fat_start = reserved;
	ctx->root_cluster = 0;
	if(ctx->cluster_count < RP_FAT12_CLUSTER_COUNT_MAX) {
		ctx->fat_type = RPF_FAT12;
	} else if(ctx->cluster_count < RP_FAT16_CLUSTER_COUNT_MAX) {
		ctx->fat_type = RPF_FAT16;
	} else {
		ctx->fat_type = RPF_FAT32;
	}

	if(ctx->fat_type == RPF_FAT32) {
		fat32_flags = replay_protect_u16(ctx->sector + 0x28);
		if(fat32_flags & 0x0080) {
			active_fat = static_cast<uint8_t>(fat32_flags & 0x000F);
		}
		ctx->root_cluster = (
			replay_protect_u32(ctx->sector + 0x2C) & RP_FAT32_ENTRY_MASK
		);
		if(
			(ctx->root_entry_count != 0) ||
			(sectors_per_fat_16 != 0) ||
			!ctx->extended_abs_read ||
			(active_fat >= fat_count) ||
			(ctx->cluster_count >= RP_FAT32_CLUSTER_MAX) ||
			(ctx->root_cluster < 2) ||
			(ctx->root_cluster > (ctx->cluster_count + 1))
		) {
			replay_protect_diag_code_set(RPD_BPB_UNSUPPORTED);
			replay_protect_ctx_free(ctx);
			return false;
		}
		ctx->fat_start += replay_protect_mul_u32_u8(
			ctx->fat_sectors, active_fat
		);
		root_delta = replay_protect_mul_u32_u8(
			(ctx->root_cluster - 2), ctx->sectors_per_cluster
		);
		ctx->root_start = ctx->first_data_sector + root_delta;
		ctx->root_sectors = 0;
	} else {
		if(
			(ctx->root_entry_count == 0) ||
			(sectors_per_fat_16 == 0) ||
			(ctx->root_sectors == 0)
		) {
			replay_protect_diag_code_set(RPD_BPB_UNSUPPORTED);
			replay_protect_ctx_free(ctx);
			return false;
		}
		ctx->root_start = root_base;
	}
	replay_protect_diag_geometry_set(
		ctx->drive, ctx->bytes_per_sector, ctx->root_entry_count,
		ctx->root_start, ctx->root_sectors
	);
	if(
		(ctx->root_start >= ctx->total_sectors) ||
		((ctx->fat_start + ctx->fat_sectors) > ctx->total_sectors)
	) {
		replay_protect_diag_code_set(RPD_ROOT_RANGE);
		replay_protect_ctx_free(ctx);
		return false;
	}
	return true;
}

static bool replay_protect_name_char_eq(char a, char b)
{
	if((a >= 'a') && (a <= 'z')) {
		a -= ('a' - 'A');
	}
	return (a == b);
}

static void replay_protect_short_name_clear(char far *short_name)
{
	int i;

	for(i = 0; i < 11; i++) {
		short_name[i] = ' ';
	}
}

static bool replay_protect_short_name_component(
	char far *short_name, const char far *component
)
{
	int i = 0;
	int ext = 8;
	char c;

	replay_protect_short_name_clear(short_name);
	while((c = *component++) != '\0') {
		if((c == '\\') || (c == '/')) {
			break;
		}
		if(c == '.') {
			i = 8;
			ext = 11;
			continue;
		}
		if((c >= 'a') && (c <= 'z')) {
			c -= ('a' - 'A');
		}
		if(i >= 11) {
			return false;
		}
		if((i == ext) && (ext == 8)) {
			return false;
		}
		short_name[i++] = c;
	}
	return true;
}

static bool replay_protect_name_match(
	const uint8_t far *entry, const char far *short_name
)
{
	int i;

	for(i = 0; i < 11; i++) {
		if(!replay_protect_name_char_eq(entry[i], short_name[i])) {
			return false;
		}
	}
	return true;
}

static const char far *replay_protect_basename(const char far *fn)
{
	const char far *p = fn;
	const char far *base = fn;
	char c;

	while((c = *p++) != '\0') {
		if((c == '\\') || (c == '/')) {
			base = p;
		}
	}
	return base;
}

enum replay_protect_root_scan_t {
	RPRS_CONTINUE = 0,
	RPRS_FOUND = 1,
	RPRS_END = 2,
};

static bool replay_protect_cluster_sector(
	replay_protect_ctx_t _ss *ctx, uint32_t cluster, uint32_t _ss *sector
)
{
	uint32_t delta;
	uint32_t cluster_end;

	if((cluster < 2) || (cluster > (ctx->cluster_count + 1))) {
		return false;
	}
	delta = replay_protect_mul_u32_u8(
		(cluster - 2), ctx->sectors_per_cluster
	);
	*sector = ctx->first_data_sector + delta;
	cluster_end = *sector + ctx->sectors_per_cluster;
	return (
		(*sector >= ctx->first_data_sector) &&
		(cluster_end >= *sector) &&
		(cluster_end <= ctx->total_sectors)
	);
}

static bool replay_protect_fat32_next_cluster(
	replay_protect_ctx_t _ss *ctx, uint32_t cluster,
	uint32_t _ss *next, bool _ss *eoc
)
{
	uint32_t fat_offset = (cluster << 2);
	uint32_t fat_sector = (
		ctx->fat_start +
		(fat_offset >> replay_protect_power_of_two_shift(ctx->bytes_per_sector))
	);
	uint16_t offset = static_cast<uint16_t>(
		fat_offset & (ctx->bytes_per_sector - 1)
	);

	if(
		(fat_sector < ctx->fat_start) ||
		(fat_sector >= (ctx->fat_start + ctx->fat_sectors)) ||
		(offset > (ctx->bytes_per_sector - 4))
	) {
		replay_protect_diag_location_set(fat_sector, offset);
		replay_protect_diag_code_set(RPD_ROOT_CHAIN);
		return false;
	}
	if(!replay_protect_sector_read(ctx, fat_sector)) {
		replay_protect_diag_location_set(fat_sector, offset);
		replay_protect_diag_code_set(RPD_ROOT_FAT_READ);
		return false;
	}
	*next = (replay_protect_u32(ctx->sector + offset) & RP_FAT32_ENTRY_MASK);
	*eoc = (*next >= RP_FAT32_CLUSTER_EOC);
	if(*eoc) {
		return true;
	}
	if(
		(*next < 2) ||
		(*next == RP_FAT32_CLUSTER_BAD) ||
		(*next > (ctx->cluster_count + 1))
	) {
		replay_protect_diag_sizes_set(cluster, *next);
		replay_protect_diag_location_set(fat_sector, offset);
		replay_protect_diag_code_set(RPD_ROOT_CHAIN);
		return false;
	}
	return true;
}

static uint8_t replay_protect_root_sector_scan(
	replay_protect_ctx_t _ss *ctx, const char far *short_name,
	uint32_t far *size, bool cache_location, uint32_t sector
)
{
	uint16_t offset;
	uint8_t attr;
	uint8_t first;

	for(offset = 0; offset < ctx->bytes_per_sector; offset += 32) {
		first = ctx->sector[offset];
		if(first == RP_FAT_ENTRY_FREE) {
			replay_protect_diag_location_set(sector, offset);
			return RPRS_END;
		}
		if(first == RP_FAT_ENTRY_DELETED) {
			continue;
		}
		attr = ctx->sector[offset + 0x0B];
		if(attr & (RP_FAT_ATTR_VOLUME | RP_FAT_ATTR_DIR)) {
			continue;
		}
		if(replay_protect_name_match(ctx->sector + offset, short_name)) {
			*size = replay_protect_u32(ctx->sector + offset + 0x1C);
			replay_protect_diag_location_set(sector, offset);
			if(cache_location) {
				replay_protect_location_set(sector, offset);
			}
			return RPRS_FOUND;
		}
	}
	return RPRS_CONTINUE;
}

static bool replay_protect_root_size_read_impl(
	const char far *fn, uint32_t far *size, bool cache_location
)
{
	replay_protect_ctx_t ctx;
	char short_name[11];
	const char far *base;
	uint32_t sector;
	uint32_t cluster;
	uint32_t next_cluster;
	uint32_t clusters_walked = 0;
	uint8_t sector_in_cluster;
	uint8_t scan;
	bool eoc;

	if(!replay_protect_volume_init(&ctx)) {
		return false;
	}
	base = replay_protect_basename(fn);
	if(!replay_protect_short_name_component(short_name, base)) {
		replay_protect_diag_code_set(RPD_SHORT_NAME);
		replay_protect_ctx_free(&ctx);
		return false;
	}

	if(ctx.fat_type != RPF_FAT32) {
		for(sector = 0; sector < ctx.root_sectors; sector++) {
			if(!replay_protect_sector_read(&ctx, ctx.root_start + sector)) {
				replay_protect_diag_location_set(ctx.root_start + sector, 0);
				replay_protect_diag_code_set(RPD_ROOT_SECTOR_READ);
				replay_protect_ctx_free(&ctx);
				return false;
			}
			scan = replay_protect_root_sector_scan(
				&ctx, short_name, size, cache_location,
				(ctx.root_start + sector)
			);
			if(scan == RPRS_FOUND) {
				replay_protect_ctx_free(&ctx);
				return true;
			}
			if(scan == RPRS_END) {
				replay_protect_diag_code_set(RPD_ROOT_NOT_FOUND);
				replay_protect_ctx_free(&ctx);
				return false;
			}
		}
	} else {
		cluster = ctx.root_cluster;
		while(true) {
			if(!replay_protect_cluster_sector(&ctx, cluster, &sector)) {
				replay_protect_diag_sizes_set(2, cluster);
				replay_protect_diag_code_set(RPD_ROOT_CHAIN);
				replay_protect_ctx_free(&ctx);
				return false;
			}
			for(
				sector_in_cluster = 0;
				sector_in_cluster < ctx.sectors_per_cluster;
				sector_in_cluster++
			) {
				if(!replay_protect_sector_read(
					&ctx, sector + sector_in_cluster
				)) {
					replay_protect_diag_location_set(
						sector + sector_in_cluster, 0
					);
					replay_protect_diag_code_set(RPD_ROOT_SECTOR_READ);
					replay_protect_ctx_free(&ctx);
					return false;
				}
				scan = replay_protect_root_sector_scan(
					&ctx, short_name, size, cache_location,
					(sector + sector_in_cluster)
				);
				if(scan == RPRS_FOUND) {
					replay_protect_ctx_free(&ctx);
					return true;
				}
				if(scan == RPRS_END) {
					replay_protect_diag_code_set(RPD_ROOT_NOT_FOUND);
					replay_protect_ctx_free(&ctx);
					return false;
				}
			}
			clusters_walked++;
			if(clusters_walked >= ctx.cluster_count) {
				replay_protect_diag_sizes_set(ctx.cluster_count, clusters_walked);
				replay_protect_diag_code_set(RPD_ROOT_CHAIN);
				replay_protect_ctx_free(&ctx);
				return false;
			}
			if(!replay_protect_fat32_next_cluster(
				&ctx, cluster, &next_cluster, &eoc
			)) {
				replay_protect_ctx_free(&ctx);
				return false;
			}
			if(eoc) {
				break;
			}
			cluster = next_cluster;
		}
	}
	replay_protect_diag_code_set(RPD_ROOT_NOT_FOUND);
	replay_protect_ctx_free(&ctx);
	return false;
}

static bool replay_protect_root_size_read(
	const char far *fn, uint32_t far *size
)
{
	return replay_protect_root_size_read_impl(fn, size, true);
}

static bool replay_protect_root_size_read_uncached(
	const char far *fn, uint32_t far *size
)
{
	return replay_protect_root_size_read_impl(fn, size, false);
}

static bool replay_protect_located_size_read(
	const char far *fn, uint32_t far *size
)
{
	replay_protect_ctx_t ctx;
	char short_name[11];
	const char far *base;
	uint32_t sector = replay_protect_handoff_u32_read(
		T3_REPLAY_RES_GUARD_SECTOR_INDEX
	);
	uint16_t offset = replay_protect_handoff_u16_read(
		T3_REPLAY_RES_GUARD_OFFSET_INDEX
	);
	uint8_t attr;
	uint8_t first;

	if(
		!replay_protect_located() ||
		(sector == 0) ||
		(offset > (RP_SECTOR_BUFFER_SIZE_MAX - 32)) ||
		((offset & 0x1F) != 0)
	) {
		replay_protect_diag_location_set(sector, offset);
		replay_protect_diag_code_set(RPD_LOCATED_RANGE);
		return false;
	}
	base = replay_protect_basename(fn);
	if(!replay_protect_short_name_component(short_name, base)) {
		replay_protect_diag_code_set(RPD_SHORT_NAME);
		return false;
	}

	if(!replay_protect_ctx_alloc(&ctx)) {
		return false;
	}
	replay_protect_raw_init(&ctx);
	if(!replay_protect_sector_read(&ctx, sector)) {
		replay_protect_diag_location_set(sector, offset);
		replay_protect_diag_code_set(RPD_LOCATED_SECTOR_READ);
		replay_protect_ctx_free(&ctx);
		return false;
	}
	first = ctx.sector[offset];
	if((first == RP_FAT_ENTRY_FREE) || (first == RP_FAT_ENTRY_DELETED)) {
		replay_protect_diag_location_set(sector, offset);
		replay_protect_diag_code_set(RPD_LOCATED_ENTRY_BAD);
		replay_protect_ctx_free(&ctx);
		return false;
	}
	attr = ctx.sector[offset + 0x0B];
	if(attr & (RP_FAT_ATTR_VOLUME | RP_FAT_ATTR_DIR)) {
		replay_protect_diag_location_set(sector, offset);
		replay_protect_diag_code_set(RPD_LOCATED_ENTRY_BAD);
		replay_protect_ctx_free(&ctx);
		return false;
	}
	if(!replay_protect_name_match(ctx.sector + offset, short_name)) {
		replay_protect_diag_location_set(sector, offset);
		replay_protect_diag_code_set(RPD_LOCATED_NAME);
		replay_protect_ctx_free(&ctx);
		return false;
	}
	*size = replay_protect_u32(ctx.sector + offset + 0x1C);
	replay_protect_diag_location_set(sector, offset);
	replay_protect_ctx_free(&ctx);
	return true;
}

static bool replay_protect_size_read(const char far *guard_fn, uint32_t far *size)
{
	if(
		replay_protect_located() &&
		replay_protect_located_size_read(guard_fn, size)
	) {
		return true;
	}
	if(!replay_protect_root_size_read(guard_fn, size)) {
		return false;
	}
	// A failed cached-location probe is recoverable if the root scan succeeds.
	replay_protect_diag_code_set(RPD_NONE);
	return true;
}

static bool replay_protect_guard_marker_read(
	const char far *guard_fn, uint8_t far *marker, uint32_t far *guard_size
)
{
	replay_protect_ctx_t ctx;
	char short_name[11];
	const char far *base;
	uint32_t root_sector;
	uint32_t data_sector;
	uint16_t root_offset;
	uint32_t cluster;
	uint8_t attr;
	uint8_t first;

	if(!replay_protect_located()) {
		if(!replay_protect_root_size_read(guard_fn, guard_size)) {
			return false;
		}
	}
	if(!replay_protect_volume_init(&ctx)) {
		return false;
	}
	root_sector = replay_protect_handoff_u32_read(
		T3_REPLAY_RES_GUARD_SECTOR_INDEX
	);
	root_offset = replay_protect_handoff_u16_read(
		T3_REPLAY_RES_GUARD_OFFSET_INDEX
	);
	if(
		(root_sector == 0) ||
		(root_offset > (ctx.bytes_per_sector - 32)) ||
		((root_offset & 0x1F) != 0)
	) {
		replay_protect_diag_location_set(root_sector, root_offset);
		replay_protect_diag_code_set(RPD_LOCATED_RANGE);
		replay_protect_ctx_free(&ctx);
		return false;
	}
	if(!replay_protect_sector_read(&ctx, root_sector)) {
		replay_protect_diag_location_set(root_sector, root_offset);
		replay_protect_diag_code_set(RPD_LOCATED_SECTOR_READ);
		replay_protect_ctx_free(&ctx);
		return false;
	}
	base = replay_protect_basename(guard_fn);
	if(!replay_protect_short_name_component(short_name, base)) {
		replay_protect_diag_code_set(RPD_SHORT_NAME);
		replay_protect_ctx_free(&ctx);
		return false;
	}
	first = ctx.sector[root_offset];
	attr = ctx.sector[root_offset + 0x0B];
	if(
		(first == RP_FAT_ENTRY_FREE) ||
		(first == RP_FAT_ENTRY_DELETED) ||
		(attr & (RP_FAT_ATTR_VOLUME | RP_FAT_ATTR_DIR)) ||
		!replay_protect_name_match(ctx.sector + root_offset, short_name)
	) {
		replay_protect_diag_location_set(root_sector, root_offset);
		replay_protect_diag_code_set(RPD_LOCATED_ENTRY_BAD);
		replay_protect_ctx_free(&ctx);
		return false;
	}
	*guard_size = replay_protect_u32(ctx.sector + root_offset + 0x1C);
	if(*guard_size < RP_GUARD_BASE_SIZE) {
		replay_protect_diag_sizes_set(RP_GUARD_BASE_SIZE, *guard_size);
		replay_protect_diag_code_set(RPD_MARKER_CLUSTER);
		replay_protect_ctx_free(&ctx);
		return false;
	}
	cluster = replay_protect_u16(ctx.sector + root_offset + 0x1A);
	if(ctx.fat_type == RPF_FAT32) {
		cluster |= (
			static_cast<uint32_t>(
				replay_protect_u16(ctx.sector + root_offset + 0x14)
			) << 16
		);
		cluster &= RP_FAT32_ENTRY_MASK;
	}
	if(!replay_protect_cluster_sector(&ctx, cluster, &data_sector)) {
		replay_protect_diag_sizes_set(2, cluster);
		replay_protect_diag_code_set(RPD_MARKER_CLUSTER);
		replay_protect_ctx_free(&ctx);
		return false;
	}
	if(!replay_protect_sector_read(&ctx, data_sector)) {
		replay_protect_diag_location_set(data_sector, 0);
		replay_protect_diag_code_set(RPD_MARKER_SECTOR_READ);
		replay_protect_ctx_free(&ctx);
		return false;
	}
	*marker = ctx.sector[0];
	replay_protect_diag_location_set(data_sector, 0);
	replay_protect_ctx_free(&ctx);
	return true;
}

static bool replay_protect_guard_marker_verify(const char far *guard_fn)
{
	uint8_t marker;
	uint32_t disk_size;
	uint32_t committed_size;

	if(replay_protect_blocked()) {
		return false;
	}
	if(!replay_protect_guard_marker_read(guard_fn, &marker, &disk_size)) {
		replay_protect_detector_error_set();
		return false;
	}
	if(marker != RP_GUARD_MARKER_CLEAR) {
		replay_protect_diag_sizes_set(RP_GUARD_MARKER_CLEAR, marker);
		replay_protect_diag_code_set(RPD_MARKER_VALUE);
		replay_protect_invalidate();
		return false;
	}
	committed_size = replay_protect_committed_size();
	if(disk_size > committed_size) {
		replay_protect_diag_sizes_set(committed_size, disk_size);
		replay_protect_diag_code_set(RPD_VERIFY_MISMATCH);
		replay_protect_invalidate();
		return false;
	}
	if(disk_size < committed_size) {
		replay_protect_diag_sizes_set(committed_size, disk_size);
		replay_protect_diag_code_set(RPD_VERIFY_MISMATCH);
		replay_protect_detector_error_set();
		return false;
	}
	return true;
}

static bool replay_protect_commit_process(void);
static bool replay_protect_flush_current_file(uint8_t diag_code);
static bool replay_protect_close_current_file(uint8_t diag_code);

static bool replay_protect_guard_marker_set(const char far *guard_fn)
{
	uint8_t marker = RP_GUARD_MARKER_SET;
	uint8_t disk_marker;
	uint32_t disk_size;

	if(!file_append(guard_fn)) {
		replay_protect_diag_code_set(RPD_MARKER_WRITE);
		replay_protect_detector_error_set();
		return false;
	}
	file_seek(0, SEEK_SET);
	if(file_write(&marker, sizeof(marker)) == 0) {
		file_close();
		replay_protect_diag_code_set(RPD_MARKER_WRITE);
		replay_protect_detector_error_set();
		return false;
	}
	if(!replay_protect_flush_current_file(RPD_MARKER_WRITE)) {
		file_close();
		return false;
	}
	(void)replay_protect_commit_process();
	if(!replay_protect_close_current_file(RPD_MARKER_WRITE)) {
		return false;
	}
	if(!replay_protect_guard_marker_read(
		guard_fn, &disk_marker, &disk_size
	)) {
		replay_protect_detector_error_set();
		return false;
	}
	if(disk_marker != RP_GUARD_MARKER_SET) {
		replay_protect_diag_sizes_set(RP_GUARD_MARKER_SET, disk_marker);
		replay_protect_diag_code_set(RPD_MARKER_VERIFY);
		replay_protect_detector_error_set();
		return false;
	}
	replay_protect_diag_sizes_set(RP_GUARD_MARKER_CLEAR, disk_marker);
	replay_protect_diag_code_set(RPD_MARKER_VALUE);
	return true;
}

static uint16_t replay_protect_current_psp(void)
{
	uint16_t psp = 0;

	asm {
		push	bp
		push	si
		push	di
		push	ds
		push	es
		mov	ah, 51h
		int	21h
		push	bx
		pop	cx
		pop	es
		pop	ds
		pop	di
		pop	si
		pop	bp
		mov	psp, cx
	}
	return psp;
}

static bool replay_protect_commit_process(void)
{
	uint16_t dpl[RP_DOS_PARAMETER_LIST_WORDS];
	uint16_t dos_ax = 0;
	uint16_t dos_flags = 1;
	int i;

	for(i = 0; i < RP_DOS_PARAMETER_LIST_WORDS; i++) {
		dpl[i] = 0;
	}
	dpl[RP_DOS_PARAMETER_LIST_PROCESS_ID] = replay_protect_current_psp();

	asm {
		push	bp
		push	si
		push	di
		push	es
		push	ds
		push	ss
		pop	ds
		lea	dx, dpl
		mov	ax, 5D01h
		int	21h
		pushf
		push	ax
		pop	cx
		pop	ax
		pop	ds
		pop	es
		pop	di
		pop	si
		pop	bp
		mov	dos_ax, cx
		mov	dos_flags, ax
	}
	if(dos_flags & 1) {
		replay_protect_diag_dos_ax_set(dos_ax);
		return false;
	}
	replay_protect_diag_dos_ax_set(0);
	return true;
}

static bool replay_protect_flush_current_file(uint8_t diag_code)
{
	file_flush();
	if(file_ErrorStat != 0) {
		replay_protect_diag_code_set(diag_code);
		replay_protect_detector_error_set();
		return false;
	}
	return true;
}

static bool replay_protect_close_current_file(uint8_t diag_code)
{
	file_close();
	if(file_ErrorStat != 0) {
		replay_protect_diag_code_set(diag_code);
		replay_protect_detector_error_set();
		return false;
	}
	if(!replay_protect_commit_process()) {
		replay_protect_diag_code_set(RPD_CLOSE_COMMIT);
		replay_protect_detector_error_set();
		return false;
	}
	return true;
}

static void replay_protect_file_delete_commit(const char far *fn)
{
	dos_axdx(0x4100, fn);
	(void)replay_protect_commit_process();
}

static bool replay_protect_guard_create(const char far *guard_fn)
{
	uint32_t disk_size;
	uint8_t marker = RP_GUARD_MARKER_CLEAR;
	uint8_t disk_marker;

	replay_protect_state_reset();
	replay_protect_committed_size_set(0);
	if(!file_create(guard_fn)) {
		replay_protect_diag_code_set(RPD_GUARD_CREATE);
		replay_protect_detector_error_set();
		return false;
	}
	if(file_write(&marker, sizeof(marker)) == 0) {
		file_close();
		replay_protect_diag_code_set(RPD_MARKER_WRITE);
		replay_protect_detector_error_set();
		return false;
	}
	if(!replay_protect_flush_current_file(RPD_COMMIT_FLUSH)) {
		file_close();
		return false;
	}
	(void)replay_protect_commit_process();
	if(!replay_protect_close_current_file(RPD_COMMIT_FLUSH)) {
		return false;
	}
	if(!replay_protect_guard_marker_read(
		guard_fn, &disk_marker, &disk_size
	)) {
		replay_protect_detector_error_set();
		return false;
	}
	if(disk_size != RP_GUARD_BASE_SIZE) {
		replay_protect_diag_sizes_set(RP_GUARD_BASE_SIZE, disk_size);
		replay_protect_diag_code_set(RPD_GUARD_CREATE_NONZERO);
		replay_protect_detector_error_set();
		return false;
	}
	if(disk_marker != RP_GUARD_MARKER_CLEAR) {
		replay_protect_diag_sizes_set(RP_GUARD_MARKER_CLEAR, disk_marker);
		replay_protect_diag_code_set(RPD_MARKER_VERIFY);
		replay_protect_detector_error_set();
		return false;
	}
	replay_protect_committed_size_set(RP_GUARD_BASE_SIZE);
	replay_protect_diag_dos_ax_set(0);
	return true;
}

static bool replay_protect_checkpoint(const char far *guard_fn)
{
	uint32_t expected_size;
	uint32_t disk_size;
	uint8_t byte = 0;
	bool committed_open;

	if(!replay_protect_guard_marker_verify(guard_fn)) {
		return false;
	}
	expected_size = replay_protect_committed_size() + 1;
	if(!file_append(guard_fn)) {
		replay_protect_diag_code_set(RPD_CHECKPOINT_APPEND);
		replay_protect_detector_error_set();
		return false;
	}
	file_seek(replay_protect_committed_size(), SEEK_SET);
	if(file_write(&byte, sizeof(byte)) == 0) {
		file_close();
		replay_protect_diag_code_set(RPD_CHECKPOINT_WRITE);
		replay_protect_detector_error_set();
		return false;
	}
	if(!replay_protect_flush_current_file(RPD_CHECKPOINT_WRITE)) {
		file_close();
		return false;
	}
	committed_open = false;
	if(replay_protect_commit_process()) {
		if(replay_protect_size_read(guard_fn, &disk_size)) {
			committed_open = (disk_size == expected_size);
		}
	}
	if(!committed_open) {
		if(!replay_protect_close_current_file(RPD_CHECKPOINT_WRITE)) {
			return false;
		}
		if(!replay_protect_size_read(guard_fn, &disk_size)) {
			replay_protect_detector_error_set();
			return false;
		}
		if(disk_size != expected_size) {
			replay_protect_diag_sizes_set(expected_size, disk_size);
			replay_protect_diag_code_set(RPD_CHECKPOINT_MISMATCH);
			replay_protect_detector_error_set();
			return false;
		}
	} else {
		if(!replay_protect_close_current_file(RPD_CHECKPOINT_WRITE)) {
			return false;
		}
	}
	replay_protect_committed_size_set(expected_size);
	replay_protect_diag_dos_ax_set(0);
	return true;
}

#undef RP_FAT_ENTRY_FREE
#undef RP_FAT_ENTRY_DELETED
#undef RP_FAT_ATTR_VOLUME
#undef RP_FAT_ATTR_DIR
#undef RP_SECTOR_BUFFER_SIZE_MAX
#undef RP_DOS_PARAMETER_LIST_WORDS
#undef RP_DOS_PARAMETER_LIST_PROCESS_ID
#undef RP_GUARD_BASE_SIZE
#undef RP_GUARD_MARKER_CLEAR
#undef RP_GUARD_MARKER_SET

#endif /* TH03_REPLAY_PROTECT_HPP */
