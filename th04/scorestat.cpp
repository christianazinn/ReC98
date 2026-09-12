#pragma option -zCSCORESTAT_TEXT -zPSCORESTAT_TEXT

#include <stddef.h>
#include "platform.h"
#include "x86real.h"
#if (GAME != 1)
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#endif
#include "th01/rank.h"
#include "th04/scorestat.hpp"

#if (GAME == 1)
#include "th01/hardware/vsync.hpp"
#define SCORESTAT_VSYNC z_vsync_Count2
#include "th01/formats/scoredat.hpp"
#include "th01/hardware/grppsafx.h"
#include "th01/score.h"
#include "shiftjis.hpp"
#include "th01/v_colors.hpp"
#elif (GAME == 2)
// Only the stock wire length is needed. The native scoredat header imports
// dos.h, which conflicts with pc98_gfx.hpp's x86real.h declarations.
#elif (GAME == 4)
#include "th02/v_colors.hpp"
#include "th04/formats/scoredat/scoredat.hpp"
#include "th04/hardware/grppsafx.h"
#include "th04/playchar.h"
#elif (GAME == 5)
#include "th02/v_colors.hpp"
#include "th04/formats/scoredat/scoredat.hpp"
#include "th04/hardware/grppsafx.h"
#include "th05/playchar.h"
#else
#error scorestat.cpp supports TH01, TH02, TH04, and TH05 only
#endif

#if (GAME != 1)
#define SCORESTAT_VSYNC vsync_Count2
#endif

#define SCORESTAT_VERSION 1
#define SCORESTAT_ACTIVE 0x01
#define SCORESTAT_HAD_CONTINUE 0x02
#define SCORESTAT_ACTIVE_FLAGS (SCORESTAT_ACTIVE | SCORESTAT_HAD_CONTINUE)
#define SCORESTAT_SLOT_NONE 0xFF
#define SCORESTAT_RECORD_COUNT 2

static int scorestat_fd;

// Tail-local DOS I/O, matching the replay adapters. No master.lib file state
// or CRT file code is pulled into any executable by statistics.
static bool scorestat_open(const char far *fn, uint8_t access)
{
	unsigned fn_seg = FP_SEG(fn);
	unsigned fn_off = FP_OFF(fn);
	int result;
	_asm {
		push ds;
		mov dx, fn_off;
		mov ds, fn_seg;
		mov ah, 3Dh;
		mov al, access;
		int 21h;
		pop ds;
		sbb dx, dx;
		or ax, dx;
		mov result, ax;
	}
	scorestat_fd = result;
	return (result >= 0);
}

static void scorestat_close(void)
{
	int fd = scorestat_fd;
	_asm {
		mov bx, fd;
		mov ah, 3Eh;
		int 21h;
	}
}

static long scorestat_seek(long offset, uint8_t origin)
{
	unsigned offset_hi = static_cast<unsigned>(offset >> 16);
	unsigned offset_lo = static_cast<unsigned>(offset);
	unsigned result_hi;
	unsigned result_lo;
	unsigned failed;
	int fd = scorestat_fd;
	_asm {
		mov bx, fd;
		mov cx, offset_hi;
		mov dx, offset_lo;
		mov ah, 42h;
		mov al, origin;
		int 21h;
		mov result_lo, ax;
		mov result_hi, dx;
		sbb ax, ax;
		mov failed, ax;
	}
	if(failed) {
		return -1;
	}
	return (static_cast<uint32_t>(result_hi) << 16) | result_lo;
}

static bool scorestat_transfer(const void far *buf, unsigned size, uint8_t op)
{
	unsigned buf_seg = FP_SEG(buf);
	unsigned buf_off = FP_OFF(buf);
	unsigned result;
	int fd = scorestat_fd;
	_asm {
		push ds;
		mov bx, fd;
		mov cx, size;
		mov dx, buf_off;
		mov ds, buf_seg;
		mov ah, op;
		int 21h;
		pop ds;
		sbb cx, cx;
		not cx;
		and ax, cx;
		mov result, ax;
	}
	return (result == size);
}

