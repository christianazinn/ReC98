/* TH01 OP replay and Practice surface.
 *
 * This module deliberately owns only title-side file inspection and transient
 * Practice selection. REIIDEN validates complete replay payloads and remains
 * the only process that can enter playback.
 */

#pragma option -zCT1REPLAY_OP_TEXT -G-

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform/x86real/pc98/keyboard.hpp"
#include "th01/common.h"
#include "th01/hardware/egc.h"
#include "th01/hardware/graph.h"
#include "th01/hardware/grppsafx.h"
#include "th01/hardware/grp_text.hpp"
#include "th01/replay_op.hpp"
#include "th01/resident.hpp"
#include "th01/rank.h"

static const screen_x_t T1REPLAY_OP_LEFT = 152;
static const pixel_t T1REPLAY_OP_W = 336;
static const screen_y_t T1REPLAY_OP_TOP = 96;
static const pixel_t T1REPLAY_OP_H = 240;
static const screen_x_t T1REPLAY_OP_VALUE_LEFT = 344;
static const screen_y_t T1REPLAY_OP_LINE_H = GLYPH_H;
static const uint8_t T1REPLAY_OP_ROWS_PER_PAGE = 10;
static const vc_t T1REPLAY_OP_COL_LABEL = 5;
static const vc_t T1REPLAY_OP_COL_VALUE = 15;
static const vc_t T1REPLAY_OP_COL_DISABLED = 3;
static const int16_t T1REPLAY_OP_FX = FX_WEIGHT_BLACK;
static const uint8_t T1REPLAY_OP_POINT_VALUE_COUNT = 17;
static const uint16_t T1REPLAY_OP_POINT_CAP = 65530;

struct t1replay_op_slot_t {
	bool exists;
	bool valid;
	t1replay_header_t header;
};

struct t1replay_op_input_t {
	bool up;
	bool down;
	bool left;
	bool right;
	bool ok;
	bool cancel;
};

static uint8_t t1replay_op_sel;
static uint8_t t1replay_op_page;
static bool t1replay_op_wait_release;
static t1replay_practice_start_t t1replay_practice_start;
static bool t1replay_op_prev_up;
static bool t1replay_op_prev_down;
static bool t1replay_op_prev_left;
static bool t1replay_op_prev_right;
static bool t1replay_op_prev_ok;
static bool t1replay_op_prev_cancel;

static void t1replay_op_slot_fn(char *fn, uint8_t slot)
{
	fn[0] = 'T'; fn[1] = 'H'; fn[2] = '1'; fn[3] = 'R';
	fn[4] = static_cast<char>('0' + (slot / 10));
	fn[5] = static_cast<char>('0' + (slot % 10));
	fn[6] = '.'; fn[7] = 'R'; fn[8] = 'P'; fn[9] = 'Y'; fn[10] = '\0';
}

static void t1replay_op_command_fn(char *fn)
{
	fn[0] = 'T'; fn[1] = '1'; fn[2] = 'R'; fn[3] = 'P'; fn[4] = 'Y';
	fn[5] = '.'; fn[6] = 'C'; fn[7] = 'F'; fn[8] = 'G'; fn[9] = '\0';
}

static uint32_t t1replay_op_fnv1a(uint32_t hash, const void *buf, unsigned size)
{
	const uint8_t *p = reinterpret_cast<const uint8_t *>(buf);

	while(size != 0) {
		hash ^= *p++;
		hash *= T1REPLAY_FNV1A_PRIME;
		size--;
	}
	return hash;
}

static bool t1replay_op_bytes_zero(const uint8_t *p, unsigned size)
{
	while(size != 0) {
		if(*p++ != 0) {
			return false;
		}
		size--;
	}
	return true;
}

static bool t1replay_op_magic_matches(const char *magic, char last)
{
	return (
		(magic[0] == 'T') && (magic[1] == '1') &&
		(magic[2] == 'R') && (magic[3] == 'P') &&
		(magic[4] == 'Y') && (magic[5] == last) &&
		(magic[6] == '\0') && (magic[7] == '\0')
	);
}

static bool t1replay_op_start_valid(const t1replay_start_t *start)
{
	return (
		(start->stage_id < STAGE_COUNT) &&
		(start->rank >= 0) && (start->rank <= RANK_LUNATIC) &&
		(start->bgm_mode >= BGM_MODE_OFF) && (start->bgm_mode < BGM_MODE_COUNT) &&
		(start->route >= ROUTE_MAKAI) && (start->route < ROUTE_COUNT) &&
		(start->end_flag >= ES_NONE) && (start->end_flag <= ES_JIGOKU) &&
		(start->debug_mode == DM_OFF) &&
		(start->snd_need_init >= 0) && (start->snd_need_init <= 1) &&
		(start->mode_test == 0) &&
		(start->start_binary == T1REPLAY_PROCESS_REIIDEN) &&
		t1replay_op_bytes_zero(start->reserved, sizeof(start->reserved))
	);
}

