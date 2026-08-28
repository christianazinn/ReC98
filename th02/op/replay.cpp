// TH02 replay and Practice title surface.
//
// This file is included by op_01.cpp so that the existing OP link list stays
// untouched.  The pragma keeps all implementation in a trailing patch-owned
// code segment; all persistent state below is uninitialized BSS.

#pragma codeseg T2OPRPLY_TEXT PATCH

#include "x86real.h"
#include "platform/x86real/pc98/keyboard.hpp"
#include "th02/replay_format.hpp"
#include "th02/practice_diag.hpp"

#define T2OP_LINE_CAPACITY 79
#define T2OP_SLOT_ROWS 10
#define T2OP_INPUT_KNOWN 0xF1FF
#define T2OP_DOS_ACCESS_READ 0
#define T2OP_DOS_ACCESS_RW 2
#define T2OP_FP_SEG(p) ((unsigned)(((unsigned long)(void far *)(p)) >> 16))
#define T2OP_FP_OFF(p) ((unsigned)((unsigned long)(void far *)(p)))

// Browser fields are deliberately independent. The original single padded
// Shift-JIS line made a character or rank width change move later columns.
#define T2OP_BROWSER_MARKER_X 5
#define T2OP_BROWSER_SLOT_X 7
#define T2OP_BROWSER_NAME_X 15
#define T2OP_BROWSER_SHOT_X 33
#define T2OP_BROWSER_RANK_X 41
#define T2OP_BROWSER_SCORE_X 50
#define T2OP_BROWSER_STAGE_X 63
#define T2OP_DETAIL_SPLIT_STAGE_X 41
#define T2OP_DETAIL_SPLIT_SCORE_X 54
#define T2OP_PRACTICE_LABEL_X 7
#define T2OP_PRACTICE_VALUE_RIGHT 75
#define T2OP_TITLE_COMMAND_FIRST_ROW 16
#define T2OP_TITLE_RANK_ROW 23
#define T2OP_TITLE_QUIT_ROW 24

extern char sel;

enum t2op_word_t {
	T2OW_START,
	T2OW_EXTRA,
	T2OW_PRACTICE,
	T2OW_REPLAY,
	T2OW_HISCORE,
	T2OW_OPTIONS,
	T2OW_MUSIC_ROOM,
	T2OW_QUIT,
	T2OW_RANK,
	T2OW_EASY,
	T2OW_NORMAL,
	T2OW_HARD,
	T2OW_LUNATIC,
	T2OW_SHOT,
	T2OW_CHARACTER,
	T2OW_REIMU,
	T2OW_MARISA,
	T2OW_MIMA,
	T2OW_STAGE,
	T2OW_SECTION,
	T2OW_STAGE_START,
	T2OW_CHAPTER_2,
	T2OW_CHAPTER_3,
	T2OW_MIDBOSS,
	T2OW_BOSS_PHASE_1,
	T2OW_BOSS_PHASE_2,
	T2OW_BOSS_PHASE_3,
	T2OW_BOSS_START,
	T2OW_INNER_PAIR,
	T2OW_OUTER_PAIR,
	T2OW_SCORE,
	T2OW_HIGH_SCORE,
	T2OW_POWER,
	T2OW_LIVES,
	T2OW_BOMBS,
	T2OW_SEED,
	T2OW_SKILL,
	T2OW_BGM,
	T2OW_REDUCED_EFFECTS,
	T2OW_OFF,
	T2OW_ON,
	T2OW_FM,
	T2OW_MIDI,
	T2OW_BROWSER,
	T2OW_SAVE_REPLAY,
	T2OW_OVERWRITE_REPLAY,
	T2OW_YES,
	T2OW_NO,
	T2OW_SLOT,
	T2OW_NAME,
	T2OW_NONE,
	T2OW_INVALID,
	T2OW_CLEAR,
	T2OW_GAME_OVER,
	T2OW_MENU_RETURN,
	T2OW_PAGE,
	T2OW_FINAL_SCORE,
	T2OW_START_POINT,
	T2OW_STAGE_SPLITS,
	T2OW_START_RUN,
	T2OW_BOSS_ROUND_2,
	T2OW_COUNT,
};

enum t2op_main_choice_t {
	T2OMC_START,
	T2OMC_EXTRA,
	T2OMC_PRACTICE,
	T2OMC_REPLAY,
	T2OMC_HISCORE,
	T2OMC_OPTIONS,
	T2OMC_MUSIC,
	T2OMC_QUIT,
	T2OMC_COUNT,
};

enum t2op_practice_choice_t {
	T2OPC_STAGE,
	T2OPC_SECTION,
	T2OPC_SCORE,
	T2OPC_HIGH_SCORE,
	T2OPC_POWER,
	T2OPC_LIVES,
	T2OPC_BOMBS,
	T2OPC_SEED,
	T2OPC_SKILL,
	T2OPC_BGM,
	T2OPC_EFFECTS,
	T2OPC_START,
	T2OPC_COUNT,
};

static char t2op_line[T2OP_LINE_CAPACITY + 1];
static uint8_t t2op_gaiji_line[T2OP_LINE_CAPACITY + 1];
static char t2op_command_fn[10];
static char t2op_slot_fn[11];
static bool t2op_paths_ready;
static bool t2op_main_initialized;
static bool t2op_main_input_allowed;
bool replay_title_restore_needed;
static uint8_t t2op_main_sel;
static uint8_t t2op_browser_sel;
static uint8_t t2op_practice_sel;
static uint8_t t2op_pending_source;
static t2replay_header_t t2op_header;
static t2replay_header_t t2op_header_saved;
static t2replay_start_t t2op_practice;
static uint8_t t2op_pending_name[T2REPLAY_NAME_LEN];

// shottype_menu() and score_menu() borrow their own gaiji state.  Patch-owned
// title surfaces must put OP's selected MIKOFT table back before writing TRAM.
static void t2op_title_font_restore(void);
static void t2op_title_return_request(void);

#if T2REPLAY_PRACTICE_DIAGNOSTICS
void replay_practice_diag_boot(unsigned char milestone)
{
	static const char fn[] = "T2BOOT.BIN";

	if(file_create(fn)) {
		file_write(&milestone, sizeof(milestone));
		file_close();
	}
}
#endif

static void t2op_memclear(void far *buf, unsigned size)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(buf);

	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

static void t2op_file_delete(const char far *fn);
static bool t2op_file_rename(
	const char far *source, const char far *destination
);

static void t2op_paths_init(void)
{
	if(t2op_paths_ready) {
		return;
	}
	t2op_command_fn[0] = 'T';
	t2op_command_fn[1] = '2';
	t2op_command_fn[2] = 'R';
	t2op_command_fn[3] = 'P';
	t2op_command_fn[4] = 'Y';
	t2op_command_fn[5] = '.';
	t2op_command_fn[6] = 'C';
	t2op_command_fn[7] = 'F';
	t2op_command_fn[8] = 'G';
	t2op_command_fn[9] = '\0';
	t2op_slot_fn[0] = 'T';
	t2op_slot_fn[1] = 'H';
	t2op_slot_fn[2] = '2';
	t2op_slot_fn[3] = 'R';
	t2op_slot_fn[4] = '0';
	t2op_slot_fn[5] = '0';
	t2op_slot_fn[6] = '.';
	t2op_slot_fn[7] = 'R';
	t2op_slot_fn[8] = 'P';
	t2op_slot_fn[9] = 'Y';
	t2op_slot_fn[10] = '\0';
	t2op_paths_ready = true;
}

static void t2op_slot_set(uint8_t slot)
{
	t2op_slot_fn[4] = static_cast<char>('0' + (slot / 10));
	t2op_slot_fn[5] = static_cast<char>('0' + (slot % 10));
}

static void t2op_temp_set(void)
{
	t2op_slot_fn[0] = 'T';
	t2op_slot_fn[1] = '2';
	t2op_slot_fn[2] = 'R';
	t2op_slot_fn[3] = 'P';
	t2op_slot_fn[4] = 'Y';
	t2op_slot_fn[5] = '.';
	t2op_slot_fn[6] = 'T';
	t2op_slot_fn[7] = 'M';
	t2op_slot_fn[8] = 'P';
	t2op_slot_fn[9] = '\0';
}

static void t2op_save_request_fn_set(char *fn)
{
	fn[0] = 'T';
	fn[1] = '2';
	fn[2] = 'R';
	fn[3] = 'S';
	fn[4] = 'A';
	fn[5] = 'V';
	fn[6] = '.';
	fn[7] = 'C';
	fn[8] = 'F';
	fn[9] = 'G';
	fn[10] = '\0';
}

static void t2op_handoff_fn_set(char *fn)
{
	fn[0] = 'T';
	fn[1] = '2';
	fn[2] = 'R';
	fn[3] = 'H';
	fn[4] = 'A';
	fn[5] = 'N';
	fn[6] = 'D';
	fn[7] = '.';
	fn[8] = 'B';
	fn[9] = 'I';
	fn[10] = 'N';
	fn[11] = '\0';
}

static bool t2op_bytes_zero(const uint8_t far *p, unsigned size)
{
	while(size != 0) {
		if(*p++ != 0) {
			return false;
		}
		size--;
	}
	return true;
}

static bool t2op_name_glyph_valid(uint8_t glyph)
{
	return (
		((glyph >= gb_0) && (glyph <= gb_Z)) ||
		(glyph == gs_YINYANG) ||
		(glyph == gs_BOMB) ||
		((glyph >= gs_BULLET) && (glyph <= gs_ELLIPSIS)) ||
		((glyph >= gs_HEART) && (glyph <= gs_SPACE))
	);
}

// An all-zero name predates the save-name UI and is retained as the
// backward-compatible representation of an unnamed capture.
static bool t2op_name_valid(const uint8_t far *name)
{
	unsigned i;
	bool all_zero = true;

	for(i = 0; i < T2REPLAY_NAME_LEN; i++) {
		if(name[i] != 0) {
			all_zero = false;
		}
	}
	if(all_zero) {
		return true;
	}
	for(i = 0; i < T2REPLAY_NAME_LEN; i++) {
		if(!t2op_name_glyph_valid(name[i])) {
			return false;
		}
	}
	return true;
}

static uint32_t t2op_fnv1a(uint32_t hash, const void far *buf, unsigned size)
{
	const uint8_t far *p = reinterpret_cast<const uint8_t far *>(buf);

	while(size != 0) {
		hash ^= static_cast<uint32_t>(*p++);
		hash *= T2REPLAY_FNV1A_PRIME;
		size--;
	}
	return hash;
}

static bool t2op_start_valid(const t2replay_start_t far *start)
{
	uint8_t practice_target = start->reserved[T2REPLAY_PRACTICE_TARGET_OFFSET];
	bool practice_target_valid = false;

	switch(practice_target) {
	case T2RPT_STAGE_START:
		practice_target_valid = true;
		break;
	case T2RPT_STAGE1_CHAPTER2:
		practice_target_valid = (start->stage == 0);
		break;
	case T2RPT_STAGE2_CHAPTER2:
		practice_target_valid = (start->stage == 1);
		break;
	case T2RPT_STAGE3_CHAPTER2:
		practice_target_valid = (start->stage == 2);
		break;
	case T2RPT_STAGE4_CHAPTER2:
	case T2RPT_STAGE4_CHAPTER3:
		practice_target_valid = (start->stage == 3);
		break;
	case T2RPT_EXTRA_CHAPTER2:
		practice_target_valid = (start->stage == 5);
		break;
	case T2RPT_STAGE1_MIDBOSS:
	case T2RPT_STAGE1_BOSS_PHASE1:
	case T2RPT_STAGE1_BOSS_PHASE2:
	case T2RPT_STAGE1_BOSS_PHASE3:
		practice_target_valid = (start->stage == 0);
		break;
	case T2RPT_STAGE2_MIDBOSS:
	case T2RPT_STAGE2_BOSS_PHASE1:
	case T2RPT_STAGE2_BOSS_PHASE2:
	case T2RPT_STAGE2_BOSS_PHASE3:
		practice_target_valid = (start->stage == 1);
		break;
	case T2RPT_STAGE3_MIDBOSS:
	case T2RPT_STAGE3_BOSS_START:
	case T2RPT_STAGE3_INNER_PAIR:
	case T2RPT_STAGE3_OUTER_PAIR:
		practice_target_valid = (start->stage == 2);
		break;
	case T2RPT_STAGE4_MIDBOSS_FIRST:
	case T2RPT_STAGE4_MIDBOSS_SECOND:
	case T2RPT_STAGE4_BOSS_START:
	case T2RPT_STAGE4_BOSS_PHASE1:
	case T2RPT_STAGE4_BOSS_ROUND2:
		practice_target_valid = (start->stage == 3);
		break;
	case T2RPT_STAGE5_BOSS_START:
	case T2RPT_STAGE5_BOSS_PHASE1:
	case T2RPT_STAGE5_BOSS_PHASE3:
		practice_target_valid = (start->stage == 4);
		break;
	case T2RPT_EXTRA_MIDBOSS:
	case T2RPT_EXTRA_BOSS_START:
	case T2RPT_EXTRA_BOSS_PHASE1:
	case T2RPT_EXTRA_BOSS_PHASE3:
		practice_target_valid = (start->stage == 5);
		break;
	default:
		break;
	}
	return (
		(start->stage >= 0) &&
		(start->stage < T2REPLAY_STAGE_COUNT) &&
		(start->rank <= RANK_EXTRA) &&
		((start->stage == (T2REPLAY_STAGE_COUNT - 1)) ==
		 (start->rank == RANK_EXTRA)) &&
		(start->rem_lives >= 0) &&
		(start->rem_lives <= 5) &&
		(start->rem_bombs >= 0) &&
		(start->rem_bombs <= 5) &&
		(start->start_lives >= 1) &&
		(start->start_lives <= 5) &&
		(start->start_bombs >= 1) &&
		(start->start_bombs <= 5) &&
		(start->start_power >= 0) &&
		(start->start_power <= 80) &&
		(start->random_seed == start->resident_frame) &&
		(start->shottype < SHOTTYPE_COUNT) &&
		(start->bgm_mode <= SND_BGM_MIDI) &&
		(start->reduce_effects <= 1) &&
		(start->debug == 0) &&
		practice_target_valid &&
		t2op_bytes_zero(
			&start->reserved[T2REPLAY_PRACTICE_RESERVED_OFFSET],
			T2REPLAY_PRACTICE_RESERVED_SIZE
		)
	);
}

static bool t2op_header_valid(void)
{
	uint32_t stored = t2op_header.header_checksum;
	uint32_t computed;
	uint8_t first_stage;
	uint8_t stage;

	if(
		(t2op_header.magic[0] != 'T') ||
		(t2op_header.magic[1] != '2') ||
		(t2op_header.magic[2] != 'R') ||
		(t2op_header.magic[3] != 'P') ||
		(t2op_header.magic[4] != 'Y') ||
		(t2op_header.magic[5] != '1') ||
		(t2op_header.magic[6] != '\0') ||
		(t2op_header.magic[7] != '\0') ||
		(t2op_header.version != T2REPLAY_VERSION) ||
		(t2op_header.header_size != T2REPLAY_HEADER_SIZE) ||
		(t2op_header.packet_size != T2REPLAY_PACKET_SIZE) ||
		((t2op_header.flags & T2REPLAY_REQUIRED_FLAGS) !=
		 T2REPLAY_REQUIRED_FLAGS) ||
		((t2op_header.flags & ~T2REPLAY_KNOWN_FLAGS) != 0) ||
		(t2op_header.status != T2REPLAY_STATUS_FINALIZED) ||
		(t2op_header.game_id != 2) ||
		(t2op_header.ruleset != T2REPLAY_RULESET_STOCK) ||
		(t2op_header.input_semantics != T2REPLAY_INPUT_SEMANTICS_KEY_DET) ||
		(t2op_header.stage_count != T2REPLAY_STAGE_COUNT) ||
		(t2op_header.stage_reached >= T2REPLAY_STAGE_COUNT) ||
		(t2op_header.terminal_stage >= T2REPLAY_STAGE_COUNT) ||
		(t2op_header.end_reason < T2REPLAY_END_GAME_OVER) ||
		(t2op_header.end_reason > T2REPLAY_END_MENU_RETURN) ||
		(t2op_header.input_offset != T2REPLAY_HEADER_SIZE) ||
		(t2op_header.input_size > T2REPLAY_INPUT_SIZE_MAX) ||
		(t2op_header.packet_count >
		 (T2REPLAY_INPUT_SIZE_MAX / T2REPLAY_PACKET_SIZE)) ||
		(t2op_header.input_size !=
		 (t2op_header.packet_count * T2REPLAY_PACKET_SIZE)) ||
		!t2op_start_valid(&t2op_header.start) ||
		!t2op_name_valid(
			t2op_header.reserved + T2REPLAY_RESERVED_NAME_OFFSET
		) ||
		!t2op_bytes_zero(
			t2op_header.reserved + T2REPLAY_RESERVED_TAIL_OFFSET,
			T2REPLAY_RESERVED_TAIL_SIZE
		)
	) {
		return false;
	}
	first_stage = static_cast<uint8_t>(t2op_header.start.stage);
	for(stage = 0; stage < T2REPLAY_STAGE_COUNT; stage++) {
		if(
			((stage < first_stage) || (stage > t2op_header.stage_reached)) &&
			(t2op_header.stage_scores[stage] != 0)
		) {
			return false;
		}
	}
	t2op_header.header_checksum = 0;
	computed = t2op_fnv1a(
		T2REPLAY_FNV1A_BASIS, &t2op_header, sizeof(t2op_header)
	);
	t2op_header.header_checksum = stored;
	return (stored == computed);
}