#if (GAME == 1)
	#define SCORESTAT_SLOT_COUNT 1
	#define SCORESTAT_STOCK_SIZE ( \
		(sizeof(SCOREDAT_MAGIC) - 1) + SCOREDAT_NAMES_SIZE + \
		(sizeof(score_t) * SCOREDAT_PLACES) + \
		(sizeof(int16_t) * SCOREDAT_PLACES) + \
		(sizeof(shiftjis_kanji_t) * SCOREDAT_PLACES) \
	)
#elif (GAME == 2)
	#define SCORESTAT_SLOT_COUNT RANK_COUNT
	#define SCORESTAT_STOCK_SIZE (182L * RANK_COUNT)
#elif (GAME == 4)
	#define SCORESTAT_SLOT_COUNT (RANK_COUNT * PLAYCHAR_COUNT)
	#define SCORESTAT_STOCK_SIZE ( \
		sizeof(scoredat_section_t) * RANK_COUNT * PLAYCHAR_COUNT \
	)
#elif (GAME == 5)
	#define SCORESTAT_SLOT_COUNT (RANK_COUNT * PLAYCHAR_COUNT)
	#define SCORESTAT_STOCK_SIZE ( \
		sizeof(scoredat_section_t) * RANK_COUNT * PLAYCHAR_COUNT \
	)
#endif

#pragma option -a1
struct scorestat_record_t {
	char magic[8];
	uint8_t version;
	uint8_t game;
	uint16_t size;
	uint32_t generation;
	uint8_t slot_count;
	uint8_t active_slot;
	uint8_t active_flags;
	uint8_t reserved;
	scorestat_totals_t totals[SCORESTAT_SLOT_COUNT];
	uint32_t checksum;
};
#pragma option -a2

typedef char scorestat_record_size_check[
	(sizeof(scorestat_record_t) == (24 + (12 * SCORESTAT_SLOT_COUNT))) ? 1 : -1
];

scorestat_totals_t scorestat_view;

static scorestat_record_t scorestat_record;
static scorestat_record_t scorestat_disk_records[SCORESTAT_RECORD_COUNT];
static bool scorestat_loaded;
static uint8_t scorestat_loaded_rank;
static int8_t scorestat_disk_slot;
static long scorestat_pair_offset;
static bool scorestat_process_tracking;
static uint16_t scorestat_process_vsync_last;

// Construct text from immediates to keep initialized DATA empty.
static bool scorestat_filename(char *fn, uint8_t rank)
{
#if (GAME == 1)
	if(rank > RANK_LUNATIC) {
		return false;
	}
	fn[0] = 'R'; fn[1] = 'E'; fn[2] = 'Y'; fn[3] = 'H'; fn[4] = 'I';
	if(rank == RANK_EASY) {
		fn[5] = 'E'; fn[6] = 'S';
	} else if(rank == RANK_NORMAL) {
		fn[5] = 'N'; fn[6] = 'O';
	} else if(rank == RANK_HARD) {
		fn[5] = 'H'; fn[6] = 'A';
	} else {
		fn[5] = 'L'; fn[6] = 'U';
	}
	fn[7] = '.'; fn[8] = 'D'; fn[9] = 'A'; fn[10] = 'T'; fn[11] = '\0';
#elif (GAME == 2)
	(void)rank;
	fn[0] = 'h'; fn[1] = 'u'; fn[2] = 'u'; fn[3] = 'h'; fn[4] = 'i';
	fn[5] = '.'; fn[6] = 'd'; fn[7] = 'a'; fn[8] = 't'; fn[9] = '\0';
#else
	(void)rank;
	fn[0] = 'G'; fn[1] = 'E'; fn[2] = 'N'; fn[3] = 'S'; fn[4] = 'O';
	fn[5] = 'U'; fn[6] = '.'; fn[7] = 'S'; fn[8] = 'C'; fn[9] = 'R';
	fn[10] = '\0';
#endif
	return true;
}

