#pragma option -zCREPLAY_TEXT

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th02/hardware/pages.hpp"
#include "th02/hardware/frmdelay.h"
#include "th02/math/randring.hpp"
#include "th03/main/defeat.hpp"
#include "th03/main/difficul.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/main/replay.hpp"
#include "th03/main/round.hpp"
#include "th03/main/score.hpp"
#include "th03/main/v_colors.hpp"
#include "th03/fast_forward.hpp"
#include "th03/keyconfig.hpp"
#include "th03/menu_font.hpp"
#include "th03/practice.hpp"
#include "th03/replay_build.hpp"
#include "th03/replay_format.hpp"
#include "th03/replay_handoff.hpp"
#include "th03/resident.hpp"
#include "th03/replay_protect.hpp"
#include "th03/snd/snd.h"

// Pack the pending packet size and stage-checkpoint state into RLE phase bits.
#define REPLAY_RLE_PHASE_MASK 0x03
#define REPLAY_RLE_PACKET_SIZE_SHIFT 2
#define REPLAY_RLE_PACKET_SIZE_MASK 0x3C
#define REPLAY_RLE_STAGE_CHECKPOINT_PENDING_MASK 0x40
#define REPLAY_RLE_STATE_OPEN 0x01
#define REPLAY_RLE_STATE_CHARGE_SHIFT 1
#define REPLAY_RLE_STATE_CHARGE_MASK 0x06
#define REPLAY_RECORD_BUFFER_SIZE ( \
	T3_REPLAY_WRITE_BUFFER_SIZE + T3R_STAGE_CKPT_PREFIX_SIZE \
)

extern "C" const unsigned char aCOul[];

static char T3_REPLAY_CFG_FN[13];
static char T3_INPUT_FN[12];
static char T3_SPLIT_FN[12];
static char T3_DONE_FN[11];
static char T3_GUARD_DIAG_FN[12];
static char T3_USER_REPLAY_DIR[7];
static char T3_USER_REPLAY_INDEX_FN[16];
static char T3_USER_REPLAY_SLOT_FN[18];
static char T3_USER_REPLAY_FALLBACK_FN[12];

enum replay_text_id_t {
	RTX_CRLF,
	RTX_ERROR,
	RTX_ERROR_SPLIT_OPEN,
	RTX_ERROR_INPUT_CREATE,
	RTX_ERROR_USER_CREATE,
	RTX_ERROR_USER_HEADER,
	RTX_ERROR_INPUT_HEADER,
	RTX_OK_USER_INPUT_END,
	RTX_OK_INPUT_END,
	RTX_ERROR_FRAME_IO,
	RTX_OK_MENU_RETURN,
	RTX_OK_MENU_RETURN_NOSAVE,
	RTX_OK_PARTIAL,
	RTX_OK_USER_PLAYBACK,
	RTX_OK,
};

enum replay_mode_t {
	REPLAY_DISABLED = 0,
	REPLAY_RECORD = 1,
	REPLAY_PLAYBACK = 2,
	REPLAY_USER_RECORD = 3,
	REPLAY_USER_PLAYBACK = 4,
	REPLAY_ERROR = 5,
};

struct replay_input_header_t {
	char magic[8];
	uint16_t version;
	uint16_t header_size;
	uint16_t sample_size;
	uint16_t flags;
	uint32_t sample_count;
	uint32_t initial_resident_rand;
	uint16_t game_mode;
	uint8_t rank;
	uint8_t key_mode;
	uint8_t playchar_p1;
	uint8_t playchar_p2;
	uint8_t story_stage;
	uint8_t reserved_1;
	uint32_t input_crc32;
	uint32_t split_crc32;
	uint32_t reserved_2;
	uint32_t reserved_3;
	uint32_t reserved_4;
	uint32_t reserved_5;
	uint32_t reserved_6;
	uint32_t reserved_7;
};

struct replay_input_sample_t {
	uint32_t frame_index;
	uint32_t round_frame;
	uint16_t round_or_result_frame;
	uint16_t input_mp_p1;
	uint16_t input_mp_p2;
	uint16_t input_sp;
};

static replay_mode_t replay_mode;
static replay_input_header_t replay_header;
static replay_user_header_t replay_user_header;
static replay_user_summary_ext_t replay_user_summary_ext;
static replay_user_snapshot_t replay_user_snapshot;
static replay_user_index_header_t replay_user_index_header;
static replay_user_index_entry_t replay_user_index_entry;
static const char *replay_user_fn;
static uint8_t replay_user_slot;
static uint32_t replay_sample_count;
static uint32_t replay_global_frame;
static uint32_t replay_input_byte_count;
static uint16_t replay_write_buffer_size;
static uint16_t replay_write_buffer_seg;
static uint8_t replay_last_route;
static uint8_t replay_rle_phase;
static uint8_t replay_rle_run;
static uint16_t replay_rle_input_mp_p1;
static uint16_t replay_rle_input_mp_p2;
static uint16_t replay_rle_input_sp;
static bool replay_done_written;
static bool replay_paths_initialized;
static bool replay_prompt_skip_queued;
static uint8_t replay_rle_packet_state;
static bool replay_user_discard_requested;
static bool replay_guard_diag_written;
static bool replay_restart_requested_flag;
static uint16_t replay_sum_flags;
static uint8_t replay_sum_route;
static uint8_t replay_sum_mode;
static uint8_t replay_sum_stage;
static uint8_t replay_sum_round;
static uint8_t replay_sum_winner;
static uint8_t replay_sum_lives;
static uint8_t replay_sum_misses;
static uint8_t replay_sum_stage_count;
static uint8_t replay_sum_stage_opps[T3_REPLAY_USER_STAGE_COUNT];
static uint8_t replay_sum_stage_scores[
	T3_REPLAY_USER_STAGE_COUNT
][T3_REPLAY_USER_PACKED_SCORE_SIZE];

extern "C" unsigned char score[];
extern "C" unsigned char byte_220FC[PLAYER_COUNT];
extern uint8_t byte_23AF9;
extern uint8_t byte_23B00;
extern uint8_t randring_p;
extern uint8_t formation_p[PLAYER_COUNT];
extern uint8_t __seg *formation_type_ring;
extern uint8_t __seg *formation_pos_type_ring;

static replay_mode_t replay_cfg_mode(void);
static replay_mode_t replay_resident_mode(void);
static void replay_paths_init(void);
static void replay_write_text(replay_text_id_t text);
static void replay_handoff_cursor_store(void);
static void replay_user_sample_commit(void);
static bool replay_user_header_is_rle(void);
static bool replay_user_play_sample(void);
static bool replay_user_play_interstitial_sample(void);

static void replay_memclear(void far *buf, unsigned size)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(buf);
	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

extern "C" void far replay_pause_request_poll(void)
{
	if(!(input_sp & INPUT_CANCEL)) {
		resident->unused_3[T3_REPLAY_RES_PAUSE_CANCEL_LATCH_INDEX] = false;
		asm { clc; }
		return;
	}
	if(resident->unused_3[T3_REPLAY_RES_PAUSE_CANCEL_LATCH_INDEX]) {
		asm { clc; }
		return;
	}
	resident->unused_3[T3_REPLAY_RES_PAUSE_CANCEL_LATCH_INDEX] = true;
	asm { stc; }
}

static void replay_fast_forward_wait_skip(bool held)
{
	uint8_t phase;

	if(
		!held ||
		(
			(replay_mode != REPLAY_PLAYBACK) &&
			(replay_mode != REPLAY_USER_PLAYBACK)
		)
	) {
		resident->unused_3[T3_RES_FAST_FORWARD_REPLAY_PHASE_INDEX] = 0;
		return;
	}

	phase = resident->unused_3[T3_RES_FAST_FORWARD_REPLAY_PHASE_INDEX];
	phase++;
	if(phase >= T3_REPLAY_FAST_FORWARD_RATE) {
		resident->unused_3[T3_RES_FAST_FORWARD_REPLAY_PHASE_INDEX] = 0;
		return;
	}
	resident->unused_3[T3_RES_FAST_FORWARD_REPLAY_PHASE_INDEX] = phase;
	vsync_Count1 = byte_23AF9;
}

static void replay_overlay_graph_fill(
	int left, int top, int right, int bottom, int color, int page
)
{
	grc_setclip(0, 0, (RES_X - 1), (SPRITE16_RES_Y - 1));
	grcg_setcolor(GC_RMW, color);
	if(page < 2) {
		graph_accesspage(page);
		grcg_boxfill(left, top, right, bottom);
	} else {
		graph_accesspage(0);
		grcg_boxfill(left, top, right, bottom);
		graph_accesspage(1);
		grcg_boxfill(left, top, right, bottom);
	}
	grcg_off();
	graph_accesspage(page_front);
}

#if defined(TH03_REPLAY_DEV_OVERLAY)
static char __ss *replay_debug_u16_put(
	char __ss *out, unsigned int value
)
{
	unsigned int digit;

#define REPLAY_DEBUG_DIGIT(place) \
	digit = 0; \
	while(value >= place) { \
		value -= place; \
		digit++; \
	} \
	*out++ = static_cast<char>('0' + digit)

	REPLAY_DEBUG_DIGIT(10000);
	REPLAY_DEBUG_DIGIT(1000);
	REPLAY_DEBUG_DIGIT(100);
	REPLAY_DEBUG_DIGIT(10);
	*out++ = static_cast<char>('0' + value);

#undef REPLAY_DEBUG_DIGIT
	return out;
}

static char __ss *replay_debug_u32_put(
	char __ss *out, unsigned long value
)
{
	unsigned int digit;

#define REPLAY_DEBUG_DIGIT(place) \
	digit = 0; \
	while(value >= place) { \
		value -= place; \
		digit++; \
	} \
	*out++ = static_cast<char>('0' + digit)

	REPLAY_DEBUG_DIGIT(1000000000UL);
	REPLAY_DEBUG_DIGIT(100000000UL);
	REPLAY_DEBUG_DIGIT(10000000UL);
	REPLAY_DEBUG_DIGIT(1000000UL);
	REPLAY_DEBUG_DIGIT(100000UL);
	REPLAY_DEBUG_DIGIT(10000UL);
	REPLAY_DEBUG_DIGIT(1000UL);
	REPLAY_DEBUG_DIGIT(100UL);
	REPLAY_DEBUG_DIGIT(10UL);
	*out++ = static_cast<char>('0' + value);

#undef REPLAY_DEBUG_DIGIT
	return out;
}

static void replay_debug_overlay_put(void)
{
	enum {
		TRAM_LEFT = 13,
		TRAM_W = 54,
		PIXEL_LEFT = (TRAM_LEFT * GLYPH_HALF_W),
		PIXEL_RIGHT = (((TRAM_LEFT + TRAM_W) * GLYPH_HALF_W) - 1),
		// MAIN uses doubled 200-line graphics under 16-pixel TRAM glyphs.
		PIXEL_BOTTOM = (GLYPH_HALF_H - 1),
	};
	char line[58];
	char __ss *out = line;

	*out++ = 'r';
	*out++ = 'o';
	*out++ = 'u';
	*out++ = 'n';
	*out++ = 'd';
	*out++ = '_';
	*out++ = 'f';
	*out++ = 'r';
	*out++ = 'a';
	*out++ = 'm';
	*out++ = 'e';
	*out++ = ' ';
	out = replay_debug_u32_put(out, round_frame);
	*out++ = ' ';
	*out++ = 'P';
	*out++ = '2';
	*out++ = ' ';
	*out++ = 'c';
	*out++ = 'p';
	*out++ = 'u';
	*out++ = '_';
	*out++ = 'f';
	*out++ = 'r';
	*out++ = 'a';
	*out++ = 'm';
	*out++ = 'e';
	*out++ = '/';
	*out++ = 's';
	*out++ = 'a';
	*out++ = 'f';
	*out++ = 'e';
	*out++ = 't';
	*out++ = 'y';
	*out++ = ' ';
	out = replay_debug_u16_put(out, players[1].cpu_frame);
	*out++ = '/';
	out = replay_debug_u16_put(out, players[1].cpu_safety_frames);
	*out = '\0';

	replay_overlay_graph_fill(
		PIXEL_LEFT, 0, PIXEL_RIGHT, PIXEL_BOTTOM, V_WHITE, 2
	);
	text_putsa(TRAM_LEFT, 0, line, (TX_BLACK | TX_REVERSE));
}
#endif

void far replay_overlay_put(void)
{
	enum {
#if defined(TH03_REPLAY_DEV_OVERLAY)
		TRAM_LEFT = 38,
		TRAM_TOP = 1,
		TRAM_W = 3,
#else
		TRAM_LEFT = 37,
		TRAM_TOP = 0,
		TRAM_W = 6,
#endif
		PIXEL_LEFT = (TRAM_LEFT * GLYPH_HALF_W),
		PIXEL_TOP = (TRAM_TOP * GLYPH_HALF_H),
		PIXEL_RIGHT = (((TRAM_LEFT + TRAM_W) * GLYPH_HALF_W) - 1),
		PIXEL_BOTTOM = (PIXEL_TOP + GLYPH_HALF_H - 1),
	};
	char line[7];

#if defined(TH03_REPLAY_DEV_OVERLAY)
	replay_debug_overlay_put();
#endif
	if(
		(replay_mode != REPLAY_PLAYBACK) &&
		(replay_mode != REPLAY_USER_PLAYBACK)
	) {
		return;
	}
	line[0] = 'R';
#if defined(TH03_REPLAY_DEV_OVERLAY)
	line[1] = 'P';
	line[2] = 'Y';
	line[3] = '\0';
#else
	line[1] = 'E';
	line[2] = 'P';
	line[3] = 'L';
	line[4] = 'A';
	line[5] = 'Y';
	line[6] = '\0';
#endif

	replay_overlay_graph_fill(
		PIXEL_LEFT, PIXEL_TOP, PIXEL_RIGHT, PIXEL_BOTTOM, V_WHITE, 2
	);
	text_putsa(TRAM_LEFT, TRAM_TOP, line, (TX_BLACK | TX_REVERSE));
}

static void replay_autofire_apply_player(input_t near *input, uint8_t pid)
{
	if(resident->is_cpu[pid]) {
		return;
	}
	if(!(resident->autofire & (1 << pid))) {
		return;
	}
	if(resident->input_charge & (1 << pid)) {
		*input |= INPUT_SHOT;
		return;
	}
	if((*input & INPUT_SHOT) && (byte_220FC[pid] <= 3)) {
		*input &= ~INPUT_SHOT;
	}
}

static void replay_autofire_apply(void)
{
	replay_autofire_apply_player(&input_mp_p1, 0);
	replay_autofire_apply_player(&input_mp_p2, 1);
}

static bool replay_char_ieq(char a, char b)
{
	if((a >= 'A') && (a <= 'Z')) {
		a += ('a' - 'A');
	}
	if((b >= 'A') && (b <= 'Z')) {
		b += ('a' - 'A');
	}
	return (a == b);
}

static void replay_write_bytes(const void far *buf, unsigned size)
{
	file_write(buf, size);
}

static bool replay_write_bytes_checked(const void far *buf, unsigned size)
{
	return (file_write(buf, size) != 0);
}

static void replay_write_char(char c)
{
	replay_write_bytes(&c, 1);
}

