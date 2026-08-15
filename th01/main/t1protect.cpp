/* ReC98 — harness/ORACLE-TH01-MASTER (MOD BRANCH, NOT byte-identical)
 * -------------------------------------------------------------------
 * TH01 savestate-protection detector — implementation.
 *
 * Included by th01/main/t1case.cpp AFTER its file-access helpers and the
 * carrier pointer, which this file uses. Not a separate translation unit: the
 * whole oracle module is one TU, last in link order, per
 * REPLAY_CORE_CONTRACT.md §1 item 3.
 *
 * Rationale, divergences and the full reference reading: th01/t1protect.hpp
 * and state/notes/t1case-protect.md.
 */

/// Carrier accessors — the whole re-binding surface
/// ------------------------------------------------
/// The reference reaches `resident->unused_3[i]` from 14 places. Here every one
/// funnels through these five, so `protect[]` has exactly one owner.

static uint8_t t1prt_u8(unsigned index)
{
	if(!t1case_res) {
		return 0;
	}
	return t1case_res->protect[index];
}

static void t1prt_u8_write(unsigned index, uint8_t value)
{
	if(!t1case_res) {
		return;
	}
	t1case_res->protect[index] = value;
}

static uint16_t t1prt_u16(unsigned index)
{
	if(!t1case_res) {
		return 0;
	}
	return static_cast<uint16_t>(
		t1case_res->protect[index] |
		(static_cast<uint16_t>(t1case_res->protect[index + 1]) << 8)
	);
}

static void t1prt_u16_write(unsigned index, uint16_t value)
{
	if(!t1case_res) {
		return;
	}
	t1case_res->protect[index + 0] = static_cast<uint8_t>(value);
	t1case_res->protect[index + 1] = static_cast<uint8_t>(value >> 8);
}

static uint32_t t1prt_u32(unsigned index)
{
	if(!t1case_res) {
		return 0;
	}
	return (
		static_cast<uint32_t>(t1case_res->protect[index + 0]) |
		(static_cast<uint32_t>(t1case_res->protect[index + 1]) << 8) |
		(static_cast<uint32_t>(t1case_res->protect[index + 2]) << 16) |
		(static_cast<uint32_t>(t1case_res->protect[index + 3]) << 24)
	);
}

static void t1prt_u32_write(unsigned index, uint32_t value)
{
	if(!t1case_res) {
		return;
	}
	t1case_res->protect[index + 0] = static_cast<uint8_t>(value);
	t1case_res->protect[index + 1] = static_cast<uint8_t>(value >> 8);
	t1case_res->protect[index + 2] = static_cast<uint8_t>(value >> 16);
	t1case_res->protect[index + 3] = static_cast<uint8_t>(value >> 24);
}

/// Diagnostics
/// -----------

static void t1prt_diag_code_set(uint8_t code)
{
	t1prt_u8_write(T1PRT_DIAG_CODE_INDEX, code);
}

// Never overwrite the FIRST failure with a later, less informative one.
static void t1prt_diag_code_set_if_none(uint8_t code)
{
	if(t1prt_u8(T1PRT_DIAG_CODE_INDEX) == T1PRT_NONE) {
		t1prt_diag_code_set(code);
	}
}

static void t1prt_diag_sizes_set(uint32_t expected, uint32_t actual)
{
	t1prt_u32_write(T1PRT_DIAG_EXPECTED_INDEX, expected);
	t1prt_u32_write(T1PRT_DIAG_ACTUAL_INDEX, actual);
}

static void t1prt_diag_location_set(uint32_t sector, uint16_t offset)
{
	t1prt_u32_write(T1PRT_DIAG_SECTOR_INDEX, sector);
	t1prt_u16_write(T1PRT_DIAG_OFFSET_INDEX, offset);
}

static void t1prt_diag_dos_ax_set(uint16_t ax)
{
	t1prt_u16_write(T1PRT_DIAG_DOS_AX_INDEX, ax);
}

static void t1prt_diag_int25_flags_set(uint16_t flags, uint16_t stack_flags)
{
	t1prt_u16_write(T1PRT_DIAG_I25_FLAGS_INDEX, flags);
	t1prt_u16_write(T1PRT_DIAG_I25_STACK_INDEX, stack_flags);
}

static void t1prt_diag_geometry_set(
	uint8_t drive, uint16_t bytes_per_sector, uint16_t root_entries,
	uint32_t root_start, uint16_t root_sectors
)
{
	t1prt_u8_write(T1PRT_DIAG_DRIVE_INDEX, drive);
	t1prt_u16_write(T1PRT_DIAG_BPS_INDEX, bytes_per_sector);
	t1prt_u16_write(T1PRT_DIAG_ROOT_ENTS_INDEX, root_entries);
	t1prt_u32_write(T1PRT_DIAG_ROOT_START_INDEX, root_start);
	t1prt_u16_write(T1PRT_DIAG_ROOT_SECS_INDEX, root_sectors);
}

/// Flags
/// -----

static uint8_t t1prt_flags(void)
{
	return t1prt_u8(T1PRT_FLAGS_INDEX);
}

static void t1prt_flag_set(uint8_t bit)
{
	t1prt_u8_write(T1PRT_FLAGS_INDEX, static_cast<uint8_t>(t1prt_flags() | bit));
}

static bool t1prt_invalid(void)
{
	return ((t1prt_flags() & T1PRT_FLAG_INVALID) != 0);
}