static bool t1replay_op_header_valid(t1replay_header_t *header)
{
	uint32_t stored_header_checksum = header->header_checksum;
	uint32_t header_checksum;
	uint32_t start_checksum;

	if(
		!t1replay_op_magic_matches(header->magic, '1') ||
		(header->version != T1REPLAY_VERSION) ||
		(header->header_size != T1REPLAY_HEADER_SIZE) ||
		(header->packet_size != T1REPLAY_PACKET_SIZE) ||
		(header->flags != T1REPLAY_FLAGS_KNOWN) ||
		(header->status != T1REPLAY_STATUS_FINALIZED) ||
		((header->end_reason != T1REPLAY_END_MENU) &&
		 (header->end_reason != T1REPLAY_END_CLEAR)) ||
		(header->game_id != 1) ||
		(header->input_semantics != T1REPLAY_INPUT_SEMANTICS_LATCHED_GROUPS) ||
		(header->process_count == 0) ||
		(header->reserved_0 != 0) ||
		(header->input_offset != T1REPLAY_HEADER_SIZE) ||
		(header->packet_count > (T1REPLAY_INPUT_SIZE_MAX / T1REPLAY_PACKET_SIZE)) ||
		(header->input_size != (header->packet_count * T1REPLAY_PACKET_SIZE)) ||
		(header->input_size > T1REPLAY_INPUT_SIZE_MAX) ||
		!t1replay_op_bytes_zero(header->reserved, sizeof(header->reserved)) ||
		!t1replay_op_start_valid(&header->start)
	) {
		return false;
	}
	start_checksum = t1replay_op_fnv1a(
		T1REPLAY_FNV1A_BASIS, &header->start, sizeof(header->start)
	);
	if(start_checksum != header->start_checksum) {
		return false;
	}
	header->header_checksum = 0;
	header_checksum = t1replay_op_fnv1a(
		T1REPLAY_FNV1A_BASIS, header, sizeof(*header)
	);
	header->header_checksum = stored_header_checksum;
	return (header_checksum == stored_header_checksum);
}

static void t1replay_op_slot_read(uint8_t slot, t1replay_op_slot_t& result)
{
	char fn[11];
	char mode[3];
	FILE *fp;

	memset(&result, 0, sizeof(result));
	if(slot >= T1REPLAY_SLOT_COUNT) {
		return;
	}
	t1replay_op_slot_fn(fn, slot);
	mode[0] = 'r'; mode[1] = 'b'; mode[2] = '\0';
	fp = fopen(fn, mode);
	if(!fp) {
		return;
	}
	result.exists = true;
	if(fread(&result.header, 1, sizeof(result.header), fp) == sizeof(result.header)) {
		result.valid = t1replay_op_header_valid(&result.header);
	}
	fclose(fp);
}

static bool t1replay_op_command_write(uint8_t mode, uint8_t slot)
{
	t1replay_command_t command;
	char fn[10];
	char fopen_mode[3];
	FILE *fp;
	bool ok;

	if(
		((mode != T1REPLAY_COMMAND_RECORD) &&
		 (mode != T1REPLAY_COMMAND_PLAYBACK)) ||
		(slot >= T1REPLAY_SLOT_COUNT)
	) {
		return false;
	}
	memset(&command, 0, sizeof(command));
	command.magic[0] = 'T'; command.magic[1] = '1';
	command.magic[2] = 'R'; command.magic[3] = 'P';
	command.magic[4] = 'Y'; command.magic[5] = 'C';
	command.mode = mode;
	command.slot = slot;
	t1replay_op_command_fn(fn);
	fopen_mode[0] = 'w'; fopen_mode[1] = 'b'; fopen_mode[2] = '\0';
	fp = fopen(fn, fopen_mode);
	if(!fp) {
		return false;
	}
	ok = (fwrite(&command, 1, sizeof(command), fp) == sizeof(command));
	if(fclose(fp) != 0) {
		ok = false;
	}
	if(!ok) {
		remove(fn);
	}
	return ok;
}

static int t1replay_op_first_empty_slot(void)
{
	t1replay_op_slot_t slot;
	int i;

	for(i = 0; i < T1REPLAY_SLOT_COUNT; i++) {
		t1replay_op_slot_read(static_cast<uint8_t>(i), slot);
		if(!slot.exists) {
			return i;
		}
	}
	return -1;
}

static void t1replay_op_backing_restore(void)
{
	egc_copy_rect_1_to_0_16(
		T1REPLAY_OP_LEFT, T1REPLAY_OP_TOP, T1REPLAY_OP_W, T1REPLAY_OP_H
	);
}

/* Keep renderer strings out of initialized DGROUP.  Aside from preserving the
 * original OP data layout, this keeps every new title-surface string in this
 * module's private code path. */
