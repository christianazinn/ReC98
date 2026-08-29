#ifndef TH01_RP_GUARD_HPP
#define TH01_RP_GUARD_HPP

/*
 * TH01 replay savestate guard.
 *
 * This is deliberately header-local: REIIDEN and FUUIN are separate EXEs,
 * while t1replay_guard_t is their checksummed resident carrier. Keeping the
 * raw FAT implementation in their isolated replay tails avoids a shared
 * build-system edit and preserves the T1RPY4 on-disk ABI.
 */

#include "platform.h"
#include "x86real.h"
#include "th01/replay_format.hpp"

#define T1RPG_FP_SEG(p) ((unsigned)(((unsigned long)(void far *)(p)) >> 16))
#define T1RPG_FP_OFF(p) ((unsigned)((unsigned long)(void far *)(p)))

#define T1REPLAY_GUARD_INTERVAL_SAMPLES 128UL
#define T1REPLAY_GUARD_FLAG_INVALID 0x01
#define T1REPLAY_GUARD_FLAG_ERROR 0x02
#define T1REPLAY_GUARD_FLAG_LOCATED 0x04
#define T1REPLAY_GUARD_FLAGS_KNOWN ( \
	T1REPLAY_GUARD_FLAG_INVALID | T1REPLAY_GUARD_FLAG_ERROR | \
	T1REPLAY_GUARD_FLAG_LOCATED \
)

#define T1RPG_SECTOR_SIZE_MAX 1024
#define T1RPG_DIR_ENTRY_SIZE 32
#define T1RPG_GUARD_BASE_SIZE 1UL

struct t1rpg_volume_t {
	uint8_t drive;
	uint8_t sectors_per_cluster;
	uint16_t bytes_per_sector;
	uint16_t root_start;
	uint16_t root_sectors;
	uint16_t first_data_sector;
	uint32_t total_sectors;
};

// One sector per EXE is sufficient. The compact guard intentionally supports
// only the FAT12/16 fixed-root media used by the supported TH01 installs.
static uint8_t t1rpg_sector[T1RPG_SECTOR_SIZE_MAX];

static void t1rpg_filename(char *fn)
{
	fn[0] = '\\';
	fn[1] = 'T'; fn[2] = '1'; fn[3] = 'L'; fn[4] = 'A';
	fn[5] = 'S'; fn[6] = 'T'; fn[7] = '.';
	fn[8] = 'G'; fn[9] = 'R'; fn[10] = 'D'; fn[11] = '\0';
}