static int t2op_dos_open(const char far *fn, unsigned char access)
{
	unsigned fn_seg = T2OP_FP_SEG(fn);
	unsigned fn_off = T2OP_FP_OFF(fn);
	int result;

	_asm {
		push	ds
		mov	dx, fn_off
		mov	ds, fn_seg
		mov	ah, 3Dh
		mov	al, access
		int	21h
		pop	ds
		sbb	dx, dx
		or	ax, dx
		mov	result, ax
	}
	return result;
}

static int t2op_dos_create(const char far *fn)
{
	unsigned fn_seg = T2OP_FP_SEG(fn);
	unsigned fn_off = T2OP_FP_OFF(fn);
	int result;

	_asm {
		push	ds
		mov	dx, fn_off
		mov	ds, fn_seg
		mov	ah, 3Ch
		xor	cx, cx
		int	21h
		pop	ds
		sbb	dx, dx
		or	ax, dx
		mov	result, ax
	}
	return result;
}

static void t2op_dos_close(int fh)
{
	_asm {
		mov	bx, fh
		mov	ah, 3Eh
		int	21h
	}
}

static bool t2op_dos_seek(int fh, uint32_t offset)
{
	unsigned offset_hi = static_cast<unsigned>(offset >> 16);
	unsigned offset_lo = static_cast<unsigned>(offset & 0xFFFFUL);
	unsigned failed;

	_asm {
		mov	bx, fh
		mov	cx, offset_hi
		mov	dx, offset_lo
		mov	ax, 4200h
		int	21h
		sbb	ax, ax
		neg	ax
		mov	failed, ax
	}
	return (failed == 0);
}

static bool t2op_dos_size(int fh, uint32_t far *size)
{
	unsigned size_hi;
	unsigned size_lo;
	unsigned failed;

	_asm {
		mov	bx, fh
		xor	cx, cx
		xor	dx, dx
		mov	ax, 4202h
		int	21h
		mov	size_lo, ax
		mov	size_hi, dx
		sbb	ax, ax
		neg	ax
		mov	failed, ax
	}
	*size = (
		(static_cast<uint32_t>(size_hi) << 16) |
		static_cast<uint32_t>(size_lo)
	);
	return (failed == 0);
}

static unsigned t2op_dos_read(int fh, void far *buf, unsigned size)
{
	unsigned buf_seg = T2OP_FP_SEG(buf);
	unsigned buf_off = T2OP_FP_OFF(buf);
	unsigned result;

	_asm {
		push	ds
		mov	bx, fh
		mov	cx, size
		mov	dx, buf_off
		mov	ds, buf_seg
		mov	ah, 3Fh
		int	21h
		pop	ds
		sbb	cx, cx
		not	cx
		and	ax, cx
		mov	result, ax
	}
	return result;
}

static unsigned t2op_dos_write(int fh, const void far *buf, unsigned size)
{
	unsigned buf_seg = T2OP_FP_SEG(buf);
	unsigned buf_off = T2OP_FP_OFF(buf);
	unsigned result;

	_asm {
		push	ds
		mov	bx, fh
		mov	cx, size
		mov	dx, buf_off
		mov	ds, buf_seg
		mov	ah, 40h
		int	21h
		pop	ds
		sbb	cx, cx
		not	cx
		and	ax, cx
		mov	result, ax
	}
	return result;
}

static void t2op_dos_flush(void)
{
	_asm {
		mov	ah, 0Dh
		int	21h
	}
}

static void t2op_header_checksum_set(void)
{
	t2op_header.header_checksum = 0;
	t2op_header.header_checksum = t2op_fnv1a(
		T2REPLAY_FNV1A_BASIS, &t2op_header, sizeof(t2op_header)
	);
}

static bool t2op_pending_header_write(void)
{
	int fd;
	bool written;

	fd = t2op_dos_open(t2op_slot_fn, T2OP_DOS_ACCESS_RW);
	if(fd < 0) {
		return false;
	}
	t2op_header_checksum_set();
	written = (
		t2op_dos_seek(fd, 0) &&
		(t2op_dos_write(fd, &t2op_header, sizeof(t2op_header)) ==
		 sizeof(t2op_header))
	);
	t2op_dos_close(fd);
	if(written) {
		t2op_dos_flush();
	}
	return written;
}

static bool t2op_save_request_read(
	const char far *fn, t2replay_save_request_t far *request
)
{
	uint32_t file_size;
	uint32_t stored;
	uint32_t computed;
	int fd;
	unsigned read;

	fd = t2op_dos_open(fn, T2OP_DOS_ACCESS_READ);
	if(fd < 0) {
		return false;
	}
	t2op_memclear(request, sizeof(*request));
	read = t2op_dos_read(fd, request, sizeof(*request));
	if(!t2op_dos_size(fd, &file_size)) {
		file_size = 0;
	}
	t2op_dos_close(fd);
	stored = request->checksum;
	request->checksum = 0;
	computed = t2op_fnv1a(T2REPLAY_FNV1A_BASIS, request, sizeof(*request));
	request->checksum = stored;
	return (
		(read == sizeof(*request)) &&
		(file_size == sizeof(*request)) &&
		(request->magic[0] == 'T') &&
		(request->magic[1] == '2') &&
		(request->magic[2] == 'R') &&
		(request->magic[3] == 'S') &&
		(request->magic[4] == 'A') &&
		(request->magic[5] == 'V') &&
		(request->magic[6] == '1') &&
		(request->magic[7] == '\0') &&
		(request->schema == T2REPLAY_SAVE_REQUEST_SCHEMA) &&
		(request->source >= T2REPLAY_SAVE_REQUEST_GAME_OVER) &&
		(request->source <= T2REPLAY_SAVE_REQUEST_MENU_RETURN) &&
		(request->reserved == 0) &&
		(request->replay_header_checksum != 0) &&
		(stored == computed)
	);
}

static bool t2op_pending_request_rebind(void)
{
	char request_fn[11];
	t2replay_save_request_t request;
	int fd;
	bool written;

	t2op_paths_init();
	t2op_save_request_fn_set(request_fn);
	if(!t2op_save_request_read(request_fn, &request)) {
		return false;
	}
	request.replay_header_checksum = t2op_header.header_checksum;
	request.checksum = 0;
	request.checksum = t2op_fnv1a(
		T2REPLAY_FNV1A_BASIS, &request, sizeof(request)
	);
	fd = t2op_dos_open(request_fn, T2OP_DOS_ACCESS_RW);
	if(fd < 0) {
		return false;
	}
	written = (
		t2op_dos_seek(fd, 0) &&
		(t2op_dos_write(fd, &request, sizeof(request)) == sizeof(request))
	);
	t2op_dos_close(fd);
	if(written) {
		t2op_dos_flush();
	}
	return written;
}

static bool t2op_packet_valid(
	const char far *packet, uint32_t *samples, bool *terminal_seen
)
{
	uint8_t tag = static_cast<uint8_t>(packet[0]);
	uint8_t phase = static_cast<uint8_t>(
		tag >> T2REPLAY_PACKET_PHASE_SHIFT
	);
	uint8_t low = static_cast<uint8_t>(tag & T2REPLAY_PACKET_RUN_MASK);
	uint8_t input_low = static_cast<uint8_t>(packet[1]);
	uint8_t input_high = static_cast<uint8_t>(packet[2]);
	uint8_t arg = static_cast<uint8_t>(packet[3]);
	input_t input;

	if(*terminal_seen) {
		return false;
	}
	if(phase < T2REPLAY_PHASE_CONTROL) {
		input = static_cast<input_t>(
			input_low | (static_cast<uint16_t>(input_high) << 8)
		);
		if((arg != 0) || (input & ~T2OP_INPUT_KNOWN)) {
			return false;
		}
		*samples += static_cast<uint32_t>(low + 1);
		return (*samples >= static_cast<uint32_t>(low + 1));
	}
	if(phase != T2REPLAY_PHASE_CONTROL) {
		return false;
	}
	if((low == T2REPLAY_CONTROL_STAGE_START) && (arg == 0)) {
		return (
			(input_high == 0) &&
			(input_low < T2REPLAY_STAGE_COUNT) &&
			!(*terminal_seen)
		);
	}
	if(low == T2REPLAY_CONTROL_TERMINAL) {
		if(
			(input_high != 0) ||
			((input_low != T2REPLAY_END_GAME_OVER) &&
			 (input_low != T2REPLAY_END_CLEAR) &&
			 (input_low != T2REPLAY_END_MENU_RETURN)) ||
			(arg >= T2REPLAY_STAGE_COUNT) ||
			*terminal_seen
		) {
			return false;
		}
		*terminal_seen = true;
		return true;
	}
	return false;
}

static bool t2op_pending_payload_valid(int fd, uint32_t file_size)
{
	uint32_t hash = T2REPLAY_FNV1A_BASIS;
	uint32_t samples = 0;
	uint32_t packets_seen = 0;
	bool terminal_seen = false;
	bool stage_seen = false;
	uint8_t expected_stage = static_cast<uint8_t>(t2op_header.start.stage);
	uint8_t terminal_reason = 0;
	uint8_t terminal_stage = 0;

	if(file_size != (t2op_header.input_offset + t2op_header.input_size)) {
		return false;
	}
	if(!t2op_dos_seek(fd, t2op_header.input_offset)) {
		return false;
	}
	while(packets_seen < t2op_header.packet_count) {
		if(t2op_dos_read(fd, t2op_line, T2REPLAY_PACKET_SIZE) !=
			T2REPLAY_PACKET_SIZE) {
			return false;
		}
		hash = t2op_fnv1a(hash, t2op_line, T2REPLAY_PACKET_SIZE);
		if(!t2op_packet_valid(t2op_line, &samples, &terminal_seen)) {
			return false;
		}
		if(static_cast<uint8_t>(t2op_line[0]) == static_cast<uint8_t>(
			(T2REPLAY_PHASE_CONTROL << T2REPLAY_PACKET_PHASE_SHIFT) |
			T2REPLAY_CONTROL_TERMINAL
		)) {
			if(!stage_seen || (static_cast<uint8_t>(t2op_line[3]) !=
				(expected_stage - 1))) {
				return false;
			}
			terminal_reason = static_cast<uint8_t>(t2op_line[1]);
			terminal_stage = static_cast<uint8_t>(t2op_line[3]);
		} else if(static_cast<uint8_t>(t2op_line[0]) == static_cast<uint8_t>(
			(T2REPLAY_PHASE_CONTROL << T2REPLAY_PACKET_PHASE_SHIFT) |
			T2REPLAY_CONTROL_STAGE_START
		)) {
			if(
				(expected_stage >= T2REPLAY_STAGE_COUNT) ||
				(static_cast<uint8_t>(t2op_line[1]) != expected_stage)
			) {
				return false;
			}
			stage_seen = true;
			expected_stage++;
		}
		packets_seen++;
	}
	return (
		(hash == t2op_header.payload_checksum) &&
		(samples == t2op_header.sample_count) &&
		terminal_seen &&
		(t2op_header.stage_reached == (expected_stage - 1)) &&
		(terminal_reason == t2op_header.end_reason) &&
		(terminal_stage == t2op_header.terminal_stage)
	);
}

static bool t2op_pending_replay_validate(const char far *fn)
{
	uint32_t file_size;
	int fd = t2op_dos_open(fn, T2OP_DOS_ACCESS_READ);

	if(fd < 0) {
		return false;
	}
	t2op_memclear(&t2op_header, sizeof(t2op_header));
	if((t2op_dos_read(fd, &t2op_header, sizeof(t2op_header)) !=
		 sizeof(t2op_header)) || !t2op_dos_size(fd, &file_size) ||
		!t2op_header_valid() || !t2op_pending_payload_valid(fd, file_size)) {
		t2op_dos_close(fd);
		return false;
	}
	t2op_dos_close(fd);
	return true;
}

static bool t2op_pending_request_valid(void)
{
	char request_fn[11];
	t2replay_save_request_t request;

	t2op_paths_init();
	t2op_save_request_fn_set(request_fn);
	t2op_temp_set();
	if(
		!t2op_save_request_read(request_fn, &request) ||
		!t2op_pending_replay_validate(t2op_slot_fn) ||
		(request.source != t2op_header.end_reason) ||
		(request.replay_header_checksum != t2op_header.header_checksum)
	) {
		// Keep an invalid temp for diagnostics, but remove its stale handoff.
		t2op_file_delete(request_fn);
		return false;
	}
	t2op_pending_source = request.source;
	return true;
}

static bool t2op_header_read_path(const char *fn)
{
	int read;

	if(!file_exist(fn) || !file_ropen(fn)) {
		return false;
	}
	t2op_memclear(&t2op_header, sizeof(t2op_header));
	read = file_read(&t2op_header, sizeof(t2op_header));
	file_close();
	if(read != sizeof(t2op_header)) {
		return false;
	}
	// Numbered-slot browsing stays header-only; MAIN owns full payload validation.
	// Pending T2RPY.TMP uses its dedicated check.
	return t2op_header_valid();
}

static void t2op_slot_backup_set(char *backup, uint8_t slot)
{
	uint8_t i;

	t2op_slot_set(slot);
	for(i = 0; i < sizeof(t2op_slot_fn); i++) {
		backup[i] = t2op_slot_fn[i];
	}
	backup[7] = 'B';
	backup[8] = 'A';
	backup[9] = 'K';
}

static void t2op_slot_recover(uint8_t slot)
{
	char backup[11];

	t2op_slot_backup_set(backup, slot);
	if(!file_exist(backup)) {
		return;
	}
	if(file_exist(t2op_slot_fn)) {
		t2op_file_delete(backup);
	} else {
		t2op_file_rename(backup, t2op_slot_fn);
	}
}

static bool t2op_header_read(uint8_t slot)
{
	t2op_paths_init();
	t2op_slot_recover(slot);
	t2op_slot_set(slot);
	return t2op_header_read_path(t2op_slot_fn);
}

static bool t2op_pending_header_read(void)
{
	return t2op_pending_request_valid();
}

