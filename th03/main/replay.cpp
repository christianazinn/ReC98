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
#include "th03/replay_format.hpp"
#include "th03/replay_handoff.hpp"
#include "th03/resident.hpp"
#include "th03/replay_protect.hpp"
#include "th03/snd/snd.h"

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
	RTX_SPLIT_HEADER,
	RTX_CRLF,
	RTX_EVENT,
	RTX_GLOBAL_FRAME,
	RTX_ROUND_FRAME,
	RTX_ROUND_OR_RESULT_FRAME,
	RTX_ROUTE,
	RTX_GAME_MODE,
	RTX_STORY_STAGE,
	RTX_ROUND_ID,
	RTX_WINNER,
	RTX_P1_SCORE,
	RTX_P2_SCORE,
	RTX_RESIDENT_RAND,
	RTX_ROUND_SPEED,
	RTX_STATE_HASH,
	RTX_START,
	RTX_ROUND_START,
	RTX_INPUT_END,
	RTX_ERROR,
	RTX_CHECKPOINT,
	RTX_FINISH,
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
static uint32_t replay_packet_tag_offset;
static uint8_t replay_last_route;
static uint8_t replay_rle_phase;
static uint8_t replay_rle_run;
static uint16_t replay_rle_input_mp_p1;
static uint16_t replay_rle_input_mp_p2;
static uint16_t replay_rle_input_sp;
static bool replay_done_written;
static bool replay_paths_initialized;
static bool replay_prompt_skip_queued;
static bool replay_rle_packet_open;
static bool replay_user_discard_requested;
static bool replay_guard_diag_written;
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
static bool replay_user_header_is_v3(void);
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

static bool replay_write_u16_checked(uint16_t value)
{
	return replay_write_bytes_checked(&value, sizeof(value));
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
	T3_SPLIT_FN[8] = 'T';
	T3_SPLIT_FN[9] = 'S';
	T3_SPLIT_FN[10] = 'V';
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
		&resident->unused_3[T3R_DIAG_CODE_INDEX],
		(T3R_DIAG_END_INDEX - T3R_DIAG_CODE_INDEX)
	);
	file_close();
}