static void scorestat_magic_init(char far *magic)
{
	magic[0] = 'T'; magic[1] = ('0' + GAME);
	magic[2] = 'S'; magic[3] = 'T'; magic[4] = 'A'; magic[5] = 'T';
	magic[6] = '0'; magic[7] = '1';
}

static uint32_t scorestat_add_saturating(uint32_t left, uint32_t right)
{
	if((0xFFFFFFFFUL - left) < right) {
		return 0xFFFFFFFFUL;
	}
	return (left + right);
}

static uint32_t scorestat_checksum(
	const uint8_t far *data, unsigned size
)
{
	uint16_t a = 0xA5A5;
	uint16_t b = 0x5A5A;

	while(size--) {
		a += *data++;
		b += a;
	}
	return (static_cast<uint32_t>(b) << 16) | a;
}

static bool scorestat_magic_valid(const char far *magic)
{
	char expected[8];
	scorestat_magic_init(expected);
	for(unsigned i = 0; i < sizeof(scorestat_record.magic); i++) {
		if(magic[i] != expected[i]) {
			return false;
		}
	}
	return true;
}

static bool scorestat_record_valid(const scorestat_record_t far& record)
{
	if(
		!scorestat_magic_valid(record.magic) ||
		(record.version != SCORESTAT_VERSION) ||
		(record.game != GAME) ||
		(record.size != sizeof(record)) ||
		(record.slot_count != SCORESTAT_SLOT_COUNT) ||
		(record.reserved != 0) ||
		(record.active_flags & ~SCORESTAT_ACTIVE_FLAGS) ||
		(record.checksum != scorestat_checksum(
			reinterpret_cast<const uint8_t far *>(&record),
			offsetof(scorestat_record_t, checksum)
		))
	) {
		return false;
	}
	if(record.active_flags & SCORESTAT_ACTIVE) {
		return (record.active_slot < SCORESTAT_SLOT_COUNT);
	}
	return (
		(record.active_slot == SCORESTAT_SLOT_NONE) &&
		(record.active_flags == 0)
	);
}

static void scorestat_record_initialize(void)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(&scorestat_record);
	unsigned i;
	for(i = 0; i < sizeof(scorestat_record); i++) {
		p[i] = 0;
	}
	scorestat_magic_init(scorestat_record.magic);
	scorestat_record.version = SCORESTAT_VERSION;
	scorestat_record.game = GAME;
	scorestat_record.size = sizeof(scorestat_record);
	scorestat_record.slot_count = SCORESTAT_SLOT_COUNT;
	scorestat_record.active_slot = SCORESTAT_SLOT_NONE;
}

// Whole-struct assignment can pull f_scopy into the stock CRT code segment.
static void scorestat_record_copy(
	scorestat_record_t far& destination, const scorestat_record_t far& source
)
{
	uint8_t far *out = reinterpret_cast<uint8_t far *>(&destination);
	const uint8_t far *in = reinterpret_cast<const uint8_t far *>(&source);
	for(unsigned i = 0; i < sizeof(scorestat_record_t); i++) {
		out[i] = in[i];
	}
}

static bool scorestat_slot_for(
	uint8_t rank, uint8_t owner, uint8_t& slot
)
{
	if(rank >= RANK_COUNT) {
		return false;
	}
#if (GAME == 1)
	(void)owner;
	slot = 0;
#elif (GAME == 2)
	(void)owner;
	slot = rank;
#else
	if(owner >= PLAYCHAR_COUNT) {
		return false;
	}
	slot = static_cast<uint8_t>((owner * RANK_COUNT) + rank);
#endif
	return true;
}