static bool t2op_command_write(
	uint8_t mode, uint8_t slot, uint8_t flags, const t2replay_start_t far *start
)
{
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	t2replay_command_t command;
	char handoff_fn[12];
	int fh;
	bool wrote;
	bool practice = ((flags & T2REPLAY_COMMAND_FLAG_PRACTICE) != 0);

	t2op_paths_init();
	if(practice) {
		t2practice_diag_reset(mode, flags, start);
	} else {
		t2practice_diag_clear();
	}
	t2op_memclear(&command, sizeof(command));
	command.magic[0] = 'T';
	command.magic[1] = '2';
	command.magic[2] = 'R';
	command.magic[3] = 'C';
	command.magic[4] = 'F';
	command.magic[5] = 'G';
	command.magic[6] = '2';
	command.magic[7] = '\0';
	command.mode = mode;
	command.slot = slot;
	command.flags = flags;
	if(start != 0) {
		command.start = *start;
	}
	fh = t2op_dos_create(t2op_command_fn);
	if(fh < 0) {
		if(practice) {
			t2practice_diag_op_command(
				T2PDR_OP_COMMAND_CREATE, mode, flags, start
			);
		}
		return false;
	}
	wrote = (t2op_dos_write(fh, &command, sizeof(command)) == sizeof(command));
	t2op_dos_close(fh);
	if(!wrote) {
		t2op_file_delete(t2op_command_fn);
		if(practice) {
			t2practice_diag_op_command(
				T2PDR_OP_COMMAND_WRITE, mode, flags, start
			);
		}
		return false;
	}
	// MAIN consumes this file immediately after execl(). AH=0Dh alone did not
	// make the preceding directory update visible reliably on the target DOS,
	// so every command receives an unconditional second create/write/close.
	t2op_dos_flush();
	t2op_handoff_fn_set(handoff_fn);
	fh = t2op_dos_create(handoff_fn);
	if(fh < 0) {
		t2op_file_delete(t2op_command_fn);
		if(practice) {
			t2practice_diag_op_command(
				T2PDR_OP_WITNESS_CREATE, mode, flags, start
			);
		}
		return false;
	}
	wrote = (t2op_dos_write(fh, &command, sizeof(command)) == sizeof(command));
	t2op_dos_close(fh);
	if(!wrote) {
		t2op_file_delete(handoff_fn);
		t2op_file_delete(t2op_command_fn);
		if(practice) {
			t2practice_diag_op_command(
				T2PDR_OP_WITNESS_WRITE, mode, flags, start
			);
		}
		return false;
	}
	if(practice) {
		t2practice_diag_op_command(T2PDR_NONE, mode, flags, start);
		t2practice_diag_op_handoff(mode, flags, start);
	}
	return true;
#else
	t2replay_command_t command;
	char handoff_fn[12];
	int fh;
	bool wrote;

	t2op_paths_init();
	t2op_memclear(&command, sizeof(command));
	command.magic[0] = 'T';
	command.magic[1] = '2';
	command.magic[2] = 'R';
	command.magic[3] = 'C';
	command.magic[4] = 'F';
	command.magic[5] = 'G';
	command.magic[6] = '2';
	command.magic[7] = '\0';
	command.mode = mode;
	command.slot = slot;
	command.flags = flags;
	if(start != 0) {
		command.start = *start;
	}
	fh = t2op_dos_create(t2op_command_fn);
	if(fh < 0) {
		return false;
	}
	wrote = (t2op_dos_write(fh, &command, sizeof(command)) == sizeof(command));
	t2op_dos_close(fh);
	if(!wrote) {
		t2op_file_delete(t2op_command_fn);
		return false;
	}
	// MAIN consumes this file immediately after execl(). AH=0Dh alone did not
	// make the preceding directory update visible reliably on the target DOS,
	// so every command receives an unconditional second create/write/close.
	t2op_dos_flush();
	t2op_handoff_fn_set(handoff_fn);
	fh = t2op_dos_create(handoff_fn);
	if(fh < 0) {
		t2op_file_delete(t2op_command_fn);
		return false;
	}
	wrote = (t2op_dos_write(fh, &command, sizeof(command)) == sizeof(command));
	t2op_dos_close(fh);
	if(!wrote) {
		t2op_file_delete(handoff_fn);
		t2op_file_delete(t2op_command_fn);
		return false;
	}
	return true;
#endif
}

static bool t2op_restart_command_read(
	t2replay_start_t far *start, uint8_t far *flags
)
{
	t2replay_command_t command;
	char handoff_fn[12];
	uint32_t file_size;
	unsigned size;
	unsigned i;
	int fd;
	bool valid;

	t2op_paths_init();
	// The primary command is authoritative. The second file exists only to
	// force the preceding directory update across the process boundary.
	t2op_handoff_fn_set(handoff_fn);
	t2op_file_delete(handoff_fn);
	fd = t2op_dos_open(t2op_command_fn, T2OP_DOS_ACCESS_READ);
	if(fd < 0) {
		return false;
	}
	t2op_memclear(&command, sizeof(command));
	size = t2op_dos_read(fd, &command, sizeof(command));
	if(!t2op_dos_size(fd, &file_size)) {
		file_size = 0;
	}
	t2op_dos_close(fd);
	// T2RPY.CFG is one-shot state. Rejecting malformed or stale content must not
	// leave OP retrying the same command on every title entry.
	t2op_file_delete(t2op_command_fn);
	if(
		(command.magic[0] != 'T') || (command.magic[1] != '2') ||
		(command.magic[2] != 'R') || (command.magic[3] != 'C') ||
		(command.magic[4] != 'F') || (command.magic[5] != 'G') ||
		(command.magic[6] != '2') || (command.magic[7] != '\0') ||
		(command.mode != T2REPLAY_COMMAND_RESTART)
	) {
		return false;
	}
	valid = (
		(size == sizeof(command)) && (file_size == sizeof(command)) &&
		(command.slot == 0) &&
		((command.flags & ~T2REPLAY_COMMAND_FLAG_PRACTICE) == 0) &&
		(command.reserved_0 == 0) && t2op_start_valid(&command.start)
	);
	for(i = 0; i < sizeof(command.reserved); i++) {
		if(command.reserved[i] != 0) {
			valid = false;
		}
	}
	if(
		(command.flags == 0) &&
		(command.start.reserved[T2REPLAY_PRACTICE_TARGET_OFFSET] !=
		 T2RPT_STAGE_START)
	) {
		valid = false;
	}
	if(!valid) {
		return false;
	}
	*start = command.start;
	*flags = command.flags;
	return true;
}

static bool t2op_file_rename(const char far *source, const char far *destination)
{
	unsigned source_seg = T2OP_FP_SEG(source);
	unsigned source_off = T2OP_FP_OFF(source);
	unsigned destination_seg = T2OP_FP_SEG(destination);
	unsigned destination_off = T2OP_FP_OFF(destination);
	unsigned failed;

	_asm {
		push	ds
		push	es
		mov	dx, source_off
		mov	ax, source_seg
		mov	ds, ax
		mov	di, destination_off
		mov	ax, destination_seg
		mov	es, ax
		mov	ah, 56h
		int	21h
		pop	es
		pop	ds
		sbb	ax, ax
		neg	ax
		mov	failed, ax
	}
	return (failed == 0);
}

static void t2op_file_delete(const char far *fn)
{
	unsigned fn_seg = T2OP_FP_SEG(fn);
	unsigned fn_off = T2OP_FP_OFF(fn);

	_asm {
		push	ds
		mov	dx, fn_off
		mov	ax, fn_seg
		mov	ds, ax
		mov	ah, 41h
		int	21h
		pop	ds
	}
}

static char *t2op_char(char *p, char c)
{
	*p++ = c;
	return p;
}

// The OP language tables own stock text.  This table is limited to the
// Replay and Practice surfaces added in this trailing patch segment.
static char *t2op_word_append_japanese(char *p, t2op_word_t word)
{
	#define P(c) p = t2op_char(p, static_cast<char>(c))
	switch(word) {
	case T2OW_START: P(0x8A); P(0x4A); P(0x8E); P(0x6E); break;
	case T2OW_EXTRA: P('E'); P('X'); P('T'); P('R'); P('A'); break;
	case T2OW_PRACTICE: P(0x97); P(0xFB); P(0x8F); P(0x4B); break;
	case T2OW_REPLAY: P(0x8D); P(0xC4); P(0x90); P(0xB6); break;
	case T2OW_HISCORE: P(0x83); P(0x6E); P(0x83); P(0x43); P(0x83); P(0x58); P(0x83); P(0x52); P(0x83); P(0x41); break;
	case T2OW_OPTIONS: P(0x83); P(0x49); P(0x83); P(0x76); P(0x83); P(0x56); P(0x83); P(0x87); P(0x83); P(0x93); break;
	case T2OW_MUSIC_ROOM: P(0x83); P(0x7E); P(0x83); P(0x85); P(0x81); P(0x5B); P(0x83); P(0x57); P(0x83); P(0x62); P(0x83); P(0x4E); break;
	case T2OW_QUIT: P(0x8F); P(0x49); P(0x97); P(0xB9); break;
	case T2OW_RANK: P(0x83); P(0x89); P(0x83); P(0x93); P(0x83); P(0x4E); break;
	case T2OW_EASY: P(0x83); P(0x43); P(0x81); P(0x5B); P(0x83); P(0x57); P(0x81); P(0x5B); break;
	case T2OW_NORMAL: P(0x83); P(0x6D); P(0x81); P(0x5B); P(0x83); P(0x7D); P(0x83); P(0x8B); break;
	case T2OW_HARD: P(0x83); P(0x6E); P(0x81); P(0x5B); P(0x83); P(0x68); break;
	case T2OW_LUNATIC: P(0x83); P(0x8B); P(0x83); P(0x69); P(0x83); P(0x65); P(0x83); P(0x42); P(0x83); P(0x62); P(0x83); P(0x4E); break;
	case T2OW_SHOT: P(0x83); P(0x56); P(0x83); P(0x87); P(0x83); P(0x62); P(0x83); P(0x67); break;
	case T2OW_CHARACTER: P(0x83); P(0x4C); P(0x83); P(0x83); P(0x83); P(0x89); P(0x83); P(0x4E); P(0x83); P(0x5E); P(0x81); P(0x5B); break;
	case T2OW_REIMU: P(0x97); P(0xEC); P(0x96); P(0xB2); break;
	case T2OW_MARISA: P(0x96); P(0x82); P(0x97); P(0x9D); P(0x8D); P(0xB9); break;
	case T2OW_MIMA: P(0x96); P(0xA3); P(0x96); P(0x82); break;
	case T2OW_STAGE: P(0x83); P(0x58); P(0x83); P(0x65); P(0x81); P(0x5B); P(0x83); P(0x57); break;
	case T2OW_SECTION: P(0x8A); P(0x4A); P(0x8E); P(0x6E); P(0x88); P(0xCA); P(0x92); P(0x75); break;
	case T2OW_STAGE_START: P(0x83); P(0x58); P(0x83); P(0x65); P(0x81); P(0x5B); P(0x83); P(0x57); P(0x8A); P(0x4A); P(0x8E); P(0x6E); break;
	case T2OW_CHAPTER_2: P(0x92); P(0x86); P(0x94); P(0xD5); break;
	case T2OW_CHAPTER_3: P(0x8C); P(0xE3); P(0x94); P(0xBC); break;
	case T2OW_MIDBOSS: P(0x92); P(0x86); P(0x83); P(0x7B); P(0x83); P(0x58); break;
	case T2OW_BOSS_PHASE_1: P(0x83); P(0x7B); P(0x83); P(0x58); P(0x91); P(0xE6); P('1'); P(0x8C); P(0x60); P(0x91); P(0xD4); break;
	case T2OW_BOSS_ROUND_2:
	case T2OW_BOSS_PHASE_2: P(0x83); P(0x7B); P(0x83); P(0x58); P(0x91); P(0xE6); P('2'); P(0x8C); P(0x60); P(0x91); P(0xD4); break;
	case T2OW_BOSS_PHASE_3: P(0x83); P(0x7B); P(0x83); P(0x58); P(0x91); P(0xE6); P('3'); P(0x8C); P(0x60); P(0x91); P(0xD4); break;
	case T2OW_BOSS_START: P(0x83); P(0x7B); P(0x83); P(0x58); P(0x8A); P(0x4A); P(0x8E); P(0x6E); break;
	case T2OW_INNER_PAIR: P(0x93); P(0xE0); P(0x91); P(0xA4); P(0x83); P(0x79); P(0x83); P(0x41); break;
	case T2OW_OUTER_PAIR: P(0x8A); P(0x4F); P(0x91); P(0xA4); P(0x83); P(0x79); P(0x83); P(0x41); break;
	case T2OW_SCORE: P(0x83); P(0x58); P(0x83); P(0x52); P(0x83); P(0x41); break;
	case T2OW_HIGH_SCORE: P(0x83); P(0x6E); P(0x83); P(0x43); P(0x83); P(0x58); P(0x83); P(0x52); P(0x83); P(0x41); break;
	case T2OW_POWER: P(0x97); P(0xEC); P(0x97); P(0xCD); break;
	case T2OW_LIVES: P(0x8E); P(0x63); P(0x8B); P(0x40); break;
	case T2OW_BOMBS: P(0x83); P(0x7B); P(0x83); P(0x80); break;
	case T2OW_SEED: P(0x97); P(0x90); P(0x90); P(0x94); break;
	case T2OW_SKILL: P(0x8B); P(0x5A); P(0x97); P(0xCA); break;
	case T2OW_BGM: P('B'); P('G'); P('M'); break;
	case T2OW_REDUCED_EFFECTS: P(0x89); P(0x89); P(0x8F); P(0x6F); P(0x8C); P(0x79); P(0x8C); P(0xB8); break;
	case T2OW_OFF: P(0x83); P(0x49); P(0x83); P(0x74); break;
	case T2OW_ON: P(0x83); P(0x49); P(0x83); P(0x93); break;
	case T2OW_FM: P('F'); P('M'); break;
	case T2OW_MIDI: P('M'); P('I'); P('D'); P('I'); break;
	case T2OW_BROWSER: P(0x83); P(0x8A); P(0x83); P(0x76); P(0x83); P(0x8C); P(0x83); P(0x43); P(0x88); P(0xEA); P(0x97); P(0x97); break;
	case T2OW_SAVE_REPLAY: P(0x83); P(0x8A); P(0x83); P(0x76); P(0x83); P(0x8C); P(0x83); P(0x43); P(0x95); P(0xDB); P(0x91); P(0xB6); break;
	case T2OW_OVERWRITE_REPLAY: P(0x8F); P(0xE3); P(0x8F); P(0x91); P(0x82); P(0xAB); P(0x82); P(0xB5); P(0x82); P(0xDC); P(0x82); P(0xB7); P(0x82); P(0xA9); break;
	case T2OW_YES: P(0x82); P(0xCD); P(0x82); P(0xA2); break;
	case T2OW_NO: P(0x82); P(0xA2); P(0x82); P(0xA2); P(0x82); P(0xA6); break;
	case T2OW_SLOT: P(0x94); P(0xD4); P(0x8D); P(0x86); break;
	case T2OW_NAME: P(0x96); P(0xBC); P(0x91); P(0x4F); break;
	case T2OW_NONE: P(0x82); P(0xC8); P(0x82); P(0xB5); break;
	case T2OW_INVALID: P(0x96); P(0xB3); P(0x8C); P(0xF8); break;
	case T2OW_CLEAR: P(0x83); P(0x4E); P(0x83); P(0x8A); P(0x83); P(0x41); break;
	case T2OW_GAME_OVER: P(0x83); P(0x51); P(0x81); P(0x5B); P(0x83); P(0x80); P(0x83); P(0x49); P(0x81); P(0x5B); P(0x83); P(0x6F); P(0x81); P(0x5B); break;
	case T2OW_MENU_RETURN: P(0x83); P(0x81); P(0x83); P(0x6A); P(0x83); P(0x85); P(0x81); P(0x5B); P(0x96); P(0xDF); P(0x82); P(0xE8); break;
	case T2OW_PAGE: P(0x83); P(0x79); P(0x81); P(0x5B); P(0x83); P(0x57); break;
	case T2OW_FINAL_SCORE: P(0x8D); P(0xC5); P(0x8F); P(0x49); P(0x83); P(0x58); P(0x83); P(0x52); P(0x83); P(0x41); break;
	case T2OW_START_POINT: P(0x8A); P(0x4A); P(0x8E); P(0x6E); P(0x92); P(0x6E); P(0x93); P(0x5F); break;
	case T2OW_STAGE_SPLITS: P(0x83); P(0x58); P(0x83); P(0x65); P(0x81); P(0x5B); P(0x83); P(0x57); P(0x95); P(0xCA); P(0x83); P(0x58); P(0x83); P(0x52); P(0x83); P(0x41); break;
	case T2OW_START_RUN: P(0x8A); P(0x4A); P(0x8E); P(0x6E); break;
	default: break;
	}
	#undef P
	return p;
}