static void replay_write_text(replay_text_id_t text)
{
#define W(c) replay_write_char(c)
	switch(text) {
	case RTX_SPLIT_HEADER:
		replay_write_text(RTX_EVENT);
		W('\t');
		replay_write_text(RTX_GLOBAL_FRAME);
		W('\t');
		replay_write_text(RTX_ROUND_FRAME);
		W('\t');
		replay_write_text(RTX_ROUND_OR_RESULT_FRAME);
		W('\t');
		replay_write_text(RTX_ROUTE);
		W('\t');
		replay_write_text(RTX_GAME_MODE);
		W('\t');
		replay_write_text(RTX_STORY_STAGE);
		W('\t');
		replay_write_text(RTX_ROUND_ID);
		W('\t');
		replay_write_text(RTX_WINNER);
		W('\t');
		replay_write_text(RTX_P1_SCORE);
		W('\t');
		replay_write_text(RTX_P2_SCORE);
		W('\t');
		replay_write_text(RTX_RESIDENT_RAND);
		W('\t');
		replay_write_text(RTX_ROUND_SPEED);
		W('\t');
		replay_write_text(RTX_STATE_HASH);
		replay_write_text(RTX_CRLF);
		break;
	case RTX_CRLF:
		W('\r');
		W('\n');
		break;
	case RTX_EVENT:
		W('e'); W('v'); W('e'); W('n'); W('t');
		break;
	case RTX_GLOBAL_FRAME:
		W('g'); W('l'); W('o'); W('b'); W('a'); W('l'); W('_');
		W('f'); W('r'); W('a'); W('m'); W('e');
		break;
	case RTX_ROUND_FRAME:
		W('r'); W('o'); W('u'); W('n'); W('d'); W('_');
		W('f'); W('r'); W('a'); W('m'); W('e');
		break;
	case RTX_ROUND_OR_RESULT_FRAME:
		W('r'); W('o'); W('u'); W('n'); W('d'); W('_'); W('o'); W('r');
		W('_'); W('r'); W('e'); W('s'); W('u'); W('l'); W('t'); W('_');
		W('f'); W('r'); W('a'); W('m'); W('e');
		break;
	case RTX_ROUTE:
		W('r'); W('o'); W('u'); W('t'); W('e');
		break;
	case RTX_GAME_MODE:
		W('g'); W('a'); W('m'); W('e'); W('_'); W('m'); W('o'); W('d');
		W('e');
		break;
	case RTX_STORY_STAGE:
		W('s'); W('t'); W('o'); W('r'); W('y'); W('_'); W('s'); W('t');
		W('a'); W('g'); W('e');
		break;
	case RTX_ROUND_ID:
		W('r'); W('o'); W('u'); W('n'); W('d'); W('_'); W('i'); W('d');
		break;
	case RTX_WINNER:
		W('w'); W('i'); W('n'); W('n'); W('e'); W('r');
		break;
	case RTX_P1_SCORE:
		W('p'); W('1'); W('_'); W('s'); W('c'); W('o'); W('r'); W('e');
		break;
	case RTX_P2_SCORE:
		W('p'); W('2'); W('_'); W('s'); W('c'); W('o'); W('r'); W('e');
		break;
	case RTX_RESIDENT_RAND:
		W('r'); W('e'); W('s'); W('i'); W('d'); W('e'); W('n'); W('t');
		W('_'); W('r'); W('a'); W('n'); W('d');
		break;
	case RTX_ROUND_SPEED:
		W('r'); W('o'); W('u'); W('n'); W('d'); W('_'); W('s'); W('p');
		W('e'); W('e'); W('d');
		break;
	case RTX_STATE_HASH:
		W('s'); W('t'); W('a'); W('t'); W('e'); W('_'); W('h'); W('a');
		W('s'); W('h');
		break;
	case RTX_START:
		W('s'); W('t'); W('a'); W('r'); W('t');
		break;
	case RTX_ROUND_START:
		W('r'); W('o'); W('u'); W('n'); W('d'); W('_');
		replay_write_text(RTX_START);
		break;
	case RTX_INPUT_END:
		W('i'); W('n'); W('p'); W('u'); W('t'); W('_'); W('e'); W('n');
		W('d');
		break;
	case RTX_ERROR:
		W('e'); W('r'); W('r'); W('o'); W('r');
		break;
	case RTX_CHECKPOINT:
		W('c'); W('h'); W('e'); W('c'); W('k'); W('p'); W('o'); W('i');
		W('n'); W('t');
		break;
	case RTX_FINISH:
		W('f'); W('i'); W('n'); W('i'); W('s'); W('h');
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

static void replay_write_u32(uint32_t value)
{
	char digits[10];
	int i = 0;

	do {
		digits[i] = static_cast<char>('0' + (value % 10));
		value /= 10;
		i++;
	} while(value != 0);

	while(i != 0) {
		i--;
		replay_write_char(digits[i]);
	}
}

static void replay_write_i32(int32_t value)
{
	if(value < 0) {
		replay_write_char('-');
		replay_write_u32(static_cast<uint32_t>(-value));
	} else {
		replay_write_u32(static_cast<uint32_t>(value));
	}
}

static void replay_write_hex_nibble(uint8_t value)
{
	value &= 0x0F;
	if(value < 10) {
		replay_write_char(static_cast<char>('0' + value));
	} else {
		replay_write_char(static_cast<char>('A' + (value - 10)));
	}
}

static void replay_write_hex32(uint32_t value)
{
	int shift;

	for(shift = 28; shift >= 0; shift -= 4) {
		replay_write_hex_nibble(static_cast<uint8_t>(value >> shift));
	}
}

static void replay_write_score(const unsigned char near *digits)
{
	int digit;

	for(digit = (SCORE_DIGITS - 1); digit >= 0; digit--) {
		replay_write_char(static_cast<char>('0' + (digits[digit] % 10)));
	}
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

	if(resident->game_mode == GM_STORY) {
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
	for(i = 0; i < T3_REPLAY_USER_STAGE_COUNT; i++) {
		replay_sum_stage_opps[i] = (
			replay_user_header.stage_opponents[i]
		);
		for(j = 0; j < T3_REPLAY_USER_PACKED_SCORE_SIZE; j++) {
			replay_sum_stage_scores[i][j] = (
				replay_user_header.stage_scores[i][j]
			);
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
	for(i = 0; i < T3_REPLAY_USER_STAGE_COUNT; i++) {
		replay_user_header.stage_opponents[i] = (
			replay_sum_stage_opps[i]
		);
		for(j = 0; j < T3_REPLAY_USER_PACKED_SCORE_SIZE; j++) {
			replay_user_header.stage_scores[i][j] = (
				replay_sum_stage_scores[i][j]
			);
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
	return replay_protect_guard_create(guard_fn);
}

static bool replay_user_guard_verify(void)
{
	char guard_fn[13];

	replay_user_guard_fn_set(guard_fn);
	return replay_protect_verify(guard_fn);
}

static bool replay_user_guard_checkpoint(void)
{
	char guard_fn[13];

	replay_user_guard_fn_set(guard_fn);
	return replay_protect_checkpoint(guard_fn);
}

static void replay_user_guard_delete(void)
{
	char guard_fn[13];

	replay_user_guard_fn_set(guard_fn);
	dos_axdx(0x4100, guard_fn);
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
	replay_user_index_header.magic[6] = '3';
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
	replay_user_index_entry.resident_rand = replay_user_header.resident_rand;
	replay_user_index_entry.random_seed_snapshot = (
		replay_user_header.random_seed_snapshot
	);
	replay_user_index_entry.input_crc32 = replay_user_header.input_crc32;
	replay_user_index_entry.snapshot_crc32 = replay_user_header.snapshot_crc32;
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
	for(i = 0; i < T3_REPLAY_USER_STAGE_COUNT; i++) {
		replay_user_index_entry.stage_opponents[i] = (
			replay_user_header.stage_opponents[i]
		);
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
	file_create(T3_SPLIT_FN);
	replay_write_text(RTX_SPLIT_HEADER);
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

static void replay_split_row(replay_text_id_t event, uint8_t route)
{
	if((replay_mode == REPLAY_DISABLED) || (replay_mode == REPLAY_ERROR)) {
		return;
	}
	if(!file_append(T3_SPLIT_FN)) {
		replay_mode = REPLAY_ERROR;
		replay_done_write(RTX_ERROR_SPLIT_OPEN);
		return;
	}

	replay_write_text(event);
	replay_write_char('\t');
	replay_write_u32(replay_global_frame);
	replay_write_char('\t');
	replay_write_u32(round_frame);
	replay_write_char('\t');
	replay_write_u32(round_or_result_frame);
	replay_write_char('\t');
	replay_write_u32(route);
	replay_write_char('\t');
	replay_write_u32(resident->game_mode);
	replay_write_char('\t');
	replay_write_u32(resident->story_stage);
	replay_write_char('\t');
	replay_write_u32(round_id);
	replay_write_char('\t');
	replay_write_i32(resident->pid_winner);
	replay_write_char('\t');
	replay_write_score(score);
	replay_write_char('\t');
	replay_write_score(score + SCORE_DIGITS);
	replay_write_char('\t');
	replay_write_i32(resident->rand);
	replay_write_char('\t');
	replay_write_u32(round_speed);
	replay_write_char('\t');
	replay_write_hex32(replay_state_hash());
	replay_write_text(RTX_CRLF);
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
	replay_user_summary_init_from_snapshot();
	replay_user_summary_ext_init();
}

static void replay_user_header_fill(
	replay_user_status_t status, replay_user_end_reason_t end_reason
)
{
	replay_memclear(&replay_user_header, sizeof(replay_user_header));
	replay_user_header.magic[0] = 'T';
	replay_user_header.magic[1] = '3';
	replay_user_header.magic[2] = 'R';
	replay_user_header.magic[3] = 'P';
	replay_user_header.magic[4] = 'L';
	replay_user_header.magic[5] = 'Y';
	replay_user_header.magic[6] = '4';
	replay_user_header.magic[7] = '\0';
	replay_user_header.version = T3_REPLAY_USER_VERSION;
	replay_user_header.header_size = (
		sizeof(replay_user_header) + sizeof(replay_user_summary_ext)
	);
	replay_user_header.sample_size = T3_REPLAY_USER_SAMPLE_SIZE_RLE;
	replay_user_header.flags = T3_REPLAY_USER_FLAG_RLE_INPUT;
	replay_user_header.status = status;
	replay_user_header.end_reason = end_reason;
	replay_user_header.game_mode = replay_user_snapshot.game_mode;
	replay_user_header.rank = replay_user_snapshot.rank;
	replay_user_header.key_mode = replay_user_snapshot.key_mode;
	replay_user_header.playchar_p1 = replay_user_snapshot.playchar_paletted[0];
	replay_user_header.playchar_p2 = replay_user_snapshot.playchar_paletted[1];
	replay_user_header.story_stage = replay_user_snapshot.story_stage;
	replay_user_header.is_cpu_p1 = replay_user_snapshot.is_cpu[0];
	replay_user_header.is_cpu_p2 = replay_user_snapshot.is_cpu[1];
	replay_user_header.sample_count = replay_sample_count;
	replay_user_header.final_frame_count = replay_global_frame;
	replay_user_header.resident_rand = replay_user_snapshot.resident_rand;
	replay_user_header.random_seed_snapshot = (
		replay_user_snapshot.random_seed_snapshot
	);
	replay_user_header.snapshot_offset = replay_user_header.header_size;
	replay_user_header.snapshot_size = sizeof(replay_user_snapshot);
	replay_user_header.input_offset = (
		static_cast<uint32_t>(replay_user_header.header_size) +
		static_cast<uint32_t>(sizeof(replay_user_snapshot))
	);
	replay_user_header.input_size = replay_input_byte_count;
	replay_user_header.snapshot_crc32 = replay_hash_bytes(
		5381, &replay_user_snapshot, sizeof(replay_user_snapshot)
	);
	replay_user_summary_copy_to_header();
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
	if(replay_protect_blocked()) {
		return false;
	}
	if(!replay_user_guard_verify()) {
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
	if(!replay_protect_close_current_file(RPD_COMMIT_FLUSH)) {
		return false;
	}
	replay_user_index_slot_write(status, end_reason);
	return true;
}

static bool replay_user_header_is_v2(void)
{
	return (
		(replay_user_header.magic[6] == '2') &&
		(replay_user_header.version == T3_REPLAY_USER_VERSION_V2) &&
		(replay_user_header.sample_size == sizeof(replay_user_sample_t))
	);
}

static bool replay_user_header_is_v3(void)
{
	return (
		(replay_user_header.magic[6] == '3') &&
		(replay_user_header.version == T3_REPLAY_USER_VERSION_V3) &&
		(replay_user_header.sample_size == T3_REPLAY_USER_SAMPLE_SIZE_RLE) &&
		((replay_user_header.flags & T3_REPLAY_USER_FLAG_RLE_INPUT) != 0)
	);
}

static bool replay_user_header_is_v4(void)
{
	return (
		(replay_user_header.magic[6] == '4') &&
		(replay_user_header.version == T3_REPLAY_USER_VERSION) &&
		(replay_user_header.sample_size == T3_REPLAY_USER_SAMPLE_SIZE_RLE) &&
		((replay_user_header.flags & T3_REPLAY_USER_FLAG_RLE_INPUT) != 0)
	);
}

static bool replay_user_header_is_rle(void)
{
	return (replay_user_header_is_v3() || replay_user_header_is_v4());
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
		(replay_user_header_is_v2() || replay_user_header_is_rle()) &&
		(
			(
				(replay_user_header_is_v2() || replay_user_header_is_v3()) &&
				(replay_user_header.header_size == sizeof(replay_user_header))
			) ||
			(
				replay_user_header_is_v4() &&
				(replay_user_header.header_size == (
					sizeof(replay_user_header) +
					sizeof(replay_user_summary_ext)
				))
			)
		) &&
		(replay_user_header.snapshot_offset == replay_user_header.header_size) &&
		(replay_user_header.snapshot_size == sizeof(replay_user_snapshot)) &&
		(replay_user_header.input_offset == (
			static_cast<uint32_t>(replay_user_header.header_size) +
			static_cast<uint32_t>(sizeof(replay_user_snapshot))
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
	if(replay_user_header_is_v4()) {
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
	}
	file_seek(replay_user_header.snapshot_offset, SEEK_SET);
	if(
		file_read(&replay_user_snapshot, sizeof(replay_user_snapshot)) !=
		sizeof(replay_user_snapshot)
	) {
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
	replay_user_header_fill(RUS_RECORDING, RUER_PARTIAL);
	replay_user_slot_fn_set(slot);
	if(slot < T3_REPLAY_USER_SLOT_COUNT) {
		replay_dir_create();
	}
	if(!file_create(replay_user_fn)) {
		replay_user_slot_fn_set(T3_REPLAY_USER_SLOT_NONE);
		if(!file_create(replay_user_fn)) {
			return false;
		}
	}
	if(!replay_write_bytes_checked(&replay_user_header, sizeof(replay_user_header))) {
		file_close();
		return false;
	}
	if(!replay_write_bytes_checked(
		&replay_user_summary_ext, sizeof(replay_user_summary_ext)
	)) {
		file_close();
		return false;
	}
	if(!replay_write_bytes_checked(&replay_user_snapshot, sizeof(replay_user_snapshot))) {
		file_close();
		return false;
	}
	if(!replay_protect_close_current_file(RPD_COMMIT_FLUSH)) {
		return false;
	}
	if(!replay_user_guard_create()) {
		replay_guard_diag_write();
		replay_user_index_slot_clear();
		return true;
	}
	replay_user_index_slot_write(RUS_RECORDING, RUER_PARTIAL);
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

static bool replay_user_record_rle_sample(uint8_t phase)
{
	uint16_t input_p1 = input_mp_p1;
	uint16_t input_p2 = input_mp_p2;
	uint16_t input_single = input_sp;
	uint8_t change = 0;
	uint8_t tag;
	uint32_t offset;
	uint32_t new_input_byte_count;

	if(replay_protect_blocked()) {
		return true;
	}

	if(
		replay_rle_packet_open &&
		(replay_rle_phase == phase) &&
		(replay_rle_input_mp_p1 == input_p1) &&
		(replay_rle_input_mp_p2 == input_p2) &&
		(replay_rle_input_sp == input_single) &&
		(replay_rle_run < T3_REPLAY_PACKET_RUN_MAX)
	) {
		tag = replay_user_rle_tag(phase, replay_rle_run + 1);
		if(!replay_user_guard_checkpoint()) {
			return true;
		}
		replay_rle_run++;
		if(!file_append(replay_user_fn)) {
			replay_protect_detector_error_set();
			return true;
		}
		file_seek(replay_packet_tag_offset, SEEK_SET);
		if(!replay_write_bytes_checked(&tag, sizeof(tag))) {
			file_close();
			replay_protect_detector_error_set();
			return true;
		}
		file_close();
		replay_sample_count++;
		return true;
	}

	if(input_p1 != replay_rle_input_mp_p1) {
		change |= T3_REPLAY_PACKET_CHANGE_P1;
	}
	if(input_p2 != replay_rle_input_mp_p2) {
		change |= T3_REPLAY_PACKET_CHANGE_P2;
	}
	if(input_single != replay_rle_input_sp) {
		change |= T3_REPLAY_PACKET_CHANGE_SP;
	}

	offset = (replay_user_header.input_offset + replay_input_byte_count);
	new_input_byte_count = replay_input_byte_count;
	tag = replay_user_rle_tag(phase, 1);
	if(!replay_user_guard_checkpoint()) {
		return true;
	}
	if(!file_append(replay_user_fn)) {
		replay_protect_detector_error_set();
		return true;
	}
	file_seek(offset, SEEK_SET);
	if(
		!replay_write_bytes_checked(&tag, sizeof(tag)) ||
		!replay_write_bytes_checked(&change, sizeof(change))
	) {
		file_close();
		replay_protect_detector_error_set();
		return true;
	}
	new_input_byte_count += 2;
	if(change & T3_REPLAY_PACKET_CHANGE_P1) {
		if(!replay_write_u16_checked(input_p1)) {
			file_close();
			replay_protect_detector_error_set();
			return true;
		}
		new_input_byte_count += sizeof(input_p1);
	}
	if(change & T3_REPLAY_PACKET_CHANGE_P2) {
		if(!replay_write_u16_checked(input_p2)) {
			file_close();
			replay_protect_detector_error_set();
			return true;
		}
		new_input_byte_count += sizeof(input_p2);
	}
	if(change & T3_REPLAY_PACKET_CHANGE_SP) {
		if(!replay_write_u16_checked(input_single)) {
			file_close();
			replay_protect_detector_error_set();
			return true;
		}
		new_input_byte_count += sizeof(input_single);
	}
	file_close();

	replay_input_byte_count = new_input_byte_count;
	replay_packet_tag_offset = offset;
	replay_rle_phase = phase;
	replay_rle_run = 1;
	replay_rle_input_mp_p1 = input_p1;
	replay_rle_input_mp_p2 = input_p2;
	replay_rle_input_sp = input_single;
	replay_rle_packet_open = true;
	replay_sample_count++;
	return true;
}

static bool replay_user_record_logical_sample(uint8_t phase)
{
	if(replay_user_header_is_rle()) {
		return replay_user_record_rle_sample(phase);
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
	replay_rle_phase = static_cast<uint8_t>(
		tag >> T3_REPLAY_PACKET_PHASE_SHIFT
	);
	replay_rle_run = static_cast<uint8_t>(
		(tag & T3_REPLAY_PACKET_RUN_MASK) + 1
	);
	if(replay_rle_phase > T3_REPLAY_PACKET_PHASE_INTERSTITIAL) {
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
	if(change & ~(T3_REPLAY_PACKET_CHANGE_P1 | T3_REPLAY_PACKET_CHANGE_P2 | T3_REPLAY_PACKET_CHANGE_SP)) {
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
	if(replay_rle_phase != phase) {
		return false;
	}
	input_mp_p1 = replay_rle_input_mp_p1;
	input_mp_p2 = replay_rle_input_mp_p2;
	input_sp = replay_rle_input_sp;
	replay_rle_run--;
	replay_sample_count++;
	return true;
}

static bool replay_user_play_logical_sample(uint8_t phase)
{
	if(replay_user_header_is_rle()) {
		return replay_user_play_rle_sample(phase);
	}
	if(phase == T3_REPLAY_PACKET_PHASE_INTERSTITIAL) {
		return replay_user_play_interstitial_sample();
	}
	return replay_user_play_sample();
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
	if((replay_global_frame & 63) == 0) {
		replay_split_row(RTX_CHECKPOINT, replay_last_route);
		if(replay_mode == REPLAY_USER_RECORD) {
			replay_user_header_write(RUS_RECORDING, RUER_PARTIAL);
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

void far replay_session_start(void)
{
	replay_paths_init();
	replay_protect_local_reset();

	replay_mode = replay_resident_mode();
	if(replay_mode == REPLAY_DISABLED) {
		replay_mode = replay_cfg_mode();
	}
	replay_sample_count = 0;
	replay_global_frame = 0;
	replay_input_byte_count = 0;
	replay_packet_tag_offset = 0;
	replay_last_route = 0;
	replay_rle_phase = T3_REPLAY_PACKET_PHASE_GAMEPLAY;
	replay_rle_run = 0;
	replay_rle_input_mp_p1 = 0;
	replay_rle_input_mp_p2 = 0;
	replay_rle_input_sp = 0;
	replay_done_written = false;
	replay_user_slot_fn_set(T3_REPLAY_USER_SLOT_NONE);
	replay_prompt_skip_queued = false;
	replay_rle_packet_open = false;
	replay_user_discard_requested = false;
	replay_guard_diag_written = false;

	if(replay_mode == REPLAY_DISABLED) {
		return;
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

	file_create(T3_DONE_FN);
	file_close();
	replay_split_write_header();

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
			replay_mode = REPLAY_ERROR;
			replay_done_write(RTX_ERROR_USER_CREATE);
			return;
		} else {
			replay_user_guard_verify();
		}
	} else if(replay_mode == REPLAY_USER_PLAYBACK) {
		if(!replay_user_read()) {
			replay_mode = REPLAY_ERROR;
			replay_done_write(RTX_ERROR_USER_HEADER);
			return;
		}
		if(replay_sample_count == 0) {
			replay_user_snapshot_restore_resident();
			replay_user_snapshot_restore_runtime();
		}
	} else if(!replay_header_read()) {
		replay_mode = REPLAY_ERROR;
		replay_done_write(RTX_ERROR_INPUT_HEADER);
		return;
	}

	replay_split_row(RTX_START, 0);
}

void far replay_round_start(void)
{
	replay_split_row(RTX_ROUND_START, replay_last_route);
}

void far replay_frame_io(void)
{
	bool ok = true;

	if(replay_mode == REPLAY_DISABLED) {
		return;
	}

	if(replay_mode == REPLAY_RECORD) {
		ok = replay_record_sample();
	} else if(replay_mode == REPLAY_USER_RECORD) {
		ok = replay_user_record_logical_sample(T3_REPLAY_PACKET_PHASE_GAMEPLAY);
	} else if(replay_mode == REPLAY_USER_PLAYBACK) {
		if(replay_user_playback_cancel()) {
			return;
		}
		if(replay_sample_count >= replay_user_header.sample_count) {
			replay_split_row(RTX_INPUT_END, replay_last_route);
			input_sp |= INPUT_CANCEL;
			replay_done_write(RTX_OK_USER_INPUT_END);
			replay_resident_handoff_clear();
			replay_mode = REPLAY_DISABLED;
			return;
		}
		ok = replay_user_play_logical_sample(T3_REPLAY_PACKET_PHASE_GAMEPLAY);
	} else if(replay_mode == REPLAY_PLAYBACK) {
		if(replay_sample_count >= replay_header.sample_count) {
			replay_split_row(RTX_INPUT_END, replay_last_route);
			input_sp |= INPUT_CANCEL;
			replay_done_write(RTX_OK_INPUT_END);
			replay_mode = REPLAY_DISABLED;
			return;
		}
		ok = replay_play_sample();
	}

	if(!ok) {
		replay_split_row(RTX_ERROR, replay_last_route);
		replay_mode = REPLAY_ERROR;
		input_sp |= INPUT_CANCEL;
		replay_done_write(RTX_ERROR_FRAME_IO);
		return;
	}

	if((replay_global_frame & 63) == 0) {
		replay_split_row(RTX_CHECKPOINT, replay_last_route);
		if(replay_mode == REPLAY_RECORD) {
			replay_header_write();
		} else if(replay_mode == REPLAY_USER_RECORD) {
			replay_user_header_write(RUS_RECORDING, RUER_PARTIAL);
			replay_handoff_cursor_store();
		}
	}
	replay_global_frame++;
	replay_handoff_cursor_store();
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
		if(replay_mode == REPLAY_USER_RECORD) {
			ok = replay_user_record_logical_sample(
				T3_REPLAY_PACKET_PHASE_INTERSTITIAL
			);
		}
	}

	if(!ok) {
		replay_split_row(RTX_ERROR, replay_last_route);
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

static void replay_text_putca(unsigned x, unsigned y, int ch, unsigned atrb)
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
	grc_setclip(0, 0, (RES_X - 1), (SPRITE16_RES_Y - 1));
	grcg_setcolor(GC_RMW, 0);
	graph_accesspage(0);
	grcg_boxfill(
		REPLAY_PAUSE_PIXEL_LEFT, REPLAY_PAUSE_PIXEL_TOP,
		REPLAY_PAUSE_PIXEL_RIGHT, REPLAY_PAUSE_PIXEL_BOTTOM
	);
	graph_accesspage(1);
	grcg_boxfill(
		REPLAY_PAUSE_PIXEL_LEFT, REPLAY_PAUSE_PIXEL_TOP,
		REPLAY_PAUSE_PIXEL_RIGHT, REPLAY_PAUSE_PIXEL_BOTTOM
	);
	grcg_off();
	graph_accesspage(page_front);
}

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
	P('E'); P('x'); P('i'); P('t'); P(' '); P('w'); P('i'); P('t');
	P('h'); P('o'); P('u'); P('t'); P(' '); P('S'); P('a'); P('v');
	P('i'); P('n'); P('g');
#undef P
}

static bool replay_pause_save_disabled(void)
{
	if(replay_mode != REPLAY_USER_RECORD) {
		return false;
	}
	if(!replay_protect_blocked()) {
		replay_user_guard_verify();
	}
	if(replay_protect_blocked()) {
		replay_guard_diag_write();
	}
	return replay_protect_blocked();
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
		sel = REPLAY_PAUSE_RESUME;
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
	replay_text_putca(REPLAY_PAUSE_CHOICE_MARK_LEFT, y, (
		(sel == REPLAY_PAUSE_RESUME) ? '>' : ' '
	), atrb);
	replay_pause_put_resume(y, atrb);

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
			replay_text_putca(
				(REPLAY_PAUSE_LEFT + x), (REPLAY_PAUSE_TOP + y),
				' ', replay_pause_clear_atrb(REPLAY_PAUSE_LEFT + x)
			);
		}
	}
}

static void replay_pause_wait_release(void)
{
	goto release_test;

release_wait:
	replay_input_sense_held();
	frame_delay(1);

release_test:
	if(input_sp != INPUT_NONE) {
		goto release_wait;
	}
}

static void replay_pause_beep(void)
{
	snd_se_reset();
	snd_se_play(21);
	snd_se_update();
}

uint8_t far replay_pause_menu(void)
{
	uint8_t sel = REPLAY_PAUSE_RESUME;

	replay_pause_beep();
	replay_pause_wait_release();
	replay_pause_put_graph_backing();
	replay_pause_put_frame();
	replay_pause_put_title();
	replay_pause_put_choices(sel);

input_wait:
	replay_input_sense_held();
	if(input_sp & INPUT_Q) {
		return REPLAY_PAUSE_DISCARD_EXIT;
	}
	if(input_sp & INPUT_CANCEL) {
		replay_pause_wait_release();
		replay_pause_clear();
		replay_pause_beep();
		return REPLAY_PAUSE_RESUME;
	}
	if(input_sp & INPUT_UP) {
		sel = replay_pause_prev_choice(sel);
		replay_pause_put_choices(sel);
		replay_pause_beep();
		replay_pause_wait_release();
		goto input_wait;
	}
	if(input_sp & INPUT_DOWN) {
		sel = replay_pause_next_choice(sel);
		replay_pause_put_choices(sel);
		replay_pause_beep();
		replay_pause_wait_release();
		goto input_wait;
	}
	if(input_sp & (INPUT_OK | INPUT_SHOT)) {
		if(sel == REPLAY_PAUSE_RESUME) {
			replay_pause_wait_release();
			replay_pause_clear();
			replay_pause_beep();
		}
		return sel;
	}
	frame_delay(1);
	goto input_wait;
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

void far replay_route(uint8_t route)
{
	replay_last_route = route;
	replay_user_round_split_capture(route);
	replay_user_summary_capture(route);
	replay_split_row(RTX_ROUTE, route);
}

void far replay_finish(uint8_t route)
{
	bool finish_error = false;

	replay_split_row(RTX_FINISH, route);
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
			dos_axdx(0x4100, replay_user_fn);
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
			}
		}
		replay_protect_local_free();
		replay_resident_handoff_clear();
	} else if(replay_mode == REPLAY_USER_PLAYBACK) {
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
	replay_mode = REPLAY_DISABLED;
}