static bool scorestat_load(uint8_t rank)
{
	char fn[12];
	long file_bytes;
	long pair_bytes = (sizeof(scorestat_record_t) * SCORESTAT_RECORD_COUNT);
	bool valid[SCORESTAT_RECORD_COUNT];

	if(rank >= RANK_COUNT) {
		return false;
	}
	if(scorestat_loaded && (scorestat_loaded_rank == rank)) {
		return true;
	}
	if(!scorestat_filename(fn, rank) || !scorestat_open(fn, 0)) {
		return false;
	}
	file_bytes = scorestat_seek(0, 2);
	if(file_bytes < static_cast<long>(SCORESTAT_STOCK_SIZE)) {
		scorestat_close();
		return false;
	}
	valid[0] = false;
	valid[1] = false;
	if(file_bytes >= (static_cast<long>(SCORESTAT_STOCK_SIZE) + pair_bytes)) {
		if((scorestat_seek((file_bytes - pair_bytes), 0) ==
			(file_bytes - pair_bytes)) && scorestat_transfer(
			scorestat_disk_records, static_cast<unsigned>(pair_bytes), 0x3F
		)) {
			valid[0] = scorestat_record_valid(scorestat_disk_records[0]);
			valid[1] = scorestat_record_valid(scorestat_disk_records[1]);
		}
	}
	scorestat_close();

	scorestat_pair_offset = file_bytes;
	scorestat_disk_slot = -1;
	if(valid[0] || valid[1]) {
		scorestat_pair_offset = (file_bytes - pair_bytes);
		if(!valid[1] || (valid[0] &&
			(static_cast<int32_t>(scorestat_disk_records[0].generation -
			 scorestat_disk_records[1].generation) > 0))) {
			scorestat_record_copy(scorestat_record, scorestat_disk_records[0]);
			scorestat_disk_slot = 0;
		} else {
			scorestat_record_copy(scorestat_record, scorestat_disk_records[1]);
			scorestat_disk_slot = 1;
		}
	} else {
		scorestat_record_initialize();
	}
	scorestat_loaded = true;
	scorestat_loaded_rank = rank;
	return true;
}

static bool scorestat_save(void)
{
	char fn[12];
	long file_bytes;
	long pair_bytes = (sizeof(scorestat_record_t) * SCORESTAT_RECORD_COUNT);
	long target_offset;
	int target_slot;
	uint32_t generation;
	bool append_pair;
	bool ok;

	if(!scorestat_loaded) {
		return false;
	}
	if(!scorestat_filename(fn, scorestat_loaded_rank) || !scorestat_open(fn, 2)) {
		return false;
	}
	file_bytes = scorestat_seek(0, 2);
	if(file_bytes < static_cast<long>(SCORESTAT_STOCK_SIZE)) {
		scorestat_close();
		return false;
	}

	append_pair = (
		(scorestat_disk_slot < 0) ||
		((scorestat_pair_offset + pair_bytes) != file_bytes)
	);
	target_slot = (append_pair ? 1 : (1 - scorestat_disk_slot));
	scorestat_record_t& candidate = scorestat_disk_records[target_slot];
	scorestat_record_copy(candidate, scorestat_record);
	generation = (scorestat_record.generation + 1);
	if(generation == 0) {
		generation = 1;
	}
	candidate.generation = generation;
	candidate.checksum = scorestat_checksum(
		reinterpret_cast<const uint8_t far *>(&candidate),
		offsetof(scorestat_record_t, checksum)
	);

	if(append_pair) {
		scorestat_record_t& base = scorestat_disk_records[0];
		scorestat_record_copy(base, scorestat_record);
		base.generation = (generation - 1);
		base.checksum = scorestat_checksum(
			reinterpret_cast<const uint8_t far *>(&base),
			offsetof(scorestat_record_t, checksum)
		);
		target_offset = file_bytes;
		ok = (
			(scorestat_seek(target_offset, 0) == target_offset) &&
			scorestat_transfer(&base, sizeof(base), 0x40) &&
			scorestat_transfer(&candidate, sizeof(candidate), 0x40)
		);
	} else {
		target_offset = (
			scorestat_pair_offset + (target_slot * sizeof(scorestat_record_t))
		);
		ok = ((scorestat_seek(target_offset, 0) == target_offset) &&
			scorestat_transfer(&candidate, sizeof(candidate), 0x40));
	}
	scorestat_close();
	if(!ok) {
		return false;
	}
	if(append_pair) {
		scorestat_pair_offset = file_bytes;
	}
	scorestat_disk_slot = target_slot;
	scorestat_record_copy(scorestat_record, candidate);
	return true;
}