enum t1replay_op_word_t {
	T1ROW_REPLAY_BROWSER,
	T1ROW_SLOT_STATUS_SCORE_STAGE_RANK,
	T1ROW_EMPTY,
	T1ROW_INVALID,
	T1ROW_CLEAR,
	T1ROW_MENU,
	T1ROW_PAGE,
	T1ROW_ESC_RETURN,
	T1ROW_PRACTICE,
	T1ROW_SCENE,
	T1ROW_ROUTE,
	T1ROW_SECTION,
	T1ROW_CHAPTER,
	T1ROW_RANK,
	T1ROW_SCORE,
	T1ROW_LIVES,
	T1ROW_BOMBS,
	T1ROW_POINT_VALUE,
	T1ROW_PELLET_SPEED,
	T1ROW_RNG_SEED,
	T1ROW_START,
	T1ROW_BACK,
	T1ROW_SHRINE,
	T1ROW_SCENE_II,
	T1ROW_SCENE_III,
	T1ROW_SCENE_IV,
	T1ROW_MAKAI,
	T1ROW_JIGOKU,
	T1ROW_STAGE_START,
	T1ROW_BOSS_START,
	T1ROW_EASY,
	T1ROW_NORMAL,
	T1ROW_HARD,
	T1ROW_LUNATIC,
};

static char t1replay_op_text[64];

static char *t1replay_op_word_append(char *p, t1replay_op_word_t word)
{
	#define T1ROW_PUTC(c) *p++ = (c)
	#define T1ROW_SPACE() T1ROW_PUTC(' ')
	#define T1ROW_WORD1(a) T1ROW_PUTC(a)
	#define T1ROW_WORD2(a, b) T1ROW_WORD1(a); T1ROW_WORD1(b)
	#define T1ROW_WORD3(a, b, c) T1ROW_WORD2(a, b); T1ROW_WORD1(c)
	#define T1ROW_WORD4(a, b, c, d) T1ROW_WORD3(a, b, c); T1ROW_WORD1(d)
	switch(word) {
	case T1ROW_REPLAY_BROWSER:
		T1ROW_WORD4('R', 'E', 'P', 'L'); T1ROW_WORD2('A', 'Y'); T1ROW_SPACE(); T1ROW_WORD3('B', 'R', 'O'); T1ROW_WORD3('W', 'S', 'E'); T1ROW_WORD1('R'); break;
	case T1ROW_SLOT_STATUS_SCORE_STAGE_RANK:
		T1ROW_WORD4('S', 'L', 'O', 'T'); T1ROW_SPACE(); T1ROW_SPACE(); T1ROW_WORD4('S', 'T', 'A', 'T'); T1ROW_WORD2('U', 'S'); T1ROW_SPACE(); T1ROW_SPACE(); T1ROW_WORD4('S', 'C', 'O', 'R'); T1ROW_WORD1('E'); T1ROW_SPACE(); T1ROW_SPACE(); T1ROW_WORD4('S', 'T', 'A', 'G'); T1ROW_WORD1('E'); T1ROW_SPACE(); T1ROW_SPACE(); T1ROW_WORD4('R', 'A', 'N', 'K'); break;
	case T1ROW_EMPTY: T1ROW_WORD4('E', 'M', 'P', 'T'); T1ROW_WORD1('Y'); break;
	case T1ROW_INVALID: T1ROW_WORD4('I', 'N', 'V', 'A'); T1ROW_WORD3('L', 'I', 'D'); break;
	case T1ROW_CLEAR: T1ROW_WORD4('C', 'L', 'E', 'A'); T1ROW_WORD1('R'); break;
	case T1ROW_MENU: T1ROW_WORD4('M', 'E', 'N', 'U'); break;
	case T1ROW_PAGE: T1ROW_WORD4('P', 'A', 'G', 'E'); break;
	case T1ROW_ESC_RETURN: T1ROW_WORD3('E', 'S', 'C'); T1ROW_PUTC(':'); T1ROW_SPACE(); T1ROW_WORD4('R', 'E', 'T', 'U'); T1ROW_WORD2('R', 'N'); break;
	case T1ROW_PRACTICE: T1ROW_WORD4('P', 'R', 'A', 'C'); T1ROW_WORD4('T', 'I', 'C', 'E'); break;
	case T1ROW_SCENE: T1ROW_WORD4('S', 'C', 'E', 'N'); T1ROW_WORD1('E'); break;
	case T1ROW_ROUTE: T1ROW_WORD4('R', 'O', 'U', 'T'); T1ROW_WORD1('E'); break;
	case T1ROW_SECTION: T1ROW_WORD4('S', 'E', 'C', 'T'); T1ROW_WORD3('I', 'O', 'N'); break;
	case T1ROW_CHAPTER: T1ROW_WORD4('C', 'H', 'A', 'P'); T1ROW_WORD3('T', 'E', 'R'); break;
	case T1ROW_RANK: T1ROW_WORD4('R', 'A', 'N', 'K'); break;
	case T1ROW_SCORE: T1ROW_WORD4('S', 'C', 'O', 'R'); T1ROW_WORD1('E'); break;
	case T1ROW_LIVES: T1ROW_WORD4('L', 'I', 'V', 'E'); T1ROW_WORD1('S'); break;
	case T1ROW_BOMBS: T1ROW_WORD4('B', 'O', 'M', 'B'); T1ROW_WORD1('S'); break;
	case T1ROW_POINT_VALUE: T1ROW_WORD4('P', 'O', 'I', 'N'); T1ROW_WORD1('T'); T1ROW_SPACE(); T1ROW_WORD4('V', 'A', 'L', 'U'); T1ROW_WORD1('E'); break;
	case T1ROW_PELLET_SPEED: T1ROW_WORD4('P', 'E', 'L', 'L'); T1ROW_WORD2('E', 'T'); T1ROW_SPACE(); T1ROW_WORD4('S', 'P', 'E', 'E'); T1ROW_WORD1('D'); break;
	case T1ROW_RNG_SEED: T1ROW_WORD3('R', 'N', 'G'); T1ROW_SPACE(); T1ROW_WORD4('S', 'E', 'E', 'D'); break;
	case T1ROW_START: T1ROW_WORD4('S', 'T', 'A', 'R'); T1ROW_WORD1('T'); break;
	case T1ROW_BACK: T1ROW_WORD4('B', 'A', 'C', 'K'); break;
	case T1ROW_SHRINE: T1ROW_WORD4('S', 'H', 'R', 'I'); T1ROW_WORD2('N', 'E'); break;
	case T1ROW_SCENE_II: T1ROW_WORD4('S', 'C', 'E', 'N'); T1ROW_WORD1('E'); T1ROW_SPACE(); T1ROW_WORD2('I', 'I'); break;
	case T1ROW_SCENE_III: T1ROW_WORD4('S', 'C', 'E', 'N'); T1ROW_WORD1('E'); T1ROW_SPACE(); T1ROW_WORD3('I', 'I', 'I'); break;
	case T1ROW_SCENE_IV: T1ROW_WORD4('S', 'C', 'E', 'N'); T1ROW_WORD1('E'); T1ROW_SPACE(); T1ROW_WORD2('I', 'V'); break;
	case T1ROW_MAKAI: T1ROW_WORD4('M', 'A', 'K', 'A'); T1ROW_WORD1('I'); break;
	case T1ROW_JIGOKU: T1ROW_WORD4('J', 'I', 'G', 'O'); T1ROW_WORD2('K', 'U'); break;
	case T1ROW_STAGE_START: T1ROW_WORD4('S', 'T', 'A', 'G'); T1ROW_WORD1('E'); T1ROW_SPACE(); T1ROW_WORD4('S', 'T', 'A', 'R'); T1ROW_WORD1('T'); break;
	case T1ROW_BOSS_START: T1ROW_WORD4('B', 'O', 'S', 'S'); T1ROW_SPACE(); T1ROW_WORD4('S', 'T', 'A', 'R'); T1ROW_WORD1('T'); break;
	case T1ROW_EASY: T1ROW_WORD4('E', 'A', 'S', 'Y'); break;
	case T1ROW_NORMAL: T1ROW_WORD4('N', 'O', 'R', 'M'); T1ROW_WORD2('A', 'L'); break;
	case T1ROW_HARD: T1ROW_WORD4('H', 'A', 'R', 'D'); break;
	case T1ROW_LUNATIC: T1ROW_WORD4('L', 'U', 'N', 'A'); T1ROW_WORD3('T', 'I', 'C'); break;
	}
	#undef T1ROW_WORD4
	#undef T1ROW_WORD3
	#undef T1ROW_WORD2
	#undef T1ROW_WORD1
	#undef T1ROW_SPACE
	#undef T1ROW_PUTC
	return p;
}