static char *t2op_word_append(char *p, t2op_word_t word)
{
	if(!t2_language_english_ready()) {
		return t2op_word_append_japanese(p, word);
	}
	#define P(c) p = t2op_char(p, c)
	switch(word) {
	case T2OW_START: P('S'); P('t'); P('a'); P('r'); P('t'); break;
	case T2OW_EXTRA: P('E'); P('x'); P('t'); P('r'); P('a'); break;
	case T2OW_PRACTICE: P('P'); P('r'); P('a'); P('c'); P('t'); P('i'); P('c'); P('e'); break;
	case T2OW_REPLAY: P('R'); P('e'); P('p'); P('l'); P('a'); P('y'); break;
	case T2OW_HISCORE: P('H'); P('i'); P('S'); P('c'); P('o'); P('r'); P('e'); break;
	case T2OW_OPTIONS: P('O'); P('p'); P('t'); P('i'); P('o'); P('n'); P('s'); break;
	case T2OW_MUSIC_ROOM: P('M'); P('u'); P('s'); P('i'); P('c'); P(' '); P('R'); P('o'); P('o'); P('m'); break;
	case T2OW_QUIT: P('Q'); P('u'); P('i'); P('t'); break;
	case T2OW_RANK: P('R'); P('a'); P('n'); P('k'); break;
	case T2OW_EASY: P('E'); P('a'); P('s'); P('y'); break;
	case T2OW_NORMAL: P('N'); P('o'); P('r'); P('m'); P('a'); P('l'); break;
	case T2OW_HARD: P('H'); P('a'); P('r'); P('d'); break;
	case T2OW_LUNATIC: P('L'); P('u'); P('n'); P('a'); P('t'); P('i'); P('c'); break;
	case T2OW_SHOT: P('S'); P('h'); P('o'); P('t'); break;
	case T2OW_CHARACTER: P('C'); P('h'); P('a'); P('r'); P('a'); P('c'); P('t'); P('e'); P('r'); break;
	case T2OW_REIMU: P('R'); P('e'); P('i'); P('m'); P('u'); break;
	case T2OW_MARISA: P('M'); P('a'); P('r'); P('i'); P('s'); P('a'); break;
	case T2OW_MIMA: P('M'); P('i'); P('m'); P('a'); break;
	case T2OW_STAGE: P('S'); P('t'); P('a'); P('g'); P('e'); break;
	case T2OW_SECTION: P('S'); P('e'); P('c'); P('t'); P('i'); P('o'); P('n'); break;
	case T2OW_STAGE_START: P('S'); P('t'); P('a'); P('g'); P('e'); P(' '); P('S'); P('t'); P('a'); P('r'); P('t'); break;
	case T2OW_CHAPTER_2: P('C'); P('h'); P('a'); P('p'); P('t'); P('e'); P('r'); P(' '); P('2'); break;
	case T2OW_CHAPTER_3: P('C'); P('h'); P('a'); P('p'); P('t'); P('e'); P('r'); P(' '); P('3'); break;
	case T2OW_MIDBOSS: P('M'); P('i'); P('d'); P('b'); P('o'); P('s'); P('s'); break;
	case T2OW_BOSS_PHASE_1: P('B'); P('o'); P('s'); P('s'); P(' '); P('P'); P('h'); P('a'); P('s'); P('e'); P(' '); P('1'); break;
	case T2OW_BOSS_PHASE_2: P('B'); P('o'); P('s'); P('s'); P(' '); P('P'); P('h'); P('a'); P('s'); P('e'); P(' '); P('2'); break;
	case T2OW_BOSS_PHASE_3: P('B'); P('o'); P('s'); P('s'); P(' '); P('P'); P('h'); P('a'); P('s'); P('e'); P(' '); P('3'); break;
	case T2OW_BOSS_ROUND_2: P('B'); P('o'); P('s'); P('s'); P(' '); P('R'); P('o'); P('u'); P('n'); P('d'); P(' '); P('2'); break;
	case T2OW_BOSS_START: P('B'); P('o'); P('s'); P('s'); P(' '); P('S'); P('t'); P('a'); P('r'); P('t'); break;
	case T2OW_INNER_PAIR: P('I'); P('n'); P('n'); P('e'); P('r'); P(' '); P('P'); P('a'); P('i'); P('r'); break;
	case T2OW_OUTER_PAIR: P('O'); P('u'); P('t'); P('e'); P('r'); P(' '); P('P'); P('a'); P('i'); P('r'); break;
	case T2OW_SCORE: P('S'); P('c'); P('o'); P('r'); P('e'); break;
	case T2OW_HIGH_SCORE: P('H'); P('i'); P('g'); P('h'); P(' '); P('S'); P('c'); P('o'); P('r'); P('e'); break;
	case T2OW_POWER: P('P'); P('o'); P('w'); P('e'); P('r'); break;
	case T2OW_LIVES: P('L'); P('i'); P('v'); P('e'); P('s'); break;
	case T2OW_BOMBS: P('B'); P('o'); P('m'); P('b'); P('s'); break;
	case T2OW_SEED: P('S'); P('e'); P('e'); P('d'); break;
	case T2OW_SKILL: P('S'); P('k'); P('i'); P('l'); P('l'); break;
	case T2OW_BGM: P('B'); P('G'); P('M'); break;
	case T2OW_REDUCED_EFFECTS: P('R'); P('e'); P('d'); P('u'); P('c'); P('e'); P('d'); P(' '); P('E'); P('f'); P('f'); P('e'); P('c'); P('t'); P('s'); break;
	case T2OW_OFF: P('O'); P('f'); P('f'); break;
	case T2OW_ON: P('O'); P('n'); break;
	case T2OW_FM: P('F'); P('M'); break;
	case T2OW_MIDI: P('M'); P('I'); P('D'); P('I'); break;
	case T2OW_BROWSER: P('R'); P('e'); P('p'); P('l'); P('a'); P('y'); P(' '); P('B'); P('r'); P('o'); P('w'); P('s'); P('e'); P('r'); break;
	case T2OW_SAVE_REPLAY: P('S'); P('a'); P('v'); P('e'); P(' '); P('R'); P('e'); P('p'); P('l'); P('a'); P('y'); break;
	case T2OW_OVERWRITE_REPLAY: P('O'); P('v'); P('e'); P('r'); P('w'); P('r'); P('i'); P('t'); P('e'); P(' '); P('R'); P('e'); P('p'); P('l'); P('a'); P('y'); P('?'); break;
	case T2OW_YES: P('Y'); P('e'); P('s'); break;
	case T2OW_NO: P('N'); P('o'); break;
	case T2OW_SLOT: P('S'); P('l'); P('o'); P('t'); break;
	case T2OW_NAME: P('N'); P('a'); P('m'); P('e'); break;
	case T2OW_NONE: P('N'); P('o'); P('n'); P('e'); break;
	case T2OW_INVALID: P('I'); P('n'); P('v'); P('a'); P('l'); P('i'); P('d'); break;
	case T2OW_CLEAR: P('C'); P('l'); P('e'); P('a'); P('r'); break;
	case T2OW_GAME_OVER: P('G'); P('a'); P('m'); P('e'); P(' '); P('O'); P('v'); P('e'); P('r'); break;
	case T2OW_MENU_RETURN: P('M'); P('e'); P('n'); P('u'); P(' '); P('R'); P('e'); P('t'); P('u'); P('r'); P('n'); break;
	case T2OW_PAGE: P('P'); P('a'); P('g'); P('e'); break;
	case T2OW_FINAL_SCORE: P('F'); P('i'); P('n'); P('a'); P('l'); P(' '); P('S'); P('c'); P('o'); P('r'); P('e'); break;
	case T2OW_START_POINT: P('S'); P('t'); P('a'); P('r'); P('t'); P(' '); P('P'); P('o'); P('i'); P('n'); P('t'); break;
	case T2OW_STAGE_SPLITS: P('S'); P('t'); P('a'); P('g'); P('e'); P(' '); P('S'); P('p'); P('l'); P('i'); P('t'); P('s'); break;
	case T2OW_START_RUN: P('S'); P('t'); P('a'); P('r'); P('t'); P(' '); P('R'); P('u'); P('n'); break;
	default: break;
	}
	#undef P
	return p;
}

static char *t2op_spaces_append(char *p, unsigned count)
{
	while(count != 0) {
		*p++ = ' ';
		count--;
	}
	return p;
}

static char *t2op_u32_append(char *p, uint32_t value, unsigned width)
{
	char digits[10];
	unsigned count = 0;
	unsigned i;

	do {
		digits[count++] = static_cast<char>('0' + (value % 10UL));
		value /= 10UL;
	} while(value != 0);
	p = t2op_spaces_append(p, (width > count) ? (width - count) : 0);
	for(i = count; i != 0; i--) {
		*p++ = digits[i - 1];
	}
	return p;
}

static char *t2op_i32_append(char *p, int32_t value, unsigned width)
{
	uint32_t magnitude = static_cast<uint32_t>(value);
	bool negative = (value < 0);

	if(negative) {
		magnitude = (0UL - magnitude);
		*p++ = '-';
		if(width != 0) {
			width--;
		}
	}
	return t2op_u32_append(p, magnitude, width);
}

static void t2op_text_put(tram_x_t x, tram_y_t y, tram_atrb2 attr, char *end)
{
	*end = '\0';
	text_putsa(x, y, reinterpret_cast<const shiftjis_t *>(t2op_line), attr);
}

static uint8_t t2op_menu_gaiji(char c)
{
	if((c >= 'a') && (c <= 'z')) {
		c = static_cast<char>(c - ('a' - 'A'));
	}
	if((c >= '0') && (c <= '9')) {
		return static_cast<uint8_t>(gb_0 + (c - '0'));
	}
	if((c >= 'A') && (c <= 'Z')) {
		if(c == 'M') {
			return gb_M;
		}
		if(c == 'N') {
			return gb_N;
		}
		return static_cast<uint8_t>(gb_A + (c - 'A'));
	}
	if(c == '.') {
		return gs_PERIOD;
	}
	if(c == '!') {
		return gs_EXCLAMATION;
	}
	if(c == '?') {
		return gs_QUESTION;
	}
	return gs_SPACE;
}

static unsigned t2op_gaiji_encode(char *end)
{
	unsigned length = static_cast<unsigned>(end - t2op_line);
	unsigned i;

	for(i = 0; i < length; i++) {
		t2op_gaiji_line[i] = t2op_menu_gaiji(t2op_line[i]);
	}
	t2op_gaiji_line[length] = gs_NULL;
	return length;
}

static void t2op_gaiji_put(
	tram_x_t x, tram_y_t y, tram_atrb2 attr, char *end
)
{
	t2op_gaiji_encode(end);
	gaiji_putsa(
		x, y, reinterpret_cast<const char *>(t2op_gaiji_line), attr
	);
}

static void t2op_gaiji_center_put(
	tram_y_t y, tram_atrb2 attr, char *end
)
{
	unsigned length = t2op_gaiji_encode(end);
	tram_x_t x = static_cast<tram_x_t>(
		((RES_X / GLYPH_HALF_W) - (length * GAIJI_TRAM_W)) / 2
	);

	gaiji_putsa(
		x, y, reinterpret_cast<const char *>(t2op_gaiji_line), attr
	);
}

static void t2op_title_gaiji_put(
	tram_x_t x, tram_y_t y, tram_atrb2 attr, char *end
)
{
	t2op_gaiji_encode(end);
	graph_gaiji_puts(
		static_cast<screen_x_t>((x * GLYPH_HALF_W) + 4),
		static_cast<screen_y_t>((y * GLYPH_H) + 4),
		GAIJI_W,
		reinterpret_cast<const char *>(t2op_gaiji_line), 0
	);
	gaiji_putsa(x, y, reinterpret_cast<const char *>(t2op_gaiji_line), attr);
}

static void t2op_title_gaiji_center_put(
	tram_y_t y, tram_atrb2 attr, char *end
)
{
	unsigned length = t2op_gaiji_encode(end);
	tram_x_t x = static_cast<tram_x_t>(
		((RES_X / GLYPH_HALF_W) - (length * GAIJI_TRAM_W)) / 2
	);

	t2op_title_gaiji_put(x, y, attr, end);
}

static bool t2op_shiftjis_lead(char c)
{
	uint8_t byte = static_cast<uint8_t>(c);

	return (
		((byte >= 0x81) && (byte <= 0x9F)) ||
		((byte >= 0xE0) && (byte <= 0xFC))
	);
}

static unsigned t2op_line_tram_width(char *end)
{
	char *p = t2op_line;
	unsigned width = 0;

	while(p < end) {
		if(t2op_shiftjis_lead(*p) && ((p + 1) < end)) {
			p += 2;
			width += 2;
		} else {
			p++;
			width++;
		}
	}
	return width;
}

static void t2op_title_text_put(
	tram_x_t x, tram_y_t y, tram_atrb2 attr, char *end
)
{
	*end = '\0';
	graph_putsa_fx(
		static_cast<screen_x_t>((x * GLYPH_HALF_W) + 4),
		static_cast<screen_y_t>((y * GLYPH_H) + 4),
		FX_WEIGHT_BOLD,
		reinterpret_cast<const shiftjis_t *>(t2op_line)
	);
	text_putsa(x, y, reinterpret_cast<const shiftjis_t *>(t2op_line), attr);
}

static void t2op_title_word_put(
	tram_x_t x, tram_y_t y, tram_atrb2 attr, char *end
)
{
	if(t2_language_english_ready()) {
		t2op_title_gaiji_put(x, y, attr, end);
	} else {
		t2op_title_text_put(x, y, attr, end);
	}
}

static void t2op_title_word_center_put(
	tram_y_t y, tram_atrb2 attr, char *end
)
{
	tram_x_t x;

	if(t2_language_english_ready()) {
		t2op_title_gaiji_center_put(y, attr, end);
		return;
	}
	x = static_cast<tram_x_t>(
		((RES_X / GLYPH_HALF_W) - t2op_line_tram_width(end)) / 2
	);
	t2op_title_text_put(x, y, attr, end);
}

static void t2op_title_word_right_put(
	tram_x_t right, tram_y_t y, tram_atrb2 attr, char *end
)
{
	tram_x_t x;

	if(t2_language_english_ready()) {
		x = static_cast<tram_x_t>(
			right - (t2op_gaiji_encode(end) * GAIJI_TRAM_W)
		);
		t2op_title_gaiji_put(x, y, attr, end);
		return;
	}
	x = static_cast<tram_x_t>(right - t2op_line_tram_width(end));
	t2op_title_text_put(x, y, attr, end);
}

static unsigned t2op_gaiji_length(const char *text)
{
	unsigned length = 0;

	while(text[length] != '\0') {
		length++;
	}
	return length;
}

static void t2op_title_gaiji_raw_put(
	tram_x_t x, tram_y_t y, tram_atrb2 attr, const char *text
)
{
	graph_gaiji_puts(
		static_cast<screen_x_t>((x * GLYPH_HALF_W) + 4),
		static_cast<screen_y_t>((y * GLYPH_H) + 4), GAIJI_W, text, 0
	);
	gaiji_putsa(x, y, text, attr);
}

static void t2op_title_gaiji_raw_center_put(
	tram_y_t y, tram_atrb2 attr, const char *text
)
{
	unsigned length = t2op_gaiji_length(text);
	tram_x_t x = static_cast<tram_x_t>(
		((RES_X / GLYPH_HALF_W) - (length * GAIJI_TRAM_W)) / 2
	);

	t2op_title_gaiji_raw_put(x, y, attr, text);
}

static void t2op_title_font_restore(void)
{
	gaiji_restore();
	t2_language_gaiji_entry_bfnt("MIKOFT.bft");
}

// The title-surface handoffs owned here return through OP's normal title
// rebuild rather than relying on whichever graphics page or gaiji table the
// preceding native menu happened to leave behind.
static void t2op_title_return_request(void)
{
	// Replay and Practice leave OP through the same normal title rebuild as
	// stock menus. Keep title input locked until that rebuild has completed.
	replay_title_restore_needed = true;
	t2op_main_input_allowed = false;
}

static void t2op_input_wait_release(void)
{
	do {
		input_reset_sense();
		if(key_det != INPUT_NONE) {
			frame_delay(1);
		}
	} while(key_det != INPUT_NONE);
}

#define T2OP_NAME_ALPHABET_ROWS 3
#define T2OP_NAME_ALPHABET_COLS 17
#define T2OP_NAME_ALPHABET_LEFT 10
#define T2OP_NAME_ALPHABET_TOP 18
#define T2OP_NAME_FIELD_LEFT 32
#define T2OP_NAME_FIELD_Y 6
#define T2OP_NAME_CELL_LEFT 48
#define T2OP_NAME_CELL_RIGHT 49
#define T2OP_NAME_CELL_END 50

// This is the native high-score alphabet's exact row-major cell vocabulary.
// TH02's MIKOFT.BFT swaps the physical M/N cels, so the two letters must use
// their named gaiji constants rather than an unqualified alphabetic offset.
static uint8_t t2op_name_keyboard_glyph(uint8_t cell)
{
	if(cell < 12) {
		return static_cast<uint8_t>(gb_A + cell);
	}
	if(cell == 12) {
		return gb_M;
	}
	if(cell == 13) {
		return gb_N;
	}
	if(cell < 17) {
		return static_cast<uint8_t>(gb_O + (cell - 14));
	}
	if(cell < 26) {
		return static_cast<uint8_t>(gb_R + (cell - 17));
	}
	if(cell < 31) {
		return static_cast<uint8_t>(gs_BULLET + (cell - 26));
	}
	if(cell == 31) {
		return gs_HEART;
	}
	if(cell == 32) {
		return gs_YINYANG;
	}
	if(cell == 33) {
		return gs_BOMB;
	}
	if(cell < 44) {
		return static_cast<uint8_t>(gb_0 + (cell - 34));
	}
	if(cell == 44) {
		return gs_SKULL;
	}
	if(cell == 45) {
		return gs_GHOST;
	}
	if(cell == 46) {
		return gs_SIDDHAM_HAM;
	}
	if(cell == 47) {
		return gs_SPACE;
	}
	if(cell == T2OP_NAME_CELL_LEFT) {
		return gs_ARROW_LEFT;
	}
	if(cell == T2OP_NAME_CELL_RIGHT) {
		return gs_ARROW_RIGHT;
	}
	return gs_END;
}