static bool t1prt_located(void)
{
	return ((t1prt_flags() & T1PRT_FLAG_LOCATED) != 0);
}

// Recording is disabled by EITHER verdict. Only one of them is an accusation.
static bool t1prt_blocked(void)
{
	return ((t1prt_flags() & (T1PRT_FLAG_INVALID | T1PRT_FLAG_ERROR)) != 0);
}

static void t1prt_invalidate(void)
{
	t1prt_flag_set(T1PRT_FLAG_INVALID);
}

static void t1prt_detector_error(void)
{
	t1prt_flag_set(T1PRT_FLAG_ERROR);
}

static void t1prt_location_set(uint32_t sector, uint16_t offset)
{
	t1prt_u32_write(T1PRT_GUARD_SECTOR_INDEX, sector);
	t1prt_u16_write(T1PRT_GUARD_OFFSET_INDEX, offset);
	t1prt_flag_set(T1PRT_FLAG_LOCATED);
}

// Zeroes everything the detector owns EXCEPT the buffer (which is not in the
// carrier here at all). Called only when a new run creates a new guard, so an
// INVALID verdict persists across every process transition until then.
static void t1prt_state_reset(void)
{
	unsigned i;

	if(!t1case_res) {
		return;
	}
	t1case_res->committed = 0;
	for(i = 0; i < T1PRT_PROTECT_END_INDEX; i++) {
		t1case_res->protect[i] = 0;
	}
}

static uint32_t t1prt_committed(void)
{
	return (t1case_res ? t1case_res->committed : 0);
}

static void t1prt_committed_set(uint32_t size)
{
	if(t1case_res) {
		t1case_res->committed = size;
	}
}

/// The sector buffer
/// -----------------
/// A module static, NOT a carrier field — see t1protect.hpp divergence 3. The
/// packet shares the allocation at a fixed offset so both far pointers have one
/// segment and the packet cannot straddle a 64 KB boundary.

static uint8_t far *t1prt_buffer;

static void t1prt_local_reset(void)
{
	t1prt_buffer = 0;
}

static void t1prt_local_free(void)
{
	if(t1prt_buffer) {
		farfree(t1prt_buffer);
		t1prt_buffer = 0;
	}
}

static bool t1prt_ctx_alloc(t1prt_ctx_t far *ctx)
{
	if(!t1prt_buffer) {
		t1prt_buffer = reinterpret_cast<uint8_t far *>(
			farmalloc(T1PRT_SECTOR_ALLOCATION_SIZE)
		);
		if(!t1prt_buffer) {
			t1prt_diag_code_set_if_none(T1PRT_CTX_ALLOC);
			return false;
		}
	}
	ctx->sector = t1prt_buffer;
	ctx->packet = (t1prt_buffer + T1PRT_SECTOR_BUFFER_SIZE_MAX);
	return true;
}

/// DOS environment probes
/// ----------------------

static uint8_t t1prt_current_drive(void)
{
	union REGS regs;

	regs.h.ah = 0x19;
	int86(0x21, &regs, &regs);
	return regs.h.al;
}

// AX=3000h returns the major in AL and the minor in AH. The extended absolute
// read is DOS 7.10 and later; below that, FAT32 and any sector above 0xFFFF are
// simply unreachable, which the module reports rather than silently failing.
static bool t1prt_extended_abs_read_available(void)
{
	union REGS regs;

	regs.x.ax = 0x3000;
	int86(0x21, &regs, &regs);
	return ((regs.h.al > 7) || ((regs.h.al == 7) && (regs.h.ah >= 10)));
}

static void t1prt_raw_init(t1prt_ctx_t far *ctx)
{
	ctx->drive = t1prt_current_drive();
	ctx->fat_type = T1PRT_FAT_UNKNOWN;
	ctx->extended_abs_read = t1prt_extended_abs_read_available();
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
static bool t1prt_abs_read_small(
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
	t1prt_diag_int25_flags_set(dos_flags, dos_stack_flags);
	if(dos_flags & 1) {
		t1prt_diag_dos_ax_set(dos_ax);
		return false;
	}
	t1prt_diag_dos_ax_set(0);
	return true;
}

// DOS 7.1 extended absolute read. Normal INT 21h convention, so NO extra stack
// word. The stack-flags diagnostic is explicitly zeroed so a dump distinguishes
// "went through 7305h" from "went through 25h".
static bool t1prt_abs_read_extended(
	t1prt_ctx_t far *ctx, uint32_t sector, uint16_t count, void far *buffer
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
	t1prt_diag_int25_flags_set(dos_flags, 0);
	if(dos_flags & 1) {
		t1prt_diag_dos_ax_set(dos_ax);
		return false;
	}
	t1prt_diag_dos_ax_set(0);
	return true;
}

// Preserve the proven FAT12/FAT16 path. FAT32 rejects old-form INT 25h, after
// which DOS 7.1's extended absolute read handles the same request. One sector
// at a time, always.
static bool t1prt_sector_read(t1prt_ctx_t far *ctx, uint32_t sector)
{
	if((ctx->fat_type == T1PRT_FAT32) || (sector > 0xFFFFUL)) {
		if(!ctx->extended_abs_read) {
			t1prt_diag_location_set(sector, 0);
			t1prt_diag_code_set_if_none(T1PRT_EXTENDED_READ_UNAVAILABLE);
			return false;
		}
		return t1prt_abs_read_extended(ctx, sector, 1, ctx->sector);
	}
	if(t1prt_abs_read_small(
		ctx->drive, static_cast<uint16_t>(sector), 1, ctx->sector
	)) {
		return true;
	}
	if(!ctx->extended_abs_read) {
		t1prt_diag_location_set(sector, 0);
		return false;
	}
	return t1prt_abs_read_extended(ctx, sector, 1, ctx->sector); // opportunistic
}

/// Geometry helpers
/// ----------------

static uint16_t t1prt_u16_at(const uint8_t far *p, unsigned offset)
{
	return static_cast<uint16_t>(
		p[offset] | (static_cast<uint16_t>(p[offset + 1]) << 8)
	);
}

static uint32_t t1prt_u32_at(const uint8_t far *p, unsigned offset)
{
	return (
		static_cast<uint32_t>(p[offset + 0]) |
		(static_cast<uint32_t>(p[offset + 1]) << 8) |
		(static_cast<uint32_t>(p[offset + 2]) << 16) |
		(static_cast<uint32_t>(p[offset + 3]) << 24)
	);
}

static bool t1prt_sector_size_ok(uint16_t bytes_per_sector)
{
	return (
		(bytes_per_sector == 512) || (bytes_per_sector == 1024) ||
		(bytes_per_sector == 2048) || (bytes_per_sector == 4096)
	);
}

// Hard-coded, never a division: a 32-bit divide would pull LDIV@ into the
// segment.
static uint8_t t1prt_power_of_two_shift(uint16_t value)
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
	case 2048: return 11;
	case 4096: return 12;
	}
	return 0xFF;
}