static char *t1replay_op_uint_append(char *p, uint32_t value, uint8_t width)
{
	char digits[10];
	uint8_t count = 0;

	do {
		digits[count++] = static_cast<char>('0' + (value % 10));
		value /= 10;
	} while(value != 0);
	while(count < width) {
		digits[count++] = '0';
	}
	while(count != 0) {
		*p++ = digits[--count];
	}
	return p;
}

static char *t1replay_op_pellet_speed_append(char *p, pellet_speed_t speed)
{
	uint16_t magnitude;

	if(speed < 0) {
		*p++ = '-';
		magnitude = static_cast<uint16_t>(-speed);
	} else {
		magnitude = speed;
	}
	p = t1replay_op_uint_append(p, magnitude / PELLET_SPEED_MULTIPLIER, 1);
	*p++ = '.';
	return t1replay_op_uint_append(
		p, ((magnitude % PELLET_SPEED_MULTIPLIER) * 100) /
			PELLET_SPEED_MULTIPLIER, 2
	);
}

// The native collection sequence has 17 user-selectable values: 0, 1000 to
// 9000, 10000 to 60000, and the overflow-safe POINT_CAP from stage/item.cpp.
// Keep the mapping in code instead of a static table so this title-only parcel
// still contributes no initialized DGROUP data.
static uint8_t t1replay_op_point_value_index(uint16_t point_value)
{
	if(point_value <= 9000) {
		return static_cast<uint8_t>(point_value / 1000);
	}
	if(point_value <= 60000) {
		return static_cast<uint8_t>(9 + (point_value / 10000));
	}
	return (T1REPLAY_OP_POINT_VALUE_COUNT - 1);
}

