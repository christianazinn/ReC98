#pragma option -zCSCOREFILE_TEXT -zPSCOREFILE_TEXT

#include <stddef.h>
#include "libs/master.lib/master.hpp"
#include "platform/x86real/flags.hpp"
#include "th03/replay_handoff.hpp"
#include "th03/resident.hpp"
#include "th03/scorefile.hpp"
#if (BINARY == 'L')
#include "th02/v_colors.hpp"
#include "th02/hardware/frmdelay.h"
#include "th03/formats/cdg.h"
#include "th03/formats/pi.hpp"
#include "th03/menu_font.hpp"
#endif

#define T3_SCORESTAT_MAGIC_0 'S'
#define T3_SCORESTAT_MAGIC_1 'T'
#define T3_SCORESTAT_VERSION 1
#define T3_SCORESTAT_ACTIVE 1
#define T3_SCORETIME_VERSION 1
#define T3_SCORETIME_INTERVAL 256UL

const char far t3_scorefile_fn[] = "YUME.NEM";
const char far t3_scorefile_temp_fn[] = "YUME.TMP";
const char far t3_scorefile_old_fn[] = "YUME.OLD";
const char far t3_scorefile_backup_fn[] = "YUME.BAK";
const char far t3_scorefile_backup_1_fn[] = "YUME.BK1";
const char far t3_scoretime_fn[] = "YUME.TIM";

static uint8_t scorestat_resident_u8(unsigned index)
{
	return static_cast<uint8_t>(resident->unused_3[index]);
}

static void scorestat_resident_u8_set(unsigned index, uint8_t value)
{
	resident->unused_3[index] = value;
}

static uint32_t scorestat_resident_u32(unsigned index)
{
	return (
		static_cast<uint32_t>(scorestat_resident_u8(index + 0)) |
		(static_cast<uint32_t>(scorestat_resident_u8(index + 1)) << 8) |
		(static_cast<uint32_t>(scorestat_resident_u8(index + 2)) << 16) |
		(static_cast<uint32_t>(scorestat_resident_u8(index + 3)) << 24)
	);
}

static void scorestat_resident_u32_set(unsigned index, uint32_t value)
{
	scorestat_resident_u8_set(index + 0, static_cast<uint8_t>(value));
	scorestat_resident_u8_set(index + 1, static_cast<uint8_t>(value >> 8));
	scorestat_resident_u8_set(index + 2, static_cast<uint8_t>(value >> 16));
	scorestat_resident_u8_set(index + 3, static_cast<uint8_t>(value >> 24));
}

bool16 far scorestat_active(void)
{
	return (
		(scorestat_resident_u8(T3_SCORESTAT_RES_MAGIC_0_INDEX) ==
		 T3_SCORESTAT_MAGIC_0) &&
		(scorestat_resident_u8(T3_SCORESTAT_RES_MAGIC_1_INDEX) ==
		 T3_SCORESTAT_MAGIC_1) &&
		(scorestat_resident_u8(T3_SCORESTAT_RES_VERSION_INDEX) ==
		 T3_SCORESTAT_VERSION) &&
		(scorestat_resident_u8(T3_SCORESTAT_RES_ACTIVE_INDEX) ==
		 T3_SCORESTAT_ACTIVE) &&
		(scorestat_resident_u8(T3_SCORESTAT_RES_RANK_INDEX) < RANK_COUNT) &&
		(scorestat_resident_u8(T3_SCORESTAT_RES_PLAYCHAR_INDEX) <
		 PLAYCHAR_COUNT)
	);
}

static void scorestat_clear(void)
{
	for(unsigned i = T3_SCORESTAT_RES_START_INDEX;
		i < T3_SCORESTAT_RES_END_INDEX; i++) {
		resident->unused_3[i] = 0;
	}
}