static void replay_paths_init(void)
{
	if(replay_paths_initialized) {
		return;
	}

	T3_REPLAY_CFG_FN[0] = 'T';
	T3_REPLAY_CFG_FN[1] = '3';
	T3_REPLAY_CFG_FN[2] = 'R';
	T3_REPLAY_CFG_FN[3] = 'E';
	T3_REPLAY_CFG_FN[4] = 'P';
	T3_REPLAY_CFG_FN[5] = 'L';
	T3_REPLAY_CFG_FN[6] = 'A';
	T3_REPLAY_CFG_FN[7] = 'Y';
	T3_REPLAY_CFG_FN[8] = '.';
	T3_REPLAY_CFG_FN[9] = 'C';
	T3_REPLAY_CFG_FN[10] = 'F';
	T3_REPLAY_CFG_FN[11] = 'G';
	T3_REPLAY_CFG_FN[12] = '\0';

	T3_INPUT_FN[0] = 'T';
	T3_INPUT_FN[1] = '3';
	T3_INPUT_FN[2] = 'I';
	T3_INPUT_FN[3] = 'N';
	T3_INPUT_FN[4] = 'P';
	T3_INPUT_FN[5] = 'U';
	T3_INPUT_FN[6] = 'T';
	T3_INPUT_FN[7] = '.';
	T3_INPUT_FN[8] = 'B';
	T3_INPUT_FN[9] = 'I';
	T3_INPUT_FN[10] = 'N';
	T3_INPUT_FN[11] = '\0';

	T3_SPLIT_FN[0] = 'T';
	T3_SPLIT_FN[1] = '3';
	T3_SPLIT_FN[2] = 'S';
	T3_SPLIT_FN[3] = 'P';
	T3_SPLIT_FN[4] = 'L';
	T3_SPLIT_FN[5] = 'I';
	T3_SPLIT_FN[6] = 'T';
	T3_SPLIT_FN[7] = '.';
	T3_SPLIT_FN[8] = 'B';
	T3_SPLIT_FN[9] = 'I';
	T3_SPLIT_FN[10] = 'N';
	T3_SPLIT_FN[11] = '\0';

	T3_DONE_FN[0] = 'T';
	T3_DONE_FN[1] = '3';
	T3_DONE_FN[2] = 'D';
	T3_DONE_FN[3] = 'O';
	T3_DONE_FN[4] = 'N';
	T3_DONE_FN[5] = 'E';
	T3_DONE_FN[6] = '.';
	T3_DONE_FN[7] = 'T';
	T3_DONE_FN[8] = 'X';
	T3_DONE_FN[9] = 'T';
	T3_DONE_FN[10] = '\0';

	T3_GUARD_DIAG_FN[0] = 'T';
	T3_GUARD_DIAG_FN[1] = '3';
	T3_GUARD_DIAG_FN[2] = 'G';
	T3_GUARD_DIAG_FN[3] = 'D';
	T3_GUARD_DIAG_FN[4] = 'I';
	T3_GUARD_DIAG_FN[5] = 'A';
	T3_GUARD_DIAG_FN[6] = 'G';
	T3_GUARD_DIAG_FN[7] = '.';
	T3_GUARD_DIAG_FN[8] = 'B';
	T3_GUARD_DIAG_FN[9] = 'I';
	T3_GUARD_DIAG_FN[10] = 'N';
	T3_GUARD_DIAG_FN[11] = '\0';

	T3_USER_REPLAY_DIR[0] = 'R';
	T3_USER_REPLAY_DIR[1] = 'E';
	T3_USER_REPLAY_DIR[2] = 'P';
	T3_USER_REPLAY_DIR[3] = 'L';
	T3_USER_REPLAY_DIR[4] = 'A';
	T3_USER_REPLAY_DIR[5] = 'Y';
	T3_USER_REPLAY_DIR[6] = '\0';

	T3_USER_REPLAY_INDEX_FN[0] = 'R';
	T3_USER_REPLAY_INDEX_FN[1] = 'E';
	T3_USER_REPLAY_INDEX_FN[2] = 'P';
	T3_USER_REPLAY_INDEX_FN[3] = 'L';
	T3_USER_REPLAY_INDEX_FN[4] = 'A';
	T3_USER_REPLAY_INDEX_FN[5] = 'Y';
	T3_USER_REPLAY_INDEX_FN[6] = '\\';
	T3_USER_REPLAY_INDEX_FN[7] = 'T';
	T3_USER_REPLAY_INDEX_FN[8] = 'H';
	T3_USER_REPLAY_INDEX_FN[9] = '3';
	T3_USER_REPLAY_INDEX_FN[10] = 'R';
	T3_USER_REPLAY_INDEX_FN[11] = '.';
	T3_USER_REPLAY_INDEX_FN[12] = 'I';
	T3_USER_REPLAY_INDEX_FN[13] = 'D';
	T3_USER_REPLAY_INDEX_FN[14] = 'X';
	T3_USER_REPLAY_INDEX_FN[15] = '\0';

	T3_USER_REPLAY_SLOT_FN[0] = 'R';
	T3_USER_REPLAY_SLOT_FN[1] = 'E';
	T3_USER_REPLAY_SLOT_FN[2] = 'P';
	T3_USER_REPLAY_SLOT_FN[3] = 'L';
	T3_USER_REPLAY_SLOT_FN[4] = 'A';
	T3_USER_REPLAY_SLOT_FN[5] = 'Y';
	T3_USER_REPLAY_SLOT_FN[6] = '\\';
	T3_USER_REPLAY_SLOT_FN[7] = 'T';
	T3_USER_REPLAY_SLOT_FN[8] = 'H';
	T3_USER_REPLAY_SLOT_FN[9] = '3';
	T3_USER_REPLAY_SLOT_FN[10] = 'R';
	T3_USER_REPLAY_SLOT_FN[11] = '0';
	T3_USER_REPLAY_SLOT_FN[12] = '0';
	T3_USER_REPLAY_SLOT_FN[13] = '.';
	T3_USER_REPLAY_SLOT_FN[14] = 'R';
	T3_USER_REPLAY_SLOT_FN[15] = 'P';
	T3_USER_REPLAY_SLOT_FN[16] = 'Y';
	T3_USER_REPLAY_SLOT_FN[17] = '\0';

	T3_USER_REPLAY_FALLBACK_FN[0] = 'T';
	T3_USER_REPLAY_FALLBACK_FN[1] = 'H';
	T3_USER_REPLAY_FALLBACK_FN[2] = '3';
	T3_USER_REPLAY_FALLBACK_FN[3] = 'L';
	T3_USER_REPLAY_FALLBACK_FN[4] = 'A';
	T3_USER_REPLAY_FALLBACK_FN[5] = 'S';
	T3_USER_REPLAY_FALLBACK_FN[6] = 'T';
	T3_USER_REPLAY_FALLBACK_FN[7] = '.';
	T3_USER_REPLAY_FALLBACK_FN[8] = 'R';
	T3_USER_REPLAY_FALLBACK_FN[9] = 'P';
	T3_USER_REPLAY_FALLBACK_FN[10] = 'Y';
	T3_USER_REPLAY_FALLBACK_FN[11] = '\0';

	replay_user_fn = T3_USER_REPLAY_FALLBACK_FN;
	replay_user_slot = T3_REPLAY_USER_SLOT_NONE;
	replay_paths_initialized = true;
}

static void replay_guard_diag_write(void)
{
	if(replay_guard_diag_written) {
		return;
	}
	replay_guard_diag_written = true;
	if(!file_create(T3_GUARD_DIAG_FN)) {
		return;
	}
	file_write(T3_GUARD_DIAG_FN, 4);
	file_write(
		&resident->unused_3[T3_REPLAY_RES_MODE_INDEX],
		(
			(T3_REPLAY_RES_MAINL_VSYNC_INDEX + 2) -
			T3_REPLAY_RES_MODE_INDEX
		)
	);
	file_close();
}

static void replay_write_text(replay_text_id_t text)
{
#define W(c) replay_write_char(c)
	switch(text) {
	case RTX_CRLF:
		W('\r');
		W('\n');
		break;
	case RTX_ERROR:
		W('e'); W('r'); W('r'); W('o'); W('r');
		break;
	case RTX_ERROR_SPLIT_OPEN:
		replay_write_text(RTX_ERROR);
		W(':'); W('s'); W('p'); W('l'); W('i'); W('t'); W('-'); W('o');
		W('p'); W('e'); W('n');
		break;
	case RTX_ERROR_INPUT_CREATE:
		replay_write_text(RTX_ERROR);
		W(':'); W('i'); W('n'); W('p'); W('u'); W('t'); W('-'); W('c');
		W('r'); W('e'); W('a'); W('t'); W('e');
		break;
	case RTX_ERROR_USER_CREATE:
		replay_write_text(RTX_ERROR);
		W(':'); W('u'); W('s'); W('e'); W('r'); W('-'); W('c'); W('r');
		W('e'); W('a'); W('t'); W('e');
		break;
	case RTX_ERROR_USER_HEADER:
		replay_write_text(RTX_ERROR);
		W(':'); W('u'); W('s'); W('e'); W('r'); W('-'); W('h'); W('e');
		W('a'); W('d'); W('e'); W('r');
		break;
	case RTX_ERROR_INPUT_HEADER:
		replay_write_text(RTX_ERROR);
		W(':'); W('i'); W('n'); W('p'); W('u'); W('t'); W('-'); W('h');
		W('e'); W('a'); W('d'); W('e'); W('r');
		break;
	case RTX_OK_USER_INPUT_END:
		replay_write_text(RTX_OK);
		W(':'); W('u'); W('s'); W('e'); W('r'); W('-'); W('i'); W('n');
		W('p'); W('u'); W('t'); W('-'); W('e'); W('n'); W('d');
		break;
	case RTX_OK_INPUT_END:
		replay_write_text(RTX_OK);
		W(':'); W('i'); W('n'); W('p'); W('u'); W('t'); W('-'); W('e');
		W('n'); W('d');
		break;
	case RTX_ERROR_FRAME_IO:
		replay_write_text(RTX_ERROR);
		W(':'); W('f'); W('r'); W('a'); W('m'); W('e'); W('-'); W('i');
		W('o');
		break;
	case RTX_OK_MENU_RETURN:
		replay_write_text(RTX_OK);
		W(':'); W('m'); W('e'); W('n'); W('u'); W('-'); W('r'); W('e');
		W('t'); W('u'); W('r'); W('n');
		break;
	case RTX_OK_MENU_RETURN_NOSAVE:
		replay_write_text(RTX_OK_MENU_RETURN);
		W('-'); W('n'); W('o'); W('s'); W('a'); W('v'); W('e');
		break;
	case RTX_OK_PARTIAL:
		replay_write_text(RTX_OK);
		W(':'); W('p'); W('a'); W('r'); W('t'); W('i'); W('a'); W('l');
		break;
	case RTX_OK_USER_PLAYBACK:
		replay_write_text(RTX_OK);
		W(':'); W('u'); W('s'); W('e'); W('r'); W('-'); W('p'); W('l');
		W('a'); W('y'); W('b'); W('a'); W('c'); W('k');
		break;
	case RTX_OK:
		W('o'); W('k');
		break;
	}
#undef W
}

static void replay_score_pack(
	uint8_t near *packed, const unsigned char near *digits
)
{
	int i;

	for(i = 0; i < T3_REPLAY_USER_PACKED_SCORE_SIZE; i++) {
		packed[i] = static_cast<uint8_t>(
			(digits[(i * 2) + 0] % 10) |
			((digits[(i * 2) + 1] % 10) << 4)
		);
	}
}

static void replay_user_summary_ext_init(void)
{
	replay_memclear(&replay_user_summary_ext, sizeof(replay_user_summary_ext));
	replay_user_summary_ext.flags = T3_REPLAY_USER_SUMMARY_VALID;
}

static uint8_t replay_user_summary_stage_round_pack(void)
{
	uint8_t stage;

	if((resident->game_mode == GM_STORY) || practice_game_active()) {
		stage = resident->story_stage;
		if(stage < T3_REPLAY_USER_STAGE_COUNT) {
			return static_cast<uint8_t>(
				((round_id & 0x0F) << 4) | stage
			);
		}
	}
	return static_cast<uint8_t>(
		((round_id & 0x0F) << 4) | T3_REPLAY_USER_ROUND_STAGE_VS
	);
}

static uint8_t replay_user_summary_route_winner_pack(uint8_t route)
{
	uint8_t winner = T3_REPLAY_USER_ROUND_VALUE_UNKNOWN;

	if(resident->pid_winner == 0) {
		winner = 0;
	} else if(resident->pid_winner == 1) {
		winner = 1;
	}
	return static_cast<uint8_t>(((route & 0x0F) << 4) | winner);
}

static void replay_user_round_split_capture(uint8_t route)
{
	replay_user_round_split_t near *split;

	if(
		(replay_mode != REPLAY_USER_RECORD) ||
		(
			replay_user_summary_ext.round_reached_count >=
			T3_REPLAY_USER_ROUND_SPLIT_COUNT
		)
	) {
		return;
	}

	split = &replay_user_summary_ext.round_splits[
		replay_user_summary_ext.round_reached_count
	];
	split->stage_round = replay_user_summary_stage_round_pack();
	split->route_winner = replay_user_summary_route_winner_pack(route);
	replay_score_pack(split->score_p1, score);
	replay_score_pack(split->score_p2, (score + SCORE_DIGITS));
	replay_user_summary_ext.round_reached_count++;
}

static void replay_user_summary_init_from_snapshot(void)
{
	int i;
	int j;

	replay_sum_flags = T3_REPLAY_USER_SUMMARY_VALID;
	replay_sum_route = T3_REPLAY_USER_SUMMARY_UNKNOWN;
	replay_sum_mode = replay_user_snapshot.game_mode;
	replay_sum_stage = replay_user_snapshot.story_stage;
	replay_sum_round = T3_REPLAY_USER_SUMMARY_UNKNOWN;
	replay_sum_winner = T3_REPLAY_USER_SUMMARY_UNKNOWN;
	replay_sum_lives = replay_user_snapshot.story_lives;
	replay_sum_misses = T3_REPLAY_USER_SUMMARY_UNKNOWN;
	replay_sum_stage_count = 0;
	for(i = 0; i < T3_REPLAY_USER_STAGE_COUNT; i++) {
		replay_sum_stage_opps[i] = (
			replay_user_snapshot.story_opponents[i]
		);
		for(j = 0; j < T3_REPLAY_USER_PACKED_SCORE_SIZE; j++) {
			replay_sum_stage_scores[i][j] = 0;
		}
	}
}

static void replay_user_summary_load_from_header(void)
{
	int i;
	int j;

	replay_user_summary_init_from_snapshot();
	if(
		(replay_user_header.summary_flags & T3_REPLAY_USER_SUMMARY_VALID) == 0
	) {
		return;
	}
	replay_sum_flags = replay_user_header.summary_flags;
	replay_sum_route = replay_user_header.final_route;
	replay_sum_mode = replay_user_header.final_game_mode;
	replay_sum_stage = replay_user_header.final_story_stage;
	replay_sum_round = replay_user_header.final_round_id;
	replay_sum_winner = replay_user_header.final_winner;
	replay_sum_lives = replay_user_header.final_story_lives;
	replay_sum_misses = replay_user_header.final_misses;
	replay_sum_stage_count = (
		replay_user_header.stage_reached_count
	);
	if((replay_user_header.flags & T3_REPLAY_USER_FLAG_PRACTICE) == 0) {
		for(i = 0; i < T3_REPLAY_USER_STAGE_COUNT; i++) {
			replay_sum_stage_opps[i] = (
				replay_user_header.scenario.story.stage_opponents[i]
			);
			for(j = 0; j < T3_REPLAY_USER_PACKED_SCORE_SIZE; j++) {
				replay_sum_stage_scores[i][j] = (
					replay_user_header.scenario.story.stage_scores[i][j]
				);
			}
		}
	}
}

static void replay_user_summary_capture(uint8_t route)
{
	uint8_t stage = resident->story_stage;

	if(replay_mode != REPLAY_USER_RECORD) {
		return;
	}

	replay_sum_flags = T3_REPLAY_USER_SUMMARY_VALID;
	replay_sum_route = route;
	replay_sum_mode = resident->game_mode;
	replay_sum_stage = resident->story_stage;
	replay_sum_round = round_id;
	replay_sum_winner = static_cast<uint8_t>(resident->pid_winner);
	replay_sum_lives = resident->story_lives;
	replay_sum_misses = T3_REPLAY_USER_SUMMARY_UNKNOWN;

	if((resident->game_mode == GM_STORY) && (stage < T3_REPLAY_USER_STAGE_COUNT)) {
		replay_sum_stage_opps[stage] = (
			resident->story_opponents[stage].v
		);
		replay_score_pack(replay_sum_stage_scores[stage], score);
		if(replay_sum_stage_count <= stage) {
			replay_sum_stage_count = (stage + 1);
		}
	}
}

static void replay_user_summary_copy_to_header(void)
{
	int i;
	int j;

	replay_user_header.summary_flags = replay_sum_flags;
	replay_user_header.final_route = replay_sum_route;
	replay_user_header.final_game_mode = replay_sum_mode;
	replay_user_header.final_story_stage = replay_sum_stage;
	replay_user_header.final_round_id = replay_sum_round;
	replay_user_header.final_winner = replay_sum_winner;
	replay_user_header.final_story_lives = replay_sum_lives;
	replay_user_header.final_misses = replay_sum_misses;
	replay_user_header.stage_reached_count = replay_sum_stage_count;
	if((replay_user_header.flags & T3_REPLAY_USER_FLAG_PRACTICE) == 0) {
		for(i = 0; i < T3_REPLAY_USER_STAGE_COUNT; i++) {
			replay_user_header.scenario.story.stage_opponents[i] = (
				replay_sum_stage_opps[i]
			);
			for(j = 0; j < T3_REPLAY_USER_PACKED_SCORE_SIZE; j++) {
				replay_user_header.scenario.story.stage_scores[i][j] = (
					replay_sum_stage_scores[i][j]
				);
			}
		}
	}
	replay_score_pack(replay_user_header.final_score, score);
}

static void replay_dir_create(void)
{
	dos_axdx(0x3900, T3_USER_REPLAY_DIR);
}

static uint8_t replay_resident_slot(void)
{
	uint8_t slot = static_cast<uint8_t>(
		resident->unused_3[T3_REPLAY_RES_SLOT_INDEX]
	);
	if(slot < T3_REPLAY_USER_SLOT_COUNT) {
		return slot;
	}
	return T3_REPLAY_USER_SLOT_NONE;
}

static void replay_user_slot_fn_set(uint8_t slot)
{
	if(slot < T3_REPLAY_USER_SLOT_COUNT) {
		replay_user_slot = slot;
		T3_USER_REPLAY_SLOT_FN[11] = static_cast<char>('0' + (slot / 10));
		T3_USER_REPLAY_SLOT_FN[12] = static_cast<char>('0' + (slot % 10));
		replay_user_fn = T3_USER_REPLAY_SLOT_FN;
	} else {
		replay_user_slot = T3_REPLAY_USER_SLOT_NONE;
		replay_user_fn = T3_USER_REPLAY_FALLBACK_FN;
	}
}