static uint16_t t1replay_op_point_value_from_index(uint8_t index)
{
	if(index <= 9) {
		return static_cast<uint16_t>(index * 1000);
	}
	if(index < (T1REPLAY_OP_POINT_VALUE_COUNT - 1)) {
		return static_cast<uint16_t>((index - 9) * 10000);
	}
	return T1REPLAY_OP_POINT_CAP;
}

static uint16_t t1replay_op_point_value_change(uint16_t point_value, int delta)
{
	uint8_t index = t1replay_op_point_value_index(point_value);

	if(delta < 0) {
		index = (index == 0) ? (T1REPLAY_OP_POINT_VALUE_COUNT - 1) :
			static_cast<uint8_t>(index - 1);
	} else {
		index = (index == (T1REPLAY_OP_POINT_VALUE_COUNT - 1)) ? 0 :
			static_cast<uint8_t>(index + 1);
	}
	return t1replay_op_point_value_from_index(index);
}

static void t1replay_op_text_left(screen_y_t y, vc_t col, char *end)
{
	*end = '\0';
	graph_putsa_fx(T1REPLAY_OP_LEFT, y, (col | T1REPLAY_OP_FX), t1replay_op_text);
}

static void t1replay_op_text_value(screen_y_t y, vc_t col, char *end)
{
	*end = '\0';
	graph_putsa_fx(
		T1REPLAY_OP_VALUE_LEFT, y, (col | T1REPLAY_OP_FX), t1replay_op_text
	);
}

static char *t1replay_op_rank_append(char *p, int8_t rank)
{
	t1replay_op_word_t word = T1ROW_EASY;

	if(rank == RANK_NORMAL) {
		word = T1ROW_NORMAL;
	} else if(rank == RANK_HARD) {
		word = T1ROW_HARD;
	} else if(rank == RANK_LUNATIC) {
		word = T1ROW_LUNATIC;
	}
	return t1replay_op_word_append(p, word);
}

static char *t1replay_op_scene_append(char *p, uint8_t scene)
{
	t1replay_op_word_t word = T1ROW_SHRINE;

	if(scene == 1) {
		word = T1ROW_SCENE_II;
	} else if(scene == 2) {
		word = T1ROW_SCENE_III;
	} else if(scene == 3) {
		word = T1ROW_SCENE_IV;
	}
	return t1replay_op_word_append(p, word);
}

static char *t1replay_op_route_append(char *p, uint8_t route)
{
	return t1replay_op_word_append(
		p, (route == ROUTE_JIGOKU) ? T1ROW_JIGOKU : T1ROW_MAKAI
	);
}

static char *t1replay_op_section_append(char *p, uint8_t section)
{
	t1replay_op_word_t word = T1ROW_STAGE_START;

	if(section == T1RPS_CHAPTER) {
		word = T1ROW_CHAPTER;
	} else if(section == T1RPS_BOSS_START) {
		word = T1ROW_BOSS_START;
	}
	return t1replay_op_word_append(p, word);
}

static void t1replay_op_input_read(t1replay_op_input_t& input)
{
	REGS in;
	REGS out7;
	REGS out8;
	REGS out9;
	REGS out0;
	REGS out3;
	REGS out5;
	bool up;
	bool down;
	bool left;
	bool right;
	bool ok;
	bool cancel;

	in.h.ah = 0x04;
	in.h.al = 7; int86(0x18, &in, &out7);
	in.h.al = 8; int86(0x18, &in, &out8);
	in.h.al = 9; int86(0x18, &in, &out9);
	in.h.al = 0; int86(0x18, &in, &out0);
	in.h.al = 3; int86(0x18, &in, &out3);
	in.h.al = 5; int86(0x18, &in, &out5);
	up = ((out7.h.ah & K7_ARROW_UP) || (out8.h.ah & K8_NUM_8));
	down = ((out7.h.ah & K7_ARROW_DOWN) || (out9.h.ah & K9_NUM_2));
	left = ((out7.h.ah & K7_ARROW_LEFT) || (out8.h.ah & K8_NUM_4));
	right = ((out7.h.ah & K7_ARROW_RIGHT) || (out9.h.ah & K9_NUM_6));
	ok = ((out3.h.ah & K3_RETURN) || (out5.h.ah & K5_Z));
	cancel = (out0.h.ah & K0_ESC);
	if(t1replay_op_wait_release) {
		input.up = input.down = input.left = input.right = input.ok = input.cancel = false;
		if(!up && !down && !left && !right && !ok && !cancel) {
			t1replay_op_wait_release = false;
		}
	} else {
		input.up = (up && !t1replay_op_prev_up);
		input.down = (down && !t1replay_op_prev_down);
		input.left = (left && !t1replay_op_prev_left);
		input.right = (right && !t1replay_op_prev_right);
		input.ok = (ok && !t1replay_op_prev_ok);
		input.cancel = (cancel && !t1replay_op_prev_cancel);
	}
	t1replay_op_prev_up = up;
	t1replay_op_prev_down = down;
	t1replay_op_prev_left = left;
	t1replay_op_prev_right = right;
	t1replay_op_prev_ok = ok;
	t1replay_op_prev_cancel = cancel;
}

