#pragma option -zCREPLAY_OP_TEXT

// Interim OP-side browser and automatic recorder for the TH04/TH05 compact
// user replay format. All implementation lives in a new segment; the stock
// title menu only calls the narrow entry points in replay.hpp.

#include "platform.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/rank.h"
#include "th03/formats/pi.hpp"
#include "th02/hardware/frmdelay.h"
#include "th02/v_colors.hpp"
#include "th04/common.h"
#include "th04/hardware/grppsafx.h"
#include "th04/op/op.hpp"
#include "th04/op/replay.hpp"
#include "th04/replay_format.hpp"
#include "th04/snd/snd.h"
#if (GAME == 5)
	#include "th05/hardware/input.h"
	#include "th05/resident.hpp"
#else
	#include "th04/hardware/input.h"
	#include "th04/resident.hpp"
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
#define REPLAY_OP_TEXT_SPACING 16
#define PRACTICE_CORE_ROWS 12
#define PRACTICE_HISTORY_ROWS 14

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
	ROW_UP_DOWN_SELECT,
	ROW_LEFT_RIGHT_PAGE,
	ROW_Z_PLAY,
	ROW_ESC_BACK,
};

enum practice_field_t {
	PF_STAGE,
	PF_RANK,
	PF_LIVES,
	PF_BOMBS,
	PF_POWER,
	PF_DREAM,
	PF_PLAYPERF,
	PF_SCORE,
	PF_CONTINUES,
	PF_EXTENDS,
	PF_TURBO,
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
	fn[0] = 'S'; fn[1] = 'L'; fn[2] = 'B'; fn[3] = '1';
	if(background == ROB_PRACTICE) {
		fn[4] = 'B'; fn[5] = '.'; fn[6] = 'P'; fn[7] = 'I'; fn[8] = '\0';
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
	pfend();
	pfstart(reinterpret_cast<const unsigned char *>(archive_fn));
	loaded = (pi_load(0, background_fn) == 0);
	pfend();
	pfstart(reinterpret_cast<const unsigned char *>(stock_archive_fn));
	return loaded;
}

static bool replay_op_screen_begin(
	replay_op_background_t background,
	graph_putsa_fx_func_t& previous_func, pixel_t& previous_spacing
)
{
	previous_func = graph_putsa_fx_func;
	previous_spacing = graph_putsa_fx_spacing;
	graph_putsa_fx_spacing = REPLAY_OP_TEXT_SPACING;
	palette_black_out(4);
	if(!replay_op_background_load(background)) {
		graph_putsa_fx_func = previous_func;
		graph_putsa_fx_spacing = previous_spacing;
		return false;
	}
	pi_palette_apply(0);
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
	graph_putsa_fx_func_t previous_func, pixel_t previous_spacing
)
{
	pi_free(0);
	graph_putsa_fx_func = previous_func;
	graph_putsa_fx_spacing = previous_spacing;
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

	replay_op_paths_init();
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
	return ok;
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
	return (page ? PRACTICE_HISTORY_ROWS : PRACTICE_CORE_ROWS);
}

static practice_field_t practice_field(uint8_t page, uint8_t sel)
{
	if(!page) {
		return static_cast<practice_field_t>(sel);
	}
	if(sel == (PRACTICE_HISTORY_ROWS - 1)) {
		return PF_START;
	}
	return static_cast<practice_field_t>(PF_GRAZE + sel);
}

static char *practice_field_append(char *p, practice_field_t field)
{
	#define P(c) *p++ = c
	switch(field) {
	case PF_STAGE:
		P('S'); P('t'); P('a'); P('g'); P('e'); break;
	case PF_RANK:
		P('R'); P('a'); P('n'); P('k'); break;
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
		P('D'); P('y'); P('n'); P('a'); P('m'); P('i'); P('c'); P(' ');
		P('R'); P('a'); P('n'); P('k'); break;
	case PF_SCORE:
		P('S'); P('c'); P('o'); P('r'); P('e'); break;
	case PF_CONTINUES:
		P('C'); P('o'); P('n'); P('t'); P('i'); P('n'); P('u'); P('e'); P('s');
		break;
	case PF_EXTENDS:
		P('E'); P('x'); P('t'); P('e'); P('n'); P('d'); P('s'); break;
	case PF_TURBO:
		P('T'); P('u'); P('r'); P('b'); P('o'); P(' '); P('M'); P('o'); P('d'); P('e');
		break;
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
	case PF_RANK:
		return replay_op_word_append(p, replay_op_rank_word(start->rank));
	case PF_LIVES: return replay_op_uint_append(p, start->lives, 1);
	case PF_BOMBS: return replay_op_uint_append(p, start->bombs, 1);
	case PF_POWER: return replay_op_uint_append(p, start->power, 3);
	case PF_DREAM: return replay_op_uint_append(p, start->dream, 3);
	case PF_PLAYPERF: return replay_op_uint_append(p, start->playperf, 2);
	case PF_SCORE: return replay_op_uint_append(p, start->score, 8);
	case PF_CONTINUES: return replay_op_uint_append(p, start->continues_used, 1);
	case PF_EXTENDS: return replay_op_uint_append(p, start->extends_gained, 2);
	case PF_TURBO:
		if(start->turbo_mode) {
			*p++ = 'O'; *p++ = 'n';
		} else {
			*p++ = 'O'; *p++ = 'f'; *p++ = 'f';
		}
		return p;
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
	replay_op_memclear(start, sizeof(*start));
	start->schema = REPLAY_START_SCHEMA;
	start->kind = RSK_STAGE;
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
		*value = ((*value > (max - delta)) ? max : (*value + delta));
	} else {
		*value = ((*value < (min + delta)) ? min : (*value - delta));
	}
}

static void practice_u16_change(
	uint16_t far *value, uint16_t max, uint16_t delta, bool right
)
{
	if(right) {
		*value = ((*value > (max - delta)) ? max : (*value + delta));
	} else {
		*value = ((*value < delta) ? 0 : (*value - delta));
	}
}

static void practice_score_change(
	uint32_t far *value, uint32_t delta, bool right
)
{
	const uint32_t max = 99999990UL;
	if(right) {
		*value = ((*value > (max - delta)) ? max : (*value + delta));
	} else {
		*value = ((*value < delta) ? 0 : (*value - delta));
	}
}

static void practice_field_change(
	replay_start_config_t far *start, practice_field_t field, bool right,
	bool fast
)
{
	uint8_t rank;

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
		break;
	case PF_RANK:
		if(start->stage != STAGE_EXTRA) {
			rank = start->rank;
			practice_u8_change(&rank, RANK_EASY, RANK_LUNATIC, 1, right);
			start->rank = rank;
			start->playperf = replay_op_native_playperf(rank);
		}
		break;
	case PF_LIVES: practice_u8_change(&start->lives, 0, 9, 1, right); break;
	case PF_BOMBS: practice_u8_change(&start->bombs, 0, 9, 1, right); break;
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
	case PF_TURBO:
		if(start->stage != STAGE_EXTRA) {
			start->turbo_mode = (1 - start->turbo_mode);
		}
		break;
	case PF_GRAZE:
		practice_u16_change(&start->graze, 65535, (fast ? 100 : 1), right); break;
	case PF_STD_FRAMES:
		practice_u16_change(&start->std_frames, 65535, (fast ? 1000 : 1), right); break;
	case PF_ITEMS_SPAWNED:
		practice_u16_change(&start->items_spawned, 65535, (fast ? 100 : 1), right); break;
	case PF_ITEMS_COLLECTED:
		practice_u16_change(&start->items_collected, 65535, (fast ? 100 : 1), right); break;
	case PF_POINT_ITEMS:
		practice_u16_change(
			&start->point_items_collected, 65535, (fast ? 100 : 1), right
		); break;
	case PF_MAX_POINT_ITEMS:
		practice_u16_change(
			&start->max_valued_point_items_collected, 65535,
			(fast ? 100 : 1), right
		); break;
	case PF_ENEMIES_GONE:
		practice_u16_change(&start->enemies_gone, 65535, (fast ? 100 : 1), right); break;
	case PF_ENEMIES_KILLED:
		practice_u16_change(&start->enemies_killed, 65535, (fast ? 100 : 1), right); break;
	case PF_MISSES:
		practice_u8_change(&start->miss_count, 0, 255, (fast ? 10 : 1), right); break;
	case PF_BOMBS_USED:
		practice_u8_change(&start->bombs_used, 0, 255, (fast ? 10 : 1), right); break;
	case PF_STAGE_ITEMS:
		practice_u16_change(
			&start->stage_point_items_collected, ((GAME == 5) ? 999 : 255),
			(fast ? 10 : 1), right
		); break;
	case PF_STAGE_GRAZE:
		practice_u16_change(&start->stage_graze, 999, (fast ? 10 : 1), right); break;
	case PF_POWER_OVERFLOW:
		practice_u16_change(&start->power_overflow, 42, (fast ? 5 : 1), right); break;
	default: break;
	}
}

