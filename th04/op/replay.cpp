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
#include "th03/core/initexit.h"
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
#include "th04/gaiji/gaiji.h"
#include "th04/hardware/grppsafx.h"
#include "th04/op/op.hpp"
#include "th04/op/replay.hpp"
#include "th04/op/replay_font.hpp"
#include "th04/op/language.hpp"
#include "th04/language_overlay.hpp"
#include "th04/replay_format.hpp"
#include "th04/replay_targets.hpp"
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
#define REPLAY_OP_ACCESS_RW 2
#define REPLAY_OP_SLOT_ROWS 10
#define REPLAY_OP_LINE_CAPACITY 80
#define REPLAY_OP_LINE_TOP 112
#define REPLAY_OP_LINE_H 24
#define REPLAY_OP_CELL_W REPLAY_OP_FONT_NUMERIC_CELL_W
#define REPLAY_SCORE_DISPLAY_DIGITS 9
#define REPLAY_OP_COL_ACTIVE ((GAME == 5) ? 14 : 8)
#define REPLAY_OP_COL_SELECTED 7
#define REPLAY_OP_TEXT_SPACING 16
#define REPLAY_OP_DOS_RESERVE_PARAS (4096 >> 4)
#define PRACTICE_PAGE_COUNT 3
#define PRACTICE_TARGET_ROWS 9
#define PRACTICE_HISTORY_ROWS 12
#define PRACTICE_STAGE_ROWS 4
#define PRACTICE_STOCK_MAX 15

// The replay-list background has an unused palette entry at index 7. Reserve
// it for the one browser selection color instead of relying on either
// background's active color, which has insufficient contrast for an empty row.
#define REPLAY_OP_SELECTED_RED 0xFF
#define REPLAY_OP_SELECTED_GREEN 0xFF
#define REPLAY_OP_SELECTED_BLUE 0x00

#define REPLAY_BROWSER_MARKER_LEFT 48
#define REPLAY_BROWSER_SLOT_LEFT 64
#define REPLAY_BROWSER_NAME_LEFT 120
#define REPLAY_BROWSER_SHOT_LEFT 240
#define REPLAY_BROWSER_RANK_LEFT 328
#define REPLAY_BROWSER_SCORE_LEFT 416
#define REPLAY_BROWSER_STAGE_LEFT 540

#define REPLAY_DETAIL_LEFT 64
#define REPLAY_DETAIL_VALUE_RIGHT 336

#define REPLAY_SAVE_NAME_LEFT 114
#define REPLAY_SAVE_POINT_LEFT 372
#define REPLAY_SAVE_DATE_LEFT 96
#define REPLAY_SAVE_DIFFICULTY_LEFT 356
#define REPLAY_SAVE_CHARACTER_CENTER 462
#define REPLAY_SAVE_STAGE_LEFT 538
#define REPLAY_SAVE_VALUE_TOP 104
#define REPLAY_SAVE_METADATA_TOP 168

enum replay_op_background_t {
	ROB_REPLAY,
	ROB_REPLAY_SAVE,
	ROB_PRACTICE,
};