static void t1replay_op_input_reset(void)
{
	t1replay_op_prev_up = false;
	t1replay_op_prev_down = false;
	t1replay_op_prev_left = false;
	t1replay_op_prev_right = false;
	t1replay_op_prev_ok = false;
	t1replay_op_prev_cancel = false;
	t1replay_op_wait_release = true;
}

static void t1replay_op_replay_render(void)
{
	t1replay_op_slot_t slot;
	char *p;
	screen_y_t y = T1REPLAY_OP_TOP;
	uint8_t row;
	uint8_t slot_id;
	vc_t col;

	t1replay_op_backing_restore();
	p = t1replay_op_word_append(t1replay_op_text, T1ROW_REPLAY_BROWSER);
	t1replay_op_text_left(y, T1REPLAY_OP_COL_VALUE, p);
	y += (T1REPLAY_OP_LINE_H * 2);
	p = t1replay_op_word_append(t1replay_op_text, T1ROW_SLOT_STATUS_SCORE_STAGE_RANK);
	t1replay_op_text_left(y, T1REPLAY_OP_COL_LABEL, p);
	y += T1REPLAY_OP_LINE_H;
	for(row = 0; row < T1REPLAY_OP_ROWS_PER_PAGE; row++) {
		slot_id = static_cast<uint8_t>((t1replay_op_page * T1REPLAY_OP_ROWS_PER_PAGE) + row);
		t1replay_op_slot_read(slot_id, slot);
		col = ((t1replay_op_sel == row) ? T1REPLAY_OP_COL_VALUE : T1REPLAY_OP_COL_LABEL);
		p = t1replay_op_uint_append(t1replay_op_text, slot_id, 2);
		*p++ = ' '; *p++ = ' '; *p++ = ' '; *p++ = ' ';
		if(!slot.exists) {
			p = t1replay_op_word_append(p, T1ROW_EMPTY);
		} else if(!slot.valid) {
			p = t1replay_op_word_append(p, T1ROW_INVALID);
			col = T1REPLAY_OP_COL_DISABLED;
		} else {
			p = t1replay_op_word_append(
				p, (slot.header.end_reason == T1REPLAY_END_CLEAR) ?
					T1ROW_CLEAR : T1ROW_MENU
			);
			*p++ = ' '; *p++ = ' ';
			p = t1replay_op_uint_append(p, slot.header.start.score, 9);
			*p++ = ' '; *p++ = ' ';
			p = t1replay_op_uint_append(p, slot.header.start.stage_id + 1, 2);
			*p++ = ' '; *p++ = ' '; *p++ = ' '; *p++ = ' '; *p++ = ' ';
			p = t1replay_op_rank_append(p, slot.header.start.rank);
		}
		t1replay_op_text_left(y, col, p);
		y += T1REPLAY_OP_LINE_H;
	}
	p = t1replay_op_word_append(t1replay_op_text, T1ROW_PAGE);
	*p++ = ' ';
	p = t1replay_op_uint_append(p, t1replay_op_page + 1, 2);
	*p++ = '/'; *p++ = '1'; *p++ = '0';
	t1replay_op_text_left(y, T1REPLAY_OP_COL_LABEL, p);
	y += T1REPLAY_OP_LINE_H;
	p = t1replay_op_word_append(t1replay_op_text, T1ROW_ESC_RETURN);
	t1replay_op_text_left(y, T1REPLAY_OP_COL_LABEL, p);
}