static uint32_t score_checksum_near(
	const uint8_t __ss *data, unsigned size
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

#pragma option -a1
struct scoretime_record_t {
	char magic[4];
	uint8_t version;
	uint8_t size;
	uint8_t rank;
	uint8_t playchar;
	uint32_t run_id;
	uint32_t frames;
	uint32_t checksum;
};
#pragma option -a2

typedef char scoretime_record_size_check[
	(sizeof(scoretime_record_t) == 20) ? 1 : -1
];

static bool scoretime_record_valid(const scoretime_record_t __ss& record)
{
	return (
		(record.magic[0] == 'T') &&
		(record.magic[1] == '3') &&
		(record.magic[2] == 'T') &&
		(record.magic[3] == 'M') &&
		(record.version == T3_SCORETIME_VERSION) &&
		(record.size == sizeof(record)) &&
		(record.rank < RANK_COUNT) &&
		(record.playchar < PLAYCHAR_COUNT) &&
		(record.run_id != 0) &&
		(record.checksum == score_checksum_near(
			reinterpret_cast<const uint8_t __ss *>(&record),
			offsetof(scoretime_record_t, checksum)
		))
	);
}

static void scorefile_delete(const char far *fn)
{
	asm {
		push	ds
		lds 	dx, fn
		mov 	ah, 41h
		int 	21h
		pop 	ds
	}
}

static bool scorefile_exists(const char far *fn)
{
	if(!file_ropen(fn)) {
		return false;
	}
	file_close();
	return true;
}

static uint16_t scorefile_current_psp(void)
{
	uint16_t psp = 0;
	asm {
		push	bp
		push	si
		push	di
		push	ds
		push	es
		mov 	ah, 51h
		int 	21h
		push	bx
		pop 	cx
		pop 	es
		pop 	ds
		pop 	di
		pop 	si
		pop 	bp
		mov 	psp, cx
	}
	return psp;
}

static bool scorefile_commit_process(void)
{
	uint16_t dpl[11];
	uint16_t dos_flags = 1;

	for(unsigned i = 0; i < 11; i++) {
		dpl[i] = 0;
	}
	dpl[10] = scorefile_current_psp();
	asm {
		push	bp
		push	si
		push	di
		push	es
		push	ds
		push	ss
		pop 	ds
		lea 	dx, dpl
		mov 	ax, 5D01h
		int 	21h
		pushf
		pop 	ax
		pop 	ds
		pop 	es
		pop 	di
		pop 	si
		pop 	bp
		mov 	dos_flags, ax
	}
	return ((dos_flags & 1) == 0);
}

static bool scoretime_write(void)
{
	scoretime_record_t record;
	scoretime_record_t empty;
	scoretime_record_t existing[2];
	uint8_t trailing;
	uint32_t frames;
	long offset;

	if(!scorestat_active()) {
		return false;
	}
	frames = scorestat_resident_u32(T3_SCORESTAT_RES_FRAMES_INDEX);
	record.magic[0] = 'T'; record.magic[1] = '3';
	record.magic[2] = 'T'; record.magic[3] = 'M';
	record.version = T3_SCORETIME_VERSION;
	record.size = sizeof(record);
	record.rank = scorestat_resident_u8(T3_SCORESTAT_RES_RANK_INDEX);
	record.playchar = scorestat_resident_u8(
		T3_SCORESTAT_RES_PLAYCHAR_INDEX
	);
	record.run_id = scorestat_resident_u32(T3_SCORESTAT_RES_RUN_ID_INDEX);
	record.frames = frames;
	record.checksum = score_checksum_near(
		reinterpret_cast<const uint8_t __ss *>(&record),
		offsetof(scoretime_record_t, checksum)
	);

	bool existing_ok = false;
	if(file_ropen(T3_SCORETIME_FN)) {
		existing_ok = (
			(file_read(existing, sizeof(existing)) == sizeof(existing)) &&
			(file_read(&trailing, sizeof(trailing)) == 0)
		);
		file_close();
	}
	if(!existing_ok) {
		scorefile_delete(T3_SCORETIME_FN);
		if(!file_create(T3_SCORETIME_FN)) {
			return false;
		}
		for(unsigned i = 0; i < sizeof(empty); i++) {
			reinterpret_cast<uint8_t __ss *>(&empty)[i] = 0;
		}
		if(!file_write(&empty, sizeof(empty)) ||
			!file_write(&empty, sizeof(empty))) {
			file_close();
			return false;
		}
		file_close();
	}
	if(!file_append(T3_SCORETIME_FN)) {
		return false;
	}
	offset = (((frames / T3_SCORETIME_INTERVAL) & 1) * sizeof(record));
	file_seek(offset, SEEK_SET);
	if(!file_write(&record, sizeof(record))) {
		file_close();
		return false;
	}
	file_close();
	(void)scorefile_commit_process();
	return true;
}

void far scorestat_process_enter(void)
{
	scorestat_resident_u32_set(T3_SCORESTAT_RES_PROCESS_VSYNC_INDEX, 0);
}

void far scorestat_process_sync(void)
{
	uint8_t replay_mode;
	uint16_t elapsed;
	uint16_t process_vsync_last;
	uint32_t frames_before;
	uint32_t frames;

	process_vsync_last = static_cast<uint16_t>(scorestat_resident_u32(
		T3_SCORESTAT_RES_PROCESS_VSYNC_INDEX
	));
	elapsed = static_cast<uint16_t>(
		vsync_Count2 - process_vsync_last
	);
	scorestat_resident_u32_set(
		T3_SCORESTAT_RES_PROCESS_VSYNC_INDEX, vsync_Count2
	);

	if(
		!scorestat_active() || (resident->game_mode != GM_STORY) ||
		(resident->demo_num != 0) || practice_game_active()
	) {
		return;
	}
	replay_mode = scorestat_resident_u8(T3_REPLAY_RES_MODE_INDEX);
	if(
		(replay_mode == T3_REPLAY_RES_MODE_PLAYBACK) ||
		(replay_mode == T3_REPLAY_RES_MODE_USER_PLAYBACK)
	) {
		return;
	}
	frames_before = scorestat_resident_u32(T3_SCORESTAT_RES_FRAMES_INDEX);
	frames = frames_before;
	if((0xFFFFFFFFUL - frames) < elapsed) {
		frames = 0xFFFFFFFFUL;
	} else {
		frames += elapsed;
	}
	scorestat_resident_u32_set(T3_SCORESTAT_RES_FRAMES_INDEX, frames);
	if(
		frames != frames_before &&
		((frames & ~(T3_SCORETIME_INTERVAL - 1)) !=
		 (frames_before & ~(T3_SCORETIME_INTERVAL - 1)))
	) {
		scoretime_write();
	}
}

void far scorestat_exit_checkpoint(void)
{
	scorestat_process_sync();
	if(scorestat_active()) {
		scoretime_write();
	}
}

#if (BINARY != 'M')

#include "th03/formats/scoredat.hpp"

#define T3_SCOREFILE_VERSION 1
#define T3_SCOREFILE_FLAG_EXTRA_UNLOCKED 0x01
#define T3_SCOREFILE_ENTRY_VALID 0x01
#define T3_SCOREFILE_STAGE_EMPTY 0xFF

#pragma option -a1
struct scorefile_entry_t {
	char name[T3_SCOREFILE_NAME_LEN];
	uint8_t score[T3_SCOREFILE_SCORE_DIGITS];
	uint8_t stage;
	uint8_t continues_used;
	uint8_t flags;
	uint8_t reserved;
};

struct scorefile_board_t {
	scorefile_entry_t entries[T3_SCOREFILE_PLACES];
	scorefile_stats_t stats;
	uint32_t checksum;
};

struct scorefile_header_t {
	char magic[8];
	uint16_t version;
	uint16_t header_size;
	uint32_t file_size;
	uint32_t generation;
	uint32_t last_run_id;
	uint16_t entry_size;
	uint16_t board_size;
	uint8_t rank_count;
	uint8_t playchar_count;
	uint8_t place_count;
	uint8_t flags;
	uint32_t checksum;
};

struct scorefile_t {
	scorefile_header_t header;
	scorefile_board_t boards[RANK_COUNT][PLAYCHAR_COUNT];
};
#pragma option -a2

typedef char scorefile_entry_size_check[
	(sizeof(scorefile_entry_t) == 20) ? 1 : -1
];
typedef char scorefile_board_size_check[
	(sizeof(scorefile_board_t) == 216) ? 1 : -1
];
typedef char scorefile_header_size_check[
	(sizeof(scorefile_header_t) == 36) ? 1 : -1
];
typedef char scorefile_size_check[
	(sizeof(scorefile_t) == 7812) ? 1 : -1
];

scorefile_stats_t scorefile_view_stats;
uint8_t scorefile_view_page;
static uint8_t scorefile_view_valid[T3_SCOREFILE_PLACES];
uint8_t scorefile_view_total[T3_SCOREFILE_PLACES];
static scorefile_t far *scorefile;
static scorefile_t far *scorefile_scratch;
static bool scorefile_recreated;
static bool scorefile_legacy_recognized;
static bool scorefile_new_recognized;
static bool scorefile_read_only;

static uint32_t score_checksum_far(const uint8_t far *data, unsigned size)
{
	uint16_t a = 0xA5A5;
	uint16_t b = 0x5A5A;

	while(size--) {
		a += *data++;
		b += a;
	}
	return (static_cast<uint32_t>(b) << 16) | a;
}

static bool scorefile_rename(
	const char far *old_fn, const char far *new_fn
)
{
	asm {
		push	ds
		push	es
		push	di
		lds 	dx, old_fn
		les 	di, new_fn
		mov 	ax, 5600h
		int 	21h
		pop 	di
		pop 	es
		pop 	ds
	}
	return !FLAGS_CARRY;
}

static uint32_t score_add_saturating(uint32_t a, uint32_t b)
{
	if((0xFFFFFFFFUL - a) < b) {
		return 0xFFFFFFFFUL;
	}
	return (a + b);
}

static int scorefile_entry_compare(
	const scorefile_entry_t far *a, const scorefile_entry_t far *b
)
{
	for(int digit = (T3_SCOREFILE_SCORE_DIGITS - 1); digit >= 0; digit--) {
		if(a->score[digit] > b->score[digit]) {
			return 1;
		}
		if(a->score[digit] < b->score[digit]) {
			return -1;
		}
	}
	return 0;
}

static void scorefile_far_copy(
	uint8_t far *dst, const uint8_t far *src, unsigned size
)
{
	while(size--) {
		*dst++ = *src++;
	}
}

static void scorefile_far_zero(uint8_t far *dst, unsigned size)
{
	while(size--) {
		*dst++ = 0;
	}
}

static void scorefile_board_checksum_set(scorefile_board_t far *board)
{
	board->checksum = score_checksum_far(
		reinterpret_cast<const uint8_t far *>(board),
		offsetof(scorefile_board_t, checksum)
	);
}

static bool scorefile_board_valid(const scorefile_board_t far *board)
{
	const scorefile_entry_t far *previous = 0;
	bool saw_empty = false;

	if(board->checksum != score_checksum_far(
		reinterpret_cast<const uint8_t far *>(board),
		offsetof(scorefile_board_t, checksum)
	)) {
		return false;
	}
	for(unsigned place = 0; place < T3_SCOREFILE_PLACES; place++) {
		const scorefile_entry_t far *entry = &board->entries[place];
		if(entry->reserved != 0 || (entry->flags & ~T3_SCOREFILE_ENTRY_VALID)) {
			return false;
		}
		if(!(entry->flags & T3_SCOREFILE_ENTRY_VALID)) {
			saw_empty = true;
			continue;
		}
		if(saw_empty || (entry->continues_used > 9)) {
			return false;
		}
		if(!(
			((entry->stage >= 1) && (entry->stage <= STAGE_COUNT)) ||
			(entry->stage == STAGE_ALL)
		)) {
			return false;
		}
		for(unsigned digit = 0; digit < T3_SCOREFILE_SCORE_DIGITS; digit++) {
			if(entry->score[digit] > 9) {
				return false;
			}
		}
		for(unsigned c = 0; c < T3_SCOREFILE_NAME_LEN; c++) {
			uint8_t value = static_cast<uint8_t>(entry->name[c]);
			if((value < 0x20) || (value > 0x7E)) {
				return false;
			}
		}
		if(previous && (scorefile_entry_compare(entry, previous) > 0)) {
			return false;
		}
		previous = entry;
	}
	return true;
}

static bool scorefile_header_valid(const scorefile_t far *data)
{
	const scorefile_header_t far *header = &data->header;
	return (
		(header->magic[0] == 'T') && (header->magic[1] == '3') &&
		(header->magic[2] == 'N') && (header->magic[3] == 'E') &&
		(header->magic[4] == 'M') && (header->magic[5] == '0') &&
		(header->magic[6] == '0') && (header->magic[7] == '1') &&
		(header->version == T3_SCOREFILE_VERSION) &&
		(header->header_size == sizeof(scorefile_header_t)) &&
		(header->file_size == sizeof(scorefile_t)) &&
		(header->entry_size == sizeof(scorefile_entry_t)) &&
		(header->board_size == sizeof(scorefile_board_t)) &&
		(header->rank_count == RANK_COUNT) &&
		(header->playchar_count == PLAYCHAR_COUNT) &&
		(header->place_count == T3_SCOREFILE_PLACES) &&
		!(header->flags & ~T3_SCOREFILE_FLAG_EXTRA_UNLOCKED) &&
		(header->checksum == score_checksum_far(
			reinterpret_cast<const uint8_t far *>(header),
			offsetof(scorefile_header_t, checksum)
		))
	);
}

static void scorefile_header_checksum_set(void)
{
	scorefile->header.checksum = score_checksum_far(
		reinterpret_cast<const uint8_t far *>(&scorefile->header),
		offsetof(scorefile_header_t, checksum)
	);
}

static void scorefile_entry_empty(scorefile_entry_t far *entry)
{
	for(unsigned c = 0; c < T3_SCOREFILE_NAME_LEN; c++) {
		entry->name[c] = ' ';
	}
	for(unsigned digit = 0; digit < T3_SCOREFILE_SCORE_DIGITS; digit++) {
		entry->score[digit] = 0;
	}
	entry->stage = T3_SCOREFILE_STAGE_EMPTY;
	entry->continues_used = 0;
	entry->flags = 0;
	entry->reserved = 0;
}

static void scorefile_board_empty(scorefile_board_t far *board)
{
	for(unsigned place = 0; place < T3_SCOREFILE_PLACES; place++) {
		scorefile_entry_empty(&board->entries[place]);
	}
	board->stats.play_frames = 0;
	board->stats.one_ccs = 0;
	board->stats.continues_used = 0;
	scorefile_board_checksum_set(board);
}

static void scorefile_empty(void)
{
	scorefile_far_zero(
		reinterpret_cast<uint8_t far *>(scorefile), sizeof(scorefile_t)
	);
	scorefile->header.magic[0] = 'T'; scorefile->header.magic[1] = '3';
	scorefile->header.magic[2] = 'N'; scorefile->header.magic[3] = 'E';
	scorefile->header.magic[4] = 'M'; scorefile->header.magic[5] = '0';
	scorefile->header.magic[6] = '0'; scorefile->header.magic[7] = '1';
	scorefile->header.version = T3_SCOREFILE_VERSION;
	scorefile->header.header_size = sizeof(scorefile_header_t);
	scorefile->header.file_size = sizeof(scorefile_t);
	scorefile->header.entry_size = sizeof(scorefile_entry_t);
	scorefile->header.board_size = sizeof(scorefile_board_t);
	scorefile->header.rank_count = RANK_COUNT;
	scorefile->header.playchar_count = PLAYCHAR_COUNT;
	scorefile->header.place_count = T3_SCOREFILE_PLACES;
	for(unsigned rank = 0; rank < RANK_COUNT; rank++) {
		for(unsigned playchar = 0; playchar < PLAYCHAR_COUNT; playchar++) {
			scorefile_board_empty(&scorefile->boards[rank][playchar]);
		}
	}
	scorefile_header_checksum_set();
}

static bool scorefile_read(
	const char far *fn, scorefile_t far *data
)
{
	bool ok;
	uint8_t trailing;
	if(!file_ropen(fn)) {
		return false;
	}
	ok = (
		(file_read(data, sizeof(scorefile_t)) == sizeof(scorefile_t)) &&
		(file_read(&trailing, sizeof(trailing)) == 0)
	);
	file_close();
	return ok;
}

static void scorefile_checksums_set(void)
{
	for(unsigned rank = 0; rank < RANK_COUNT; rank++) {
		for(unsigned playchar = 0; playchar < PLAYCHAR_COUNT; playchar++) {
			scorefile_board_checksum_set(
				&scorefile->boards[rank][playchar]
			);
		}
	}
	scorefile_header_checksum_set();
}

static bool scorefile_temp_write_and_validate(void)
{
	scorefile_delete(T3_SCOREFILE_TEMP_FN);
	if(!file_create(T3_SCOREFILE_TEMP_FN)) {
		return false;
	}
	if(!file_write(scorefile, sizeof(scorefile_t))) {
		file_close();
		scorefile_delete(T3_SCOREFILE_TEMP_FN);
		return false;
	}
	file_close();
	if(!scorefile_read(T3_SCOREFILE_TEMP_FN, scorefile_scratch) ||
		!scorefile_header_valid(scorefile_scratch)) {
		scorefile_delete(T3_SCOREFILE_TEMP_FN);
		return false;
	}
	for(unsigned rank = 0; rank < RANK_COUNT; rank++) {
		for(unsigned playchar = 0; playchar < PLAYCHAR_COUNT; playchar++) {
			if(!scorefile_board_valid(
				&scorefile_scratch->boards[rank][playchar]
			)) {
				scorefile_delete(T3_SCOREFILE_TEMP_FN);
				return false;
			}
		}
	}
	return true;
}

static bool scorefile_save_atomic(void)
{
	if(scorefile_read_only) {
		return false;
	}
	bool had_old = scorefile_exists(T3_SCOREFILE_FN);
	uint32_t generation = scorefile->header.generation;

	if(generation != 0xFFFFFFFFUL) {
		scorefile->header.generation++;
	}
	scorefile_checksums_set();
	if(!scorefile_temp_write_and_validate()) {
		scorefile->header.generation = generation;
		scorefile_checksums_set();
		return false;
	}
	scorefile_delete(T3_SCOREFILE_OLD_FN);
	if(had_old && !scorefile_rename(
		T3_SCOREFILE_FN, T3_SCOREFILE_OLD_FN
	)) {
		scorefile_delete(T3_SCOREFILE_TEMP_FN);
		scorefile->header.generation = generation;
		scorefile_checksums_set();
		return false;
	}
	if(!scorefile_rename(T3_SCOREFILE_TEMP_FN, T3_SCOREFILE_FN)) {
		if(had_old) {
			scorefile_rename(T3_SCOREFILE_OLD_FN, T3_SCOREFILE_FN);
		}
		scorefile_delete(T3_SCOREFILE_TEMP_FN);
		scorefile->header.generation = generation;
		scorefile_checksums_set();
		return false;
	}
	scorefile_delete(T3_SCOREFILE_OLD_FN);
	(void)scorefile_commit_process();
	return true;
}

static bool scorefile_alloc(void)
{
	if(scorefile) {
		return true;
	}
	scorefile = reinterpret_cast<scorefile_t far *>(
		hmem_allocbyte(sizeof(scorefile_t))
	);
	scorefile_scratch = reinterpret_cast<scorefile_t far *>(
		hmem_allocbyte(sizeof(scorefile_t))
	);
	if(!scorefile || !scorefile_scratch) {
		if(scorefile) {
			hmem_free(reinterpret_cast<void __seg *>(scorefile));
		}
		if(scorefile_scratch) {
			hmem_free(reinterpret_cast<void __seg *>(scorefile_scratch));
		}
		scorefile = 0;
		scorefile_scratch = 0;
		return false;
	}
	return true;
}

static bool scorefile_candidate_consider(
	const char far *fn, uint8_t source, bool __ss& found,
	uint32_t __ss& generation, uint8_t __ss& best_source
)
{
	if(!scorefile_read(fn, scorefile_scratch) ||
		!scorefile_header_valid(scorefile_scratch)) {
		return false;
	}
	if(!found || (scorefile_scratch->header.generation > generation)) {
		scorefile_far_copy(
			reinterpret_cast<uint8_t far *>(scorefile),
			reinterpret_cast<const uint8_t far *>(scorefile_scratch),
			sizeof(scorefile_t)
		);
		generation = scorefile->header.generation;
		best_source = source;
		found = true;
	}
	return true;
}

static bool scorefile_candidate_promote(uint8_t source)
{
	bool had_canonical;

	if(source == 0) {
		return true;
	}
	if(source == 1) {
		had_canonical = scorefile_exists(T3_SCOREFILE_FN);
		scorefile_delete(T3_SCOREFILE_OLD_FN);
		if(had_canonical && !scorefile_rename(
			T3_SCOREFILE_FN, T3_SCOREFILE_OLD_FN
		)) {
			return false;
		}
		if(!scorefile_rename(T3_SCOREFILE_TEMP_FN, T3_SCOREFILE_FN)) {
			if(had_canonical) {
				scorefile_rename(T3_SCOREFILE_OLD_FN, T3_SCOREFILE_FN);
			}
			return false;
		}
		scorefile_delete(T3_SCOREFILE_OLD_FN);
	} else {
		had_canonical = scorefile_exists(T3_SCOREFILE_FN);
		scorefile_delete(T3_SCOREFILE_TEMP_FN);
		if(had_canonical && !scorefile_rename(
			T3_SCOREFILE_FN, T3_SCOREFILE_TEMP_FN
		)) {
			return false;
		}
		if(!scorefile_rename(T3_SCOREFILE_OLD_FN, T3_SCOREFILE_FN)) {
			if(had_canonical) {
				scorefile_rename(T3_SCOREFILE_TEMP_FN, T3_SCOREFILE_FN);
			}
			return false;
		}
		scorefile_delete(T3_SCOREFILE_TEMP_FN);
	}
	(void)scorefile_commit_process();
	return true;
}

static bool scorefile_new_load(void)
{
	bool found = false;
	bool repaired = false;
	uint32_t generation = 0;
	uint8_t source = 0;

	scorefile_new_recognized = false;
	scorefile_candidate_consider(
		T3_SCOREFILE_FN, 0, found, generation, source
	);
	scorefile_candidate_consider(
		T3_SCOREFILE_TEMP_FN, 1, found, generation, source
	);
	scorefile_candidate_consider(
		T3_SCOREFILE_OLD_FN, 2, found, generation, source
	);
	if(!found) {
		return false;
	}
	scorefile_new_recognized = true;
	for(unsigned rank = 0; rank < RANK_COUNT; rank++) {
		for(unsigned playchar = 0; playchar < PLAYCHAR_COUNT; playchar++) {
			if(!scorefile_board_valid(&scorefile->boards[rank][playchar])) {
				scorefile_board_empty(&scorefile->boards[rank][playchar]);
				repaired = true;
			}
		}
	}
	if(source != 0) {
		if(!scorefile_candidate_promote(source)) {
			return false;
		}
	}
	if(repaired) {
		if(!scorefile_save_atomic()) {
			return false;
		}
	} else if(source == 0) {
		scorefile_delete(T3_SCOREFILE_TEMP_FN);
		scorefile_delete(T3_SCOREFILE_OLD_FN);
	}
	return true;
}

static char legacy_regi_ascii(uint8_t regi)
{
	if(regi <= REGI_M) {
		return static_cast<char>('A' + (regi - REGI_A));
	}
	if((regi >= REGI_N) && (regi <= REGI_Z)) {
		return static_cast<char>('N' + (regi - REGI_N));
	}
	if((regi >= REGI_0) && (regi <= REGI_9)) {
		return static_cast<char>('0' + (regi - REGI_0));
	}
	switch(regi) {
	case REGI_PERIOD:      return '.';
	case REGI_COMMA:       return ',';
	case REGI_EXCLAMATION: return '!';
	case REGI_QUESTION:    return '?';
	case REGI_SP:          return ' ';
	default:               return ' ';
	}
}

static int scorefile_ascii_regi(char c)
{
	if((c >= 'a') && (c <= 'z')) {
		c = static_cast<char>('A' + (c - 'a'));
	}
	if((c >= 'A') && (c <= 'M')) {
		return (REGI_A + (c - 'A'));
	}
	if((c >= 'N') && (c <= 'Z')) {
		return (REGI_N + (c - 'N'));
	}
	if((c >= '0') && (c <= '9')) {
		return (REGI_0 + (c - '0'));
	}
	switch(c) {
	case '.': return REGI_PERIOD;
	case ',': return REGI_COMMA;
	case '!': return REGI_EXCLAMATION;
	case '?': return REGI_QUESTION;
	default:  return REGI_SP;
	}
}

static void scorefile_legacy_decode(scoredat_section_t __ss& legacy)
{
	uint8_t __ss *p = reinterpret_cast<uint8_t __ss *>(&legacy);
	uint8_t tmp;
	unsigned i = 0;

	while(i < (offsetof(scoredat_section_t, score.key1) - 1)) {
		tmp = p[1];
		_AL = legacy.score.key2;
		asm { ror tmp, 3; }
		tmp ^= _AL;
		p[0] = (legacy.score.key1 + tmp + p[0]);
		i++;
		p++;
	}
	p[0] = (legacy.score.key1 + legacy.score.key2 + p[0]);
}

static bool scorefile_legacy_valid(const scoredat_section_t __ss& legacy)
{
	uint16_t sum = 0;
	const uint8_t __ss *p = reinterpret_cast<const uint8_t __ss *>(
		&legacy.score
	);
	for(unsigned i = 0; i < sizeof(legacy.score); i++) {
		sum += *p++;
	}
	return (sum == legacy.sum);
}

static bool scorefile_legacy_read_and_convert(void)
{
	scoredat_section_t legacy;
	uint8_t trailing;
	bool unlocked = false;

	scorefile_legacy_recognized = false;
	if(!file_ropen(T3_SCOREFILE_FN)) {
		return false;
	}
	scorefile_empty();
	for(unsigned rank = 0; rank < RANK_COUNT; rank++) {
		if(file_read(&legacy, sizeof(legacy)) != sizeof(legacy)) {
			file_close();
			return false;
		}
		scorefile_legacy_decode(legacy);
		if(!scorefile_legacy_valid(legacy)) {
			file_close();
			return false;
		}
		if(legacy.score.cleared == SCOREDAT_CLEARED) {
			unlocked = true;
		}
		for(unsigned place = 0; place < SCOREDAT_PLACES; place++) {
			uint8_t optional = legacy.score.playchar[place].v;
			if((optional == 0) || (optional > PLAYCHAR_COUNT)) {
				continue;
			}
			scorefile_entry_t far *entry = &scorefile->boards[
				rank
			][optional - 1].entries[0];
			unsigned dst = 0;
			while((dst < T3_SCOREFILE_PLACES) &&
				(entry[dst].flags & T3_SCOREFILE_ENTRY_VALID)) {
				dst++;
			}
			if(dst >= T3_SCOREFILE_PLACES) {
				continue;
			}
			for(unsigned c = 0; c < T3_SCOREFILE_NAME_LEN; c++) {
				entry[dst].name[c] = legacy_regi_ascii(
					legacy.score.name[place][
						(T3_SCOREFILE_NAME_LEN - 1) - c
					]
				);
			}
			bool digits_valid = true;
			for(unsigned digit = 0;
				digit < T3_SCOREFILE_SCORE_DIGITS; digit++) {
				uint8_t regi = legacy.score.score[place][digit + 1];
				if((regi < REGI_0) || (regi > REGI_9)) {
					digits_valid = false;
					break;
				}
				entry[dst].score[digit] = (regi - REGI_0);
			}
			if(!digits_valid) {
				scorefile_entry_empty(&entry[dst]);
				continue;
			}
			if(legacy.score.stage[place] == REGI_ALL) {
				entry[dst].stage = STAGE_ALL;
			} else if(
				(legacy.score.stage[place] >= REGI_1) &&
				(legacy.score.stage[place] <= REGI_9)
			) {
				entry[dst].stage = (
					legacy.score.stage[place] - REGI_0
				);
			} else {
				scorefile_entry_empty(&entry[dst]);
				continue;
			}
			entry[dst].continues_used = 0;
			if((legacy.score.score[place][0] >= REGI_0) &&
				(legacy.score.score[place][0] <= REGI_9)) {
				entry[dst].continues_used = (
					legacy.score.score[place][0] - REGI_0
				);
			}
			entry[dst].flags = T3_SCOREFILE_ENTRY_VALID;
			entry[dst].reserved = 0;
		}
	}
	if(file_read(&trailing, sizeof(trailing)) != 0) {
		file_close();
		return false;
	}
	file_close();
	scorefile_legacy_recognized = true;
	if(unlocked) {
		scorefile->header.flags |= T3_SCOREFILE_FLAG_EXTRA_UNLOCKED;
	}
	scorefile_checksums_set();
	return true;
}

static bool scorefile_legacy_migrate(void)
{
	const char far *backup_fn = T3_SCOREFILE_BACKUP_FN;

	if(!scorefile_legacy_read_and_convert()) {
		return false;
	}
	if(scorefile_exists(backup_fn)) {
		backup_fn = T3_SCOREFILE_BACKUP_1_FN;
		if(scorefile_exists(backup_fn)) {
			return false;
		}
	}
	if(!scorefile_rename(T3_SCOREFILE_FN, backup_fn)) {
		return false;
	}
	if(!scorefile_save_atomic()) {
		scorefile_rename(backup_fn, T3_SCOREFILE_FN);
		return false;
	}
	return true;
}

static bool scoretime_read_newest(scoretime_record_t __ss& newest)
{
	scoretime_record_t records[2];
	uint8_t trailing;
	bool valid[2];

	if(!file_ropen(T3_SCORETIME_FN)) {
		return false;
	}
	if((file_read(records, sizeof(records)) != sizeof(records)) ||
		(file_read(&trailing, sizeof(trailing)) != 0)) {
		file_close();
		return false;
	}
	file_close();
	valid[0] = scoretime_record_valid(records[0]);
	valid[1] = scoretime_record_valid(records[1]);
	if(!valid[0] && !valid[1]) {
		return false;
	}
	if(!valid[1] || (valid[0] && (
		(records[0].run_id > records[1].run_id) ||
		((records[0].run_id == records[1].run_id) &&
		 (records[0].frames >= records[1].frames))
	))) {
		newest = records[0];
	} else {
		newest = records[1];
	}
	return true;
}

static void scorefile_journal_recover(void)
{
	scoretime_record_t newest;

	if(!scoretime_read_newest(newest)) {
		if(scorefile_exists(T3_SCORETIME_FN)) {
			scorefile_delete(T3_SCORETIME_FN);
		}
		return;
	}
#if (BINARY == 'L')
	if(
		scorestat_active() &&
		(resident->game_mode == GM_STORY) &&
		(scorestat_resident_u32(T3_SCORESTAT_RES_RUN_ID_INDEX) ==
		 newest.run_id)
	) {
		return;
	}
#endif
	if(newest.run_id <= scorefile->header.last_run_id) {
		scorefile_delete(T3_SCORETIME_FN);
		return;
	}
	scorefile_board_t far *board = &scorefile->boards[
		newest.rank
	][newest.playchar];
	board->stats.play_frames = score_add_saturating(
		board->stats.play_frames, newest.frames
	);
	scorefile->header.last_run_id = newest.run_id;
	if(scorefile_save_atomic()) {
		scorefile_delete(T3_SCORETIME_FN);
		if(scorestat_active() &&
			(scorestat_resident_u32(T3_SCORESTAT_RES_RUN_ID_INDEX) ==
			 newest.run_id)) {
			scorestat_clear();
		}
	}
}

static bool scorefile_ensure(void)
{
	static const char far BAD_FN[] = "YUME.BAD";

	if(scorefile) {
		return true;
	}
	if(!scorefile_alloc()) {
		return false;
	}
	scorefile_recreated = false;
	scorefile_read_only = false;
	if(!scorefile_new_load()) {
		if(scorefile_new_recognized) {
			scorefile_read_only = true;
			scorefile_journal_recover();
			return true;
		}
		if(scorefile_legacy_migrate()) {
			// Converted in memory and on disk.
		} else {
			// A valid legacy file stays untouched if its backup or replacement
			// cannot be created. Keep the converted in-memory view read-only
			// for this process and never reclassify it as corrupt.
			if(scorefile_legacy_recognized) {
				scorefile_read_only = true;
				return true;
			}
			if(scorefile_exists(T3_SCOREFILE_FN)) {
				if(scorefile_exists(BAD_FN) || !scorefile_rename(
					T3_SCOREFILE_FN, BAD_FN
				)) {
					scorefile_empty();
					scorefile_read_only = true;
					scorefile_recreated = true;
					return true;
				}
			}
			scorefile_empty();
			if(!scorefile_save_atomic()) {
				scorefile_read_only = true;
			}
			scorefile_recreated = true;
		}
	}
	scorefile_journal_recover();
	return true;
}

static void scorefile_hi_clear(void)
{
#if (BINARY == 'L')
	unsigned place;
	int placeholder = REGI_9;

	for(place = 0; place < T3_SCOREFILE_PLACES; place++) {
		for(unsigned c = 0; c < T3_SCOREFILE_NAME_LEN; c++) {
			hi.score.name[place][c] = REGI_PERIOD;
		}
		for(unsigned digit = 0; digit < (SCORE_DIGITS + 2); digit++) {
			hi.score.score[place][digit] = REGI_0;
		}
		hi.score.playchar[place].set_none();
		hi.score.stage[place] = REGI_1;
		scorefile_view_valid[place] = false;
		scorefile_view_total[place] = false;
	}
	hi.score.score[0][4] = REGI_1;
	for(place = 1; place < T3_SCOREFILE_PLACES; place++) {
		hi.score.score[place][3] = static_cast<regi_patnum_t>(placeholder);
		placeholder--;
	}
#else
	for(unsigned place = 0; place < T3_SCOREFILE_PLACES; place++) {
		for(unsigned c = 0; c < T3_SCOREFILE_NAME_LEN; c++) {
			hi.score.name[place][c] = REGI_SP;
		}
		for(unsigned digit = 0; digit < (SCORE_DIGITS + 2); digit++) {
			hi.score.score[place][digit] = REGI_0;
		}
		hi.score.playchar[place].set_none();
		hi.score.stage[place] = REGI_1;
		scorefile_view_valid[place] = false;
		scorefile_view_total[place] = false;
	}
#endif
	hi.score.cleared = (
		(scorefile->header.flags & T3_SCOREFILE_FLAG_EXTRA_UNLOCKED) ?
		SCOREDAT_CLEARED : SCOREDAT_NOT_CLEARED
	);
}

static void scorefile_entry_to_hi(
	unsigned place, const scorefile_entry_t far *entry, uint8_t playchar
)
{
	for(unsigned c = 0; c < T3_SCOREFILE_NAME_LEN; c++) {
		hi.score.name[place][(T3_SCOREFILE_NAME_LEN - 1) - c] =
			static_cast<regi_patnum_t>(scorefile_ascii_regi(entry->name[c]));
	}
	for(unsigned digit = 0; digit < T3_SCOREFILE_SCORE_DIGITS; digit++) {
		hi.score.score[place][digit + 1] = REGI_DIGIT(entry->score[digit]);
	}
	hi.score.score[place][0] = REGI_DIGIT(entry->continues_used);
	hi.score.playchar[place].v = (playchar + 1);
	hi.score.stage[place] = (
		(entry->stage == STAGE_ALL) ? REGI_ALL : REGI_DIGIT(entry->stage)
	);
	scorefile_view_valid[place] = true;
}

#if (BINARY == 'L')

static int scorefile_entry_compare_hi(
	const scorefile_entry_t far *entry, unsigned place
)
{
	for(int digit = (T3_SCOREFILE_SCORE_DIGITS - 1); digit >= 0; digit--) {
		int existing = (hi.score.score[place][digit + 1] - REGI_0);
		if(entry->score[digit] > existing) {
			return 1;
		}
		if(entry->score[digit] < existing) {
			return -1;
		}
	}
	return 0;
}

static void scorefile_hi_row_copy(unsigned dst, unsigned src)
{
	for(unsigned c = 0; c < T3_SCOREFILE_NAME_LEN; c++) {
		hi.score.name[dst][c] = hi.score.name[src][c];
	}
	for(unsigned digit = 0; digit < (SCORE_DIGITS + 2); digit++) {
		hi.score.score[dst][digit] = hi.score.score[src][digit];
	}
	hi.score.playchar[dst] = hi.score.playchar[src];
	hi.score.stage[dst] = hi.score.stage[src];
	scorefile_view_valid[dst] = scorefile_view_valid[src];
	scorefile_view_total[dst] = false;
}

static void scorefile_entry_insert_hi(
	const scorefile_entry_t far *entry, uint8_t playchar, unsigned place_limit
)
{
	unsigned place;
	int shift;

	for(place = 0; place < place_limit; place++) {
		if(scorefile_entry_compare_hi(entry, place) > 0) {
			break;
		}
	}
	if(place >= place_limit) {
		return;
	}
	for(shift = (place_limit - 2); shift >= static_cast<int>(place); shift--) {
		scorefile_hi_row_copy((shift + 1), shift);
	}
	scorefile_entry_to_hi(place, entry, playchar);
}

#endif

static void scorefile_stats_zero(void)
{
	scorefile_view_stats.play_frames = 0;
	scorefile_view_stats.one_ccs = 0;
	scorefile_view_stats.continues_used = 0;
}

static void scorefile_stats_add(const scorefile_stats_t far *stats)
{
	scorefile_view_stats.play_frames = score_add_saturating(
		scorefile_view_stats.play_frames, stats->play_frames
	);
	scorefile_view_stats.one_ccs = score_add_saturating(
		scorefile_view_stats.one_ccs, stats->one_ccs
	);
	scorefile_view_stats.continues_used = score_add_saturating(
		scorefile_view_stats.continues_used, stats->continues_used
	);
}

static void scorefile_total_to_hi(
	unsigned place, const uint8_t __ss *digits
)
{
	static const char far TOTAL[] = "TOTAL   ";
	for(unsigned c = 0; c < T3_SCOREFILE_NAME_LEN; c++) {
		hi.score.name[place][(T3_SCOREFILE_NAME_LEN - 1) - c] =
			static_cast<regi_patnum_t>(scorefile_ascii_regi(TOTAL[c]));
	}
	for(unsigned digit = 0; digit < 10; digit++) {
		hi.score.score[place][digit] = REGI_DIGIT(digits[digit]);
	}
	scorefile_view_valid[place] = true;
	scorefile_view_total[place] = true;
}

bool16 far scorefile_view_load(rank_t rank, uint8_t page)
{
	uint8_t visible_count;

	if(!scorefile_ensure() || (rank >= RANK_COUNT)) {
		return false;
	}
	visible_count = (
		(scorefile->header.flags & T3_SCOREFILE_FLAG_EXTRA_UNLOCKED) ?
		PLAYCHAR_COUNT : PLAYCHAR_COUNT_LOCKED
	);
	if((page != T3_SCOREFILE_VIEW_ALL) && (page >= visible_count)) {
		return false;
	}
	scorefile_hi_clear();
	scorefile_stats_zero();
	scorefile_view_page = page;
	if(page != T3_SCOREFILE_VIEW_ALL) {
		scorefile_board_t far *board = &scorefile->boards[rank][page];
		for(unsigned place = 0; place < T3_SCOREFILE_PLACES; place++) {
			if(board->entries[place].flags & T3_SCOREFILE_ENTRY_VALID) {
#if (BINARY == 'L')
				scorefile_entry_insert_hi(
					&board->entries[place], page, T3_SCOREFILE_PLACES
				);
#else
				scorefile_entry_to_hi(place, &board->entries[place], page);
#endif
			}
		}
		scorefile_view_stats = board->stats;
		return true;
	}

	struct unified_t {
		uint8_t playchar;
		const scorefile_entry_t far *entry;
	};
	unified_t best[PLAYCHAR_COUNT];
	unsigned best_count = 0;
	uint8_t total_digits[10];
	for(unsigned digit = 0; digit < sizeof(total_digits); digit++) {
		total_digits[digit] = 0;
	}
	for(unsigned playchar = 0; playchar < visible_count; playchar++) {
		scorefile_board_t far *board = &scorefile->boards[rank][playchar];
		scorefile_stats_add(&board->stats);
		if(board->entries[0].flags & T3_SCOREFILE_ENTRY_VALID) {
			best[best_count].playchar = playchar;
			best[best_count].entry = &board->entries[0];
			best_count++;
			uint8_t carry = 0;
#if (BINARY == 'L')
			for(unsigned digit = 1; digit < 10; digit++) {
				uint8_t add = (
					(digit <= T3_SCOREFILE_SCORE_DIGITS) ?
					board->entries[0].score[digit - 1] : 0
				);
				uint8_t sum = (total_digits[digit] + add + carry);
				total_digits[digit] = (sum % 10);
				carry = (sum / 10);
			}
			uint8_t continues = (
				total_digits[0] + board->entries[0].continues_used
			);
			total_digits[0] = ((continues > 9) ? 9 : continues);
#else
			for(unsigned digit = 0; digit < 10; digit++) {
				uint8_t add = (
					(digit < T3_SCOREFILE_SCORE_DIGITS) ?
					board->entries[0].score[digit] : 0
				);
				uint8_t sum = (total_digits[digit] + add + carry);
				total_digits[digit] = (sum % 10);
				carry = (sum / 10);
			}
#endif
		}
	}
	for(unsigned i = 0; i < best_count; i++) {
		for(unsigned j = (i + 1); j < best_count; j++) {
			int comparison = scorefile_entry_compare(best[j].entry, best[i].entry);
			if((comparison > 0) || ((comparison == 0) &&
				(best[j].playchar < best[i].playchar))) {
				unified_t swap = best[i];
				best[i] = best[j];
				best[j] = swap;
			}
		}
	}
	for(unsigned place = 0; place < best_count; place++) {
#if (BINARY == 'L')
		scorefile_entry_insert_hi(
			best[place].entry, best[place].playchar,
			(T3_SCOREFILE_PLACES - 1)
		);
#else
		scorefile_entry_to_hi(
			place, best[place].entry, best[place].playchar
		);
#endif
	}
	scorefile_total_to_hi((T3_SCOREFILE_PLACES - 1), total_digits);
	return true;
}

bool16 far scorefile_row_valid(uint8_t place)
{
	return (
		(place < T3_SCOREFILE_PLACES) && scorefile_view_valid[place]
	);
}

bool16 far scorefile_row_total(uint8_t place)
{
	return (
		(place < T3_SCOREFILE_PLACES) && scorefile_view_total[place]
	);
}

void far scorefile_row_insert(uint8_t place)
{
	if(place >= T3_SCOREFILE_PLACES) {
		return;
	}
	for(int shift = (T3_SCOREFILE_PLACES - 2); shift >= place; shift--) {
		scorefile_view_valid[shift + 1] = scorefile_view_valid[shift];
		scorefile_view_total[shift + 1] = false;
	}
	scorefile_view_valid[place] = true;
	scorefile_view_total[place] = false;
}

bool16 far scorefile_compat_load(rank_t rank)
{
	uint8_t page = T3_SCOREFILE_VIEW_ALL;
	if(resident->story_stage != STAGE_NONE) {
		page = resident->playchar_paletted[0].char_id();
	}
	if(!scorefile_view_load(rank, page)) {
		return true;
	}
	return scorefile_recreated;
}

static void scorefile_hi_to_board(
	scorefile_board_t far *board, uint8_t playchar
)
{
#if (BINARY == 'L')
	unsigned dst = 0;
	for(unsigned place = 0; place < T3_SCOREFILE_PLACES; place++) {
		if(!scorefile_view_valid[place]) {
			continue;
		}
		scorefile_entry_t far *entry = &board->entries[dst++];
#else
	for(unsigned place = 0; place < T3_SCOREFILE_PLACES; place++) {
		scorefile_entry_t far *entry = &board->entries[place];
		if(!scorefile_view_valid[place]) {
			scorefile_entry_empty(entry);
			continue;
		}
#endif
		for(unsigned c = 0; c < T3_SCOREFILE_NAME_LEN; c++) {
			entry->name[c] = legacy_regi_ascii(
				hi.score.name[place][(T3_SCOREFILE_NAME_LEN - 1) - c]
			);
		}
		for(unsigned digit = 0; digit < T3_SCOREFILE_SCORE_DIGITS; digit++) {
			entry->score[digit] = (
				hi.score.score[place][digit + 1] - REGI_0
			);
		}
		entry->stage = (
			(hi.score.stage[place] == REGI_ALL) ?
			STAGE_ALL : (hi.score.stage[place] - REGI_0)
		);
		entry->continues_used = (
			hi.score.score[place][0] - REGI_0
		);
		entry->flags = T3_SCOREFILE_ENTRY_VALID;
		entry->reserved = 0;
	}
#if (BINARY == 'L')
	while(dst < T3_SCOREFILE_PLACES) {
		scorefile_entry_empty(&board->entries[dst++]);
	}
#endif
	(void)playchar;
}

void far scorefile_compat_save(rank_t rank)
{
	uint8_t playchar;
	scorefile_board_t far *board;
	uint32_t run_id;
	uint32_t frames;
	uint32_t last_run_id_before;
	scorefile_stats_t stats_before;
	uint8_t flags_before;
	bool one_cc;
	bool scorestat_eligible;

	if(
		(resident->story_stage == STAGE_NONE) ||
		!scorefile_ensure() || (rank >= RANK_COUNT)
	) {
		return;
	}
	playchar = resident->playchar_paletted[0].char_id();
	if(playchar >= PLAYCHAR_COUNT) {
		return;
	}
	board = &scorefile->boards[rank][playchar];
	stats_before = board->stats;
	flags_before = scorefile->header.flags;
	last_run_id_before = scorefile->header.last_run_id;
	scorefile_hi_to_board(board, playchar);
	scorestat_eligible = (
		(resident->game_mode == GM_STORY) &&
		(resident->demo_num == 0) && !practice_game_active()
	);
	one_cc = (scorestat_eligible &&
		(resident->story_stage == STAGE_ALL) &&
		(resident->rem_credits == 3)
	);
	if(scorestat_eligible && scorestat_active() &&
		(scorestat_resident_u8(T3_SCORESTAT_RES_RANK_INDEX) == rank) &&
		(scorestat_resident_u8(T3_SCORESTAT_RES_PLAYCHAR_INDEX) == playchar)) {
		scorestat_process_sync();
		frames = scorestat_resident_u32(T3_SCORESTAT_RES_FRAMES_INDEX);
		run_id = scorestat_resident_u32(T3_SCORESTAT_RES_RUN_ID_INDEX);
		scoretime_write();
		board->stats.play_frames = score_add_saturating(
			board->stats.play_frames, frames
		);
		if(one_cc) {
			board->stats.one_ccs = score_add_saturating(
				board->stats.one_ccs, 1
			);
			scorefile->header.flags |= T3_SCOREFILE_FLAG_EXTRA_UNLOCKED;
		}
		if(run_id > scorefile->header.last_run_id) {
			scorefile->header.last_run_id = run_id;
		}
	}
	if(scorefile_save_atomic()) {
		if(scorestat_eligible && scorestat_active()) {
			scorestat_run_begin();
		} else {
			scorestat_clear();
			scorefile_delete(T3_SCORETIME_FN);
		}
	} else {
		board->stats = stats_before;
		scorefile->header.flags = flags_before;
		scorefile->header.last_run_id = last_run_id_before;
		scorefile_checksums_set();
	}
}

bool16 far scorefile_unlocked(void)
{
	if(!scorefile_ensure()) {
		return false;
	}
	return (
		scorefile->header.flags & T3_SCOREFILE_FLAG_EXTRA_UNLOCKED
	);
}

#if (BINARY == 'O')
bool16 far scorefile_extra_unlock(void)
{
	uint8_t flags_before;
	bool16 unlocked = false;

	if(!scorefile_ensure() ||
		(scorefile->header.flags & T3_SCOREFILE_FLAG_EXTRA_UNLOCKED)) {
		goto done;
	}
	flags_before = scorefile->header.flags;
	scorefile->header.flags |= T3_SCOREFILE_FLAG_EXTRA_UNLOCKED;
	if(scorefile_save_atomic()) {
		unlocked = true;
		goto done;
	}
	scorefile->header.flags = flags_before;
	scorefile_checksums_set();

done:
	// The title code calls this before loading the final character-select
	// portraits. Retaining both 7,812-byte scorefile buffers here can exhaust
	// the smaller DOS heap left by the resident MMD driver.
	scorefile_close();
	return unlocked;
}
#endif

#if (BINARY == 'L')

static const char far SCORE_PAGE_ALL[] = "ALL CHARACTERS";
static const char far SCORE_PAGE_REIMU[] = "REIMU";
static const char far SCORE_PAGE_MIMA[] = "MIMA";
static const char far SCORE_PAGE_MARISA[] = "MARISA";
static const char far SCORE_PAGE_ELLEN[] = "ELLEN";
static const char far SCORE_PAGE_KOTOHIME[] = "KOTOHIME";
static const char far SCORE_PAGE_KANA[] = "KANA";
static const char far SCORE_PAGE_RIKAKO[] = "RIKAKO";
static const char far SCORE_PAGE_CHIYURI[] = "CHIYURI";
static const char far SCORE_PAGE_YUMEMI[] = "YUMEMI";
static const char far SCORE_PLAY_TIME[] = "PLAY TIME ";
static const char far SCORE_1CC[] = "1CC ";
static const char far SCORE_CONTINUES[] = "   CONTINUES ";

static const char far *scorefile_view_page_name(void)
{
	switch(scorefile_view_page) {
	case PLAYCHAR_REIMU:    return SCORE_PAGE_REIMU;
	case PLAYCHAR_MIMA:     return SCORE_PAGE_MIMA;
	case PLAYCHAR_MARISA:   return SCORE_PAGE_MARISA;
	case PLAYCHAR_ELLEN:    return SCORE_PAGE_ELLEN;
	case PLAYCHAR_KOTOHIME: return SCORE_PAGE_KOTOHIME;
	case PLAYCHAR_KANA:     return SCORE_PAGE_KANA;
	case PLAYCHAR_RIKAKO:   return SCORE_PAGE_RIKAKO;
	case PLAYCHAR_CHIYURI:  return SCORE_PAGE_CHIYURI;
	case PLAYCHAR_YUMEMI:   return SCORE_PAGE_YUMEMI;
	default:                 return SCORE_PAGE_ALL;
	}
}

static int scorefile_text_append(char __ss *line, int at, const char far *str)
{
	while(*str) {
		line[at++] = *str++;
	}
	return at;
}

static int scorefile_text_u32(char __ss *line, int at, uint32_t value)
{
	char reversed[10];
	int count = 0;
	do {
		reversed[count++] = static_cast<char>('0' + (value % 10));
		value /= 10;
	} while(value && (count < sizeof(reversed)));
	while(count) {
		line[at++] = reversed[--count];
	}
	return at;
}

static int scorefile_text_fixed(
	char __ss *line, int at, uint32_t value, int digits
)
{
	int end = (at + digits);
	while(digits--) {
		line[at + digits] = static_cast<char>('0' + (value % 10));
		value /= 10;
	}
	return end;
}

#pragma codeseg SCOREFONT_TEXT

static void far scorefile_time_put(
	screen_x_t left, vram_y_t top, const char far *str, int color
)
{
	int glyph_left;

	while(*str) {
		glyph_left = left;
		if((*str >= '0') && (*str <= '9')) {
			if(*str == '1') {
				glyph_left += MENU_FONT_ONE_INSET;
			}
			left += MENU_FONT_NUMERIC_CELL_W;
		} else {
			left += menu_font_width_n(str, 1);
		}
		menu_font_put_n(glyph_left, top, str, 1, color);
		str++;
	}
}

#pragma codeseg

void far scorefile_view_overlay_put(void)
{
	char line[48];
	uint32_t frames = scorefile_view_stats.play_frames;
	uint32_t seconds = ((frames / 282UL) * 5UL) +
		(((frames % 282UL) * 5UL) / 282UL);
	uint32_t hours = (seconds / 3600UL);
	int at;

	if(hours > 9999UL) {
		hours = 9999UL;
	}
	menu_font_put(24, 320, scorefile_view_page_name(), V_WHITE);
	at = scorefile_text_append(line, 0, SCORE_PLAY_TIME);
	at = scorefile_text_fixed(line, at, hours, 4);
	line[at++] = ':';
	at = scorefile_text_fixed(line, at, ((seconds / 60UL) % 60UL), 2);
	line[at++] = ':';
	at = scorefile_text_fixed(line, at, (seconds % 60UL), 2);
	line[at] = '\0';
	scorefile_time_put(24, 340, line, V_WHITE);
	at = scorefile_text_append(line, 0, SCORE_1CC);
	at = scorefile_text_u32(line, at, scorefile_view_stats.one_ccs);
	at = scorefile_text_append(line, at, SCORE_CONTINUES);
	at = scorefile_text_u32(line, at, scorefile_view_stats.continues_used);
	line[at] = '\0';
	menu_font_put(24, 360, line, V_WHITE);
}

void far scorefile_view_assets_load(void)
{
	extern unsigned char near *rank_image_fn;
	extern const char regib_pi[];

	pi_load(0, regib_pi);
	cdg_load_single(0, rank_image_fn, 0);
}

void far scorefile_view_assets_free(void)
{
	extern int entered_place;

	if(entered_place == -1) {
		cdg_free(0);
		pi_free(0);
	}
}

void far scorefile_view_frame_begin(void)
{
	enum {
		RANK_IMAGE_W = 320,
		RANK_IMAGE_H = 88,
	};

	// All (0xFF) and the even final character page make every browser step
	// alternate this bit, so it always identifies the hidden graphics page.
	graph_accesspage(scorefile_view_page & 1);
	pi_put_8(0, 0, 0);
	cdg_put_8((RES_X - RANK_IMAGE_W), (RES_Y - RANK_IMAGE_H), 0);
}

void far scorefile_view_frame_end(void)
{
	scorefile_view_overlay_put();
	vsync_wait();
	graph_showpage(scorefile_view_page & 1);
	graph_accesspage(scorefile_view_page & 1);
}

#else

void far scorefile_view_overlay_put(void)
{
}

#endif

void far scorestat_run_begin(void)
{
	uint32_t run_id;
	uint8_t playchar;

	playchar = resident->playchar_paletted[0].char_id();
	if(
		(resident->game_mode != GM_STORY) ||
		(resident->demo_num != 0) || practice_game_active() ||
		(resident->rank >= RANK_COUNT) || (playchar >= PLAYCHAR_COUNT) ||
		!scorefile_ensure()
	) {
		scorestat_clear();
		scorefile_close();
		return;
	}
	run_id = scorefile->header.last_run_id + 1;
	if(run_id == 0) {
		run_id = 1;
	}
	scorefile_delete(T3_SCORETIME_FN);
	scorestat_resident_u8_set(
		T3_SCORESTAT_RES_MAGIC_0_INDEX, T3_SCORESTAT_MAGIC_0
	);
	scorestat_resident_u8_set(
		T3_SCORESTAT_RES_MAGIC_1_INDEX, T3_SCORESTAT_MAGIC_1
	);
	scorestat_resident_u8_set(
		T3_SCORESTAT_RES_VERSION_INDEX, T3_SCORESTAT_VERSION
	);
	scorestat_resident_u8_set(
		T3_SCORESTAT_RES_ACTIVE_INDEX, T3_SCORESTAT_ACTIVE
	);
	scorestat_resident_u8_set(
		T3_SCORESTAT_RES_RANK_INDEX, resident->rank
	);
	scorestat_resident_u8_set(
		T3_SCORESTAT_RES_PLAYCHAR_INDEX, playchar
	);
	scorestat_resident_u32_set(T3_SCORESTAT_RES_FRAMES_INDEX, 0);
	scorestat_resident_u32_set(T3_SCORESTAT_RES_RUN_ID_INDEX, run_id);
	scorestat_process_sync();
	scorefile_close();
}

void far scorestat_continue_accept(void)
{
	uint8_t playchar;
	uint32_t continues_before;
	scorefile_board_t far *board;

	scorestat_process_sync();
	if(
		!scorefile_ensure() || (resident->game_mode != GM_STORY) ||
		(resident->demo_num != 0) || practice_game_active()
	) {
		return;
	}
	playchar = resident->playchar_paletted[0].char_id();
	if((resident->rank >= RANK_COUNT) || (playchar >= PLAYCHAR_COUNT)) {
		return;
	}
	board = &scorefile->boards[resident->rank][playchar];
	continues_before = board->stats.continues_used;
	board->stats.continues_used = score_add_saturating(
		board->stats.continues_used, 1
	);
	if(scorefile_save_atomic()) {
		if(!scorestat_active()) {
			scorestat_run_begin();
		}
	} else {
		board->stats.continues_used = continues_before;
		scorefile_checksums_set();
	}
}

void far scorefile_close(void)
{
	if(scorefile) {
		hmem_free(reinterpret_cast<void __seg *>(scorefile));
		scorefile = 0;
	}
	if(scorefile_scratch) {
		hmem_free(reinterpret_cast<void __seg *>(scorefile_scratch));
		scorefile_scratch = 0;
	}
}

#endif /* BINARY != 'M' */

// Keep the compiler runtime segment at its accepted per-binary paragraph
// phase. These bytes live entirely in this patch-owned segment.
#if (BINARY == 'O')
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90"
#elif (BINARY == 'L')
#pragma codestring "\x90\x90\x90"
#elif (BINARY == 'M')
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#endif
