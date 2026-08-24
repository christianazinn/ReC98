#pragma option -zCREPLAY_OP_TEXT

// Interim OP-side browser and automatic recorder for the TH04/TH05 compact
// user replay format. All implementation lives in a new segment; the stock
// title menu only calls the narrow entry points in replay.hpp.

#include "platform.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/rank.h"
#include "th02/hardware/frmdelay.h"
#include "th02/v_colors.hpp"
#include "th04/common.h"
#include "th04/hardware/grppsafx.h"
#include "th04/op/replay.hpp"
#include "th04/replay_format.hpp"
#include "th04/snd/snd.h"
#if (GAME == 5)
	#include "th05/hardware/input.h"
#else
	#include "th04/hardware/input.h"
#endif

#define REPLAY_OP_FP_SEG(p) ((unsigned)(((unsigned long)(void far *)(p)) >> 16))
#define REPLAY_OP_FP_OFF(p) ((unsigned)((unsigned long)(void far *)(p)))

#define REPLAY_OP_ACCESS_READ 0
#define REPLAY_OP_SLOT_ROWS 10
#define REPLAY_OP_LINE_CAPACITY 80
#define REPLAY_OP_LINE_LEFT 96
#define REPLAY_OP_LINE_TOP 88
#define REPLAY_OP_LINE_H 24
#define REPLAY_OP_COL_ACTIVE ((GAME == 5) ? 14 : 8)

enum replay_op_word_t {
	ROW_REPLAY,
	ROW_VIEW_RECORDED_RUNS,
	ROW_SLOT,
	ROW_CHARACTER,
	ROW_SHOT,
	ROW_RANK,
	ROW_SCORE,
	ROW_STAGE,
	ROW_NONE,
	ROW_REIMU,
	ROW_MARISA,
	ROW_MIMA,
	ROW_YUUKA,
	ROW_EASY,
	ROW_NORMAL,
	ROW_HARD,
	ROW_LUNATIC,
	ROW_EXTRA,
	ROW_PAGE,
	ROW_UP_DOWN_SELECT,
	ROW_LEFT_RIGHT_PAGE,
	ROW_Z_PLAY,
	ROW_ESC_BACK,
};

static char replay_op_cfg_fn[11];
static char replay_op_slot_fn[11];
static char replay_op_line[REPLAY_OP_LINE_CAPACITY + 1];
static replay_user_header_t replay_op_header;
static bool replay_op_paths_ready;
static uint8_t replay_op_page_shown;

static int replay_op_dos_open(const char far *fn)
{
	unsigned fn_seg = REPLAY_OP_FP_SEG(fn);
	unsigned fn_off = REPLAY_OP_FP_OFF(fn);
	int result;

	_asm {
		push	ds
		mov	dx, fn_off
		mov	ds, fn_seg
		mov	ax, 3D00h
		int	21h
		pop	ds
		sbb	dx, dx
		or	ax, dx
		mov	result, ax
	}
	return result;
}