static void t1replay_op_practice_render(void)
{
	char *p;
	screen_y_t y = T1REPLAY_OP_TOP;
	uint8_t row = 0;
	vc_t label_col;
	vc_t value_col;

	t1replay_op_backing_restore();
	p = t1replay_op_word_append(t1replay_op_text, T1ROW_PRACTICE);
	t1replay_op_text_left(y, T1REPLAY_OP_COL_VALUE, p);
	y += (T1REPLAY_OP_LINE_H * 2);
	#define T1REPLAY_OP_PRACTICE_LINE(label_word, value_append) \
		label_col = ((t1replay_op_sel == row) ? T1REPLAY_OP_COL_VALUE : T1REPLAY_OP_COL_LABEL); \
		value_col = label_col; \
		p = t1replay_op_word_append(t1replay_op_text, label_word); \
		t1replay_op_text_left(y, label_col, p); \
		p = (value_append); \
		t1replay_op_text_value(y, value_col, p); \
		y += T1REPLAY_OP_LINE_H; row++
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_SCENE,
		t1replay_op_scene_append(t1replay_op_text, t1replay_practice_start.scene)
	);
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_ROUTE,
		t1replay_op_word_append(
			t1replay_op_text,
			(t1replay_practice_start.scene == 0) ? T1ROW_SHRINE :
				((t1replay_practice_start.route == ROUTE_JIGOKU) ?
					T1ROW_JIGOKU : T1ROW_MAKAI)
		)
	);
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_SECTION,
		t1replay_op_section_append(t1replay_op_text, t1replay_practice_start.section)
	);
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_CHAPTER,
		(t1replay_practice_start.section == T1RPS_CHAPTER) ?
			t1replay_op_uint_append(t1replay_op_text, t1replay_practice_start.chapter + 1, 1) :
			(t1replay_op_text[0] = '-', t1replay_op_text + 1)
	);
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_RANK,
		t1replay_op_rank_append(t1replay_op_text, t1replay_practice_start.rank)
	);
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_SCORE,
		t1replay_op_uint_append(t1replay_op_text, t1replay_practice_start.score, 1)
	);
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_LIVES,
		t1replay_op_uint_append(t1replay_op_text, t1replay_practice_start.lives, 1)
	);
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_BOMBS,
		t1replay_op_uint_append(t1replay_op_text, t1replay_practice_start.bombs, 1)
	);
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_POINT_VALUE,
		t1replay_op_uint_append(t1replay_op_text, t1replay_practice_start.point_value, 1)
	);
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_PELLET_SPEED,
		t1replay_op_pellet_speed_append(t1replay_op_text, t1replay_practice_start.pellet_speed)
	);
	T1REPLAY_OP_PRACTICE_LINE(
		T1ROW_RNG_SEED,
		t1replay_op_uint_append(t1replay_op_text, t1replay_practice_start.rand, 1)
	);
	T1REPLAY_OP_PRACTICE_LINE(T1ROW_START, t1replay_op_text);
	T1REPLAY_OP_PRACTICE_LINE(T1ROW_BACK, t1replay_op_text);
	#undef T1REPLAY_OP_PRACTICE_LINE
}

void t1replay_op_replay_enter(void)
{
	t1replay_op_sel = 0;
	t1replay_op_page = 0;
	t1replay_op_input_reset();
	t1replay_op_replay_render();
}

void t1replay_op_practice_enter(int8_t rank, int8_t lives, int8_t bombs, uint32_t rand)
{
	memset(&t1replay_practice_start, 0, sizeof(t1replay_practice_start));
	t1replay_practice_start.scene = 0;
	t1replay_practice_start.route = ROUTE_MAKAI;
	t1replay_practice_start.section = T1RPS_STAGE_START;
	t1replay_practice_start.rank = rank;
	t1replay_practice_start.lives = lives;
	t1replay_practice_start.bombs = bombs;
	t1replay_practice_start.pellet_speed = to_pellet_speed(-0.1f);
	t1replay_practice_start.rand = rand;
	t1replay_op_sel = 0;
	t1replay_op_input_reset();
	t1replay_op_practice_render();
}

void t1replay_op_restore(void)
{
	t1replay_op_backing_restore();
}

void t1replay_op_command_clear(void)
{
	char fn[10];

	t1replay_op_command_fn(fn);
	remove(fn);
}

t1replay_op_result_t t1replay_op_replay_update(void)
{
	t1replay_op_input_t input;
	t1replay_op_slot_t slot;
	t1replay_op_result_t result;

	result.action = T1ROA_NONE;
	result.slot = 0;

	t1replay_op_input_read(input);
	if(input.cancel) {
		result.action = T1ROA_RETURN;
		return result;
	}
	if(input.up) {
		if(t1replay_op_sel == 0) {
			t1replay_op_sel = (T1REPLAY_OP_ROWS_PER_PAGE - 1);
		} else {
			t1replay_op_sel--;
		}
		t1replay_op_replay_render();
	}
	if(input.down) {
		t1replay_op_sel = static_cast<uint8_t>(
			(t1replay_op_sel + 1) % T1REPLAY_OP_ROWS_PER_PAGE
		);
		t1replay_op_replay_render();
	}
	if(input.left) {
		if(t1replay_op_page == 0) {
			t1replay_op_page = 9;
		} else {
			t1replay_op_page--;
		}
		t1replay_op_replay_render();
	}
	if(input.right) {
		t1replay_op_page = static_cast<uint8_t>((t1replay_op_page + 1) % 10);
		t1replay_op_replay_render();
	}
	if(input.ok) {
		result.slot = static_cast<uint8_t>(
			(t1replay_op_page * T1REPLAY_OP_ROWS_PER_PAGE) + t1replay_op_sel
		);
		t1replay_op_slot_read(result.slot, slot);
		if(slot.valid && t1replay_op_command_write(T1REPLAY_COMMAND_PLAYBACK, result.slot)) {
			result.action = T1ROA_PLAYBACK;
		}
	}
	return result;
}

