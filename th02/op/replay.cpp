// TH02 replay and Practice title surface.
//
// This translation unit is linked after every stock OP object. The pragma
// keeps all implementation in a trailing patch-owned code segment; all
// persistent state below is uninitialized BSS.

#pragma codeseg T2OPRPLY_TEXT PATCH

#include <process.h>
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "x86real.h"
#include "platform/x86real/pc98/keyboard.hpp"
#include "th01/rank.h"
#include "th01/hardware/grppsafx.h"
#include "th01/rpyfont.hpp"
#include "th02/common.h"
#include "th02/resident.hpp"
#include "th02/hardware/frmdelay.h"
#include "th02/hardware/input.hpp"
#include "th02/core/globals.hpp"
#include "th02/core/initexit.h"
#include "th02/formats/cfg.hpp"
#include "th02/formats/pi.h"
#include "th02/snd/snd.h"
#include "th02/gaiji/gaiji.h"
#include "th02/gaiji/str.hpp"
#include "th02/shiftjis/fns.hpp"
#include "th02/op/op.h"
#include "th02/op/m_music.hpp"
#include "th02/replay_format.hpp"
#include "th02/practice_diag.hpp"
#include "th02/language.hpp"
#include "th02/v_colors.hpp"

#define T2OP_LINE_CAPACITY 79
#define T2OP_SLOT_ROWS 10
#define T2OP_SLOT_CELL_W 13
#define T2OP_SLOT_CELL_STEP (T2OP_SLOT_CELL_W + 1)
#define T2OP_SLOT_ONE_INSET 3
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
#define T2OP_BROWSER_MARKER_LEFT 48
#define T2OP_BROWSER_SLOT_LEFT 64
#define T2OP_BROWSER_NAME_LEFT 120
#define T2OP_BROWSER_SHOT_LEFT 240
#define T2OP_BROWSER_RANK_LEFT 328
#define T2OP_BROWSER_SCORE_RIGHT 522
#define T2OP_BROWSER_STAGE_LEFT 552
#define T2OP_BROWSER_LINE_TOP 112
#define T2OP_BROWSER_LINE_H 24
#define T2OP_DETAIL_LEFT 40
#define T2OP_DETAIL_NAME_LEFT T2OP_DETAIL_LEFT
#define T2OP_DETAIL_SLOT_LABEL_LEFT 224
#define T2OP_DETAIL_SLOT_LEFT 265
#define T2OP_DETAIL_VALUE_RIGHT 320
#define T2OP_DETAIL_SPLIT_LEFT 368
#define T2OP_DETAIL_SPLIT_SCORE_RIGHT 600
#define T2OP_DETAIL_TOP 80
#define T2OP_DETAIL_ROW_H 24
#define T2OP_PRACTICE_MARKER_X 10
#define T2OP_PRACTICE_LABEL_X 13
#define T2OP_PRACTICE_LABEL_LEFT 104
#define T2OP_PRACTICE_VALUE_RIGHT 512
// Preserve row 22 as the native visual gap before the stock Rank row.
#define T2OP_TITLE_COMMAND_FIRST_ROW 14
#define T2OP_TITLE_RANK_ROW 23
#define T2OP_TITLE_QUIT_ROW 21

void far replay_op_animate_finish(void)
{
	int slot;

	palette_white_in(6);
	for(slot = 0; slot < 3; slot++) {
		pi_free(slot);
		pi_buffers[slot] = 0;
	}
}

// Match the TH04/TH05 patch surfaces: PI palette entry 7 is reserved for the
// active row and is made independent of the background image's own palette.
#define T2OP_SELECTED_COLOR 7
#define T2OP_SELECTED_RED 0xFF
#define T2OP_SELECTED_GREEN 0xFF
#define T2OP_SELECTED_BLUE 0x00

extern char sel;
extern char menu_sel;
extern bool in_option;
extern bool quit;
extern unsigned char snd_bgm_mode;
extern unsigned int idle_frame;
extern char extra_unlocked;
extern resident_t __seg *resident_seg;

void text_wipe(void);

// Patch-local copies keep the tail independent from OP_01_TEXT's private data.
// The byte values are the stock gaiji IDs and therefore render identically.
#include "th02/gaiji/ranks_c.c"
static const char gbSTART[10] = { g_chr_5(gb, S,T,A,R,T), '\0' };
static const char gbEXTRA_START[] = {
	g_chr_11(gb, E,X,T,R,A,_,S,T,A,R,T), '\0'
};
static const char gbHISCORE[10] = { g_chr_7(gb, H,I,S,C,O,R,E), '\0' };
static const char gbOPTION[10] = { g_chr_6(gb, O,P,T,I,O,N), '\0' };
static const char gbQUIT[10] = { g_chr_4(gb, Q,U,I,T), '\0' };
static const char gbRANK[10] = { g_chr_4(gb, R,A,N,K), '\0' };
static const char gbMUSIC_MODE[] = {
	g_chr_10(gb, M,U,S,I,C,_,M,O,D,E), '\0'
};

static void cfg_save(void)
{
	t2_language_op_bridge(T2LOB_CFG_SAVE, 0, 0);
}