static bool t1prt_power_of_two(uint16_t value)
{
	return ((value != 0) && ((value & (value - 1)) == 0));
}

/// 8.3 names
/// ---------

static const char *t1prt_basename(const char *fn)
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

// Expands "T1LAST.GRD" into the 11-byte padded directory form.
static bool t1prt_short_name(const char *fn, uint8_t far *out)
{
	const char *p = t1prt_basename(fn);
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

static bool t1prt_name_eq(const uint8_t far *entry, const uint8_t far *name)
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

static bool t1prt_volume_init(t1prt_ctx_t far *ctx)
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

	if(!t1prt_ctx_alloc(ctx)) {
		return false;
	}
	t1prt_raw_init(ctx);
	if(!t1prt_sector_read(ctx, 0)) {
		t1prt_diag_code_set_if_none(T1PRT_BPB_READ);
		return false;
	}
	s = ctx->sector;
	ctx->bytes_per_sector = t1prt_u16_at(s, 0x0B);
	ctx->sectors_per_cluster = s[0x0D];
	reserved = t1prt_u16_at(s, 0x0E);
	fat_count = s[0x10];
	ctx->root_entry_count = t1prt_u16_at(s, 0x11);
	total_16 = t1prt_u16_at(s, 0x13);
	fat_16 = t1prt_u16_at(s, 0x16);
	ctx->total_sectors = (total_16 != 0) ? total_16 : t1prt_u32_at(s, 0x20);
	ctx->fat_sectors = (fat_16 != 0) ? fat_16 : t1prt_u32_at(s, 0x24);

	if(
		!t1prt_sector_size_ok(ctx->bytes_per_sector) ||
		!t1prt_power_of_two(ctx->sectors_per_cluster) ||
		(reserved == 0) || (fat_count == 0) ||
		(ctx->total_sectors == 0) || (ctx->fat_sectors == 0)
	) {
		t1prt_diag_geometry_set(
			ctx->drive, ctx->bytes_per_sector, ctx->root_entry_count, 0, 0
		);
		t1prt_diag_code_set_if_none(T1PRT_BPB_UNSUPPORTED);
		return false;
	}

	// Accumulate the FAT span with an overflow check PER FAT.
	fat_span = 0;
	for(i = 0; i < fat_count; i++) {
		if((fat_span + ctx->fat_sectors) < fat_span) {
			t1prt_diag_code_set_if_none(T1PRT_BPB_UNSUPPORTED);
			return false;
		}
		fat_span += ctx->fat_sectors;
	}
	root_base = (reserved + fat_span);
	if(root_base < fat_span) {
		t1prt_diag_code_set_if_none(T1PRT_BPB_UNSUPPORTED);
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
		t1prt_diag_code_set_if_none(T1PRT_BPB_UNSUPPORTED);
		return false;
	}

	data_sectors = (ctx->total_sectors - ctx->first_data_sector);
	shift = t1prt_power_of_two_shift(ctx->sectors_per_cluster);
	ctx->cluster_count = (data_sectors >> shift);
	ctx->fat_start = reserved;
	ctx->root_cluster = 0;

	if(ctx->cluster_count < T1PRT_FAT12_CLUSTER_COUNT_MAX) {
		ctx->fat_type = T1PRT_FAT12;
	} else if(ctx->cluster_count < T1PRT_FAT16_CLUSTER_COUNT_MAX) {
		ctx->fat_type = T1PRT_FAT16;
	} else {
		ctx->fat_type = T1PRT_FAT32;
	}

	if(ctx->fat_type == T1PRT_FAT32) {
		ext_flags = t1prt_u16_at(s, 0x28);
		if(ext_flags & 0x0080) {
			active_fat = static_cast<uint8_t>(ext_flags & 0x000F);
		}
		ctx->root_cluster = (t1prt_u32_at(s, 0x2C) & T1PRT_FAT32_ENTRY_MASK);
		if(
			(ctx->root_entry_count != 0) || (fat_16 != 0) ||
			!ctx->extended_abs_read || // FAT32 needs DOS >= 7.10; no fallback
			(active_fat >= fat_count) ||
			(ctx->cluster_count >= T1PRT_FAT32_CLUSTER_MAX) ||
			(ctx->root_cluster < 2) ||
			(ctx->root_cluster > (ctx->cluster_count + 1))
		) {
			t1prt_diag_code_set_if_none(T1PRT_BPB_UNSUPPORTED);
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
			t1prt_diag_code_set_if_none(T1PRT_BPB_UNSUPPORTED);
			return false;
		}
		ctx->root_start = root_base;
	}

	// Set BEFORE the range check, so a range failure still carries numbers.
	t1prt_diag_geometry_set(
		ctx->drive, ctx->bytes_per_sector, ctx->root_entry_count,
		ctx->root_start, static_cast<uint16_t>(ctx->root_sectors)
	);
	fat_end = (ctx->fat_start + ctx->fat_sectors);
	if(
		(ctx->root_start >= ctx->total_sectors) ||
		(fat_end > ctx->total_sectors)
	) {
		t1prt_diag_code_set_if_none(T1PRT_ROOT_RANGE);
		return false;
	}
	return true;
}