static void t1replay_op_practice_change(int delta)
{
	switch(t1replay_op_sel) {
	case 0:
		t1replay_practice_start.scene = static_cast<uint8_t>(
			(t1replay_practice_start.scene + SCENE_COUNT + delta) % SCENE_COUNT
		);
		if(t1replay_practice_start.scene == 0) {
			t1replay_practice_start.route = ROUTE_MAKAI;
		}
		break;
	case 1:
		if(t1replay_practice_start.scene != 0) {
			t1replay_practice_start.route = static_cast<uint8_t>(
				(t1replay_practice_start.route + ROUTE_COUNT + delta) % ROUTE_COUNT
			);
		}
		break;
	case 2:
		t1replay_practice_start.section = static_cast<uint8_t>(
			(t1replay_practice_start.section + 3 + delta) % 3
		);
		break;
	case 3:
		if(t1replay_practice_start.section == T1RPS_CHAPTER) {
			t1replay_practice_start.chapter = static_cast<uint8_t>(
				(t1replay_practice_start.chapter + BOSS_STAGE + delta) % BOSS_STAGE
			);
		}
		break;
	case 4:
		t1replay_practice_start.rank = static_cast<int8_t>(
			(t1replay_practice_start.rank + RANK_COUNT + delta) % RANK_COUNT
		);
		break;
	case 5:
		t1replay_practice_start.score += (delta * 10000L);
		if(t1replay_practice_start.score < 0) {
			t1replay_practice_start.score = 99990000L;
		} else if(t1replay_practice_start.score > 99990000L) {
			t1replay_practice_start.score = 0;
		}
		break;
	case 6:
		t1replay_practice_start.lives += delta;
		if(t1replay_practice_start.lives < 1) {
			t1replay_practice_start.lives = LIVES_MAX;
		} else if(t1replay_practice_start.lives > LIVES_MAX) {
			t1replay_practice_start.lives = 1;
		}
		break;
	case 7:
		t1replay_practice_start.bombs += delta;
		if(t1replay_practice_start.bombs < 0) {
			t1replay_practice_start.bombs = BOMBS_MAX;
		} else if(t1replay_practice_start.bombs > BOMBS_MAX) {
			t1replay_practice_start.bombs = 0;
		}
		break;
	case 8:
		t1replay_practice_start.point_value = t1replay_op_point_value_change(
			t1replay_practice_start.point_value, delta
		);
		break;
	case 9:
		t1replay_practice_start.pellet_speed += delta;
		if(t1replay_practice_start.pellet_speed < PELLET_SPEED_LOWER_MIN) {
			t1replay_practice_start.pellet_speed = PELLET_SPEED_RAISE_MAX;
		} else if(t1replay_practice_start.pellet_speed > PELLET_SPEED_RAISE_MAX) {
			t1replay_practice_start.pellet_speed = PELLET_SPEED_LOWER_MIN;
		}
		break;
	case 10:
		t1replay_practice_start.rand += delta;
		break;
	}
}

t1replay_op_result_t t1replay_op_practice_update(void)
{
	t1replay_op_input_t input;
	t1replay_op_result_t result;
	int slot;

	result.action = T1ROA_NONE;
	result.slot = 0;

	t1replay_op_input_read(input);
	if(input.cancel) {
		result.action = T1ROA_RETURN;
		return result;
	}
	if(input.up) {
		if(t1replay_op_sel == 0) {
			t1replay_op_sel = 12;
		} else {
			t1replay_op_sel--;
		}
		t1replay_op_practice_render();
	}
	if(input.down) {
		t1replay_op_sel = static_cast<uint8_t>((t1replay_op_sel + 1) % 13);
		t1replay_op_practice_render();
	}
	if(input.left) {
		t1replay_op_practice_change(-1);
		t1replay_op_practice_render();
	}
	if(input.right) {
		t1replay_op_practice_change(1);
		t1replay_op_practice_render();
	}
	if(input.ok) {
		if(t1replay_op_sel == 11) {
			slot = t1replay_op_first_empty_slot();
			if((slot >= 0) && t1replay_op_command_write(
				T1REPLAY_COMMAND_RECORD, static_cast<uint8_t>(slot)
			)) {
				result.action = T1ROA_PRACTICE_RECORD;
				result.slot = static_cast<uint8_t>(slot);
			} else {
				result.action = T1ROA_PRACTICE_UNRECORDED;
			}
		} else if(t1replay_op_sel == 12) {
			result.action = T1ROA_RETURN;
		}
	}
	return result;
}

void t1replay_op_practice_start_get(t1replay_practice_start_t& start)
{
	start = t1replay_practice_start;
	start.route = (start.scene == 0) ? ROUTE_MAKAI : start.route;
	if(start.section == T1RPS_BOSS_START) {
		start.chapter = BOSS_STAGE;
	} else if(start.section == T1RPS_STAGE_START) {
		start.chapter = 0;
	}
}