static bool t2op_name_empty(const uint8_t far *name)
{
	unsigned i;

	for(i = 0; i < T2REPLAY_NAME_LEN; i++) {
		if((name[i] != 0) && (name[i] != gs_SPACE)) {
			return false;
		}
	}
	return true;
}

static void t2op_name_put(
	tram_x_t left, tram_y_t y, const uint8_t far *name, tram_atrb2 attr
)
{
	unsigned i;
	uint8_t glyph;

	for(i = 0; i < T2REPLAY_NAME_LEN; i++) {
		glyph = (name[i] == 0) ? gs_SPACE : name[i];
		gaiji_putca(
			(left + (i * GAIJI_TRAM_W)), y, glyph, attr
		);
	}
}

static void t2op_name_keyboard_cell_put(
	uint8_t col, uint8_t row, tram_atrb2 attr
)
{
	uint8_t cell = static_cast<uint8_t>(
		(row * T2OP_NAME_ALPHABET_COLS) + col
	);

	gaiji_putca(
		(T2OP_NAME_ALPHABET_LEFT + (col * GAIJI_TRAM_W)),
		(T2OP_NAME_ALPHABET_TOP + row),
		t2op_name_keyboard_glyph(cell), attr
	);
}

static void t2op_name_keyboard_put(uint8_t selected_col, uint8_t selected_row)
{
	uint8_t row;
	uint8_t col;

	for(row = 0; row < T2OP_NAME_ALPHABET_ROWS; row++) {
		for(col = 0; col < T2OP_NAME_ALPHABET_COLS; col++) {
			t2op_name_keyboard_cell_put(col, row, TX_WHITE);
		}
	}
	t2op_name_keyboard_cell_put(
		selected_col, selected_row, (TX_GREEN | TX_REVERSE)
	);
}

static void t2op_name_menu_render(const uint8_t far *name)
{
	char *p;

	text_clear();
	if(t2_language_english_ready()) {
		gaiji_putca(36, 4, gb_N, TX_GREEN);
		gaiji_putca(38, 4, gb_A, TX_GREEN);
		gaiji_putca(40, 4, gb_M, TX_GREEN);
		gaiji_putca(42, 4, gb_E, TX_GREEN);
	} else {
		p = t2op_word_append(t2op_line, T2OW_NAME);
		t2op_text_put(38, 4, TX_GREEN, p);
	}
	t2op_name_put(T2OP_NAME_FIELD_LEFT, T2OP_NAME_FIELD_Y, name, TX_WHITE);
}

static bool t2op_name_menu(uint8_t far *name)
{
	uint8_t col = 0;
	uint8_t row = 0;
	uint8_t cursor = 0;
	uint8_t cell;
	bool input_locked = true;
	uint8_t input_delay = 0;
	unsigned i;

	for(i = 0; i < T2REPLAY_NAME_LEN; i++) {
		name[i] = gs_SPACE;
	}
	t2op_name_menu_render(name);
	t2op_name_keyboard_put(col, row);
	while(1) {
		input_reset_sense();
		if(!input_locked) {
			if(key_det & INPUT_UP) {
				t2op_name_keyboard_cell_put(col, row, TX_WHITE);
				row = ((row == 0) ? (T2OP_NAME_ALPHABET_ROWS - 1) : (row - 1));
				t2op_name_keyboard_cell_put(col, row, (TX_GREEN | TX_REVERSE));
			}
			if(key_det & INPUT_DOWN) {
				t2op_name_keyboard_cell_put(col, row, TX_WHITE);
				row = ((row == (T2OP_NAME_ALPHABET_ROWS - 1)) ? 0 : (row + 1));
				t2op_name_keyboard_cell_put(col, row, (TX_GREEN | TX_REVERSE));
			}
			if(key_det & INPUT_LEFT) {
				t2op_name_keyboard_cell_put(col, row, TX_WHITE);
				col = ((col == 0) ? (T2OP_NAME_ALPHABET_COLS - 1) : (col - 1));
				t2op_name_keyboard_cell_put(col, row, (TX_GREEN | TX_REVERSE));
			}
			if(key_det & INPUT_RIGHT) {
				t2op_name_keyboard_cell_put(col, row, TX_WHITE);
				col = ((col == (T2OP_NAME_ALPHABET_COLS - 1)) ? 0 : (col + 1));
				t2op_name_keyboard_cell_put(col, row, (TX_GREEN | TX_REVERSE));
			}
			if(key_det & INPUT_BOMB) {
				name[cursor] = gs_SPACE;
				if(cursor != 0) {
					cursor--;
				}
				t2op_name_put(
					T2OP_NAME_FIELD_LEFT, T2OP_NAME_FIELD_Y, name, TX_WHITE
				);
			}
			if(key_det & INPUT_CANCEL) {
				return false;
			}
			if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
				cell = static_cast<uint8_t>(
					(row * T2OP_NAME_ALPHABET_COLS) + col
				);
				if(cell == T2OP_NAME_CELL_END) {
					return true;
				}
				if(cell == T2OP_NAME_CELL_LEFT) {
					if(cursor != 0) {
						cursor--;
					}
					name[cursor] = gs_SPACE;
				} else if(cell == T2OP_NAME_CELL_RIGHT) {
					if(cursor < (T2REPLAY_NAME_LEN - 1)) {
						cursor++;
					}
				} else {
					name[cursor] = t2op_name_keyboard_glyph(cell);
					if(cursor < (T2REPLAY_NAME_LEN - 1)) {
						cursor++;
					} else {
						t2op_name_keyboard_cell_put(col, row, TX_WHITE);
						col = (T2OP_NAME_ALPHABET_COLS - 1);
						row = (T2OP_NAME_ALPHABET_ROWS - 1);
						t2op_name_keyboard_cell_put(
							col, row, (TX_GREEN | TX_REVERSE)
						);
					}
				}
				t2op_name_put(
					T2OP_NAME_FIELD_LEFT, T2OP_NAME_FIELD_Y, name, TX_WHITE
				);
			}
		}
		frame_delay(1);
		input_locked = (key_det != INPUT_NONE);
		if(input_locked) {
			input_delay++;
			if((input_delay > 30) && ((input_delay & 1) == 0)) {
				input_locked = false;
			}
		} else {
			input_delay = 0;
		}
	}
}

static char *t2op_rank_append(char *p, uint8_t value)
{
	switch(value) {
	case RANK_EASY: return t2op_word_append(p, T2OW_EASY);
	case RANK_NORMAL: return t2op_word_append(p, T2OW_NORMAL);
	case RANK_HARD: return t2op_word_append(p, T2OW_HARD);
	case RANK_LUNATIC: return t2op_word_append(p, T2OW_LUNATIC);
	default: return t2op_word_append(p, T2OW_EXTRA);
	}
}

static char *t2op_character_append(char *p, uint8_t value)
{
	switch(value) {
	case 0: return t2op_word_append(p, T2OW_REIMU);
	case 1: return t2op_word_append(p, T2OW_MARISA);
	default: return t2op_word_append(p, T2OW_MIMA);
	}
}

static char *t2op_end_reason_append(char *p, uint8_t value)
{
	if(value == T2REPLAY_END_CLEAR) {
		return t2op_word_append(p, T2OW_CLEAR);
	} else if(value == T2REPLAY_END_MENU_RETURN) {
		return t2op_word_append(p, T2OW_MENU_RETURN);
	}
	return t2op_word_append(p, T2OW_GAME_OVER);
}

static char *t2op_stage_append(char *p, int8_t stage)
{
	if(stage == (T2REPLAY_STAGE_COUNT - 1)) {
		return t2op_word_append(p, T2OW_EXTRA);
	}
	p = t2op_word_append(p, T2OW_STAGE);
	p = t2op_char(p, ' ');
	return t2op_char(p, static_cast<char>('1' + stage));
}

static char *t2op_bgm_append(char *p, uint8_t value)
{
	if(value == SND_BGM_OFF) {
		return t2op_word_append(p, T2OW_OFF);
	} else if(value == SND_BGM_FM) {
		return t2op_word_append(p, T2OW_FM);
	}
	return t2op_word_append(p, T2OW_MIDI);
}

void replay_title_background_restore(void)
{
	// OP's page 1 is the native title-background source.  Reblit it before
	// rebuilding patch text so leaving a browser never exposes stale graphics.
	palette_settone(0);
	text_clear();
	graph_showpage(0);
	graph_accesspage(1);
	pi_load_put_8_free_to(MENU_MAIN_BG_FN, 1);
	palette_entry_rgb_show(MENU_MAIN_PALETTE_FN);
	graph_copy_page(0);
	graph_accesspage(0);
	t2op_title_font_restore();
	palette_100();
}

static void t2op_main_line_put(
	tram_y_t y, bool selected, t2op_word_t label, bool locked
)
{
	char *p = t2op_line;
	const char *native_label = 0;
	tram_atrb2 attr = (selected ? TX_WHITE : TX_YELLOW);

	if(locked) {
		attr = TX_BLUE;
	}
	switch(label) {
	case T2OW_START: native_label = gbSTART; break;
	case T2OW_EXTRA: native_label = gbEXTRA_START; break;
	case T2OW_HISCORE: native_label = gbHISCORE; break;
	case T2OW_OPTIONS: native_label = gbOPTION; break;
	case T2OW_MUSIC_ROOM: native_label = gbMUSIC_MODE; break;
	case T2OW_QUIT: native_label = gbQUIT; break;
	default: break;
	}
	if(native_label) {
		t2op_title_gaiji_raw_center_put(y, attr, native_label);
		return;
	}
	p = t2op_word_append(p, label);
	t2op_title_word_center_put(y, attr, p);
}

static void t2op_main_render(void)
{
	uint8_t row;

	if(replay_title_restore_needed) {
		replay_title_background_restore();
		replay_title_restore_needed = false;
	} else {
		text_clear();
	}
	for(row = 0; row < T2OMC_COUNT; row++) {
		t2op_word_t label;
		bool locked = false;

		switch(row) {
		case T2OMC_START: label = T2OW_START; break;
		case T2OMC_EXTRA: label = T2OW_EXTRA; locked = !extra_unlocked; break;
		case T2OMC_PRACTICE: label = T2OW_PRACTICE; break;
		case T2OMC_REPLAY: label = T2OW_REPLAY; break;
		case T2OMC_HISCORE: label = T2OW_HISCORE; break;
		case T2OMC_OPTIONS: label = T2OW_OPTIONS; break;
		case T2OMC_MUSIC: label = T2OW_MUSIC_ROOM; break;
		default: label = T2OW_QUIT; break;
		}
		t2op_main_line_put(
			static_cast<tram_y_t>(
				(row == T2OMC_QUIT) ? T2OP_TITLE_QUIT_ROW :
				(T2OP_TITLE_COMMAND_FIRST_ROW + row)
			),
			(t2op_main_sel == row), label, locked
		);
	}
	t2op_title_gaiji_raw_put(26, T2OP_TITLE_RANK_ROW, TX_GREEN, gbRANK);
	t2op_title_gaiji_raw_put(38, T2OP_TITLE_RANK_ROW, TX_GREEN, gbcRANKS[rank]);
}

static void t2op_main_selection_step(int8_t direction)
{
	do {
		if(direction < 0) {
			t2op_main_sel = ((t2op_main_sel == 0)
				? (T2OMC_COUNT - 1)
				: (t2op_main_sel - 1)
			);
		} else {
			t2op_main_sel = ((t2op_main_sel == (T2OMC_COUNT - 1))
				? 0
				: (t2op_main_sel + 1)
			);
		}
	} while(!extra_unlocked && (t2op_main_sel == T2OMC_EXTRA));
}

static void t2op_practice_defaults(void)
{
	t2op_memclear(&t2op_practice, sizeof(t2op_practice));
	t2op_practice.resident_frame = static_cast<uint32_t>(resident->frame);
	t2op_practice.random_seed = t2op_practice.resident_frame;
	t2op_practice.score = 0;
	t2op_practice.score_highest = 0;
	t2op_practice.continues_used = 0;
	t2op_practice.skill = resident->skill;
	if(t2op_practice.skill < 0) {
		t2op_practice.skill = 0;
	} else if(t2op_practice.skill > 100) {
		t2op_practice.skill = 100;
	}
	t2op_practice.stage = 0;
	t2op_practice.rank = rank;
	if(t2op_practice.rank == RANK_EXTRA) {
		t2op_practice.rank = RANK_NORMAL;
	}
	t2op_practice.rem_lives = lives;
	t2op_practice.rem_bombs = bombs;
	t2op_practice.start_lives = lives;
	t2op_practice.start_bombs = bombs;
	t2op_practice.start_power = 1;
	t2op_practice.shottype = resident->shottype;
	t2op_practice.bgm_mode = snd_bgm_mode;
	t2op_practice.reduce_effects = (resident->reduce_effects ? 1 : 0);
}

static void t2op_practice_stage_set(int8_t stage)
{
	t2op_practice.stage = stage;
	t2op_practice.reserved[T2REPLAY_PRACTICE_TARGET_OFFSET] =
		T2RPT_STAGE_START;
	if(stage == (T2REPLAY_STAGE_COUNT - 1)) {
		t2op_practice.rank = RANK_EXTRA;
	} else if(t2op_practice.rank == RANK_EXTRA) {
		t2op_practice.rank = rank;
	}
}