static bool t1prt_cluster_sector(
	t1prt_ctx_t far *ctx, uint32_t cluster, uint32_t far *sector
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

static int t1prt_root_sector_scan(
	t1prt_ctx_t far *ctx, uint32_t sector_index, const uint8_t far *name,
	uint32_t far *size, bool cache_location
)
{
	uint16_t offset;
	const uint8_t far *e;

	for(
		offset = 0;
		offset < ctx->bytes_per_sector;
		offset = static_cast<uint16_t>(offset + T1PRT_DIR_ENTRY_SIZE)
	) {
		e = (ctx->sector + offset);
		if(e[0] == T1PRT_FAT_ENTRY_FREE) {
			t1prt_diag_location_set(sector_index, offset);
			return T1PRT_SCAN_END; // end of directory: it is not here
		}
		if(e[0] == T1PRT_FAT_ENTRY_DELETED) {
			continue;
		}
		if(e[0x0B] & T1PRT_FAT_ATTR_SKIP) {
			continue;
		}
		if(t1prt_name_eq(e, name)) {
			*size = t1prt_u32_at(e, 0x1C);
			t1prt_diag_location_set(sector_index, offset);
			if(cache_location) {
				t1prt_location_set(sector_index, offset);
			}
			return T1PRT_SCAN_FOUND;
		}
	}
	return T1PRT_SCAN_CONTINUE;
}

static bool t1prt_fat32_next_cluster(
	t1prt_ctx_t far *ctx, uint32_t cluster, uint32_t far *next, bool far *eoc
)
{
	uint32_t fat_offset = (cluster << 2);
	uint32_t fat_sector;
	uint16_t offset;

	fat_sector = (
		ctx->fat_start +
		(fat_offset >> t1prt_power_of_two_shift(ctx->bytes_per_sector))
	);
	offset = static_cast<uint16_t>(fat_offset & (ctx->bytes_per_sector - 1));
	if(
		(fat_sector < ctx->fat_start) ||
		(fat_sector >= (ctx->fat_start + ctx->fat_sectors)) ||
		(offset > (ctx->bytes_per_sector - 4))
	) {
		t1prt_diag_location_set(fat_sector, offset);
		t1prt_diag_code_set_if_none(T1PRT_ROOT_CHAIN);
		return false;
	}
	if(!t1prt_sector_read(ctx, fat_sector)) {
		t1prt_diag_location_set(fat_sector, offset);
		t1prt_diag_code_set_if_none(T1PRT_ROOT_FAT_READ);
		return false;
	}
	*next = (t1prt_u32_at(ctx->sector, offset) & T1PRT_FAT32_ENTRY_MASK);
	*eoc = (*next >= T1PRT_FAT32_CLUSTER_EOC);
	if(*eoc) {
		return true;
	}
	if(
		(*next < 2) || (*next == T1PRT_FAT32_CLUSTER_BAD) ||
		(*next > (ctx->cluster_count + 1))
	) {
		t1prt_diag_sizes_set(cluster, *next);
		t1prt_diag_code_set_if_none(T1PRT_ROOT_CHAIN);
		return false;
	}
	return true;
}

static bool t1prt_root_size_read(const char *fn, uint32_t far *size)
{
	t1prt_ctx_t ctx;
	uint8_t name[11];
	uint32_t sector;
	uint32_t cluster;
	uint32_t next;
	uint32_t clusters_walked = 0;
	uint32_t i;
	bool eoc;
	int scan;

	if(!t1prt_short_name(fn, name)) {
		t1prt_diag_code_set_if_none(T1PRT_SHORT_NAME);
		return false;
	}
	if(!t1prt_volume_init(&ctx)) {
		return false;
	}
	if(ctx.fat_type != T1PRT_FAT32) {
		for(i = 0; i < ctx.root_sectors; i++) {
			if(!t1prt_sector_read(&ctx, ctx.root_start + i)) {
				t1prt_diag_code_set_if_none(T1PRT_ROOT_SECTOR_READ);
				return false;
			}
			scan = t1prt_root_sector_scan(
				&ctx, (ctx.root_start + i), name, size, true
			);
			if(scan == T1PRT_SCAN_FOUND) {
				return true;
			}
			if(scan == T1PRT_SCAN_END) {
				t1prt_diag_code_set_if_none(T1PRT_ROOT_NOT_FOUND);
				return false;
			}
		}
		t1prt_diag_code_set_if_none(T1PRT_ROOT_NOT_FOUND);
		return false;
	}

	cluster = ctx.root_cluster;
	for(;;) {
		if(!t1prt_cluster_sector(&ctx, cluster, &sector)) {
			t1prt_diag_sizes_set(2, cluster);
			t1prt_diag_code_set_if_none(T1PRT_ROOT_CHAIN);
			return false;
		}
		for(i = 0; i < ctx.sectors_per_cluster; i++) {
			if(!t1prt_sector_read(&ctx, (sector + i))) {
				t1prt_diag_location_set((sector + i), 0);
				t1prt_diag_code_set_if_none(T1PRT_ROOT_SECTOR_READ);
				return false;
			}
			scan = t1prt_root_sector_scan(
				&ctx, (sector + i), name, size, true
			);
			if(scan == T1PRT_SCAN_FOUND) {
				return true;
			}
			if(scan == T1PRT_SCAN_END) {
				t1prt_diag_code_set_if_none(T1PRT_ROOT_NOT_FOUND);
				return false;
			}
		}
		clusters_walked++;

		// THE LOOP GUARD, and it is checked BEFORE the FAT read. A chain longer
		// than the volume's cluster count is a cycle or a corrupt FAT. This is
		// the module's only unbounded loop and this is what bounds it.
		if(clusters_walked >= ctx.cluster_count) {
			t1prt_diag_sizes_set(ctx.cluster_count, clusters_walked);
			t1prt_diag_code_set_if_none(T1PRT_ROOT_CHAIN);
			return false;
		}
		if(!t1prt_fat32_next_cluster(&ctx, cluster, &next, &eoc)) {
			return false;
		}
		if(eoc) {
			break;
		}
		cluster = next;
	}
	t1prt_diag_code_set_if_none(T1PRT_ROOT_NOT_FOUND);
	return false;
}

// The fast path. Fully revalidates the cache before trusting it, and does NOT
// run volume_init — no BPB parse, so [fat_type] stays UNKNOWN and sector_read
// takes the INT 25h path unless the sector is above 0xFFFF. The offset bound is
// the compile-time buffer maximum, not the volume's sector size, because the
// geometry has deliberately not been read.
static bool t1prt_located_size_read(const char *fn, uint32_t far *size)
{
	t1prt_ctx_t ctx;
	uint8_t name[11];
	uint32_t sector = t1prt_u32(T1PRT_GUARD_SECTOR_INDEX);
	uint16_t offset = t1prt_u16(T1PRT_GUARD_OFFSET_INDEX);
	const uint8_t far *e;

	if(
		!t1prt_located() || (sector == 0) ||
		(offset > (T1PRT_SECTOR_BUFFER_SIZE_MAX - T1PRT_DIR_ENTRY_SIZE)) ||
		((offset & 0x1F) != 0)
	) {
		t1prt_diag_code_set_if_none(T1PRT_LOCATED_RANGE);
		return false;
	}
	if(!t1prt_short_name(fn, name)) {
		t1prt_diag_code_set_if_none(T1PRT_SHORT_NAME);
		return false;
	}
	if(!t1prt_ctx_alloc(&ctx)) {
		return false;
	}
	t1prt_raw_init(&ctx);
	if(!t1prt_sector_read(&ctx, sector)) {
		t1prt_diag_code_set_if_none(T1PRT_LOCATED_SECTOR_READ);
		return false;
	}
	e = (ctx.sector + offset);
	if(
		(e[0] == T1PRT_FAT_ENTRY_FREE) || (e[0] == T1PRT_FAT_ENTRY_DELETED) ||
		(e[0x0B] & T1PRT_FAT_ATTR_SKIP)
	) {
		t1prt_diag_code_set_if_none(T1PRT_LOCATED_ENTRY_BAD);
		return false;
	}
	if(!t1prt_name_eq(e, name)) {
		t1prt_diag_code_set_if_none(T1PRT_LOCATED_NAME);
		return false;
	}
	*size = t1prt_u32_at(e, 0x1C);
	t1prt_diag_location_set(sector, offset);
	return true;
}

static bool t1prt_size_read(const char *fn, uint32_t far *size)
{
	if(t1prt_located() && t1prt_located_size_read(fn, size)) {
		return true;
	}
	if(!t1prt_root_size_read(fn, size)) {
		return false;
	}

	// A cached probe that missed is RECOVERABLE. Without this reset the
	// T1PRT_LOCATED_* code stays in the carrier and the next _set_if_none
	// declines to overwrite it, mislabelling a later, real failure.
	t1prt_diag_code_set(T1PRT_NONE);
	return true;
}

// Reads the guard's marker byte — byte 0 of its first data cluster — and its
// directory size, both RAW. This is the read DOS cannot fake, and it is the
// whole detector.
static bool t1prt_guard_marker_read(
	const char *fn, uint8_t far *marker, uint32_t far *disk_size
)
{
	t1prt_ctx_t ctx;
	uint8_t name[11];
	uint32_t sector;
	uint32_t cluster;
	uint32_t data_sector;
	uint16_t offset;
	const uint8_t far *e;

	if(!t1prt_located()) {
		if(!t1prt_root_size_read(fn, disk_size)) { // primes the cache
			return false;
		}
	}
	if(!t1prt_short_name(fn, name)) {
		t1prt_diag_code_set_if_none(T1PRT_SHORT_NAME);
		return false;
	}
	if(!t1prt_volume_init(&ctx)) { // needs the geometry to map cluster->sector
		return false;
	}
	sector = t1prt_u32(T1PRT_GUARD_SECTOR_INDEX);
	offset = t1prt_u16(T1PRT_GUARD_OFFSET_INDEX);
	if(
		!t1prt_located() || (sector == 0) ||
		(offset > (ctx.bytes_per_sector - T1PRT_DIR_ENTRY_SIZE))
	) {
		t1prt_diag_code_set_if_none(T1PRT_LOCATED_RANGE);
		return false;
	}
	if(!t1prt_sector_read(&ctx, sector)) {
		t1prt_diag_code_set_if_none(T1PRT_LOCATED_SECTOR_READ);
		return false;
	}
	e = (ctx.sector + offset);
	if(
		(e[0] == T1PRT_FAT_ENTRY_FREE) || (e[0] == T1PRT_FAT_ENTRY_DELETED) ||
		(e[0x0B] & T1PRT_FAT_ATTR_SKIP)
	) {
		t1prt_diag_code_set_if_none(T1PRT_LOCATED_ENTRY_BAD);
		return false;
	}
	if(!t1prt_name_eq(e, name)) {
		t1prt_diag_code_set_if_none(T1PRT_LOCATED_NAME);
		return false;
	}
	*disk_size = t1prt_u32_at(e, 0x1C);
	if(*disk_size < T1PRT_GUARD_BASE_SIZE) {
		t1prt_diag_code_set_if_none(T1PRT_MARKER_CLUSTER);
		return false;
	}
	cluster = t1prt_u16_at(e, 0x1A);
	if(ctx.fat_type == T1PRT_FAT32) {
		cluster |= (static_cast<uint32_t>(t1prt_u16_at(e, 0x14)) << 16);
		cluster &= T1PRT_FAT32_ENTRY_MASK;
	}
	if(!t1prt_cluster_sector(&ctx, cluster, &data_sector)) {
		t1prt_diag_sizes_set(2, cluster);
		t1prt_diag_code_set_if_none(T1PRT_MARKER_CLUSTER);
		return false;
	}
	if(!t1prt_sector_read(&ctx, data_sector)) {
		t1prt_diag_code_set_if_none(T1PRT_MARKER_SECTOR_READ);
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

static uint16_t t1prt_current_psp(void)
{
	union REGS regs;

	regs.h.ah = 0x51; // undocumented, as the reference uses
	int86(0x21, &regs, &regs);
	return regs.x.bx;
}

static bool t1prt_commit_process(void)
{
	uint16_t dpl[11];
	uint16_t dos_ax = 0;
	uint16_t dos_flags = 1;
	int i;

	for(i = 0; i < 11; i++) {
		dpl[i] = 0;
	}
	dpl[10] = t1prt_current_psp();

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
		t1prt_diag_dos_ax_set(dos_ax);
		return false;
	}
	return true;
}

// close() + a second commit, so directory-entry updates made by the close are
// not left only in DOS's buffers.
static bool t1prt_close_and_commit(int fd, uint8_t code)
{
	close(fd);
	if(!t1prt_commit_process()) {
		t1prt_diag_code_set_if_none(T1PRT_CLOSE_COMMIT);
		t1prt_detector_error();
		return false;
	}
	(void)code;
	return true;
}

/// The guard file
/// --------------

static bool t1prt_guard_create(const char *fn)
{
	uint8_t marker = 0;
	uint8_t disk_marker;
	uint32_t disk_size;
	int fd;

	t1prt_state_reset();
	fd = t1f_create(fn);
	if(fd < 0) {
		t1prt_diag_code_set_if_none(T1PRT_GUARD_CREATE);
		t1prt_detector_error();
		return false;
	}
	if(!t1f_write(fd, &marker, 1)) {
		close(fd);
		t1prt_diag_code_set_if_none(T1PRT_MARKER_WRITE);
		t1prt_detector_error();
		return false;
	}
	(void)t1prt_commit_process();
	if(!t1prt_close_and_commit(fd, T1PRT_MARKER_WRITE)) {
		return false;
	}
	if(!t1prt_guard_marker_read(fn, &disk_marker, &disk_size)) {
		t1prt_detector_error();
		return false;
	}
	if(disk_size != T1PRT_GUARD_BASE_SIZE) {
		t1prt_diag_sizes_set(T1PRT_GUARD_BASE_SIZE, disk_size);
		t1prt_diag_code_set_if_none(T1PRT_GUARD_CREATE_NONZERO);
		t1prt_detector_error();
		return false;
	}
	if(disk_marker != 0) {
		t1prt_diag_sizes_set(0, disk_marker);
		t1prt_diag_code_set_if_none(T1PRT_MARKER_VERIFY);
		t1prt_detector_error();
		return false;
	}
	t1prt_committed_set(T1PRT_GUARD_BASE_SIZE);
	return true;
}

/// THE THREE-WAY VERDICT
/// ---------------------
/// Rows 3 and 4 share T1PRT_VERIFY_MISMATCH and the same argument order. What
/// separates an accusation from an I/O failure is the FLAG BIT and the SIGN of
/// (actual - expected). See t1protect.hpp's note on the code, and the acceptance
/// procedure in state/notes/t1case-protect.md.

// The ladder itself, factored out of the read so it can be driven directly.
// That is not a testing convenience bolted on: rows 3 and 4 differ only by a
// flag bit and the sign of (actual - expected), and a verdict that can only be
// reached through a successful raw disk read cannot be exercised at all on a
// host that has no FAT image. See t1prt_verdict_selftest().
static bool t1prt_verdict(uint8_t marker, uint32_t disk_size, uint32_t committed)
{
	if(marker != 0) {
		// Row 2: a previous run already poisoned this guard — a repeated load.
		t1prt_diag_sizes_set(0, marker);
		t1prt_diag_code_set(T1PRT_MARKER_VALUE);
		t1prt_invalidate();
		return false;
	}
	if(disk_size > committed) {
		// Row 3: THE DISK IS AHEAD OF RAM, so RAM was rolled back.
		t1prt_diag_sizes_set(committed, disk_size);
		t1prt_diag_code_set(T1PRT_VERIFY_MISMATCH);
		t1prt_invalidate();
		return false;
	}
	if(disk_size < committed) {
		// Row 4: a write failed. Same code, same order — but ERROR, not
		// INVALID, and actual < expected. NOT an accusation.
		t1prt_diag_sizes_set(committed, disk_size);
		t1prt_diag_code_set(T1PRT_VERIFY_MISMATCH);
		t1prt_detector_error();
		return false;
	}
	return true; // row 5: OK
}

static bool t1prt_guard_marker_verify(const char *fn)
{
	uint8_t marker;
	uint32_t disk_size;

	if(t1prt_blocked()) {
		return false; // already blocked; short-circuit, no new evidence
	}
	if(!t1prt_guard_marker_read(fn, &marker, &disk_size)) {
		t1prt_detector_error(); // row 1: I/O or parse failure
		return false;
	}
	return t1prt_verdict(marker, disk_size, t1prt_committed());
}

// THE POISON. Writes marker byte 1 into the guard's first DATA byte.
//
// The lseek(0) is what makes it durable and it must not be "simplified" into an
// append: seeking to 0 and overwriting neither extends the file nor allocates,
// so the write stays inside the already-allocated first cluster and touches no
// FAT sector. An append would allocate from a FAT the savestate is about to
// restore, and the poison would vanish with it.
static void t1prt_guard_marker_set(const char *fn)
{
	uint8_t marker = 1;
	uint8_t disk_marker;
	uint32_t disk_size;
	int fd;

	fd = t1f_update(fn);
	if(fd < 0) {
		t1prt_diag_code_set_if_none(T1PRT_MARKER_WRITE);
		t1prt_detector_error();
		return;
	}
	lseek(fd, 0L, SEEK_SET);
	if(!t1f_write(fd, &marker, 1)) {
		close(fd);
		t1prt_diag_code_set_if_none(T1PRT_MARKER_WRITE);
		t1prt_detector_error();
		return;
	}
	(void)t1prt_commit_process();
	if(!t1prt_close_and_commit(fd, T1PRT_MARKER_WRITE)) {
		return;
	}
	if(!t1prt_guard_marker_read(fn, &disk_marker, &disk_size)) {
		t1prt_detector_error();
		return;
	}
	if(disk_marker != 1) {
		t1prt_diag_sizes_set(1, disk_marker);
		t1prt_diag_code_set_if_none(T1PRT_MARKER_VERIFY);
		t1prt_detector_error();
		return;
	}

	// NOT bookkeeping — this is the accusation. Leaving the diagnostic that
	// reads as a detection, rather than one that reads as "poison written
	// successfully", is what makes the evidence legible afterwards.
	t1prt_diag_sizes_set(0, disk_marker);
	t1prt_diag_code_set(T1PRT_MARKER_VALUE);
}

// One checkpoint: verify FIRST, then append exactly one byte and prove the disk
// agrees.
static bool t1prt_checkpoint(const char *fn)
{
	uint32_t expected;
	uint32_t disk_size;
	bool committed_open;
	uint8_t byte = 0;
	int fd;

	if(!t1prt_guard_marker_verify(fn)) {
		return false; // flags and diagnostics already set by the verdict
	}
	expected = (t1prt_committed() + 1);
	fd = t1f_update(fn);
	if(fd < 0) {
		t1prt_diag_code_set_if_none(T1PRT_CHECKPOINT_APPEND);
		t1prt_detector_error();
		return false;
	}
	lseek(fd, static_cast<long>(t1prt_committed()), SEEK_SET);
	if(!t1f_write(fd, &byte, 1)) {
		close(fd);
		t1prt_diag_code_set_if_none(T1PRT_CHECKPOINT_WRITE);
		t1prt_detector_error();
		return false;
	}

	// Commit WHILE THE HANDLE IS STILL OPEN, then read the size raw.
	committed_open = false;
	if(t1prt_commit_process()) {
		if(t1prt_size_read(fn, &disk_size)) {
			committed_open = (disk_size == expected);
		}
	}
	if(!t1prt_close_and_commit(fd, T1PRT_CHECKPOINT_WRITE)) {
		return false;
	}
	if(!committed_open) {
		if(!t1prt_size_read(fn, &disk_size)) {
			t1prt_detector_error();
			return false;
		}
		if(disk_size != expected) {
			t1prt_diag_sizes_set(expected, disk_size);
			t1prt_diag_code_set_if_none(T1PRT_CHECKPOINT_MISMATCH);
			t1prt_detector_error();
			return false;
		}
	}
	t1prt_committed_set(expected);
	t1prt_diag_dos_ax_set(0);
	return true;
}

/// The verdict selftest
/// --------------------
/// Drives the verdict ladder over the four outcomes that matter and emits each
/// one's evidence tuple. Reachable in the SHIPPED image via `T1CASE.CFG` = "g",
/// for two reasons:
///
///  1. It is the only way to exercise rows 3, 4 and 5 on a host with no FAT
///     image. On a DOSBox-X mounted directory the BPB parse cannot succeed, so
///     every disk-driven path stops at row 1 and the interesting ladder is
///     unreachable. [emu] Measured: run `w3s3rec01` reports PRT code 3
///     (BPB_UNSUPPORTED) / flags 4 (ERROR).
///  2. It gives the human running the savestate test a NEGATIVE CONTROL. Before
///     trusting a green savestate result they can run this and confirm the
///     harness distinguishes the four tuples at all — a test whose power has
///     not been demonstrated proves nothing, which is the standing rule in
///     kb/conventions/agent-working-discipline.md.
///
/// Each case is emitted as the `PRT`/`PSZ` pair the host checker parses. The
/// flags accumulate across cases because they are sticky by design, so the
/// expected sequence is 0x00 -> 0x01 -> 0x05 -> 0x05, not four independent
/// values; the checker knows this.

static void t1prt_verdict_case(
	char tag, uint8_t marker, uint32_t disk_size, uint32_t committed
)
{
	bool ok = t1prt_verdict(marker, disk_size, committed);

	t1case_diag(
		'P', 'V', tag,
		static_cast<uint32_t>(t1prt_u8(T1PRT_DIAG_CODE_INDEX)),
		static_cast<uint32_t>(t1prt_flags())
	);
	t1case_diag(
		'P', 'S', 'Z',
		t1prt_u32(T1PRT_DIAG_EXPECTED_INDEX),
		t1prt_u32(T1PRT_DIAG_ACTUAL_INDEX)
	);
	t1case_diag('P', 'V', 'R', (ok ? 1UL : 0UL), 0);
}

static void t1prt_verdict_selftest(void)
{
	t1case_paths_init();
	if(!t1case_res_open(true)) {
		return;
	}
	t1prt_state_reset();

	// Row 5 — OK. disk == committed, marker clean.
	t1prt_verdict_case('5', 0, 64, 64);

	// Row 3 — THE ROLLBACK. The disk is ahead of RAM. INVALID (flag 0x01), and
	// actual > expected.
	t1prt_state_reset();
	t1prt_verdict_case('3', 0, 65, 64);

	// Row 4 — A FAILED WRITE. Same code as row 3, same argument order; ERROR
	// (flag 0x04) and actual < expected. This pair is the whole point.
	t1prt_state_reset();
	t1prt_verdict_case('4', 0, 63, 64);

	// Row 2 — ALREADY POISONED. A repeated savestate load.
	t1prt_state_reset();
	t1prt_verdict_case('2', 1, 64, 64);

	// The poison's DURABILITY property, which is testable without a savestate
	// and without a FAT image: writing the marker must NOT extend the guard.
	// `lseek(0, SEEK_SET)` is what keeps the write inside the already-allocated
	// first cluster; an append would allocate from a FAT a savestate is about to
	// restore and the poison would vanish with it. Both calls below will report
	// a raw-read failure on a host with no readable FAT — that is expected and
	// irrelevant here, because the DOS-side write still happens and the RESULTING
	// FILE is the evidence: it must be exactly 1 byte, and that byte must be 1.
	t1prt_state_reset();
	(void)t1prt_guard_create(T1CASE_GUARD_FN);
	t1prt_guard_marker_set(T1CASE_GUARD_FN);
	t1prt_state_reset();

	t1case_done_write(T1T_OK_SELFTEST);
}

/// The T1DIAG.TXT sink
/// -------------------
/// A parsed text record, not a raw carrier dump: the acceptance gate has to be a
/// program. `PRT <code> <flags>` and `PSZ <expected> <actual>` together are the
/// tuple §6.1 requires — the code alone cannot separate a rollback from a failed
/// write.

static void t1prt_diag_emit(void)
{
	t1case_diag(
		'P', 'R', 'T',
		static_cast<uint32_t>(t1prt_u8(T1PRT_DIAG_CODE_INDEX)),
		static_cast<uint32_t>(t1prt_flags())
	);
	t1case_diag(
		'P', 'S', 'Z',
		t1prt_u32(T1PRT_DIAG_EXPECTED_INDEX),
		t1prt_u32(T1PRT_DIAG_ACTUAL_INDEX)
	);
	t1case_diag(
		'P', 'L', 'C',
		t1prt_u32(T1PRT_DIAG_SECTOR_INDEX),
		static_cast<uint32_t>(t1prt_u16(T1PRT_DIAG_OFFSET_INDEX))
	);
}