enum t2op_word_t {
	T2OW_START,
	T2OW_EXTRA,
	T2OW_PRACTICE,
	T2OW_STORY,
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
	T2OW_SLOWDOWN,
	T2OW_LIVES,
	T2OW_BOMBS,
	T2OW_SEED,
	T2OW_SKILL,
	T2OW_RANK_LOCK,
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
	T2OW_DATE,
	T2OW_START_POINT,
	T2OW_STAGE_SPLITS,
	T2OW_START_RUN,
	T2OW_ADVANCED_SETTINGS,
	T2OW_BOSS_ROUND_2,
	T2OW_BOSS_ROUND_3,
	T2OW_BOSS_ROUND_4,
	T2OW_BOSS_ROUND_5,
	T2OW_BOSS_ROUND_6,
	T2OW_BOSS_ROUND_7,
	T2OW_BOSS_PHASE_5,
	T2OW_BOSS_PHASE_7,
	T2OW_BOSS_PHASE_9,
	T2OW_RANDOM,
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
	T2OPC_LIVES,
	T2OPC_BOMBS,
	T2OPC_POWER,
	T2OPC_SCORE,
	T2OPC_SEED,
	T2OPC_RANK,
	T2OPC_RANK_LOCK,
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
static bool t2op_title_pictures_loaded;
bool replay_title_restore_needed;
static bool t2op_title_redraw_needed;
static bool t2op_title_return_fade;
static bool t2op_surface_loaded;
static uint8_t t2op_page_shown;
static uint8_t t2op_main_sel;
static uint8_t t2op_browser_sel;
static uint8_t t2op_practice_sel;
static bool t2op_practice_seed_random;
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
void far replay_practice_diag_boot(unsigned char milestone)
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
	t2op_slot_fn[0] = 'T';
	t2op_slot_fn[1] = 'H';
	t2op_slot_fn[2] = '2';
	t2op_slot_fn[3] = 'R';
	t2op_slot_fn[4] = static_cast<char>('0' + (slot / 10));
	t2op_slot_fn[5] = static_cast<char>('0' + (slot % 10));
	t2op_slot_fn[6] = '.';
	t2op_slot_fn[7] = 'R';
	t2op_slot_fn[8] = 'P';
	t2op_slot_fn[9] = 'Y';
	t2op_slot_fn[10] = '\0';
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

static uint16_t t2op_reserved_u16_get(unsigned offset)
{
	return static_cast<uint16_t>(
		t2op_header.reserved[offset] |
		(static_cast<uint16_t>(t2op_header.reserved[offset + 1]) << 8)
	);
}

static bool t2op_dos_datetime_valid(void)
{
	uint16_t date = t2op_reserved_u16_get(T2REPLAY_RESERVED_DOS_DATE_OFFSET);
	uint16_t time = t2op_reserved_u16_get(T2REPLAY_RESERVED_DOS_TIME_OFFSET);

	if((date == 0) && (time == 0)) {
		return true;
	}
	return (
		(((date >> 5) & 0x0F) >= 1) &&
		(((date >> 5) & 0x0F) <= 12) &&
		((date & 0x1F) >= 1) &&
		((date & 0x1F) <= 31) &&
		((time >> 11) <= 23) &&
		(((time >> 5) & 0x3F) <= 59) &&
		((time & 0x1F) <= 29)
	);
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

	if(practice_target == T2RPT_STAGE_START) {
		practice_target_valid = true;
	} else if(
		(practice_target == T2RPT_STAGE1_CHAPTER2) ||
		((practice_target >= T2RPT_STAGE1_MIDBOSS) &&
		 (practice_target <= T2RPT_STAGE1_BOSS_PHASE3))
	) {
		practice_target_valid = (start->stage == 0);
	} else if(
		(practice_target == T2RPT_STAGE2_CHAPTER2) ||
		((practice_target >= T2RPT_STAGE2_MIDBOSS) &&
		 (practice_target <= T2RPT_STAGE2_BOSS_PHASE3))
	) {
		practice_target_valid = (start->stage == 1);
	} else if(
		(practice_target == T2RPT_STAGE3_CHAPTER2) ||
		(practice_target == T2RPT_STAGE3_MIDBOSS) ||
		(practice_target == T2RPT_STAGE3_BOSS_START) ||
		(practice_target == T2RPT_STAGE3_INNER_PAIR) ||
		(practice_target == T2RPT_STAGE3_OUTER_PAIR) ||
		(practice_target == T2RPT_STAGE3_NORTH_PHASE4)
	) {
		practice_target_valid = (start->stage == 2);
	} else if(
		(practice_target == T2RPT_STAGE4_CHAPTER2) ||
		(practice_target == T2RPT_STAGE4_CHAPTER3) ||
		((practice_target >= T2RPT_STAGE4_MIDBOSS_FIRST) &&
		 (practice_target <= T2RPT_STAGE4_BOSS_START)) ||
		(practice_target == T2RPT_STAGE4_BOSS_PHASE1) ||
		(practice_target == T2RPT_STAGE4_BOSS_ROUND2) ||
		(practice_target == T2RPT_STAGE4_BOSS_ROUND3) ||
		((practice_target >= T2RPT_STAGE4_BOSS_ROUND4) &&
		 (practice_target <= T2RPT_STAGE4_BOSS_ROUND7))
	) {
		practice_target_valid = (start->stage == 3);
	} else if(
		(practice_target == T2RPT_STAGE5_BOSS_START) ||
		(practice_target == T2RPT_STAGE5_BOSS_PHASE1) ||
		(practice_target == T2RPT_STAGE5_BOSS_PHASE3) ||
		(practice_target == T2RPT_STAGE5_BOSS_PHASE5) ||
		(practice_target == T2RPT_STAGE5_BOSS_PHASE7) ||
		(practice_target == T2RPT_STAGE5_BOSS_PHASE9)
	) {
		practice_target_valid = (
			(start->stage == 4) &&
			((practice_target != T2RPT_STAGE5_BOSS_PHASE9) ||
			 (start->continues_used == 0))
		);
	} else if(
		(practice_target == T2RPT_EXTRA_CHAPTER2) ||
		(practice_target == T2RPT_EXTRA_MIDBOSS) ||
		(practice_target == T2RPT_EXTRA_BOSS_START) ||
		(practice_target == T2RPT_EXTRA_BOSS_PHASE1) ||
		(practice_target == T2RPT_EXTRA_BOSS_PHASE3) ||
		(practice_target == T2RPT_EXTRA_BOSS_PHASE5) ||
		(practice_target == T2RPT_EXTRA_BOSS_PHASE7) ||
		(practice_target == T2RPT_EXTRA_BOSS_PHASE9)
	) {
		practice_target_valid = (start->stage == 5);
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
		(t2replay_practice_playperf_decode(
			start->reserved[T2REPLAY_PRACTICE_PLAYPERF_OFFSET]
		) >= -6) &&
		(t2replay_practice_playperf_decode(
			start->reserved[T2REPLAY_PRACTICE_PLAYPERF_OFFSET]
		) <= ((start->rank == RANK_EASY) ? 4 : 16)) &&
		(start->reserved[T2REPLAY_PRACTICE_RANK_LOCK_OFFSET] <= 1) &&
		practice_target_valid &&
		t2op_bytes_zero(
			&start->reserved[T2REPLAY_PRACTICE_RESERVED_OFFSET],
			T2REPLAY_PRACTICE_RESERVED_SIZE
		)
	);
}

static uint16_t t2op_header_wire_size(void)
{
	if(
		(t2op_header.magic[0] != 'T') ||
		(t2op_header.magic[1] != '2') ||
		(t2op_header.magic[2] != 'R') ||
		(t2op_header.magic[3] != 'P') ||
		(t2op_header.magic[4] != 'Y') ||
		(t2op_header.magic[6] != '\0') ||
		(t2op_header.magic[7] != '\0')
	) {
		return 0;
	}
	if(
		(t2op_header.magic[5] == '1') &&
		(t2op_header.version == T2REPLAY_VERSION_LEGACY) &&
		(t2op_header.header_size == T2REPLAY_HEADER_SIZE_LEGACY)
	) {
		return T2REPLAY_HEADER_SIZE_LEGACY;
	}
	if(
		(t2op_header.magic[5] == '2') &&
		(t2op_header.version == T2REPLAY_VERSION_PREVIOUS) &&
		(t2op_header.header_size == T2REPLAY_HEADER_SIZE)
	) {
		return T2REPLAY_HEADER_SIZE;
	}
	if(
		(t2op_header.magic[5] == '3') &&
		(t2op_header.version == T2REPLAY_VERSION) &&
		(t2op_header.header_size == T2REPLAY_HEADER_SIZE)
	) {
		return T2REPLAY_HEADER_SIZE;
	}
	return 0;
}

static bool t2op_header_valid(void)
{
	uint32_t stored = t2op_header.header_checksum;
	uint32_t computed;
	uint16_t wire_size = t2op_header_wire_size();
	uint8_t first_stage;
	uint8_t stage;

	if(
		(wire_size == 0) ||
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
		(t2op_header.input_offset != wire_size) ||
		(t2op_header.input_size > T2REPLAY_INPUT_SIZE_MAX) ||
		(t2op_header.packet_count >
		 (T2REPLAY_INPUT_SIZE_MAX / T2REPLAY_PACKET_SIZE)) ||
		(t2op_header.input_size !=
		 (t2op_header.packet_count * T2REPLAY_PACKET_SIZE)) ||
		(t2op_header.continues_final > 9) ||
		!t2op_start_valid(&t2op_header.start) ||
		!t2op_name_valid(
			t2op_header.reserved + T2REPLAY_RESERVED_NAME_OFFSET
		) ||
		!t2op_dos_datetime_valid() ||
		((wire_size == T2REPLAY_HEADER_SIZE) &&
		 (t2op_header.slow_frames > t2op_header.timed_frames))
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
		T2REPLAY_FNV1A_BASIS, &t2op_header, wire_size
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
	uint16_t wire_size = t2op_header_wire_size();

	t2op_header.header_checksum = 0;
	t2op_header.header_checksum = t2op_fnv1a(
		T2REPLAY_FNV1A_BASIS, &t2op_header, wire_size
	);
}

static bool t2op_pending_header_write(void)
{
	int fd;
	bool written;
	uint16_t wire_size = t2op_header_wire_size();

	if(wire_size == 0) {
		return false;
	}
	fd = t2op_dos_open(t2op_slot_fn, T2OP_DOS_ACCESS_RW);
	if(fd < 0) {
		return false;
	}
	t2op_header_checksum_set();
	written = (
		t2op_dos_seek(fd, 0) &&
		(t2op_dos_write(fd, &t2op_header, wire_size) == wire_size)
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
	uint16_t wire_size;
	int fd = t2op_dos_open(fn, T2OP_DOS_ACCESS_READ);

	if(fd < 0) {
		return false;
	}
	t2op_memclear(&t2op_header, sizeof(t2op_header));
	if(t2op_dos_read(fd, &t2op_header, T2REPLAY_HEADER_SIZE_LEGACY) !=
		T2REPLAY_HEADER_SIZE_LEGACY) {
		t2op_dos_close(fd);
		return false;
	}
	wire_size = t2op_header_wire_size();
	if(
		(wire_size == T2REPLAY_HEADER_SIZE) &&
		(t2op_dos_read(
			fd, reinterpret_cast<uint8_t far *>(&t2op_header) +
			T2REPLAY_HEADER_SIZE_LEGACY,
			(T2REPLAY_HEADER_SIZE - T2REPLAY_HEADER_SIZE_LEGACY)
		) != (T2REPLAY_HEADER_SIZE - T2REPLAY_HEADER_SIZE_LEGACY))
	) {
		t2op_dos_close(fd);
		return false;
	}
	if((wire_size == 0) || !t2op_dos_size(fd, &file_size) ||
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
	uint16_t wire_size;

	if(!file_exist(fn) || !file_ropen(fn)) {
		return false;
	}
	t2op_memclear(&t2op_header, sizeof(t2op_header));
	read = file_read(&t2op_header, T2REPLAY_HEADER_SIZE_LEGACY);
	if(read == T2REPLAY_HEADER_SIZE_LEGACY) {
		wire_size = t2op_header_wire_size();
		if(wire_size == T2REPLAY_HEADER_SIZE) {
			read += file_read(
				reinterpret_cast<uint8_t *>(&t2op_header) +
				T2REPLAY_HEADER_SIZE_LEGACY,
				(T2REPLAY_HEADER_SIZE - T2REPLAY_HEADER_SIZE_LEGACY)
			);
		}
	} else {
		wire_size = 0;
	}
	file_close();
	if((wire_size == 0) || (read != wire_size)) {
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

static char *t2op_word_append(char *p, t2op_word_t word)
{
	// Patch-owned labels deliberately remain English under both locale choices.
	// They are encoded into TH02's menu gaiji below, never into the standard
	// Shift-JIS renderer used by stock text.
	#define P(c) p = t2op_char(p, c)
	switch(word) {
	case T2OW_START: P('S'); P('t'); P('a'); P('r'); P('t'); break;
	case T2OW_EXTRA: P('E'); P('x'); P('t'); P('r'); P('a'); break;
	case T2OW_PRACTICE: P('P'); P('r'); P('a'); P('c'); P('t'); P('i'); P('c'); P('e'); break;
	case T2OW_STORY: P('S'); P('t'); P('o'); P('r'); P('y'); break;
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
	case T2OW_BOSS_ROUND_3: P('B'); P('o'); P('s'); P('s'); P(' '); P('R'); P('o'); P('u'); P('n'); P('d'); P(' '); P('3'); break;
	case T2OW_BOSS_ROUND_4: P('B'); P('o'); P('s'); P('s'); P(' '); P('R'); P('o'); P('u'); P('n'); P('d'); P(' '); P('4'); break;
	case T2OW_BOSS_ROUND_5: P('B'); P('o'); P('s'); P('s'); P(' '); P('R'); P('o'); P('u'); P('n'); P('d'); P(' '); P('5'); break;
	case T2OW_BOSS_ROUND_6: P('B'); P('o'); P('s'); P('s'); P(' '); P('R'); P('o'); P('u'); P('n'); P('d'); P(' '); P('6'); break;
	case T2OW_BOSS_ROUND_7: P('B'); P('o'); P('s'); P('s'); P(' '); P('R'); P('o'); P('u'); P('n'); P('d'); P(' '); P('7'); break;
	case T2OW_BOSS_PHASE_5: P('B'); P('o'); P('s'); P('s'); P(' '); P('P'); P('h'); P('a'); P('s'); P('e'); P(' '); P('5'); break;
	case T2OW_BOSS_PHASE_7: P('B'); P('o'); P('s'); P('s'); P(' '); P('P'); P('h'); P('a'); P('s'); P('e'); P(' '); P('7'); break;
	case T2OW_BOSS_PHASE_9: P('B'); P('o'); P('s'); P('s'); P(' '); P('P'); P('h'); P('a'); P('s'); P('e'); P(' '); P('9'); break;
	case T2OW_RANDOM: P('R'); P('a'); P('n'); P('d'); P('o'); P('m'); break;
	case T2OW_BOSS_START: P('B'); P('o'); P('s'); P('s'); P(' '); P('S'); P('t'); P('a'); P('r'); P('t'); break;
	case T2OW_INNER_PAIR: P('I'); P('n'); P('n'); P('e'); P('r'); P(' '); P('P'); P('a'); P('i'); P('r'); break;
	case T2OW_OUTER_PAIR: P('O'); P('u'); P('t'); P('e'); P('r'); P(' '); P('P'); P('a'); P('i'); P('r'); break;
	case T2OW_SCORE: P('S'); P('c'); P('o'); P('r'); P('e'); break;
	case T2OW_HIGH_SCORE: P('H'); P('i'); P('g'); P('h'); P(' '); P('S'); P('c'); P('o'); P('r'); P('e'); break;
	case T2OW_POWER: P('P'); P('o'); P('w'); P('e'); P('r'); break;
	case T2OW_SLOWDOWN: P('S'); P('l'); P('o'); P('w'); P('d'); P('o'); P('w'); P('n'); break;
	case T2OW_LIVES: P('L'); P('i'); P('v'); P('e'); P('s'); break;
	case T2OW_BOMBS: P('B'); P('o'); P('m'); P('b'); P('s'); break;
	case T2OW_SEED: P('S'); P('e'); P('e'); P('d'); break;
	case T2OW_SKILL: P('S'); P('k'); P('i'); P('l'); P('l'); break;
	case T2OW_RANK_LOCK:
		P('R'); P('a'); P('n'); P('k'); P(' '); P('L'); P('o'); P('c'); P('k');
		break;
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
	case T2OW_DATE: P('D'); P('a'); P('t'); P('e'); break;
	case T2OW_START_POINT: P('S'); P('t'); P('a'); P('r'); P('t'); P(' '); P('P'); P('o'); P('i'); P('n'); P('t'); break;
	case T2OW_STAGE_SPLITS: P('S'); P('t'); P('a'); P('g'); P('e'); P(' '); P('S'); P('p'); P('l'); P('i'); P('t'); P('s'); break;
	case T2OW_START_RUN: P('S'); P('t'); P('a'); P('r'); P('t'); P(' '); P('P'); P('r'); P('a'); P('c'); P('t'); P('i'); P('c'); P('e'); break;
	case T2OW_ADVANCED_SETTINGS: P('A'); P('d'); P('v'); P('a'); P('n'); P('c'); P('e'); P('d'); P(' '); P('S'); P('e'); P('t'); P('t'); P('i'); P('n'); P('g'); P('s'); break;
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

static char *t2op_slowdown_append(char *p)
{
	uint32_t accumulator;
	uint32_t remainder;
	uint32_t threshold;
	uint16_t percent;
	uint8_t i;

	if(
		(t2op_header_wire_size() == T2REPLAY_HEADER_SIZE_LEGACY) ||
		(t2op_header.timed_frames == 0)
	) {
		return t2op_char(p, '-');
	}
	percent = static_cast<uint16_t>(
		(t2op_header.slow_frames / t2op_header.timed_frames) * 100UL
	);
	remainder = (
		t2op_header.slow_frames % t2op_header.timed_frames
	);
	accumulator = 0;
	threshold = (t2op_header.timed_frames - remainder);
	for(i = 0; i < 100; i++) {
		if(accumulator >= threshold) {
			accumulator -= threshold;
			percent++;
		} else {
			accumulator += remainder;
		}
	}
	p = t2op_u32_append(p, percent, 0);
	return t2op_char(p, '%');
}

static void t2op_gaiji_put(
	tram_x_t x, tram_y_t y, tram_atrb2 attr, char *end
);
static void t2op_title_gaiji_put(
	tram_x_t x, tram_y_t y, tram_atrb2 attr, char *end
);

static int t2op_text_color(tram_atrb2 attr)
{
	int color = V_WHITE;

	if(attr == TX_GREEN) {
		color = 9;
	} else if(attr == TX_WHITE) {
		color = 7;
	} else if(attr == TX_BLUE) {
		color = 4;
	}
	return color;
}

static void t2op_text_put(tram_x_t x, tram_y_t y, tram_atrb2 attr, char *end)
{
	*end = '\0';
	if(replay_op_font) {
		replay_op_font_put(
			static_cast<screen_x_t>(x * GLYPH_HALF_W),
			static_cast<vram_y_t>(y * GLYPH_H), t2op_line,
			t2op_text_color(attr)
		);
		return;
	}
	t2op_gaiji_put(x, y, attr, end);
}

static void t2op_text_put_cells(
	tram_x_t x, tram_y_t y, tram_atrb2 attr, char *end, bool numeric
)
{
	*end = '\0';
	if(replay_op_font) {
		if(numeric) {
			replay_op_font_put_numeric_cells(
				static_cast<screen_x_t>(x * GLYPH_HALF_W),
				static_cast<vram_y_t>(y * GLYPH_H), t2op_line,
				static_cast<unsigned>(end - t2op_line),
				t2op_text_color(attr)
			);
		} else {
			replay_op_font_put_cells(
				static_cast<screen_x_t>(x * GLYPH_HALF_W),
				static_cast<vram_y_t>(y * GLYPH_H), t2op_line,
				t2op_text_color(attr)
			);
		}
		return;
	}
	t2op_gaiji_put(x, y, attr, end);
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
	unsigned length = t2op_gaiji_encode(end);
	unsigned i;

	for(i = 0; i < length; i++) {
		if(t2op_line[i] == ' ') {
			text_putca(x, y, ' ', attr);
			text_putca((x + 1), y, ' ', attr);
		} else {
			gaiji_putca(x, y, t2op_gaiji_line[i], attr);
		}
		x += GAIJI_TRAM_W;
	}
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

static unsigned t2op_line_tram_width(char *end)
{
	return static_cast<unsigned>((end - t2op_line) * GAIJI_TRAM_W);
}

static void t2op_title_text_put(
	tram_x_t x, tram_y_t y, tram_atrb2 attr, char *end
)
{
	t2op_title_gaiji_put(x, y, attr, end);
}

static void t2op_menu_word_put(
	tram_x_t x, tram_y_t y, tram_atrb2 attr, char *end
)
{
	t2op_text_put(x, y, attr, end);
}

static void t2op_menu_word_center_put(
	tram_y_t y, tram_atrb2 attr, char *end
)
{
	*end = '\0';
	if(replay_op_font) {
		replay_op_font_put_centered(
			(RES_X / 2), static_cast<vram_y_t>(y * GLYPH_H), t2op_line,
			t2op_text_color(attr)
		);
		return;
	}
	t2op_gaiji_center_put(y, attr, end);
}

static void t2op_title_word_center_put(
	tram_y_t y, tram_atrb2 attr, char *end
)
{
	tram_x_t x;

	x = static_cast<tram_x_t>(
		((RES_X / GLYPH_HALF_W) - t2op_line_tram_width(end)) / 2
	);
	t2op_title_text_put(x, y, attr, end);
}

static void t2op_menu_word_right_put(
	tram_x_t right, tram_y_t y, tram_atrb2 attr, char *end
)
{
	tram_x_t x;

	*end = '\0';
	if(replay_op_font) {
		replay_op_font_put_right(
			static_cast<screen_x_t>(right * GLYPH_HALF_W),
			static_cast<vram_y_t>(y * GLYPH_H), t2op_line,
			t2op_text_color(attr)
		);
		return;
	}
	x = static_cast<tram_x_t>(right - t2op_line_tram_width(end));
	t2op_text_put(x, y, attr, end);
}

static void t2op_word_put_at(
	screen_x_t left, vram_y_t top, tram_atrb2 attr, char *end
)
{
	*end = '\0';
	if(replay_op_font) {
		replay_op_font_put(left, top, t2op_line, t2op_text_color(attr));
	} else {
		t2op_gaiji_put(
			static_cast<tram_x_t>(left / GLYPH_HALF_W),
			static_cast<tram_y_t>(top / GLYPH_H), attr, end
		);
	}
}

static void t2op_word_center_at(vram_y_t top, tram_atrb2 attr, char *end)
{
	*end = '\0';
	if(replay_op_font) {
		replay_op_font_put_centered(
			(RES_X / 2), top, t2op_line, t2op_text_color(attr)
		);
	} else {
		t2op_gaiji_center_put(
			static_cast<tram_y_t>(top / GLYPH_H), attr, end
		);
	}
}

static void t2op_word_right_at(
	screen_x_t right, vram_y_t top, tram_atrb2 attr, char *end
)
{
	*end = '\0';
	if(replay_op_font) {
		replay_op_font_put_right(right, top, t2op_line, t2op_text_color(attr));
	} else {
		t2op_menu_word_right_put(
			static_cast<tram_x_t>(right / GLYPH_HALF_W),
			static_cast<tram_y_t>(top / GLYPH_H), attr, end
		);
	}
}

static void t2op_word_cells_at(
	screen_x_t left, vram_y_t top, tram_atrb2 attr, char *end, bool numeric
)
{
	*end = '\0';
	if(replay_op_font) {
		if(numeric) {
			replay_op_font_put_numeric_cells(
				left, top, t2op_line,
				static_cast<unsigned>(end - t2op_line), t2op_text_color(attr)
			);
		} else {
			replay_op_font_put_cells(
				left, top, t2op_line, t2op_text_color(attr)
			);
		}
	} else {
		t2op_gaiji_put(
			static_cast<tram_x_t>(left / GLYPH_HALF_W),
			static_cast<tram_y_t>(top / GLYPH_H), attr, end
		);
	}
}

static void t2op_word_numeric_cells_right_at(
	screen_x_t right, vram_y_t top, tram_atrb2 attr, char *end
)
{
	unsigned cells = static_cast<unsigned>(end - t2op_line);

	*end = '\0';
	if(replay_op_font) {
		replay_op_font_put_numeric_cells(
			static_cast<screen_x_t>(
				right - (cells * REPLAY_OP_FONT_NUMERIC_CELL_W)
			),
			top, t2op_line, cells, t2op_text_color(attr)
		);
	} else {
		t2op_menu_word_right_put(
			static_cast<tram_x_t>(right / GLYPH_HALF_W),
			static_cast<tram_y_t>(top / GLYPH_H), attr, end
		);
	}
}

static void t2op_slot_cells_at(
	screen_x_t left, vram_y_t top, tram_atrb2 attr, char *end,
	bool single_in_right_cell
)
{
	unsigned count = static_cast<unsigned>(end - t2op_line);
	const char *digit = t2op_line;
	screen_x_t cell_left = left;
	screen_x_t glyph_left;

	*end = '\0';
	if(replay_op_font) {
		if(single_in_right_cell && (count == 1)) {
			cell_left += T2OP_SLOT_CELL_STEP;
		}
		while(count != 0) {
			glyph_left = cell_left;
			if(*digit == '1') {
				glyph_left += T2OP_SLOT_ONE_INSET;
			}
			replay_op_font_put_n(
				glyph_left, top, digit, 1, t2op_text_color(attr)
			);
			cell_left += T2OP_SLOT_CELL_STEP;
			digit++;
			count--;
		}
	} else {
		t2op_gaiji_put(
			static_cast<tram_x_t>(left / GLYPH_HALF_W),
			static_cast<tram_y_t>(top / GLYPH_H), attr, end
		);
	}
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

enum t2op_replay_surface_t {
	T2ORS_BROWSER,
	T2ORS_NAME,
	T2ORS_PRACTICE,
};

static void t2op_title_pictures_free(void)
{
	if(!t2op_title_pictures_loaded) {
		return;
	}
	// TH02's pi_load() does not release the previous contents of a slot.
	// Slot 0 still owns opa.pi after the title animation; leaving it alive here
	// can make the larger Replay and Practice PI loads fail and bounce back to
	// the title. A restored title has a null slot 0, which graph_pi_free()
	// explicitly accepts.
	pi_free(0);
	pi_buffers[0] = 0;
	pi_free(1);
	pi_buffers[1] = 0;
	pi_free(2);
	pi_buffers[2] = 0;
	t2op_title_pictures_loaded = false;
}

static void t2op_title_pictures_load(void)
{
	if(t2op_title_pictures_loaded) {
		return;
	}
	if(pi_load(2, "ts3.pi") != 0) {
		pi_buffers[2] = 0;
		return;
	}
	if(pi_load(1, "ts2.pi") != 0) {
		pi_free(2);
		pi_buffers[2] = 0;
		pi_buffers[1] = 0;
		return;
	}
	t2op_title_pictures_loaded = true;
}

static void t2op_surface_background_restore(void)
{
	text_clear();
	graph_accesspage(1);
	graph_copy_page(0);
	graph_showpage(0);
	graph_accesspage(0);
}

static void t2op_surface_release(void)
{
	if(!t2op_surface_loaded) {
		return;
	}
	pi_free(0);
	pi_buffers[0] = 0;
	t2op_surface_loaded = false;
}

static uint8_t t2op_surface_draw_begin(void)
{
	uint8_t page_drawn = static_cast<uint8_t>(1 - t2op_page_shown);

	text_clear();
	graph_accesspage(page_drawn);
	pi_put_8(0, 0, 0);
	return page_drawn;
}

static void t2op_surface_draw_end(uint8_t page_drawn)
{
	graph_showpage(page_drawn);
	t2op_page_shown = page_drawn;
	graph_accesspage(page_drawn);
}

// Replay browser/detail/confirmation and name entry use distinct patch-owned
// PI files. The active surface stays resident so every update can be drawn in
// full on the hidden page before the CRT page flip.
static bool t2op_replay_surface_prepare(enum t2op_replay_surface_t surface)
{
	const char *fn = (
		(surface == T2ORS_NAME) ? "SLB1B.PI" :
		((surface == T2ORS_PRACTICE) ? "PRACTIC.PI" : "SLB1.PI")
	);

	t2op_surface_release();
	t2op_title_pictures_free();
	replay_op_font_free();
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	replay_practice_diag_boot(51);
#endif
	// Match TH04/TH05's proven ownership order: reserve the small persistent
	// font before decoding the much larger transient PI. Loading it afterward
	// can fail on a fragmented OP heap even though both allocations fit when
	// acquired in this order.
	if((surface != T2ORS_NAME) && !replay_op_font_load()) {
#if T2REPLAY_PRACTICE_DIAGNOSTICS
		replay_practice_diag_boot(55);
#endif
		return false;
	}
	if(pi_load(0, fn) != 0) {
#if T2REPLAY_PRACTICE_DIAGNOSTICS
		replay_practice_diag_boot(52);
#endif
		return false;
	}
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	replay_practice_diag_boot(53);
#endif
	if(t2op_title_return_fade) {
		palette_settone(0);
	}
	pi_palette_apply(0);
	palette_set(
		T2OP_SELECTED_COLOR,
		T2OP_SELECTED_RED,
		T2OP_SELECTED_GREEN,
		T2OP_SELECTED_BLUE
	);
	palette_show();
	graph_accesspage(0);
	pi_put_8(0, 0, 0);
	graph_accesspage(1);
	pi_put_8(0, 0, 0);
	graph_showpage(0);
	graph_accesspage(0);
	t2op_surface_loaded = true;
	t2op_page_shown = 0;
	if(surface == T2ORS_NAME) {
		t2op_title_font_restore();
	}
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	replay_practice_diag_boot(56);
#endif
	text_clear();
	return true;
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
	t2op_surface_release();
	replay_op_font_free();
}

void far replay_title_restore_request(void)
{
	t2op_title_return_request();
}

static void t2op_title_return_request_faded(void)
{
	palette_black_out(1);
	t2op_title_return_fade = true;
	t2op_title_return_request();
}

void far replay_title_redraw_request(void)
{
	// Stock Option and Music Room both draw their shadows into the hidden title
	// page. The PI ownership transaction is now balanced, so a clean title
	// rebuild is both safe and necessary on return.
	t2op_title_return_request();
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
#define T2OP_NAME_ALPHABET_LEFT 23
#define T2OP_NAME_ALPHABET_TOP 18
#define T2OP_NAME_FIELD_LEFT 14
#define T2OP_NAME_FIELD_Y 7
#define T2OP_NAME_POINT_LEFT 46
#define T2OP_NAME_DATE_LEFT 12
#define T2OP_NAME_META_Y 11
#define T2OP_NAME_DIFFICULTY_LEFT 44
#define T2OP_NAME_CHARACTER_CENTER 58
#define T2OP_NAME_STAGE_LEFT 67
#define T2OP_NAME_CELL_LEFT 48
#define T2OP_NAME_CELL_RIGHT 49
#define T2OP_NAME_CELL_END 50

static char *t2op_shot_append(char *p, uint8_t value);

// Byte-for-byte vocabulary of TH02's native High Score gALPHABET. Its
// defining object is linked only by score registration, so OP carries this
// compact source-backed copy rather than adding a cross-executable link edge.
static const uint8_t t2op_name_keyboard[T2OP_NAME_ALPHABET_ROWS][
	T2OP_NAME_ALPHABET_COLS
] = {
	{
		0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2,
		0xB3, 0xB4, 0xB5, 0xB7, 0xB6, 0xB8, 0xB9, 0xBA,
	},
	{
		0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3,
		0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xE0, 0x02, 0x03,
	},
	{
		0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8,
		0xA9, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
	},
};

// This is the native high-score alphabet's exact row-major cell vocabulary.
// TH02's MIKOFT.BFT swaps the physical M/N cels, so the two letters must use
// their named gaiji constants rather than an unqualified alphabetic offset.
static uint8_t t2op_name_keyboard_glyph(uint8_t cell)
{
	return t2op_name_keyboard[cell / T2OP_NAME_ALPHABET_COLS][
		cell % T2OP_NAME_ALPHABET_COLS
	];
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
		if(glyph == gs_SPACE) {
			text_putca((left + (i * GAIJI_TRAM_W)), y, ' ', attr);
			text_putca((left + (i * GAIJI_TRAM_W) + 1), y, ' ', attr);
		} else {
			gaiji_putca(
				(left + (i * GAIJI_TRAM_W)), y, glyph, attr
			);
		}
	}
}

static char *t2op_name_ascii_append(char *p, const uint8_t far *name)
{
	unsigned i;
	unsigned cell;
	uint8_t glyph;

	for(i = 0; i < T2REPLAY_NAME_LEN; i++) {
		glyph = name[i];
		if((glyph == 0) || (glyph == gs_SPACE)) {
			*p++ = ' ';
			continue;
		}
		for(cell = 0; cell <
			(T2OP_NAME_ALPHABET_ROWS * T2OP_NAME_ALPHABET_COLS); cell++) {
			if(t2op_name_keyboard_glyph(static_cast<uint8_t>(cell)) == glyph) {
				break;
			}
		}
		if(cell < 26) {
			*p++ = static_cast<char>('A' + cell);
		} else if((cell >= 34) && (cell < 44)) {
			*p++ = static_cast<char>('0' + (cell - 34));
		} else {
			*p++ = ' ';
		}
	}
	return p;
}

// Mirrors scoredat_name_puts(): the entry row is green and its active
// character remains visible through TH02's native reverse attribute.
static void t2op_name_entry_put(const uint8_t far *name, uint8_t cursor)
{
	uint8_t glyph = (name[cursor] == 0) ? gs_SPACE : name[cursor];

	t2op_name_put(
		T2OP_NAME_FIELD_LEFT, T2OP_NAME_FIELD_Y, name, TX_GREEN
	);
	if(glyph == gs_SPACE) {
		text_putca(
			(T2OP_NAME_FIELD_LEFT + (cursor * GAIJI_TRAM_W)),
			T2OP_NAME_FIELD_Y, ' ', (TX_GREEN | TX_REVERSE)
		);
		text_putca(
			(T2OP_NAME_FIELD_LEFT + (cursor * GAIJI_TRAM_W) + 1),
			T2OP_NAME_FIELD_Y, ' ', (TX_GREEN | TX_REVERSE)
		);
	} else {
		gaiji_putca(
			(T2OP_NAME_FIELD_LEFT + (cursor * GAIJI_TRAM_W)),
			T2OP_NAME_FIELD_Y, glyph, (TX_GREEN | TX_REVERSE)
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

static char *t2op_u32_zero_append(char *p, uint32_t value, unsigned width)
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

static int32_t t2op_final_score_display(void)
{
	return static_cast<int32_t>(
		(static_cast<uint32_t>(t2op_header.score_final) * 10UL) +
		t2op_header.continues_final
	);
}

static void t2op_name_date_put(void)
{
	uint16_t date = t2op_reserved_u16_get(T2REPLAY_RESERVED_DOS_DATE_OFFSET);
	uint16_t year = static_cast<uint16_t>(1980 + (date >> 9));
	uint8_t month = static_cast<uint8_t>((date >> 5) & 0x0F);
	uint8_t day = static_cast<uint8_t>(date & 0x1F);
	char *p;

	p = t2op_u32_zero_append(t2op_line, month, 2);
	t2op_gaiji_put(T2OP_NAME_DATE_LEFT, T2OP_NAME_META_Y, TX_WHITE, p);
	text_putca((T2OP_NAME_DATE_LEFT + 4), T2OP_NAME_META_Y, '-', TX_WHITE);
	p = t2op_u32_zero_append(t2op_line, day, 2);
	t2op_gaiji_put((T2OP_NAME_DATE_LEFT + 6), T2OP_NAME_META_Y, TX_WHITE, p);
	text_putca((T2OP_NAME_DATE_LEFT + 10), T2OP_NAME_META_Y, '-', TX_WHITE);
	p = t2op_u32_zero_append(t2op_line, year, 4);
	t2op_gaiji_put((T2OP_NAME_DATE_LEFT + 12), T2OP_NAME_META_Y, TX_WHITE, p);
}

static void t2op_name_metadata_put(void)
{
	char *p;
	unsigned length;
	tram_x_t left;

	p = t2op_i32_append(t2op_line, t2op_final_score_display(), 10);
	t2op_gaiji_put(T2OP_NAME_POINT_LEFT, T2OP_NAME_FIELD_Y, TX_WHITE, p);
	t2op_name_date_put();
	p = t2op_line;
	if(t2op_header.start.rank == RANK_EASY) {
		*p++ = 'E';
	} else if(t2op_header.start.rank == RANK_NORMAL) {
		*p++ = 'N';
	} else if(t2op_header.start.rank == RANK_HARD) {
		*p++ = 'H';
	} else if(t2op_header.start.rank == RANK_LUNATIC) {
		*p++ = 'L';
	} else {
		*p++ = 'X';
	}
	t2op_gaiji_put(
		T2OP_NAME_DIFFICULTY_LEFT, T2OP_NAME_META_Y, TX_WHITE, p
	);
	p = t2op_shot_append(t2op_line, t2op_header.start.shottype);
	length = static_cast<unsigned>(p - t2op_line);
	left = static_cast<tram_x_t>(
		T2OP_NAME_CHARACTER_CENTER - (length * GAIJI_TRAM_W / 2)
	);
	t2op_gaiji_put(left, T2OP_NAME_META_Y, TX_WHITE, p);
	p = t2op_line;
	if(t2op_header.start.stage == (T2REPLAY_STAGE_COUNT - 1)) {
		*p++ = 'X';
	} else {
		*p++ = static_cast<char>('1' + t2op_header.start.stage);
	}
	t2op_gaiji_put(T2OP_NAME_STAGE_LEFT, T2OP_NAME_META_Y, TX_WHITE, p);
}

static void t2op_name_menu_render(const uint8_t far *name, uint8_t cursor)
{
	text_clear();
	t2op_name_metadata_put();
	t2op_name_entry_put(name, cursor);
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

	if(!t2op_replay_surface_prepare(T2ORS_NAME)) {
		return false;
	}
	for(i = 0; i < T2REPLAY_NAME_LEN; i++) {
		name[i] = gs_SPACE;
	}
	t2op_name_menu_render(name, cursor);
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
				t2op_name_entry_put(name, cursor);
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
				t2op_name_entry_put(name, cursor);
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

static char *t2op_shot_append(char *p, uint8_t value)
{
	p = t2op_word_append(p, T2OW_REIMU);
	return t2op_char(p, static_cast<char>('A' + value));
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

static char *t2op_browser_stage_append(char *p)
{
	if(t2op_header.start.stage == (T2REPLAY_STAGE_COUNT - 1)) {
		*p++ = 'E';
		*p++ = 'X';
		return p;
	}
	if(t2op_header.end_reason == T2REPLAY_END_CLEAR) {
		*p++ = 'A';
		*p++ = 'L';
		*p++ = 'L';
		return p;
	}
	return t2op_u32_append(
		p, static_cast<uint32_t>(t2op_header.stage_reached + 1), 0
	);
}

static char *t2op_date_append(char *p)
{
	uint16_t date = t2op_reserved_u16_get(T2REPLAY_RESERVED_DOS_DATE_OFFSET);
	uint16_t year;

	if(date == 0) {
		return t2op_char(p, '-');
	}
	year = static_cast<uint16_t>(1980 + (date >> 9));
	p = t2op_u32_zero_append(p, ((date >> 5) & 0x0F), 2);
	p = t2op_char(p, '-');
	p = t2op_u32_zero_append(p, (date & 0x1F), 2);
	p = t2op_char(p, '-');
	return t2op_u32_zero_append(p, year, 4);
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

void replay_title_background_prepare_hidden(void)
{
	// Full-screen modal surfaces and the title portraits share OP's PI heap.
	// Keep only the selected locale's title backing while another menu owns the
	// foreground, then restore the portraits after that menu has returned.
	t2op_title_pictures_free();
	replay_op_font_free();
	palette_settone(0);
	graph_accesspage(1);
	t2_language_op_bridge(T2LOB_TITLE_BG_LOAD, 0, 0);
	// pi_load_put_8_free_to() releases slot 0 but master.lib leaves its pointer
	// nonzero. The next PI load would otherwise free the same block again.
	pi_buffers[0] = 0;
	palette_entry_rgb_show(MENU_MAIN_PALETTE_FN);
	graph_accesspage(0);
	t2op_title_font_restore();
}

void replay_title_background_restore(void)
{
	text_clear();
	replay_title_background_prepare_hidden();
	graph_accesspage(1);
	graph_copy_page(0);
	graph_showpage(0);
	graph_accesspage(0);
	t2op_title_pictures_load();
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
		if(t2op_title_return_fade) {
			palette_black_in(1);
			t2op_title_return_fade = false;
		} else {
			palette_100();
		}
	} else if(t2op_title_redraw_needed) {
		t2op_surface_background_restore();
	} else {
		text_clear();
	}
	t2op_title_redraw_needed = false;
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
	t2op_practice_seed_random = true;
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
	t2op_practice.reserved[T2REPLAY_PRACTICE_PLAYPERF_OFFSET] =
		t2replay_practice_playperf_encode(0);
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
			case T2RPT_STAGE_START: return T2RPT_STAGE3_NORTH_PHASE4;
			case T2RPT_STAGE3_MIDBOSS: return T2RPT_STAGE_START;
			case T2RPT_STAGE3_CHAPTER2: return T2RPT_STAGE3_MIDBOSS;
			case T2RPT_STAGE3_BOSS_START: return T2RPT_STAGE3_CHAPTER2;
			case T2RPT_STAGE3_INNER_PAIR: return T2RPT_STAGE3_BOSS_START;
			case T2RPT_STAGE3_OUTER_PAIR: return T2RPT_STAGE3_INNER_PAIR;
			default: return T2RPT_STAGE3_OUTER_PAIR;
			}
		}
		switch(target) {
		case T2RPT_STAGE_START: return T2RPT_STAGE3_MIDBOSS;
		case T2RPT_STAGE3_MIDBOSS: return T2RPT_STAGE3_CHAPTER2;
		case T2RPT_STAGE3_CHAPTER2: return T2RPT_STAGE3_BOSS_START;
		case T2RPT_STAGE3_BOSS_START: return T2RPT_STAGE3_INNER_PAIR;
		case T2RPT_STAGE3_INNER_PAIR: return T2RPT_STAGE3_OUTER_PAIR;
		case T2RPT_STAGE3_OUTER_PAIR: return T2RPT_STAGE3_NORTH_PHASE4;
		default: return T2RPT_STAGE_START;
		}
	case 3:
		if(direction < 0) {
			if(target == T2RPT_STAGE_START) {
				return T2RPT_STAGE4_BOSS_ROUND7;
			}
			if(target == T2RPT_STAGE4_BOSS_ROUND7) {
				return T2RPT_STAGE4_BOSS_ROUND6;
			}
			if(target == T2RPT_STAGE4_BOSS_ROUND6) {
				return T2RPT_STAGE4_BOSS_ROUND5;
			}
			if(target == T2RPT_STAGE4_BOSS_ROUND5) {
				return T2RPT_STAGE4_BOSS_ROUND4;
			}
			if(target == T2RPT_STAGE4_BOSS_ROUND4) {
				return T2RPT_STAGE4_BOSS_ROUND3;
			}
			if(target == T2RPT_STAGE4_BOSS_ROUND3) {
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
		if(target == T2RPT_STAGE4_BOSS_ROUND2) {
			return T2RPT_STAGE4_BOSS_ROUND3;
		}
		if(target == T2RPT_STAGE4_BOSS_ROUND3) {
			return T2RPT_STAGE4_BOSS_ROUND4;
		}
		if(target == T2RPT_STAGE4_BOSS_ROUND4) {
			return T2RPT_STAGE4_BOSS_ROUND5;
		}
		if(target == T2RPT_STAGE4_BOSS_ROUND5) {
			return T2RPT_STAGE4_BOSS_ROUND6;
		}
		if(target == T2RPT_STAGE4_BOSS_ROUND6) {
			return T2RPT_STAGE4_BOSS_ROUND7;
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
				return T2RPT_STAGE5_BOSS_PHASE9;
			}
			if(target == T2RPT_STAGE5_BOSS_PHASE9) {
				return T2RPT_STAGE5_BOSS_PHASE7;
			}
			if(target == T2RPT_STAGE5_BOSS_PHASE7) {
				return T2RPT_STAGE5_BOSS_PHASE5;
			}
			if(target == T2RPT_STAGE5_BOSS_PHASE5) {
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
		if(target == T2RPT_STAGE5_BOSS_PHASE3) {
			return T2RPT_STAGE5_BOSS_PHASE5;
		}
		if(target == T2RPT_STAGE5_BOSS_PHASE5) {
			return T2RPT_STAGE5_BOSS_PHASE7;
		}
		if(target == T2RPT_STAGE5_BOSS_PHASE7) {
			return T2RPT_STAGE5_BOSS_PHASE9;
		}
		if(target == T2RPT_STAGE_START) {
			return T2RPT_STAGE5_BOSS_START;
		}
		return ((target == T2RPT_STAGE5_BOSS_START)
			? T2RPT_STAGE5_BOSS_PHASE1 : T2RPT_STAGE_START);
	case 5:
		if(direction < 0) {
			if(target == T2RPT_STAGE_START) {
				return T2RPT_EXTRA_BOSS_PHASE9;
			}
			if(target == T2RPT_EXTRA_BOSS_PHASE9) {
				return T2RPT_EXTRA_BOSS_PHASE7;
			}
			if(target == T2RPT_EXTRA_BOSS_PHASE7) {
				return T2RPT_EXTRA_BOSS_PHASE5;
			}
			if(target == T2RPT_EXTRA_BOSS_PHASE5) {
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
		if(target == T2RPT_EXTRA_BOSS_PHASE3) {
			return T2RPT_EXTRA_BOSS_PHASE5;
		}
		if(target == T2RPT_EXTRA_BOSS_PHASE5) {
			return T2RPT_EXTRA_BOSS_PHASE7;
		}
		if(target == T2RPT_EXTRA_BOSS_PHASE7) {
			return T2RPT_EXTRA_BOSS_PHASE9;
		}
		if(target == T2RPT_EXTRA_BOSS_PHASE9) {
			return T2RPT_STAGE_START;
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

static int16_t t2op_practice_rank_get(void)
{
	return t2replay_practice_playperf_decode(
		t2op_practice.reserved[T2REPLAY_PRACTICE_PLAYPERF_OFFSET]
	);
}

static void t2op_practice_rank_set(int16_t value)
{
	t2op_practice.reserved[T2REPLAY_PRACTICE_PLAYPERF_OFFSET] =
		t2replay_practice_playperf_encode(value);
}

static int16_t t2op_practice_rank_max(void)
{
	return ((t2op_practice.rank == RANK_EASY) ? 4 : 16);
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
		if(t2op_practice_seed_random) {
			t2op_practice_seed_random = false;
			t2op_practice.resident_frame = (right ? 0 : 0xFFFFFFFFUL);
			t2op_practice.random_seed = t2op_practice.resident_frame;
			break;
		}
		seed = t2op_practice.resident_frame;
		if(right && (seed == 0xFFFFFFFFUL)) {
			t2op_practice_seed_random = true;
			break;
		} else if(!right && (seed == 0)) {
			t2op_practice_seed_random = true;
			break;
		}
		t2op_practice_u32_change(&seed, 0, 0xFFFFFFFFUL,
			(fast ? 256UL : 1UL), right);
		t2op_practice.resident_frame = seed;
		t2op_practice.random_seed = seed;
		break;
	case T2OPC_RANK: {
		int16_t value = t2op_practice_rank_get();
		t2op_practice_i16_change(
			&value, -6, t2op_practice_rank_max(), (fast ? 4 : 1), right
		);
		t2op_practice_rank_set(value);
		break;
	}
	case T2OPC_RANK_LOCK:
		t2op_practice.reserved[T2REPLAY_PRACTICE_RANK_LOCK_OFFSET] ^= 1;
		break;
	default:
		break;
	}
}

static bool t2op_practice_field_is_numeric(uint8_t field)
{
	return ((field >= T2OPC_LIVES) && (field <= T2OPC_RANK));
}

static uint32_t t2op_practice_numeric_get(uint8_t field)
{
	switch(field) {
	case T2OPC_SCORE: return static_cast<uint32_t>(t2op_practice.score);
	case T2OPC_POWER: return t2op_practice.start_power;
	case T2OPC_LIVES: return t2op_practice.start_lives;
	case T2OPC_BOMBS: return t2op_practice.start_bombs;
	case T2OPC_SEED: return t2op_practice.resident_frame;
	case T2OPC_RANK:
		return static_cast<uint32_t>(t2op_practice_rank_get() + 6);
	default: return 0;
	}
}

static uint32_t t2op_practice_numeric_min(uint8_t field)
{
	switch(field) {
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
	case T2OPC_SEED: return 0xFFFFFFFFUL;
	case T2OPC_POWER: return 80;
	case T2OPC_LIVES:
	case T2OPC_BOMBS: return 5;
	case T2OPC_RANK:
		return static_cast<uint32_t>(t2op_practice_rank_max() + 6);
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
		t2op_practice_seed_random = false;
		t2op_practice.resident_frame = value;
		t2op_practice.random_seed = value;
		break;
	case T2OPC_RANK:
		t2op_practice_rank_set(static_cast<int16_t>(value) - 6);
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
	bool original_seed_random = t2op_practice_seed_random;
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
			t2op_practice_seed_random = original_seed_random;
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
		resident->frame++;
		frame_delay(1);
	}
}

static void t2op_practice_render(void)
{
	char *p;
	uint8_t page_drawn;
	uint8_t row;
	vram_y_t y;

	page_drawn = t2op_surface_draw_begin();
	p = t2op_line;
	p = t2op_word_append(p, T2OW_PRACTICE);
	t2op_word_center_at(16, TX_YELLOW, p);

	for(row = 0; row < T2OPC_START; row++) {
		t2op_word_t label;
		y = static_cast<vram_y_t>(60 + (row * 22));
		tram_atrb2 attr = (
			(t2op_practice_sel == row) ? TX_WHITE : TX_YELLOW
		);

		if(t2op_practice_sel == row) {
			p = t2op_char(t2op_line, '>');
			t2op_word_put_at(80, y, attr, p);
		}
		p = t2op_line;
		switch(row) {
		case T2OPC_STAGE: label = T2OW_STAGE; break;
		case T2OPC_SECTION: label = T2OW_SECTION; break;
		case T2OPC_LIVES: label = T2OW_LIVES; break;
		case T2OPC_BOMBS: label = T2OW_BOMBS; break;
		case T2OPC_POWER: label = T2OW_POWER; break;
		case T2OPC_SCORE: label = T2OW_SCORE; break;
		case T2OPC_SEED: label = T2OW_SEED; break;
		case T2OPC_RANK: label = T2OW_RANK; break;
		default: label = T2OW_RANK_LOCK; break;
		}
		p = t2op_word_append(p, label);
		t2op_word_put_at(T2OP_PRACTICE_LABEL_LEFT, y, attr, p);

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
			case T2RPT_STAGE4_BOSS_ROUND3:
				p = t2op_word_append(p, T2OW_BOSS_ROUND_3);
				break;
			case T2RPT_STAGE4_BOSS_ROUND4:
				p = t2op_word_append(p, T2OW_BOSS_ROUND_4);
				break;
			case T2RPT_STAGE4_BOSS_ROUND5:
				p = t2op_word_append(p, T2OW_BOSS_ROUND_5);
				break;
			case T2RPT_STAGE4_BOSS_ROUND6:
				p = t2op_word_append(p, T2OW_BOSS_ROUND_6);
				break;
			case T2RPT_STAGE4_BOSS_ROUND7:
				p = t2op_word_append(p, T2OW_BOSS_ROUND_7);
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
			case T2RPT_STAGE5_BOSS_PHASE5:
			case T2RPT_EXTRA_BOSS_PHASE5:
				p = t2op_word_append(p, T2OW_BOSS_PHASE_5);
				break;
			case T2RPT_STAGE5_BOSS_PHASE7:
			case T2RPT_EXTRA_BOSS_PHASE7:
				p = t2op_word_append(p, T2OW_BOSS_PHASE_7);
				break;
			case T2RPT_STAGE5_BOSS_PHASE9:
			case T2RPT_EXTRA_BOSS_PHASE9:
				p = t2op_word_append(p, T2OW_BOSS_PHASE_9);
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
			case T2RPT_STAGE3_NORTH_PHASE4:
				p = t2op_word_append(p, T2OW_BOSS_PHASE_1);
				break;
			case T2RPT_EXTRA_MIDBOSS:
				p = t2op_word_append(p, T2OW_MIDBOSS);
				break;
			default:
				p = t2op_word_append(p, T2OW_CHAPTER_2);
				break;
			}
			break;
		case T2OPC_LIVES: p = t2op_u32_append(p, t2op_practice.start_lives, 0); break;
		case T2OPC_BOMBS: p = t2op_u32_append(p, t2op_practice.start_bombs, 0); break;
		case T2OPC_POWER: p = t2op_i32_append(p, t2op_practice.start_power, 0); break;
		case T2OPC_SCORE: p = t2op_i32_append(p, t2op_practice.score, 0); break;
		case T2OPC_SEED:
			p = (t2op_practice_seed_random
				? t2op_word_append(p, T2OW_RANDOM)
				: t2op_u32_append(p, t2op_practice.resident_frame, 0));
			break;
		case T2OPC_RANK:
			p = t2op_i32_append(p, t2op_practice_rank_get(), 0);
			break;
		default:
			p = t2op_word_append(
				p,
				t2op_practice.reserved[T2REPLAY_PRACTICE_RANK_LOCK_OFFSET]
					? T2OW_ON : T2OW_OFF
			);
			break;
		}
		t2op_word_right_at(T2OP_PRACTICE_VALUE_RIGHT, y, attr, p);
	}
	p = t2op_line;
	p = t2op_word_append(p, T2OW_START_RUN);
	t2op_word_center_at(static_cast<vram_y_t>(60 + (T2OPC_START * 22)),
		(t2op_practice_sel == T2OPC_START) ? TX_WHITE : TX_YELLOW, p);
	t2op_surface_draw_end(page_drawn);
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
	t2op_surface_release();
	replay_op_font_free();
	pi_load(0, pi_fn);
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	replay_practice_diag_boot(11);
#endif
	text_clear();
	snd_kaja_func(KAJA_SONG_FADE, 15);
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	replay_practice_diag_boot(12);
#endif
	// The proportional font is a patch-owned far allocation. Native OP never
	// knows about it, so release it before the stock gaiji/SUPER/DOS teardown.
	replay_op_font_free();
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	replay_practice_diag_boot(13);
#endif
	gaiji_restore();
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	replay_practice_diag_boot(14);
#endif
	super_free();
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	replay_practice_diag_boot(15);
#endif
	game_exit();
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	replay_practice_diag_boot(16);
#endif
	execl(main_fn, main_fn, nullptr);
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	// execl() does not return on success. Reaching this marker proves a DOS
	// child-load failure rather than a crash inside MAIN.
	replay_practice_diag_boot(17);
#endif
}

void far replay_op_restart_or_snd_load(const char *fn, int func)
{
	t2replay_start_t start;
	uint8_t flags;
	char request_fn[11];

#if T2REPLAY_PRACTICE_DIAGNOSTICS
	replay_practice_diag_boot(31);
#endif
	if(!t2op_restart_command_read(&start, &flags)) {
#if T2REPLAY_PRACTICE_DIAGNOSTICS
		replay_practice_diag_boot(32);
#endif
		if(!t2practice_diag_no_sound()) {
			snd_load(fn, static_cast<snd_load_func_t>(func));
		}
#if T2REPLAY_PRACTICE_DIAGNOSTICS
		replay_practice_diag_boot(33);
#endif
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
	t2_language_op_bridge(T2LOB_START_INIT, 0, 0);
	t2op_resident_apply(&start);
	t2op_main_exec();
}

static void t2op_playback_start(uint8_t slot)
{
	// Playback owns its launch state in the replay header. Preserve the user's
	// title options before start_init() writes the transient resident fields.
	t2_language_op_bridge(T2LOB_CFG_SAVE, 0, 0);
	if(t2op_command_write(T2REPLAY_COMMAND_PLAYBACK, slot, 0, 0)) {
		t2_language_op_bridge(T2LOB_START_INIT, 0, 0);
		t2op_main_exec();
	}
}

static void t2op_practice_start(void)
{
	char request_fn[11];
	uint32_t seed;

	if(t2op_practice_seed_random) {
		seed = static_cast<uint32_t>(resident->frame);
		t2op_practice.resident_frame = seed;
		t2op_practice.random_seed = seed;
	}

	// Keep the selected title options persistent. The Practice payload is a
	// one-run resident override consumed by MAIN, never a new configuration.
	t2_language_op_bridge(T2LOB_CFG_SAVE, 0, 0);
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
	t2_language_op_bridge(T2LOB_START_INIT, 0, 0);
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	replay_practice_diag_boot(7);
#endif
	t2op_resident_apply(&t2op_practice);
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	replay_practice_diag_boot(8);
	t2practice_diag_lifecycle(T2PDLM_OP_DIRECT_LAUNCH, 0, 0, 0);
#endif
	t2op_main_exec();
}

#if T2REPLAY_PRACTICE_DIAGNOSTICS
void far replay_practice_diag_autostart(void)
{
	static const char fn[] = "T2PAUTO.CFG";
	static const char ui_fn[] = "T2UI.CFG";
	uint8_t config[3];
	int fd = t2op_dos_open(ui_fn, T2OP_DOS_ACCESS_READ);

	if(fd >= 0) {
		if(t2op_dos_read(fd, config, 1) == 1) {
			t2op_replay_surface_prepare(
				(config[0] == T2ORS_PRACTICE) ? T2ORS_PRACTICE :
				T2ORS_BROWSER
			);
			t2op_title_return_request();
		}
		t2op_dos_close(fd);
		return;
	}
	fd = t2op_dos_open(fn, T2OP_DOS_ACCESS_READ);

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
	// start_init() performs the native title-song fade before MAIN executes.
	// Keep the configured BGM route for this no-input launch; forcing Off here
	// makes the diagnostic exercise a driver state no interactive Practice uses.
	t2op_practice.bgm_mode = snd_bgm_mode;
	t2op_practice.reserved[T2REPLAY_PRACTICE_TARGET_OFFSET] = config[1];
	if(t2op_start_valid(&t2op_practice)) {
		// Exercise the production launch routine itself. A parallel diagnostic
		// copy previously drifted from the interactive handoff and could stall
		// without proving anything about the public Practice path.
		t2op_practice_start();
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
		t2_language_op_bridge(T2LOB_START_EXTRA, 0, 0);
	} else {
		t2_language_op_bridge(T2LOB_START_GAME, 0, 0);
	}
}

static void t2op_browser_slot_render(uint8_t slot, vram_y_t y)
{
	char *p;
	bool valid = t2op_header_read(slot);
	tram_atrb2 attr = (slot == t2op_browser_sel) ? TX_WHITE : TX_YELLOW;

	if(slot == t2op_browser_sel) {
		p = t2op_char(t2op_line, '>');
		t2op_word_put_at(T2OP_BROWSER_MARKER_LEFT, y, attr, p);
	}
	p = t2op_line;
	p = t2op_u32_append(p, slot, 1);
	t2op_slot_cells_at(T2OP_BROWSER_SLOT_LEFT, y, attr, p, true);
	if(!valid) {
		p = t2op_word_append(
			t2op_line, file_exist(t2op_slot_fn) ? T2OW_INVALID : T2OW_NONE
		);
		t2op_word_put_at(T2OP_BROWSER_NAME_LEFT, y, attr, p);
		return;
	}
	p = t2op_name_ascii_append(
		t2op_line, t2op_header.reserved + T2REPLAY_RESERVED_NAME_OFFSET
	);
	t2op_word_cells_at(T2OP_BROWSER_NAME_LEFT, y, attr, p, false);
	p = t2op_line;
	p = t2op_shot_append(p, t2op_header.start.shottype);
	t2op_word_put_at(T2OP_BROWSER_SHOT_LEFT, y, attr, p);
	p = t2op_line;
	p = t2op_rank_append(p, t2op_header.start.rank);
	t2op_word_put_at(T2OP_BROWSER_RANK_LEFT, y, attr, p);
	p = t2op_line;
	p = t2op_i32_append(p, t2op_final_score_display(), 1);
	t2op_word_numeric_cells_right_at(T2OP_BROWSER_SCORE_RIGHT, y, attr, p);
	p = t2op_line;
	p = t2op_browser_stage_append(p);
	t2op_word_put_at(T2OP_BROWSER_STAGE_LEFT, y, attr, p);
}

static void t2op_browser_render(void)
{
	char *p;
	uint8_t page_drawn;
	uint8_t first = static_cast<uint8_t>(
		(t2op_browser_sel / T2OP_SLOT_ROWS) * T2OP_SLOT_ROWS
	);
	uint8_t i;

	page_drawn = t2op_surface_draw_begin();
	p = t2op_word_append(t2op_line, T2OW_SLOT);
	t2op_word_put_at(T2OP_BROWSER_SLOT_LEFT, 80, TX_YELLOW, p);
	p = t2op_word_append(t2op_line, T2OW_NAME);
	t2op_word_put_at(T2OP_BROWSER_NAME_LEFT, 80, TX_YELLOW, p);
	p = t2op_word_append(t2op_line, T2OW_SHOT);
	t2op_word_put_at(T2OP_BROWSER_SHOT_LEFT, 80, TX_YELLOW, p);
	p = t2op_word_append(t2op_line, T2OW_RANK);
	t2op_word_put_at(T2OP_BROWSER_RANK_LEFT, 80, TX_YELLOW, p);
	p = t2op_word_append(t2op_line, T2OW_SCORE);
	t2op_word_right_at(T2OP_BROWSER_SCORE_RIGHT, 80, TX_YELLOW, p);
	p = t2op_word_append(t2op_line, T2OW_STAGE);
	t2op_word_put_at(T2OP_BROWSER_STAGE_LEFT, 80, TX_YELLOW, p);
	for(i = 0; i < T2OP_SLOT_ROWS; i++) {
		t2op_browser_slot_render(
			static_cast<uint8_t>(first + i),
			static_cast<vram_y_t>(T2OP_BROWSER_LINE_TOP + (i * T2OP_BROWSER_LINE_H))
		);
	}
	p = t2op_line;
	p = t2op_word_append(p, T2OW_PAGE);
	p = t2op_char(p, ' ');
	p = t2op_u32_append(p, ((t2op_browser_sel / T2OP_SLOT_ROWS) + 1), 2);
	p = t2op_char(p, '/');
	p = t2op_u32_append(p, (T2REPLAY_SLOT_COUNT / T2OP_SLOT_ROWS), 2);
	t2op_word_center_at(356, TX_YELLOW, p);
	t2op_surface_draw_end(page_drawn);
}

static void t2op_detail_render(uint8_t slot, bool settings_focus)
{
	char *p;
	uint8_t stage;
	uint8_t page_drawn;
	vram_y_t top;

	page_drawn = t2op_surface_draw_begin();
	if(t2op_name_empty(
		t2op_header.reserved + T2REPLAY_RESERVED_NAME_OFFSET
	)) {
		p = t2op_word_append(t2op_line, T2OW_NONE);
	} else {
		p = t2op_name_ascii_append(
			t2op_line, t2op_header.reserved + T2REPLAY_RESERVED_NAME_OFFSET
		);
	}
	t2op_word_cells_at(
		T2OP_DETAIL_NAME_LEFT, T2OP_DETAIL_TOP, TX_YELLOW, p, false
	);
	p = t2op_word_append(t2op_line, T2OW_SLOT);
	t2op_word_put_at(
		T2OP_DETAIL_SLOT_LABEL_LEFT, T2OP_DETAIL_TOP, TX_GREEN, p
	);
	p = t2op_u32_append(t2op_line, slot, 1);
	t2op_slot_cells_at(
		T2OP_DETAIL_SLOT_LEFT, T2OP_DETAIL_TOP, TX_GREEN, p, false
	);

	p = t2op_line;
	if(t2op_header.flags & T2REPLAY_FLAG_PRACTICE) {
		p = t2op_word_append(p, T2OW_PRACTICE);
	} else {
		p = t2op_word_append(p, T2OW_STORY);
		p = t2op_char(p, ' ');
		p = t2op_char(p, '-');
		p = t2op_char(p, ' ');
		p = t2op_end_reason_append(p, t2op_header.end_reason);
	}
	t2op_word_put_at(T2OP_DETAIL_LEFT, 112, TX_GREEN, p);

	#define T2OP_DETAIL_LABEL(top, word) \
		p = t2op_word_append(t2op_line, word); \
		t2op_word_put_at(T2OP_DETAIL_LEFT, top, TX_YELLOW, p)
	T2OP_DETAIL_LABEL(136, T2OW_FINAL_SCORE);
	p = t2op_i32_append(t2op_line, t2op_final_score_display(), 1);
	t2op_word_numeric_cells_right_at(
		T2OP_DETAIL_VALUE_RIGHT, 136, TX_YELLOW, p
	);
	T2OP_DETAIL_LABEL(160, T2OW_DATE);
	p = t2op_date_append(t2op_line);
	t2op_word_right_at(T2OP_DETAIL_VALUE_RIGHT, 160, TX_YELLOW, p);
	p = t2op_rank_append(t2op_line, t2op_header.start.rank);
	p = t2op_char(p, ' ');
	p = t2op_shot_append(p, t2op_header.start.shottype);
	t2op_word_put_at(T2OP_DETAIL_LEFT, 184, TX_YELLOW, p);
	T2OP_DETAIL_LABEL(208, T2OW_SLOWDOWN);
	p = t2op_slowdown_append(t2op_line);
	t2op_word_right_at(T2OP_DETAIL_VALUE_RIGHT, 208, TX_YELLOW, p);
	if(t2op_header.flags & T2REPLAY_FLAG_PRACTICE) {
		T2OP_DETAIL_LABEL(232, T2OW_START_POINT);
		p = t2op_stage_append(t2op_line, t2op_header.start.stage);
		t2op_word_right_at(T2OP_DETAIL_VALUE_RIGHT, 232, TX_YELLOW, p);
		p = t2op_word_append(t2op_line, T2OW_ADVANCED_SETTINGS);
		t2op_word_put_at(
			T2OP_DETAIL_LEFT, 280,
			settings_focus ? TX_WHITE : TX_GREEN, p
		);
	}
	#undef T2OP_DETAIL_LABEL

	p = t2op_line;
	p = t2op_word_append(p, T2OW_STAGE_SPLITS);
	t2op_word_put_at(
		T2OP_DETAIL_SPLIT_LEFT, T2OP_DETAIL_TOP, TX_GREEN, p
	);
	top = 112;
	for(
		stage = static_cast<uint8_t>(t2op_header.start.stage);
		stage <= t2op_header.stage_reached;
		stage++
	) {
		p = t2op_line;
		p = t2op_stage_append(p, static_cast<int8_t>(stage));
		t2op_word_put_at(T2OP_DETAIL_SPLIT_LEFT, top, TX_YELLOW, p);
		p = t2op_u32_append(t2op_line, t2op_header.stage_scores[stage], 1);
		t2op_word_numeric_cells_right_at(
			T2OP_DETAIL_SPLIT_SCORE_RIGHT, top, TX_YELLOW, p
		);
		top += T2OP_DETAIL_ROW_H;
	}
	t2op_surface_draw_end(page_drawn);
}

static void t2op_detail_settings_render(void)
{
	char *p;
	uint8_t page_drawn = t2op_surface_draw_begin();
	vram_y_t top = 88;

	#define T2OP_SETTINGS_ROW(label, value_code) \
		p = t2op_word_append(t2op_line, label); \
		t2op_word_put_at(112, top, TX_YELLOW, p); \
		p = t2op_line; value_code; \
		t2op_word_right_at(528, top, TX_YELLOW, p); \
		top += 28
	T2OP_SETTINGS_ROW(
		T2OW_LIVES,
		p = t2op_u32_append(p, t2op_header.start.start_lives, 0)
	);
	T2OP_SETTINGS_ROW(
		T2OW_BOMBS,
		p = t2op_u32_append(p, t2op_header.start.start_bombs, 0)
	);
	T2OP_SETTINGS_ROW(
		T2OW_POWER,
		p = t2op_u32_append(p, t2op_header.start.start_power, 0)
	);
	T2OP_SETTINGS_ROW(
		T2OW_SCORE,
		p = t2op_u32_append(p, t2op_header.start.score, 0)
	);
	T2OP_SETTINGS_ROW(
		T2OW_SEED,
		p = t2op_u32_append(p, t2op_header.start.random_seed, 0)
	);
	T2OP_SETTINGS_ROW(
		T2OW_RANK,
		p = t2op_i32_append(
			p, t2replay_practice_playperf_decode(
				t2op_header.start.reserved[T2REPLAY_PRACTICE_PLAYPERF_OFFSET]
			), 0
		)
	);
	T2OP_SETTINGS_ROW(
		T2OW_RANK_LOCK,
		p = t2op_word_append(
			p, t2op_header.start.reserved[T2REPLAY_PRACTICE_RANK_LOCK_OFFSET]
				? T2OW_ON : T2OW_OFF
		)
	);
	#undef T2OP_SETTINGS_ROW
	t2op_surface_draw_end(page_drawn);
}

static void t2op_detail_settings(void)
{
	bool released = false;

	t2op_detail_settings_render();
	while(1) {
		input_reset_sense();
		if(key_det == INPUT_NONE) {
			released = true;
		} else if(released) {
			break;
		}
		frame_delay(1);
	}
}

static void t2op_detail(uint8_t slot)
{
	bool input_allowed = false;
	bool settings_focus = false;
	bool practice = ((t2op_header.flags & T2REPLAY_FLAG_PRACTICE) != 0);

	t2op_detail_render(slot, settings_focus);
	while(1) {
		input_reset_sense();
		if(key_det == INPUT_NONE) {
			input_allowed = true;
		}
		if(input_allowed) {
			if(key_det & INPUT_CANCEL) {
				break;
			} else if(practice && (key_det & INPUT_LEFT)) {
				settings_focus = true;
				t2op_detail_render(slot, settings_focus);
			} else if(practice && (key_det & INPUT_RIGHT)) {
				settings_focus = false;
				t2op_detail_render(slot, settings_focus);
			} else if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
				if(settings_focus) {
					t2op_detail_settings();
					t2op_detail_render(slot, settings_focus);
				} else {
					t2op_playback_start(slot);
				}
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
	uint8_t page_drawn = t2op_surface_draw_begin();

	p = t2op_word_append(t2op_line, T2OW_SAVE_REPLAY);
	p = t2op_char(p, '?');
	t2op_text_put(31, 9, TX_YELLOW, p);
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
	t2op_surface_draw_end(page_drawn);
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
		t2op_detail_render(slot, false);
	} else {
		t2op_browser_render();
	}
	p = t2op_word_append(t2op_line, T2OW_OVERWRITE_REPLAY);
	t2op_text_put(29, 19, TX_YELLOW, p);
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

	if(!t2op_replay_surface_prepare(T2ORS_BROWSER)) {
		t2op_title_return_request();
		return;
	}
	t2op_browser_render();
	while(1) {
		input_reset_sense();
		if(key_det == INPUT_NONE) {
			input_allowed = true;
		}
		if(input_allowed) {
			if(key_det & INPUT_UP) {
				t2op_browser_sel = ((t2op_browser_sel == 0)
					? (T2REPLAY_SLOT_COUNT - 1) : (t2op_browser_sel - 1));
				t2op_browser_render();
			} else if(key_det & INPUT_DOWN) {
				t2op_browser_sel = ((t2op_browser_sel == (T2REPLAY_SLOT_COUNT - 1))
					? 0 : (t2op_browser_sel + 1));
				t2op_browser_render();
			} else if(key_det & INPUT_LEFT) {
				t2op_browser_sel = ((t2op_browser_sel < T2OP_SLOT_ROWS)
					? (t2op_browser_sel + (T2REPLAY_SLOT_COUNT - T2OP_SLOT_ROWS))
					: (t2op_browser_sel - T2OP_SLOT_ROWS));
				t2op_browser_render();
			} else if(key_det & INPUT_RIGHT) {
				t2op_browser_sel = ((t2op_browser_sel >=
					(T2REPLAY_SLOT_COUNT - T2OP_SLOT_ROWS))
					? (t2op_browser_sel - (T2REPLAY_SLOT_COUNT - T2OP_SLOT_ROWS))
					: (t2op_browser_sel + T2OP_SLOT_ROWS));
				t2op_browser_render();
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
						t2op_browser_render();
						t2op_browser_saved_wait();
						break;
					}
					t2op_browser_render();
				} else if(t2op_header_read(t2op_browser_sel)) {
					t2op_detail(t2op_browser_sel);
					t2op_browser_render();
				}
			}
			if(key_det != INPUT_NONE) {
				input_allowed = false;
			}
		}
		frame_delay(1);
	}
	t2op_title_return_request_faded();
	key_det = INPUT_NONE;
}

static bool t2op_pending_save(void)
{
	if(!t2op_pending_header_read()) {
		return false;
	}
	if(!t2op_replay_surface_prepare(T2ORS_BROWSER)) {
		return false;
	}
	if(
		(t2op_pending_source == T2REPLAY_SAVE_REQUEST_CLEAR) &&
		!t2op_save_confirm()
	) {
		t2op_pending_discard();
		t2op_title_return_request();
		return true;
	}
	if(!t2op_name_menu(t2op_pending_name)) {
		t2op_pending_discard();
		t2op_title_return_request();
		return true;
	}
	t2op_browser(true, t2op_pending_name);
	return true;
}

bool far replay_op_pending_save(void)
{
	return t2op_pending_save();
}

static void t2op_practice_menu(void)
{
	bool input_allowed = false;
	uint8_t horizontal_hold = 0;
	bool horizontal_trigger;
	bool right;

	// The title confirmation must not become the character-select confirmation.
	t2op_input_wait_release();

	// Reuse the native selector for the shot choice, then hand off to the same
	// patch-owned Practice surface used by the later games.
	resident->stage = 0;
	sel = 1;
	// Stock OP enters shottype_menu() with ts2.pi and ts3.pi still resident in
	// slots 1 and 2. Only slot 0 is unused here; discarding all three title
	// pictures made the selector decode B/C from freed memory.
	pi_free(0);
	pi_buffers[0] = 0;
	pi_load(0, "ts1.pi");
	text_clear();
	t2_language_op_bridge(T2LOB_SHOTTYPE_MENU, 0, 0);
	// shottype_menu() owns and frees all three selector PI slots. master.lib
	// leaves the freed pointers intact, so invalidate them before any title or
	// patch surface can consider those slots live again.
	pi_buffers[0] = 0;
	pi_buffers[1] = 0;
	pi_buffers[2] = 0;
	t2op_title_pictures_loaded = false;
	if(!t2op_replay_surface_prepare(T2ORS_PRACTICE)) {
		t2op_title_return_request();
		return;
	}
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
		resident->frame++;
		frame_delay(1);
	}
	t2op_title_return_request_faded();
	key_det = INPUT_NONE;
}

void far replay_title_update_and_render(void)
{
	if(!t2op_main_initialized) {
		t2op_main_initialized = true;
		// Stock OP loaded ts2.pi and ts3.pi immediately before entering the
		// title loop. Adopt that ownership exactly once.
		t2op_title_pictures_loaded = true;
		t2op_main_input_allowed = false;
		t2op_main_render();
	} else if(replay_title_restore_needed || t2op_title_redraw_needed) {
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
			break;
		case T2OMC_REPLAY:
			t2op_browser(false, 0);
			break;
		case T2OMC_HISCORE:
			text_clear();
			t2_language_op_bridge(T2LOB_SCORE_MENU, 0, 2000);
			t2op_title_return_request();
			break;
		case T2OMC_OPTIONS:
			menu_sel = 0;
			in_option = true;
			t2op_title_redraw_needed = true;
			break;
		case T2OMC_MUSIC:
			text_clear();
			t2_language_op_bridge(T2LOB_MUSICROOM_MENU, 0, 0);
			// musicroom_menu() also leaves its released slot-0 pointer stale.
			pi_buffers[0] = 0;
			replay_title_redraw_request();
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
		t2_language_op_bridge(T2LOB_START_DEMO, 0, 0);
	}
}

#pragma codeseg

#define T2PRACT_DIAG_OP 1
#include "th02/t2pdiag.cpp"
#undef T2PRACT_DIAG_OP
#define T2M9DIAG_OP 1
#include "th02/t2m9diag.cpp"
#undef T2M9DIAG_OP