bool16 far scorestat_run_begin(uint8_t rank, uint8_t owner)
{
	uint8_t slot;
	if(!scorestat_slot_for(rank, owner, slot) || !scorestat_load(rank)) {
		return false;
	}
	scorestat_record.active_slot = slot;
	scorestat_record.active_flags = SCORESTAT_ACTIVE;
	return scorestat_save();
}

void far scorestat_process_enter(
	uint8_t rank, uint8_t owner, bool16 story_eligible
)
{
	uint8_t slot;
	scorestat_process_tracking = false;
	scorestat_process_vsync_last = SCORESTAT_VSYNC;
	if(
		!story_eligible || !scorestat_slot_for(rank, owner, slot) ||
		!scorestat_load(rank)
	) {
		return;
	}
	scorestat_process_tracking = (
		(scorestat_record.active_flags & SCORESTAT_ACTIVE) &&
		(scorestat_record.active_slot == slot)
	);
}

void far scorestat_process_sync(void)
{
	uint16_t now = SCORESTAT_VSYNC;
	uint16_t elapsed = static_cast<uint16_t>(
		now - scorestat_process_vsync_last
	);
	scorestat_process_vsync_last = now;
	if(!scorestat_process_tracking) {
		return;
	}
	scorestat_totals_t& totals = scorestat_record.totals[
		scorestat_record.active_slot
	];
	totals.play_frames = scorestat_add_saturating(
		totals.play_frames, elapsed
	);
}

void far scorestat_process_rebase(void)
{
	scorestat_process_vsync_last = SCORESTAT_VSYNC;
}

void far scorestat_process_checkpoint(void)
{
	scorestat_process_sync();
	if(scorestat_process_tracking) {
		scorestat_save();
	}
}

void far scorestat_continue_accept(void)
{
	if(!scorestat_process_tracking) {
		return;
	}
	scorestat_process_sync();
	scorestat_totals_t& totals = scorestat_record.totals[
		scorestat_record.active_slot
	];
	totals.continues_used = scorestat_add_saturating(
		totals.continues_used, 1
	);
	scorestat_record.active_flags |= SCORESTAT_HAD_CONTINUE;
	scorestat_save();
}

void far scorestat_run_complete(void)
{
	if(!scorestat_process_tracking) {
		return;
	}
	scorestat_process_sync();
	if(!(scorestat_record.active_flags & SCORESTAT_HAD_CONTINUE)) {
		scorestat_totals_t& totals = scorestat_record.totals[
			scorestat_record.active_slot
		];
		totals.one_ccs = scorestat_add_saturating(totals.one_ccs, 1);
	}
	scorestat_record.active_slot = SCORESTAT_SLOT_NONE;
	scorestat_record.active_flags = 0;
	scorestat_save();
	scorestat_process_tracking = false;
}

void far scorestat_run_end(void)
{
	if(!scorestat_process_tracking) {
		return;
	}
	scorestat_process_sync();
	scorestat_record.active_slot = SCORESTAT_SLOT_NONE;
	scorestat_record.active_flags = 0;
	scorestat_save();
	scorestat_process_tracking = false;
}

bool16 far scorestat_view_load(uint8_t rank)
{
	scorestat_view.play_frames = 0;
	scorestat_view.one_ccs = 0;
	scorestat_view.continues_used = 0;
	if(!scorestat_load(rank)) {
		return false;
	}
#if (GAME == 1)
	uint8_t first = 0;
	uint8_t count = 1;
#elif (GAME == 2)
	uint8_t first = rank;
	uint8_t count = 1;
#else
	uint8_t first = rank;
	uint8_t count = PLAYCHAR_COUNT;
#endif
	for(uint8_t owner = 0; owner < count; owner++) {
#if (GAME <= 2)
		uint8_t slot = first;
#else
		uint8_t slot = static_cast<uint8_t>(
			first + (owner * RANK_COUNT)
		);
#endif
		scorestat_view.play_frames = scorestat_add_saturating(
			scorestat_view.play_frames,
			scorestat_record.totals[slot].play_frames
		);
		scorestat_view.one_ccs = scorestat_add_saturating(
			scorestat_view.one_ccs,
			scorestat_record.totals[slot].one_ccs
		);
		scorestat_view.continues_used = scorestat_add_saturating(
			scorestat_view.continues_used,
			scorestat_record.totals[slot].continues_used
		);
	}
	return true;
}

