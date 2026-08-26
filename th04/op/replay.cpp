#pragma option -zCREPLAY_OP_TEXT

// Interim OP-side browser and automatic recorder for the TH04/TH05 compact
// user replay format. All implementation lives in a new segment; the stock
// title menu only calls the narrow entry points in replay.hpp.

#include "platform.h"
#include "x86real.h"
#include <conio.h>
#include <process.h>
#include <string.h>
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "platform/x86real/pc98/keyboard.hpp"
#include "th02/core/initexit.h"
#include "th01/rank.h"
#include "th01/hardware/egc.h"
#include "th03/formats/pi.hpp"
#include "th02/hardware/frmdelay.h"
#include "th02/op/menu.hpp"
#include "th02/op/m_music.hpp"
#include "th02/v_colors.hpp"
#include "th04/common.h"
#include "th04/end/end.h"
#include "th04/formats/cdg.h"
#include "th04/formats/cfg.hpp"
#include "th04/hardware/grppsafx.h"
#include "th04/op/op.hpp"
#include "th04/op/replay.hpp"
#include "th04/replay_format.hpp"
#include "th04/snd/snd.h"
#include "th04/sprites/op_cdg.hpp"
#if (GAME == 5)
	#include "th05/hardware/input.h"
	#include "th05/resident.hpp"
	#include "th05/shiftjis/fns.hpp"
#else
	#include "th04/hardware/input.h"
	#include "th04/resident.hpp"
	#include "th04/shiftjis/fns.hpp"
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
#define REPLAY_OP_COL_LOCKED ((GAME == 5) ? 2 : 12)
#define REPLAY_OP_TEXT_SPACING 16
#define PRACTICE_PAGE_COUNT 3
#define PRACTICE_TARGET_ROWS 9
#define PRACTICE_HISTORY_ROWS 13
#define PRACTICE_STAGE_ROWS 4

enum replay_op_background_t {
	ROB_REPLAY,
	ROB_PRACTICE,
};

enum replay_op_word_t {
	ROW_REPLAY,
	ROW_PRACTICE,
	ROW_PRACTICE_SETUP,
	ROW_CONFIGURE_PRACTICE,
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
};

enum practice_field_t {
	PF_STAGE,
	PF_SECTION,
	PF_LIVES,
	PF_BOMBS,
	PF_POWER,
	PF_DREAM,
	PF_PLAYPERF,
	PF_SCORE,
	PF_CONTINUES,
	PF_EXTENDS,
	PF_START,
	PF_GRAZE,
	PF_STD_FRAMES,
	PF_ITEMS_SPAWNED,
	PF_ITEMS_COLLECTED,
	PF_POINT_ITEMS,
	PF_MAX_POINT_ITEMS,
	PF_ENEMIES_GONE,
	PF_ENEMIES_KILLED,
	PF_MISSES,
	PF_BOMBS_USED,
	PF_STAGE_ITEMS,
	PF_STAGE_GRAZE,
	PF_POWER_OVERFLOW,
};

static char replay_op_cfg_fn[11];
static char replay_op_slot_fn[11];
static char replay_op_main_binary[5];
static char replay_op_debug_binary[4];
static const char *replay_op_main_bg_fn;
static char replay_op_line[REPLAY_OP_LINE_CAPACITY + 1];
static replay_user_header_t replay_op_header;
static bool replay_op_paths_ready;
static uint8_t replay_op_page_shown;

static void replay_op_patch_archive_name_set(char *fn)
{
	fn[0] = 'P'; fn[1] = 'A'; fn[2] = 'T'; fn[3] = 'C'; fn[4] = 'H';
	fn[5] = '0'; fn[6] = ('0' + GAME); fn[7] = '.';
	fn[8] = 'D'; fn[9] = 'A'; fn[10] = 'T'; fn[11] = '\0';
}

static void replay_op_stock_archive_name_set(char *fn)
{
	#if (GAME == 5)
		fn[0] = 0x89; fn[1] = 0xF6; fn[2] = 0xE3; fn[3] = 0x59;
		fn[4] = 0x92; fn[5] = 0x6B; fn[6] = '1'; fn[7] = '.';
		fn[8] = 'd'; fn[9] = 'a'; fn[10] = 't'; fn[11] = '\0';
	#else
		fn[0] = 0x8C; fn[1] = 0xB6; fn[2] = 0x91; fn[3] = 0x7A;
		fn[4] = 0x8B; fn[5] = 0xBD; fn[6] = 'e'; fn[7] = 'd';
		fn[8] = '.'; fn[9] = 'd'; fn[10] = 'a'; fn[11] = 't'; fn[12] = '\0';
	#endif
}

static void replay_op_background_name_set(
	char *fn, replay_op_background_t background
)
{
	fn[0] = ((background == ROB_PRACTICE) ? 's' : 'S');
	fn[1] = ((background == ROB_PRACTICE) ? 'l' : 'L');
	fn[2] = ((background == ROB_PRACTICE) ? 'b' : 'B');
	fn[3] = '1';
	if(background == ROB_PRACTICE) {
		fn[4] = '.'; fn[5] = 'p'; fn[6] = 'i'; fn[7] = '\0';
	} else {
		fn[4] = '.'; fn[5] = 'P'; fn[6] = 'I'; fn[7] = '\0';
	}
}

static bool replay_op_background_load(replay_op_background_t background)
{
	char archive_fn[12];
	char background_fn[9];
	char stock_archive_fn[13];
	bool loaded;

	replay_op_patch_archive_name_set(archive_fn);
	replay_op_background_name_set(background_fn, background);
	replay_op_stock_archive_name_set(stock_archive_fn);
	if(background == ROB_PRACTICE) {
		return (pi_load(0, background_fn) == 0);
	}
	pfend();
	pfstart(reinterpret_cast<const unsigned char *>(archive_fn));
	loaded = (pi_load(0, background_fn) == 0);
	pfend();
	pfstart(reinterpret_cast<const unsigned char *>(stock_archive_fn));
	return loaded;
}

static bool replay_op_screen_begin(
	replay_op_background_t background,
	graph_putsa_fx_func_t& previous_func,
	bool fade_out
)
{
	previous_func = graph_putsa_fx_func;
	graph_putsa_fx_spacing = REPLAY_OP_TEXT_SPACING;
	if(fade_out) {
		palette_black_out(1);
	}
	if(!replay_op_background_load(background)) {
		graph_putsa_fx_func = previous_func;
		return false;
	}
	pi_palette_apply(0);
	palette_settone(0);
	graph_accesspage(0);
	pi_put_8(0, 0, 0);
	graph_accesspage(1);
	pi_put_8(0, 0, 0);
	graph_showpage(0);
	graph_accesspage(0);
	replay_op_page_shown = 0;
	return true;
}

static void replay_op_screen_end(
	graph_putsa_fx_func_t previous_func
)
{
	pi_free(0);
	graph_putsa_fx_func = previous_func;

	// The only native layout is an 8-pixel ANK advance (16 before the renderer
	// halves it). Restoring a stale zero repeats the one-cell menu regression.
	graph_putsa_fx_spacing = REPLAY_OP_TEXT_SPACING;
}

// Keep the accepted replay OP data/BSS addresses stable after removing the
// stale-spacing restore path above. This is deliberately never called.
static void replay_op_layout_pad(void)
{
	_asm {
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
	}
}

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