static void replay_user_guard_fn_set(char far *fn)
{
	fn[0] = '\\';
	if(replay_user_slot < T3_REPLAY_USER_SLOT_COUNT) {
		fn[1] = 'T';
		fn[2] = 'H';
		fn[3] = '3';
		fn[4] = 'G';
		fn[5] = static_cast<char>('0' + (replay_user_slot / 10));
		fn[6] = static_cast<char>('0' + (replay_user_slot % 10));
		fn[7] = '.';
		fn[8] = 'T';
		fn[9] = 'M';
		fn[10] = 'P';
		fn[11] = '\0';
	} else {
		fn[1] = 'T';
		fn[2] = 'H';
		fn[3] = '3';
		fn[4] = 'L';
		fn[5] = 'A';
		fn[6] = 'S';
		fn[7] = 'T';
		fn[8] = '.';
		fn[9] = 'G';
		fn[10] = 'R';
		fn[11] = 'D';
		fn[12] = '\0';
	}
}

static bool replay_user_guard_create(void)
{
	char guard_fn[13];

	replay_user_guard_fn_set(guard_fn);
	replay_protect_file_delete_commit(guard_fn);
	return replay_protect_guard_create(guard_fn);
}

static bool replay_user_guard_checkpoint(void)
{
	char guard_fn[13];

	replay_user_guard_fn_set(guard_fn);
	if(!replay_protect_checkpoint(guard_fn)) {
		if(replay_protect_invalid()) {
			replay_protect_guard_marker_set(guard_fn);
		}
		return false;
	}
	return true;
}

static void replay_user_guard_delete(void)
{
	char guard_fn[13];

	replay_user_guard_fn_set(guard_fn);
	replay_protect_file_delete_commit(guard_fn);
}

static void replay_user_index_header_fill(uint8_t next_slot)
{
	replay_memclear(&replay_user_index_header, sizeof(replay_user_index_header));
	replay_user_index_header.magic[0] = 'T';
	replay_user_index_header.magic[1] = '3';
	replay_user_index_header.magic[2] = 'R';
	replay_user_index_header.magic[3] = 'I';
	replay_user_index_header.magic[4] = 'D';
	replay_user_index_header.magic[5] = 'X';
	replay_user_index_header.magic[6] = '7';
	replay_user_index_header.magic[7] = '\0';
	replay_user_index_header.version = T3_REPLAY_USER_INDEX_VERSION;
	replay_user_index_header.header_size = sizeof(replay_user_index_header);
	replay_user_index_header.entry_size = sizeof(replay_user_index_entry);
	replay_user_index_header.slot_count = T3_REPLAY_USER_SLOT_COUNT;
	replay_user_index_header.next_slot = next_slot;
}

static void replay_user_index_entry_fill(
	replay_user_status_t status, replay_user_end_reason_t end_reason
)
{
	int i;

	replay_memclear(&replay_user_index_entry, sizeof(replay_user_index_entry));
	replay_user_index_entry.used = true;
	replay_user_index_entry.slot_id = replay_user_slot;
	replay_user_index_entry.status = status;
	replay_user_index_entry.end_reason = end_reason;
	replay_user_index_entry.game_mode = replay_user_header.game_mode;
	replay_user_index_entry.rank = replay_user_header.rank;
	replay_user_index_entry.key_mode = replay_user_header.key_mode;
	replay_user_index_entry.playchar_p1 = replay_user_header.playchar_p1;
	replay_user_index_entry.playchar_p2 = replay_user_header.playchar_p2;
	replay_user_index_entry.story_stage = replay_user_header.story_stage;
	replay_user_index_entry.is_cpu_p1 = replay_user_header.is_cpu_p1;
	replay_user_index_entry.is_cpu_p2 = replay_user_header.is_cpu_p2;
	replay_user_index_entry.sample_count = replay_user_header.sample_count;
	replay_user_index_entry.final_frame_count = (
		replay_user_header.final_frame_count
	);
	for(i = 0; i < T3_REPLAY_USER_NAME_LEN; i++) {
		replay_user_index_entry.name[i] = replay_user_header.name[i];
	}
	replay_user_index_entry.dos_date = replay_user_header.dos_date;
	replay_user_index_entry.autofire = replay_user_header.autofire;
	replay_user_index_entry.replay_flags = static_cast<uint8_t>(
		replay_user_header.flags & T3_REPLAY_USER_FLAG_PRACTICE
	);
	replay_user_index_entry.summary_flags = replay_user_header.summary_flags;
	replay_user_index_entry.final_route = replay_user_header.final_route;
	replay_user_index_entry.final_story_stage = (
		replay_user_header.final_story_stage
	);
	replay_user_index_entry.final_story_lives = (
		replay_user_header.final_story_lives
	);
	replay_user_index_entry.final_misses = replay_user_header.final_misses;
	replay_user_index_entry.stage_reached_count = (
		replay_user_header.stage_reached_count
	);
	for(i = 0; i < T3_REPLAY_USER_PACKED_SCORE_SIZE; i++) {
		replay_user_index_entry.final_score[i] = replay_user_header.final_score[i];
	}
	if((replay_user_header.flags & T3_REPLAY_USER_FLAG_PRACTICE) == 0) {
		for(i = 0; i < T3_REPLAY_USER_STAGE_COUNT; i++) {
			replay_user_index_entry.stage_opponents[i] = (
				replay_user_header.scenario.story.stage_opponents[i]
			);
		}
	}
}

static bool replay_user_index_create(void)
{
	int slot;

	replay_user_index_header_fill(
		((replay_user_slot + 1) % T3_REPLAY_USER_SLOT_COUNT)
	);
	replay_memclear(&replay_user_index_entry, sizeof(replay_user_index_entry));

	if(!file_create(T3_USER_REPLAY_INDEX_FN)) {
		return false;
	}
	if(!replay_write_bytes_checked(
		&replay_user_index_header, sizeof(replay_user_index_header)
	)) {
		file_close();
		return false;
	}
	for(slot = 0; slot < T3_REPLAY_USER_SLOT_COUNT; slot++) {
		if(!replay_write_bytes_checked(
			&replay_user_index_entry, sizeof(replay_user_index_entry)
		)) {
			file_close();
			return false;
		}
	}
	file_close();
	return true;
}

static bool replay_user_index_slot_write(
	replay_user_status_t status, replay_user_end_reason_t end_reason
)
{
	uint32_t offset;
	bool ret;

	if(replay_user_slot >= T3_REPLAY_USER_SLOT_COUNT) {
		return false;
	}

	replay_dir_create();
	if(!file_append(T3_USER_REPLAY_INDEX_FN)) {
		if(!replay_user_index_create()) {
			return false;
		}
		if(!file_append(T3_USER_REPLAY_INDEX_FN)) {
			return false;
		}
	}

	replay_user_index_entry_fill(status, end_reason);
	offset = (
		static_cast<uint32_t>(sizeof(replay_user_index_header)) +
		(
			static_cast<uint32_t>(replay_user_slot) *
			static_cast<uint32_t>(sizeof(replay_user_index_entry))
		)
	);
	file_seek(offset, SEEK_SET);
	ret = replay_write_bytes_checked(
		&replay_user_index_entry, sizeof(replay_user_index_entry)
	);
	file_close();
	return ret;
}

static bool replay_user_index_slot_clear(void)
{
	uint32_t offset;
	bool ret;

	if(replay_user_slot >= T3_REPLAY_USER_SLOT_COUNT) {
		return false;
	}

	replay_dir_create();
	if(!file_append(T3_USER_REPLAY_INDEX_FN)) {
		return false;
	}

	replay_memclear(&replay_user_index_entry, sizeof(replay_user_index_entry));
	offset = (
		static_cast<uint32_t>(sizeof(replay_user_index_header)) +
		(
			static_cast<uint32_t>(replay_user_slot) *
			static_cast<uint32_t>(sizeof(replay_user_index_entry))
		)
	);
	file_seek(offset, SEEK_SET);
	ret = replay_write_bytes_checked(
		&replay_user_index_entry, sizeof(replay_user_index_entry)
	);
	file_close();
	return ret;
}

static uint32_t replay_hash_u8(uint32_t hash, uint8_t value)
{
	return (((hash << 5) + hash) ^ value);
}

static uint32_t replay_hash_u16(uint32_t hash, uint16_t value)
{
	hash = replay_hash_u8(hash, static_cast<uint8_t>(value));
	hash = replay_hash_u8(hash, static_cast<uint8_t>(value >> 8));
	return hash;
}

static uint32_t replay_hash_u32(uint32_t hash, uint32_t value)
{
	hash = replay_hash_u16(hash, static_cast<uint16_t>(value));
	hash = replay_hash_u16(hash, static_cast<uint16_t>(value >> 16));
	return hash;
}

static uint32_t replay_hash_score(uint32_t hash, const unsigned char near *digits)
{
	int digit;

	for(digit = 0; digit < SCORE_DIGITS; digit++) {
		hash = replay_hash_u8(hash, digits[digit]);
	}
	return hash;
}

static uint32_t replay_hash_bytes(
	uint32_t hash, const void near *buf, unsigned size
)
{
	const uint8_t near *p = reinterpret_cast<const uint8_t near *>(buf);

	while(size != 0) {
		hash = replay_hash_u8(hash, *p++);
		size--;
	}
	return hash;
}

static uint32_t replay_hash_player(
	uint32_t hash, const player_stuff_t near *player
)
{
	hash = replay_hash_u16(hash, player->center.x.v);
	hash = replay_hash_u16(hash, player->center.y.v);
	hash = replay_hash_u8(hash, player->halfhearts);
	hash = replay_hash_u8(hash, player->invincibility_time);
	hash = replay_hash_u8(hash, player->shot_mode);
	hash = replay_hash_u8(hash, player->knockback_time);
	hash = replay_hash_u8(hash, player->move_lock_time);
	hash = replay_hash_u16(hash, player->gauge_charged);
	hash = replay_hash_u16(hash, player->gauge_avail);
	hash = replay_hash_u8(hash, player->bombs);
	hash = replay_hash_u8(hash, player->rounds_won);
	hash = replay_hash_u16(hash, player->cpu_frame);
	hash = replay_hash_u8(hash, player->hit_damage_next);
	hash = replay_hash_u8(hash, player->shot_active);
	hash = replay_hash_u8(hash, player->gauge_attacks_fired);
	hash = replay_hash_u8(hash, player->boss_attacks_fired);
	hash = replay_hash_u8(hash, player->boss_attacks_reversed);
	hash = replay_hash_u8(hash, player->boss_panics_fired);
	return hash;
}

static uint32_t replay_state_hash(void)
{
	uint32_t hash = 5381;

	hash = replay_hash_u32(hash, round_frame);
	hash = replay_hash_u16(hash, round_or_result_frame);
	hash = replay_hash_u8(hash, round_speed);
	hash = replay_hash_u8(hash, defeat_flag);
	hash = replay_hash_u8(hash, resident->pid_winner);
	hash = replay_hash_u16(hash, input_mp_p1);
	hash = replay_hash_u16(hash, input_mp_p2);
	hash = replay_hash_u16(hash, input_sp);
	hash = replay_hash_score(hash, score);
	hash = replay_hash_score(hash, (score + SCORE_DIGITS));
	hash = replay_hash_player(hash, &players[0]);
	hash = replay_hash_player(hash, &players[1]);
	hash = replay_hash_u32(hash, random_seed);
	return hash;
}

static void replay_split_write_header(void)
{
	replay_split_header_t header;

	replay_memclear(&header, sizeof(header));
	header.magic[0] = 'T';
	header.magic[1] = '3';
	header.magic[2] = 'S';
	header.magic[3] = 'P';
	header.magic[4] = 'L';
	header.magic[5] = 'T';
	header.magic[6] = '1';
	header.version = T3_REPLAY_SPLIT_VERSION;
	header.header_size = sizeof(header);
	header.row_size = sizeof(replay_split_row_t);
	if(!file_create(T3_SPLIT_FN)) {
		return;
	}
	file_write(&header, sizeof(header));
	file_close();
}

static void replay_done_write(replay_text_id_t status)
{
	if(replay_done_written) {
		return;
	}
	if(file_create(T3_DONE_FN)) {
		replay_write_text(status);
		replay_write_text(RTX_CRLF);
		file_close();
	}
	replay_done_written = true;
}

static void replay_split_row(replay_split_event_t event, uint8_t route)
{
	replay_split_row_t row;
	int i;

	if(
		(replay_mode == REPLAY_DISABLED) ||
		(replay_mode == REPLAY_ERROR) ||
		(replay_mode == REPLAY_USER_RECORD)
	) {
		return;
	}
	if(!file_append(T3_SPLIT_FN)) {
		replay_mode = REPLAY_ERROR;
		replay_done_write(RTX_ERROR_SPLIT_OPEN);
		return;
	}
	replay_memclear(&row, sizeof(row));
	row.event = event;
	row.route = route;
	row.game_mode = resident->game_mode;
	row.story_stage = resident->story_stage;
	row.round_id = round_id;
	row.winner = resident->pid_winner;
	row.round_speed = round_speed;
	row.global_frame = replay_global_frame;
	row.round_frame = round_frame;
	row.round_or_result_frame = round_or_result_frame;
	for(i = 0; i < T3_REPLAY_USER_PACKED_SCORE_SIZE; i++) {
		row.score_p1[i] = static_cast<uint8_t>(
			(score[(i * 2) + 0] % 10) |
			((score[(i * 2) + 1] % 10) << 4)
		);
		row.score_p2[i] = static_cast<uint8_t>(
			(score[SCORE_DIGITS + (i * 2) + 0] % 10) |
			((score[SCORE_DIGITS + (i * 2) + 1] % 10) << 4)
		);
	}
	row.resident_rand = resident->rand;
	row.state_hash = replay_state_hash();
	if(file_write(&row, sizeof(row)) == 0) {
		replay_mode = REPLAY_ERROR;
	}
	file_close();
}

static void replay_header_fill(void)
{
	replay_memclear(&replay_header, sizeof(replay_header));
	replay_header.magic[0] = 'T';
	replay_header.magic[1] = '3';
	replay_header.magic[2] = 'R';
	replay_header.magic[3] = 'I';
	replay_header.magic[4] = 'N';
	replay_header.magic[5] = 'P';
	replay_header.magic[6] = '1';
	replay_header.magic[7] = '\0';
	replay_header.version = 1;
	replay_header.header_size = sizeof(replay_header);
	replay_header.sample_size = sizeof(replay_input_sample_t);
	replay_header.sample_count = replay_sample_count;
	replay_header.initial_resident_rand = resident->rand;
	replay_header.game_mode = resident->game_mode;
	replay_header.rank = resident->rank;
	replay_header.key_mode = resident->key_mode;
	replay_header.playchar_p1 = resident->playchar_paletted[0].v;
	replay_header.playchar_p2 = resident->playchar_paletted[1].v;
	replay_header.story_stage = resident->story_stage;
}

static void replay_user_snapshot_fill(void)
{
	int i;
	int digit;

	replay_memclear(&replay_user_snapshot, sizeof(replay_user_snapshot));
	replay_user_snapshot.resident_rand = resident->rand;
	replay_user_snapshot.random_seed_snapshot = random_seed;
	replay_user_snapshot.rank = resident->rank;
	replay_user_snapshot.key_mode = resident->key_mode;
	replay_user_snapshot.game_mode = resident->game_mode;
	replay_user_snapshot.story_stage = resident->story_stage;
	replay_user_snapshot.story_lives = resident->story_lives;
	replay_user_snapshot.rem_credits = resident->rem_credits;
	replay_user_snapshot.skill = resident->skill;
	replay_user_snapshot.demo_num = resident->demo_num;
	replay_user_snapshot.pid_winner = resident->pid_winner;
	replay_user_snapshot.show_score_menu = resident->show_score_menu;
	replay_user_snapshot.op_animation_fast = resident->op_animation_fast;
	replay_user_snapshot.autofire = resident->autofire;

	for(i = 0; i < PLAYER_COUNT; i++) {
		replay_user_snapshot.is_cpu[i] = resident->is_cpu[i];
		replay_user_snapshot.playchar_paletted[i] = (
			resident->playchar_paletted[i].v
		);
		for(digit = 0; digit < SCORE_DIGITS; digit++) {
			replay_user_snapshot.score_last[i][digit] = (
				resident->score_last[i].digits[digit]
			);
		}
	}
	for(i = 0; i < STAGE_COUNT; i++) {
		replay_user_snapshot.story_opponents[i] = resident->story_opponents[i].v;
	}
	replay_user_snapshot.randring_p = randring_p;
	for(i = 0; i < RANDRING_SIZE; i++) {
		replay_user_snapshot.randring[i] = randring[i];
	}
	for(i = 0; i < T3_REPLAY_USER_FORMATION_RING_SIZE; i++) {
		replay_user_snapshot.formation_type_ring[i] = formation_type_ring[i];
		replay_user_snapshot.formation_pos_type_ring[i] = formation_pos_type_ring[i];
	}
	for(i = 0; i < PLAYER_COUNT; i++) {
		replay_user_snapshot.formation_p[i] = formation_p[i];
		replay_user_snapshot.cpu_charge_at_avail_ring_p[i] = (
			players[i].cpu_charge_at_avail_ring_p
		);
		for(digit = 0; digit < CHARGE_AT_AVAIL_RING_SIZE; digit++) {
			replay_user_snapshot.cpu_charge_at_avail_ring[i][digit] = (
				players[i].cpu_charge_at_avail_ring[digit]
			);
		}
		replay_user_snapshot.player_center_x[i] = players[i].center.x.v;
		replay_user_snapshot.player_center_y[i] = players[i].center.y.v;
		replay_user_snapshot.player_halfhearts[i] = players[i].halfhearts;
		replay_user_snapshot.player_invincibility_time[i] = (
			players[i].invincibility_time
		);
		replay_user_snapshot.player_gauge_charge_speed[i] = (
			players[i].gauge_charge_speed
		);
		replay_user_snapshot.player_gauge_charged[i] = (
			players[i].gauge_charged
		);
		replay_user_snapshot.player_gauge_avail[i] = players[i].gauge_avail;
		replay_user_snapshot.player_bombs[i] = players[i].bombs;
		replay_user_snapshot.player_shot_active[i] = players[i].shot_active;
		replay_user_snapshot.player_cpu_frame[i] = players[i].cpu_frame;
	}
}