static uint8_t t2op_practice_target_step(
	int8_t stage, uint8_t target, int8_t direction
)
{
	switch(stage) {
	case 0:
		if(direction < 0) {
			switch(target) {
			case T2RPT_STAGE_START: return T2RPT_STAGE1_BOSS_PHASE3;
			case T2RPT_STAGE1_MIDBOSS: return T2RPT_STAGE_START;
			case T2RPT_STAGE1_CHAPTER2: return T2RPT_STAGE1_MIDBOSS;
			case T2RPT_STAGE1_BOSS_PHASE1: return T2RPT_STAGE1_CHAPTER2;
			case T2RPT_STAGE1_BOSS_PHASE2: return T2RPT_STAGE1_BOSS_PHASE1;
			default: return T2RPT_STAGE1_BOSS_PHASE2;
			}
		}
		switch(target) {
		case T2RPT_STAGE_START: return T2RPT_STAGE1_MIDBOSS;
		case T2RPT_STAGE1_MIDBOSS: return T2RPT_STAGE1_CHAPTER2;
		case T2RPT_STAGE1_CHAPTER2: return T2RPT_STAGE1_BOSS_PHASE1;
		case T2RPT_STAGE1_BOSS_PHASE1: return T2RPT_STAGE1_BOSS_PHASE2;
		case T2RPT_STAGE1_BOSS_PHASE2: return T2RPT_STAGE1_BOSS_PHASE3;
		default: return T2RPT_STAGE_START;
		}
	case 1:
		if(direction < 0) {
			switch(target) {
			case T2RPT_STAGE_START: return T2RPT_STAGE2_BOSS_PHASE3;
			case T2RPT_STAGE2_MIDBOSS: return T2RPT_STAGE_START;
			case T2RPT_STAGE2_CHAPTER2: return T2RPT_STAGE2_MIDBOSS;
			case T2RPT_STAGE2_BOSS_PHASE1: return T2RPT_STAGE2_CHAPTER2;
			case T2RPT_STAGE2_BOSS_PHASE2: return T2RPT_STAGE2_BOSS_PHASE1;
			default: return T2RPT_STAGE2_BOSS_PHASE2;
			}
		}
		switch(target) {
		case T2RPT_STAGE_START: return T2RPT_STAGE2_MIDBOSS;
		case T2RPT_STAGE2_MIDBOSS: return T2RPT_STAGE2_CHAPTER2;
		case T2RPT_STAGE2_CHAPTER2: return T2RPT_STAGE2_BOSS_PHASE1;
		case T2RPT_STAGE2_BOSS_PHASE1: return T2RPT_STAGE2_BOSS_PHASE2;
		case T2RPT_STAGE2_BOSS_PHASE2: return T2RPT_STAGE2_BOSS_PHASE3;
		default: return T2RPT_STAGE_START;
		}
	case 2:
		if(direction < 0) {
			switch(target) {
			case T2RPT_STAGE_START: return T2RPT_STAGE3_OUTER_PAIR;
			case T2RPT_STAGE3_MIDBOSS: return T2RPT_STAGE_START;
			case T2RPT_STAGE3_CHAPTER2: return T2RPT_STAGE3_MIDBOSS;
			case T2RPT_STAGE3_BOSS_START: return T2RPT_STAGE3_CHAPTER2;
			case T2RPT_STAGE3_INNER_PAIR: return T2RPT_STAGE3_BOSS_START;
			default: return T2RPT_STAGE3_INNER_PAIR;
			}
		}
		switch(target) {
		case T2RPT_STAGE_START: return T2RPT_STAGE3_MIDBOSS;
		case T2RPT_STAGE3_MIDBOSS: return T2RPT_STAGE3_CHAPTER2;
		case T2RPT_STAGE3_CHAPTER2: return T2RPT_STAGE3_BOSS_START;
		case T2RPT_STAGE3_BOSS_START: return T2RPT_STAGE3_INNER_PAIR;
		case T2RPT_STAGE3_INNER_PAIR: return T2RPT_STAGE3_OUTER_PAIR;
		default: return T2RPT_STAGE_START;
		}
	case 3:
		if(direction < 0) {
			if(target == T2RPT_STAGE_START) {
				return T2RPT_STAGE4_BOSS_ROUND2;
			}
			if(target == T2RPT_STAGE4_BOSS_ROUND2) {
				return T2RPT_STAGE4_BOSS_PHASE1;
			}
			switch(target) {
			case T2RPT_STAGE_START: return T2RPT_STAGE4_BOSS_PHASE1;
			case T2RPT_STAGE4_MIDBOSS_FIRST: return T2RPT_STAGE_START;
			case T2RPT_STAGE4_CHAPTER2: return T2RPT_STAGE4_MIDBOSS_FIRST;
			case T2RPT_STAGE4_MIDBOSS_SECOND: return T2RPT_STAGE4_CHAPTER2;
			case T2RPT_STAGE4_CHAPTER3: return T2RPT_STAGE4_MIDBOSS_SECOND;
			case T2RPT_STAGE4_BOSS_START: return T2RPT_STAGE4_CHAPTER3;
			default: return T2RPT_STAGE4_BOSS_START;
			}
		}
		if(target == T2RPT_STAGE4_BOSS_PHASE1) {
			return T2RPT_STAGE4_BOSS_ROUND2;
		}
		switch(target) {
		case T2RPT_STAGE_START: return T2RPT_STAGE4_MIDBOSS_FIRST;
		case T2RPT_STAGE4_MIDBOSS_FIRST: return T2RPT_STAGE4_CHAPTER2;
		case T2RPT_STAGE4_CHAPTER2: return T2RPT_STAGE4_MIDBOSS_SECOND;
		case T2RPT_STAGE4_MIDBOSS_SECOND: return T2RPT_STAGE4_CHAPTER3;
		case T2RPT_STAGE4_CHAPTER3: return T2RPT_STAGE4_BOSS_START;
		case T2RPT_STAGE4_BOSS_START: return T2RPT_STAGE4_BOSS_PHASE1;
		default: return T2RPT_STAGE_START;
		}
	case 4:
		if(direction < 0) {
			if(target == T2RPT_STAGE_START) {
				return T2RPT_STAGE5_BOSS_PHASE3;
			}
			if(target == T2RPT_STAGE5_BOSS_PHASE3) {
				return T2RPT_STAGE5_BOSS_PHASE1;
			}
			switch(target) {
			case T2RPT_STAGE_START: return T2RPT_STAGE5_BOSS_PHASE1;
			case T2RPT_STAGE5_BOSS_START: return T2RPT_STAGE_START;
			default: return T2RPT_STAGE5_BOSS_START;
			}
		}
		if(target == T2RPT_STAGE5_BOSS_PHASE1) {
			return T2RPT_STAGE5_BOSS_PHASE3;
		}
		if(target == T2RPT_STAGE_START) {
			return T2RPT_STAGE5_BOSS_START;
		}
		return ((target == T2RPT_STAGE5_BOSS_START)
			? T2RPT_STAGE5_BOSS_PHASE1 : T2RPT_STAGE_START);
	case 5:
		if(direction < 0) {
			if(target == T2RPT_STAGE_START) {
				return T2RPT_EXTRA_BOSS_PHASE3;
			}
			if(target == T2RPT_EXTRA_BOSS_PHASE3) {
				return T2RPT_EXTRA_BOSS_PHASE1;
			}
			switch(target) {
			case T2RPT_STAGE_START: return T2RPT_EXTRA_BOSS_PHASE1;
			case T2RPT_EXTRA_MIDBOSS: return T2RPT_STAGE_START;
			case T2RPT_EXTRA_CHAPTER2: return T2RPT_EXTRA_MIDBOSS;
			case T2RPT_EXTRA_BOSS_START: return T2RPT_EXTRA_CHAPTER2;
			default: return T2RPT_EXTRA_BOSS_START;
			}
		}
		if(target == T2RPT_EXTRA_BOSS_PHASE1) {
			return T2RPT_EXTRA_BOSS_PHASE3;
		}
		switch(target) {
		case T2RPT_STAGE_START: return T2RPT_EXTRA_MIDBOSS;
		case T2RPT_EXTRA_MIDBOSS: return T2RPT_EXTRA_CHAPTER2;
		case T2RPT_EXTRA_CHAPTER2: return T2RPT_EXTRA_BOSS_START;
		case T2RPT_EXTRA_BOSS_START: return T2RPT_EXTRA_BOSS_PHASE1;
		default: return T2RPT_STAGE_START;
		}
	default:
		return T2RPT_STAGE_START;
	}
}

static void t2op_practice_u8_change(
	uint8_t *value, uint8_t min, uint8_t max, uint8_t delta, bool right
)
{
	if(right) {
		*value = ((*value > (max - delta))
			? min
			: static_cast<uint8_t>(*value + delta)
		);
	} else {
		*value = ((*value < (min + delta))
			? max
			: static_cast<uint8_t>(*value - delta)
		);
	}
}

static void t2op_practice_i16_change(
	int16_t *value, int16_t min, int16_t max, int16_t delta, bool right
)
{
	if(right) {
		*value = ((*value > (max - delta))
			? min
			: static_cast<int16_t>(*value + delta)
		);
	} else {
		*value = ((*value < (min + delta))
			? max
			: static_cast<int16_t>(*value - delta)
		);
	}
}

static void t2op_practice_u32_change(
	uint32_t *value, uint32_t min, uint32_t max, uint32_t delta,
	bool right
)
{
	if(right) {
		if(*value == max) {
			*value = min;
		} else if(*value > (max - delta)) {
			*value = max;
		} else {
			*value += delta;
		}
	} else {
		if(*value == min) {
			*value = max;
		} else if(*value < (min + delta)) {
			*value = min;
		} else {
			*value -= delta;
		}
	}
}

static void t2op_practice_score_change(uint32_t delta, bool right)
{
	uint32_t value = static_cast<uint32_t>(t2op_practice.score);
	const uint32_t max = 2147483647UL;

	t2op_practice_u32_change(&value, 0, max, delta, right);
	t2op_practice.score = static_cast<int32_t>(value);
	if(t2op_practice.score_highest < value) {
		t2op_practice.score_highest = value;
	}
}

static bool t2op_shift_pressed(void)
{
	_AH = 2;
	geninterrupt(0x18);
	return ((_AL & 1) != 0);
}

static void t2op_practice_value_step(int8_t direction, bool fast)
{
	uint32_t seed;
	uint32_t score_highest;
	uint8_t practice_target;
	bool right = (direction > 0);

	switch(t2op_practice_sel) {
	case T2OPC_STAGE:
		if(direction < 0) {
			t2op_practice_stage_set((t2op_practice.stage == 0)
				? (T2REPLAY_STAGE_COUNT - 1)
				: (t2op_practice.stage - 1));
		} else {
			t2op_practice_stage_set(
				(t2op_practice.stage == (T2REPLAY_STAGE_COUNT - 1))
					? 0 : (t2op_practice.stage + 1)
			);
		}
		break;
	case T2OPC_SECTION:
		practice_target = t2op_practice.reserved[
			T2REPLAY_PRACTICE_TARGET_OFFSET
		];
		practice_target = t2op_practice_target_step(
			t2op_practice.stage, practice_target, direction
		);
		t2op_practice.reserved[T2REPLAY_PRACTICE_TARGET_OFFSET] =
			practice_target;
		break;
	case T2OPC_SCORE:
		t2op_practice_score_change((fast ? 1000000UL : 1000UL), right);
		break;
	case T2OPC_HIGH_SCORE:
		score_highest = t2op_practice.score_highest;
		t2op_practice_u32_change(
			&score_highest, static_cast<uint32_t>(t2op_practice.score),
			0xFFFFFFFFUL, (fast ? 1000000UL : 1000UL), right
		);
		t2op_practice.score_highest = score_highest;
		break;
	case T2OPC_POWER:
		t2op_practice_u8_change(
			reinterpret_cast<uint8_t *>(&t2op_practice.start_power),
			1, 80, (fast ? 16 : 1), right
		);
		break;
	case T2OPC_LIVES:
		t2op_practice_u8_change(
			&t2op_practice.start_lives, 1, 5, 1, right
		);
		t2op_practice.rem_lives = t2op_practice.start_lives;
		break;
	case T2OPC_BOMBS:
		t2op_practice_u8_change(
			&t2op_practice.start_bombs, 1, 5, 1, right
		);
		t2op_practice.rem_bombs = t2op_practice.start_bombs;
		break;
	case T2OPC_SEED:
		seed = t2op_practice.resident_frame;
		t2op_practice_u32_change(
			&seed, 0, 0xFFFFFFFFUL, (fast ? 256UL : 1UL), right
		);
		t2op_practice.resident_frame = seed;
		t2op_practice.random_seed = seed;
		break;
	case T2OPC_SKILL:
		t2op_practice_i16_change(
			&t2op_practice.skill, 0, 100, (fast ? 10 : 1), right
		);
		break;
	case T2OPC_BGM:
		if(direction < 0) {
			t2op_practice.bgm_mode = ((t2op_practice.bgm_mode == SND_BGM_OFF)
				? SND_BGM_MIDI : (t2op_practice.bgm_mode - 1));
		} else {
			t2op_practice.bgm_mode = ((t2op_practice.bgm_mode == SND_BGM_MIDI)
				? SND_BGM_OFF : (t2op_practice.bgm_mode + 1));
		}
		break;
	case T2OPC_EFFECTS:
		t2op_practice.reduce_effects = (1 - t2op_practice.reduce_effects);
		break;
	default:
		break;
	}
}

static bool t2op_practice_field_is_numeric(uint8_t field)
{
	return (
		(field >= T2OPC_SCORE) &&
		(field <= T2OPC_SKILL)
	);
}

static uint32_t t2op_practice_numeric_get(uint8_t field)
{
	switch(field) {
	case T2OPC_SCORE: return static_cast<uint32_t>(t2op_practice.score);
	case T2OPC_HIGH_SCORE: return t2op_practice.score_highest;
	case T2OPC_POWER: return t2op_practice.start_power;
	case T2OPC_LIVES: return t2op_practice.start_lives;
	case T2OPC_BOMBS: return t2op_practice.start_bombs;
	case T2OPC_SEED: return t2op_practice.resident_frame;
	case T2OPC_SKILL: return static_cast<uint32_t>(t2op_practice.skill);
	default: return 0;
	}
}

static uint32_t t2op_practice_numeric_min(uint8_t field)
{
	switch(field) {
	case T2OPC_HIGH_SCORE:
		return static_cast<uint32_t>(t2op_practice.score);
	case T2OPC_POWER:
	case T2OPC_LIVES:
	case T2OPC_BOMBS:
		return 1;
	default:
		return 0;
	}
}

static uint32_t t2op_practice_numeric_max(uint8_t field)
{
	switch(field) {
	case T2OPC_SCORE: return 2147483647UL;
	case T2OPC_HIGH_SCORE:
	case T2OPC_SEED: return 0xFFFFFFFFUL;
	case T2OPC_POWER: return 80;
	case T2OPC_LIVES:
	case T2OPC_BOMBS: return 5;
	case T2OPC_SKILL: return 100;
	default: return 0;
	}
}

static void t2op_practice_numeric_set(uint8_t field, uint32_t value)
{
	switch(field) {
	case T2OPC_SCORE:
		t2op_practice.score = static_cast<int32_t>(value);
		if(t2op_practice.score_highest < value) {
			t2op_practice.score_highest = value;
		}
		break;
	case T2OPC_HIGH_SCORE:
		t2op_practice.score_highest = value;
		break;
	case T2OPC_POWER:
		t2op_practice.start_power = static_cast<int8_t>(value);
		break;
	case T2OPC_LIVES:
		t2op_practice.start_lives = static_cast<uint8_t>(value);
		t2op_practice.rem_lives = t2op_practice.start_lives;
		break;
	case T2OPC_BOMBS:
		t2op_practice.start_bombs = static_cast<uint8_t>(value);
		t2op_practice.rem_bombs = t2op_practice.start_bombs;
		break;
	case T2OPC_SEED:
		t2op_practice.resident_frame = value;
		t2op_practice.random_seed = value;
		break;
	case T2OPC_SKILL:
		t2op_practice.skill = static_cast<int16_t>(value);
		break;
	default:
		break;
	}
}

static int t2op_practice_digit_edge(
	uint8_t now0, uint8_t prev0, uint8_t now1, uint8_t prev1
)
{
	#define PRESSED(now, prev, bit) (((now) & (bit)) && !((prev) & (bit)))
	if(PRESSED(now0, prev0, K0_1)) return 1;
	if(PRESSED(now0, prev0, K0_2)) return 2;
	if(PRESSED(now0, prev0, K0_3)) return 3;
	if(PRESSED(now0, prev0, K0_4)) return 4;
	if(PRESSED(now0, prev0, K0_5)) return 5;
	if(PRESSED(now0, prev0, K0_6)) return 6;
	if(PRESSED(now0, prev0, K0_7)) return 7;
	if(PRESSED(now1, prev1, K1_8)) return 8;
	if(PRESSED(now1, prev1, K1_9)) return 9;
	if(PRESSED(now1, prev1, K1_0)) return 0;
	#undef PRESSED
	return -1;
}

static void t2op_practice_render(void);

static void t2op_practice_numeric_entry(uint8_t field)
{
	uint32_t original = t2op_practice_numeric_get(field);
	uint32_t original_high_score = t2op_practice.score_highest;
	uint32_t value = 0;
	uint32_t min = t2op_practice_numeric_min(field);
	uint32_t max = t2op_practice_numeric_max(field);
	uint8_t now0;
	uint8_t now1;
	uint8_t now3;
	uint8_t prev0;
	uint8_t prev1;
	uint8_t prev3;
	int digit;
	bool entered = false;

	do {
		prev3 = static_cast<uint8_t>(peekb(0, KEYGROUP_3));
		frame_delay(1);
	} while(prev3 & K3_RETURN);
	prev0 = static_cast<uint8_t>(peekb(0, KEYGROUP_0));
	prev1 = static_cast<uint8_t>(peekb(0, KEYGROUP_1));
	while(1) {
		now0 = static_cast<uint8_t>(peekb(0, KEYGROUP_0));
		now1 = static_cast<uint8_t>(peekb(0, KEYGROUP_1));
		now3 = static_cast<uint8_t>(peekb(0, KEYGROUP_3));
		if((now0 & K0_ESC) && !(prev0 & K0_ESC)) {
			t2op_practice_numeric_set(field, original);
			t2op_practice.score_highest = original_high_score;
			t2op_practice_render();
			return;
		}
		if((now3 & K3_RETURN) && !(prev3 & K3_RETURN)) {
			if(entered) {
				if(value < min) {
					value = min;
				}
				t2op_practice_numeric_set(field, value);
			}
			t2op_practice_render();
			return;
		}
		if((now1 & K1_BACKSPACE) && !(prev1 & K1_BACKSPACE)) {
			value /= 10UL;
			t2op_practice_numeric_set(field, ((value < min) ? min : value));
			entered = true;
			t2op_practice_render();
		} else {
			digit = t2op_practice_digit_edge(now0, prev0, now1, prev1);
			if(digit >= 0) {
				if(value > ((max - digit) / 10UL)) {
					value = max;
				} else {
					value = ((value * 10UL) + digit);
				}
				t2op_practice_numeric_set(
					field, ((value < min) ? min : value)
				);
				entered = true;
				t2op_practice_render();
			}
		}
		prev0 = now0;
		prev1 = now1;
		prev3 = now3;
		frame_delay(1);
	}
}