#if (GAME == 1) || (BINARY == 'O')
static int scorestat_text_fixed(
	char *line, int at, uint32_t value, int digits
)
{
	int end = (at + digits);
	if(value > 9999UL) {
		value = 9999UL;
	}
	while(digits--) {
		line[at + digits] = static_cast<char>('0' + (value % 10));
		value /= 10;
	}
	return end;
}

static void scorestat_view_text(char *line)
{
	uint32_t frames = scorestat_view.play_frames;
	uint32_t seconds = ((frames / 282UL) * 5UL) +
		(((frames % 282UL) * 5UL) / 282UL);
	uint32_t hours = (seconds / 3600UL);
	line[0] = 'P'; line[1] = 'L'; line[2] = 'A'; line[3] = 'Y'; line[4] = ' ';
	int at = 5;
	at = scorestat_text_fixed(line, at, hours, 4);
	line[at++] = ':';
	at = scorestat_text_fixed(line, at, ((seconds / 60UL) % 60UL), 2);
	line[at++] = ':';
	at = scorestat_text_fixed(line, at, (seconds % 60UL), 2);
	line[at++] = ' '; line[at++] = ' '; line[at++] = '1';
	line[at++] = 'C'; line[at++] = 'C'; line[at++] = ' ';
	at = scorestat_text_fixed(line, at, scorestat_view.one_ccs, 4);
	line[at++] = ' '; line[at++] = ' '; line[at++] = 'C';
	line[at++] = 'O'; line[at++] = 'N'; line[at++] = 'T'; line[at++] = ' ';
	at = scorestat_text_fixed(line, at, scorestat_view.continues_used, 4);
	line[at] = '\0';
}

void far scorestat_view_format(uint8_t rank, char *line)
{
	scorestat_view_load(rank);
	scorestat_view_text(line);
}

#if (GAME != 1)
void far scorestat_view_put(uint8_t rank)
{
	char line[48];
	scorestat_view_format(rank, line);
#if (GAME == 2)
	text_putsa(22, 19, line, TX_GREEN);
#else
	graph_putsa_fx_func_t previous_func = graph_putsa_fx_func;
	pixel_t previous_spacing = graph_putsa_fx_spacing;
	graph_putsa_fx_func = FX_WEIGHT_BOLD;
	graph_putsa_fx_spacing = 16;
	#if (GAME == 4)
		#define SCORESTAT_VIEW_TOP 304
	#else
		#define SCORESTAT_VIEW_TOP 336
	#endif
	graph_putsa_fx(178, (SCORESTAT_VIEW_TOP + 2), 0, line);
	graph_putsa_fx(176, SCORESTAT_VIEW_TOP, V_WHITE, line);
	graph_putsa_fx_func = previous_func;
	graph_putsa_fx_spacing = previous_spacing;
#endif
}

#if (GAME == 2)
extern char rank;
int far pascal scorestat_clip(int left, int top, int right, int bottom)
{
	int result = grc_setclip(left, top, right, bottom);
	scorestat_view_put(rank);
	return result;
}
#else
extern uint8_t rank;
void far pascal scorestat_rank_put(int left, int top, int patnum)
{
	super_put(left, top, patnum);
	scorestat_view_put(rank);
}
#endif
#endif
#endif

void far scorestat_stock_save_prepare(uint8_t rank)
{
	if(scorestat_load(rank)) {
		scorestat_process_checkpoint();
	}
}

void far scorestat_stock_save_finish(void)
{
	if(scorestat_loaded) {
		scorestat_save();
	}
}