static void replay_user_header_fill(
	replay_user_status_t status, replay_user_end_reason_t end_reason
)
{
	if(replay_user_header.version != T3_REPLAY_USER_VERSION) {
		replay_memclear(&replay_user_header, sizeof(replay_user_header));
		replay_user_header.magic[0] = 'T';
		replay_user_header.magic[1] = '3';
		replay_user_header.magic[2] = 'R';
		replay_user_header.magic[3] = 'P';
		replay_user_header.magic[4] = 'L';
		replay_user_header.magic[5] = 'Y';
		replay_user_header.magic[6] = '1';
		replay_user_header.magic[7] = '1';
		replay_user_header.version = T3_REPLAY_USER_VERSION;
		replay_user_header.header_size = (
			sizeof(replay_user_header) + sizeof(replay_user_summary_ext)
		);
		replay_user_header.sample_size = T3_REPLAY_USER_SAMPLE_SIZE_RLE;
		replay_user_header.flags = (
			T3_REPLAY_USER_FLAG_RLE_INPUT |
			T3_REPLAY_USER_FLAG_CHARGE_INPUT
		);
		if(practice_game_active()) {
			replay_user_header.flags |= T3_REPLAY_USER_FLAG_PRACTICE;
			practice_replay_config_capture(
				replay_user_header.scenario.practice.config,
				players[0].cpu_safety_frames
			);
		}
		replay_user_header.game_mode = replay_user_snapshot.game_mode;
		replay_user_header.rank = replay_user_snapshot.rank;
		replay_user_header.key_mode = replay_user_snapshot.key_mode;
		replay_user_header.playchar_p1 = (
			replay_user_snapshot.playchar_paletted[0]
		);
		replay_user_header.playchar_p2 = (
			replay_user_snapshot.playchar_paletted[1]
		);
		replay_user_header.story_stage = replay_user_snapshot.story_stage;
		replay_user_header.is_cpu_p1 = replay_user_snapshot.is_cpu[0];
		replay_user_header.is_cpu_p2 = replay_user_snapshot.is_cpu[1];
		replay_user_header.resident_rand = replay_user_snapshot.resident_rand;
		replay_user_header.random_seed_snapshot = (
			replay_user_snapshot.random_seed_snapshot
		);
		replay_user_header.snapshot_offset = replay_user_header.header_size;
		replay_user_header.snapshot_size = (
			T3R_STAGE_CKPT_SIZE
		);
		replay_user_header.input_offset = (
			static_cast<uint32_t>(replay_user_header.header_size) +
			static_cast<uint32_t>(
				(replay_user_snapshot.game_mode == GM_STORY) ?
				T3R_STAGE_CKPTS_SIZE : T3R_STAGE_CKPT_SIZE
			)
		);
		replay_user_header.autofire = replay_user_snapshot.autofire;
	}
	replay_user_header.status = status;
	replay_user_header.end_reason = end_reason;
	replay_user_header.sample_count = replay_sample_count;
	replay_user_header.final_frame_count = replay_global_frame;
	replay_user_header.input_size = replay_input_byte_count;
	replay_user_summary_copy_to_header();
}

static uint8_t replay_user_checkpoint_stage(void)
{
	if(
		(replay_user_snapshot.game_mode == GM_STORY) &&
		(replay_user_snapshot.story_stage < T3_REPLAY_USER_STAGE_COUNT)
	) {
		return replay_user_snapshot.story_stage;
	}
	return 0;
}

static uint32_t far *replay_user_checkpoint_cursor(void)
{
	return reinterpret_cast<uint32_t far *>(MK_FP(
		replay_write_buffer_seg, T3_REPLAY_WRITE_BUFFER_SIZE
	));
}

static void replay_user_checkpoint_cursor_capture(void)
{
	if(replay_write_buffer_seg == 0) {
		return;
	}
	uint32_t far *cursor = replay_user_checkpoint_cursor();

	cursor[0] = replay_sample_count;
	cursor[1] = replay_global_frame;
	cursor[2] = replay_input_byte_count;
}

static bool replay_user_checkpoint_write(void)
{
	uint32_t far *cursor = replay_user_checkpoint_cursor();
	uint32_t offset = (
		replay_user_header.snapshot_offset +
		(
			static_cast<uint32_t>(replay_user_checkpoint_stage()) *
			static_cast<uint32_t>(T3R_STAGE_CKPT_SIZE)
		)
	);

	file_seek(offset, SEEK_SET);
	return (
		replay_write_bytes_checked(&cursor[0], sizeof(cursor[0])) &&
		replay_write_bytes_checked(&cursor[1], sizeof(cursor[1])) &&
		replay_write_bytes_checked(&cursor[2], sizeof(cursor[2])) &&
		replay_write_bytes_checked(
			&replay_user_snapshot, sizeof(replay_user_snapshot)
		)
	);
}

static bool replay_header_write(void)
{
	replay_header.sample_count = replay_sample_count;
	if(!file_append(T3_INPUT_FN)) {
		return false;
	}
	file_seek(0, SEEK_SET);
	replay_write_bytes(&replay_header, sizeof(replay_header));
	file_close();
	return true;
}

static bool replay_user_header_write(
	replay_user_status_t status, replay_user_end_reason_t end_reason
)
{
	uint32_t input_offset;
	bool checkpoint_pending = (
		(replay_rle_phase & REPLAY_RLE_STAGE_CHECKPOINT_PENDING_MASK) != 0
	);
	uint8_t far *write_buffer = reinterpret_cast<uint8_t far *>(
		MK_FP(replay_write_buffer_seg, 0)
	);

	if(replay_protect_blocked()) {
		return false;
	}
	if(!replay_user_guard_checkpoint()) {
		return false;
	}
	replay_user_summary_capture(replay_last_route);
	replay_user_header_fill(status, end_reason);
	if(!file_append(replay_user_fn)) {
		replay_protect_detector_error_set();
		return false;
	}
	file_seek(0, SEEK_SET);
	if(!replay_write_bytes_checked(&replay_user_header, sizeof(replay_user_header))) {
		file_close();
		replay_protect_detector_error_set();
		return false;
	}
	if(!replay_write_bytes_checked(
		&replay_user_summary_ext, sizeof(replay_user_summary_ext)
	)) {
		file_close();
		replay_protect_detector_error_set();
		return false;
	}
	if(checkpoint_pending) {
		if(!replay_user_checkpoint_write()) {
			file_close();
			replay_protect_detector_error_set();
			return false;
		}
	}
	if(replay_write_buffer_size != 0) {
		input_offset = (
			replay_user_header.input_offset +
			replay_input_byte_count - replay_write_buffer_size
		);
		file_seek(input_offset, SEEK_SET);
		if(!replay_write_bytes_checked(
			write_buffer, replay_write_buffer_size
		)) {
			file_close();
			replay_protect_detector_error_set();
			return false;
		}
	}
	if(!replay_protect_close_current_file(RPD_COMMIT_FLUSH)) {
		return false;
	}
	replay_rle_phase &= ~REPLAY_RLE_STAGE_CHECKPOINT_PENDING_MASK;
	replay_write_buffer_size = 0;
	replay_rle_packet_state &= ~REPLAY_RLE_STATE_OPEN;
	replay_user_index_slot_write(status, end_reason);
	return true;
}

static bool replay_user_periodic_flush(void)
{
	return replay_user_header_write(RUS_RECORDING, RUER_PARTIAL);
}

static bool replay_user_header_is_rle(void)
{
	return (
		(replay_user_header.magic[6] == '1') &&
		(replay_user_header.magic[7] == '1') &&
		(replay_user_header.version == T3_REPLAY_USER_VERSION) &&
		(replay_user_header.sample_size == T3_REPLAY_USER_SAMPLE_SIZE_RLE) &&
		((replay_user_header.flags & T3_REPLAY_USER_FLAG_RLE_INPUT) != 0) &&
		((replay_user_header.flags & T3_REPLAY_USER_FLAG_CHARGE_INPUT) != 0)
	);
}

static bool replay_user_header_practice_valid(void)
{
	if(replay_user_header.flags & T3_REPLAY_USER_FLAG_PRACTICE) {
		return (
			(replay_user_header.game_mode == GM_VS_1P_CPU) &&
			practice_replay_config_valid(
				replay_user_header.scenario.practice.config
			)
		);
	}
	return true;
}

static bool replay_user_header_valid(void)
{
	return (
		(replay_user_header.magic[0] == 'T') &&
		(replay_user_header.magic[1] == '3') &&
		(replay_user_header.magic[2] == 'R') &&
		(replay_user_header.magic[3] == 'P') &&
		(replay_user_header.magic[4] == 'L') &&
		(replay_user_header.magic[5] == 'Y') &&
		replay_user_header_is_rle() &&
		replay_user_header_practice_valid() &&
		(replay_user_header.header_size == (
			sizeof(replay_user_header) + sizeof(replay_user_summary_ext)
		)) &&
		(replay_user_header.snapshot_offset == replay_user_header.header_size) &&
		(replay_user_header.snapshot_size ==
		 T3R_STAGE_CKPT_SIZE) &&
		(replay_user_header.autofire <= 0x03) &&
		(replay_user_header.input_offset == (
			static_cast<uint32_t>(replay_user_header.header_size) +
			static_cast<uint32_t>(
				(replay_user_header.game_mode == GM_STORY) ?
				T3R_STAGE_CKPTS_SIZE : T3R_STAGE_CKPT_SIZE
			)
		)) &&
		(replay_user_header.sample_count != 0)
	);
}

static bool replay_user_read_from(const char *fn)
{
	replay_user_fn = fn;
	if(!file_ropen(replay_user_fn)) {
		return false;
	}
	if(
		file_read(&replay_user_header, sizeof(replay_user_header)) !=
		sizeof(replay_user_header)
	) {
		file_close();
		return false;
	}
	replay_user_summary_ext_init();
	if(!replay_user_header_valid()) {
		file_close();
		return false;
	}
	if(
		file_read(
			&replay_user_summary_ext, sizeof(replay_user_summary_ext)
		) != sizeof(replay_user_summary_ext)
	) {
		file_close();
		return false;
	}
	if(
		replay_user_summary_ext.round_reached_count >
		T3_REPLAY_USER_ROUND_SPLIT_COUNT
	) {
		file_close();
		return false;
	}
	file_seek((replay_user_header.snapshot_offset + (
		static_cast<uint32_t>(
			(replay_user_header.game_mode == GM_STORY) ?
				replay_user_header.story_stage : 0
		) * static_cast<uint32_t>(T3R_STAGE_CKPT_SIZE)
	) + T3R_STAGE_CKPT_PREFIX_SIZE), SEEK_SET);
	if(
		file_read(&replay_user_snapshot, sizeof(replay_user_snapshot)) !=
		sizeof(replay_user_snapshot)
	) {
		file_close();
		return false;
	}
	if(replay_user_snapshot.autofire != replay_user_header.autofire) {
		file_close();
		return false;
	}
	file_close();
	replay_user_summary_load_from_header();
	return true;
}

static bool replay_user_read(void)
{
	uint8_t slot = replay_resident_slot();

	if(slot < T3_REPLAY_USER_SLOT_COUNT) {
		replay_user_slot_fn_set(slot);
		if(replay_user_read_from(replay_user_fn)) {
			return true;
		}
	}

	if(replay_user_read_from(T3_USER_REPLAY_FALLBACK_FN)) {
		replay_user_slot = T3_REPLAY_USER_SLOT_NONE;
		return true;
	}
	replay_user_slot_fn_set(0);
	return replay_user_read_from(replay_user_fn);
}