static void t2op_practice_render(void)
{
	char *p;
	uint8_t row;

	// shottype_menu() leaves page 1 as the unadorned character-select
	// background and page 0 as its selected foreground. Explicitly select the
	// former before copying it so that each Setup redraw removes every prior
	// patch shadow instead of self-copying page 0.
	text_clear();
	graph_accesspage(1);
	graph_copy_page(0);
	graph_showpage(0);
	graph_accesspage(0);
	p = t2op_line;
	p = t2op_word_append(p, T2OW_PRACTICE);
	t2op_title_word_center_put(2, TX_GREEN, p);

	for(row = 0; row < T2OPC_START; row++) {
		t2op_word_t label;
		tram_y_t y = static_cast<tram_y_t>(4 + row);
		tram_atrb2 attr = (
			(t2op_practice_sel == row) ? TX_WHITE : TX_YELLOW
		);

		p = t2op_line;
		switch(row) {
		case T2OPC_STAGE: label = T2OW_STAGE; break;
		case T2OPC_SECTION: label = T2OW_SECTION; break;
		case T2OPC_SCORE: label = T2OW_SCORE; break;
		case T2OPC_HIGH_SCORE: label = T2OW_HIGH_SCORE; break;
		case T2OPC_POWER: label = T2OW_POWER; break;
		case T2OPC_LIVES: label = T2OW_LIVES; break;
		case T2OPC_BOMBS: label = T2OW_BOMBS; break;
		case T2OPC_SEED: label = T2OW_SEED; break;
		case T2OPC_SKILL: label = T2OW_SKILL; break;
		case T2OPC_BGM: label = T2OW_BGM; break;
		default: label = T2OW_REDUCED_EFFECTS; break;
		}
		p = t2op_word_append(p, label);
		t2op_title_word_put(T2OP_PRACTICE_LABEL_X, y, attr, p);

		p = t2op_line;
		switch(row) {
		case T2OPC_STAGE: p = t2op_stage_append(p, t2op_practice.stage); break;
		case T2OPC_SECTION:
			switch(t2op_practice.reserved[
				T2REPLAY_PRACTICE_TARGET_OFFSET
			]) {
			case T2RPT_STAGE_START:
				p = t2op_word_append(p, T2OW_STAGE_START);
				break;
			case T2RPT_STAGE4_CHAPTER3:
				p = t2op_word_append(p, T2OW_CHAPTER_3);
				break;
			case T2RPT_STAGE1_MIDBOSS:
			case T2RPT_STAGE2_MIDBOSS:
			case T2RPT_STAGE3_MIDBOSS:
				p = t2op_word_append(p, T2OW_MIDBOSS);
				break;
			case T2RPT_STAGE4_MIDBOSS_FIRST:
				p = t2op_word_append(p, T2OW_MIDBOSS);
				*p++ = ' ';
				*p++ = '1';
				break;
			case T2RPT_STAGE4_MIDBOSS_SECOND:
				p = t2op_word_append(p, T2OW_MIDBOSS);
				*p++ = ' ';
				*p++ = '2';
				break;
			case T2RPT_STAGE1_BOSS_PHASE1:
			case T2RPT_STAGE2_BOSS_PHASE1:
			case T2RPT_STAGE4_BOSS_PHASE1:
			case T2RPT_STAGE5_BOSS_PHASE1:
			case T2RPT_EXTRA_BOSS_PHASE1:
				p = t2op_word_append(p, T2OW_BOSS_PHASE_1);
				break;
			case T2RPT_STAGE4_BOSS_ROUND2:
				p = t2op_word_append(p, T2OW_BOSS_ROUND_2);
				break;
			case T2RPT_STAGE1_BOSS_PHASE2:
			case T2RPT_STAGE2_BOSS_PHASE2:
				p = t2op_word_append(p, T2OW_BOSS_PHASE_2);
				break;
			case T2RPT_STAGE1_BOSS_PHASE3:
			case T2RPT_STAGE2_BOSS_PHASE3:
			case T2RPT_STAGE5_BOSS_PHASE3:
			case T2RPT_EXTRA_BOSS_PHASE3:
				p = t2op_word_append(p, T2OW_BOSS_PHASE_3);
				break;
			case T2RPT_STAGE3_BOSS_START:
			case T2RPT_STAGE4_BOSS_START:
			case T2RPT_STAGE5_BOSS_START:
			case T2RPT_EXTRA_BOSS_START:
				p = t2op_word_append(p, T2OW_BOSS_START);
				break;
			case T2RPT_STAGE3_INNER_PAIR:
				p = t2op_word_append(p, T2OW_INNER_PAIR);
				break;
			case T2RPT_STAGE3_OUTER_PAIR:
				p = t2op_word_append(p, T2OW_OUTER_PAIR);
				break;
			case T2RPT_EXTRA_MIDBOSS:
				p = t2op_word_append(p, T2OW_MIDBOSS);
				break;
			default:
				p = t2op_word_append(p, T2OW_CHAPTER_2);
				break;
			}
			break;
		case T2OPC_SCORE: p = t2op_i32_append(p, t2op_practice.score, 0); break;
		case T2OPC_HIGH_SCORE: p = t2op_u32_append(p, t2op_practice.score_highest, 0); break;
		case T2OPC_POWER: p = t2op_i32_append(p, t2op_practice.start_power, 0); break;
		case T2OPC_LIVES: p = t2op_u32_append(p, t2op_practice.start_lives, 0); break;
		case T2OPC_BOMBS: p = t2op_u32_append(p, t2op_practice.start_bombs, 0); break;
		case T2OPC_SEED: p = t2op_u32_append(p, t2op_practice.resident_frame, 0); break;
		case T2OPC_SKILL: p = t2op_i32_append(p, t2op_practice.skill, 0); break;
		case T2OPC_BGM: p = t2op_bgm_append(p, t2op_practice.bgm_mode); break;
		default: p = t2op_word_append(p, t2op_practice.reduce_effects ? T2OW_ON : T2OW_OFF); break;
		}
		t2op_title_word_right_put(T2OP_PRACTICE_VALUE_RIGHT, y, attr, p);
	}
	p = t2op_line;
	p = t2op_word_append(p, T2OW_START_RUN);
	t2op_title_word_center_put(static_cast<tram_y_t>(4 + T2OPC_START),
		(t2op_practice_sel == T2OPC_START) ? TX_WHITE : TX_GREEN, p);
}

static void t2op_resident_apply(const t2replay_start_t far *start)
{
	resident->frame = static_cast<long>(start->resident_frame);
	resident->score = start->score;
	resident->score_highest = start->score_highest;
	resident->continues_used = start->continues_used;
	resident->skill = start->skill;
	resident->stage = start->stage;
	resident->rank = start->rank;
	resident->rem_lives = start->rem_lives;
	resident->rem_bombs = start->rem_bombs;
	resident->start_lives = start->start_lives;
	resident->start_bombs = start->start_bombs;
	resident->start_power = start->start_power;
	resident->shottype = start->shottype;
	resident->bgm_mode = start->bgm_mode;
	resident->reduce_effects = (start->reduce_effects != 0);
	resident->debug = false;
	resident->demo_num = 0;
	rank = start->rank;
	lives = start->start_lives;
	bombs = start->start_bombs;
	snd_bgm_mode = start->bgm_mode;
}

static void t2op_main_exec(void)
{
	char pi_fn[7];
	char main_fn[5];

#if T2REPLAY_PRACTICE_DIAGNOSTICS
	replay_practice_diag_boot(10);
#endif
	pi_fn[0] = 't'; pi_fn[1] = 's'; pi_fn[2] = '1';
	pi_fn[3] = '.'; pi_fn[4] = 'p'; pi_fn[5] = 'i'; pi_fn[6] = '\0';
	main_fn[0] = 'm'; main_fn[1] = 'a'; main_fn[2] = 'i'; main_fn[3] = 'n';
	main_fn[4] = '\0';
	pi_load(0, pi_fn);
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	replay_practice_diag_boot(11);
#endif
	text_clear();
	snd_kaja_func(KAJA_SONG_FADE, 15);
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	replay_practice_diag_boot(12);
#endif
	gaiji_restore();
	super_free();
	game_exit();
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	replay_practice_diag_boot(13);
#endif
	execl(main_fn, main_fn, nullptr);
}

void replay_op_restart_or_snd_load(const char *fn, int func)
{
	t2replay_start_t start;
	uint8_t flags;
	char request_fn[11];

	if(!t2op_restart_command_read(&start, &flags)) {
		snd_load(fn, static_cast<snd_load_func_t>(func));
		return;
	}
	t2op_save_request_fn_set(request_fn);
	t2op_file_delete(request_fn);
	t2op_temp_set();
	t2op_file_delete(t2op_slot_fn);
	if(!t2op_command_write(
		T2REPLAY_COMMAND_RECORD, T2REPLAY_TEMP_SLOT, flags,
		(flags & T2REPLAY_COMMAND_FLAG_PRACTICE) ? &start : 0
	)) {
		snd_load(fn, static_cast<snd_load_func_t>(func));
		return;
	}
	start_init();
	t2op_resident_apply(&start);
	t2op_main_exec();
}

static void t2op_playback_start(uint8_t slot)
{
	// Playback owns its launch state in the replay header. Preserve the user's
	// title options before start_init() writes the transient resident fields.
	cfg_save();
	if(t2op_command_write(T2REPLAY_COMMAND_PLAYBACK, slot, 0, 0)) {
		start_init();
		t2op_main_exec();
	}
}

static void t2op_practice_start(void)
{
	char request_fn[11];

	// Keep the selected title options persistent. The Practice payload is a
	// one-run resident override consumed by MAIN, never a new configuration.
	cfg_save();
	t2op_save_request_fn_set(request_fn);
	t2op_file_delete(request_fn);
	t2op_temp_set();
	t2op_file_delete(t2op_slot_fn);
	if(!t2op_command_write(
		T2REPLAY_COMMAND_RECORD,
		T2REPLAY_TEMP_SLOT,
		T2REPLAY_COMMAND_FLAG_PRACTICE,
		&t2op_practice
	)) {
		return;
	}
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	replay_practice_diag_boot(6);
#endif
	start_init();
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	replay_practice_diag_boot(7);
#endif
	t2op_resident_apply(&t2op_practice);
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	replay_practice_diag_boot(8);
#endif
	t2op_main_exec();
}

#if T2REPLAY_PRACTICE_DIAGNOSTICS
void replay_practice_diag_autostart(void)
{
	static const char fn[] = "T2PAUTO.CFG";
	char request_fn[11];
	uint8_t config[3];
	int fd = t2op_dos_open(fn, T2OP_DOS_ACCESS_READ);

	if(fd < 0) {
		return;
	}
	if(t2op_dos_read(fd, config, sizeof(config)) != sizeof(config)) {
		t2op_dos_close(fd);
		t2op_file_delete(fn);
		return;
	}
	t2op_dos_close(fd);
	t2op_file_delete(fn);
	if((config[0] >= T2REPLAY_STAGE_COUNT) ||
	   (config[2] >= SHOTTYPE_COUNT)) {
		return;
	}
	t2op_practice_defaults();
	t2op_practice_stage_set(static_cast<int8_t>(config[0]));
	t2op_practice.shottype = config[2];
	t2op_practice.bgm_mode = SND_BGM_OFF;
	t2op_practice.reserved[T2REPLAY_PRACTICE_TARGET_OFFSET] = config[1];
	if(t2op_start_valid(&t2op_practice)) {
		cfg_save();
		t2op_save_request_fn_set(request_fn);
		t2op_file_delete(request_fn);
		t2op_temp_set();
		t2op_file_delete(t2op_slot_fn);
		if(!t2op_command_write(
			T2REPLAY_COMMAND_RECORD,
			T2REPLAY_TEMP_SLOT,
			T2REPLAY_COMMAND_FLAG_PRACTICE,
			&t2op_practice
		)) {
			return;
		}
		// start_init() only adds a title SFX/delay before initializing fields
		// that the complete Practice payload replaces. Keep this private launch
		// independent of title timing while preserving its remaining zeros.
		resident->unused_3 = 0;
		resident->unused_1 = 0;
		t2op_resident_apply(&t2op_practice);
		t2op_main_exec();
	}
}
#endif

static void t2op_record_then_start(bool extra)
{
	char request_fn[11];

	t2op_paths_init();
	t2op_save_request_fn_set(request_fn);
	t2op_file_delete(request_fn);
	t2op_temp_set();
	t2op_file_delete(t2op_slot_fn);
	if(!t2op_command_write(
		T2REPLAY_COMMAND_RECORD, T2REPLAY_TEMP_SLOT, 0, 0
	)) {
		return;
	}
	if(extra) {
		start_extra();
	} else {
		start_game();
	}
}

static void t2op_browser_slot_render(uint8_t slot, tram_y_t y)
{
	char *p;
	bool valid = t2op_header_read(slot);
	tram_atrb2 attr = (slot == t2op_browser_sel) ? TX_WHITE : TX_YELLOW;

	if(slot == t2op_browser_sel) {
		p = t2op_char(t2op_line, '>');
		t2op_text_put(T2OP_BROWSER_MARKER_X, y, attr, p);
	}
	p = t2op_line;
	p = t2op_u32_append(p, slot, 2);
	t2op_text_put(T2OP_BROWSER_SLOT_X, y, attr, p);
	if(!valid) {
		p = t2op_word_append(
			t2op_line, file_exist(t2op_slot_fn) ? T2OW_INVALID : T2OW_NONE
		);
		t2op_text_put(T2OP_BROWSER_NAME_X, y, attr, p);
		return;
	}
	t2op_name_put(
		T2OP_BROWSER_NAME_X, y,
		t2op_header.reserved + T2REPLAY_RESERVED_NAME_OFFSET, attr
	);
	p = t2op_line;
	p = t2op_character_append(p, t2op_header.start.shottype);
	t2op_text_put(T2OP_BROWSER_SHOT_X, y, attr, p);
	p = t2op_line;
	p = t2op_rank_append(p, t2op_header.start.rank);
	t2op_text_put(T2OP_BROWSER_RANK_X, y, attr, p);
	p = t2op_line;
	p = t2op_i32_append(p, t2op_header.score_final, 10);
	t2op_text_put(T2OP_BROWSER_SCORE_X, y, attr, p);
	p = t2op_line;
	p = t2op_stage_append(p, static_cast<int8_t>(t2op_header.stage_reached));
	t2op_text_put(T2OP_BROWSER_STAGE_X, y, attr, p);
}

static void t2op_browser_render(bool save_pending)
{
	char *p;
	uint8_t first = static_cast<uint8_t>(
		(t2op_browser_sel / T2OP_SLOT_ROWS) * T2OP_SLOT_ROWS
	);
	uint8_t i;

	text_clear();
	p = t2op_line;
	p = t2op_word_append(p, save_pending ? T2OW_SAVE_REPLAY : T2OW_BROWSER);
	t2op_text_put(31, 2, TX_GREEN, p);
	p = t2op_word_append(t2op_line, T2OW_SLOT);
	t2op_text_put(T2OP_BROWSER_SLOT_X, 4, TX_GREEN, p);
	p = t2op_word_append(t2op_line, T2OW_NAME);
	t2op_text_put(T2OP_BROWSER_NAME_X, 4, TX_GREEN, p);
	p = t2op_word_append(t2op_line, T2OW_SHOT);
	t2op_text_put(T2OP_BROWSER_SHOT_X, 4, TX_GREEN, p);
	p = t2op_word_append(t2op_line, T2OW_RANK);
	t2op_text_put(T2OP_BROWSER_RANK_X, 4, TX_GREEN, p);
	p = t2op_word_append(t2op_line, T2OW_SCORE);
	t2op_text_put(T2OP_BROWSER_SCORE_X, 4, TX_GREEN, p);
	p = t2op_word_append(t2op_line, T2OW_STAGE);
	t2op_text_put(T2OP_BROWSER_STAGE_X, 4, TX_GREEN, p);
	for(i = 0; i < T2OP_SLOT_ROWS; i++) {
		t2op_browser_slot_render(static_cast<uint8_t>(first + i), 6 + i);
	}
	p = t2op_line;
	p = t2op_word_append(p, T2OW_PAGE);
	p = t2op_char(p, ' ');
	p = t2op_u32_append(p, ((t2op_browser_sel / T2OP_SLOT_ROWS) + 1), 2);
	p = t2op_char(p, '/');
	p = t2op_u32_append(p, (T2REPLAY_SLOT_COUNT / T2OP_SLOT_ROWS), 2);
	t2op_text_put(34, 18, TX_GREEN, p);
}

