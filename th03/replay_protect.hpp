#ifndef TH03_REPLAY_PROTECT_HPP
#define TH03_REPLAY_PROTECT_HPP

#include "libs/master.lib/master.hpp"
#include "platform.h"
#include "th03/replay_handoff.hpp"
#include "x86real.h"

extern "C" void MASTER_RET file_flush(void);
extern "C" int __cdecl file_ErrorStat;
extern "C" int __cdecl file_Handle;

#define RP_FAT_ENTRY_FREE 0x00
#define RP_FAT_ENTRY_DELETED 0xE5
#define RP_FAT_ATTR_VOLUME 0x08
#define RP_FAT_ATTR_DIR 0x10
#define RP_SECTOR_BUFFER_SIZE 512

struct replay_protect_ctx_t {
	uint8_t sector[RP_SECTOR_BUFFER_SIZE];
	uint16_t bytes_per_sector;
	uint16_t root_entry_count;
	uint32_t root_start;
	uint16_t root_sectors;
	uint8_t drive;
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

static uint32_t replay_protect_mul_u16(uint16_t a, uint16_t b)
{
	uint32_t ret = 0;
	uint32_t add = a;

	while(b != 0) {
		if(b & 1) {
			ret += add;
		}
		add <<= 1;
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

static void replay_protect_invalidate(void)
{
	resident->unused_3[T3_REPLAY_RES_PROTECT_FLAGS_INDEX] |= (
		T3_REPLAY_RES_PROTECT_INVALID
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

static void replay_protect_local_reset(void)
{
}

static uint8_t replay_protect_current_drive(void)
{
	_AH = 0x19;
	geninterrupt(0x21);
	return _AL;
}

static bool replay_protect_abs_read_small(
	uint8_t drive, uint16_t sector, uint16_t count, void far *buffer
)
{
	_AL = drive;
	_CX = count;
	_DX = sector;
	asm {
		push	ds
		lds	bx, buffer
		int	25h
		popf
		pop	ds
		jnc	short rp_abs_small_ok
	}
	return false;
rp_abs_small_ok:
	return true;
}

static bool replay_protect_sector_read(
	replay_protect_ctx_t _ss *ctx, uint32_t sector
)
{
	if(sector > 0xFFFFUL) {
		return false;
	}
	return replay_protect_abs_read_small(
		ctx->drive, static_cast<uint16_t>(sector), 1, ctx->sector
	);
}

static bool replay_protect_volume_init(replay_protect_ctx_t _ss *ctx)
{
	uint16_t reserved;
	uint8_t fat_count;
	uint16_t sectors_per_fat;
	uint32_t root_bytes;

	ctx->drive = replay_protect_current_drive();
	if(!replay_protect_abs_read_small(ctx->drive, 0, 1, ctx->sector)) {
		return false;
	}

	ctx->bytes_per_sector = replay_protect_u16(ctx->sector + 0x0B);
	reserved = replay_protect_u16(ctx->sector + 0x0E);
	fat_count = ctx->sector[0x10];
	ctx->root_entry_count = replay_protect_u16(ctx->sector + 0x11);
	sectors_per_fat = replay_protect_u16(ctx->sector + 0x16);

	if(
		(ctx->bytes_per_sector != RP_SECTOR_BUFFER_SIZE) ||
		(reserved == 0) ||
		(fat_count == 0) ||
		(ctx->root_entry_count == 0) ||
		(sectors_per_fat == 0)
	) {
		return false;
	}

	ctx->root_start = (
		static_cast<uint32_t>(reserved) +
		replay_protect_mul_u16(fat_count, sectors_per_fat)
	);
	root_bytes = (static_cast<uint32_t>(ctx->root_entry_count) << 5);
	ctx->root_sectors = static_cast<uint16_t>(
		(root_bytes + (RP_SECTOR_BUFFER_SIZE - 1)) >> 9
	);
	if(
		(ctx->root_sectors == 0) ||
		((ctx->root_start + ctx->root_sectors) > 0x10000UL)
	) {
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

static bool replay_protect_root_size_read(
	const char far *fn, uint32_t far *size
)
{
	replay_protect_ctx_t ctx;
	char short_name[11];
	const char far *base;
	uint32_t sector;
	uint16_t offset;
	uint8_t attr;
	uint8_t first;

	if(!replay_protect_volume_init(&ctx)) {
		return false;
	}
	base = replay_protect_basename(fn);
	if(!replay_protect_short_name_component(short_name, base)) {
		return false;
	}

	for(sector = 0; sector < ctx.root_sectors; sector++) {
		if(!replay_protect_sector_read(&ctx, ctx.root_start + sector)) {
			return false;
		}
		for(offset = 0; offset < RP_SECTOR_BUFFER_SIZE; offset += 32) {
			first = ctx.sector[offset];
			if(first == RP_FAT_ENTRY_FREE) {
				return false;
			}
			if(first == RP_FAT_ENTRY_DELETED) {
				continue;
			}
			attr = ctx.sector[offset + 0x0B];
			if(attr & (RP_FAT_ATTR_VOLUME | RP_FAT_ATTR_DIR)) {
				continue;
			}
			if(replay_protect_name_match(ctx.sector + offset, short_name)) {
				*size = replay_protect_u32(ctx.sector + offset + 0x1C);
				replay_protect_location_set(ctx.root_start + sector, offset);
				return true;
			}
		}
	}
	return false;
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
		(offset > (RP_SECTOR_BUFFER_SIZE - 32)) ||
		((offset & 0x1F) != 0)
	) {
		return false;
	}
	base = replay_protect_basename(fn);
	if(!replay_protect_short_name_component(short_name, base)) {
		return false;
	}

	ctx.drive = replay_protect_current_drive();
	if(!replay_protect_sector_read(&ctx, sector)) {
		return false;
	}
	first = ctx.sector[offset];
	if((first == RP_FAT_ENTRY_FREE) || (first == RP_FAT_ENTRY_DELETED)) {
		return false;
	}
	attr = ctx.sector[offset + 0x0B];
	if(attr & (RP_FAT_ATTR_VOLUME | RP_FAT_ATTR_DIR)) {
		return false;
	}
	if(!replay_protect_name_match(ctx.sector + offset, short_name)) {
		return false;
	}
	*size = replay_protect_u32(ctx.sector + offset + 0x1C);
	return true;
}

static bool replay_protect_size_read(const char far *guard_fn, uint32_t far *size)
{
	if(replay_protect_located_size_read(guard_fn, size)) {
		return true;
	}
	return replay_protect_root_size_read(guard_fn, size);
}

static bool replay_protect_verify(const char far *guard_fn)
{
	uint32_t disk_size;

	if(replay_protect_invalid()) {
		return false;
	}
	if(!replay_protect_size_read(guard_fn, &disk_size)) {
		replay_protect_invalidate();
		return false;
	}
	if(disk_size != replay_protect_committed_size()) {
		replay_protect_invalidate();
		return false;
	}
	return true;
}

static bool replay_protect_commit_handle(void)
{
	_BX = static_cast<uint16_t>(file_Handle);
	_AH = 0x68;
	asm {
		int	21h
		jnc	short rp_commit_ok
	}
	return false;
rp_commit_ok:
	return true;
}

static bool replay_protect_commit_current_file(void)
{
	file_flush();
	if(file_ErrorStat != 0) {
		replay_protect_invalidate();
		return false;
	}
	if(!replay_protect_commit_handle()) {
		replay_protect_invalidate();
		return false;
	}
	return true;
}

static bool replay_protect_guard_create(const char far *guard_fn)
{
	uint32_t disk_size;

	replay_protect_committed_size_set(0);
	if(!file_create(guard_fn)) {
		replay_protect_invalidate();
		return false;
	}
	if(!replay_protect_commit_current_file()) {
		file_close();
		return false;
	}
	file_close();
	if(!replay_protect_size_read(guard_fn, &disk_size)) {
		replay_protect_invalidate();
		return false;
	}
	if(disk_size != 0) {
		replay_protect_invalidate();
		return false;
	}
	return true;
}

static bool replay_protect_checkpoint(const char far *guard_fn)
{
	uint32_t expected_size;
	uint32_t disk_size;
	uint8_t byte = 0;

	if(!replay_protect_verify(guard_fn)) {
		return false;
	}
	expected_size = replay_protect_committed_size() + 1;
	if(!file_append(guard_fn)) {
		replay_protect_invalidate();
		return false;
	}
	file_seek(replay_protect_committed_size(), SEEK_SET);
	if(file_write(&byte, sizeof(byte)) == 0) {
		file_close();
		replay_protect_invalidate();
		return false;
	}
	if(!replay_protect_commit_current_file()) {
		file_close();
		return false;
	}
	file_close();

	if(!replay_protect_size_read(guard_fn, &disk_size)) {
		replay_protect_invalidate();
		return false;
	}
	if(disk_size != expected_size) {
		replay_protect_invalidate();
		return false;
	}
	replay_protect_committed_size_set(expected_size);
	return true;
}

#undef RP_FAT_ENTRY_FREE
#undef RP_FAT_ENTRY_DELETED
#undef RP_FAT_ATTR_VOLUME
#undef RP_FAT_ATTR_DIR
#undef RP_SECTOR_BUFFER_SIZE

#endif /* TH03_REPLAY_PROTECT_HPP */