static bool replay_user_checkpoint_snapshot_read(uint8_t stage)
{
	uint32_t offset;

	if(
		(stage >= T3_REPLAY_USER_STAGE_COUNT) ||
		(stage >= replay_user_header.stage_reached_count)
	) {
		return false;
	}
	offset = (
		replay_user_header.snapshot_offset +
		(
			static_cast<uint32_t>(stage) *
			static_cast<uint32_t>(T3R_STAGE_CKPT_SIZE)
		) +
		T3R_STAGE_CKPT_PREFIX_SIZE
	);
	if(!file_ropen(replay_user_fn)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	if(
		file_read(&replay_user_snapshot, sizeof(replay_user_snapshot)) !=
		sizeof(replay_user_snapshot)
	) {
		file_close();
		return false;
	}
	file_close();
	return (
		(replay_user_snapshot.game_mode == GM_STORY) &&
		(replay_user_snapshot.story_stage == stage) &&
		(replay_user_snapshot.autofire == replay_user_header.autofire)
	);
}

static void replay_user_snapshot_restore_resident(void)
{
	int i;
	int digit;

	resident->rand = replay_user_snapshot.resident_rand;
	resident->rank = replay_user_snapshot.rank;
	resident->key_mode = replay_user_snapshot.key_mode;
	resident->game_mode = replay_user_snapshot.game_mode;
	resident->story_stage = replay_user_snapshot.story_stage;
	resident->story_lives = replay_user_snapshot.story_lives;
	resident->rem_credits = replay_user_snapshot.rem_credits;
	resident->skill = replay_user_snapshot.skill;
	resident->demo_num = replay_user_snapshot.demo_num;
	resident->pid_winner = replay_user_snapshot.pid_winner;
	resident->show_score_menu = replay_user_snapshot.show_score_menu;
	resident->op_animation_fast = replay_user_snapshot.op_animation_fast;
	resident->autofire = replay_user_snapshot.autofire;

	for(i = 0; i < PLAYER_COUNT; i++) {
		resident->is_cpu[i] = replay_user_snapshot.is_cpu[i];
		resident->playchar_paletted[i].v = (
			replay_user_snapshot.playchar_paletted[i]
		);
		for(digit = 0; digit < SCORE_DIGITS; digit++) {
			resident->score_last[i].digits[digit] = (
				replay_user_snapshot.score_last[i][digit]
			);
		}
	}
	for(i = 0; i < STAGE_COUNT; i++) {
		resident->story_opponents[i].v = replay_user_snapshot.story_opponents[i];
	}
}

static void replay_user_snapshot_restore_runtime(void)
{
	int i;
	int digit;

	random_seed = replay_user_snapshot.random_seed_snapshot;
	randring_p = replay_user_snapshot.randring_p;
	for(i = 0; i < RANDRING_SIZE; i++) {
		randring[i] = replay_user_snapshot.randring[i];
	}
	for(i = 0; i < T3_REPLAY_USER_FORMATION_RING_SIZE; i++) {
		formation_type_ring[i] = replay_user_snapshot.formation_type_ring[i];
		formation_pos_type_ring[i] = (
			replay_user_snapshot.formation_pos_type_ring[i]
		);
	}
	for(i = 0; i < PLAYER_COUNT; i++) {
		formation_p[i] = replay_user_snapshot.formation_p[i];
		players[i].cpu_charge_at_avail_ring_p = (
			replay_user_snapshot.cpu_charge_at_avail_ring_p[i]
		);
		for(digit = 0; digit < CHARGE_AT_AVAIL_RING_SIZE; digit++) {
			players[i].cpu_charge_at_avail_ring[digit] = (
				replay_user_snapshot.cpu_charge_at_avail_ring[i][digit]
			);
		}
		players[i].center.x.v = replay_user_snapshot.player_center_x[i];
		players[i].center.y.v = replay_user_snapshot.player_center_y[i];
		players[i].halfhearts = replay_user_snapshot.player_halfhearts[i];
		players[i].invincibility_time = (
			replay_user_snapshot.player_invincibility_time[i]
		);
		players[i].gauge_charge_speed = (
			replay_user_snapshot.player_gauge_charge_speed[i]
		);
		players[i].gauge_charged = replay_user_snapshot.player_gauge_charged[i];
		players[i].gauge_avail = replay_user_snapshot.player_gauge_avail[i];
		players[i].bombs = replay_user_snapshot.player_bombs[i];
		players[i].shot_active = static_cast<shot_active_t>(
			replay_user_snapshot.player_shot_active[i]
		);
		players[i].cpu_frame = replay_user_snapshot.player_cpu_frame[i];
	}
}

static bool replay_user_create(void)
{
	uint8_t slot = replay_resident_slot();

	replay_user_snapshot_fill();
	replay_user_summary_init_from_snapshot();
	replay_user_summary_ext_init();
	replay_user_checkpoint_cursor_capture();
	replay_user_header_fill(RUS_RECORDING, RUER_PARTIAL);
	replay_rle_phase |= REPLAY_RLE_STAGE_CHECKPOINT_PENDING_MASK;
	replay_user_slot_fn_set(slot);
	if(replay_protect_blocked()) {
		return true;
	}
	if(replay_protect_located()) {
		return true;
	}
	if(slot < T3_REPLAY_USER_SLOT_COUNT) {
		replay_dir_create();
	}
	if(!file_create(replay_user_fn)) {
		replay_user_slot_fn_set(T3_REPLAY_USER_SLOT_NONE);
		if(!file_create(replay_user_fn)) {
			return false;
		}
	}
	if(!replay_protect_close_current_file(RPD_COMMIT_FLUSH)) {
		return false;
	}
	if(!replay_user_guard_create()) {
		replay_guard_diag_write();
		replay_user_index_slot_clear();
		return true;
	}
	return true;
}

static bool replay_record_sample(void)
{
	replay_input_sample_t sample;
	uint32_t offset;

	sample.frame_index = replay_global_frame;
	sample.round_frame = round_frame;
	sample.round_or_result_frame = round_or_result_frame;
	sample.input_mp_p1 = input_mp_p1;
	sample.input_mp_p2 = input_mp_p2;
	sample.input_sp = input_sp;

	offset = (
		static_cast<uint32_t>(sizeof(replay_header)) +
		(replay_sample_count * static_cast<uint32_t>(sizeof(sample)))
	);
	if(!file_append(T3_INPUT_FN)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	replay_write_bytes(&sample, sizeof(sample));
	file_close();
	replay_sample_count++;
	return true;
}

static bool replay_user_record_sample(void)
{
	replay_user_sample_t sample;
	uint32_t offset;

	sample.frame_index = replay_global_frame;
	sample.input_mp_p1 = input_mp_p1;
	sample.input_mp_p2 = input_mp_p2;
	sample.input_sp = input_sp;
	sample.round_or_result_frame = round_or_result_frame;
	sample.round_frame = round_frame;

	offset = (
		replay_user_header.input_offset +
		(replay_sample_count * static_cast<uint32_t>(sizeof(sample)))
	);
	if(!replay_user_guard_checkpoint()) {
		return true;
	}
	if(!file_append(replay_user_fn)) {
		replay_protect_detector_error_set();
		return true;
	}
	file_seek(offset, SEEK_SET);
	if(!replay_write_bytes_checked(&sample, sizeof(sample))) {
		file_close();
		replay_protect_detector_error_set();
		return true;
	}
	file_close();
	replay_sample_count++;
	return true;
}

static bool replay_user_record_interstitial_sample(void)
{
	replay_user_sample_t sample;
	uint32_t offset;

	sample.frame_index = replay_global_frame;
	sample.input_mp_p1 = input_mp_p1;
	sample.input_mp_p2 = input_mp_p2;
	sample.input_sp = input_sp;
	sample.round_or_result_frame = T3_REPLAY_INTERSTITIAL_ROUND_OR_RESULT_FRAME;
	sample.round_frame = T3_REPLAY_INTERSTITIAL_ROUND_FRAME;

	offset = (
		replay_user_header.input_offset +
		(replay_sample_count * static_cast<uint32_t>(sizeof(sample)))
	);
	if(!replay_user_guard_checkpoint()) {
		return true;
	}
	if(!file_append(replay_user_fn)) {
		replay_protect_detector_error_set();
		return true;
	}
	file_seek(offset, SEEK_SET);
	if(!replay_write_bytes_checked(&sample, sizeof(sample))) {
		file_close();
		replay_protect_detector_error_set();
		return true;
	}
	file_close();
	replay_sample_count++;
	return true;
}

static uint8_t replay_user_rle_tag(uint8_t phase, uint8_t run)
{
	return static_cast<uint8_t>(
		(phase << T3_REPLAY_PACKET_PHASE_SHIFT) | (run - 1)
	);
}

static bool replay_user_buffer_u8(uint8_t value)
{
	uint8_t far *write_buffer = reinterpret_cast<uint8_t far *>(
		MK_FP(replay_write_buffer_seg, 0)
	);

	if(replay_write_buffer_size >= T3_REPLAY_WRITE_BUFFER_SIZE) {
		return false;
	}
	write_buffer[replay_write_buffer_size++] = value;
	replay_input_byte_count++;
	return true;
}

static bool replay_user_buffer_u16(uint16_t value)
{
	uint8_t far *write_buffer = reinterpret_cast<uint8_t far *>(
		MK_FP(replay_write_buffer_seg, 0)
	);

	if(
		(replay_write_buffer_size + sizeof(value)) >
		T3_REPLAY_WRITE_BUFFER_SIZE
	) {
		return false;
	}
	write_buffer[replay_write_buffer_size++] = static_cast<uint8_t>(value);
	write_buffer[replay_write_buffer_size++] = static_cast<uint8_t>(
		value >> 8
	);
	replay_input_byte_count += sizeof(value);
	return true;
}

static bool replay_user_control_write(uint8_t control)
{
	uint8_t tag = static_cast<uint8_t>(
		(T3_REPLAY_PACKET_PHASE_CONTROL << T3_REPLAY_PACKET_PHASE_SHIFT) |
		control
	);
	uint8_t marker = T3_REPLAY_PACKET_CONTROL_MARKER;
	if(replay_protect_blocked()) {
		return true;
	}
	if(
		((replay_write_buffer_size + 2) > T3_REPLAY_WRITE_BUFFER_SIZE) &&
		!replay_user_periodic_flush()
	) {
		return false;
	}
	if(!replay_user_buffer_u8(tag) || !replay_user_buffer_u8(marker)) {
		replay_protect_detector_error_set();
		return false;
	}
	replay_rle_packet_state &= ~REPLAY_RLE_STATE_OPEN;
	return true;
}

static bool replay_user_control_consume(uint8_t control)
{
	uint8_t tag;
	uint8_t marker;
	uint32_t offset = (replay_user_header.input_offset + replay_input_byte_count);

	if(
		(replay_rle_run != 0) ||
		((replay_input_byte_count + 2) > replay_user_header.input_size)
	) {
		return false;
	}
	if(!file_ropen(replay_user_fn)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	if(
		(file_read(&tag, sizeof(tag)) != sizeof(tag)) ||
		(file_read(&marker, sizeof(marker)) != sizeof(marker))
	) {
		file_close();
		return false;
	}
	file_close();
	if(
		(tag != static_cast<uint8_t>(
			(T3_REPLAY_PACKET_PHASE_CONTROL << T3_REPLAY_PACKET_PHASE_SHIFT) |
			control
		)) ||
		(marker != T3_REPLAY_PACKET_CONTROL_MARKER)
	) {
		return false;
	}
	replay_input_byte_count += 2;
	return true;
}

static bool replay_user_record_rle_sample(uint8_t phase, uint8_t shot_bits)
{
	uint16_t input_p1 = input_mp_p1;
	uint16_t input_p2 = input_mp_p2;
	uint16_t input_single = input_sp;
	uint8_t input_charge = (resident->input_charge & 0x03);
	uint8_t previous_charge = static_cast<uint8_t>(
		(replay_rle_packet_state & REPLAY_RLE_STATE_CHARGE_MASK) >>
		REPLAY_RLE_STATE_CHARGE_SHIFT
	);
	uint8_t packet_size = static_cast<uint8_t>(
		(replay_rle_phase & REPLAY_RLE_PACKET_SIZE_MASK) >>
		REPLAY_RLE_PACKET_SIZE_SHIFT
	);
	uint8_t far *write_buffer = reinterpret_cast<uint8_t far *>(
		MK_FP(replay_write_buffer_seg, 0)
	);
	uint8_t change = 0;
	uint8_t tag;

	if(phase == T3_REPLAY_PACKET_PHASE_GAMEPLAY) {
		input_p1 &= ~INPUT_SHOT;
		input_p2 &= ~INPUT_SHOT;
		if(shot_bits & 0x01) {
			input_p1 |= INPUT_SHOT;
		}
		if(shot_bits & 0x02) {
			input_p2 |= INPUT_SHOT;
		}
	}

	if(replay_protect_blocked()) {
		return true;
	}
	if(
		(replay_write_buffer_size >
		 (T3_REPLAY_WRITE_BUFFER_SIZE - T3_REPLAY_PACKET_SIZE_MAX)) &&
		!replay_user_periodic_flush()
	) {
		return true;
	}

	if(
		(replay_rle_packet_state & REPLAY_RLE_STATE_OPEN) &&
		((replay_rle_phase & REPLAY_RLE_PHASE_MASK) == phase) &&
		(replay_rle_input_mp_p1 == input_p1) &&
		(replay_rle_input_mp_p2 == input_p2) &&
		(replay_rle_input_sp == input_single) &&
		(previous_charge == input_charge) &&
		(replay_rle_run < T3_REPLAY_PACKET_RUN_MAX)
	) {
		tag = replay_user_rle_tag(phase, replay_rle_run + 1);
		replay_rle_run++;
		write_buffer[replay_write_buffer_size - packet_size] = tag;
		replay_sample_count++;
		return true;
	}

	if(!(replay_rle_packet_state & REPLAY_RLE_STATE_OPEN)) {
		change = (
			T3_REPLAY_PACKET_CHANGE_P1 |
			T3_REPLAY_PACKET_CHANGE_P2 |
			T3_REPLAY_PACKET_CHANGE_SP |
			T3_REPLAY_PACKET_CHANGE_CHARGE
		);
	} else {
		if(input_p1 != replay_rle_input_mp_p1) {
			change |= T3_REPLAY_PACKET_CHANGE_P1;
		}
		if(input_p2 != replay_rle_input_mp_p2) {
			change |= T3_REPLAY_PACKET_CHANGE_P2;
		}
		if(input_single != replay_rle_input_sp) {
			change |= T3_REPLAY_PACKET_CHANGE_SP;
		}
		if(input_charge != previous_charge) {
			change |= T3_REPLAY_PACKET_CHANGE_CHARGE;
		}
	}

	tag = replay_user_rle_tag(phase, 1);
	packet_size = replay_write_buffer_size;
	if(!replay_user_buffer_u8(tag) || !replay_user_buffer_u8(change)) {
		replay_protect_detector_error_set();
		return true;
	}
	if(change & T3_REPLAY_PACKET_CHANGE_P1) {
		if(!replay_user_buffer_u16(input_p1)) {
			replay_protect_detector_error_set();
			return true;
		}
	}
	if(change & T3_REPLAY_PACKET_CHANGE_P2) {
		if(!replay_user_buffer_u16(input_p2)) {
			replay_protect_detector_error_set();
			return true;
		}
	}
	if(change & T3_REPLAY_PACKET_CHANGE_SP) {
		if(!replay_user_buffer_u16(input_single)) {
			replay_protect_detector_error_set();
			return true;
		}
	}
	if(change & T3_REPLAY_PACKET_CHANGE_CHARGE) {
		if(!replay_user_buffer_u8(input_charge)) {
			replay_protect_detector_error_set();
			return true;
		}
	}
	packet_size = static_cast<uint8_t>(replay_write_buffer_size - packet_size);
	replay_rle_phase = static_cast<uint8_t>(
		(replay_rle_phase & REPLAY_RLE_STAGE_CHECKPOINT_PENDING_MASK) |
		phase |
		(packet_size << REPLAY_RLE_PACKET_SIZE_SHIFT)
	);
	replay_rle_packet_state = static_cast<uint8_t>(
		REPLAY_RLE_STATE_OPEN |
		(input_charge << REPLAY_RLE_STATE_CHARGE_SHIFT)
	);
	replay_rle_run = 1;
	replay_rle_input_mp_p1 = input_p1;
	replay_rle_input_mp_p2 = input_p2;
	replay_rle_input_sp = input_single;
	replay_sample_count++;
	return true;
}

static bool replay_user_record_logical_sample(uint8_t phase, uint8_t shot_bits)
{
	if(replay_user_header_is_rle()) {
		return replay_user_record_rle_sample(phase, shot_bits);
	}
	if(phase == T3_REPLAY_PACKET_PHASE_INTERSTITIAL) {
		return replay_user_record_interstitial_sample();
	}
	return replay_user_record_sample();
}

static bool replay_user_play_sample(void)
{
	replay_user_sample_t sample;
	uint32_t offset;

	if(replay_sample_count >= replay_user_header.sample_count) {
		return false;
	}

	offset = (
		replay_user_header.input_offset +
		(replay_sample_count * static_cast<uint32_t>(sizeof(sample)))
	);
	if(!file_ropen(replay_user_fn)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	if(file_read(&sample, sizeof(sample)) != sizeof(sample)) {
		file_close();
		return false;
	}
	file_close();

	if(
		(sample.frame_index != replay_global_frame) ||
		(sample.round_frame == T3_REPLAY_INTERSTITIAL_ROUND_FRAME) ||
		(
			sample.round_or_result_frame ==
			T3_REPLAY_INTERSTITIAL_ROUND_OR_RESULT_FRAME
		) ||
		(sample.round_frame != round_frame) ||
		(sample.round_or_result_frame != round_or_result_frame)
	) {
		return false;
	}

	input_mp_p1 = sample.input_mp_p1;
	input_mp_p2 = sample.input_mp_p2;
	input_sp = sample.input_sp;
	replay_sample_count++;
	if(
		(replay_sample_count >= replay_user_header.sample_count) &&
		(replay_user_header.status == RUS_FINALIZED) &&
		(replay_user_header.end_reason == RUER_MENU_RETURN)
	) {
		// Backward compatibility with recordings made before prompt samples.
		input_sp = 0;
		byte_23B00 = 1;
		replay_prompt_skip_queued = true;
	}
	return true;
}

static bool replay_user_read_rle_packet(void)
{
	uint8_t tag;
	uint8_t change;
	uint8_t phase;
	uint8_t charge_input = static_cast<uint8_t>(
		(replay_rle_packet_state & REPLAY_RLE_STATE_CHARGE_MASK) >>
		REPLAY_RLE_STATE_CHARGE_SHIFT
	);
	uint32_t offset = (replay_user_header.input_offset + replay_input_byte_count);

	if((replay_input_byte_count + 2) > replay_user_header.input_size) {
		return false;
	}
	if(!file_ropen(replay_user_fn)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	if(
		(file_read(&tag, sizeof(tag)) != sizeof(tag)) ||
		(file_read(&change, sizeof(change)) != sizeof(change))
	) {
		file_close();
		return false;
	}
	replay_input_byte_count += 2;
	phase = static_cast<uint8_t>(tag >> T3_REPLAY_PACKET_PHASE_SHIFT);
	replay_rle_phase = phase;
	replay_rle_run = static_cast<uint8_t>(
		(tag & T3_REPLAY_PACKET_RUN_MASK) + 1
	);
	if(phase > T3_REPLAY_PACKET_PHASE_INTERSTITIAL) {
		file_close();
		return false;
	}
	if(change & T3_REPLAY_PACKET_CHANGE_P1) {
		if(
			(replay_input_byte_count + sizeof(replay_rle_input_mp_p1)) >
			replay_user_header.input_size
		) {
			file_close();
			return false;
		}
		if(
			file_read(
				&replay_rle_input_mp_p1, sizeof(replay_rle_input_mp_p1)
			) != sizeof(replay_rle_input_mp_p1)
		) {
			file_close();
			return false;
		}
		replay_input_byte_count += sizeof(replay_rle_input_mp_p1);
	}
	if(change & T3_REPLAY_PACKET_CHANGE_P2) {
		if(
			(replay_input_byte_count + sizeof(replay_rle_input_mp_p2)) >
			replay_user_header.input_size
		) {
			file_close();
			return false;
		}
		if(
			file_read(
				&replay_rle_input_mp_p2, sizeof(replay_rle_input_mp_p2)
			) != sizeof(replay_rle_input_mp_p2)
		) {
			file_close();
			return false;
		}
		replay_input_byte_count += sizeof(replay_rle_input_mp_p2);
	}
	if(change & T3_REPLAY_PACKET_CHANGE_SP) {
		if(
			(replay_input_byte_count + sizeof(replay_rle_input_sp)) >
			replay_user_header.input_size
		) {
			file_close();
			return false;
		}
		if(
			file_read(&replay_rle_input_sp, sizeof(replay_rle_input_sp)) !=
			sizeof(replay_rle_input_sp)
		) {
			file_close();
			return false;
		}
		replay_input_byte_count += sizeof(replay_rle_input_sp);
	}
	if(change & T3_REPLAY_PACKET_CHANGE_CHARGE) {
		if(
			(replay_input_byte_count + sizeof(charge_input)) >
			replay_user_header.input_size
		) {
			file_close();
			return false;
		}
		if(
			file_read(
				&charge_input, sizeof(charge_input)
			) != sizeof(charge_input)
		) {
			file_close();
			return false;
		}
		replay_input_byte_count += sizeof(charge_input);
	}
	if(charge_input > 0x03) {
		file_close();
		return false;
	}
	replay_rle_phase = phase;
	replay_rle_packet_state = static_cast<uint8_t>(
		(replay_rle_packet_state & REPLAY_RLE_STATE_OPEN) |
		(charge_input << REPLAY_RLE_STATE_CHARGE_SHIFT)
	);
	if(change & ~(
		T3_REPLAY_PACKET_CHANGE_P1 |
		T3_REPLAY_PACKET_CHANGE_P2 |
		T3_REPLAY_PACKET_CHANGE_SP |
		T3_REPLAY_PACKET_CHANGE_CHARGE
	)) {
		file_close();
		return false;
	}
	file_close();
	return true;
}

static bool replay_user_play_rle_sample(uint8_t phase)
{
	if(replay_sample_count >= replay_user_header.sample_count) {
		return false;
	}
	if(replay_rle_run == 0) {
		if(!replay_user_read_rle_packet()) {
			return false;
		}
	}
	if((replay_rle_phase & REPLAY_RLE_PHASE_MASK) != phase) {
		return false;
	}
	input_mp_p1 = replay_rle_input_mp_p1;
	input_mp_p2 = replay_rle_input_mp_p2;
	input_sp = replay_rle_input_sp;
	resident->input_charge = static_cast<uint8_t>(
		(replay_rle_packet_state & REPLAY_RLE_STATE_CHARGE_MASK) >>
		REPLAY_RLE_STATE_CHARGE_SHIFT
	);
	replay_rle_run--;
	replay_sample_count++;
	return true;
}

static bool replay_user_play_logical_sample(uint8_t phase)
{
	bool ok;

	if(replay_user_header_is_rle()) {
		ok = replay_user_play_rle_sample(phase);
	} else if(phase == T3_REPLAY_PACKET_PHASE_INTERSTITIAL) {
		ok = replay_user_play_interstitial_sample();
	} else {
		ok = replay_user_play_sample();
	}
	if(ok && (phase == T3_REPLAY_PACKET_PHASE_GAMEPLAY)) {
		replay_autofire_apply();
	}
	return ok;
}

static bool replay_user_playback_cancel(void)
{
	if(input_sp & INPUT_CANCEL) {
		input_sp = INPUT_NONE;
		byte_23B00 = 1;
		replay_prompt_skip_queued = true;
		return true;
	}
	return false;
}

static bool replay_user_play_interstitial_sample(void)
{
	replay_user_sample_t sample;
	uint32_t offset;

	if(replay_sample_count >= replay_user_header.sample_count) {
		return false;
	}

	offset = (
		replay_user_header.input_offset +
		(replay_sample_count * static_cast<uint32_t>(sizeof(sample)))
	);
	if(!file_ropen(replay_user_fn)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	if(file_read(&sample, sizeof(sample)) != sizeof(sample)) {
		file_close();
		return false;
	}
	file_close();

	if(
		(sample.frame_index != replay_global_frame) ||
		(sample.round_frame != T3_REPLAY_INTERSTITIAL_ROUND_FRAME) ||
		(
			sample.round_or_result_frame !=
			T3_REPLAY_INTERSTITIAL_ROUND_OR_RESULT_FRAME
		)
	) {
		return false;
	}

	input_mp_p1 = sample.input_mp_p1;
	input_mp_p2 = sample.input_mp_p2;
	input_sp = sample.input_sp;
	replay_sample_count++;
	return true;
}

static bool replay_play_sample(void)
{
	replay_input_sample_t sample;
	uint32_t offset;

	if(replay_sample_count >= replay_header.sample_count) {
		return false;
	}

	offset = (
		static_cast<uint32_t>(sizeof(replay_header)) +
		(replay_sample_count * static_cast<uint32_t>(sizeof(sample)))
	);
	if(!file_ropen(T3_INPUT_FN)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	if(file_read(&sample, sizeof(sample)) != sizeof(sample)) {
		file_close();
		return false;
	}
	file_close();

	if(
		(sample.frame_index != replay_global_frame) ||
		(sample.round_frame != round_frame) ||
		(sample.round_or_result_frame != round_or_result_frame)
	) {
		return false;
	}

	input_mp_p1 = sample.input_mp_p1;
	input_mp_p2 = sample.input_mp_p2;
	input_sp = sample.input_sp;
	replay_sample_count++;
	return true;
}

static bool replay_header_read(void)
{
	if(!file_ropen(T3_INPUT_FN)) {
		return false;
	}
	if(file_read(&replay_header, sizeof(replay_header)) != sizeof(replay_header)) {
		file_close();
		return false;
	}
	file_close();

	return (
		(replay_header.magic[0] == 'T') &&
		(replay_header.magic[1] == '3') &&
		(replay_header.magic[2] == 'R') &&
		(replay_header.magic[3] == 'I') &&
		(replay_header.magic[4] == 'N') &&
		(replay_header.magic[5] == 'P') &&
		(replay_header.magic[6] == '1') &&
		(replay_header.version == 1) &&
		(replay_header.header_size == sizeof(replay_header)) &&
		(replay_header.sample_size == sizeof(replay_input_sample_t))
	);
}

static replay_mode_t replay_cfg_mode(void)
{
	char cfg[64];
	unsigned read_len;
	unsigned i;
	char mode = '\0';

	replay_memclear(cfg, sizeof(cfg));
	if(!file_ropen(T3_REPLAY_CFG_FN)) {
		return REPLAY_DISABLED;
	}
	read_len = file_read(cfg, (sizeof(cfg) - 1));
	file_close();

	for(i = 0; i < read_len; i++) {
		if(
			(cfg[i] != ' ') &&
			(cfg[i] != '\t') &&
			(cfg[i] != '\r') &&
			(cfg[i] != '\n')
		) {
			mode = cfg[i];
			break;
		}
	}

	if(replay_char_ieq(mode, 'r')) {
		return REPLAY_RECORD;
	}
	if(replay_char_ieq(mode, 'p')) {
		return REPLAY_PLAYBACK;
	}
	if(replay_char_ieq(mode, 'v')) {
		return REPLAY_USER_PLAYBACK;
	}
	return REPLAY_DISABLED;
}

static replay_mode_t replay_resident_mode(void)
{
	if(
		(resident->unused_3[0] != T3_REPLAY_RES_MAGIC_0) ||
		(resident->unused_3[1] != T3_REPLAY_RES_MAGIC_1) ||
		(resident->unused_3[2] != T3_REPLAY_RES_MAGIC_2) ||
		(resident->unused_3[3] != T3_REPLAY_RES_MAGIC_3)
	) {
		return REPLAY_DISABLED;
	}
	if(resident->unused_3[T3_REPLAY_RES_MODE_INDEX] == T3_REPLAY_RES_MODE_RECORD) {
		return REPLAY_RECORD;
	}
	if(resident->unused_3[T3_REPLAY_RES_MODE_INDEX] == T3_REPLAY_RES_MODE_PLAYBACK) {
		return REPLAY_PLAYBACK;
	}
	if(resident->unused_3[T3_REPLAY_RES_MODE_INDEX] == T3_REPLAY_RES_MODE_USER_RECORD) {
		return REPLAY_USER_RECORD;
	}
	if(resident->unused_3[T3_REPLAY_RES_MODE_INDEX] == T3_REPLAY_RES_MODE_USER_PLAYBACK) {
		return REPLAY_USER_PLAYBACK;
	}
	return REPLAY_DISABLED;
}

static uint8_t replay_handoff_u8(unsigned index)
{
	return static_cast<uint8_t>(resident->unused_3[index]);
}

static uint32_t replay_handoff_u32_read(unsigned index)
{
	return (
		static_cast<uint32_t>(replay_handoff_u8(index)) |
		(static_cast<uint32_t>(replay_handoff_u8(index + 1)) << 8) |
		(static_cast<uint32_t>(replay_handoff_u8(index + 2)) << 16) |
		(static_cast<uint32_t>(replay_handoff_u8(index + 3)) << 24)
	);
}

static void replay_handoff_u32_write(unsigned index, uint32_t value)
{
	resident->unused_3[index + 0] = static_cast<uint8_t>(value);
	resident->unused_3[index + 1] = static_cast<uint8_t>(value >> 8);
	resident->unused_3[index + 2] = static_cast<uint8_t>(value >> 16);
	resident->unused_3[index + 3] = static_cast<uint8_t>(value >> 24);
}

static void replay_handoff_cursor_store(void)
{
	if(
		(replay_mode != REPLAY_USER_RECORD) &&
		(replay_mode != REPLAY_USER_PLAYBACK)
	) {
		return;
	}
	replay_handoff_u32_write(
		T3_REPLAY_RES_SAMPLE_COUNT_INDEX, replay_sample_count
	);
	replay_handoff_u32_write(
		T3_REPLAY_RES_GLOBAL_FRAME_INDEX, replay_global_frame
	);
	replay_handoff_u32_write(
		T3_REPLAY_RES_INPUT_SIZE_INDEX, replay_input_byte_count
	);
}

static void replay_user_sample_commit(void)
{
	if(
		(replay_global_frame & (T3_REPLAY_DISK_INTERVAL_SAMPLES - 1)) ==
		(T3_REPLAY_DISK_INTERVAL_SAMPLES - 1)
	) {
		replay_split_row(RSE_CHECKPOINT, replay_last_route);
		if(replay_mode == REPLAY_USER_RECORD) {
			replay_user_periodic_flush();
			replay_handoff_cursor_store();
		}
	}
	replay_global_frame++;
	replay_handoff_cursor_store();
}

static void replay_resident_handoff_clear(void)
{
	int i;

	resident->unused_3[0] = 0;
	resident->unused_3[1] = 0;
	resident->unused_3[2] = 0;
	resident->unused_3[3] = 0;
	resident->unused_3[T3_REPLAY_RES_MODE_INDEX] = 0;
	resident->unused_3[T3_REPLAY_RES_SLOT_INDEX] = T3_REPLAY_USER_SLOT_NONE;
	for(
		i = T3_REPLAY_RES_SAMPLE_COUNT_INDEX;
		i < T3_REPLAY_RES_CURSOR_END_INDEX;
		i++
	) {
		resident->unused_3[i] = 0;
	}
}

static void replay_resident_handoff_mode_set(uint8_t mode)
{
	replay_resident_handoff_clear();
	resident->unused_3[0] = T3_REPLAY_RES_MAGIC_0;
	resident->unused_3[1] = T3_REPLAY_RES_MAGIC_1;
	resident->unused_3[2] = T3_REPLAY_RES_MAGIC_2;
	resident->unused_3[3] = T3_REPLAY_RES_MAGIC_3;
	resident->unused_3[T3_REPLAY_RES_MODE_INDEX] = mode;
}

static void replay_user_record_error_disable(
	uint8_t diag_code, replay_text_id_t done_text
)
{
	if(replay_handoff_u8(T3R_DIAG_CODE_INDEX) == RPD_NONE) {
		replay_protect_diag_code_set(diag_code);
	}
	replay_protect_detector_error_set();
	replay_guard_diag_write();
	replay_done_write(done_text);
	replay_protect_local_free();
	replay_resident_handoff_clear();
	replay_mode = REPLAY_DISABLED;
}

void far replay_session_start(void)
{
	uint8_t playback_stage;

	menu_font_load(aCOul);
	replay_paths_init();
	replay_protect_local_reset();

	replay_mode = replay_resident_mode();
	if(replay_mode == REPLAY_DISABLED) {
		replay_mode = replay_cfg_mode();
	}
	replay_sample_count = 0;
	replay_global_frame = 0;
	replay_input_byte_count = 0;
	replay_write_buffer_size = 0;
	replay_write_buffer_seg = 0;
	replay_last_route = 0;
	replay_rle_phase = T3_REPLAY_PACKET_PHASE_GAMEPLAY;
	replay_rle_run = 0;
	replay_rle_input_mp_p1 = 0;
	replay_rle_input_mp_p2 = 0;
	replay_rle_input_sp = 0;
	resident->unused_3[T3_RES_FAST_FORWARD_REPLAY_PHASE_INDEX] = 0;
	resident->unused_3[T3_REPLAY_RES_PAUSE_CANCEL_LATCH_INDEX] = false;
	resident->unused_3[T3_REPLAY_RES_GUARD_FRESH_INDEX] = 0;
	resident->input_charge = 0;
	replay_done_written = false;
	replay_user_slot_fn_set(T3_REPLAY_USER_SLOT_NONE);
	replay_prompt_skip_queued = false;
	replay_rle_packet_state = 0;
	replay_user_discard_requested = false;
	replay_guard_diag_written = false;
	replay_restart_requested_flag = false;

	if(replay_mode == REPLAY_DISABLED) {
		return;
	}
	if(
		(replay_mode == REPLAY_USER_RECORD) &&
		replay_protect_blocked()
	) {
		replay_guard_diag_write();
	}
	if(
		(replay_mode == REPLAY_USER_RECORD) ||
		(replay_mode == REPLAY_USER_PLAYBACK)
	) {
		replay_sample_count = replay_handoff_u32_read(
			T3_REPLAY_RES_SAMPLE_COUNT_INDEX
		);
		replay_global_frame = replay_handoff_u32_read(
			T3_REPLAY_RES_GLOBAL_FRAME_INDEX
		);
		replay_input_byte_count = replay_handoff_u32_read(
			T3_REPLAY_RES_INPUT_SIZE_INDEX
		);
	}
	if(replay_mode == REPLAY_USER_RECORD) {
		replay_write_buffer_seg = reinterpret_cast<uint16_t>(
			hmem_allocbyte(REPLAY_RECORD_BUFFER_SIZE)
		);
		if(replay_write_buffer_seg == 0) {
			replay_protect_detector_error_set();
		}
	}

	if(replay_mode != REPLAY_USER_RECORD) {
		file_create(T3_DONE_FN);
		file_close();
		replay_split_write_header();
	}

	if(replay_mode == REPLAY_RECORD) {
		replay_header_fill();
		if(file_create(T3_INPUT_FN)) {
			replay_write_bytes(&replay_header, sizeof(replay_header));
			file_close();
		} else {
			replay_mode = REPLAY_ERROR;
			replay_done_write(RTX_ERROR_INPUT_CREATE);
			return;
		}
	} else if(replay_mode == REPLAY_USER_RECORD) {
		if(replay_sample_count == 0) {
			if(!replay_user_create()) {
				replay_mode = REPLAY_ERROR;
				replay_done_write(RTX_ERROR_USER_CREATE);
				return;
			}
		} else if(!replay_user_read()) {
			replay_user_record_error_disable(
				RPD_MAIN_USER_READ, RTX_ERROR_USER_CREATE
			);
			return;
		} else {
			if(!replay_user_guard_checkpoint()) {
				replay_guard_diag_write();
			} else {
				resident->unused_3[
					T3_REPLAY_RES_GUARD_FRESH_INDEX
				] = 2;
			}
			replay_user_snapshot_fill();
			replay_user_checkpoint_cursor_capture();
			replay_rle_phase |= REPLAY_RLE_STAGE_CHECKPOINT_PENDING_MASK;
		}
	} else if(replay_mode == REPLAY_USER_PLAYBACK) {
		if(!replay_user_read()) {
			replay_mode = REPLAY_ERROR;
			replay_done_write(RTX_ERROR_USER_HEADER);
			return;
		}
		if(
			(replay_user_header.flags & T3_REPLAY_USER_FLAG_PRACTICE) &&
			(players[0].cpu_safety_frames !=
			 replay_user_header.scenario.practice.config.initial_cpu_safety_frames)
		) {
			replay_mode = REPLAY_ERROR;
			replay_done_write(RTX_ERROR_USER_HEADER);
			return;
		}
		playback_stage = replay_handoff_u8(
			T3_REPLAY_RES_PLAYBACK_STAGE_INDEX
		);
		if(playback_stage != 0) {
			playback_stage--;
			if(!replay_user_checkpoint_snapshot_read(playback_stage)) {
				replay_mode = REPLAY_ERROR;
				replay_done_write(RTX_ERROR_USER_HEADER);
				return;
			}
			replay_user_snapshot_restore_resident();
			replay_user_snapshot_restore_runtime();
			resident->unused_3[T3_REPLAY_RES_PLAYBACK_STAGE_INDEX] = 0;
		} else if(replay_sample_count == 0) {
			replay_user_snapshot_restore_resident();
			replay_user_snapshot_restore_runtime();
		}
	} else if(!replay_header_read()) {
		replay_mode = REPLAY_ERROR;
		replay_done_write(RTX_ERROR_INPUT_HEADER);
		return;
	}

	replay_split_row(RSE_START, 0);
}

void far replay_round_start(void)
{
	replay_split_row(RSE_ROUND_START, replay_last_route);
}

void far replay_frame_io(void)
{
	bool ok = true;
	bool fast_forward_held = false;
	uint8_t shot_bits;

	keyconfig_charge_mask_human();

	if(replay_mode == REPLAY_DISABLED) {
		replay_autofire_apply();
		return;
	}
	if(replay_mode == REPLAY_ERROR) {
		input_mp_p1 = INPUT_NONE;
		input_mp_p2 = INPUT_NONE;
		input_sp = INPUT_CANCEL;
		return;
	}

	if(replay_mode == REPLAY_RECORD) {
		replay_autofire_apply();
		ok = replay_record_sample();
	} else if(replay_mode == REPLAY_USER_RECORD) {
		shot_bits = (
			((input_mp_p1 & INPUT_SHOT) ? 0x01 : 0x00) |
			((input_mp_p2 & INPUT_SHOT) ? 0x02 : 0x00)
		);
		replay_autofire_apply();
		ok = replay_user_record_logical_sample(
			T3_REPLAY_PACKET_PHASE_GAMEPLAY, shot_bits
		);
	} else if(replay_mode == REPLAY_USER_PLAYBACK) {
		fast_forward_held = ((input_sp & INPUT_SHOT) != 0);
		if(replay_user_playback_cancel()) {
			replay_fast_forward_wait_skip(false);
			return;
		}
		if(replay_sample_count >= replay_user_header.sample_count) {
			replay_split_row(RSE_INPUT_END, replay_last_route);
			input_sp |= INPUT_CANCEL;
			replay_done_write(RTX_OK_USER_INPUT_END);
			replay_resident_handoff_clear();
			resident->game_mode = GM_NONE;
			replay_mode = REPLAY_DISABLED;
			replay_fast_forward_wait_skip(false);
			return;
		}
		ok = replay_user_play_logical_sample(T3_REPLAY_PACKET_PHASE_GAMEPLAY);
	} else if(replay_mode == REPLAY_PLAYBACK) {
		fast_forward_held = ((input_sp & INPUT_SHOT) != 0);
		if(replay_sample_count >= replay_header.sample_count) {
			replay_split_row(RSE_INPUT_END, replay_last_route);
			input_sp |= INPUT_CANCEL;
			replay_done_write(RTX_OK_INPUT_END);
			replay_mode = REPLAY_DISABLED;
			replay_fast_forward_wait_skip(false);
			return;
		}
		ok = replay_play_sample();
	}

	if(!ok) {
		if(replay_mode == REPLAY_USER_RECORD) {
			replay_user_record_error_disable(
				RPD_MAIN_RECORD_IO, RTX_ERROR_FRAME_IO
			);
			return;
		}
		replay_fast_forward_wait_skip(false);
		replay_split_row(RSE_ERROR, replay_last_route);
		if(replay_mode == REPLAY_USER_PLAYBACK) {
			resident->game_mode = GM_NONE;
		}
		replay_mode = REPLAY_ERROR;
		input_sp |= INPUT_CANCEL;
		replay_done_write(RTX_ERROR_FRAME_IO);
		return;
	}

	replay_fast_forward_wait_skip(fast_forward_held);

	if(
		(replay_global_frame & (T3_REPLAY_DISK_INTERVAL_SAMPLES - 1)) ==
		(T3_REPLAY_DISK_INTERVAL_SAMPLES - 1)
	) {
		replay_split_row(RSE_CHECKPOINT, replay_last_route);
		if(replay_mode == REPLAY_RECORD) {
			replay_header_write();
		} else if(replay_mode == REPLAY_USER_RECORD) {
			replay_user_periodic_flush();
			replay_handoff_cursor_store();
		}
	}
	replay_global_frame++;
	replay_handoff_cursor_store();
	if(
		(replay_mode == REPLAY_USER_RECORD) &&
		(resident->unused_3[T3_REPLAY_RES_GUARD_FRESH_INDEX] != 0)
	) {
		resident->unused_3[T3_REPLAY_RES_GUARD_FRESH_INDEX]--;
	}
}

void far replay_input_sense_held(void)
{
	bool ok = true;

	if(replay_mode == REPLAY_USER_PLAYBACK) {
		input_reset_sense_key_held();
		if(replay_user_playback_cancel()) {
			return;
		}
		ok = replay_user_play_logical_sample(
			T3_REPLAY_PACKET_PHASE_INTERSTITIAL
		);
	} else {
		input_reset_sense_key_held();
		keyconfig_charge_mask_human();
		if(replay_mode == REPLAY_USER_RECORD) {
			ok = replay_user_record_logical_sample(
				T3_REPLAY_PACKET_PHASE_INTERSTITIAL, 0
			);
		}
	}

	if(!ok) {
		if(replay_mode == REPLAY_USER_RECORD) {
			replay_user_record_error_disable(
				RPD_MAIN_RECORD_IO, RTX_ERROR_FRAME_IO
			);
			return;
		}
		replay_split_row(RSE_ERROR, replay_last_route);
		if(replay_mode == REPLAY_USER_PLAYBACK) {
			resident->game_mode = GM_NONE;
		}
		replay_mode = REPLAY_ERROR;
		input_sp |= INPUT_CANCEL;
		replay_done_write(RTX_ERROR_FRAME_IO);
		return;
	}
	if(
		(replay_mode == REPLAY_USER_RECORD) ||
		(replay_mode == REPLAY_USER_PLAYBACK)
	) {
		replay_user_sample_commit();
	}
}

#define REPLAY_PAUSE_LEFT 24
#define REPLAY_PAUSE_TOP 8
#define REPLAY_PAUSE_W 32
#define REPLAY_PAUSE_H 7
#define REPLAY_PAUSE_PIXEL_LEFT (REPLAY_PAUSE_LEFT * GLYPH_HALF_W)
#define REPLAY_PAUSE_PIXEL_TOP (REPLAY_PAUSE_TOP * GLYPH_HALF_H)
#define REPLAY_PAUSE_PIXEL_RIGHT ( \
	REPLAY_PAUSE_PIXEL_LEFT + (REPLAY_PAUSE_W * GLYPH_HALF_W) - 1 \
)
#define REPLAY_PAUSE_PIXEL_BOTTOM ( \
	REPLAY_PAUSE_PIXEL_TOP + (REPLAY_PAUSE_H * GLYPH_HALF_H) - 1 \
)
#define REPLAY_PAUSE_TEXT_LEFT (REPLAY_PAUSE_LEFT + 6)
#define REPLAY_PAUSE_CHOICE_MARK_LEFT (REPLAY_PAUSE_LEFT + 3)
#define REPLAY_PAUSE_BG_ATRB TX_BLACK
#define REPLAY_PAUSE_FRAME_ATRB TX_WHITE
#define REPLAY_PAUSE_TITLE_ATRB TX_CYAN
#define REPLAY_PAUSE_CHOICE_ATRB TX_WHITE
#define REPLAY_PAUSE_SELECTED_ATRB TX_YELLOW
#define REPLAY_PAUSE_DISABLED_ATRB TX_BLUE
#define REPLAY_PAUSE_CHOICE_COLOR_W ( \
	(REPLAY_PAUSE_TEXT_LEFT + 20) - REPLAY_PAUSE_CHOICE_MARK_LEFT \
)
#define REPLAY_PAUSE_FONT_PIXEL_BOTTOM ( \
	REPLAY_PAUSE_PIXEL_TOP + (REPLAY_PAUSE_H * GLYPH_H) - 1 \
)
#define REPLAY_PAUSE_FONT_CHOICE_PIXEL_LEFT ( \
	REPLAY_PAUSE_CHOICE_MARK_LEFT * GLYPH_HALF_W \
)
#define REPLAY_PAUSE_FONT_TEXT_PIXEL_LEFT ( \
	REPLAY_PAUSE_TEXT_LEFT * GLYPH_HALF_W \
)

enum replay_pause_vram_color_t {
	REPLAY_PAUSE_VRAM_BLUE = 9,
	REPLAY_PAUSE_VRAM_CYAN = 13,
};

enum replay_pause_font_color_t {
	REPLAY_PAUSE_FONT_BLACK = 0,
	REPLAY_PAUSE_FONT_BLUE = 9,
	REPLAY_PAUSE_FONT_YELLOW = 12,
	REPLAY_PAUSE_FONT_CYAN = 13,
	REPLAY_PAUSE_FONT_WHITE = 15,
};

static void replay_text_putca(unsigned x, unsigned y, int ch, unsigned atrb)
{
	char str[2];

	str[0] = static_cast<char>(ch);
	str[1] = '\0';
	(void)atrb;
	text_putsa(x, y, str, (TX_BLACK | TX_REVERSE));
}

#pragma codestring "\x90"

static void replay_text_putca_raw(
	unsigned x, unsigned y, int ch, unsigned atrb
)
{
	char str[2];

	str[0] = static_cast<char>(ch);
	str[1] = '\0';
	text_putsa(x, y, str, atrb);
}

static unsigned replay_pause_clear_atrb(unsigned x)
{
	if(
		(
			(x >= playfield_tram_x(0, 0)) &&
			(x < playfield_tram_x(0, PLAYFIELD_W))
		) ||
		(
			(x >= playfield_tram_x(1, 0)) &&
			(x < playfield_tram_x(1, PLAYFIELD_W))
		)
	) {
		return TX_WHITE;
	}
	return (TX_BLACK | TX_REVERSE);
}

static void replay_pause_put_graph_backing(void)
{
	graph_copy_page(page_back);
	replay_overlay_graph_fill(
		REPLAY_PAUSE_PIXEL_LEFT, REPLAY_PAUSE_PIXEL_TOP,
		REPLAY_PAUSE_PIXEL_RIGHT, REPLAY_PAUSE_PIXEL_BOTTOM,
		V_WHITE, page_front
	);
}

static int replay_pause_vram_color(unsigned atrb)
{
	if(atrb == REPLAY_PAUSE_TITLE_ATRB) {
		return REPLAY_PAUSE_VRAM_CYAN;
	}
	if(atrb == REPLAY_PAUSE_SELECTED_ATRB) {
		return V_YELLOW_BRIGHT;
	}
	if(atrb == REPLAY_PAUSE_DISABLED_ATRB) {
		return REPLAY_PAUSE_VRAM_BLUE;
	}
	return V_WHITE;
}

static void replay_pause_put_color_backing(
	unsigned left, unsigned top, unsigned width, unsigned atrb
)
{
	replay_overlay_graph_fill(
		(left * GLYPH_HALF_W), (top * GLYPH_HALF_H),
		((left + width) * GLYPH_HALF_W) - 1,
		((top + 1) * GLYPH_HALF_H) - 1,
		replay_pause_vram_color(atrb), page_front
	);
}

// Keep the following pause-menu helpers at their accepted offsets.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"

static void replay_pause_put_frame(void)
{
	int x;
	int y;

	for(y = 0; y < REPLAY_PAUSE_H; y++) {
		for(x = 0; x < REPLAY_PAUSE_W; x++) {
			replay_text_putca(
				(REPLAY_PAUSE_LEFT + x), (REPLAY_PAUSE_TOP + y),
				' ', REPLAY_PAUSE_BG_ATRB
			);
		}
	}

	replay_text_putca(
		REPLAY_PAUSE_LEFT, REPLAY_PAUSE_TOP, '+', REPLAY_PAUSE_FRAME_ATRB
	);
	replay_text_putca(
		(REPLAY_PAUSE_LEFT + (REPLAY_PAUSE_W - 1)), REPLAY_PAUSE_TOP,
		'+', REPLAY_PAUSE_FRAME_ATRB
	);
	replay_text_putca(
		REPLAY_PAUSE_LEFT, (REPLAY_PAUSE_TOP + (REPLAY_PAUSE_H - 1)),
		'+', REPLAY_PAUSE_FRAME_ATRB
	);
	replay_text_putca(
		(REPLAY_PAUSE_LEFT + (REPLAY_PAUSE_W - 1)),
		(REPLAY_PAUSE_TOP + (REPLAY_PAUSE_H - 1)),
		'+', REPLAY_PAUSE_FRAME_ATRB
	);
	for(x = 1; x < (REPLAY_PAUSE_W - 1); x++) {
		replay_text_putca(
			(REPLAY_PAUSE_LEFT + x), REPLAY_PAUSE_TOP,
			'-', REPLAY_PAUSE_FRAME_ATRB
		);
		replay_text_putca(
			(REPLAY_PAUSE_LEFT + x),
			(REPLAY_PAUSE_TOP + (REPLAY_PAUSE_H - 1)),
			'-', REPLAY_PAUSE_FRAME_ATRB
		);
	}
	for(y = 1; y < (REPLAY_PAUSE_H - 1); y++) {
		replay_text_putca(
			REPLAY_PAUSE_LEFT, (REPLAY_PAUSE_TOP + y),
			'|', REPLAY_PAUSE_FRAME_ATRB
		);
		replay_text_putca(
			(REPLAY_PAUSE_LEFT + (REPLAY_PAUSE_W - 1)),
			(REPLAY_PAUSE_TOP + y), '|', REPLAY_PAUSE_FRAME_ATRB
		);
	}
}

static void replay_pause_put_title(void)
{
	unsigned x = (REPLAY_PAUSE_LEFT + 13);
	unsigned y = (REPLAY_PAUSE_TOP + 1);

	replay_pause_put_color_backing(x, y, 6, REPLAY_PAUSE_TITLE_ATRB);
#define P(c) replay_text_putca(x++, y, c, REPLAY_PAUSE_TITLE_ATRB)
	P('P'); P('A'); P('U'); P('S'); P('E'); P('D');
#undef P
}

static void replay_pause_put_resume(unsigned y, unsigned atrb)
{
	unsigned x = REPLAY_PAUSE_TEXT_LEFT;

#define P(c) replay_text_putca(x++, y, c, atrb)
	P('R'); P('e'); P('s'); P('u'); P('m'); P('e');
#undef P
}

static void replay_pause_put_restart(unsigned y, unsigned atrb)
{
	unsigned x = REPLAY_PAUSE_TEXT_LEFT;

#define P(c) replay_text_putca(x++, y, c, atrb)
	P('R'); P('e'); P('s'); P('t'); P('a'); P('r'); P('t');
#undef P
}

static void replay_pause_put_save_exit(unsigned y, unsigned atrb)
{
	unsigned x = REPLAY_PAUSE_TEXT_LEFT;

#define P(c) replay_text_putca(x++, y, c, atrb)
	P('S'); P('a'); P('v'); P('e'); P(' '); P('R'); P('e'); P('p');
	P('l'); P('a'); P('y'); P(' '); P('a'); P('n'); P('d'); P(' ');
	P('E'); P('x'); P('i'); P('t');
#undef P
}

static void replay_pause_put_discard_exit(unsigned y, unsigned atrb)
{
	unsigned x = REPLAY_PAUSE_TEXT_LEFT;

#define P(c) replay_text_putca(x++, y, c, atrb)
	P('E'); P('x'); P('i'); P('t'); P(' '); P('W'); P('i'); P('t');
	P('h'); P('o'); P('u'); P('t'); P(' '); P('S'); P('a'); P('v');
	P('i'); P('n'); P('g');
#undef P
}

static bool replay_pause_save_disabled(void)
{
	if(replay_mode == REPLAY_USER_PLAYBACK) {
		return false;
	}
	if(replay_mode != REPLAY_USER_RECORD) {
		return true;
	}
	return replay_protect_blocked();
}

static void replay_pause_save_refresh(void)
{
	if(replay_mode == REPLAY_USER_PLAYBACK) {
		return;
	}
	if(replay_mode != REPLAY_USER_RECORD) {
		return;
	}
	if(!replay_protect_blocked()) {
		if(
			resident->unused_3[T3_REPLAY_RES_GUARD_FRESH_INDEX] != 0
		) {
			resident->unused_3[T3_REPLAY_RES_GUARD_FRESH_INDEX] = 0;
			return;
		}
		if(!replay_user_periodic_flush()) {
			if(replay_handoff_u8(T3R_DIAG_CODE_INDEX) == RPD_NONE) {
				replay_protect_diag_code_set(RPD_MAIN_PAUSE_FLUSH);
			}
		}
	}
	if(replay_protect_blocked()) {
		replay_guard_diag_write();
	}
}

static uint8_t replay_pause_next_choice(uint8_t sel)
{
	sel++;
	if(sel > REPLAY_PAUSE_DISCARD_EXIT) {
		sel = REPLAY_PAUSE_RESUME;
	}
	if((sel == REPLAY_PAUSE_SAVE_EXIT) && replay_pause_save_disabled()) {
		sel = REPLAY_PAUSE_DISCARD_EXIT;
	}
	return sel;
}

static uint8_t replay_pause_prev_choice(uint8_t sel)
{
	if(sel == REPLAY_PAUSE_RESUME) {
		sel = REPLAY_PAUSE_DISCARD_EXIT;
	} else {
		sel--;
	}
	if((sel == REPLAY_PAUSE_SAVE_EXIT) && replay_pause_save_disabled()) {
		sel = REPLAY_PAUSE_RESTART;
	}
	return sel;
}

static uint8_t replay_pause_validate_choice(uint8_t sel)
{
	if((sel == REPLAY_PAUSE_SAVE_EXIT) && replay_pause_save_disabled()) {
		return REPLAY_PAUSE_DISCARD_EXIT;
	}
	return sel;
}

static void replay_pause_put_choices(uint8_t sel)
{
	unsigned y;
	unsigned atrb;

	y = (REPLAY_PAUSE_TOP + 2);
	atrb = (
		(sel == REPLAY_PAUSE_RESUME) ?
		REPLAY_PAUSE_SELECTED_ATRB :
		REPLAY_PAUSE_CHOICE_ATRB
	);
	replay_pause_put_color_backing(
		REPLAY_PAUSE_CHOICE_MARK_LEFT, y,
		REPLAY_PAUSE_CHOICE_COLOR_W, atrb
	);
	replay_text_putca(REPLAY_PAUSE_CHOICE_MARK_LEFT, y, (
		(sel == REPLAY_PAUSE_RESUME) ? '>' : ' '
	), atrb);
	replay_pause_put_resume(y, atrb);

	y++;
	atrb = (
		(sel == REPLAY_PAUSE_RESTART) ?
		REPLAY_PAUSE_SELECTED_ATRB :
		REPLAY_PAUSE_CHOICE_ATRB
	);
	replay_pause_put_color_backing(
		REPLAY_PAUSE_CHOICE_MARK_LEFT, y,
		REPLAY_PAUSE_CHOICE_COLOR_W, atrb
	);
	replay_text_putca(REPLAY_PAUSE_CHOICE_MARK_LEFT, y, (
		(sel == REPLAY_PAUSE_RESTART) ? '>' : ' '
	), atrb);
	replay_pause_put_restart(y, atrb);

	y++;
	if(replay_pause_save_disabled()) {
		atrb = REPLAY_PAUSE_DISABLED_ATRB;
	} else {
		atrb = (
			(sel == REPLAY_PAUSE_SAVE_EXIT) ?
			REPLAY_PAUSE_SELECTED_ATRB :
			REPLAY_PAUSE_CHOICE_ATRB
		);
	}
	replay_pause_put_color_backing(
		REPLAY_PAUSE_CHOICE_MARK_LEFT, y,
		REPLAY_PAUSE_CHOICE_COLOR_W, atrb
	);
	replay_text_putca(REPLAY_PAUSE_CHOICE_MARK_LEFT, y, (
		(sel == REPLAY_PAUSE_SAVE_EXIT) ? '>' : ' '
	), atrb);
	replay_pause_put_save_exit(y, atrb);

	y++;
	atrb = (
		(sel == REPLAY_PAUSE_DISCARD_EXIT) ?
		REPLAY_PAUSE_SELECTED_ATRB :
		REPLAY_PAUSE_CHOICE_ATRB
	);
	replay_pause_put_color_backing(
		REPLAY_PAUSE_CHOICE_MARK_LEFT, y,
		REPLAY_PAUSE_CHOICE_COLOR_W, atrb
	);
	replay_text_putca(REPLAY_PAUSE_CHOICE_MARK_LEFT, y, (
		(sel == REPLAY_PAUSE_DISCARD_EXIT) ? '>' : ' '
	), atrb);
	replay_pause_put_discard_exit(y, atrb);
}

static void replay_pause_clear(void)
{
	int x;
	int y;

	for(y = 0; y < REPLAY_PAUSE_H; y++) {
		for(x = 0; x < REPLAY_PAUSE_W; x++) {
			replay_text_putca_raw(
				(REPLAY_PAUSE_LEFT + x), (REPLAY_PAUSE_TOP + y),
				' ', replay_pause_clear_atrb(REPLAY_PAUSE_LEFT + x)
			);
		}
	}
}

static void replay_pause_restore_graphics(void)
{
	graph_copy_page(page_front);
	graph_accesspage(page_back);
}

static void replay_pause_wait_release(void)
{
	goto release_test;

release_wait:
	replay_input_sense_held();
	frame_delay(1);

release_test:
	if(
		(input_sp != INPUT_NONE) ||
		(input_mp_p1 != INPUT_NONE) ||
		(input_mp_p2 != INPUT_NONE)
	) {
		goto release_wait;
	}
}

static void replay_pause_beep(void)
{
	snd_se_reset();
	snd_se_play(21);
	snd_se_update();
}

static void replay_pause_font_put_graph_backing(void)
{
	graph_copy_page(page_back);
	replay_overlay_graph_fill(
		REPLAY_PAUSE_PIXEL_LEFT, REPLAY_PAUSE_PIXEL_TOP,
		REPLAY_PAUSE_PIXEL_RIGHT, REPLAY_PAUSE_FONT_PIXEL_BOTTOM,
		REPLAY_PAUSE_FONT_BLACK, page_front
	);
}

static void replay_pause_font_put_frame(void)
{
	replay_overlay_graph_fill(
		REPLAY_PAUSE_PIXEL_LEFT, REPLAY_PAUSE_PIXEL_TOP,
		REPLAY_PAUSE_PIXEL_RIGHT, REPLAY_PAUSE_PIXEL_TOP,
		REPLAY_PAUSE_FONT_WHITE, page_front
	);
	replay_overlay_graph_fill(
		REPLAY_PAUSE_PIXEL_LEFT, REPLAY_PAUSE_FONT_PIXEL_BOTTOM,
		REPLAY_PAUSE_PIXEL_RIGHT, REPLAY_PAUSE_FONT_PIXEL_BOTTOM,
		REPLAY_PAUSE_FONT_WHITE, page_front
	);
	replay_overlay_graph_fill(
		REPLAY_PAUSE_PIXEL_LEFT, REPLAY_PAUSE_PIXEL_TOP,
		REPLAY_PAUSE_PIXEL_LEFT, REPLAY_PAUSE_FONT_PIXEL_BOTTOM,
		REPLAY_PAUSE_FONT_WHITE, page_front
	);
	replay_overlay_graph_fill(
		REPLAY_PAUSE_PIXEL_RIGHT, REPLAY_PAUSE_PIXEL_TOP,
		REPLAY_PAUSE_PIXEL_RIGHT, REPLAY_PAUSE_FONT_PIXEL_BOTTOM,
		REPLAY_PAUSE_FONT_WHITE, page_front
	);
}

static void replay_pause_font_put_title(void)
{
	char str[7];

	str[0] = 'P'; str[1] = 'A'; str[2] = 'U'; str[3] = 'S';
	str[4] = 'E'; str[5] = 'D'; str[6] = '\0';
	menu_font_put_centered(
		((REPLAY_PAUSE_PIXEL_LEFT + REPLAY_PAUSE_PIXEL_RIGHT + 1) / 2),
		(REPLAY_PAUSE_PIXEL_TOP + GLYPH_H), str,
		REPLAY_PAUSE_FONT_CYAN
	);
}

static void replay_pause_font_choice_put(uint8_t choice, uint8_t sel)
{
	char cursor[2];
	char str[21];
	char *p = str;
	int color;
	int top = (
		REPLAY_PAUSE_PIXEL_TOP + ((choice + 2) * GLYPH_H)
	);

	if(
		(choice == REPLAY_PAUSE_SAVE_EXIT) &&
		replay_pause_save_disabled()
	) {
		color = REPLAY_PAUSE_FONT_BLUE;
	} else if(choice == sel) {
		color = REPLAY_PAUSE_FONT_YELLOW;
	} else {
		color = REPLAY_PAUSE_FONT_WHITE;
	}

	replay_overlay_graph_fill(
		(REPLAY_PAUSE_PIXEL_LEFT + 1), top,
		(REPLAY_PAUSE_PIXEL_RIGHT - 1), (top + GLYPH_H - 1),
		REPLAY_PAUSE_FONT_BLACK, page_front
	);
	if(choice == REPLAY_PAUSE_RESUME) {
		*p++ = 'R'; *p++ = 'e'; *p++ = 's';
		*p++ = 'u'; *p++ = 'm'; *p++ = 'e';
	} else if(choice == REPLAY_PAUSE_RESTART) {
		*p++ = 'R'; *p++ = 'e'; *p++ = 's'; *p++ = 't';
		*p++ = 'a'; *p++ = 'r'; *p++ = 't';
	} else if(choice == REPLAY_PAUSE_SAVE_EXIT) {
		*p++ = 'S'; *p++ = 'a'; *p++ = 'v'; *p++ = 'e'; *p++ = ' ';
		*p++ = 'R'; *p++ = 'e'; *p++ = 'p'; *p++ = 'l'; *p++ = 'a';
		*p++ = 'y'; *p++ = ' '; *p++ = 'a'; *p++ = 'n'; *p++ = 'd';
		*p++ = ' '; *p++ = 'E'; *p++ = 'x'; *p++ = 'i'; *p++ = 't';
	} else {
		*p++ = 'E'; *p++ = 'x'; *p++ = 'i'; *p++ = 't'; *p++ = ' ';
		*p++ = 'W'; *p++ = 'i'; *p++ = 't'; *p++ = 'h'; *p++ = 'o';
		*p++ = 'u'; *p++ = 't'; *p++ = ' '; *p++ = 'S'; *p++ = 'a';
		*p++ = 'v'; *p++ = 'i'; *p++ = 'n'; *p++ = 'g';
	}
	*p = '\0';
	cursor[0] = '>';
	cursor[1] = '\0';
	if(choice == sel) {
		menu_font_put(
			REPLAY_PAUSE_FONT_CHOICE_PIXEL_LEFT, top, cursor, color
		);
	}
	menu_font_put(
		REPLAY_PAUSE_FONT_TEXT_PIXEL_LEFT, top, str, color
	);
}

static void replay_pause_font_put_choices(uint8_t sel)
{
	uint8_t choice;

	for(
		choice = REPLAY_PAUSE_RESUME;
		choice <= REPLAY_PAUSE_DISCARD_EXIT;
		choice++
	) {
		replay_pause_font_choice_put(choice, sel);
	}
}

static void replay_pause_choices_redraw(uint8_t old_sel, uint8_t sel)
{
	(void)old_sel;
	replay_pause_put_choices(sel);
}

// Preserve the accepted offset of replay_pause_menu() after returning its
// redraw path to the smaller native TRAM implementation.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90"

uint8_t far replay_pause_menu(void)
{
	uint8_t sel = REPLAY_PAUSE_RESUME;
	uint8_t old_sel;

	replay_pause_save_refresh();
	replay_pause_beep();
	replay_pause_wait_release();
	// Gameplay uses doubled 200-line graphics, while TRAM remains 400-line and
	// owns the center HUD. Keep this menu on TRAM so it retains native height,
	// true black backing, and priority over that HUD.
	replay_pause_put_graph_backing();
	replay_pause_put_frame();
	replay_pause_put_title();
	sel = replay_pause_validate_choice(sel);
	replay_pause_put_choices(sel);

input_wait:
	replay_input_sense_held();
	if(input_sp & INPUT_Q) {
		return REPLAY_PAUSE_DISCARD_EXIT;
	}
	asm {
		call far ptr keyconfig_restart_request_poll
		jnc restart_not_requested
	}
	return REPLAY_PAUSE_RESTART;
restart_not_requested:
	if(input_sp & INPUT_CANCEL) {
		goto resume;
	}
	if(input_sp & INPUT_UP) {
		replay_pause_save_refresh();
		old_sel = sel;
		sel = replay_pause_validate_choice(sel);
		sel = replay_pause_prev_choice(sel);
		replay_pause_choices_redraw(old_sel, sel);
		replay_pause_beep();
		replay_pause_wait_release();
		goto input_wait;
	}
	if(input_sp & INPUT_DOWN) {
		replay_pause_save_refresh();
		old_sel = sel;
		sel = replay_pause_validate_choice(sel);
		sel = replay_pause_next_choice(sel);
		replay_pause_choices_redraw(old_sel, sel);
		replay_pause_beep();
		replay_pause_wait_release();
		goto input_wait;
	}
	if(input_sp & (INPUT_OK | INPUT_SHOT)) {
		if(sel == REPLAY_PAUSE_SAVE_EXIT) {
			replay_pause_save_refresh();
			if(replay_pause_save_disabled()) {
				old_sel = sel;
				sel = REPLAY_PAUSE_DISCARD_EXIT;
				replay_pause_choices_redraw(old_sel, sel);
				replay_pause_beep();
				replay_pause_wait_release();
				goto input_wait;
			}
		}
		if((sel == REPLAY_PAUSE_SAVE_EXIT) && replay_pause_save_disabled()) {
			old_sel = sel;
			sel = REPLAY_PAUSE_DISCARD_EXIT;
			replay_pause_choices_redraw(old_sel, sel);
			replay_pause_beep();
			replay_pause_wait_release();
			goto input_wait;
		}
		if(sel == REPLAY_PAUSE_RESUME) {
			goto resume;
		}
		return sel;
	}
	frame_delay(1);
	goto input_wait;

resume:
	replay_pause_wait_release();
	replay_pause_restore_graphics();
	replay_pause_clear();
	replay_pause_beep();
	return REPLAY_PAUSE_RESUME;
}

#undef REPLAY_PAUSE_LEFT
#undef REPLAY_PAUSE_TOP
#undef REPLAY_PAUSE_W
#undef REPLAY_PAUSE_H
#undef REPLAY_PAUSE_PIXEL_LEFT
#undef REPLAY_PAUSE_PIXEL_TOP
#undef REPLAY_PAUSE_PIXEL_RIGHT
#undef REPLAY_PAUSE_PIXEL_BOTTOM
#undef REPLAY_PAUSE_TEXT_LEFT
#undef REPLAY_PAUSE_CHOICE_MARK_LEFT
#undef REPLAY_PAUSE_BG_ATRB
#undef REPLAY_PAUSE_FRAME_ATRB
#undef REPLAY_PAUSE_TITLE_ATRB
#undef REPLAY_PAUSE_CHOICE_ATRB
#undef REPLAY_PAUSE_SELECTED_ATRB
#undef REPLAY_PAUSE_DISABLED_ATRB
#undef REPLAY_PAUSE_CHOICE_COLOR_W
#undef REPLAY_PAUSE_FONT_PIXEL_BOTTOM
#undef REPLAY_PAUSE_FONT_CHOICE_PIXEL_LEFT
#undef REPLAY_PAUSE_FONT_TEXT_PIXEL_LEFT

bool far replay_prompt_skip(void)
{
	return replay_prompt_skip_queued;
}

void far replay_user_record_discard_on_exit(void)
{
	if(replay_mode == REPLAY_USER_RECORD) {
		replay_user_discard_requested = true;
	}
}

void far replay_restart_request(void)
{
	replay_restart_requested_flag = true;
	replay_user_record_discard_on_exit();
}

void far replay_route(uint8_t route)
{
	replay_last_route = route;
	replay_user_round_split_capture(route);
	replay_user_summary_capture(route);
	replay_split_row(RSE_ROUTE, route);
}

void far replay_finish(uint8_t route)
{
	bool finish_error = false;
	bool save_pending = false;
	bool control_ok = true;

	replay_split_row(RSE_FINISH, route);
	if(replay_mode == REPLAY_USER_RECORD) {
		control_ok = replay_user_control_write(
			T3_REPLAY_PACKET_CONTROL_MAIN_END
		);
	} else if(
		(replay_mode == REPLAY_USER_PLAYBACK) &&
		!replay_prompt_skip_queued
	) {
		control_ok = replay_user_control_consume(
			T3_REPLAY_PACKET_CONTROL_MAIN_END
		);
	}
	if(!control_ok) {
		replay_split_row(RSE_ERROR, route);
		if(replay_mode == REPLAY_USER_PLAYBACK) {
			resident->game_mode = GM_NONE;
		}
		replay_done_write(RTX_ERROR_FRAME_IO);
		replay_protect_local_free();
		replay_resident_handoff_clear();
		replay_mode = REPLAY_DISABLED;
		return;
	}
	if(
		(route != 0) &&
		(
			(replay_mode == REPLAY_USER_RECORD) ||
			(replay_mode == REPLAY_USER_PLAYBACK)
		)
	) {
		if(replay_mode == REPLAY_USER_RECORD) {
			if(!replay_user_header_write(RUS_RECORDING, RUER_PARTIAL)) {
				replay_guard_diag_write();
			}
		}
		replay_handoff_cursor_store();
		replay_protect_local_free();
		replay_mode = REPLAY_DISABLED;
		return;
	}

	if(replay_mode == REPLAY_RECORD) {
		replay_header_write();
	} else if(replay_mode == REPLAY_USER_RECORD) {
		if(
			(route == 0) &&
			(replay_user_discard_requested || replay_protect_invalid())
		) {
			if(replay_protect_invalid()) {
				replay_guard_diag_write();
			}
			replay_protect_file_delete_commit(replay_user_fn);
			replay_user_index_slot_clear();
			replay_user_guard_delete();
		} else {
			if(!replay_user_header_write(
				((route == 0) ? RUS_FINALIZED : RUS_PARTIAL),
				((route == 0) ? RUER_MENU_RETURN : RUER_PARTIAL)
			)) {
				finish_error = true;
				replay_guard_diag_write();
			} else if(route == 0) {
				replay_user_guard_delete();
				save_pending = true;
			}
		}
		replay_protect_local_free();
		replay_resident_handoff_clear();
	} else if(replay_mode == REPLAY_USER_PLAYBACK) {
		resident->game_mode = GM_NONE;
		replay_protect_local_free();
		replay_resident_handoff_clear();
	}
	if(replay_mode != REPLAY_DISABLED) {
		if(replay_mode == REPLAY_USER_RECORD) {
			if(finish_error) {
				replay_done_write(RTX_ERROR_USER_HEADER);
			} else if(
				(route == 0) &&
				(replay_user_discard_requested || replay_protect_invalid())
			) {
				replay_done_write(RTX_OK_MENU_RETURN_NOSAVE);
			} else {
				replay_done_write(
					(route == 0) ? RTX_OK_MENU_RETURN : RTX_OK_PARTIAL
				);
			}
		} else if(replay_mode == REPLAY_USER_PLAYBACK) {
			replay_done_write(RTX_OK_USER_PLAYBACK);
		} else {
			replay_done_write(RTX_OK);
		}
	}
	if(replay_restart_requested_flag && (route == 0)) {
		replay_resident_handoff_mode_set(T3_REPLAY_RES_MODE_RESTART);
	} else if(save_pending) {
		replay_resident_handoff_mode_set(T3_REPLAY_RES_MODE_SAVE_DIRECT);
	}
	replay_mode = REPLAY_DISABLED;
}

// Keep the following C runtime segment at its accepted paragraph phase.
#if defined(TH03_REPLAY_DEV_OVERLAY)
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#else
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#endif
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