static void replay_op_dos_delete(const char far *fn)
{
	unsigned fn_seg = REPLAY_OP_FP_SEG(fn);
	unsigned fn_off = REPLAY_OP_FP_OFF(fn);

	_asm {
		push	ds
		mov	dx, fn_off
		mov	ds, fn_seg
		mov	ah, 41h
		int	21h
		pop	ds
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

static bool replay_op_dos_seek(int fh, uint32_t pos)
{
	unsigned pos_hi = static_cast<unsigned>(pos >> 16);
	unsigned pos_lo = static_cast<unsigned>(pos & 0xFFFFUL);
	unsigned failed;

	_asm {
		mov	bx, fh
		mov	cx, pos_hi
		mov	dx, pos_lo
		mov	ax, 4200h
		int	21h
		sbb	ax, ax
		neg	ax
		mov	failed, ax
	}
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

static void replay_op_copy(
	void far *dst, const void far *src, unsigned size
)
{
	uint8_t far *d = reinterpret_cast<uint8_t far *>(dst);
	const uint8_t far *s = reinterpret_cast<const uint8_t far *>(src);

	while(size != 0) {
		*d++ = *s++;
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

	replay_op_main_binary[0] = 'm'; replay_op_main_binary[1] = 'a';
	replay_op_main_binary[2] = 'i'; replay_op_main_binary[3] = 'n';
	replay_op_main_binary[4] = '\0';
	replay_op_debug_binary[0] = 'd'; replay_op_debug_binary[1] = 'e';
	replay_op_debug_binary[2] = 'b'; replay_op_debug_binary[3] = '\0';
	replay_op_paths_ready = true;
}

static void replay_op_exit_into_main(bool fade_out_bgm, bool allow_debug)
{
	replay_op_paths_init();
	replay_op_bridge(ROBF_EXIT_PREPARE);
	#if (GAME == 4)
		gaiji_restore();
	#endif
	if(fade_out_bgm) {
		snd_kaja_func(KAJA_SONG_FADE, 10);
	}
	game_exit();
	if(!allow_debug || !resident->debug) {
		execl(
			replay_op_main_binary, replay_op_main_binary, nullptr
		);
	} else {
		execl(
			replay_op_debug_binary, replay_op_debug_binary, nullptr
		);
	}
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

static bool replay_op_checkpoint_identity_valid(
	const replay_start_config_t far *start
)
{
	if(start->kind == RSK_CHAPTER) {
		if(start->phase != 0) {
			return false;
		}
		#if (GAME == 5)
			return (
				(start->section == RCS_CHAPTER_2) &&
				(start->stage != 5)
			);
		#else
			if(start->stage == 3) {
				return (
					(start->section == RCS_CHAPTER_2) ||
					(start->section == RCS_CHAPTER_3)
				);
			}
			return (
				(start->section == RCS_CHAPTER_2) &&
				((start->stage <= 2) || (start->stage == STAGE_EXTRA))
			);
		#endif
	}
	if(start->kind == RSK_MIDBOSS) {
		if(start->phase != 0) {
			return false;
		}
		#if (GAME == 5)
			return (
				(start->section == RCS_MIDBOSS_PRIMARY) &&
				(start->stage != 5)
			);
		#else
			if(start->stage == 3) {
				return (start->section <= RCS_MIDBOSS_SECONDARY);
			}
			return (
				(start->section == RCS_MIDBOSS_PRIMARY) &&
				((start->stage <= 2) || (start->stage == STAGE_EXTRA))
			);
		#endif
	}
	if(start->kind == RSK_BOSS_PHASE) {
		#if (GAME == 5)
			switch(start->stage) {
			case 0: return ((start->section == 0) && (start->phase <= 4));
			case 1: return ((start->section == 0) && (start->phase <= 7));
			case 2: return ((start->section == 0) && (start->phase <= 14));
			case 3:
				if(start->section == RCS_TH05_PAIR) {
					return (start->phase <= 2);
				}
				return (
					(start->section <= RCS_TH05_YUKI) &&
					(start->phase <= 9)
				);
			case 4: return ((start->section == 0) && (start->phase <= 10));
			case 5: return ((start->section == 0) && (start->phase <= 12));
			case STAGE_EXTRA:
				return ((start->section == 0) && (start->phase <= 17));
			}
		#else
			switch(start->stage) {
			case 0: return ((start->section == 0) && (start->phase <= 5));
			case 1: return ((start->section == 0) && (start->phase <= 6));
			case 2: return ((start->section == 0) && (start->phase <= 4));
			case 3:
				return (
					(start->section == 0) &&
					(start->phase <= ((start->playchar == 0) ? 3 : 12))
				);
			case 4: return ((start->section == 0) && (start->phase <= 18));
			case 5: return ((start->section == 0) && (start->phase <= 17));
			case STAGE_EXTRA:
				if(start->section == RCS_TH04_MUGETSU) {
					return (start->phase <= 7);
				}
				return (
					(start->section == RCS_TH04_GENGETSU) &&
					(start->phase <= 9)
				);
			}
		#endif
	}
	return false;
}

static uint8_t replay_op_playperf_min(uint8_t start_rank)
{
	#if (GAME == 5)
		switch(start_rank) {
		case RANK_EASY: return 16;
		case RANK_NORMAL: return 24;
		case RANK_HARD: return 44;
		case RANK_LUNATIC: return 48;
		default: return 32;
		}
	#else
		switch(start_rank) {
		case RANK_EASY: return 4;
		case RANK_NORMAL: return 11;
		case RANK_HARD: return 20;
		case RANK_LUNATIC: return 22;
		default: return 16;
		}
	#endif
}

static uint8_t replay_op_playperf_max(uint8_t start_rank)
{
	#if (GAME == 5)
		switch(start_rank) {
		case RANK_EASY: return 32;
		case RANK_NORMAL: return 40;
		case RANK_HARD: return 54;
		case RANK_LUNATIC: return 58;
		default: return 36;
		}
	#else
		switch(start_rank) {
		case RANK_EASY: return 16;
		case RANK_NORMAL: return 24;
		case RANK_HARD: return 32;
		case RANK_LUNATIC: return 34;
		default: return 20;
		}
	#endif
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
	return (
		(value >= replay_op_playperf_min(start_rank)) &&
		(value <= replay_op_playperf_max(start_rank))
	);
}

static bool replay_op_start_valid(
	const replay_start_config_t far *start, bool practice, bool checkpoint
)
{
	bool kind_valid = (
		practice
			? (checkpoint
				? replay_op_checkpoint_identity_valid(start)
				: (start->kind == RSK_STAGE))
			: (start->kind == RSK_NATIVE)
	);
	if(
		(start->schema != REPLAY_START_SCHEMA) ||
		!kind_valid ||
		(start->stage > STAGE_EXTRA) ||
		((!checkpoint) && ((start->section != 0) || (start->phase != 0))) ||
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
	uint32_t input_end;
	uint32_t expected_file_size;
	bool checkpoint;
	unsigned i;

	checkpoint = (
		(replay_op_header.flags & REPLAY_USER_FLAG_CHECKPOINT) != 0
	);
	input_end = (
		replay_op_header.input_offset + replay_op_header.input_size
	);
	expected_file_size = input_end;
	if(checkpoint) {
		if(
			(replay_op_header.checkpoint_schema != REPLAY_CHECKPOINT_SCHEMA) ||
			(replay_op_header.checkpoint_offset != input_end) ||
			(replay_op_header.checkpoint_size < REPLAY_CHECKPOINT_HEADER_SIZE) ||
			(replay_op_header.checkpoint_size > REPLAY_CHECKPOINT_SIZE_MAX) ||
			(replay_op_header.checkpoint_checksum == 0) ||
			(replay_op_header.source_fingerprint !=
			 REPLAY_CHECKPOINT_SOURCE_FINGERPRINT) ||
			(replay_op_header.state_digest == 0)
		) {
			return false;
		}
		expected_file_size += replay_op_header.checkpoint_size;
	} else if(
		(replay_op_header.checkpoint_schema != 0) ||
		(replay_op_header.checkpoint_offset != 0) ||
		(replay_op_header.checkpoint_size != 0) ||
		(replay_op_header.checkpoint_checksum != 0) ||
		(replay_op_header.source_fingerprint != 0) ||
		(replay_op_header.state_digest != 0)
	) {
		return false;
	}

	if(
		(replay_op_header.magic[0] != 'T') ||
		(replay_op_header.magic[1] != ('0' + GAME)) ||
		(replay_op_header.magic[2] != 'R') ||
		(replay_op_header.magic[3] != 'P') ||
		(replay_op_header.magic[4] != 'Y') ||
		(replay_op_header.magic[5] != ('0' + REPLAY_USER_VERSION)) ||
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
		(file_size != expected_file_size) ||
		(replay_op_header.stage_reached > STAGE_EXTRA) ||
		!replay_op_start_valid(
			&replay_op_header.start,
			(replay_op_header.mode == RUM_PRACTICE), checkpoint
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

static bool replay_op_extent_checksum(
	int fh, uint32_t offset, uint32_t size, uint32_t expected
)
{
	uint32_t hash = REPLAY_FNV1A_BASIS;
	uint32_t remaining = size;
	uint8_t far *buffer;
	unsigned len;
	bool ok = false;

	buffer = reinterpret_cast<uint8_t far *>(hmem_allocbyte(1024));
	if((buffer == 0) || !replay_op_dos_seek(fh, offset)) {
		if(buffer != 0) {
			hmem_free(reinterpret_cast<void __seg *>(buffer));
		}
		return false;
	}
	while(remaining != 0) {
		len = ((remaining > 1024UL)
			? 1024
			: static_cast<unsigned>(remaining)
		);
		if(replay_op_dos_read(fh, buffer, len) != len) {
			break;
		}
		hash = replay_op_fnv1a(hash, buffer, len);
		remaining -= len;
	}
	if((remaining == 0) && (hash == expected)) {
		ok = true;
	}
	hmem_free(reinterpret_cast<void __seg *>(buffer));
	return ok;
}

static bool replay_op_header_read(uint8_t slot, bool deep)
{
	uint32_t file_size;
	int fh;
	bool valid;

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
	valid = replay_op_header_valid(file_size);
	if(
		valid && deep &&
		((replay_op_header.flags & REPLAY_USER_FLAG_CHECKPOINT) != 0)
	) {
		valid = replay_op_extent_checksum(
			fh,
			replay_op_header.checkpoint_offset,
			replay_op_header.checkpoint_size,
			replay_op_header.checkpoint_checksum
		);
	}
	replay_op_dos_close(fh);
	return valid;
}

static bool replay_op_command_write(
	replay_command_mode_t mode, uint8_t slot, uint8_t flags,
	const replay_start_config_t far *start
)
{
	replay_command_t command;
	int fh;
	bool ok;

	replay_command_clear();
	replay_op_memclear(&command, sizeof(command));
	command.magic[0] = 'T'; command.magic[1] = ('0' + GAME);
	command.magic[2] = 'R'; command.magic[3] = 'C';
	command.magic[4] = 'F'; command.magic[5] = 'G';
	command.magic[6] = '2'; command.magic[7] = '\0';
	command.mode = mode;
	command.slot = slot;
	command.flags = flags;
	if(start != NULL) {
		replay_op_copy(&command.start, start, sizeof(command.start));
	}
	fh = replay_op_dos_create(replay_op_cfg_fn);
	if(fh < 0) {
		return false;
	}
	ok = (
		replay_op_dos_write(fh, &command, sizeof(command)) == sizeof(command)
	);
	replay_op_dos_close(fh);
	if(!ok) {
		replay_command_clear();
	}
	return ok;
}

void replay_command_clear(void)
{
	replay_op_paths_init();
	replay_op_dos_delete(replay_op_cfg_fn);
}

bool replay_private_record_command_start(
	replay_start_config_t far *start
)
{
	replay_command_t command;
	uint32_t file_size;
	int fh;
	unsigned size;
	unsigned i;

	replay_op_paths_init();
	fh = replay_op_dos_open(replay_op_cfg_fn);
	if(fh < 0) {
		return false;
	}
	replay_op_memclear(&command, sizeof(command));
	size = replay_op_dos_read(fh, &command, sizeof(command));
	if(!replay_op_dos_size(fh, &file_size)) {
		file_size = 0;
	}
	replay_op_dos_close(fh);
	if(
		(size != sizeof(command)) || (file_size != sizeof(command)) ||
		(command.magic[0] != 'T') ||
		(command.magic[1] != ('0' + GAME)) ||
		(command.magic[2] != 'R') || (command.magic[3] != 'C') ||
		(command.magic[4] != 'F') || (command.magic[5] != 'G') ||
		(command.magic[6] != '2') || (command.magic[7] != '\0') ||
		(command.mode != RCM_RECORD) ||
		(command.slot >= REPLAY_USER_SLOT_COUNT) ||
		(command.flags != (REPLAY_COMMAND_FLAG_PRACTICE |
		 REPLAY_COMMAND_FLAG_PRIVATE_TEST)) ||
		(command.reserved_0 != 0) ||
		(command.start.kind <= RSK_STAGE) ||
		!replay_op_start_valid(&command.start, true, true)
	) {
		return false;
	}
	for(i = 0; i < sizeof(command.reserved); i++) {
		if(command.reserved[i] != 0) {
			return false;
		}
	}
	replay_op_copy(start, &command.start, sizeof(command.start));
	return true;
}

static char *replay_op_word_append(char *p, replay_op_word_t word)
{
	#define P(c) *p++ = c
	switch(word) {
	case ROW_REPLAY:
		P('R'); P('e'); P('p'); P('l'); P('a'); P('y'); break;
	case ROW_PRACTICE:
		P('P'); P('r'); P('a'); P('c'); P('t'); P('i'); P('c'); P('e');
		break;
	case ROW_PRACTICE_SETUP:
		P('P'); P('r'); P('a'); P('c'); P('t'); P('i'); P('c'); P('e'); P(' ');
		P('S'); P('e'); P('t'); P('u'); P('p'); break;
	case ROW_CONFIGURE_PRACTICE:
		P('C'); P('o'); P('n'); P('f'); P('i'); P('g'); P('u'); P('r'); P('e');
		P(' '); P('a'); P(' '); P('p'); P('r'); P('a'); P('c'); P('t'); P('i');
		P('c'); P('e'); P(' '); P('r'); P('u'); P('n'); break;
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

void replay_practice_title_label_put(screen_y_t top, vc2 col)
{
	char *p = replay_op_line;
	p = replay_op_word_append(p, ROW_PRACTICE);
	replay_op_line_put(((RES_X - (8 * 8)) / 2), top, col, p);
}

void replay_practice_title_desc_put(void)
{
	char *p = replay_op_line;
	p = replay_op_word_append(p, ROW_CONFIGURE_PRACTICE);
	replay_op_line_put((RES_X - 16 - (24 * 8)), (RES_Y - 16), V_WHITE, p);
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
	bool valid = replay_op_header_read(slot, false);

	*p++ = (selected ? '>' : ' ');
	*p++ = ' ';
	p = replay_op_uint_append(p, slot, 2);
	p = replay_op_spaces_append(p, 2);
	if(!valid) {
		p = replay_op_word_append(p, ROW_NONE);
		replay_op_line_put(
			REPLAY_OP_LINE_LEFT, top,
			(selected ? REPLAY_OP_COL_LOCKED : V_WHITE), p
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
	replay_op_line_put(280, 356, V_WHITE, p);
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
	pi_put_8(0, 0, 0);
	graph_putsa_fx_func = FX_WEIGHT_BOLD;
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

static uint8_t practice_row_count(uint8_t page)
{
	if(page == 0) {
		return PRACTICE_TARGET_ROWS;
	}
	return ((page == 1) ? PRACTICE_HISTORY_ROWS : PRACTICE_STAGE_ROWS);
}

static practice_field_t practice_field(uint8_t page, uint8_t sel)
{
	if(page == 0) {
		switch(sel) {
		case 0: return PF_STAGE;
		case 1: return PF_SECTION;
		case 2: return PF_LIVES;
		case 3: return PF_BOMBS;
		case 4: return PF_POWER;
		case 5: return PF_DREAM;
		case 6: return PF_PLAYPERF;
		case 7: return PF_SCORE;
		default: return PF_START;
		}
	}
	if(page == 1) {
		switch(sel) {
		case 0: return PF_CONTINUES;
		case 1: return PF_EXTENDS;
		case 2: return PF_GRAZE;
		case 3: return PF_STD_FRAMES;
		case 4: return PF_ITEMS_SPAWNED;
		case 5: return PF_ITEMS_COLLECTED;
		case 6: return PF_POINT_ITEMS;
		case 7: return PF_MAX_POINT_ITEMS;
		case 8: return PF_ENEMIES_GONE;
		case 9: return PF_ENEMIES_KILLED;
		case 10: return PF_MISSES;
		case 11: return PF_BOMBS_USED;
		default: return PF_START;
		}
	}
	switch(sel) {
	case 0: return PF_STAGE_ITEMS;
	case 1: return PF_STAGE_GRAZE;
	case 2: return PF_POWER_OVERFLOW;
	default: return PF_START;
	}
}

static char *practice_field_append(char *p, practice_field_t field)
{
	#define P(c) *p++ = c
	switch(field) {
	case PF_STAGE:
		P('S'); P('t'); P('a'); P('g'); P('e'); break;
	case PF_SECTION:
		P('S'); P('t'); P('a'); P('r'); P('t'); P(' '); P('P'); P('o'); P('i');
		P('n'); P('t'); break;
	case PF_LIVES:
		P('L'); P('i'); P('f'); P('e'); P(' '); P('S'); P('t'); P('o'); P('c'); P('k');
		break;
	case PF_BOMBS:
		P('B'); P('o'); P('m'); P('b'); P(' '); P('S'); P('t'); P('o'); P('c'); P('k');
		break;
	case PF_POWER:
		P('P'); P('o'); P('w'); P('e'); P('r'); break;
	case PF_DREAM:
		P('D'); P('r'); P('e'); P('a'); P('m'); break;
	case PF_PLAYPERF:
		P('R'); P('a'); P('n'); P('k'); break;
	case PF_SCORE:
		P('S'); P('c'); P('o'); P('r'); P('e'); break;
	case PF_CONTINUES:
		P('C'); P('o'); P('n'); P('t'); P('i'); P('n'); P('u'); P('e'); P('s');
		break;
	case PF_EXTENDS:
		P('E'); P('x'); P('t'); P('e'); P('n'); P('d'); P('s'); break;
	case PF_START:
		P('S'); P('t'); P('a'); P('r'); P('t'); P(' '); P('P'); P('r');
		P('a'); P('c'); P('t'); P('i'); P('c'); P('e'); break;
	case PF_GRAZE:
		P('T'); P('o'); P('t'); P('a'); P('l'); P(' '); P('G'); P('r'); P('a'); P('z'); P('e');
		break;
	case PF_STD_FRAMES:
		P('S'); P('T'); P('D'); P(' '); P('F'); P('r'); P('a'); P('m'); P('e'); P('s');
		break;
	case PF_ITEMS_SPAWNED:
		P('I'); P('t'); P('e'); P('m'); P('s'); P(' '); P('S'); P('p'); P('a');
		P('w'); P('n'); P('e'); P('d'); break;
	case PF_ITEMS_COLLECTED:
		P('I'); P('t'); P('e'); P('m'); P('s'); P(' '); P('C'); P('o'); P('l');
		P('l'); P('e'); P('c'); P('t'); P('e'); P('d'); break;
	case PF_POINT_ITEMS:
		P('P'); P('o'); P('i'); P('n'); P('t'); P(' '); P('I'); P('t'); P('e'); P('m'); P('s');
		break;
	case PF_MAX_POINT_ITEMS:
		P('M'); P('a'); P('x'); P(' '); P('P'); P('o'); P('i'); P('n'); P('t');
		P(' '); P('I'); P('t'); P('e'); P('m'); P('s'); break;
	case PF_ENEMIES_GONE:
		P('E'); P('n'); P('e'); P('m'); P('i'); P('e'); P('s'); P(' '); P('G'); P('o'); P('n'); P('e');
		break;
	case PF_ENEMIES_KILLED:
		P('E'); P('n'); P('e'); P('m'); P('i'); P('e'); P('s'); P(' '); P('K');
		P('i'); P('l'); P('l'); P('e'); P('d'); break;
	case PF_MISSES:
		P('M'); P('i'); P('s'); P('s'); P('e'); P('s'); break;
	case PF_BOMBS_USED:
		P('B'); P('o'); P('m'); P('b'); P('s'); P(' '); P('U'); P('s'); P('e'); P('d');
		break;
	case PF_STAGE_ITEMS:
		P('S'); P('t'); P('a'); P('g'); P('e'); P(' '); P('P'); P('o'); P('i');
		P('n'); P('t'); P(' '); P('I'); P('t'); P('e'); P('m'); P('s'); break;
	case PF_STAGE_GRAZE:
		P('S'); P('t'); P('a'); P('g'); P('e'); P(' '); P('G'); P('r'); P('a'); P('z'); P('e');
		break;
	case PF_POWER_OVERFLOW:
		P('P'); P('o'); P('w'); P('e'); P('r'); P(' '); P('O'); P('v'); P('e');
		P('r'); P('f'); P('l'); P('o'); P('w'); break;
	}
	#undef P
	return p;
}

static uint8_t practice_chapter_count(
	const replay_start_config_t far *start
)
{
#if (GAME == 5)
	return ((start->stage == 5) ? 0 : 1);
#else
	if(start->stage == 3) {
		return 2;
	}
	return (((start->stage <= 2) || (start->stage == STAGE_EXTRA)) ? 1 : 0);
#endif
}

static uint8_t practice_midboss_count(
	const replay_start_config_t far *start
)
{
#if (GAME == 5)
	return ((start->stage == 5) ? 0 : 1);
#else
	if(start->stage == 3) {
		return 2;
	}
	return (((start->stage <= 2) || (start->stage == STAGE_EXTRA)) ? 1 : 0);
#endif
}

static uint8_t practice_boss_section_count(
	const replay_start_config_t far *start
)
{
#if (GAME == 5)
	return ((start->stage == 3) ? 3 : 1);
#else
	return ((start->stage == STAGE_EXTRA) ? 2 : 1);
#endif
}

static uint8_t practice_boss_phase_max(
	const replay_start_config_t far *start, uint8_t section
)
{
#if (GAME == 5)
	switch(start->stage) {
	case 0: return 4;
	case 1: return 7;
	case 2: return 14;
	case 3: return ((section == RCS_TH05_PAIR) ? 2 : 9);
	case 4: return 10;
	case 5: return 12;
	default: return 17;
	}
#else
	switch(start->stage) {
	case 0: return 5;
	case 1: return 6;
	case 2: return 4;
	case 3: return ((start->playchar == 0) ? 3 : 12);
	case 4: return 18;
	case 5: return 17;
	default: return ((section == RCS_TH04_MUGETSU) ? 7 : 9);
	}
#endif
}

static void practice_target_reset(replay_start_config_t far *start)
{
	start->kind = RSK_STAGE;
	start->section = 0;
	start->phase = 0;
}

static uint8_t practice_target_count(
	const replay_start_config_t far *start
)
{
	uint8_t count = static_cast<uint8_t>(
		1 + practice_chapter_count(start) + practice_midboss_count(start)
	);
	uint8_t section;

	for(section = 0; section < practice_boss_section_count(start); section++) {
		count = static_cast<uint8_t>(
			count + practice_boss_phase_max(start, section) + 1
		);
	}
	return count;
}

static uint8_t practice_target_index(
	const replay_start_config_t far *start
)
{
	uint8_t index;
	uint8_t section;
	uint8_t chapters = practice_chapter_count(start);
	uint8_t midbosses = practice_midboss_count(start);

	if(start->kind == RSK_STAGE) {
		return 0;
	}
	if(start->kind == RSK_CHAPTER) {
		if(
			(start->section < RCS_CHAPTER_2) ||
			((start->section - RCS_CHAPTER_2) >= chapters)
		) {
			return 0;
		}
		return static_cast<uint8_t>(
			2 + ((start->section - RCS_CHAPTER_2) * 2)
		);
	}
	if(start->kind == RSK_MIDBOSS) {
		if(start->section >= midbosses) {
			return 0;
		}
		return static_cast<uint8_t>(1 + (start->section * 2));
	}
	if(
		(start->kind != RSK_BOSS_PHASE) ||
		(start->section >= practice_boss_section_count(start)) ||
		(start->phase > practice_boss_phase_max(start, start->section))
	) {
		return 0;
	}
	index = static_cast<uint8_t>(1 + chapters + midbosses);
	for(section = 0; section < start->section; section++) {
		index = static_cast<uint8_t>(
			index + practice_boss_phase_max(start, section) + 1
		);
	}
	return static_cast<uint8_t>(index + start->phase);
}

static void practice_target_set(
	replay_start_config_t far *start, uint8_t index
)
{
	uint8_t count;
	uint8_t section;

	practice_target_reset(start);
	if(index == 0) {
		return;
	}
	index--;
	count = practice_midboss_count(start);
	if(index < (count * 2)) {
		start->section = static_cast<uint8_t>(index / 2);
		if((index & 1) == 0) {
			start->kind = RSK_MIDBOSS;
		} else {
			start->kind = RSK_CHAPTER;
			start->section = static_cast<uint8_t>(
				RCS_CHAPTER_2 + start->section
			);
		}
		return;
	}
	index = static_cast<uint8_t>(index - (count * 2));
	for(section = 0; section < practice_boss_section_count(start); section++) {
		count = static_cast<uint8_t>(
			practice_boss_phase_max(start, section) + 1
		);
		if(index < count) {
			start->kind = RSK_BOSS_PHASE;
			start->section = section;
			start->phase = index;
			return;
		}
		index = static_cast<uint8_t>(index - count);
	}
}

static void practice_target_change(
	replay_start_config_t far *start, bool right, bool fast
)
{
	uint8_t count = practice_target_count(start);
	uint8_t index = practice_target_index(start);
	uint8_t delta = (fast ? 5 : 1);

	delta %= count;
	if(right) {
		index = static_cast<uint8_t>((index + delta) % count);
	} else {
		index = static_cast<uint8_t>(
			(index < delta) ? (count - (delta - index)) : (index - delta)
		);
	}
	practice_target_set(start, index);
}

static char *practice_boss_name_append(
	char *p, const replay_start_config_t far *start
)
{
	#define P(c) *p++ = c
#if (GAME == 5)
	if(start->stage == 3) {
		if(start->section == RCS_TH05_PAIR) {
			P('P'); P('a'); P('i'); P('r');
		} else if(start->section == RCS_TH05_MAI) {
			P('M'); P('a'); P('i');
		} else {
			P('Y'); P('u'); P('k'); P('i');
		}
		return p;
	}
#else
	if(start->stage == STAGE_EXTRA) {
		if(start->section == RCS_TH04_MUGETSU) {
			P('M'); P('u'); P('g'); P('e'); P('t'); P('s'); P('u');
		} else {
			P('G'); P('e'); P('n'); P('g'); P('e'); P('t'); P('s'); P('u');
		}
		return p;
	}
#endif
	P('B'); P('o'); P('s'); P('s');
	#undef P
	return p;
}

static char *practice_target_value_append(
	char *p, const replay_start_config_t far *start
)
{
	#define P(c) *p++ = c
	if(start->kind == RSK_STAGE) {
		P('S'); P('t'); P('a'); P('g'); P('e'); P(' ');
		P('S'); P('t'); P('a'); P('r'); P('t');
		return p;
	}
	if(start->kind == RSK_CHAPTER) {
		P('A'); P('f'); P('t'); P('e'); P('r'); P(' ');
		P('M'); P('i'); P('d'); P('b'); P('o'); P('s'); P('s');
		if(practice_midboss_count(start) > 1) {
			P(' ');
			return replay_op_uint_append(
				p, (start->section - RCS_CHAPTER_2 + 1), 1
			);
		}
		return p;
	}
	if(start->kind == RSK_MIDBOSS) {
		P('M'); P('i'); P('d'); P('b'); P('o'); P('s'); P('s');
		if(practice_midboss_count(start) > 1) {
			P(' ');
			return replay_op_uint_append(p, (start->section + 1), 1);
		}
		return p;
	}
	p = practice_boss_name_append(p, start);
	P(' ');
	if(start->phase == 0) {
		P('S'); P('t'); P('a'); P('r'); P('t');
		return p;
	}
	P('P'); P('h'); P('a'); P('s'); P('e'); P(' ');
	#undef P
	return replay_op_uint_append(p, start->phase, 1);
}

static bool practice_start_valid(const replay_start_config_t far *start)
{
	return replay_op_start_valid(
		start, true, (start->kind > RSK_STAGE)
	);
}

static char *practice_value_append(
	char *p, practice_field_t field, const replay_start_config_t far *start
)
{
	switch(field) {
	case PF_STAGE:
		if(start->stage == STAGE_EXTRA) {
			return replay_op_word_append(p, ROW_EXTRA);
		}
		return replay_op_uint_append(p, (start->stage + 1), 1);
	case PF_SECTION:
		return practice_target_value_append(p, start);
	case PF_LIVES: return replay_op_uint_append(p, start->lives, 1);
	case PF_BOMBS: return replay_op_uint_append(p, start->bombs, 1);
	case PF_POWER: return replay_op_uint_append(p, start->power, 3);
	case PF_DREAM: return replay_op_uint_append(p, start->dream, 3);
	case PF_PLAYPERF: return replay_op_uint_append(p, start->playperf, 2);
	case PF_SCORE: return replay_op_uint_append(p, start->score, 8);
	case PF_CONTINUES: return replay_op_uint_append(p, start->continues_used, 1);
	case PF_EXTENDS: return replay_op_uint_append(p, start->extends_gained, 2);
	case PF_GRAZE: return replay_op_uint_append(p, start->graze, 5);
	case PF_STD_FRAMES: return replay_op_uint_append(p, start->std_frames, 5);
	case PF_ITEMS_SPAWNED: return replay_op_uint_append(p, start->items_spawned, 5);
	case PF_ITEMS_COLLECTED: return replay_op_uint_append(p, start->items_collected, 5);
	case PF_POINT_ITEMS: return replay_op_uint_append(p, start->point_items_collected, 5);
	case PF_MAX_POINT_ITEMS:
		return replay_op_uint_append(p, start->max_valued_point_items_collected, 5);
	case PF_ENEMIES_GONE: return replay_op_uint_append(p, start->enemies_gone, 5);
	case PF_ENEMIES_KILLED: return replay_op_uint_append(p, start->enemies_killed, 5);
	case PF_MISSES: return replay_op_uint_append(p, start->miss_count, 3);
	case PF_BOMBS_USED: return replay_op_uint_append(p, start->bombs_used, 3);
	case PF_STAGE_ITEMS:
		return replay_op_uint_append(p, start->stage_point_items_collected, 3);
	case PF_STAGE_GRAZE: return replay_op_uint_append(p, start->stage_graze, 3);
	case PF_POWER_OVERFLOW:
		return replay_op_uint_append(p, start->power_overflow, 2);
	default: return p;
	}
}

static void practice_defaults(replay_start_config_t far *start)
{
	uint8_t playchar;

	replay_op_memclear(start, sizeof(*start));
	start->schema = REPLAY_START_SCHEMA;
	start->kind = RSK_STAGE;
	#if (GAME == 5)
		playchar = resident->playchar;
		start->playchar = ((playchar <= 3) ? playchar : 0);
	#else
		playchar = static_cast<uint8_t>(resident->playchar_ascii - '0');
		start->playchar = ((playchar <= 1) ? playchar : 0);
		start->shottype = ((resident->shottype <= 1) ? resident->shottype : 0);
	#endif
	start->rank = resident->rank;
	start->lives = resident->cfg_lives;
	start->bombs = resident->cfg_bombs;
	start->power = 1;
	start->dream = ((GAME == 5) ? 1 : 0);
	start->playperf = replay_op_native_playperf(start->rank);
	start->turbo_mode = static_cast<uint8_t>(resident->turbo_mode);
	start->credit_lives = resident->cfg_lives;
	start->credit_bombs = resident->cfg_bombs;
}

static void practice_u8_change(
	uint8_t far *value, uint8_t min, uint8_t max, uint8_t delta, bool right
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

static void practice_u16_change(
	uint16_t far *value, uint16_t min, uint16_t max, uint16_t delta,
	bool right
)
{
	if(right) {
		*value = ((*value > (max - delta))
			? min
			: static_cast<uint16_t>(*value + delta)
		);
	} else {
		*value = ((*value < (min + delta))
			? max
			: static_cast<uint16_t>(*value - delta)
		);
	}
}

static void practice_score_change(
	uint32_t far *value, uint32_t delta, bool right
)
{
	const uint32_t max = 99999990UL;
	if(right) {
		if(*value == max) {
			*value = 0;
		} else if(*value > (max - delta)) {
			*value = max;
		} else {
			*value += delta;
		}
	} else {
		if(*value == 0) {
			*value = max;
		} else if(*value < delta) {
			*value = 0;
		} else {
			*value -= delta;
		}
	}
}

static void practice_field_change(
	replay_start_config_t far *start, practice_field_t field, bool right,
	bool fast
)
{
	switch(field) {
	case PF_STAGE:
		practice_u8_change(
			&start->stage, 0,
			static_cast<uint8_t>(extra_unlocked ? STAGE_EXTRA : (STAGE_EXTRA - 1)),
			1, right
		);
		if(start->stage == STAGE_EXTRA) {
			start->rank = RANK_EXTRA;
			start->turbo_mode = 1;
		} else if(start->rank == RANK_EXTRA) {
			start->rank = resident->rank;
			start->turbo_mode = static_cast<uint8_t>(resident->turbo_mode);
		}
		start->playperf = replay_op_native_playperf(start->rank);
		practice_target_reset(start);
		break;
	case PF_SECTION:
		practice_target_change(start, right, fast);
		break;
	case PF_LIVES:
		practice_u8_change(&start->lives, 1, CFG_LIVES_MAX, 1, right);
		start->credit_lives = start->lives;
		break;
	case PF_BOMBS:
		practice_u8_change(&start->bombs, 0, CFG_BOMBS_MAX, 1, right);
		start->credit_bombs = start->bombs;
		break;
	case PF_POWER:
		practice_u8_change(&start->power, 1, 128, (fast ? 16 : 1), right);
		break;
	case PF_DREAM:
		practice_u8_change(
			&start->dream, 0, ((GAME == 5) ? 128 : 7),
			(fast ? ((GAME == 5) ? 16 : 2) : 1), right
		);
		break;
	case PF_PLAYPERF:
		practice_u8_change(
			&start->playperf, replay_op_playperf_min(start->rank),
			replay_op_playperf_max(start->rank), (fast ? 4 : 1), right
		);
		break;
	case PF_SCORE:
		practice_score_change(&start->score, (fast ? 1000000UL : 10UL), right);
		break;
	case PF_CONTINUES:
		practice_u8_change(&start->continues_used, 0, 9, 1, right); break;
	case PF_EXTENDS:
		practice_u8_change(&start->extends_gained, 0, 10, 1, right); break;
	case PF_GRAZE:
		practice_u16_change(&start->graze, 0, 65535, (fast ? 100 : 1), right); break;
	case PF_STD_FRAMES:
		practice_u16_change(&start->std_frames, 0, 65535, (fast ? 1000 : 1), right); break;
	case PF_ITEMS_SPAWNED:
		practice_u16_change(&start->items_spawned, 0, 65535, (fast ? 100 : 1), right); break;
	case PF_ITEMS_COLLECTED:
		practice_u16_change(&start->items_collected, 0, 65535, (fast ? 100 : 1), right); break;
	case PF_POINT_ITEMS:
		practice_u16_change(
			&start->point_items_collected, 0, 65535, (fast ? 100 : 1), right
		); break;
	case PF_MAX_POINT_ITEMS:
		practice_u16_change(
			&start->max_valued_point_items_collected, 0, 65535,
			(fast ? 100 : 1), right
		); break;
	case PF_ENEMIES_GONE:
		practice_u16_change(&start->enemies_gone, 0, 65535, (fast ? 100 : 1), right); break;
	case PF_ENEMIES_KILLED:
		practice_u16_change(&start->enemies_killed, 0, 65535, (fast ? 100 : 1), right); break;
	case PF_MISSES:
		practice_u8_change(&start->miss_count, 0, 255, (fast ? 10 : 1), right); break;
	case PF_BOMBS_USED:
		practice_u8_change(&start->bombs_used, 0, 255, (fast ? 10 : 1), right); break;
	case PF_STAGE_ITEMS:
		practice_u16_change(
			&start->stage_point_items_collected, 0, ((GAME == 5) ? 999 : 255),
			(fast ? 10 : 1), right
		); break;
	case PF_STAGE_GRAZE:
		practice_u16_change(&start->stage_graze, 0, 999, (fast ? 10 : 1), right); break;
	case PF_POWER_OVERFLOW:
		practice_u16_change(&start->power_overflow, 0, 42, (fast ? 5 : 1), right); break;
	default: break;
	}
}

static bool practice_field_is_numeric(practice_field_t field)
{
	return (
		(field != PF_STAGE) && (field != PF_SECTION) && (field != PF_START)
	);
}

static uint32_t practice_field_numeric_get(
	const replay_start_config_t far *start, practice_field_t field
)
{
	switch(field) {
	case PF_LIVES: return start->lives;
	case PF_BOMBS: return start->bombs;
	case PF_POWER: return start->power;
	case PF_DREAM: return start->dream;
	case PF_PLAYPERF: return start->playperf;
	case PF_SCORE: return start->score;
	case PF_CONTINUES: return start->continues_used;
	case PF_EXTENDS: return start->extends_gained;
	case PF_GRAZE: return start->graze;
	case PF_STD_FRAMES: return start->std_frames;
	case PF_ITEMS_SPAWNED: return start->items_spawned;
	case PF_ITEMS_COLLECTED: return start->items_collected;
	case PF_POINT_ITEMS: return start->point_items_collected;
	case PF_MAX_POINT_ITEMS: return start->max_valued_point_items_collected;
	case PF_ENEMIES_GONE: return start->enemies_gone;
	case PF_ENEMIES_KILLED: return start->enemies_killed;
	case PF_MISSES: return start->miss_count;
	case PF_BOMBS_USED: return start->bombs_used;
	case PF_STAGE_ITEMS: return start->stage_point_items_collected;
	case PF_STAGE_GRAZE: return start->stage_graze;
	case PF_POWER_OVERFLOW: return start->power_overflow;
	default: return 0;
	}
}

static uint32_t practice_field_numeric_min(
	const replay_start_config_t far *start, practice_field_t field
)
{
	if(field == PF_POWER) {
		return 1;
	}
	if(field == PF_LIVES) {
		return 1;
	}
	if(field == PF_PLAYPERF) {
		return replay_op_playperf_min(start->rank);
	}
	return 0;
}

static uint32_t practice_field_numeric_max(
	const replay_start_config_t far *start, practice_field_t field
)
{
	switch(field) {
	case PF_LIVES: return CFG_LIVES_MAX;
	case PF_BOMBS: return CFG_BOMBS_MAX;
	case PF_CONTINUES: return 9;
	case PF_POWER: return 128;
	case PF_DREAM: return ((GAME == 5) ? 128 : 7);
	case PF_PLAYPERF: return replay_op_playperf_max(start->rank);
	case PF_SCORE: return 99999990UL;
	case PF_EXTENDS: return 10;
	case PF_GRAZE:
	case PF_STD_FRAMES:
	case PF_ITEMS_SPAWNED:
	case PF_ITEMS_COLLECTED:
	case PF_POINT_ITEMS:
	case PF_MAX_POINT_ITEMS:
	case PF_ENEMIES_GONE:
	case PF_ENEMIES_KILLED: return 65535UL;
	case PF_MISSES:
	case PF_BOMBS_USED: return 255;
	case PF_STAGE_ITEMS: return ((GAME == 5) ? 999 : 255);
	case PF_STAGE_GRAZE: return 999;
	case PF_POWER_OVERFLOW: return 42;
	default: return 0;
	}
}

static void practice_field_numeric_set(
	replay_start_config_t far *start, practice_field_t field, uint32_t value
)
{
	switch(field) {
	case PF_LIVES:
		start->lives = static_cast<uint8_t>(value);
		start->credit_lives = start->lives;
		break;
	case PF_BOMBS:
		start->bombs = static_cast<uint8_t>(value);
		start->credit_bombs = start->bombs;
		break;
	case PF_POWER: start->power = static_cast<uint8_t>(value); break;
	case PF_DREAM: start->dream = static_cast<uint8_t>(value); break;
	case PF_PLAYPERF: start->playperf = static_cast<uint8_t>(value); break;
	case PF_SCORE: start->score = value; break;
	case PF_CONTINUES:
		start->continues_used = static_cast<uint8_t>(value); break;
	case PF_EXTENDS:
		start->extends_gained = static_cast<uint8_t>(value); break;
	case PF_GRAZE: start->graze = static_cast<uint16_t>(value); break;
	case PF_STD_FRAMES: start->std_frames = static_cast<uint16_t>(value); break;
	case PF_ITEMS_SPAWNED:
		start->items_spawned = static_cast<uint16_t>(value); break;
	case PF_ITEMS_COLLECTED:
		start->items_collected = static_cast<uint16_t>(value); break;
	case PF_POINT_ITEMS:
		start->point_items_collected = static_cast<uint16_t>(value); break;
	case PF_MAX_POINT_ITEMS:
		start->max_valued_point_items_collected = static_cast<uint16_t>(value);
		break;
	case PF_ENEMIES_GONE:
		start->enemies_gone = static_cast<uint16_t>(value); break;
	case PF_ENEMIES_KILLED:
		start->enemies_killed = static_cast<uint16_t>(value); break;
	case PF_MISSES: start->miss_count = static_cast<uint8_t>(value); break;
	case PF_BOMBS_USED: start->bombs_used = static_cast<uint8_t>(value); break;
	case PF_STAGE_ITEMS:
		start->stage_point_items_collected = static_cast<uint16_t>(value); break;
	case PF_STAGE_GRAZE:
		start->stage_graze = static_cast<uint16_t>(value); break;
	case PF_POWER_OVERFLOW:
		start->power_overflow = static_cast<uint16_t>(value); break;
	default: break;
	}
}

static int practice_digit_edge(
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

static void practice_render(
	const replay_start_config_t far *start, uint8_t page, uint8_t sel
);

static void practice_numeric_entry(
	replay_start_config_t far *start, practice_field_t field,
	uint8_t page, uint8_t sel
)
{
	uint32_t original = practice_field_numeric_get(start, field);
	uint32_t value = 0;
	uint32_t min = practice_field_numeric_min(start, field);
	uint32_t max = practice_field_numeric_max(start, field);
	uint8_t now0;
	uint8_t now1;
	uint8_t now3;
	uint8_t prev0;
	uint8_t prev1;
	uint8_t prev3;
	int digit;
	bool entered = false;

	// The Enter that opened this editor must be released before it can commit.
	do {
		prev3 = peekb(0, KEYGROUP_3);
		resident->rand++;
		frame_delay(1);
	} while(prev3 & K3_RETURN);
	prev0 = peekb(0, KEYGROUP_0);
	prev1 = peekb(0, KEYGROUP_1);
	while(1) {
		now0 = peekb(0, KEYGROUP_0);
		now1 = peekb(0, KEYGROUP_1);
		now3 = peekb(0, KEYGROUP_3);
		if((now0 & K0_ESC) && !(prev0 & K0_ESC)) {
			practice_field_numeric_set(start, field, original);
			practice_render(start, page, sel);
			return;
		}
		if((now3 & K3_RETURN) && !(prev3 & K3_RETURN)) {
			if(entered) {
				if(value < min) {
					value = min;
				}
				practice_field_numeric_set(start, field, value);
			}
			practice_render(start, page, sel);
			return;
		}
		if((now1 & K1_BACKSPACE) && !(prev1 & K1_BACKSPACE)) {
			value /= 10UL;
			practice_field_numeric_set(start, field, ((value < min) ? min : value));
			entered = true;
			practice_render(start, page, sel);
		} else {
			digit = practice_digit_edge(now0, prev0, now1, prev1);
			if(digit >= 0) {
				if(value > ((max - digit) / 10UL)) {
					value = max;
				} else {
					value = ((value * 10UL) + digit);
				}
				practice_field_numeric_set(
					start, field, ((value < min) ? min : value)
				);
				entered = true;
				practice_render(start, page, sel);
			}
		}
		prev0 = now0;
		prev1 = now1;
		prev3 = now3;
		resident->rand++;
		frame_delay(1);
	}
}

static void practice_page_name_put(uint8_t page)
{
	char *p = replay_op_line;
	screen_x_t left;
	#define P(c) *p++ = c
	P('<'); P(' ');
	if(page == 0) {
		P('T'); P('a'); P('r'); P('g'); P('e'); P('t'); P(' ');
		P('S'); P('e'); P('t'); P('t'); P('i'); P('n'); P('g'); P('s');
	} else if(page == 1) {
		P('R'); P('u'); P('n'); P(' '); P('H'); P('i'); P('s'); P('t');
		P('o'); P('r'); P('y');
	} else {
		P('S'); P('t'); P('a'); P('g'); P('e'); P(' '); P('H'); P('i');
		P('s'); P('t'); P('o'); P('r'); P('y');
	}
	P(' '); P('('); P('1' + page); P('/'); P('3'); P(')'); P(' '); P('>');
	left = static_cast<screen_x_t>(
		(RES_X - ((p - replay_op_line) * 8)) / 2
	);
	#undef P
	replay_op_line_put(left, 44, V_WHITE, p);
}

static void practice_render(
	const replay_start_config_t far *start, uint8_t page, uint8_t sel
)
{
	uint8_t rows = practice_row_count(page);
	uint8_t page_drawn = (1 - replay_op_page_shown);
	practice_field_t field;
	char *p;
	int i;

	graph_accesspage(page_drawn);
	pi_put_8(0, 0, 0);
	graph_putsa_fx_func = FX_WEIGHT_BOLD;
	p = replay_op_line;
	p = replay_op_word_append(p, ROW_PRACTICE_SETUP);
	replay_op_line_put(((RES_X - (14 * 8)) / 2), 16, REPLAY_OP_COL_ACTIVE, p);
	graph_putsa_fx_func = FX_WEIGHT_NORMAL;
	practice_page_name_put(page);
	for(i = 0; i < rows; i++) {
		field = practice_field(page, i);
		p = replay_op_line;
		*p++ = ((i == sel) ? '>' : ' ');
		replay_op_line_put(80, (68 + (i * 20)),
			((i == sel) ? REPLAY_OP_COL_ACTIVE : V_WHITE), p);
		p = replay_op_line;
		p = practice_field_append(p, field);
		if(field == PF_START) {
			replay_op_line_put(
				((RES_X - (14 * 8)) / 2), (68 + (i * 20)),
				((i == sel) ? REPLAY_OP_COL_ACTIVE : V_WHITE), p
			);
		} else {
			replay_op_line_put(104, (68 + (i * 20)),
				((i == sel) ? REPLAY_OP_COL_ACTIVE : V_WHITE), p);
			p = replay_op_line;
			p = practice_value_append(p, field, start);
			replay_op_line_put(384, (68 + (i * 20)),
				((i == sel) ? REPLAY_OP_COL_ACTIVE : V_WHITE), p);
		}
	}
	graph_showpage(page_drawn);
	replay_op_page_shown = page_drawn;
}

bool replay_practice_setup(replay_start_config_t far *start)
{
	uint8_t page = 0;
	uint8_t sel = 0;
	uint8_t rows;
	practice_field_t field;
	bool input_allowed = false;
	uint8_t horizontal_hold = 0;
	bool horizontal_trigger;
	bool right;
	graph_putsa_fx_func_t previous_func;

	practice_defaults(start);
	// The native character-selection screen already faded to black.
	if(!replay_op_screen_begin(ROB_PRACTICE, previous_func, false)) {
		return false;
	}
	practice_render(start, page, sel);
	palette_black_in(1);
	while(1) {
		input_reset_sense_interface();
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
		rows = practice_row_count(page);
		field = practice_field(page, sel);
		if(horizontal_trigger) {
			if(field == PF_START) {
				page = right
					? ((page == (PRACTICE_PAGE_COUNT - 1)) ? 0 : (page + 1))
					: ((page == 0) ? (PRACTICE_PAGE_COUNT - 1) : (page - 1));
				rows = practice_row_count(page);
				if(sel >= rows) {
					sel = (rows - 1);
				}
			} else {
				practice_field_change(start, field, right, shiftkey);
			}
			practice_render(start, page, sel);
			if(input_allowed) {
				snd_se_play_force(1);
			}
			input_allowed = false;
		} else if(input_allowed) {
			if(key_det & INPUT_UP) {
				if(sel == 0) {
					page = ((page == 0) ? (PRACTICE_PAGE_COUNT - 1) : (page - 1));
					sel = (practice_row_count(page) - 1);
				} else {
					sel--;
				}
				practice_render(start, page, sel);
				snd_se_play_force(1);
			} else if(key_det & INPUT_DOWN) {
				if(sel == (rows - 1)) {
					page = ((page == (PRACTICE_PAGE_COUNT - 1)) ? 0 : (page + 1));
					sel = 0;
				} else {
					sel++;
				}
				practice_render(start, page, sel);
				snd_se_play_force(1);
			} else if(key_det & INPUT_BOMB) {
				page = ((page == (PRACTICE_PAGE_COUNT - 1)) ? 0 : (page + 1));
				rows = practice_row_count(page);
				if(sel >= rows) {
					sel = (rows - 1);
				}
				practice_render(start, page, sel);
				snd_se_play_force(1);
			} else if(key_det & INPUT_CANCEL) {
				palette_black_out(1);
				replay_op_screen_end(previous_func);
				return false;
			} else if((key_det & INPUT_OK) && practice_field_is_numeric(field)) {
				practice_numeric_entry(start, field, page, sel);
			} else if((key_det & (INPUT_SHOT | INPUT_OK)) && (field == PF_START)) {
				if(practice_start_valid(start)) {
					palette_black_out(1);
					replay_op_screen_end(previous_func);
					return true;
				}
			}
			if(key_det != INPUT_NONE) {
				input_allowed = false;
			}
		}
		resident->rand++;
		frame_delay(1);
	}
}

bool replay_practice_record_prepare(
	const replay_start_config_t far *start_in
)
{
	replay_start_config_t start;
	uint8_t slot;

	replay_command_clear();
	replay_op_copy(&start, start_in, sizeof(start));
	start.resident_rand = resident->rand;
	start.random_seed = resident->rand;
	if(!practice_start_valid(&start)) {
		return false;
	}
	for(slot = 0; slot < REPLAY_USER_SLOT_COUNT; slot++) {
		if(!replay_op_header_read(slot, false)) {
			return replay_op_command_write(
				RCM_RECORD, slot, REPLAY_COMMAND_FLAG_PRACTICE, &start
			);
		}
	}
	return replay_op_command_write(
		RCM_RECORD, 0,
		(REPLAY_COMMAND_FLAG_PRACTICE | REPLAY_COMMAND_FLAG_NO_RECORD),
		&start
	);
}

bool replay_browser(void)
{
	uint8_t sel = 0;
	bool input_allowed = false;
	graph_putsa_fx_func_t previous_func;

	if(!replay_op_screen_begin(ROB_REPLAY, previous_func, true)) {
		return false;
	}
	replay_browser_render(sel);
	palette_black_in(1);

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
				palette_black_out(1);
				replay_op_screen_end(previous_func);
				return false;
			} else if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
				if(
					replay_op_header_read(sel, true) &&
					replay_op_command_write(RCM_PLAYBACK, sel, 0, NULL)
				) {
					palette_black_out(1);
					replay_op_screen_end(previous_func);
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

	replay_command_clear();
	for(slot = 0; slot < REPLAY_USER_SLOT_COUNT; slot++) {
		if(!replay_op_header_read(slot, false)) {
			replay_op_command_write(RCM_RECORD, slot, 0, NULL);
			return;
		}
	}
}

// Title integration
// -----------------
// OP_MAIN_TEXT still contains every native function at its stock offset. Its
// main-menu updater is a same-span trampoline into this segment so the two new
// choices cannot shift OP_SETUP_TEXT or any later native OP segment.

enum replay_main_choice_t {
	RMC_GAME,
	RMC_EXTRA,
	RMC_PRACTICE,
	RMC_REGIST_VIEW,
	RMC_MUSICROOM,
	RMC_REPLAY,
	RMC_OPTION,
	RMC_QUIT,
	RMC_COUNT,
};

#define REPLAY_MAIN_LABEL_W 96
#define REPLAY_MAIN_LABEL_H 16
#define REPLAY_MAIN_CURSOR_W 32
#define REPLAY_MAIN_TOP ((GAME == 5) ? 210 : 214)
#define REPLAY_MAIN_COMMAND_LEFT ((RES_X / 2) - (REPLAY_MAIN_LABEL_W / 2))
#define REPLAY_MAIN_COMMAND_H (REPLAY_MAIN_LABEL_H + 4)
#define REPLAY_MAIN_CURSOR_LEFT ( \
	REPLAY_MAIN_COMMAND_LEFT - (REPLAY_MAIN_CURSOR_W / 2) \
)
#define REPLAY_MAIN_CURSOR_RIGHT ( \
	(REPLAY_MAIN_COMMAND_LEFT + REPLAY_MAIN_LABEL_W) - \
	(REPLAY_MAIN_CURSOR_W / 2) \
)
#define REPLAY_MAIN_MENU_LEFT REPLAY_MAIN_CURSOR_LEFT
#define REPLAY_MAIN_MENU_W ( \
	REPLAY_MAIN_CURSOR_RIGHT + REPLAY_MAIN_CURSOR_W - REPLAY_MAIN_MENU_LEFT \
)
#define REPLAY_MAIN_DESC_TOP (RES_Y - GLYPH_H)

extern int8_t menu_sel;
extern bool quit;
extern int8_t main_menu_unused_1;
extern const shiftjis_t* MENU_DESC[];
extern int8_t in_option;
extern menu_unput_and_put_func_t menu_unput_and_put;

static bool replay_main_initialized;
static bool replay_main_input_allowed;
static bool replay_main_private_checked;
static bool replay_main_returning_from_option;

static screen_y_t replay_main_choice_top(int sel)
{
	return static_cast<screen_y_t>(
		REPLAY_MAIN_TOP + (sel * REPLAY_MAIN_COMMAND_H)
	);
}

static void replay_main_desc_put(int desc_id)
{
	egc_copy_rect_1_to_0_16(0, REPLAY_MAIN_DESC_TOP, RES_X, GLYPH_H);
	graph_putsa_fx_func = FX_WEIGHT_BOLD;
	graph_putsa_fx(
		(RES_X - GLYPH_FULL_W - (strlen(MENU_DESC[desc_id]) * GLYPH_HALF_W)),
		REPLAY_MAIN_DESC_TOP, ((GAME == 5) ? 9 : V_WHITE),
		MENU_DESC[desc_id]
	);
}

static void replay_main_command_put(screen_y_t top, op_cdg_slot_t slot)
{
	cdg_put_nocolors_8(REPLAY_MAIN_COMMAND_LEFT, top, slot);
}

static void pascal near replay_main_unput_and_put(int sel, vc2 col)
{
	screen_y_t top = replay_main_choice_top(sel);
	int desc_id = sel;
	int custom = 0;

	egc_copy_rect_1_to_0_16(
		REPLAY_MAIN_MENU_LEFT, top, REPLAY_MAIN_MENU_W, REPLAY_MAIN_LABEL_H
	);
	grcg_setcolor(GC_RMW, col);
	switch(sel) {
	case RMC_GAME:
		replay_main_command_put(top, CDG_MAIN_GAME);
		desc_id = (22 + resident->rank);
		break;
	case RMC_EXTRA:
		if(!extra_unlocked) {
			grcg_setcolor(GC_RMW, ((GAME == 5) ? 2 : 12));
		}
		replay_main_command_put(top, CDG_MAIN_EXTRA);
		break;
	case RMC_PRACTICE:
		custom = 1;
		break;
	case RMC_REGIST_VIEW:
		replay_main_command_put(top, CDG_MAIN_REGIST_VIEW);
		desc_id = 2;
		break;
	case RMC_MUSICROOM:
		replay_main_command_put(top, CDG_MAIN_MUSICROOM);
		desc_id = 3;
		break;
	case RMC_REPLAY:
		custom = 2;
		break;
	case RMC_OPTION:
		replay_main_command_put(top, CDG_MAIN_OPTION);
		desc_id = 4;
		break;
	case RMC_QUIT:
		replay_main_command_put(top, CDG_QUIT);
		desc_id = 5;
		break;
	}
	grcg_off();
	if(custom == 1) {
		replay_practice_title_label_put(top, col);
	} else if(custom == 2) {
		replay_title_label_put(top, col);
	}

	if(col == ((GAME == 5) ? 14 : 8)) {
		cdg_put_8(REPLAY_MAIN_CURSOR_LEFT, top, CDG_CURSOR_LEFT);
		cdg_put_8(REPLAY_MAIN_CURSOR_RIGHT, top, CDG_CURSOR_RIGHT);
		if(custom == 1) {
			egc_copy_rect_1_to_0_16(0, REPLAY_MAIN_DESC_TOP, RES_X, GLYPH_H);
			replay_practice_title_desc_put();
		} else if(custom == 2) {
			egc_copy_rect_1_to_0_16(0, REPLAY_MAIN_DESC_TOP, RES_X, GLYPH_H);
			replay_title_desc_put();
		} else {
			replay_main_desc_put(desc_id);
		}
	}
}

static void replay_main_selection_move(int8_t direction)
{
	replay_main_unput_and_put(menu_sel, ((GAME == 5) ? 8 : 1));
	menu_sel += direction;
	if(menu_sel < 0) {
		menu_sel = (RMC_COUNT - 1);
	}
	if(menu_sel >= RMC_COUNT) {
		menu_sel = 0;
	}
	if(!extra_unlocked && (menu_sel == RMC_EXTRA)) {
		menu_sel += direction;
	}
	replay_main_unput_and_put(menu_sel, ((GAME == 5) ? 14 : 8));
	snd_se_play_force(1);
}

static void replay_main_return(int sel)
{
	replay_op_paths_init();
	graph_accesspage(1);
	pi_fullres_load_palette_apply_put_free(0, replay_op_main_bg_fn);
	graph_copy_page(0);
	graph_showpage(0);
	graph_accesspage(0);
	graph_putsa_fx_func = FX_WEIGHT_NORMAL;
	graph_putsa_fx_spacing = REPLAY_OP_TEXT_SPACING;
	palette_100();
	replay_main_initialized = false;
	in_option = false;
	menu_sel = sel;
}

#if (GAME == 5)
static void replay_main_th05_scores_reset(void)
{
	int digit;
	int stage;
	for(digit = 0; digit < SCORE_DIGITS; digit++) {
		resident->score_last.digits[digit] = 0;
		resident->score_highest.digits[digit] = 0;
		for(stage = 0; stage < MAIN_STAGE_COUNT; stage++) {
			resident->stage_score[stage].digits[digit] = 0;
		}
	}
}
#endif

static void replay_main_start_game(void)
{
	#if (GAME == 5)
		resident->end_sequence = ES_SCORE;
		resident->demo_num = 0;
		resident->stage = 0;
		resident->credit_lives = resident->cfg_lives;
		resident->credit_bombs = resident->cfg_bombs;
	#else
		resident->stage = 0;
		resident->credit_lives = resident->cfg_lives;
		resident->credit_bombs = resident->cfg_bombs;
		resident->playchar_ascii = ('0' + PLAYCHAR_REIMU);
		resident->stage_ascii = ('0' + 0);
	#endif
	if(replay_op_bridge(ROBF_PLAYCHAR_MENU)) {
		return;
	}
	#if (GAME == 5)
		replay_main_th05_scores_reset();
	#else
		resident->demo_num = 0;
	#endif
	if(!resident->debug) {
		replay_record_next_prepare();
	} else {
		replay_command_clear();
	}
	replay_op_exit_into_main(true, true);
}

static void replay_main_start_extra(void)
{
	#if (GAME == 5)
		resident->demo_num = 0;
		resident->stage = STAGE_EXTRA;
		resident->credit_lives = 3;
		resident->credit_bombs = 3;
	#else
		resident->stage = STAGE_EXTRA;
		resident->credit_lives = 3;
		resident->credit_bombs = 2;
		resident->playchar_ascii = ('0' + PLAYCHAR_REIMU);
		resident->stage_ascii = ('0' + STAGE_EXTRA);
	#endif
	if(replay_op_bridge(ROBF_PLAYCHAR_MENU)) {
		return;
	}
	#if (GAME == 5)
		replay_main_th05_scores_reset();
	#else
		resident->demo_num = 0;
	#endif
	if(!resident->debug) {
		replay_record_next_prepare();
	} else {
		replay_command_clear();
	}
	replay_op_exit_into_main(true, false);
}

static void replay_main_start_practice_apply(
	const replay_start_config_t far *start, bool prepare_record
)
{
	#if (GAME == 5)
		resident->end_sequence = ES_SCORE;
		resident->demo_num = 0;
		resident->stage = start->stage;
		resident->credit_lives = start->credit_lives;
		resident->credit_bombs = start->credit_bombs;
		resident->playchar = start->playchar;
		replay_main_th05_scores_reset();
	#else
		resident->stage = start->stage;
		resident->credit_lives = start->credit_lives;
		resident->credit_bombs = start->credit_bombs;
		resident->playchar_ascii = ('0' + start->playchar);
		resident->stage_ascii = ('0' + start->stage);
		resident->shottype = start->shottype;
		resident->demo_num = 0;
	#endif
	if(prepare_record && !replay_practice_record_prepare(start)) {
		return;
	}
	replay_op_exit_into_main(true, false);
}

static void replay_main_start_practice(void)
{
	replay_start_config_t start;

	#if (GAME == 5)
		resident->end_sequence = ES_SCORE;
		resident->demo_num = 0;
		resident->stage = 0;
		resident->credit_lives = resident->cfg_lives;
		resident->credit_bombs = resident->cfg_bombs;
	#else
		resident->stage = 0;
		resident->credit_lives = resident->cfg_lives;
		resident->credit_bombs = resident->cfg_bombs;
		resident->playchar_ascii = ('0' + PLAYCHAR_REIMU);
		resident->stage_ascii = ('0' + 0);
	#endif
	if(replay_op_bridge(ROBF_PLAYCHAR_MENU)) {
		return;
	}
	if(!replay_practice_setup(&start)) {
		return;
	}
	replay_main_start_practice_apply(&start, true);
}

static void replay_main_initialize(void)
{
	int i;

	main_menu_unused_1 = 0;
	egc_copy_rect_1_to_0_16(
		(REPLAY_MAIN_MENU_LEFT - 128), REPLAY_MAIN_TOP,
		(REPLAY_MAIN_MENU_W + 256), (RES_Y - REPLAY_MAIN_TOP)
	);
	for(i = 0; i < RMC_COUNT; i++) {
		replay_main_unput_and_put(
			i, ((menu_sel == i)
				? ((GAME == 5) ? 14 : 8)
				: ((GAME == 5) ? 8 : 1)
			)
		);
	}
	menu_unput_and_put = replay_main_unput_and_put;
	replay_main_initialized = true;
	replay_main_input_allowed = false;
}

void far replay_main_update_and_render(const char *main_bg_fn)
{
	replay_start_config_t private_start;
	replay_op_main_bg_fn = main_bg_fn;

	if(!replay_main_private_checked) {
		replay_main_private_checked = true;
		if(replay_private_record_command_start(&private_start)) {
			replay_main_start_practice_apply(&private_start, false);
			return;
		}
		// MAIN consumes valid commands before returning to OP. Anything left
		// here is stale or malformed and must not turn an attract demo into a
		// replay/recording handoff.
		replay_command_clear();
	}
	if(replay_main_returning_from_option && !in_option) {
		replay_main_returning_from_option = false;
		menu_sel = RMC_OPTION;
		replay_main_initialized = false;
	}
	if(!replay_main_initialized) {
		replay_main_initialize();
	}
	if(!key_det) {
		replay_main_input_allowed = true;
	}
	if(!replay_main_input_allowed) {
		return;
	}
	if(key_det & INPUT_UP) {
		replay_main_selection_move(-1);
	}
	if(key_det & INPUT_DOWN) {
		replay_main_selection_move(+1);
	}

	if((key_det & INPUT_OK) || (key_det & INPUT_SHOT)) {
		snd_se_play_force(11);
		switch(menu_sel) {
		case RMC_GAME:
			replay_main_start_game();
			replay_main_return(RMC_GAME);
			return;
		case RMC_EXTRA:
			replay_main_start_extra();
			replay_main_return(RMC_EXTRA);
			return;
		case RMC_PRACTICE:
			replay_main_start_practice();
			replay_main_return(RMC_PRACTICE);
			return;
		case RMC_REGIST_VIEW:
			replay_op_bridge(ROBF_REGIST_VIEW_MENU);
			replay_main_initialized = false;
			break;
		case RMC_MUSICROOM:
			replay_op_bridge(ROBF_MUSICROOM_MENU);
			replay_op_bridge(ROBF_MAIN_CDG_LOAD);
			replay_main_return((GAME == 5) ? RMC_MUSICROOM : RMC_GAME);
			return;
		case RMC_REPLAY:
			if(replay_browser()) {
				resident->demo_num = 0;
				replay_op_exit_into_main(true, false);
			}
			replay_main_return(RMC_REPLAY);
			return;
		case RMC_OPTION:
			replay_main_initialized = false;
			replay_main_returning_from_option = true;
			in_option = true;
			menu_sel = 0;
			break;
		case RMC_QUIT:
			replay_main_initialized = false;
			quit = true;
			break;
		}
	}
	if(key_det & INPUT_CANCEL) {
		quit = true;
	}
	if(key_det) {
		replay_main_input_allowed = false;
	}
}

#undef REPLAY_MAIN_DESC_TOP
#undef REPLAY_MAIN_MENU_W
#undef REPLAY_MAIN_MENU_LEFT
#undef REPLAY_MAIN_CURSOR_RIGHT
#undef REPLAY_MAIN_CURSOR_LEFT
#undef REPLAY_MAIN_COMMAND_H
#undef REPLAY_MAIN_COMMAND_LEFT
#undef REPLAY_MAIN_TOP
#undef REPLAY_MAIN_CURSOR_W
#undef REPLAY_MAIN_LABEL_H
#undef REPLAY_MAIN_LABEL_W

// Preserve the paragraph phase of the following stock runtime segment.
#if (GAME == 4)
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90"
#else
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90"
#endif

#pragma codeseg