static void practice_page_name_put(uint8_t page)
{
	char *p = replay_op_line;
	#define P(c) *p++ = c
	if(page) {
		P('H'); P('i'); P('s'); P('t'); P('o'); P('r'); P('y');
	} else {
		P('C'); P('o'); P('r'); P('e');
	}
	P(' '); P('S'); P('e'); P('t'); P('t'); P('i'); P('n'); P('g'); P('s');
	P(' '); P('('); P('1' + page); P('/'); P('2'); P(')');
	#undef P
	replay_op_line_put((page ? 224 : 232), 44, V_WHITE, p);
}

static void practice_footer_put(void)
{
	char *p = replay_op_line;
	#define P(c) *p++ = c
	P('U'); P('p'); P('/'); P('D'); P('o'); P('w'); P('n'); P(' '); P('S'); P('e'); P('l'); P('e'); P('c'); P('t');
	P(' '); P(' '); P('L'); P('e'); P('f'); P('t'); P('/'); P('R'); P('i'); P('g'); P('h'); P('t'); P(' '); P('C'); P('h'); P('a'); P('n'); P('g'); P('e');
	#undef P
	replay_op_line_put(112, 356, V_WHITE, p);
	p = replay_op_line;
	#define P(c) *p++ = c
	P('X'); P(' '); P('P'); P('a'); P('g'); P('e'); P('/'); P('F'); P('a'); P('s'); P('t');
	P(' '); P(' '); P('Z'); P(' '); P('S'); P('t'); P('a'); P('r'); P('t');
	P(' '); P(' '); P('E'); P('s'); P('c'); P(' '); P('B'); P('a'); P('c'); P('k');
	#undef P
	replay_op_line_put(176, 376, V_WHITE, p);
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
	practice_footer_put();
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
	graph_putsa_fx_func_t previous_func;
	pixel_t previous_spacing;

	practice_defaults(start);
	if(!replay_op_screen_begin(ROB_PRACTICE, previous_func, previous_spacing)) {
		return false;
	}
	practice_render(start, page, sel);
	palette_100();
	while(1) {
		input_reset_sense_interface();
		if(key_det == INPUT_NONE) {
			input_allowed = true;
		}
		if(input_allowed) {
			rows = practice_row_count(page);
			field = practice_field(page, sel);
			if(key_det & INPUT_UP) {
				sel = ((sel == 0) ? (rows - 1) : (sel - 1));
				practice_render(start, page, sel);
				snd_se_play_force(1);
			} else if(key_det & INPUT_DOWN) {
				sel = ((sel == (rows - 1)) ? 0 : (sel + 1));
				practice_render(start, page, sel);
				snd_se_play_force(1);
			} else if(key_det & (INPUT_LEFT | INPUT_RIGHT)) {
				if(field != PF_START) {
					practice_field_change(
						start, field, ((key_det & INPUT_RIGHT) != 0),
						((key_det & INPUT_BOMB) != 0)
					);
					practice_render(start, page, sel);
					snd_se_play_force(1);
				}
			} else if(key_det & INPUT_BOMB) {
				page = (1 - page);
				rows = practice_row_count(page);
				if(sel >= rows) {
					sel = (rows - 1);
				}
				practice_render(start, page, sel);
				snd_se_play_force(1);
			} else if(key_det & INPUT_CANCEL) {
				palette_black_out(4);
				replay_op_screen_end(previous_func, previous_spacing);
				return false;
			} else if((key_det & (INPUT_SHOT | INPUT_OK)) && (field == PF_START)) {
				if(replay_op_start_valid(start, true, false)) {
					palette_black_out(4);
					replay_op_screen_end(previous_func, previous_spacing);
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

	replay_op_copy(&start, start_in, sizeof(start));
	start.resident_rand = resident->rand;
	start.random_seed = resident->rand;
	if(!replay_op_start_valid(&start, true, false)) {
		return false;
	}
	for(slot = 0; slot < REPLAY_USER_SLOT_COUNT; slot++) {
		if(!replay_op_header_read(slot, false)) {
			return replay_op_command_write(
				RCM_RECORD, slot, REPLAY_COMMAND_FLAG_PRACTICE, &start
			);
		}
	}
	return false;
}

bool replay_browser(void)
{
	uint8_t sel = 0;
	bool input_allowed = false;
	graph_putsa_fx_func_t previous_func;
	pixel_t previous_spacing;

	if(!replay_op_screen_begin(ROB_REPLAY, previous_func, previous_spacing)) {
		return false;
	}
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
				replay_op_screen_end(previous_func, previous_spacing);
				return false;
			} else if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
				if(
					replay_op_header_read(sel, true) &&
					replay_op_command_write(RCM_PLAYBACK, sel, 0, NULL)
				) {
					palette_black_out(4);
					replay_op_screen_end(previous_func, previous_spacing);
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
		if(!replay_op_header_read(slot, false)) {
			replay_op_command_write(RCM_RECORD, slot, 0, NULL);
			return;
		}
	}
}

#pragma codeseg