static int replay_op_dos_create(const char far *fn)
{
	unsigned fn_seg = REPLAY_OP_FP_SEG(fn);
	unsigned fn_off = REPLAY_OP_FP_OFF(fn);
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

static void replay_op_dos_close(int fh)
{
	_asm {
		mov	bx, fh
		mov	ah, 3Eh
		int	21h
	}
}

static unsigned replay_op_dos_read(int fh, void far *buf, unsigned len)
{
	unsigned buf_seg = REPLAY_OP_FP_SEG(buf);
	unsigned buf_off = REPLAY_OP_FP_OFF(buf);
	unsigned result;

	_asm {
		push	ds
		mov	bx, fh
		mov	cx, len
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

static unsigned replay_op_dos_write(
	int fh, const void far *buf, unsigned len
)
{
	unsigned buf_seg = REPLAY_OP_FP_SEG(buf);
	unsigned buf_off = REPLAY_OP_FP_OFF(buf);
	unsigned result;

	_asm {
		push	ds
		mov	bx, fh
		mov	cx, len
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

static bool replay_op_dos_size(int fh, uint32_t *size)
{
	unsigned pos_hi;
	unsigned pos_lo;
	unsigned failed;

	_asm {
		mov	bx, fh
		xor	cx, cx
		xor	dx, dx
		mov	ax, 4202h
		int	21h
		mov	pos_lo, ax
		mov	pos_hi, dx
		sbb	ax, ax
		neg	ax
		mov	failed, ax
	}
	*size = (
		(static_cast<uint32_t>(pos_hi) << 16) |
		static_cast<uint32_t>(pos_lo)
	);
	return (failed == 0);
}

static void replay_op_memclear(void far *buf, unsigned size)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(buf);
	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

static uint32_t replay_op_fnv1a(
	uint32_t hash, const void far *buf, unsigned size
)
{
	const uint8_t far *p = reinterpret_cast<const uint8_t far *>(buf);
	while(size != 0) {
		hash ^= static_cast<uint32_t>(*p++);
		hash *= REPLAY_FNV1A_PRIME;
		size--;
	}
	return hash;
}

static void replay_op_paths_init(void)
{
	if(replay_op_paths_ready) {
		return;
	}
	replay_op_cfg_fn[0] = 'T'; replay_op_cfg_fn[1] = ('0' + GAME);
	replay_op_cfg_fn[2] = 'R'; replay_op_cfg_fn[3] = 'P';
	replay_op_cfg_fn[4] = 'Y'; replay_op_cfg_fn[5] = '.';
	replay_op_cfg_fn[6] = 'C'; replay_op_cfg_fn[7] = 'F';
	replay_op_cfg_fn[8] = 'G'; replay_op_cfg_fn[9] = '\0';

	replay_op_slot_fn[0] = 'T'; replay_op_slot_fn[1] = 'H';
	replay_op_slot_fn[2] = ('0' + GAME); replay_op_slot_fn[3] = 'R';
	replay_op_slot_fn[4] = '0'; replay_op_slot_fn[5] = '0';
	replay_op_slot_fn[6] = '.'; replay_op_slot_fn[7] = 'R';
	replay_op_slot_fn[8] = 'P'; replay_op_slot_fn[9] = 'Y';
	replay_op_slot_fn[10] = '\0';
	replay_op_paths_ready = true;
}

static void replay_op_slot_set(uint8_t slot)
{
	replay_op_slot_fn[4] = ('0' + (slot / 10));
	replay_op_slot_fn[5] = ('0' + (slot % 10));
}

static bool replay_op_bytes_zero(const uint8_t far *p, unsigned size)
{
	while(size != 0) {
		if(*p++ != 0) {
			return false;
		}
		size--;
	}
	return true;
}

static uint8_t replay_op_native_playperf(uint8_t start_rank)
{
	#if (GAME == 5)
		if(start_rank == RANK_HARD) {
			return 44;
		}
		if(start_rank == RANK_LUNATIC) {
			return 48;
		}
		return 32;
	#else
		if(start_rank == RANK_HARD) {
			return 20;
		}
		if(start_rank == RANK_LUNATIC) {
			return 22;
		}
		return 16;
	#endif
}

static bool replay_op_playperf_valid(uint8_t start_rank, uint8_t value)
{
	uint8_t min;
	uint8_t max;

	#if (GAME == 5)
		switch(start_rank) {
		case RANK_EASY: min = 16; max = 32; break;
		case RANK_NORMAL: min = 24; max = 40; break;
		case RANK_HARD: min = 44; max = 54; break;
		case RANK_LUNATIC: min = 48; max = 58; break;
		default: min = 32; max = 36; break;
		}
	#else
		switch(start_rank) {
		case RANK_EASY: min = 4; max = 16; break;
		case RANK_NORMAL: min = 11; max = 24; break;
		case RANK_HARD: min = 20; max = 32; break;
		case RANK_LUNATIC: min = 22; max = 34; break;
		default: min = 16; max = 20; break;
		}
	#endif
	return ((value >= min) && (value <= max));
}

static bool replay_op_start_valid(
	const replay_start_config_t far *start, bool practice
)
{
	if(
		(start->schema != REPLAY_START_SCHEMA) ||
		(start->kind != (practice ? RSK_STAGE : RSK_NATIVE)) ||
		(start->stage > STAGE_EXTRA) ||
		(start->section != 0) || (start->phase != 0) ||
		(start->rank > RANK_EXTRA) ||
		((start->stage == STAGE_EXTRA) != (start->rank == RANK_EXTRA)) ||
		(start->lives > 9) || (start->bombs > 9) ||
		(start->power < 1) || (start->power > 128) ||
		(start->continues_used > 9) || (start->extends_gained > 10) ||
		(start->turbo_mode > 1) ||
		((start->stage == STAGE_EXTRA) && !start->turbo_mode) ||
		(start->score > 99999990UL) || ((start->score % 10UL) != 0) ||
		(start->credit_lives < 1) || (start->credit_lives > 6) ||
		(start->credit_bombs > ((GAME == 5) ? 3 : 2)) ||
		(start->stage_graze > 999) || (start->power_overflow > 42) ||
		!replay_op_playperf_valid(start->rank, start->playperf) ||
		!replay_op_bytes_zero(start->reserved, sizeof(start->reserved))
	) {
		return false;
	}
	#if (GAME == 5)
		if(
			(start->playchar > 3) || start->shottype || (start->dream > 128) ||
			(start->stage_point_items_collected > 999)
		) {
			return false;
		}
	#else
		if(
			(start->playchar > 1) || (start->shottype > 1) ||
			(start->dream > 7) || (start->stage_point_items_collected > 255)
		) {
			return false;
		}
	#endif
	if(!practice && (
		((start->stage != 0) && (start->stage != STAGE_EXTRA)) ||
		(start->score != 0) ||
		(start->lives != start->credit_lives) ||
		(start->bombs != start->credit_bombs) ||
		(start->power != 1) ||
		(start->dream != ((GAME == 5) ? 1 : 0)) ||
		(start->continues_used != 0) || (start->extends_gained != 0) ||
		(start->graze != 0) || (start->std_frames != 0) ||
		(start->items_spawned != 0) || (start->items_collected != 0) ||
		(start->point_items_collected != 0) ||
		(start->max_valued_point_items_collected != 0) ||
		(start->enemies_gone != 0) || (start->enemies_killed != 0) ||
		(start->miss_count != 0) || (start->bombs_used != 0) ||
		(start->stage_point_items_collected != 0) ||
		(start->stage_graze != 0) || (start->power_overflow != 0) ||
		(start->playperf != replay_op_native_playperf(start->rank))
	)) {
		return false;
	}
	return true;
}

static bool replay_op_header_valid(uint32_t file_size)
{
	uint32_t stored = replay_op_header.header_checksum;
	uint32_t computed;
	unsigned i;

	if(
		(replay_op_header.magic[0] != 'T') ||
		(replay_op_header.magic[1] != ('0' + GAME)) ||
		(replay_op_header.magic[2] != 'R') ||
		(replay_op_header.magic[3] != 'P') ||
		(replay_op_header.magic[4] != 'Y') ||
		(replay_op_header.magic[5] != '2') ||
		(replay_op_header.magic[6] != '\0') ||
		(replay_op_header.magic[7] != '\0') ||
		(replay_op_header.version != REPLAY_USER_VERSION) ||
		(replay_op_header.header_size != REPLAY_USER_HEADER_SIZE) ||
		(replay_op_header.packet_size != REPLAY_USER_PACKET_SIZE) ||
		((replay_op_header.flags & ~REPLAY_USER_KNOWN_FLAGS) != 0) ||
		((replay_op_header.flags & (REPLAY_USER_FLAG_RLE_INPUT |
		 REPLAY_USER_FLAG_SHIFT_INPUT)) !=
		 (REPLAY_USER_FLAG_RLE_INPUT | REPLAY_USER_FLAG_SHIFT_INPUT)) ||
		(replay_op_header.status != RUS_FINALIZED) ||
		(replay_op_header.game_id != GAME) ||
		(replay_op_header.ruleset != REPLAY_USER_RULESET_STOCK) ||
		(replay_op_header.mode > RUM_PRACTICE) ||
		((replay_op_header.mode == RUM_PRACTICE) !=
		 ((replay_op_header.flags & REPLAY_USER_FLAG_PRACTICE) != 0)) ||
		(replay_op_header.input_semantics != REPLAY_USER_INPUT_SEMANTICS) ||
		(replay_op_header.input_offset != REPLAY_USER_HEADER_SIZE) ||
		(replay_op_header.input_size > REPLAY_USER_INPUT_SIZE_MAX) ||
		(replay_op_header.packet_count >
		 (REPLAY_USER_INPUT_SIZE_MAX / REPLAY_USER_PACKET_SIZE)) ||
		(replay_op_header.input_size !=
		 (replay_op_header.packet_count * REPLAY_USER_PACKET_SIZE)) ||
		(file_size !=
		 (replay_op_header.input_offset + replay_op_header.input_size)) ||
		(replay_op_header.checkpoint_offset != 0) ||
		(replay_op_header.checkpoint_size != 0) ||
		(replay_op_header.checkpoint_checksum != 0) ||
		(replay_op_header.checkpoint_schema != 0) ||
		((replay_op_header.flags & REPLAY_USER_FLAG_CHECKPOINT) != 0) ||
		(replay_op_header.source_fingerprint != 0) ||
		(replay_op_header.state_digest != 0) ||
		(replay_op_header.stage_reached > STAGE_EXTRA) ||
		!replay_op_start_valid(
			&replay_op_header.start,
			(replay_op_header.mode == RUM_PRACTICE)
		)
	) {
		return false;
	}
	for(i = 0; i < sizeof(replay_op_header.reserved); i++) {
		if(replay_op_header.reserved[i] != 0) {
			return false;
		}
	}
	replay_op_header.header_checksum = 0;
	computed = replay_op_fnv1a(
		REPLAY_FNV1A_BASIS, &replay_op_header, sizeof(replay_op_header)
	);
	replay_op_header.header_checksum = stored;
	return (stored == computed);
}

static bool replay_op_header_read(uint8_t slot)
{
	uint32_t file_size;
	int fh;

	replay_op_paths_init();
	replay_op_slot_set(slot);
	fh = replay_op_dos_open(replay_op_slot_fn);
	if(fh < 0) {
		return false;
	}
	if(
		(replay_op_dos_read(
			fh, &replay_op_header, sizeof(replay_op_header)
		) != sizeof(replay_op_header)) ||
		!replay_op_dos_size(fh, &file_size)
	) {
		replay_op_dos_close(fh);
		return false;
	}
	replay_op_dos_close(fh);
	return replay_op_header_valid(file_size);
}

static bool replay_op_command_write(replay_command_mode_t mode, uint8_t slot)
{
	replay_command_t command;
	int fh;
	bool ok;

	replay_op_paths_init();
	replay_op_memclear(&command, sizeof(command));
	command.magic[0] = 'T'; command.magic[1] = ('0' + GAME);
	command.magic[2] = 'R'; command.magic[3] = 'C';
	command.magic[4] = 'F'; command.magic[5] = 'G';
	command.magic[6] = '2'; command.magic[7] = '\0';
	command.mode = mode;
	command.slot = slot;
	fh = replay_op_dos_create(replay_op_cfg_fn);
	if(fh < 0) {
		return false;
	}
	ok = (
		replay_op_dos_write(fh, &command, sizeof(command)) == sizeof(command)
	);
	replay_op_dos_close(fh);
	return ok;
}

static char *replay_op_word_append(char *p, replay_op_word_t word)
{
	#define P(c) *p++ = c
	switch(word) {
	case ROW_REPLAY:
		P('R'); P('e'); P('p'); P('l'); P('a'); P('y'); break;
	case ROW_VIEW_RECORDED_RUNS:
		P('V'); P('i'); P('e'); P('w'); P(' '); P('r'); P('e'); P('c');
		P('o'); P('r'); P('d'); P('e'); P('d'); P(' '); P('r'); P('u');
		P('n'); P('s'); break;
	case ROW_SLOT:
		P('S'); P('l'); P('o'); P('t'); break;
	case ROW_CHARACTER:
		P('C'); P('h'); P('a'); P('r'); P('a'); P('c'); P('t'); P('e');
		P('r'); break;
	case ROW_SHOT:
		P('S'); P('h'); P('o'); P('t'); break;
	case ROW_RANK:
		P('R'); P('a'); P('n'); P('k'); break;
	case ROW_SCORE:
		P('S'); P('c'); P('o'); P('r'); P('e'); break;
	case ROW_STAGE:
		P('S'); P('t'); P('a'); P('g'); P('e'); break;
	case ROW_NONE:
		P('N'); P('o'); P('n'); P('e'); break;
	case ROW_REIMU:
		P('R'); P('e'); P('i'); P('m'); P('u'); break;
	case ROW_MARISA:
		P('M'); P('a'); P('r'); P('i'); P('s'); P('a'); break;
	case ROW_MIMA:
		P('M'); P('i'); P('m'); P('a'); break;
	case ROW_YUUKA:
		P('Y'); P('u'); P('u'); P('k'); P('a'); break;
	case ROW_EASY:
		P('E'); P('a'); P('s'); P('y'); break;
	case ROW_NORMAL:
		P('N'); P('o'); P('r'); P('m'); P('a'); P('l'); break;
	case ROW_HARD:
		P('H'); P('a'); P('r'); P('d'); break;
	case ROW_LUNATIC:
		P('L'); P('u'); P('n'); P('a'); P('t'); P('i'); P('c'); break;
	case ROW_EXTRA:
		P('E'); P('x'); P('t'); P('r'); P('a'); break;
	case ROW_PAGE:
		P('P'); P('a'); P('g'); P('e'); break;
	case ROW_UP_DOWN_SELECT:
		P('U'); P('p'); P('/'); P('D'); P('o'); P('w'); P('n'); P(' ');
		P('S'); P('e'); P('l'); P('e'); P('c'); P('t'); break;
	case ROW_LEFT_RIGHT_PAGE:
		P('L'); P('e'); P('f'); P('t'); P('/'); P('R'); P('i'); P('g');
		P('h'); P('t'); P(' '); P('P'); P('a'); P('g'); P('e'); break;
	case ROW_Z_PLAY:
		P('Z'); P(' '); P('P'); P('l'); P('a'); P('y'); break;
	case ROW_ESC_BACK:
		P('E'); P('s'); P('c'); P(' '); P('B'); P('a'); P('c'); P('k');
		break;
	}
	#undef P
	return p;
}

static char *replay_op_spaces_append(char *p, unsigned count)
{
	while(count != 0) {
		*p++ = ' ';
		count--;
	}
	return p;
}

static char *replay_op_word_padded_append(
	char *p, replay_op_word_t word, unsigned width
)
{
	char *start = p;
	p = replay_op_word_append(p, word);
	return replay_op_spaces_append(p, width - (p - start));
}

static char *replay_op_uint_append(char *p, uint32_t value, unsigned width)
{
	char digits[10];
	unsigned count = 0;
	unsigned i;

	do {
		digits[count++] = ('0' + static_cast<uint8_t>(value % 10UL));
		value /= 10UL;
	} while(value != 0);
	if(width > count) {
		p = replay_op_spaces_append(p, width - count);
	}
	for(i = count; i != 0; i--) {
		*p++ = digits[i - 1];
	}
	return p;
}

static void replay_op_line_put(screen_x_t left, vram_y_t top, vc2 col, char *p)
{
	*p = '\0';
	graph_putsa_fx(
		left, top, col,
		reinterpret_cast<const shiftjis_t *>(replay_op_line)
	);
}

static replay_op_word_t replay_op_playchar_word(uint8_t playchar)
{
	switch(playchar) {
	case 0: return ROW_REIMU;
	case 1: return ROW_MARISA;
	case 2: return ROW_MIMA;
	default: return ROW_YUUKA;
	}
}

static replay_op_word_t replay_op_rank_word(uint8_t rank)
{
	switch(rank) {
	case RANK_EASY: return ROW_EASY;
	case RANK_NORMAL: return ROW_NORMAL;
	case RANK_HARD: return ROW_HARD;
	case RANK_LUNATIC: return ROW_LUNATIC;
	default: return ROW_EXTRA;
	}
}

void replay_title_label_put(screen_y_t top, vc2 col)
{
	char *p = replay_op_line;
	p = replay_op_word_append(p, ROW_REPLAY);
	replay_op_line_put(((RES_X - (6 * 8)) / 2), top, col, p);
}

void replay_title_desc_put(void)
{
	char *p = replay_op_line;
	p = replay_op_word_append(p, ROW_VIEW_RECORDED_RUNS);
	replay_op_line_put((RES_X - 16 - (18 * 8)), (RES_Y - 16), V_WHITE, p);
}

static void replay_browser_header_put(void)
{
	char *p = replay_op_line;
	p = replay_op_word_padded_append(p, ROW_SLOT, 6);
	p = replay_op_word_padded_append(p, ROW_CHARACTER, 11);
	p = replay_op_word_padded_append(p, ROW_SHOT, 6);
	p = replay_op_word_padded_append(p, ROW_RANK, 10);
	p = replay_op_word_padded_append(p, ROW_SCORE, 12);
	p = replay_op_word_append(p, ROW_STAGE);
	replay_op_line_put(REPLAY_OP_LINE_LEFT, 56, V_WHITE, p);
}

static void replay_browser_slot_put(uint8_t slot, bool selected, vram_y_t top)
{
	char *p = replay_op_line;
	bool valid = replay_op_header_read(slot);

	*p++ = (selected ? '>' : ' ');
	*p++ = ' ';
	p = replay_op_uint_append(p, slot, 2);
	p = replay_op_spaces_append(p, 2);
	if(!valid) {
		p = replay_op_word_append(p, ROW_NONE);
		replay_op_line_put(
			REPLAY_OP_LINE_LEFT, top,
			(selected ? REPLAY_OP_COL_ACTIVE : V_WHITE), p
		);
		return;
	}
	p = replay_op_word_padded_append(
		p, replay_op_playchar_word(replay_op_header.start.playchar), 11
	);
	#if (GAME == 4)
		*p++ = (replay_op_header.start.shottype ? 'B' : 'A');
	#else
		*p++ = '-';
	#endif
	p = replay_op_spaces_append(p, 5);
	p = replay_op_word_padded_append(
		p, replay_op_rank_word(replay_op_header.start.rank), 10
	);
	p = replay_op_uint_append(p, replay_op_header.score_final, 10);
	p = replay_op_spaces_append(p, 2);
	if(replay_op_header.start.stage == STAGE_EXTRA) {
		p = replay_op_word_append(p, ROW_EXTRA);
	} else {
		p = replay_op_word_append(p, ROW_STAGE);
		*p++ = ' ';
		*p++ = static_cast<char>('1' + replay_op_header.start.stage);
	}
	replay_op_line_put(
		REPLAY_OP_LINE_LEFT, top,
		(selected ? REPLAY_OP_COL_ACTIVE : V_WHITE), p
	);
}

static void replay_browser_footer_put(uint8_t sel)
{
	char *p = replay_op_line;
	p = replay_op_word_append(p, ROW_PAGE);
	*p++ = ' ';
	p = replay_op_uint_append(p, ((sel / REPLAY_OP_SLOT_ROWS) + 1), 2);
	*p++ = '/';
	*p++ = '1';
	*p++ = '0';
	p = replay_op_spaces_append(p, 4);
	p = replay_op_word_append(p, ROW_UP_DOWN_SELECT);
	p = replay_op_spaces_append(p, 3);
	p = replay_op_word_append(p, ROW_LEFT_RIGHT_PAGE);
	replay_op_line_put(72, 344, V_WHITE, p);

	p = replay_op_line;
	p = replay_op_word_append(p, ROW_Z_PLAY);
	p = replay_op_spaces_append(p, 5);
	p = replay_op_word_append(p, ROW_ESC_BACK);
	replay_op_line_put(240, 368, V_WHITE, p);
}

static void replay_browser_render(uint8_t sel)
{
	uint8_t page_top = static_cast<uint8_t>(
		(sel / REPLAY_OP_SLOT_ROWS) * REPLAY_OP_SLOT_ROWS
	);
	uint8_t page_drawn = (1 - replay_op_page_shown);
	char *p;
	int i;

	graph_accesspage(page_drawn);
	graph_clear();
	graph_putsa_fx_func = FX_WEIGHT_BOLD;
	graph_putsa_fx_spacing = 0;
	p = replay_op_line;
	p = replay_op_word_append(p, ROW_REPLAY);
	replay_op_line_put(((RES_X - (6 * 8)) / 2), 24, REPLAY_OP_COL_ACTIVE, p);
	replay_browser_header_put();
	graph_putsa_fx_func = FX_WEIGHT_NORMAL;
	for(i = 0; i < REPLAY_OP_SLOT_ROWS; i++) {
		replay_browser_slot_put(
			static_cast<uint8_t>(page_top + i),
			((page_top + i) == sel),
			(REPLAY_OP_LINE_TOP + (i * REPLAY_OP_LINE_H))
		);
	}
	replay_browser_footer_put(sel);
	graph_showpage(page_drawn);
	replay_op_page_shown = page_drawn;
}

bool replay_browser(void)
{
	uint8_t sel = 0;
	bool input_allowed = false;

	palette_black_out(4);
	graph_accesspage(0);
	graph_clear();
	graph_accesspage(1);
	graph_clear();
	graph_showpage(0);
	replay_op_page_shown = 0;
	replay_browser_render(sel);
	palette_100();

	while(1) {
		input_reset_sense_interface();
		if(key_det == INPUT_NONE) {
			input_allowed = true;
		}
		if(input_allowed) {
			if(key_det & INPUT_UP) {
				sel = ((sel == 0) ? 99 : (sel - 1));
				replay_browser_render(sel);
				snd_se_play_force(1);
			} else if(key_det & INPUT_DOWN) {
				sel = ((sel == 99) ? 0 : (sel + 1));
				replay_browser_render(sel);
				snd_se_play_force(1);
			} else if(key_det & INPUT_LEFT) {
				sel = ((sel < 10) ? (sel + 90) : (sel - 10));
				replay_browser_render(sel);
				snd_se_play_force(1);
			} else if(key_det & INPUT_RIGHT) {
				sel = ((sel >= 90) ? (sel - 90) : (sel + 10));
				replay_browser_render(sel);
				snd_se_play_force(1);
			} else if(key_det & INPUT_CANCEL) {
				palette_black_out(4);
				return false;
			} else if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
				if(
					replay_op_header_read(sel) &&
					replay_op_command_write(RCM_PLAYBACK, sel)
				) {
					palette_black_out(4);
					return true;
				}
			}
			if(key_det != INPUT_NONE) {
				input_allowed = false;
			}
		}
		frame_delay(1);
	}
}

void replay_record_next_prepare(void)
{
	uint8_t slot;

	for(slot = 0; slot < REPLAY_USER_SLOT_COUNT; slot++) {
		if(!replay_op_header_read(slot)) {
			replay_op_command_write(RCM_RECORD, slot);
			return;
		}
	}
}

#pragma codeseg