static int t1rpg_file_create(const char *fn)
{
	unsigned fn_seg = T1RPG_FP_SEG(fn);
	unsigned fn_off = T1RPG_FP_OFF(fn);
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

static int t1rpg_file_update(const char *fn)
{
	unsigned fn_seg = T1RPG_FP_SEG(fn);
	unsigned fn_off = T1RPG_FP_OFF(fn);
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

static void t1rpg_file_close(int fd)
{
	asm {
		mov bx, fd
		mov ah, 3Eh
		int 21h
	}
}

static bool t1rpg_file_seek(int fd, uint32_t pos)
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

static bool t1rpg_file_write(int fd, const void far *buf, unsigned size)
{
	unsigned buf_seg = T1RPG_FP_SEG(buf);
	unsigned buf_off = T1RPG_FP_OFF(buf);
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

static void t1rpg_file_delete(const char *fn)
{
	unsigned fn_seg = T1RPG_FP_SEG(fn);
	unsigned fn_off = T1RPG_FP_OFF(fn);

	asm {
		push ds
		mov  dx, fn_off
		mov  ds, fn_seg
		mov  ah, 41h
		int  21h
		pop  ds
	}
}

static uint8_t t1rpg_current_drive(void)
{
	uint8_t drive;

	asm {
		mov ah, 19h
		int 21h
		mov drive, al
	}
	return drive;
}

// INT 25h leaves an extra FLAGS word on the stack. This wrapper preserves the
// registers the DOS path may clobber and removes that word explicitly.
static bool t1rpg_sector_read(uint8_t drive, uint16_t sector)
{
	uint16_t flags = 1;
	void far *buffer = t1rpg_sector;

	_AL = drive;
	_CX = 1;
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
		pop  ax
		pop  dx
		pop  ds
		pop  es
		pop  di
		pop  si
		pop  bp
		mov  flags, ax
	}
	return ((flags & 1) == 0);
}

static uint16_t t1rpg_u16(const uint8_t far *p, unsigned offset)
{
	return static_cast<uint16_t>(
		p[offset] | (static_cast<uint16_t>(p[offset + 1]) << 8)
	);
}

static uint32_t t1rpg_u32(const uint8_t far *p, unsigned offset)
{
	return (
		static_cast<uint32_t>(p[offset + 0]) |
		(static_cast<uint32_t>(p[offset + 1]) << 8) |
		(static_cast<uint32_t>(p[offset + 2]) << 16) |
		(static_cast<uint32_t>(p[offset + 3]) << 24)
	);
}

static bool t1rpg_volume_open(t1rpg_volume_t *volume)
{
	const uint8_t far *bpb;
	uint16_t reserved;
	uint16_t root_entries;
	uint16_t total_16;
	uint16_t fat_sectors;
	uint16_t root_sectors;
	uint16_t root_start;
	uint16_t first_data;
	uint32_t fat_span;
	uint8_t fat_count;

	volume->drive = t1rpg_current_drive();
	if(!t1rpg_sector_read(volume->drive, 0)) {
		return false;
	}
	bpb = t1rpg_sector;
	volume->bytes_per_sector = t1rpg_u16(bpb, 0x0B);
	volume->sectors_per_cluster = bpb[0x0D];
	reserved = t1rpg_u16(bpb, 0x0E);
	fat_count = bpb[0x10];
	root_entries = t1rpg_u16(bpb, 0x11);
	total_16 = t1rpg_u16(bpb, 0x13);
	fat_sectors = t1rpg_u16(bpb, 0x16);
	volume->total_sectors = (
		(total_16 != 0) ? static_cast<uint32_t>(total_16) :
		t1rpg_u32(bpb, 0x20)
	);
	if(
		((volume->bytes_per_sector != 512) &&
		 (volume->bytes_per_sector != 1024)) ||
		(volume->sectors_per_cluster == 0) ||
		((volume->sectors_per_cluster &
		  (volume->sectors_per_cluster - 1)) != 0) ||
		(fat_count == 0) || (root_entries == 0) || (fat_sectors == 0) ||
		(volume->total_sectors == 0)
	) {
		return false;
	}
	// A fixed root directory and a nonzero FAT16 size deliberately exclude
	// FAT32. Both supported TH01 HDI families meet this narrower contract.
	if(volume->bytes_per_sector == 512) {
		root_sectors = static_cast<uint16_t>((root_entries + 15) >> 4);
	} else {
		root_sectors = static_cast<uint16_t>((root_entries + 31) >> 5);
	}
	if(root_sectors == 0) {
		return false;
	}
	fat_span = static_cast<uint32_t>(fat_count) * fat_sectors;
	if(
		(fat_span > 0xFFFFUL) ||
		(reserved > static_cast<uint16_t>(0xFFFFUL - fat_span))
	) {
		return false;
	}
	root_start = static_cast<uint16_t>(reserved + fat_span);
	if(root_start > static_cast<uint16_t>(0xFFFF - root_sectors)) {
		return false;
	}
	first_data = static_cast<uint16_t>(root_start + root_sectors);
	if(
		(static_cast<uint32_t>(first_data) >= volume->total_sectors) ||
		(root_start == 0)
	) {
		return false;
	}
	volume->root_start = root_start;
	volume->root_sectors = root_sectors;
	volume->first_data_sector = first_data;
	return true;
}

static bool t1rpg_name_matches(const uint8_t far *entry)
{
	return (
		(entry[0] == 'T') && (entry[1] == '1') &&
		(entry[2] == 'L') && (entry[3] == 'A') &&
		(entry[4] == 'S') && (entry[5] == 'T') &&
		(entry[6] == ' ') && (entry[7] == ' ') &&
		(entry[8] == 'G') && (entry[9] == 'R') &&
		(entry[10] == 'D')
	);
}

static bool t1rpg_entry_live(const uint8_t far *entry)
{
	return (
		(entry[0] != 0x00) && (entry[0] != 0xE5) &&
		((entry[0x0B] & 0x18) == 0) && t1rpg_name_matches(entry)
	);
}

static bool t1rpg_locate(
	t1replay_guard_t far *guard, const t1rpg_volume_t *volume
)
{
	uint16_t sector;
	uint16_t offset;
	const uint8_t far *entry;

	for(sector = volume->root_start;
		sector < static_cast<uint16_t>(
			volume->root_start + volume->root_sectors
		);
		sector++
	) {
		if(!t1rpg_sector_read(volume->drive, sector)) {
			return false;
		}
		for(offset = 0;
			offset <= static_cast<uint16_t>(
				volume->bytes_per_sector - T1RPG_DIR_ENTRY_SIZE
			);
			offset += T1RPG_DIR_ENTRY_SIZE
		) {
			entry = (t1rpg_sector + offset);
			if(entry[0] == 0x00) {
				return false;
			}
			if(t1rpg_entry_live(entry)) {
				guard->root_sector = sector;
				guard->root_offset = offset;
				guard->flags |= T1REPLAY_GUARD_FLAG_LOCATED;
				return true;
			}
		}
	}
	return false;
}

static bool t1rpg_entry_read(
	t1replay_guard_t far *guard, const t1rpg_volume_t *volume,
	const uint8_t far **entry_out
)
{
	const uint8_t far *entry;

	if(
		((guard->flags & T1REPLAY_GUARD_FLAG_LOCATED) == 0) ||
		(guard->root_sector < volume->root_start) ||
		(guard->root_sector >= static_cast<uint16_t>(
			volume->root_start + volume->root_sectors
		)) ||
		(guard->root_offset > static_cast<uint16_t>(
			volume->bytes_per_sector - T1RPG_DIR_ENTRY_SIZE
		)) ||
		((guard->root_offset & 0x1F) != 0)
	) {
		guard->flags &= ~T1REPLAY_GUARD_FLAG_LOCATED;
		if(!t1rpg_locate(guard, volume)) {
			return false;
		}
	}
	if(!t1rpg_sector_read(volume->drive, guard->root_sector)) {
		return false;
	}
	entry = (t1rpg_sector + guard->root_offset);
	if(!t1rpg_entry_live(entry)) {
		guard->flags &= ~T1REPLAY_GUARD_FLAG_LOCATED;
		if(!t1rpg_locate(guard, volume) ||
			!t1rpg_sector_read(volume->drive, guard->root_sector)) {
			return false;
		}
		entry = (t1rpg_sector + guard->root_offset);
		if(!t1rpg_entry_live(entry)) {
			return false;
		}
	}
	*entry_out = entry;
	return true;
}

static bool t1rpg_marker_read(
	t1replay_guard_t far *guard, uint8_t far *marker,
	uint32_t far *disk_size
)
{
	t1rpg_volume_t volume;
	const uint8_t far *entry;
	uint16_t cluster;
	uint32_t data_sector;

	if(!t1rpg_volume_open(&volume) || !t1rpg_entry_read(guard, &volume, &entry)) {
		return false;
	}
	*disk_size = t1rpg_u32(entry, 0x1C);
	cluster = t1rpg_u16(entry, 0x1A);
	if((*disk_size < T1RPG_GUARD_BASE_SIZE) || (cluster < 2)) {
		return false;
	}
	data_sector = (
		static_cast<uint32_t>(volume.first_data_sector) +
		(static_cast<uint32_t>(cluster - 2) * volume.sectors_per_cluster)
	);
	if((data_sector >= volume.total_sectors) || (data_sector > 0xFFFFUL)) {
		return false;
	}
	if(!t1rpg_sector_read(volume.drive, static_cast<uint16_t>(data_sector))) {
		return false;
	}
	*marker = t1rpg_sector[0];
	return true;
}

// AX=5D01h commits every open file of this PSP. This forces DOS directory
// cache state to disk before the raw FAT read that validates the checkpoint.
static uint16_t t1rpg_current_psp(void)
{
	uint16_t psp;

	asm {
		mov ah, 51h
		int 21h
		mov psp, bx
	}
	return psp;
}

// AX=5D01h is an undocumented process-wide commit. It is absent from the
// supported PC-98 DOS 3.x family, where the documented AH=0Dh disk reset is
// the available cache flush. Every caller validates the result through the
// raw FAT path immediately afterward, so this fallback cannot turn a cached
// DOS view into an accepted guard checkpoint.
static void t1rpg_disk_reset(void)
{
	asm {
		mov ah, 0Dh
		int 21h
	}
}

static bool t1rpg_commit_process(void)
{
	uint16_t dpl[11];
	uint16_t flags = 1;
	int i;

	for(i = 0; i < 11; i++) {
		dpl[i] = 0;
	}
	dpl[10] = t1rpg_current_psp();
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
		pop  ax
		pop  ds
		pop  es
		pop  di
		pop  si
		pop  bp
		mov  flags, ax
	}
	if((flags & 1) == 0) {
		return true;
	}
	t1rpg_disk_reset();
	return true;
}

static void t1rpg_error(t1replay_guard_t far *guard)
{
	guard->flags |= T1REPLAY_GUARD_FLAG_ERROR;
}

static void t1rpg_invalid(t1replay_guard_t far *guard)
{
	guard->flags |= T1REPLAY_GUARD_FLAG_INVALID;
}

static bool t1rpg_blocked(const t1replay_guard_t far *guard)
{
	return ((guard->flags &
		(T1REPLAY_GUARD_FLAG_INVALID | T1REPLAY_GUARD_FLAG_ERROR)) != 0);
}

static bool t1rpg_state_valid(const t1replay_guard_t far *guard)
{
	return (
		(guard->committed_size >= T1RPG_GUARD_BASE_SIZE) &&
		(guard->root_sector != 0) &&
		((guard->root_offset & 0x1F) == 0) &&
		(guard->root_offset <=
			static_cast<uint16_t>(T1RPG_SECTOR_SIZE_MAX - T1RPG_DIR_ENTRY_SIZE)) &&
		(guard->flags == T1REPLAY_GUARD_FLAG_LOCATED) &&
		(guard->reserved[0] == 0) && (guard->reserved[1] == 0) &&
		(guard->reserved[2] == 0)
	);
}

static bool t1rpg_state_empty(const t1replay_guard_t far *guard)
{
	unsigned i;
	const uint8_t far *bytes = reinterpret_cast<const uint8_t far *>(guard);

	for(i = 0; i < sizeof(*guard); i++) {
		if(bytes[i] != 0) {
			return false;
		}
	}
	return true;
}

static bool t1rpg_verdict(
	t1replay_guard_t far *guard, uint8_t marker, uint32_t disk_size
)
{
	if(marker != 0) {
		t1rpg_invalid(guard);
		return false;
	}
	if(disk_size > guard->committed_size) {
		t1rpg_invalid(guard);
		return false;
	}
	if(disk_size < guard->committed_size) {
		t1rpg_error(guard);
		return false;
	}
	return true;
}

static bool t1rpg_verify(t1replay_guard_t far *guard)
{
	uint8_t marker;
	uint32_t disk_size;

	if(t1rpg_blocked(guard)) {
		return false;
	}
	if(!t1rpg_marker_read(guard, &marker, &disk_size)) {
		t1rpg_error(guard);
		return false;
	}
	return t1rpg_verdict(guard, marker, disk_size);
}

// Overwrite byte zero rather than append. It remains in the first allocated
// data sector, so a later RAM/filesystem-cache restore cannot undo the poison.
static void t1rpg_poison(t1replay_guard_t far *guard)
{
	char fn[12];
	uint8_t marker = 1;
	uint8_t disk_marker;
	uint32_t disk_size;
	int fd;

	t1rpg_filename(fn);
	fd = t1rpg_file_update(fn);
	if((fd < 0) || !t1rpg_file_seek(fd, 0) ||
		!t1rpg_file_write(fd, &marker, 1)) {
		if(fd >= 0) {
			t1rpg_file_close(fd);
		}
		t1rpg_error(guard);
		return;
	}
	if(!t1rpg_commit_process()) {
		t1rpg_file_close(fd);
		t1rpg_error(guard);
		return;
	}
	t1rpg_file_close(fd);
	if(!t1rpg_commit_process() ||
		!t1rpg_marker_read(guard, &disk_marker, &disk_size) ||
		(disk_marker != 1)) {
		t1rpg_error(guard);
	}
}

static bool t1rpg_checkpoint(t1replay_guard_t far *guard)
{
	char fn[12];
	uint8_t byte = 0;
	uint8_t marker;
	uint32_t expected;
	uint32_t disk_size;
	int fd;

	if(!t1rpg_verify(guard)) {
		if(guard->flags & T1REPLAY_GUARD_FLAG_INVALID) {
			t1rpg_poison(guard);
		}
		return false;
	}
	expected = guard->committed_size + 1;
	if(expected == 0) {
		t1rpg_error(guard);
		return false;
	}
	t1rpg_filename(fn);
	fd = t1rpg_file_update(fn);
	if((fd < 0) || !t1rpg_file_seek(fd, guard->committed_size) ||
		!t1rpg_file_write(fd, &byte, 1)) {
		if(fd >= 0) {
			t1rpg_file_close(fd);
		}
		t1rpg_error(guard);
		return false;
	}
	if(!t1rpg_commit_process()) {
		t1rpg_file_close(fd);
		t1rpg_error(guard);
		return false;
	}
	t1rpg_file_close(fd);
	if(!t1rpg_commit_process() ||
		!t1rpg_marker_read(guard, &marker, &disk_size) ||
		(marker != 0) || (disk_size != expected)) {
		t1rpg_error(guard);
		return false;
	}
	guard->committed_size = expected;
	return true;
}

static bool t1rpg_begin(t1replay_guard_t far *guard)
{
	char fn[12];
	uint8_t marker = 0;
	uint8_t disk_marker;
	uint32_t disk_size;
	int fd;
	unsigned i;

	for(i = 0; i < sizeof(*guard); i++) {
		reinterpret_cast<uint8_t far *>(guard)[i] = 0;
	}
	t1rpg_filename(fn);
	t1rpg_file_delete(fn);
	if(!t1rpg_commit_process()) {
		t1rpg_error(guard);
		return false;
	}
	fd = t1rpg_file_create(fn);
	if((fd < 0) || !t1rpg_file_write(fd, &marker, 1)) {
		if(fd >= 0) {
			t1rpg_file_close(fd);
		}
		t1rpg_error(guard);
		return false;
	}
	if(!t1rpg_commit_process()) {
		t1rpg_file_close(fd);
		t1rpg_error(guard);
		return false;
	}
	t1rpg_file_close(fd);
	if(!t1rpg_commit_process() ||
		!t1rpg_marker_read(guard, &disk_marker, &disk_size) ||
		(disk_marker != 0) || (disk_size != T1RPG_GUARD_BASE_SIZE)) {
		t1rpg_error(guard);
		return false;
	}
	guard->committed_size = T1RPG_GUARD_BASE_SIZE;
	return true;
}

static bool t1rpg_sample(t1replay_guard_t far *guard)
{
	if(guard->sample_count == 0xFFFFFFFFUL) {
		t1rpg_error(guard);
		return false;
	}
	guard->sample_count++;
	if((guard->sample_count % T1REPLAY_GUARD_INTERVAL_SAMPLES) == 0) {
		return t1rpg_checkpoint(guard);
	}
	return !t1rpg_blocked(guard);
}

static void t1rpg_end(t1replay_guard_t far *guard)
{
	char fn[12];

	// A poisoned guard deliberately survives an invalid run. A fresh recording
	// deletes and recreates it; deleting it here would let an older savestate
	// escape the repeated-load poison.
	if(t1rpg_blocked(guard)) {
		return;
	}
	t1rpg_filename(fn);
	t1rpg_file_delete(fn);
	(void)t1rpg_commit_process();
}

#endif /* TH01_RP_GUARD_HPP */