static void t2op_detail_render(uint8_t slot)
{
	char *p;
	uint8_t stage;
	tram_y_t y;

	text_clear();
	p = t2op_line;
	p = t2op_word_append(p, T2OW_SLOT);
	p = t2op_char(p, ' ');
	p = t2op_u32_append(p, slot, 2);
	t2op_text_put(5, 3, TX_GREEN, p);
	if(t2op_name_empty(
		t2op_header.reserved + T2REPLAY_RESERVED_NAME_OFFSET
	)) {
		p = t2op_word_append(t2op_line, T2OW_NONE);
		t2op_text_put(20, 3, TX_WHITE, p);
	} else {
		t2op_name_put(
			20, 3, t2op_header.reserved + T2REPLAY_RESERVED_NAME_OFFSET,
			TX_WHITE
		);
	}

	p = t2op_line;
	p = t2op_end_reason_append(p, t2op_header.end_reason);
	t2op_text_put(5, 5, TX_WHITE, p);

	#define T2OP_DETAIL_LABEL(y, word) \
		p = t2op_word_append(t2op_line, word); \
		t2op_text_put(5, y, TX_YELLOW, p)
	T2OP_DETAIL_LABEL(6, T2OW_FINAL_SCORE);
	p = t2op_i32_append(t2op_line, t2op_header.score_final, 10);
	t2op_text_put(20, 6, TX_WHITE, p);
	T2OP_DETAIL_LABEL(7, T2OW_CHARACTER);
	p = t2op_character_append(t2op_line, t2op_header.start.shottype);
	t2op_text_put(20, 7, TX_WHITE, p);
	T2OP_DETAIL_LABEL(8, T2OW_RANK);
	p = t2op_rank_append(t2op_line, t2op_header.start.rank);
	t2op_text_put(20, 8, TX_WHITE, p);
	T2OP_DETAIL_LABEL(9, T2OW_START_POINT);
	p = t2op_stage_append(t2op_line, t2op_header.start.stage);
	t2op_text_put(20, 9, TX_WHITE, p);
	T2OP_DETAIL_LABEL(10, T2OW_LIVES);
	p = t2op_i32_append(t2op_line, t2op_header.lives_final, 0);
	t2op_text_put(20, 10, TX_WHITE, p);
	T2OP_DETAIL_LABEL(11, T2OW_BOMBS);
	p = t2op_i32_append(t2op_line, t2op_header.bombs_final, 0);
	t2op_text_put(20, 11, TX_WHITE, p);
	T2OP_DETAIL_LABEL(12, T2OW_POWER);
	p = t2op_u32_append(t2op_line, t2op_header.power_final, 0);
	t2op_text_put(20, 12, TX_WHITE, p);
	#undef T2OP_DETAIL_LABEL

	p = t2op_line;
	p = t2op_word_append(p, T2OW_STAGE_SPLITS);
	t2op_text_put(43, 3, TX_GREEN, p);
	y = 5;
	for(
		stage = static_cast<uint8_t>(t2op_header.start.stage);
		stage <= t2op_header.stage_reached;
		stage++
	) {
		p = t2op_line;
		p = t2op_stage_append(p, static_cast<int8_t>(stage));
		t2op_text_put(T2OP_DETAIL_SPLIT_STAGE_X, y, TX_WHITE, p);
		p = t2op_u32_append(t2op_line, t2op_header.stage_scores[stage], 10);
		t2op_text_put(T2OP_DETAIL_SPLIT_SCORE_X, y, TX_WHITE, p);
		y++;
	}
}

static void t2op_detail(uint8_t slot)
{
	bool input_allowed = false;

	t2op_detail_render(slot);
	while(1) {
		input_reset_sense();
		if(key_det == INPUT_NONE) {
			input_allowed = true;
		}
		if(input_allowed) {
			if(key_det & INPUT_CANCEL) {
				break;
			} else if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
				t2op_playback_start(slot);
			}
			if(key_det != INPUT_NONE) {
				input_allowed = false;
			}
		}
		frame_delay(1);
	}
	key_det = INPUT_NONE;
}

static void t2op_pending_discard(void)
{
	char request_fn[11];

	t2op_paths_init();
	t2op_save_request_fn_set(request_fn);
	t2op_file_delete(request_fn);
	t2op_temp_set();
	t2op_file_delete(t2op_slot_fn);
}

static void t2op_save_confirm_render(bool save)
{
	char *p;

	text_clear();
	p = t2op_word_append(t2op_line, T2OW_SAVE_REPLAY);
	p = t2op_char(p, '?');
	t2op_text_put(31, 9, TX_GREEN, p);
	p = t2op_line;
	p = t2op_char(p, save ? '>' : ' ');
	p = t2op_char(p, ' ');
	p = t2op_word_append(p, T2OW_YES);
	t2op_text_put(34, 11, save ? TX_WHITE : TX_YELLOW, p);
	p = t2op_line;
	p = t2op_char(p, save ? ' ' : '>');
	p = t2op_char(p, ' ');
	p = t2op_word_append(p, T2OW_NO);
	t2op_text_put(34, 12, save ? TX_YELLOW : TX_WHITE, p);
}

static bool t2op_save_confirm(void)
{
	bool input_allowed = false;
	bool save = true;

	t2op_save_confirm_render(save);
	while(1) {
		input_reset_sense();
		if(key_det == INPUT_NONE) {
			input_allowed = true;
		}
		if(input_allowed) {
			if(
				(key_det & INPUT_UP) || (key_det & INPUT_DOWN) ||
				(key_det & INPUT_LEFT) || (key_det & INPUT_RIGHT)
			) {
				save = !save;
				t2op_save_confirm_render(save);
			} else if(key_det & INPUT_CANCEL) {
				key_det = INPUT_NONE;
				return false;
			} else if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
				key_det = INPUT_NONE;
				return save;
			}
			if(key_det != INPUT_NONE) {
				input_allowed = false;
			}
		}
		frame_delay(1);
	}
}

static void t2op_overwrite_render(uint8_t slot, bool overwrite)
{
	char *p;

	if(t2op_header_read(slot)) {
		t2op_detail_render(slot);
	} else {
		t2op_browser_render(true);
	}
	p = t2op_word_append(t2op_line, T2OW_OVERWRITE_REPLAY);
	t2op_text_put(29, 19, TX_GREEN, p);
	p = t2op_line;
	p = t2op_char(p, overwrite ? '>' : ' ');
	p = t2op_char(p, ' ');
	p = t2op_word_append(p, T2OW_YES);
	t2op_text_put(31, 20, overwrite ? TX_WHITE : TX_YELLOW, p);
	p = t2op_line;
	p = t2op_char(p, overwrite ? ' ' : '>');
	p = t2op_char(p, ' ');
	p = t2op_word_append(p, T2OW_NO);
	t2op_text_put(31, 21, overwrite ? TX_YELLOW : TX_WHITE, p);
}

static bool t2op_overwrite_confirm(uint8_t slot)
{
	bool input_allowed = false;
	bool overwrite = false;

	t2op_overwrite_render(slot, overwrite);
	while(1) {
		input_reset_sense();
		if(key_det == INPUT_NONE) {
			input_allowed = true;
		}
		if(input_allowed) {
			if(
				(key_det & INPUT_UP) || (key_det & INPUT_DOWN) ||
				(key_det & INPUT_LEFT) || (key_det & INPUT_RIGHT)
			) {
				overwrite = !overwrite;
				t2op_overwrite_render(slot, overwrite);
			} else if(key_det & INPUT_CANCEL) {
				key_det = INPUT_NONE;
				return false;
			} else if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
				key_det = INPUT_NONE;
				return overwrite;
			}
			if(key_det != INPUT_NONE) {
				input_allowed = false;
			}
		}
		frame_delay(1);
	}
}

static bool t2op_pending_commit(
	uint8_t slot, bool overwrite, const uint8_t far *name
)
{
	char request_fn[11];
	char destination[11];
	char backup[11];
	uint8_t i;
	bool backed_up = false;
	bool renamed;

	t2op_paths_init();
	t2op_slot_backup_set(backup, slot);
	for(i = 0; i < sizeof(destination); i++) {
		destination[i] = t2op_slot_fn[i];
	}
	t2op_temp_set();
	if(!t2op_pending_request_valid()) {
		return false;
	}
	if(!t2op_name_valid(name)) {
		return false;
	}
	t2op_header_saved = t2op_header;
	for(i = 0; i < T2REPLAY_NAME_LEN; i++) {
		t2op_header.reserved[T2REPLAY_RESERVED_NAME_OFFSET + i] = name[i];
	}
	if(
		!t2op_pending_header_write() ||
		!t2op_pending_request_rebind()
	) {
		t2op_header = t2op_header_saved;
		t2op_pending_header_write();
		return false;
	}
	if(file_exist(destination)) {
		if(!overwrite || file_exist(backup) ||
			!t2op_file_rename(destination, backup)) {
			return false;
		}
		backed_up = true;
	}
	renamed = t2op_file_rename(t2op_slot_fn, destination);
	if(!renamed) {
		if(backed_up) {
			t2op_file_rename(backup, destination);
		}
		return false;
	}
	if(backed_up) {
		t2op_file_delete(backup);
	}
	t2op_save_request_fn_set(request_fn);
	t2op_file_delete(request_fn);
	return true;
}

static void t2op_browser_saved_wait(void)
{
	bool released = false;

	while(1) {
		input_reset_sense();
		if(key_det == INPUT_NONE) {
			released = true;
		} else if(released) {
			break;
		}
		frame_delay(1);
	}
	key_det = INPUT_NONE;
}

static void t2op_browser(bool save_pending, const uint8_t far *pending_name)
{
	bool input_allowed = false;

	t2op_browser_render(save_pending);
	while(1) {
		input_reset_sense();
		if(key_det == INPUT_NONE) {
			input_allowed = true;
		}
		if(input_allowed) {
			if(key_det & INPUT_UP) {
				t2op_browser_sel = ((t2op_browser_sel == 0)
					? (T2REPLAY_SLOT_COUNT - 1) : (t2op_browser_sel - 1));
				t2op_browser_render(save_pending);
			} else if(key_det & INPUT_DOWN) {
				t2op_browser_sel = ((t2op_browser_sel == (T2REPLAY_SLOT_COUNT - 1))
					? 0 : (t2op_browser_sel + 1));
				t2op_browser_render(save_pending);
			} else if(key_det & INPUT_LEFT) {
				t2op_browser_sel = ((t2op_browser_sel < T2OP_SLOT_ROWS)
					? (t2op_browser_sel + (T2REPLAY_SLOT_COUNT - T2OP_SLOT_ROWS))
					: (t2op_browser_sel - T2OP_SLOT_ROWS));
				t2op_browser_render(save_pending);
			} else if(key_det & INPUT_RIGHT) {
				t2op_browser_sel = ((t2op_browser_sel >=
					(T2REPLAY_SLOT_COUNT - T2OP_SLOT_ROWS))
					? (t2op_browser_sel - (T2REPLAY_SLOT_COUNT - T2OP_SLOT_ROWS))
					: (t2op_browser_sel + T2OP_SLOT_ROWS));
				t2op_browser_render(save_pending);
			} else if(key_det & INPUT_CANCEL) {
				if(save_pending) {
					t2op_pending_discard();
				}
				break;
			} else if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
				if(save_pending) {
					bool occupied;

					t2op_header_read(t2op_browser_sel);
					occupied = file_exist(t2op_slot_fn);
					if(
						(!occupied || t2op_overwrite_confirm(t2op_browser_sel)) &&
						t2op_pending_commit(
							t2op_browser_sel, occupied, pending_name
						)
					) {
						t2op_browser_render(false);
						t2op_browser_saved_wait();
						break;
					}
					t2op_browser_render(true);
				} else if(t2op_header_read(t2op_browser_sel)) {
					t2op_detail(t2op_browser_sel);
					t2op_browser_render(false);
				}
			}
			if(key_det != INPUT_NONE) {
				input_allowed = false;
			}
		}
		frame_delay(1);
	}
	t2op_title_return_request();
	key_det = INPUT_NONE;
}

static bool t2op_pending_save(void)
{
	if(!t2op_pending_header_read()) {
		return false;
	}
	if(
		(t2op_pending_source == T2REPLAY_SAVE_REQUEST_CLEAR) &&
		!t2op_save_confirm()
	) {
		t2op_pending_discard();
		return true;
	}
	if(!t2op_name_menu(t2op_pending_name)) {
		t2op_pending_discard();
		return true;
	}
	t2op_browser(true, t2op_pending_name);
	return true;
}

static void t2op_practice_menu(void)
{
	bool input_allowed = false;
	uint8_t horizontal_hold = 0;
	bool horizontal_trigger;
	bool right;

	// The title confirmation must not become the character-select confirmation.
	t2op_input_wait_release();

	// Reuse the native selector and keep its background in VRAM for Setup.
	resident->stage = 0;
	sel = 1;
	pi_load(0, "ts1.pi");
	text_clear();
	shottype_menu();
	t2op_title_font_restore();
	t2op_practice_defaults();
	t2op_practice_sel = T2OPC_STAGE;
	t2op_practice_render();
	palette_black_in(2);
	while(1) {
		input_reset_sense();
		if(key_det == INPUT_NONE) {
			input_allowed = true;
		}
		right = ((key_det & INPUT_RIGHT) != 0);
		if((key_det & (INPUT_LEFT | INPUT_RIGHT)) == 0) {
			horizontal_hold = 0;
			horizontal_trigger = false;
		} else {
			horizontal_trigger = (
				input_allowed ||
				((horizontal_hold >= 12) && ((horizontal_hold & 1) == 0))
			);
			if(horizontal_hold != 255) {
				horizontal_hold++;
			}
		}
		if(horizontal_trigger) {
			t2op_practice_value_step((right ? +1 : -1), t2op_shift_pressed());
			t2op_practice_render();
			input_allowed = false;
		} else if(input_allowed) {
			if(key_det & INPUT_UP) {
				t2op_practice_sel = ((t2op_practice_sel == 0)
					? (T2OPC_COUNT - 1) : (t2op_practice_sel - 1));
				t2op_practice_render();
			} else if(key_det & INPUT_DOWN) {
				t2op_practice_sel = ((t2op_practice_sel == (T2OPC_COUNT - 1))
					? 0 : (t2op_practice_sel + 1));
				t2op_practice_render();
			} else if(key_det & INPUT_CANCEL) {
				break;
			} else if((key_det & INPUT_OK) &&
				t2op_practice_field_is_numeric(t2op_practice_sel)) {
				t2op_practice_numeric_entry(t2op_practice_sel);
			} else if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
				if(t2op_practice_sel == T2OPC_START) {
					t2op_practice_start();
				}
			}
			if(key_det != INPUT_NONE) {
				input_allowed = false;
			}
		}
		frame_delay(1);
	}
	// shottype_menu() frees every portrait slot. Vanilla never returns from it,
	// but Practice can, so restore the two title-resident portraits.
	pi_load(2, "ts3.pi");
	pi_load(1, "ts2.pi");
	t2op_title_return_request();
	key_det = INPUT_NONE;
}

void replay_title_update_and_render(void)
{
	if(!t2op_main_initialized) {
		t2op_main_initialized = true;
		t2op_main_input_allowed = false;
		t2op_main_render();
		if(t2op_pending_save()) {
			t2op_main_render();
		}
	} else if(replay_title_restore_needed) {
		t2op_main_render();
	}
	if(key_det == INPUT_NONE) {
		t2op_main_input_allowed = true;
	}
	if(!t2op_main_input_allowed) {
		return;
	}
	if(key_det & INPUT_UP) {
		t2op_main_selection_step(-1);
		t2op_main_render();
	} else if(key_det & INPUT_DOWN) {
		t2op_main_selection_step(+1);
		t2op_main_render();
	} else if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
		switch(t2op_main_sel) {
		case T2OMC_START:
			t2op_record_then_start(false);
			break;
		case T2OMC_EXTRA:
			if(extra_unlocked) {
				t2op_record_then_start(true);
			}
			break;
		case T2OMC_PRACTICE:
			t2op_practice_menu();
			t2op_main_render();
			break;
		case T2OMC_REPLAY:
			t2op_browser(false, 0);
			t2op_main_render();
			break;
		case T2OMC_HISCORE:
			score_frames = 2000;
			text_clear();
			score_menu();
			t2op_title_return_request();
			t2op_main_render();
			break;
		case T2OMC_OPTIONS:
			menu_sel = 0;
			in_option = true;
			t2op_title_return_request();
			break;
		case T2OMC_MUSIC:
			text_clear();
			musicroom_menu();
			t2op_title_return_request();
			t2op_main_render();
			break;
		default:
			quit = true;
			break;
		}
	}
	if(key_det & INPUT_CANCEL) {
		quit = true;
	}
	if(key_det != INPUT_NONE) {
		t2op_main_input_allowed = false;
		idle_frame = 0;
	}
	if(idle_frame > 640) {
		start_demo();
	}
}

#pragma codeseg