enum replay_op_word_t {
	ROW_REPLAY,
	ROW_PRACTICE,
	ROW_PRACTICE_SETUP,
	ROW_CONFIGURE_PRACTICE,
	ROW_VIEW_RECORDED_RUNS,
	ROW_SLOT,
	ROW_NAME,
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
	ROW_ALL,
	ROW_EX,
	ROW_PAGE,
	ROW_REPLAY_DETAILS,
	ROW_FINAL_SCORE,
	ROW_DATE,
	ROW_SLOWDOWN,
	ROW_STAGE_SPLITS,
	ROW_COMPLETE,
	ROW_MENU_RETURN,
	ROW_GAME_OVER,
	ROW_TURBO,
	ROW_ON,
	ROW_OFF,
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
static char replay_op_command_witness_fn[11];
static char replay_op_slot_fn[11];
static char replay_op_temp_fn[12];
static char replay_op_save_request_fn[12];
static char replay_op_save_request_witness_fn[12];
static char replay_op_save_txn_fn[12];
static char replay_op_save_backup_fn[12];
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

static void replay_op_font_ensure(void)
{
	char stock_archive_fn[13];

	if(replay_op_font) {
		return;
	}
	replay_op_stock_archive_name_set(stock_archive_fn);
	replay_op_font_load(
		reinterpret_cast<const unsigned char *>(stock_archive_fn)
	);
}

static void replay_op_background_name_set(
	char *fn, replay_op_background_t background
)
{
	#if (GAME == 4)
		if(background == ROB_PRACTICE) {
			fn[0] = 'P'; fn[1] = 'R'; fn[2] = 'A'; fn[3] = 'C';
			fn[4] = 'T'; fn[5] = 'I'; fn[6] = 'C';
			fn[7] = '.'; fn[8] = 'P'; fn[9] = 'I'; fn[10] = '\0';
			return;
		}
	#endif
	fn[0] = ((background == ROB_PRACTICE) ? 's' : 'S');
	fn[1] = ((background == ROB_PRACTICE) ? 'l' : 'L');
	fn[2] = ((background == ROB_PRACTICE) ? 'b' : 'B');
	fn[3] = '1';
	if(background == ROB_PRACTICE) {
		fn[4] = '.'; fn[5] = 'p'; fn[6] = 'i'; fn[7] = '\0';
	} else if(background == ROB_REPLAY_SAVE) {
		fn[4] = 'B'; fn[5] = '.'; fn[6] = 'P'; fn[7] = 'I';
		fn[8] = '\0';
	} else {
		fn[4] = '.'; fn[5] = 'P'; fn[6] = 'I'; fn[7] = '\0';
	}
}

static bool replay_op_background_load(replay_op_background_t background)
{
	char archive_fn[12];
	char background_fn[12];
	char stock_archive_fn[13];
	bool loaded;

	replay_op_patch_archive_name_set(archive_fn);
	replay_op_background_name_set(background_fn, background);
	replay_op_stock_archive_name_set(stock_archive_fn);
	#if (GAME == 5)
		if(background == ROB_PRACTICE) {
			return (pi_load(0, background_fn) == 0);
		}
	#endif
	pfend();
	pfstart(reinterpret_cast<const unsigned char *>(archive_fn));
	loaded = (pi_load(0, background_fn) == 0);
	pfend();
	pfstart(reinterpret_cast<const unsigned char *>(stock_archive_fn));
	return loaded;
}

static void replay_op_selected_palette_apply(void)
{
	palette_set(
		REPLAY_OP_COL_SELECTED,
		REPLAY_OP_SELECTED_RED,
		REPLAY_OP_SELECTED_GREEN,
		REPLAY_OP_SELECTED_BLUE
	);
}

static bool replay_op_screen_begin(
	replay_op_background_t background,
	graph_putsa_fx_func_t& previous_func,
	bool fade_out
)
{
	previous_func = graph_putsa_fx_func;
	graph_putsa_fx_spacing = REPLAY_OP_TEXT_SPACING;
	replay_op_font_ensure();
	if(fade_out) {
		palette_black_out(1);
	}
	if(!replay_op_background_load(background)) {
		graph_putsa_fx_func = previous_func;
		return false;
	}
	pi_palette_apply(0);
	replay_op_selected_palette_apply();
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

static bool replay_op_screen_background_replace(
	replay_op_background_t background
)
{
	pi_free(0);
	if(!replay_op_background_load(background)) {
		return false;
	}
	pi_palette_apply(0);
	replay_op_selected_palette_apply();
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

static void replay_op_practice_diagnostic_fn_set(char far *fn, bool start)
{
	fn[0] = 'T'; fn[1] = ('0' + GAME); fn[2] = 'P';
	if(start) {
		fn[3] = 'S'; fn[4] = 'T'; fn[5] = 'A'; fn[6] = 'R'; fn[7] = 'T';
		fn[8] = '.'; fn[9] = 'B'; fn[10] = 'I'; fn[11] = 'N'; fn[12] = '\0';
	} else {
		fn[3] = 'D'; fn[4] = 'I'; fn[5] = 'A'; fn[6] = 'G'; fn[7] = '.';
		fn[8] = 'C'; fn[9] = 'F'; fn[10] = 'G'; fn[11] = '\0';
	}
}

// LANGUAGE RC16 adds English-only OP presentation in the replay tail. Keep
// the stock CRT paragraph phase unchanged; the exact bytes are measured by
// verify_th0405_structural_layout.py against the frozen foundation maps.
#if (GAME == 4)
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#else
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#endif
	// RC19 adds OP-to-MAIN durability witnesses. Preserve the tail phase of the
	// stock CRT segment that follows REPLAY_OP_TEXT in both game builds.
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90"

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

static int replay_op_dos_open_rw(const char far *fn)
{
	unsigned fn_seg = REPLAY_OP_FP_SEG(fn);
	unsigned fn_off = REPLAY_OP_FP_OFF(fn);
	int result;

	_asm {
		push	ds
		mov	dx, fn_off
		mov	ds, fn_seg
		mov	ax, 3D02h
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

static bool replay_op_dos_rename(
	const char far *source, const char far *destination
)
{
	unsigned source_seg = REPLAY_OP_FP_SEG(source);
	unsigned source_off = REPLAY_OP_FP_OFF(source);
	unsigned destination_seg = REPLAY_OP_FP_SEG(destination);
	unsigned destination_off = REPLAY_OP_FP_OFF(destination);
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

static void replay_op_dos_flush(void)
{
	_asm {
		mov	ah, 0Dh
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

	replay_op_command_witness_fn[0] = 'T';
	replay_op_command_witness_fn[1] = ('0' + GAME);
	replay_op_command_witness_fn[2] = 'R';
	replay_op_command_witness_fn[3] = 'H';
	replay_op_command_witness_fn[4] = 'D';
	replay_op_command_witness_fn[5] = '.';
	replay_op_command_witness_fn[6] = 'C';
	replay_op_command_witness_fn[7] = 'F';
	replay_op_command_witness_fn[8] = 'G';
	replay_op_command_witness_fn[9] = '\0';

	replay_op_slot_fn[0] = 'T'; replay_op_slot_fn[1] = 'H';
	replay_op_slot_fn[2] = ('0' + GAME); replay_op_slot_fn[3] = 'R';
	replay_op_slot_fn[4] = '0'; replay_op_slot_fn[5] = '0';
	replay_op_slot_fn[6] = '.'; replay_op_slot_fn[7] = 'R';
	replay_op_slot_fn[8] = 'P'; replay_op_slot_fn[9] = 'Y';
	replay_op_slot_fn[10] = '\0';

	replay_op_temp_fn[0] = 'T'; replay_op_temp_fn[1] = ('0' + GAME);
	replay_op_temp_fn[2] = 'R'; replay_op_temp_fn[3] = 'P';
	replay_op_temp_fn[4] = 'T'; replay_op_temp_fn[5] = 'M';
	replay_op_temp_fn[6] = 'P'; replay_op_temp_fn[7] = '.';
	replay_op_temp_fn[8] = 'R'; replay_op_temp_fn[9] = 'P';
	replay_op_temp_fn[10] = 'Y'; replay_op_temp_fn[11] = '\0';

	replay_op_save_request_fn[0] = 'T';
	replay_op_save_request_fn[1] = ('0' + GAME);
	replay_op_save_request_fn[2] = 'R';
	replay_op_save_request_fn[3] = 'P';
	replay_op_save_request_fn[4] = 'S';
	replay_op_save_request_fn[5] = 'A';
	replay_op_save_request_fn[6] = 'V';
	replay_op_save_request_fn[7] = '.';
	replay_op_save_request_fn[8] = 'C';
	replay_op_save_request_fn[9] = 'F';
	replay_op_save_request_fn[10] = 'G';
	replay_op_save_request_fn[11] = '\0';

	replay_op_save_request_witness_fn[0] = 'T';
	replay_op_save_request_witness_fn[1] = ('0' + GAME);
	replay_op_save_request_witness_fn[2] = 'R';
	replay_op_save_request_witness_fn[3] = 'S';
	replay_op_save_request_witness_fn[4] = 'H';
	replay_op_save_request_witness_fn[5] = 'D';
	replay_op_save_request_witness_fn[6] = '.';
	replay_op_save_request_witness_fn[7] = 'C';
	replay_op_save_request_witness_fn[8] = 'F';
	replay_op_save_request_witness_fn[9] = 'G';
	replay_op_save_request_witness_fn[10] = '\0';

	replay_op_save_txn_fn[0] = 'T';
	replay_op_save_txn_fn[1] = ('0' + GAME);
	replay_op_save_txn_fn[2] = 'R'; replay_op_save_txn_fn[3] = 'P';
	replay_op_save_txn_fn[4] = 'T'; replay_op_save_txn_fn[5] = 'X';
	replay_op_save_txn_fn[6] = 'N'; replay_op_save_txn_fn[7] = '.';
	replay_op_save_txn_fn[8] = 'C'; replay_op_save_txn_fn[9] = 'F';
	replay_op_save_txn_fn[10] = 'G'; replay_op_save_txn_fn[11] = '\0';

	replay_op_save_backup_fn[0] = 'T';
	replay_op_save_backup_fn[1] = ('0' + GAME);
	replay_op_save_backup_fn[2] = 'R';
	replay_op_save_backup_fn[3] = 'P';
	replay_op_save_backup_fn[4] = 'B';
	replay_op_save_backup_fn[5] = 'A';
	replay_op_save_backup_fn[6] = 'K';
	replay_op_save_backup_fn[7] = '.';
	replay_op_save_backup_fn[8] = 'R';
	replay_op_save_backup_fn[9] = 'P';
	replay_op_save_backup_fn[10] = 'Y';
	replay_op_save_backup_fn[11] = '\0';

	replay_op_main_binary[0] = 'm'; replay_op_main_binary[1] = 'a';
	replay_op_main_binary[2] = 'i'; replay_op_main_binary[3] = 'n';
	replay_op_main_binary[4] = '\0';
	replay_op_debug_binary[0] = 'd'; replay_op_debug_binary[1] = 'e';
	replay_op_debug_binary[2] = 'b'; replay_op_debug_binary[3] = '\0';
	replay_op_paths_ready = true;
}

void far replay_op_memory_prepare(void)
{
	uint16_t largest;

	_asm {
		mov bx, 0FFFFh
		mov ah, 48h
		int 21h
		mov largest, bx
	}
	mem_assign_paras = largest;
	if(largest > REPLAY_OP_DOS_RESERVE_PARAS) {
		mem_assign_paras = static_cast<uint16_t>(
			largest - REPLAY_OP_DOS_RESERVE_PARAS
		);
	}
}

static void replay_op_exit_into_main(
	bool fade_out_bgm, bool allow_debug, bool black_out
)
{
	replay_op_paths_init();
	replay_op_font_free();
	replay_op_bridge(ROBF_EXIT_PREPARE);
	#if (GAME == 4)
		gaiji_restore();
	#endif
	if(fade_out_bgm) {
		snd_kaja_func(KAJA_SONG_FADE, 10);
	}
	if(black_out) {
		palette_black_out(1);
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

#if (GAME == 5)
void far replay_op_demo_exit_into_main(void)
{
	replay_op_exit_into_main(false, false, true);
}
#endif

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
		return (
			(start->phase == 0) &&
			replay_practice_chapter_valid(start->stage, start->section)
		);
	}
	if(start->kind == RSK_MIDBOSS) {
		return (
			(start->phase == 0) &&
			replay_practice_midboss_valid(start->stage, start->section)
		);
	}
	if(start->kind == RSK_BOSS_PHASE) {
		#if (GAME == 5)
			switch(start->stage) {
			case 0: return ((start->section == 0) && (start->phase <= 4));
			case 1: return ((start->section == 0) && (start->phase <= 7));
			case 2: return ((start->section == 0) && (start->phase <= 13));
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
			case 0: return ((start->section == 0) && (start->phase <= 4));
			case 1: return ((start->section == 0) && (start->phase <= 5));
			case 2: return ((start->section == 0) && (start->phase <= 3));
			case 3:
				return (
					(start->section == 0) &&
					(start->phase <= ((start->playchar == 0) ? 2 : 11))
				);
			case 4: return ((start->section == 0) && (start->phase <= 17));
			case 5: return ((start->section == 0) && (start->phase <= 16));
			case STAGE_EXTRA:
				if(start->section == RCS_TH04_MUGETSU) {
					return (start->phase <= 6);
				}
				return (
					(start->section == RCS_TH04_GENGETSU) &&
					(start->phase <= 8)
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

static bool replay_op_header_valid(
	uint32_t file_size, replay_user_status_t expected_status
)
{
	uint32_t stored = replay_op_header.header_checksum;
	uint32_t computed;
	uint32_t input_end;
	uint32_t expected_file_size;
	bool checkpoint;

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
		(replay_op_header.magic[6] != '\0') ||
		(replay_op_header.magic[7] != '\0') ||
		(replay_op_header.magic[5] != ('0' + replay_op_header.version)) ||
		(
			(replay_op_header.version != REPLAY_USER_VERSION) &&
			(replay_op_header.version != REPLAY_USER_VERSION_LEGACY)
		) ||
		(replay_op_header.header_size != REPLAY_USER_HEADER_SIZE) ||
		(replay_op_header.packet_size != REPLAY_USER_PACKET_SIZE) ||
		((replay_op_header.flags & ~REPLAY_USER_KNOWN_FLAGS) != 0) ||
		((replay_op_header.flags & (REPLAY_USER_FLAG_RLE_INPUT |
		 REPLAY_USER_FLAG_SHIFT_INPUT)) !=
		 (REPLAY_USER_FLAG_RLE_INPUT | REPLAY_USER_FLAG_SHIFT_INPUT)) ||
		(replay_op_header.status != expected_status) ||
		(replay_op_header.game_id != GAME) ||
		(replay_op_header.ruleset != REPLAY_USER_RULESET_STOCK) ||
		(replay_op_header.mode > RUM_PRACTICE) ||
		((replay_op_header.mode == RUM_PRACTICE) !=
		 ((replay_op_header.flags & REPLAY_USER_FLAG_PRACTICE) != 0)) ||
		(replay_op_header.input_semantics != REPLAY_USER_INPUT_SEMANTICS) ||
		(replay_op_header.input_offset != REPLAY_USER_INPUT_OFFSET) ||
		(replay_op_header.input_size > REPLAY_USER_INPUT_SIZE_MAX) ||
		(replay_op_header.packet_count >
		 (REPLAY_USER_INPUT_SIZE_MAX / REPLAY_USER_PACKET_SIZE)) ||
		(replay_op_header.input_size !=
		 (replay_op_header.packet_count * REPLAY_USER_PACKET_SIZE)) ||
		(file_size != expected_file_size) ||
		(replay_op_header.stage_reached > STAGE_EXTRA) ||
		(replay_op_header.stage_directory_checksum == 0) ||
		!replay_op_start_valid(
			&replay_op_header.start,
			(replay_op_header.mode == RUM_PRACTICE), checkpoint
		)
	) {
		return false;
	}
	if(
		(replay_op_header.version == REPLAY_USER_VERSION_LEGACY) &&
		(replay_op_header.timed_frames || replay_op_header.slow_frames)
	) {
		return false;
	}
	if(
		(replay_op_header.version == REPLAY_USER_VERSION) &&
		(replay_op_header.slow_frames > replay_op_header.timed_frames)
	) {
		return false;
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

static bool replay_op_stage_entry_read(
	uint8_t stage, replay_stage_entry_t far *entry
)
{
	uint32_t offset;
	int fh;
	bool ok;

	if(stage >= REPLAY_USER_STAGE_COUNT) {
		return false;
	}
	offset = (
		REPLAY_USER_HEADER_SIZE +
		static_cast<uint16_t>(stage * REPLAY_STAGE_ENTRY_SIZE)
	);
	fh = replay_op_dos_open(replay_op_slot_fn);
	if(fh < 0) {
		return false;
	}
	ok = (
		replay_op_dos_seek(fh, offset) &&
		(replay_op_dos_read(fh, entry, sizeof(*entry)) == sizeof(*entry))
	);
	replay_op_dos_close(fh);
	return ok;
}

static bool replay_op_stage_directory_valid(int fh)
{
	replay_stage_entry_t far *entries;
	replay_stage_entry_t far *entry;
	uint8_t far *buffer;
	uint32_t hash;
	uint32_t previous_sample = 0;
	uint32_t previous_packet = 0;
	uint8_t stage;
	bool expected;
	bool ok = false;

	buffer = reinterpret_cast<uint8_t far *>(
		hmem_allocbyte(REPLAY_STAGE_DIRECTORY_SIZE)
	);
	if(buffer == 0) {
		return false;
	}
	if(
		!replay_op_dos_seek(fh, REPLAY_USER_HEADER_SIZE) ||
		(replay_op_dos_read(
			fh, buffer, REPLAY_STAGE_DIRECTORY_SIZE
		) != REPLAY_STAGE_DIRECTORY_SIZE)
	) {
		hmem_free(reinterpret_cast<void __seg *>(buffer));
		return false;
	}
	hash = replay_op_fnv1a(
		REPLAY_FNV1A_BASIS, buffer, REPLAY_STAGE_DIRECTORY_SIZE
	);
	if(hash != replay_op_header.stage_directory_checksum) {
		hmem_free(reinterpret_cast<void __seg *>(buffer));
		return false;
	}
	entries = reinterpret_cast<replay_stage_entry_t far *>(buffer);
	for(stage = 0; stage < REPLAY_USER_STAGE_COUNT; stage++) {
		entry = &entries[stage];
		expected = (
			(replay_op_header.mode == RUM_STORY)
				? (
					(stage >= replay_op_header.start.stage) &&
					(stage <= replay_op_header.stage_reached)
				)
				: (
					(replay_op_header.start.kind == RSK_STAGE) &&
					(stage == replay_op_header.start.stage)
				)
		);
		if(!expected) {
			if(!replay_op_bytes_zero(
				reinterpret_cast<const uint8_t far *>(entry), sizeof(*entry)
			)) {
				break;
			}
			continue;
		}
		if(
			(entry->start.stage != stage) ||
			!replay_op_start_valid(&entry->start, true, false) ||
			(entry->sample_index > replay_op_header.sample_count) ||
			(entry->packet_index >= replay_op_header.packet_count) ||
			(entry->payload_checksum == 0) ||
			((stage != replay_op_header.start.stage) &&
			 ((entry->sample_index < previous_sample) ||
			  (entry->packet_index <= previous_packet)))
		) {
			break;
		}
		previous_sample = entry->sample_index;
		previous_packet = entry->packet_index;
	}
	if(stage == REPLAY_USER_STAGE_COUNT) {
		ok = true;
	}
	hmem_free(reinterpret_cast<void __seg *>(buffer));
	return ok;
}

static bool replay_op_file_header_read(
	const char far *fn, replay_user_status_t expected_status, bool deep
)
{
	uint32_t file_size;
	int fh;
	bool valid;

	fh = replay_op_dos_open(fn);
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
	valid = replay_op_header_valid(file_size, expected_status);
	if(valid && deep) {
		valid = replay_op_stage_directory_valid(fh);
	}
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

static bool replay_op_header_read(uint8_t slot, bool deep)
{
	replay_op_paths_init();
	replay_op_slot_set(slot);
	return replay_op_file_header_read(
		replay_op_slot_fn, RUS_FINALIZED, deep
	);
}

static bool replay_op_file_exists(const char far *fn)
{
	int fh = replay_op_dos_open(fn);

	if(fh < 0) {
		return false;
	}
	replay_op_dos_close(fh);
	return true;
}

static bool replay_op_save_request_read(replay_save_request_t far *request)
{
	uint32_t file_size;
	uint32_t stored;
	uint32_t computed;
	int fh;
	unsigned size;

	fh = replay_op_dos_open(replay_op_save_request_fn);
	if(fh < 0) {
		return false;
	}
	replay_op_memclear(request, sizeof(*request));
	size = replay_op_dos_read(fh, request, sizeof(*request));
	if(!replay_op_dos_size(fh, &file_size)) {
		file_size = 0;
	}
	replay_op_dos_close(fh);
	stored = request->checksum;
	request->checksum = 0;
	computed = replay_op_fnv1a(
		REPLAY_FNV1A_BASIS, request, sizeof(*request)
	);
	request->checksum = stored;
	return (
		(size == sizeof(*request)) &&
		(file_size == sizeof(*request)) &&
		(request->magic[0] == 'T') &&
		(request->magic[1] == ('0' + GAME)) &&
		(request->magic[2] == 'R') && (request->magic[3] == 'S') &&
		(request->magic[4] == 'A') && (request->magic[5] == 'V') &&
		(request->magic[6] == '1') && (request->magic[7] == '\0') &&
		(request->schema == REPLAY_SAVE_REQUEST_SCHEMA) &&
		(request->source <= RSRS_PAUSE_SAVE_EXIT) &&
		(request->reserved == 0) &&
		(request->replay_header_checksum != 0) &&
		(stored == computed)
	);
}

static bool replay_op_pending_read(
	replay_save_request_t far *request, bool deep
)
{
	return (
		replay_op_save_request_read(request) &&
		replay_op_file_header_read(replay_op_temp_fn, RUS_PENDING, deep) &&
		(request->replay_header_checksum == replay_op_header.header_checksum)
	);
}

static void replay_op_pending_request_validate(void)
{
	replay_save_request_t request;

	// The request primary is authoritative. Its unparsed witness only forces a
	// directory update before OP starts, so consume it even without a primary.
	replay_op_dos_delete(replay_op_save_request_witness_fn);
	if(
		replay_op_file_exists(replay_op_save_request_fn) &&
		!replay_op_pending_read(&request, true)
	) {
		// Leave an invalid capture unavailable for diagnostic recovery, but do
		// not let its stale handoff enter the save UI.
		replay_op_dos_delete(replay_op_save_request_fn);
		replay_op_dos_flush();
	}
}

static bool replay_op_save_txn_read(replay_save_txn_t far *txn)
{
	uint32_t file_size;
	uint32_t stored;
	uint32_t computed;
	int fh;
	unsigned size;

	fh = replay_op_dos_open(replay_op_save_txn_fn);
	if(fh < 0) {
		return false;
	}
	replay_op_memclear(txn, sizeof(*txn));
	size = replay_op_dos_read(fh, txn, sizeof(*txn));
	if(!replay_op_dos_size(fh, &file_size)) {
		file_size = 0;
	}
	replay_op_dos_close(fh);
	stored = txn->checksum;
	txn->checksum = 0;
	computed = replay_op_fnv1a(REPLAY_FNV1A_BASIS, txn, sizeof(*txn));
	txn->checksum = stored;
	return (
		(size == sizeof(*txn)) && (file_size == sizeof(*txn)) &&
		(txn->magic[0] == 'T') && (txn->magic[1] == ('0' + GAME)) &&
		(txn->magic[2] == 'R') && (txn->magic[3] == 'P') &&
		(txn->magic[4] == 'T') && (txn->magic[5] == 'X') &&
		(txn->magic[6] == '1') && (txn->magic[7] == '\0') &&
		(txn->schema == REPLAY_SAVE_TXN_SCHEMA) &&
		(txn->state >= RSTS_PREPARED) && (txn->state <= RSTS_INSTALLED) &&
		(txn->slot < REPLAY_USER_SLOT_COUNT) &&
		(txn->destination_existed <= 1) &&
		(txn->temp_header_checksum != 0) &&
		(stored == computed)
	);
}

static bool replay_op_save_txn_write(
	replay_save_txn_state_t state, uint8_t slot, bool destination_existed,
	uint32_t temp_header_checksum
)
{
	replay_save_txn_t txn;
	int fh;
	bool created = false;
	bool ok;

	replay_op_memclear(&txn, sizeof(txn));
	txn.magic[0] = 'T'; txn.magic[1] = ('0' + GAME);
	txn.magic[2] = 'R'; txn.magic[3] = 'P';
	txn.magic[4] = 'T'; txn.magic[5] = 'X';
	txn.magic[6] = '1'; txn.magic[7] = '\0';
	txn.schema = REPLAY_SAVE_TXN_SCHEMA;
	txn.state = state;
	txn.slot = slot;
	txn.destination_existed = destination_existed;
	txn.temp_header_checksum = temp_header_checksum;
	txn.checksum = 0;
	txn.checksum = replay_op_fnv1a(
		REPLAY_FNV1A_BASIS, &txn, sizeof(txn)
	);
	fh = replay_op_dos_open_rw(replay_op_save_txn_fn);
	if(fh < 0) {
		fh = replay_op_dos_create(replay_op_save_txn_fn);
		created = true;
	}
	if(fh < 0) {
		return false;
	}
	ok = (
		replay_op_dos_seek(fh, 0) &&
		(replay_op_dos_write(fh, &txn, sizeof(txn)) == sizeof(txn))
	);
	replay_op_dos_close(fh);
	if(created && !ok) {
		replay_op_dos_delete(replay_op_save_txn_fn);
	}
	replay_op_dos_flush();
	return ok;
}

static uint32_t replay_op_pending_header_identity(void)
{
	uint8_t status = replay_op_header.status;
	uint32_t stored = replay_op_header.header_checksum;
	uint32_t identity;

	replay_op_header.status = RUS_PENDING;
	replay_op_header.header_checksum = 0;
	identity = replay_op_fnv1a(
		REPLAY_FNV1A_BASIS, &replay_op_header, sizeof(replay_op_header)
	);
	replay_op_header.status = status;
	replay_op_header.header_checksum = stored;
	return identity;
}

static bool replay_op_header_write(const char far *fn)
{
	int fh;
	bool ok;

	replay_op_header.header_checksum = 0;
	replay_op_header.header_checksum = replay_op_fnv1a(
		REPLAY_FNV1A_BASIS, &replay_op_header, sizeof(replay_op_header)
	);
	fh = replay_op_dos_open_rw(fn);
	if(fh < 0) {
		return false;
	}
	ok = (
		replay_op_dos_seek(fh, 0) &&
		(replay_op_dos_write(
			fh, &replay_op_header, sizeof(replay_op_header)
		) == sizeof(replay_op_header))
	);
	replay_op_dos_close(fh);
	replay_op_dos_flush();
	return ok;
}

static void replay_op_save_transaction_cleanup(void)
{
	replay_op_dos_delete(replay_op_save_backup_fn);
	replay_op_dos_delete(replay_op_save_txn_fn);
	replay_op_dos_delete(replay_op_save_request_fn);
	replay_op_dos_delete(replay_op_save_request_witness_fn);
	replay_op_dos_delete(replay_op_temp_fn);
	replay_op_dos_flush();
}

static void replay_op_save_transaction_recover(void)
{
	replay_save_txn_t txn;
	bool destination_exists;
	bool backup_exists;
	bool backup_valid = false;
	bool destination_valid;

	replay_op_paths_init();
	if(!replay_op_file_exists(replay_op_save_txn_fn)) {
		return;
	}
	if(!replay_op_save_txn_read(&txn)) {
		replay_op_dos_delete(replay_op_save_txn_fn);
		replay_op_dos_flush();
		return;
	}
	replay_op_slot_set(txn.slot);
	destination_exists = replay_op_file_exists(replay_op_slot_fn);
	backup_exists = replay_op_file_exists(replay_op_save_backup_fn);
	if(txn.destination_existed) {
		backup_valid = replay_op_file_header_read(
			replay_op_save_backup_fn, RUS_FINALIZED, true
		);
	}
	if(destination_exists) {
		destination_valid = replay_op_file_header_read(
			replay_op_slot_fn, RUS_FINALIZED, true
		);
		if(
			destination_valid &&
			(replay_op_pending_header_identity() == txn.temp_header_checksum)
		) {
			replay_op_save_transaction_cleanup();
			return;
		}
		if(txn.state == RSTS_INSTALLED) {
			return;
		}
		if(
			(txn.state == RSTS_PREPARED) && destination_valid &&
			!backup_exists && replay_op_file_exists(replay_op_temp_fn)
		) {
			replay_op_dos_delete(replay_op_save_txn_fn);
			replay_op_dos_flush();
			return;
		}
		if(
			replay_op_file_header_read(
				replay_op_slot_fn, RUS_PENDING, true
			) &&
			(replay_op_header.header_checksum == txn.temp_header_checksum) &&
			!replay_op_file_exists(replay_op_temp_fn) &&
			(!txn.destination_existed || backup_valid) &&
			replay_op_dos_rename(replay_op_slot_fn, replay_op_temp_fn)
		) {
			if(
				txn.destination_existed &&
				!replay_op_dos_rename(
					replay_op_save_backup_fn, replay_op_slot_fn
				)
			) {
				return;
			}
			replay_op_dos_delete(replay_op_save_txn_fn);
			replay_op_dos_flush();
		}
		return;
	}
	if(txn.state == RSTS_INSTALLED) {
		return;
	}
	if(
		txn.destination_existed &&
		(!backup_valid || !replay_op_dos_rename(
			replay_op_save_backup_fn, replay_op_slot_fn
		))
	) {
		return;
	}
	if(
		!txn.destination_existed &&
		!replay_op_file_exists(replay_op_temp_fn)
	) {
		return;
	}
	replay_op_dos_delete(replay_op_save_txn_fn);
	replay_op_dos_flush();
}

static bool replay_op_save_transaction_fail(void)
{
	replay_op_save_transaction_recover();
	return false;
}

static bool replay_op_pending_commit(
	uint8_t slot, const char far *name
)
{
	replay_save_request_t request;
	bool destination_exists;
	uint32_t identity;
	unsigned i;

	replay_op_paths_init();
	replay_op_save_transaction_recover();
	if(replay_op_file_exists(replay_op_save_txn_fn)) {
		return false;
	}
	if((slot >= REPLAY_USER_SLOT_COUNT) || !replay_op_pending_read(&request, true)) {
		return false;
	}
	replay_op_slot_set(slot);
	destination_exists = replay_op_file_exists(replay_op_slot_fn);
	if(destination_exists && !replay_op_header_read(slot, true)) {
		return false;
	}
	if(!replay_op_pending_read(&request, true)) {
		return false;
	}
	for(i = 0; i < REPLAY_USER_NAME_LEN; i++) {
		replay_op_header.name[i] = name[i];
	}
	if(!replay_op_header_write(replay_op_temp_fn)) {
		return false;
	}
	identity = replay_op_header.header_checksum;
	if(!replay_op_save_txn_write(
		RSTS_PREPARED, slot, destination_exists, identity
	)) {
		return false;
	}
	if(destination_exists) {
		replay_op_dos_delete(replay_op_save_backup_fn);
		if(!replay_op_dos_rename(
			replay_op_slot_fn, replay_op_save_backup_fn
		)) {
			return replay_op_save_transaction_fail();
		}
		replay_op_dos_flush();
	}
	if(!replay_op_save_txn_write(
		RSTS_BACKUP_MOVED, slot, destination_exists, identity
	)) {
		return replay_op_save_transaction_fail();
	}
	if(!replay_op_dos_rename(replay_op_temp_fn, replay_op_slot_fn)) {
		return replay_op_save_transaction_fail();
	}
	replay_op_dos_flush();
	if(
		!replay_op_file_header_read(replay_op_slot_fn, RUS_PENDING, true) ||
		(replay_op_header.header_checksum != identity)
	) {
		return replay_op_save_transaction_fail();
	}
	replay_op_header.status = RUS_FINALIZED;
	if(!replay_op_header_write(replay_op_slot_fn)) {
		return replay_op_save_transaction_fail();
	}
	if(!replay_op_save_txn_write(
		RSTS_INSTALLED, slot, destination_exists, identity
	)) {
		return replay_op_save_transaction_fail();
	}
	replay_op_save_transaction_cleanup();
	return true;
}

static bool replay_op_command_write(
	replay_command_mode_t mode, uint8_t slot, uint8_t flags,
	const replay_start_config_t far *start
)
{
	replay_command_t command;
	char diagnostic_fn[13];
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
	if(flags & REPLAY_COMMAND_FLAG_PRACTICE) {
		replay_op_practice_diagnostic_fn_set(diagnostic_fn, false);
		if(replay_op_file_exists(diagnostic_fn)) {
			command.flags |= REPLAY_COMMAND_FLAG_DIAGNOSTIC;
		}
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
		return false;
	}
	// MAIN consumes this one-shot file immediately after execl(). Ensure that
	// the handoff does not depend on a later diagnostic write flushing DOS.
	replay_op_dos_flush();
	// This unparsed second copy is part of every OP-to-MAIN handoff. On some
	// DOS implementations, AH=0Dh alone does not make the preceding directory
	// update visible reliably across execl(). The diagnostic marker never
	// decides whether this durability witness is written.
	fh = replay_op_dos_create(replay_op_command_witness_fn);
	ok = (
		(fh >= 0) &&
		(replay_op_dos_write(fh, &command, sizeof(command)) == sizeof(command))
	);
	if(fh >= 0) {
		replay_op_dos_close(fh);
	}
	if(!ok) {
		// A later missing witness is cleanup-only at MAIN, but the producer
		// must not cross execl() after failing to issue this durability write.
		replay_command_clear();
		return false;
	}
	if(flags & REPLAY_COMMAND_FLAG_PRACTICE) {
		replay_op_practice_diagnostic_fn_set(diagnostic_fn, true);
		fh = replay_op_dos_create(diagnostic_fn);
		if(fh >= 0) {
			replay_op_dos_write(fh, &command, sizeof(command));
			replay_op_dos_close(fh);
		}
	}
	return true;
}

void replay_command_clear(void)
{
	replay_op_paths_init();
	replay_op_dos_delete(replay_op_cfg_fn);
	replay_op_dos_delete(replay_op_command_witness_fn);
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
		replay_command_clear();
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
		((command.flags & ~REPLAY_COMMAND_FLAG_DIAGNOSTIC) !=
		 (REPLAY_COMMAND_FLAG_PRACTICE |
		 REPLAY_COMMAND_FLAG_PRIVATE_TEST)) ||
		(command.reserved_0 != 0) ||
		(command.start.kind <= RSK_STAGE) ||
		!replay_op_start_valid(&command.start, true, true)
	) {
		replay_command_clear();
		return false;
	}
	for(i = 0; i < sizeof(command.reserved); i++) {
		if(command.reserved[i] != 0) {
			replay_command_clear();
			return false;
		}
	}
	replay_op_copy(start, &command.start, sizeof(command.start));
	return true;
}

static bool replay_restart_command_start(
	replay_start_config_t far *start, uint8_t far *flags
)
{
	replay_command_t command;
	uint32_t file_size;
	int fh;
	unsigned size;
	unsigned i;
	bool practice;

	replay_op_paths_init();
	fh = replay_op_dos_open(replay_op_cfg_fn);
	if(fh < 0) {
		replay_command_clear();
		return false;
	}
	replay_op_memclear(&command, sizeof(command));
	size = replay_op_dos_read(fh, &command, sizeof(command));
	if(!replay_op_dos_size(fh, &file_size)) {
		file_size = 0;
	}
	replay_op_dos_close(fh);
	practice = ((command.flags & REPLAY_COMMAND_FLAG_PRACTICE) != 0);
	if(
		(size != sizeof(command)) || (file_size != sizeof(command)) ||
		(command.magic[0] != 'T') ||
		(command.magic[1] != ('0' + GAME)) ||
		(command.magic[2] != 'R') || (command.magic[3] != 'C') ||
		(command.magic[4] != 'F') || (command.magic[5] != 'G') ||
		(command.magic[6] != '2') || (command.magic[7] != '\0') ||
		(command.mode != RCM_RESTART) || (command.slot != 0) ||
		(command.flags & ~REPLAY_COMMAND_FLAG_PRACTICE) ||
		(command.reserved_0 != 0) ||
		!replay_op_start_valid(
			&command.start, practice,
			(practice && (command.start.kind > RSK_STAGE))
		)
	) {
		replay_command_clear();
		return false;
	}
	for(i = 0; i < sizeof(command.reserved); i++) {
		if(command.reserved[i] != 0) {
			replay_command_clear();
			return false;
		}
	}
	replay_op_copy(start, &command.start, sizeof(command.start));
	*flags = command.flags;
	replay_command_clear();
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
	case ROW_NAME:
		P('N'); P('a'); P('m'); P('e'); break;
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
	case ROW_ALL:
		P('A'); P('L'); P('L'); break;
	case ROW_EX:
		P('E'); P('X'); break;
	case ROW_PAGE:
		P('P'); P('a'); P('g'); P('e'); break;
	case ROW_REPLAY_DETAILS:
		P('R'); P('e'); P('p'); P('l'); P('a'); P('y'); P(' ');
		P('D'); P('e'); P('t'); P('a'); P('i'); P('l'); P('s'); break;
	case ROW_FINAL_SCORE:
		P('F'); P('i'); P('n'); P('a'); P('l'); P(' ');
		P('S'); P('c'); P('o'); P('r'); P('e'); break;
	case ROW_DATE:
		P('D'); P('a'); P('t'); P('e'); break;
	case ROW_SLOWDOWN:
		P('S'); P('l'); P('o'); P('w'); P('d'); P('o'); P('w'); P('n'); break;
	case ROW_STAGE_SPLITS:
		P('S'); P('t'); P('a'); P('g'); P('e'); P(' ');
		P('S'); P('p'); P('l'); P('i'); P('t'); P('s'); break;
	case ROW_COMPLETE:
		P('C'); P('o'); P('m'); P('p'); P('l'); P('e'); P('t'); P('e'); break;
	case ROW_MENU_RETURN:
		P('M'); P('e'); P('n'); P('u'); P(' ');
		P('R'); P('e'); P('t'); P('u'); P('r'); P('n'); break;
	case ROW_GAME_OVER:
		P('G'); P('a'); P('m'); P('e'); P(' ');
		P('O'); P('v'); P('e'); P('r'); break;
	case ROW_TURBO:
		P('T'); P('u'); P('r'); P('b'); P('o'); break;
	case ROW_ON:
		P('O'); P('n'); break;
	case ROW_OFF:
		P('O'); P('f'); P('f'); break;
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
	if(!replay_op_font) {
		return replay_op_spaces_append(p, width - (p - start));
	}
	// The replay browser still has fixed columns, but its words do not. Pad
	// only the inter-column gap with the font's genuine proportional space.
	*p = '\0';
	while(replay_op_font_width(start) < (width * REPLAY_OP_CELL_W)) {
		*p++ = ' ';
		*p = '\0';
	}
	return p;
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
	if(replay_op_font) {
		replay_op_font_put(left, top, replay_op_line, col);
	} else {
		graph_putsa_fx(
			left, top, col,
			reinterpret_cast<const shiftjis_t *>(replay_op_line)
		);
	}
}

static void replay_op_line_put_centered(vram_y_t top, vc2 col, char *p)
{
	*p = '\0';
	if(replay_op_font) {
		replay_op_font_put_centered((RES_X / 2), top, replay_op_line, col);
	} else {
		replay_op_line_put(
			((RES_X - ((p - replay_op_line) * 8)) / 2), top, col, p
		);
	}
}

static void replay_op_line_put_right(
	screen_x_t right, vram_y_t top, vc2 col, char *p
)
{
	*p = '\0';
	if(replay_op_font) {
		replay_op_font_put_right(right, top, replay_op_line, col);
	} else {
		replay_op_line_put(
			(right - ((p - replay_op_line) * 8)), top, col, p
		);
	}
}

static void replay_op_line_put_cells(
	screen_x_t left, vram_y_t top, vc2 col, char *p
)
{
	*p = '\0';
	if(replay_op_font) {
		replay_op_font_put_cells(left, top, replay_op_line, col);
	} else {
		replay_op_line_put(left, top, col, p);
	}
}

static void replay_op_line_put_numeric_cells(
	screen_x_t left, vram_y_t top, vc2 col, char *p
)
{
	*p = '\0';
	if(replay_op_font) {
		replay_op_font_put_numeric_cells(
			left, top, replay_op_line, (p - replay_op_line), col
		);
	} else {
		replay_op_line_put(left, top, col, p);
	}
}

static void replay_op_line_put_cells_right(
	screen_x_t right, vram_y_t top, vc2 col, char *p
)
{
	replay_op_line_put_cells(
		(right - ((p - replay_op_line) * REPLAY_OP_CELL_W)), top, col, p
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

static char *replay_op_shot_append(char *p)
{
	p = replay_op_word_append(
		p, replay_op_playchar_word(replay_op_header.start.playchar)
	);
	#if (GAME == 4)
		*p++ = (replay_op_header.start.shottype ? 'B' : 'A');
	#endif
	return p;
}

static char *replay_op_browser_stage_append(char *p)
{
	if(replay_op_header.start.stage == STAGE_EXTRA) {
		return replay_op_word_append(p, ROW_EX);
	}
	if(replay_op_header.end_reason == RUER_COMPLETE) {
		return replay_op_word_append(p, ROW_ALL);
	}
	return replay_op_uint_append(p, (replay_op_header.stage_reached + 1), 1);
}

static char *replay_op_name_append(char *p);

void replay_title_desc_put(void)
{
	if(language_op_english_selected()) {
		graph_putsa_fx_func = FX_WEIGHT_BOLD;
		graph_putsa_fx(
			(RES_X - GLYPH_FULL_W -
				(strlen(language_op_custom_desc(false)) * GLYPH_HALF_W)),
			(RES_Y - 16), ((GAME == 5) ? 9 : V_WHITE),
			reinterpret_cast<const shiftjis_t *>(language_op_custom_desc(false))
		);
		return;
	}
	char *p = replay_op_line;
	#define P(c) *p++ = c
	P(0x83); P(0x8A); P(0x83); P(0x76); P(0x83); P(0x8C); P(0x83); P(0x43);
	P(0x82); P(0xF0); P(0x8C); P(0xA9); P(0x82); P(0xDC); P(0x82); P(0xB7);
	#undef P
	*p = '\0';
	graph_putsa_fx(
		(RES_X - 16 - (16 * 8)), (RES_Y - 16),
		((GAME == 5) ? 9 : V_WHITE),
		reinterpret_cast<const shiftjis_t *>(replay_op_line)
	);
}

void replay_practice_title_desc_put(void)
{
	if(language_op_english_selected()) {
		graph_putsa_fx_func = FX_WEIGHT_BOLD;
		graph_putsa_fx(
			(RES_X - GLYPH_FULL_W -
				(strlen(language_op_custom_desc(true)) * GLYPH_HALF_W)),
			(RES_Y - 16), ((GAME == 5) ? 9 : V_WHITE),
			reinterpret_cast<const shiftjis_t *>(language_op_custom_desc(true))
		);
		return;
	}
	char *p = replay_op_line;
	#define P(c) *p++ = c
	P(0x97); P(0xFB); P(0x8F); P(0x4B); P(0x82); P(0xCC);
	P(0x90); P(0xDD); P(0x92); P(0xE8); P(0x82); P(0xF0);
	P(0x82); P(0xB5); P(0x82); P(0xDC); P(0x82); P(0xB7);
	#undef P
	*p = '\0';
	graph_putsa_fx(
		(RES_X - 16 - (18 * 8)), (RES_Y - 16),
		((GAME == 5) ? 9 : V_WHITE),
		reinterpret_cast<const shiftjis_t *>(replay_op_line)
	);
}

static void replay_browser_header_put(void)
{
	char *p = replay_op_line;
	p = replay_op_word_append(p, ROW_SLOT);
	replay_op_line_put(REPLAY_BROWSER_SLOT_LEFT, 80, V_WHITE, p);
	p = replay_op_line;
	p = replay_op_word_append(p, ROW_NAME);
	replay_op_line_put(REPLAY_BROWSER_NAME_LEFT, 80, V_WHITE, p);
	p = replay_op_line;
	p = replay_op_word_append(p, ROW_SHOT);
	replay_op_line_put(REPLAY_BROWSER_SHOT_LEFT, 80, V_WHITE, p);
	p = replay_op_line;
	p = replay_op_word_append(p, ROW_RANK);
	replay_op_line_put(REPLAY_BROWSER_RANK_LEFT, 80, V_WHITE, p);
	p = replay_op_line;
	p = replay_op_word_append(p, ROW_SCORE);
	replay_op_line_put(REPLAY_BROWSER_SCORE_LEFT, 80, V_WHITE, p);
	p = replay_op_line;
	p = replay_op_word_append(p, ROW_STAGE);
	replay_op_line_put(REPLAY_BROWSER_STAGE_LEFT, 80, V_WHITE, p);
}

static void replay_browser_slot_put(uint8_t slot, bool selected, vram_y_t top)
{
	char *p = replay_op_line;
	bool valid = replay_op_header_read(slot, false);
	vc2 col = (selected ? REPLAY_OP_COL_SELECTED : V_WHITE);

	if(selected) {
		*p++ = '>';
		replay_op_line_put(REPLAY_BROWSER_MARKER_LEFT, top, col, p);
	}
	p = replay_op_line;
	p = replay_op_uint_append(p, slot, 2);
	replay_op_line_put_numeric_cells(REPLAY_BROWSER_SLOT_LEFT, top, col, p);
	p = replay_op_line;
	if(!valid) {
		p = replay_op_word_append(p, ROW_NONE);
		replay_op_line_put(REPLAY_BROWSER_NAME_LEFT, top, col, p);
		return;
	}
	p = replay_op_name_append(p);
	replay_op_line_put_cells(REPLAY_BROWSER_NAME_LEFT, top, col, p);
	p = replay_op_line;
	p = replay_op_shot_append(p);
	replay_op_line_put(REPLAY_BROWSER_SHOT_LEFT, top, col, p);
	p = replay_op_line;
	p = replay_op_word_append(p, replay_op_rank_word(replay_op_header.start.rank));
	replay_op_line_put(REPLAY_BROWSER_RANK_LEFT, top, col, p);
	p = replay_op_line;
	p = replay_op_uint_append(
		p, replay_op_header.score_final, REPLAY_SCORE_DISPLAY_DIGITS
	);
	replay_op_line_put_cells(REPLAY_BROWSER_SCORE_LEFT, top, col, p);
	p = replay_op_line;
	p = replay_op_browser_stage_append(p);
	replay_op_line_put(REPLAY_BROWSER_STAGE_LEFT, top, col, p);
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
	int i;

	graph_accesspage(page_drawn);
	pi_put_8(0, 0, 0);
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

static char *replay_op_uint_zero_append(
	char *p, uint32_t value, unsigned width
)
{
	char digits[10];
	unsigned i;

	for(i = 0; i < width; i++) {
		digits[width - i - 1] = static_cast<char>('0' + (value % 10UL));
		value /= 10UL;
	}
	for(i = 0; i < width; i++) {
		*p++ = digits[i];
	}
	return p;
}

static char *replay_op_name_append(char *p)
{
	unsigned i;
	bool any = false;

	for(i = 0; i < REPLAY_USER_NAME_LEN; i++) {
		if(replay_op_header.name[i] != ' ') {
			any = true;
		}
	}
	if(!any) {
		return replay_op_word_append(p, ROW_NONE);
	}
	for(i = 0; i < REPLAY_USER_NAME_LEN; i++) {
		*p++ = replay_op_header.name[i];
	}
	return p;
}

static char *replay_op_end_reason_append(char *p)
{
	switch(replay_op_header.end_reason) {
	case RUER_COMPLETE:
		return replay_op_word_append(p, ROW_COMPLETE);
	case RUER_GAME_OVER:
		return replay_op_word_append(p, ROW_GAME_OVER);
	default:
		return replay_op_word_append(p, ROW_MENU_RETURN);
	}
}

static void replay_detail_left_put(uint8_t slot)
{
	char *p;
	uint16_t date = replay_op_header.dos_date;
	uint16_t year = static_cast<uint16_t>(1980 + (date >> 9));
	uint8_t month = static_cast<uint8_t>((date >> 5) & 0x0F);
	uint8_t day = static_cast<uint8_t>(date & 0x1F);

	p = replay_op_line;
	p = replay_op_word_append(p, ROW_SLOT);
	*p++ = ' ';
	p = replay_op_uint_zero_append(p, slot, 2);
	p = replay_op_spaces_append(p, 4);
	p = replay_op_name_append(p);
	replay_op_line_put_cells(REPLAY_DETAIL_LEFT, 80, REPLAY_OP_COL_ACTIVE, p);

	p = replay_op_line;
	p = replay_op_end_reason_append(p);
	replay_op_line_put(REPLAY_DETAIL_LEFT, 112, V_WHITE, p);

	p = replay_op_line;
	p = replay_op_word_append(p, ROW_FINAL_SCORE);
	replay_op_line_put(REPLAY_DETAIL_LEFT, 136, V_WHITE, p);
	p = replay_op_line;
	p = replay_op_uint_append(
		p, replay_op_header.score_final, REPLAY_SCORE_DISPLAY_DIGITS
	);
	replay_op_line_put_cells_right(
		REPLAY_DETAIL_VALUE_RIGHT, 136, V_WHITE, p
	);

	p = replay_op_line;
	p = replay_op_word_append(p, ROW_DATE);
	replay_op_line_put(REPLAY_DETAIL_LEFT, 160, V_WHITE, p);
	p = replay_op_line;
	p = replay_op_uint_zero_append(p, month, 2);
	*p++ = '-';
	p = replay_op_uint_zero_append(p, day, 2);
	*p++ = '-';
	p = replay_op_uint_zero_append(p, year, 4);
	replay_op_line_put_right(REPLAY_DETAIL_VALUE_RIGHT, 160, V_WHITE, p);

	p = replay_op_line;
	p = replay_op_word_append(p, replay_op_rank_word(replay_op_header.start.rank));
	*p++ = ' ';
	p = replay_op_word_append(
		p, replay_op_playchar_word(replay_op_header.start.playchar)
	);
	#if (GAME == 4)
		*p++ = ' ';
		*p++ = (replay_op_header.start.shottype ? 'B' : 'A');
	#endif
	replay_op_line_put(REPLAY_DETAIL_LEFT, 184, V_WHITE, p);

	#if (GAME == 4)
		p = replay_op_line;
		p = replay_op_word_append(p, ROW_TURBO);
		*p++ = ' ';
		p = replay_op_word_append(
			p, replay_op_header.start.turbo_mode ? ROW_ON : ROW_OFF
		);
		replay_op_line_put(REPLAY_DETAIL_LEFT, 208, V_WHITE, p);
	#endif
	p = replay_op_line;
	p = replay_op_word_append(p, ROW_SLOWDOWN);
	replay_op_line_put(
		REPLAY_DETAIL_LEFT, ((GAME == 4) ? 232 : 208), V_WHITE, p
	);
	p = replay_op_line;
	if(
		(replay_op_header.version == REPLAY_USER_VERSION_LEGACY) ||
		(replay_op_header.timed_frames == 0)
	) {
		*p++ = '-';
	} else {
		uint32_t fraction = (
			replay_op_header.slow_frames % replay_op_header.timed_frames
		);
		uint32_t accumulator = 0;
		uint16_t percent = (
			(replay_op_header.slow_frames / replay_op_header.timed_frames) *
			100
		);
		uint8_t i;

		// Accumulate the fractional hundredths without overflowing a 32-bit
		// numerator on a long recording.
		for(i = 0; i < 100; i++) {
			if(accumulator >= (replay_op_header.timed_frames - fraction)) {
				accumulator -= (replay_op_header.timed_frames - fraction);
				percent++;
			} else {
				accumulator += fraction;
			}
		}
		p = replay_op_uint_append(p, percent, 3);
		*p++ = '%';
	}
	replay_op_line_put_right(
		REPLAY_DETAIL_VALUE_RIGHT, ((GAME == 4) ? 232 : 208), V_WHITE, p
	);
	if(replay_op_header.mode == RUM_PRACTICE) {
		p = replay_op_line;
		p = replay_op_word_append(p, ROW_PRACTICE);
		replay_op_line_put(
			REPLAY_DETAIL_LEFT, ((GAME == 4) ? 256 : 232), V_WHITE, p
		);
	}
}

static void replay_detail_splits_put(uint8_t selected_stage)
{
	char *p;
	uint8_t first = replay_op_header.start.stage;
	uint8_t last = (
		(replay_op_header.mode == RUM_PRACTICE)
			? first
			: replay_op_header.stage_reached
	);
	uint8_t stage;
	vram_y_t top = 112;

	p = replay_op_line;
	p = replay_op_word_append(p, ROW_STAGE_SPLITS);
	replay_op_line_put(368, 80, REPLAY_OP_COL_ACTIVE, p);
	for(stage = first; stage <= last; stage++) {
		p = replay_op_line;
		*p++ = ((stage == selected_stage) ? '>' : ' ');
		*p++ = ' ';
		p = replay_op_word_append(p, ROW_STAGE);
		*p++ = ' ';
		*p++ = static_cast<char>('1' + stage);
		p = replay_op_spaces_append(p, 4);
		p = replay_op_uint_append(
			p,
			((stage == last)
				? replay_op_header.score_final
				: replay_op_header.stage_scores[stage]
			),
			REPLAY_SCORE_DISPLAY_DIGITS
		);
		replay_op_line_put_cells(
			368, top,
			((stage == selected_stage) ? REPLAY_OP_COL_ACTIVE : V_WHITE), p
		);
		top += 28;
	}
}

static void replay_detail_render(uint8_t slot, uint8_t selected_stage)
{
	uint8_t page_drawn = (1 - replay_op_page_shown);

	graph_accesspage(page_drawn);
	pi_put_8(0, 0, 0);
	graph_putsa_fx_func = FX_WEIGHT_NORMAL;
	replay_detail_left_put(slot);
	replay_detail_splits_put(selected_stage);
	graph_showpage(page_drawn);
	replay_op_page_shown = page_drawn;
}

static bool replay_detail(uint8_t slot)
{
	uint8_t selected_stage = replay_op_header.start.stage;
	uint8_t last_stage = (
		(replay_op_header.mode == RUM_PRACTICE)
			? selected_stage
			: replay_op_header.stage_reached
	);
	uint8_t command_flags;
	bool input_allowed = false;

	replay_detail_render(slot, selected_stage);
	while(1) {
		input_reset_sense_interface();
		if(key_det == INPUT_NONE) {
			input_allowed = true;
		}
		if(input_allowed) {
			if(key_det & INPUT_UP) {
				selected_stage = ((selected_stage == replay_op_header.start.stage)
					? last_stage
					: static_cast<uint8_t>(selected_stage - 1)
				);
				replay_detail_render(slot, selected_stage);
				snd_se_play_force(1);
			} else if(key_det & INPUT_DOWN) {
				selected_stage = ((selected_stage == last_stage)
					? replay_op_header.start.stage
					: static_cast<uint8_t>(selected_stage + 1)
				);
				replay_detail_render(slot, selected_stage);
				snd_se_play_force(1);
				} else if(key_det & INPUT_CANCEL) {
					while(key_det != INPUT_NONE) {
						input_reset_sense_interface();
						frame_delay(1);
					}
					return false;
			} else if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
				command_flags = 0;
				if(
					(replay_op_header.mode == RUM_STORY) &&
					(selected_stage != replay_op_header.start.stage)
				) {
					command_flags = static_cast<uint8_t>(
						(selected_stage + 1) << REPLAY_COMMAND_STAGE_SHIFT
					);
				}
				if(replay_op_command_write(
					RCM_PLAYBACK, slot, command_flags, NULL
				)) {
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
		case 3: return PF_ITEMS_SPAWNED;
		case 4: return PF_ITEMS_COLLECTED;
		case 5: return PF_POINT_ITEMS;
		case 6: return PF_MAX_POINT_ITEMS;
		case 7: return PF_ENEMIES_GONE;
		case 8: return PF_ENEMIES_KILLED;
		case 9: return PF_MISSES;
		case 10: return PF_BOMBS_USED;
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
		P('P'); P('o'); P('i'); P('n'); P('t'); P(' '); P('I'); P('t'); P('e');
		P('m'); P('s'); P(' '); P('a'); P('t'); P(' '); P('M'); P('a'); P('x'); break;
	case PF_ENEMIES_GONE:
		P('E'); P('n'); P('e'); P('m'); P('i'); P('e'); P('s'); P(' '); P('E'); P('s');
		P('c'); P('a'); P('p'); P('e'); P('d'); break;
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
	case 2: return 13;
	case 3: return ((section == RCS_TH05_PAIR) ? 2 : 9);
	case 4: return 10;
	case 5: return 12;
	default: return 17;
	}
#else
	switch(start->stage) {
	case 0: return 4;
	case 1: return 5;
	case 2: return 3;
	case 3: return ((start->playchar == 0) ? 2 : 11);
	case 4: return 17;
	case 5: return 16;
	default: return ((section == RCS_TH04_MUGETSU) ? 6 : 8);
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
		replay_practice_chapter_count(start->stage) +
		replay_practice_midboss_count(start->stage)
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
	uint8_t chapter;
	uint8_t midboss;
	uint8_t section;
	uint8_t chapters = replay_practice_chapter_count(start->stage);
	uint8_t midbosses = replay_practice_midboss_count(start->stage);

	index = 0;
	for(chapter = 1; chapter <= chapters; chapter++) {
		if(
			((chapter == 1) && (start->kind == RSK_STAGE)) ||
			((chapter > 1) && (start->kind == RSK_CHAPTER) &&
			 (start->section == chapter))
		) {
			return index;
		}
		index++;
		for(midboss = 0; midboss < midbosses; midboss++) {
			if(
				(replay_practice_midboss_after_chapter(
					start->stage, midboss
				) == chapter)
			) {
				if(
					(start->kind == RSK_MIDBOSS) &&
					(start->section == midboss)
				) {
					return index;
				}
				index++;
			}
		}
	}
	if(
		(start->kind != RSK_BOSS_PHASE) ||
		(start->section >= practice_boss_section_count(start)) ||
		(start->phase > practice_boss_phase_max(start, start->section))
	) {
		return 0;
	}
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
	uint8_t chapter;
	uint8_t count;
	uint8_t midboss;
	uint8_t section;

	practice_target_reset(start);
	count = replay_practice_chapter_count(start->stage);
	for(chapter = 1; chapter <= count; chapter++) {
		if(index == 0) {
			if(chapter > 1) {
				start->kind = RSK_CHAPTER;
				start->section = chapter;
			}
			return;
		}
		index--;
		for(
			midboss = 0;
			midboss < replay_practice_midboss_count(start->stage);
			midboss++
		) {
			if(
				replay_practice_midboss_after_chapter(
					start->stage, midboss
				) != chapter
			) {
				continue;
			}
			if(index == 0) {
				start->kind = RSK_MIDBOSS;
				start->section = midboss;
				return;
			}
			index--;
		}
	}
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
	if((start->kind == RSK_STAGE) || (start->kind == RSK_CHAPTER)) {
		P('C'); P('h'); P('a'); P('p'); P('t'); P('e'); P('r'); P(' ');
		return replay_op_uint_append(
			p, ((start->kind == RSK_STAGE) ? 1 : start->section), 2
		);
	}
	if(start->kind == RSK_MIDBOSS) {
		P('M'); P('i'); P('d'); P('b'); P('o'); P('s'); P('s');
		if(replay_practice_midboss_count(start->stage) > 1) {
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
	case PF_LIVES: return replay_op_uint_append(p, start->lives, 2);
	case PF_BOMBS: return replay_op_uint_append(p, start->bombs, 2);
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
		practice_u8_change(&start->lives, 1, PRACTICE_STOCK_MAX, 1, right);
		start->credit_lives = start->lives;
		break;
	case PF_BOMBS:
		practice_u8_change(&start->bombs, 0, PRACTICE_STOCK_MAX, 1, right);
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
	case PF_LIVES:
	case PF_BOMBS: return PRACTICE_STOCK_MAX;
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
	#undef P
	replay_op_line_put_centered(44, V_WHITE, p);
}

static void practice_render(
	const replay_start_config_t far *start, uint8_t page, uint8_t sel
)
{
	#define PRACTICE_VALUE_RIGHT 512
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
	replay_op_line_put_centered(16, REPLAY_OP_COL_ACTIVE, p);
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
			replay_op_line_put_centered(
				(68 + (i * 20)),
				((i == sel) ? REPLAY_OP_COL_ACTIVE : V_WHITE), p
			);
		} else {
			replay_op_line_put(104, (68 + (i * 20)),
				((i == sel) ? REPLAY_OP_COL_ACTIVE : V_WHITE), p);
			p = replay_op_line;
			p = practice_value_append(p, field, start);
			if(replay_op_font) {
				*p = '\0';
				replay_op_font_put_right(
					PRACTICE_VALUE_RIGHT, (68 + (i * 20)), replay_op_line,
					((i == sel) ? REPLAY_OP_COL_ACTIVE : V_WHITE)
				);
			} else {
				replay_op_line_put(
					static_cast<screen_x_t>(
						PRACTICE_VALUE_RIGHT - ((p - replay_op_line) * 8)
					), (68 + (i * 20)),
					((i == sel) ? REPLAY_OP_COL_ACTIVE : V_WHITE), p
				);
			}
		}
	}
	graph_showpage(page_drawn);
	replay_op_page_shown = page_drawn;
	#undef PRACTICE_VALUE_RIGHT
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
	replay_command_clear();
	replay_op_paths_init();
	replay_op_dos_delete(replay_op_temp_fn);
	replay_op_dos_delete(replay_op_save_request_fn);
	replay_op_dos_delete(replay_op_save_request_witness_fn);
	replay_op_copy(&start, start_in, sizeof(start));
	start.resident_rand = resident->rand;
	start.random_seed = resident->rand;
	if(!practice_start_valid(&start)) {
		return false;
	}
	return replay_op_command_write(
		RCM_RECORD, 0,
		(REPLAY_COMMAND_FLAG_PRACTICE |
		 REPLAY_COMMAND_FLAG_TEMP_CAPTURE),
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
				if(replay_op_header_read(sel, true)) {
					if(replay_detail(sel)) {
						palette_black_out(1);
						replay_op_screen_end(previous_func);
						return true;
					}
					replay_browser_render(sel);
				}
			}
			if(key_det != INPUT_NONE) {
				input_allowed = false;
			}
		}
		frame_delay(1);
	}
}

enum replay_save_modal_t {
	RSM_SAVE,
	RSM_DISCARD,
	RSM_OVERWRITE,
};

#define REPLAY_SAVE_ALPHABET_ROWS 3
#define REPLAY_SAVE_ALPHABET_COLS 17
#define REPLAY_SAVE_ALPHABET_LEFT 23
#define REPLAY_SAVE_ALPHABET_TOP ((GAME == 5) ? 20 : 17)
#define REPLAY_SAVE_CELL_LEFT 47
#define REPLAY_SAVE_CELL_RIGHT 48
#define REPLAY_SAVE_CELL_SPACE 49
#define REPLAY_SAVE_CELL_END 50

static unsigned replay_save_keyboard_glyph(uint8_t cell)
{
	if(cell < 28) {
		return (0xAA + cell);
	}
	if(cell < 34) {
		switch(cell) {
		case 28: return 0x03;
		case 29: return 0x06;
		case 30: return 0x07;
		case 31: return 0x08;
		case 32: return 0x0C;
		default: return 0x0F;
		}
	}
	if(cell < 44) {
		return (0xA0 + (cell - 34));
	}
	if(cell < 47) {
		return (gs_TEN + (cell - 44));
	}
	switch(cell) {
	case REPLAY_SAVE_CELL_LEFT: return gs_ARROW_LEFT;
	case REPLAY_SAVE_CELL_RIGHT: return gs_ARROW_RIGHT;
	case REPLAY_SAVE_CELL_SPACE: return gs_SPACE;
	default: return gs_END;
	}
}

static char replay_save_keyboard_ascii(uint8_t cell)
{
	if(cell < 26) {
		return ('A' + cell);
	}
	if((cell == 26) || (cell == 28) || (cell == 32)) {
		return '.';
	}
	if(cell == 29) {
		return '*';
	}
	if(cell == 30) {
		return '!';
	}
	if(cell == 31) {
		return '?';
	}
	if((cell == 33) || (cell == 45)) {
		return '@';
	}
	if((cell >= 34) && (cell < 44)) {
		return ('0' + (cell - 34));
	}
	return ' ';
}

static void replay_save_gaiji_putca(
	unsigned x, unsigned y, unsigned glyph, unsigned attr
)
{
	uint16_t far *tram = reinterpret_cast<uint16_t far *>(
		MK_FP((0xA000 + (y * 10)), (x * 2))
	);
	uint16_t far *attributes = reinterpret_cast<uint16_t far *>(
		MK_FP((0xA000 + (y * 10)), ((x * 2) + 0x2000))
	);
	uint16_t jis;
	uint8_t glyph_low = static_cast<uint8_t>(glyph);

	_asm {
		mov al, glyph_low
		mov ah, al
		mov al, 0
		rol ax, 1
		shr ax, 1
		adc ax, 56h
		mov jis, ax
	}
	tram[0] = jis;
	attributes[0] = static_cast<uint16_t>(attr);
	tram[1] = static_cast<uint16_t>(jis | 0x8000);
	attributes[1] = static_cast<uint16_t>(attr);
}

static void replay_save_keyboard_cell_put(
	uint8_t col, uint8_t row, tram_atrb2 attr
)
{
	uint8_t cell = static_cast<uint8_t>(
		(row * REPLAY_SAVE_ALPHABET_COLS) + col
	);
	replay_save_gaiji_putca(
		(REPLAY_SAVE_ALPHABET_LEFT + (col * GAIJI_TRAM_W)),
		(REPLAY_SAVE_ALPHABET_TOP + row),
		replay_save_keyboard_glyph(cell), attr
	);
}

static void replay_save_keyboard_put(uint8_t selected_col, uint8_t selected_row)
{
	uint8_t row;
	uint8_t col;

	text_clear();
	for(row = 0; row < REPLAY_SAVE_ALPHABET_ROWS; row++) {
		for(col = 0; col < REPLAY_SAVE_ALPHABET_COLS; col++) {
			replay_save_keyboard_cell_put(col, row, TX_WHITE);
		}
	}
	replay_save_keyboard_cell_put(
		selected_col, selected_row, (TX_GREEN | TX_REVERSE)
	);
}

static char *replay_save_modal_question_append(
	char *p, replay_save_modal_t modal
)
{
	#define P(c) *p++ = static_cast<char>(c)
	if(modal == RSM_SAVE) {
		P(0x83); P(0x8A); P(0x83); P(0x76); P(0x83); P(0x8C); P(0x83); P(0x43);
		P(0x82); P(0xF0); P(0x95); P(0xDB); P(0x91); P(0xB6); P(0x82); P(0xB5);
		P(0x82); P(0xDC); P(0x82); P(0xB7); P(0x82); P(0xA9); P(0x81); P(0x48);
	} else if(modal == RSM_DISCARD) {
		P(0x95); P(0xDB); P(0x91); P(0xB6); P(0x82); P(0xF0); P(0x82); P(0xE2);
		P(0x82); P(0xDF); P(0x82); P(0xDC); P(0x82); P(0xB7); P(0x82); P(0xA9);
		P(0x81); P(0x48);
	} else {
		P(0x82); P(0xB1); P(0x82); P(0xCC); P(0x83); P(0x58); P(0x83); P(0x8D);
		P(0x83); P(0x62); P(0x83); P(0x67); P(0x82); P(0xF0); P(0x8F); P(0xE3);
		P(0x8F); P(0x91); P(0x82); P(0xAB); P(0x82); P(0xB5); P(0x82); P(0xDC);
		P(0x82); P(0xB7); P(0x82); P(0xA9); P(0x81); P(0x48);
	}
	#undef P
	return p;
}

static char *replay_save_modal_choice_append(char *p, bool yes)
{
	#define P(c) *p++ = static_cast<char>(c)
	if(yes) {
		P(0x82); P(0xCD); P(0x82); P(0xA2);
	} else {
		P(0x82); P(0xA2); P(0x82); P(0xA2); P(0x82); P(0xA6);
	}
	#undef P
	return p;
}

static void replay_save_sjis_put_centered(vram_y_t top, vc2 color, char *p)
{
	*p = '\0';
	graph_putsa_fx_func = FX_WEIGHT_NORMAL;
	graph_putsa_fx(
		((RES_X - ((p - replay_op_line) * 8)) / 2), top, color,
		reinterpret_cast<const shiftjis_t *>(replay_op_line)
	);
}

static void replay_save_modal_render(
	replay_save_modal_t modal, bool selected_yes
)
{
	uint8_t page_drawn = (1 - replay_op_page_shown);
	char *p;

	graph_accesspage(page_drawn);
	pi_put_8(0, 0, 0);
	p = replay_op_line;
	p = replay_save_modal_question_append(p, modal);
	replay_save_sjis_put_centered(152, V_WHITE, p);
	p = replay_op_line;
	p = replay_save_modal_choice_append(p, true);
	replay_save_sjis_put_centered(
		200, (selected_yes ? REPLAY_OP_COL_ACTIVE : V_WHITE), p
	);
	p = replay_op_line;
	p = replay_save_modal_choice_append(p, false);
	replay_save_sjis_put_centered(
		232, (selected_yes ? V_WHITE : REPLAY_OP_COL_ACTIVE), p
	);
	graph_showpage(page_drawn);
	replay_op_page_shown = page_drawn;
}

static bool replay_save_modal(
	replay_save_modal_t modal, bool default_yes, bool fade_in
)
{
	bool selected_yes = default_yes;
	bool input_allowed = false;

	replay_save_modal_render(modal, selected_yes);
	if(fade_in) {
		palette_black_in(1);
	}
	while(1) {
		input_reset_sense_interface();
		if(key_det == INPUT_NONE) {
			input_allowed = true;
		}
		if(input_allowed) {
			if(key_det & (INPUT_UP | INPUT_DOWN | INPUT_LEFT | INPUT_RIGHT)) {
				selected_yes = !selected_yes;
				replay_save_modal_render(modal, selected_yes);
				snd_se_play_force(1);
			} else if(key_det & INPUT_CANCEL) {
				return false;
			} else if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
				return selected_yes;
			}
			if(key_det != INPUT_NONE) {
				input_allowed = false;
			}
		}
		frame_delay(1);
	}
}

static int replay_save_gaiji_for(char c)
{
	if((c >= '0') && (c <= '9')) {
		return (gb_0 + (c - '0'));
	}
	if((c >= 'A') && (c <= 'Z')) {
		return (gb_A + (c - 'A'));
	}
	return g_EMPTY;
}

static void replay_save_gaiji_puts(
	screen_x_t left, vram_y_t top, const char far *text,
	unsigned len, pixel_t step
)
{
	unsigned i;
	int glyph;

	for(i = 0; i < len; i++, left += step) {
		glyph = replay_save_gaiji_for(text[i]);
		if(glyph != g_EMPTY) {
			graph_gaiji_putc(left, top, glyph, V_WHITE);
		}
	}
}

static void replay_save_date_gaiji_put(
	screen_x_t left, vram_y_t top, uint16_t dos_date
)
{
	uint16_t year = static_cast<uint16_t>(1980 + (dos_date >> 9));
	uint8_t month = static_cast<uint8_t>((dos_date >> 5) & 0x0F);
	uint8_t day = static_cast<uint8_t>(dos_date & 0x1F);
	char *p = replay_op_line;
	char hyphen[2];

	p = replay_op_uint_zero_append(p, month, 2);
	p = replay_op_uint_zero_append(p, day, 2);
	p = replay_op_uint_zero_append(p, year, 4);
	replay_save_gaiji_puts(left, top, replay_op_line, 2, GAIJI_W);
	replay_save_gaiji_puts((left + 48), top, (replay_op_line + 2), 2, GAIJI_W);
	replay_save_gaiji_puts((left + 96), top, (replay_op_line + 4), 4, GAIJI_W);
	hyphen[0] = '-';
	hyphen[1] = '\0';
	graph_putsa_fx_func = FX_WEIGHT_BOLD;
	graph_putsa_fx((left + 36), top, V_WHITE, hyphen);
	graph_putsa_fx((left + 84), top, V_WHITE, hyphen);
}

static void replay_save_name_render(const char far *name)
{
	uint8_t page_drawn = (1 - replay_op_page_shown);
	uint16_t date = replay_op_header.dos_date;
	char *p;

	graph_accesspage(page_drawn);
	pi_put_8(0, 0, 0);
	// Redrawing the PI before the eight cells is the explicit clear for an
	// erased final glyph. Blank gaiji cells deliberately do not erase VRAM.
	replay_save_gaiji_puts(
		REPLAY_SAVE_NAME_LEFT, REPLAY_SAVE_VALUE_TOP,
		name, REPLAY_USER_NAME_LEN, GAIJI_W
	);
	p = replay_op_line;
	p = replay_op_uint_append(p, replay_op_header.score_final, 10);
	replay_save_gaiji_puts(
		REPLAY_SAVE_POINT_LEFT, REPLAY_SAVE_VALUE_TOP,
		replay_op_line, 10, GAIJI_W
	);
	replay_save_date_gaiji_put(
		REPLAY_SAVE_DATE_LEFT, REPLAY_SAVE_METADATA_TOP, date
	);
	switch(replay_op_header.start.rank) {
	case RANK_EASY: replay_op_line[0] = 'E'; break;
	case RANK_NORMAL: replay_op_line[0] = 'N'; break;
	case RANK_HARD: replay_op_line[0] = 'H'; break;
	case RANK_LUNATIC: replay_op_line[0] = 'L'; break;
	default: replay_op_line[0] = 'X'; break;
	}
	replay_save_gaiji_puts(
		REPLAY_SAVE_DIFFICULTY_LEFT, REPLAY_SAVE_METADATA_TOP,
		replay_op_line, 1, GAIJI_W
	);
	p = replay_op_line;
	p = replay_op_word_append(
		p, replay_op_playchar_word(replay_op_header.start.playchar)
	);
	#if (GAME == 4)
		*p++ = ' ';
		*p++ = (replay_op_header.start.shottype ? 'B' : 'A');
	#endif
	*p = '\0';
	replay_op_font_put_centered(
		REPLAY_SAVE_CHARACTER_CENTER, REPLAY_SAVE_METADATA_TOP,
		replay_op_line, V_WHITE
	);
	p = replay_op_line;
	if(replay_op_header.start.stage == STAGE_EXTRA) {
		*p++ = 'X';
	} else {
		*p++ = static_cast<char>('1' + replay_op_header.start.stage);
	}
	replay_save_gaiji_puts(
		REPLAY_SAVE_STAGE_LEFT, REPLAY_SAVE_METADATA_TOP,
		replay_op_line, 1, GAIJI_W
	);
	graph_showpage(page_drawn);
	replay_op_page_shown = page_drawn;
}

static void replay_save_name_menu_render(
	const char far *name, uint8_t col, uint8_t row
)
{
	replay_save_name_render(name);
	replay_save_keyboard_put(col, row);
}

static bool replay_save_name_menu(char far *name, bool fade_in)
{
	uint8_t col = 0;
	uint8_t row = 0;
	uint8_t cell;
	uint8_t cursor = 0;
	bool input_allowed = false;
	unsigned i;

	for(i = 0; i < REPLAY_USER_NAME_LEN; i++) {
		name[i] = ' ';
	}
	replay_save_name_menu_render(name, col, row);
	if(fade_in) {
		palette_black_in(1);
	}
	while(1) {
		input_reset_sense_interface();
		if(key_det == INPUT_NONE) {
			input_allowed = true;
		}
		if(input_allowed) {
			if(key_det & (INPUT_UP | INPUT_DOWN | INPUT_LEFT | INPUT_RIGHT)) {
				replay_save_keyboard_cell_put(col, row, TX_WHITE);
				if(key_det & INPUT_UP) {
					row = ((row == 0) ? (REPLAY_SAVE_ALPHABET_ROWS - 1) : (row - 1));
				}
				if(key_det & INPUT_DOWN) {
					row = ((row == (REPLAY_SAVE_ALPHABET_ROWS - 1)) ? 0 : (row + 1));
				}
				if(key_det & INPUT_LEFT) {
					col = ((col == 0) ? (REPLAY_SAVE_ALPHABET_COLS - 1) : (col - 1));
				}
				if(key_det & INPUT_RIGHT) {
					col = ((col == (REPLAY_SAVE_ALPHABET_COLS - 1)) ? 0 : (col + 1));
				}
				replay_save_keyboard_cell_put(
					col, row, (TX_GREEN | TX_REVERSE)
				);
				snd_se_play_force(1);
			} else if(key_det & INPUT_CANCEL) {
				text_clear();
				if(replay_save_modal(RSM_DISCARD, false, false)) {
					return false;
				}
				replay_save_name_menu_render(name, col, row);
			} else if(key_det & INPUT_BOMB) {
				if(cursor > 0) {
					cursor--;
				}
				name[cursor] = ' ';
				replay_save_name_menu_render(name, col, row);
			} else if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
				cell = static_cast<uint8_t>(
					(row * REPLAY_SAVE_ALPHABET_COLS) + col
				);
				if(cell == REPLAY_SAVE_CELL_END) {
					text_clear();
					return true;
				}
				if(cell == REPLAY_SAVE_CELL_LEFT) {
					if(cursor > 0) {
						cursor--;
					}
					name[cursor] = ' ';
				} else if(cell == REPLAY_SAVE_CELL_RIGHT) {
					if(cursor < (REPLAY_USER_NAME_LEN - 1)) {
						cursor++;
					}
				} else {
					name[cursor] = ((cell == REPLAY_SAVE_CELL_SPACE)
						? ' '
						: replay_save_keyboard_ascii(cell)
					);
					if(cursor < (REPLAY_USER_NAME_LEN - 1)) {
						cursor++;
					} else {
						replay_save_keyboard_cell_put(col, row, TX_WHITE);
						col = (REPLAY_SAVE_ALPHABET_COLS - 1);
						row = (REPLAY_SAVE_ALPHABET_ROWS - 1);
						replay_save_keyboard_cell_put(
							col, row, (TX_GREEN | TX_REVERSE)
						);
					}
				}
				replay_save_name_menu_render(name, col, row);
			}
			if(key_det != INPUT_NONE) {
				input_allowed = false;
			}
		}
		frame_delay(1);
	}
}

static void replay_save_pending_discard(void)
{
	replay_op_save_transaction_recover();
	if(!replay_op_file_exists(replay_op_save_txn_fn)) {
		replay_op_dos_delete(replay_op_save_request_fn);
		replay_op_dos_delete(replay_op_save_request_witness_fn);
		replay_op_dos_delete(replay_op_temp_fn);
		replay_op_dos_flush();
	}
}

static void replay_save_wait_for_key(void)
{
	do {
		input_reset_sense_interface();
		frame_delay(1);
	} while(key_det != INPUT_NONE);
	do {
		input_reset_sense_interface();
		frame_delay(1);
	} while(key_det == INPUT_NONE);
}

static void replay_save_complete_put(void)
{
	char *p = replay_op_line;

	graph_accesspage(replay_op_page_shown);
	if(language_op_english_selected()) {
		#define P(c) *p++ = static_cast<char>(c)
		P('S'); P('a'); P('v'); P('e'); P('d'); P('.'); P(' ');
		P('P'); P('r'); P('e'); P('s'); P('s'); P(' ');
		P('a'); P('n'); P('y'); P(' '); P('k'); P('e'); P('y'); P('.');
		#undef P
		replay_op_line_put_centered(332, REPLAY_OP_COL_ACTIVE, p);
	} else {
	#define P(c) *p++ = static_cast<char>(c)
	P(0x95); P(0xDB); P(0x91); P(0xB6); P(0x82); P(0xB5);
	P(0x82); P(0xDC); P(0x82); P(0xB5); P(0x82); P(0xBD);
	#undef P
		replay_save_sjis_put_centered(332, REPLAY_OP_COL_ACTIVE, p);
	}
	graph_showpage(replay_op_page_shown);
}

static void replay_save_slot_menu(
	const char far *name, graph_putsa_fx_func_t previous_func
)
{
	uint8_t sel = 0;
	bool occupied;
	bool input_allowed = false;

	text_clear();
	// Name entry and the browser own different PI surfaces. Tear the first one
	// down before loading the second; loading over it or replacing it in place
	// both left some machines displaying a black page until the next redraw.
	replay_op_screen_end(previous_func);
	if(!replay_op_screen_begin(ROB_REPLAY, previous_func, false)) {
		replay_save_pending_discard();
		return;
	}

	replay_browser_render(sel);
	palette_settone(100);
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
				if(replay_save_modal(RSM_DISCARD, false, false)) {
					replay_save_pending_discard();
					palette_black_out(1);
					replay_op_screen_end(previous_func);
					return;
				}
				replay_browser_render(sel);
			} else if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
				occupied = replay_op_header_read(sel, true);
				if(
					(!occupied || replay_save_modal(RSM_OVERWRITE, false, false)) &&
					replay_op_pending_commit(sel, name)
				) {
					replay_browser_render(sel);
					replay_save_complete_put();
					replay_save_wait_for_key();
					palette_black_out(1);
					replay_op_screen_end(previous_func);
					return;
				}
				if(occupied) {
					replay_browser_render(sel);
				}
			}
			if(key_det != INPUT_NONE) {
				input_allowed = false;
			}
		}
		frame_delay(1);
	}
}

static bool replay_save_pending(void)
{
	replay_save_request_t request;
	char name[REPLAY_USER_NAME_LEN];
	graph_putsa_fx_func_t previous_func;
	replay_op_background_t background;

	if(!replay_op_pending_read(&request, true)) {
		return false;
	}
	background = ((request.source == RSRS_POSTGAME)
		? ROB_REPLAY : ROB_REPLAY_SAVE
	);
	if(!replay_op_screen_begin(background, previous_func, true)) {
		return false;
	}
	text_clear();
	if(
		(request.source == RSRS_POSTGAME) &&
		!replay_save_modal(RSM_SAVE, true, true)
	) {
		replay_save_pending_discard();
		palette_black_out(1);
		replay_op_screen_end(previous_func);
		return true;
	}
	if(
		(request.source == RSRS_POSTGAME) &&
		!replay_op_screen_background_replace(ROB_REPLAY_SAVE)
	) {
		replay_save_pending_discard();
		replay_op_screen_end(previous_func);
		return true;
	}
	if(request.source == RSRS_POSTGAME) {
		// The modal is already visible. Reveal its in-place replacement without
		// adding a second fade before name entry.
		palette_settone(100);
	}
	if(!replay_save_name_menu(
		name, (request.source == RSRS_PAUSE_SAVE_EXIT)
	)) {
		replay_save_pending_discard();
		palette_black_out(1);
		replay_op_screen_end(previous_func);
		return true;
	}
	replay_save_slot_menu(name, previous_func);
	return true;
}

bool replay_record_next_prepare(void)
{
	replay_command_clear();
	replay_op_paths_init();
	replay_op_dos_delete(replay_op_temp_fn);
	replay_op_dos_delete(replay_op_save_request_fn);
	replay_op_dos_delete(replay_op_save_request_witness_fn);
	return replay_op_command_write(
		RCM_RECORD, 0, REPLAY_COMMAND_FLAG_TEMP_CAPTURE, NULL
	);
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
#define REPLAY_MAIN_RESTORE_TOP ((GAME == 4) ? 208 : REPLAY_MAIN_TOP)
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
	const char *desc = language_op_main_desc(desc_id);

	egc_copy_rect_1_to_0_16(0, REPLAY_MAIN_DESC_TOP, RES_X, GLYPH_H);
	if(language_op_english_selected() && desc) {
		graph_putsa_fx_func = FX_WEIGHT_BOLD;
		graph_putsa_fx(
			(RES_X - GLYPH_FULL_W - (strlen(desc) * GLYPH_HALF_W)),
			REPLAY_MAIN_DESC_TOP, ((GAME == 5) ? 9 : V_WHITE),
			reinterpret_cast<const shiftjis_t *>(desc)
		);
		return;
	}
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
		replay_main_command_put(top, CDG_MAIN_PRACTICE);
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
		replay_main_command_put(top, CDG_MAIN_REPLAY);
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

void far replay_main_language_assets_reload(void)
{
	replay_op_paths_init();
	graph_accesspage(1);
	language_asset_pi_load(0, replay_op_main_bg_fn);
	pi_palette_apply(0);
	pi_put_8(0, 0, 0);
	pi_free(0);
	graph_copy_page(0);
	graph_showpage(0);
	graph_accesspage(0);
	graph_putsa_fx_func = FX_WEIGHT_NORMAL;
	graph_putsa_fx_spacing = REPLAY_OP_TEXT_SPACING;
	palette_100();
	replay_main_initialized = false;
}

static void replay_main_return(int sel)
{
	replay_main_language_assets_reload();
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

// Returns true only after committing to the OP -> MAIN handoff. A rejected
// command-pair write leaves the caller responsible for reconstructing OP's
// title surface after the native character-select menu has faded it away.
static bool replay_main_start_game(void)
{
	#if (GAME == 4)
		language_op_character_prepare();
	#endif
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
		return false;
	}
	#if (GAME == 5)
		replay_main_th05_scores_reset();
	#else
		resident->demo_num = 0;
	#endif
	if(!resident->debug) {
		if(!replay_record_next_prepare()) {
			return false;
		}
	} else {
		replay_command_clear();
	}
	replay_op_exit_into_main(true, true, false);
	return true;
}

static bool replay_main_start_extra(void)
{
	#if (GAME == 4)
		language_op_character_prepare();
	#endif
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
		return false;
	}
	#if (GAME == 5)
		replay_main_th05_scores_reset();
	#else
		resident->demo_num = 0;
	#endif
	if(!resident->debug) {
		if(!replay_record_next_prepare()) {
			return false;
		}
	} else {
		replay_command_clear();
	}
	replay_op_exit_into_main(true, false, false);
	return true;
}

static bool replay_main_start_practice_apply(
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
		return false;
	}
	replay_op_exit_into_main(true, false, false);
	return true;
}

static bool replay_main_start_restart_apply(
	const replay_start_config_t far *start, uint8_t flags
)
{
	if(flags & REPLAY_COMMAND_FLAG_PRACTICE) {
		uint16_t previous_rand = resident->rand;

		// Practice serializes resident->rand into its new record. Only this
		// temporary assignment precedes the durability write, and it is undone
		// if the pair cannot be made durable.
		resident->rand = start->resident_rand;
		if(!replay_practice_record_prepare(start)) {
			resident->rand = previous_rand;
			return false;
		}
		return replay_main_start_practice_apply(start, false);
	}
	// A normal RECORD command has no launch-state payload. Make its primary
	// and witness durable before changing any resident value used by OP's title.
	if(!replay_record_next_prepare()) {
		return false;
	}
	resident->rand = start->resident_rand;
	if(start->stage != STAGE_EXTRA) {
		resident->rank = start->rank;
	}
	resident->stage = start->stage;
	resident->credit_lives = start->credit_lives;
	resident->credit_bombs = start->credit_bombs;
	resident->cfg_lives = start->credit_lives;
	resident->cfg_bombs = start->credit_bombs;
	resident->turbo_mode = (start->turbo_mode != 0);
	resident->demo_num = 0;
	#if (GAME == 5)
		resident->end_sequence = ES_SCORE;
		resident->playchar = start->playchar;
		replay_main_th05_scores_reset();
	#else
		resident->playchar_ascii = ('0' + start->playchar);
		resident->stage_ascii = ('0' + start->stage);
		resident->shottype = start->shottype;
	#endif
	replay_op_exit_into_main(true, false, false);
	return true;
}

static void replay_main_title_labels_load(void)
{
	char archive_fn[12];
	char stock_archive_fn[13];
	char labels_fn[11];

	replay_op_patch_archive_name_set(archive_fn);
	replay_op_stock_archive_name_set(stock_archive_fn);
	labels_fn[0] = 'R'; labels_fn[1] = 'P'; labels_fn[2] = 'Y';
	labels_fn[3] = 'T'; labels_fn[4] = 'T'; labels_fn[5] = 'L';
	labels_fn[6] = '.'; labels_fn[7] = 'C'; labels_fn[8] = 'D';
	labels_fn[9] = '2'; labels_fn[10] = '\0';
	pfend();
	pfstart(reinterpret_cast<const unsigned char *>(archive_fn));
	cdg_load_all(CDG_MAIN_PRACTICE, labels_fn);
	pfend();
	pfstart(reinterpret_cast<const unsigned char *>(stock_archive_fn));
}

void far replay_op_startup_dispatch(void)
{
	replay_start_config_t start;
	uint8_t flags;
	char se_fn[5];

	se_fn[0] = 'm'; se_fn[1] = 'i'; se_fn[2] = 'k'; se_fn[3] = 'o';
	se_fn[4] = '\0';
	snd_determine_modes(resident->bgm_mode, resident->se_mode);
	snd_load(se_fn, SND_LOAD_SE);
	replay_op_paths_init();
	replay_main_title_labels_load();
	replay_op_save_transaction_recover();
	replay_op_pending_request_validate();
	if(replay_save_pending()) {
		return;
	}
	if(replay_restart_command_start(&start, &flags)) {
		// Invoke the accepted request exactly once. On the normal success path
		// this execl()s into MAIN; false only means no replay-directed redirect,
		// so startup deliberately falls through into OP's stock title loop.
		replay_main_start_restart_apply(&start, flags);
	}
}

static bool replay_main_start_practice(void)
{
	replay_start_config_t start;
	#if (GAME == 4)
		language_op_character_prepare();
	#endif

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
		return false;
	}
	if(!replay_practice_setup(&start)) {
		return false;
	}
	return replay_main_start_practice_apply(&start, true);
}

static void replay_main_initialize(void)
{
	int i;

	replay_op_font_ensure();
	main_menu_unused_1 = 0;
	egc_copy_rect_1_to_0_16(
		(REPLAY_MAIN_MENU_LEFT - 128), REPLAY_MAIN_RESTORE_TOP,
		(REPLAY_MAIN_MENU_W + 256), (RES_Y - REPLAY_MAIN_RESTORE_TOP)
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
	uint8_t restart_flags;
	replay_op_main_bg_fn = main_bg_fn;

	if(!replay_main_private_checked) {
		replay_main_private_checked = true;
		replay_op_save_transaction_recover();
		replay_op_pending_request_validate();
		if(replay_save_pending()) {
			replay_main_return(RMC_REPLAY);
			return;
		}
		if(replay_restart_command_start(&private_start, &restart_flags)) {
			if(replay_main_start_restart_apply(&private_start, restart_flags)) {
				return;
			}
		}
		if(replay_private_record_command_start(&private_start)) {
			if(replay_main_start_practice_apply(&private_start, false)) {
				return;
			}
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
			if(!replay_main_start_game()) {
				replay_main_return(RMC_GAME);
			}
			return;
		case RMC_EXTRA:
			if(!replay_main_start_extra()) {
				replay_main_return(RMC_EXTRA);
			}
			return;
		case RMC_PRACTICE:
			if(!replay_main_start_practice()) {
				replay_main_return(RMC_PRACTICE);
			}
			return;
		case RMC_REGIST_VIEW:
			replay_op_bridge(ROBF_REGIST_VIEW_MENU);
			replay_main_initialized = false;
			break;
		case RMC_MUSICROOM:
			language_asset_music_prepare();
			graph_putsa_fx_func = FX_WEIGHT_BOLD;
			graph_putsa_fx_spacing = REPLAY_OP_TEXT_SPACING;
			replay_op_bridge(ROBF_MUSICROOM_MENU);
			replay_op_bridge(ROBF_MAIN_CDG_LOAD);
			replay_main_title_labels_load();
			replay_main_return((GAME == 5) ? RMC_MUSICROOM : RMC_GAME);
			return;
		case RMC_REPLAY:
			if(replay_browser()) {
				resident->demo_num = 0;
				replay_op_exit_into_main(true, false, false);
			}
			replay_main_return(RMC_REPLAY);
			return;
		case RMC_OPTION:
			egc_copy_rect_1_to_0_16(
				REPLAY_MAIN_MENU_LEFT, REPLAY_MAIN_TOP,
				REPLAY_MAIN_MENU_W,
				(RES_Y - REPLAY_MAIN_TOP)
			);
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
#undef REPLAY_MAIN_RESTORE_TOP
#undef REPLAY_MAIN_CURSOR_W
#undef REPLAY_MAIN_LABEL_H
#undef REPLAY_MAIN_LABEL_W

// Preserve the paragraph phase of the following stock runtime segment.
#if (GAME == 4)
	#pragma codestring "\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90"
	#pragma codestring "\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90"
#else
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90"
#endif

// RC21's presentation helpers remain in the OP tail. Keep the next stock
// runtime segment on its original paragraph phase; raw near offsets rely on
// it, and a shifted phase produces title-demo and game-start black screens.
#if (GAME == 4)
	#pragma codestring "\x90\x90\x90"
#endif

// Fixed-cell replay fields preserve the following stock segment phase.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"

#pragma codeseg
