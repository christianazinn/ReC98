#pragma option -zCRPYFONT_TEXT -zPRPYFONT_TEXT -dc

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/rank.h"
#include "th01/hardware/grppsafx.h"
#include "th02/v_colors.hpp"
#include "th03/common.h"
#include "th03/language.hpp"
#include "th03/menu_font.hpp"
#include "th03/opfont.hpp"
#include "th03/practice.hpp"
#include "th03/replay_format.hpp"
#include "th03/rpyfont.hpp"
#include "th03/shiftjis/main.hpp"
#include <stddef.h>

enum {
	LIST_LEFT = 4,
	LIST_W = 72,
	HEAD_Y = 5,
	DETAIL_Y = 5,
	LIST_PIXEL_LEFT = (LIST_LEFT * GLYPH_HALF_W),
	CURSOR_PIXEL_LEFT = (LIST_PIXEL_LEFT + 8),
	SLOT_PIXEL_LEFT = 64,
	NAME_PIXEL_LEFT = 104,
	SCORE_PIXEL_RIGHT = 340,
	CHAR_PIXEL_LEFT = 364,
	RANK_PIXEL_LEFT = 476,
	STAGE_PIXEL_LEFT = 560,
	DETAIL_PIXEL_LEFT = 48,
	DETAIL_PIXEL_RIGHT = 304,
	DETAIL_STORY_CURSOR_LEFT = 328,
	DETAIL_STORY_PIXEL_LEFT = 344,
	DETAIL_STORY_OPPONENT_LEFT = 376,
	DETAIL_STORY_ROUND_LEFT = 400,
	DETAIL_STORY_SCORE_RIGHT = 592,
	DETAIL_ROUND_CURSOR_LEFT = 304,
	DETAIL_ROUND_PIXEL_LEFT = 328,
	DETAIL_ROUND_P1_SCORE_RIGHT = 452,
	DETAIL_ROUND_P2_SCORE_RIGHT = 568,
	DETAIL_ROUND_WINNER_LEFT = 584,
	DETAIL_SPLIT_ROWS_Y = 8,
	DETAIL_SPLIT_ROWS_VISIBLE = 14,
	DETAIL_PRACTICE_SETTINGS_Y = 12,
	REPLAY_FONT_FIXED_NUMERIC = 0x100,
	REPLAY_FONT_FIXED_NAME = 0x200,
	REPLAY_FONT_FIXED_MASK = (
		REPLAY_FONT_FIXED_NUMERIC | REPLAY_FONT_FIXED_NAME
	),
	REPLAY_FONT_SELECTED_COLOR = 12,
};

extern char replay_menu_line[81];
extern replay_user_header_t replay_user_menu_header;
extern replay_user_menu_summary_ext_t replay_user_menu_summary_ext;
extern replay_user_snapshot_t replay_user_menu_snapshot;
extern uint32_t far replay_user_menu_round_real_frames[
	T3_REPLAY_USER_ROUND_SPLIT_COUNT
];
extern replay_user_stage_clear_bonus_t far
	replay_user_menu_stage_clear_bonuses[T3_REPLAY_USER_STAGE_COUNT];
extern uint32_t far replay_user_menu_timed_frames;
extern uint32_t far replay_user_menu_slow_frames;

static bool summary_valid(void)
{
	return (
		(replay_user_menu_header.summary_flags & T3_REPLAY_USER_SUMMARY_VALID) !=
		0
	);
}

static bool replay_vs(void)
{
	return (replay_user_menu_header.game_mode >= GM_VS);
}

static bool replay_practice(void)
{
	return (
		(replay_user_menu_header.flags & T3_REPLAY_USER_FLAG_PRACTICE) != 0
	);
}

static bool round_summary_valid(void)
{
	return (
		(replay_user_menu_summary_ext.flags & T3_REPLAY_USER_SUMMARY_VALID) != 0
	);
}

static replay_user_menu_round_split_t near *round_split_at(
	uint8_t stage, uint8_t round
)
{
	uint8_t i;
	replay_user_menu_round_split_t near *split;

	if(!round_summary_valid()) {
		return NULL;
	}
	for(i = 0; i < replay_user_menu_summary_ext.round_reached_count; i++) {
		split = &replay_user_menu_summary_ext.round_splits[i];
		if(
			((split->stage_round & 0x0F) == stage) &&
			((split->stage_round >> 4) == round)
		) {
			return split;
		}
	}
	return NULL;
}

static int round_split_index_at(uint8_t stage, uint8_t round)
{
	uint8_t i;
	replay_user_menu_round_split_t near *split;

	if(!round_summary_valid()) {
		return -1;
	}
	for(i = 0; i < replay_user_menu_summary_ext.round_reached_count; i++) {
		split = &replay_user_menu_summary_ext.round_splits[i];
		if(
			((split->stage_round & 0x0F) == stage) &&
			((split->stage_round >> 4) == round)
		) {
			return i;
		}
	}
	return -1;
}

static replay_user_menu_round_split_t near *round_split(uint8_t round)
{
	return round_split_at(
		(replay_practice() ?
			replay_user_menu_header.scenario.practice.config.stage :
			T3_REPLAY_USER_ROUND_STAGE_VS),
		round
	);
}

static int packed_score_cmp(
	const uint8_t near *left, const uint8_t near *right
)
{
	uint8_t l;
	uint8_t r;
	int digit;

	for(digit = (T3_REPLAY_USER_SCORE_DIGITS - 1); digit >= 0; digit--) {
		l = left[digit / 2];
		r = right[digit / 2];
		if(digit & 1) {
			l >>= 4;
			r >>= 4;
		}
		l &= 0x0F;
		r &= 0x0F;
		if(l != r) {
			return (l - r);
		}
	}
	return 0;
}

static const uint8_t near *list_score(void)
{
	replay_user_menu_round_split_t near *split;
	uint8_t round;

	if(replay_practice()) {
		return replay_user_menu_header.final_score;
	}
	if(replay_vs()) {
		for(round = 2; round != 0xFF; round--) {
			split = round_split(round);
			if(split != NULL) {
				if(replay_user_menu_header.game_mode == GM_VS_1P_CPU) {
					return split->score_p1;
				}
				if(replay_user_menu_header.game_mode == GM_VS_1P_2P) {
					if(replay_user_menu_header.final_winner == 0) {
						return split->score_p1;
					}
					if(replay_user_menu_header.final_winner == 1) {
						return split->score_p2;
					}
				}
				if(packed_score_cmp(split->score_p2, split->score_p1) > 0) {
					return split->score_p2;
				}
				return split->score_p1;
			}
		}
	}
	return replay_user_menu_header.final_score;
}

static uint8_t stage_opponent(uint8_t stage, bool show_unreached)
{
	if(
		summary_valid() &&
		(stage < replay_user_menu_header.stage_reached_count)
	) {
		return replay_user_menu_header.scenario.story.stage_opponents[stage];
	}
	if(show_unreached) {
		return replay_user_menu_snapshot.story_opponents[stage];
	}
	return 0xFF;
}

static char *append_cstr(char *p, const char *str)
{
	while(*str) {
		*p++ = *str++;
	}
	return p;
}

static char *append_u8_2(char *p, uint8_t value)
{
	*p++ = static_cast<char>('0' + (value / 10));
	*p++ = static_cast<char>('0' + (value % 10));
	return p;
}

static char *append_u32(char *p, uint32_t value)
{
	char digits[10];
	int i = 0;

	do {
		digits[i++] = static_cast<char>('0' + (value % 10));
		value /= 10;
	} while(value != 0);
	while(i != 0) {
		*p++ = digits[--i];
	}
	return p;
}

static char *append_q4(char *p, uint8_t value)
{
	uint16_t fraction = static_cast<uint16_t>((value & 0x0F) * 625);

	p = append_u32(p, (value >> 4));
	*p++ = '.';
	*p++ = static_cast<char>('0' + ((fraction / 1000) % 10));
	*p++ = static_cast<char>('0' + ((fraction / 100) % 10));
	*p++ = static_cast<char>('0' + ((fraction / 10) % 10));
	*p++ = static_cast<char>('0' + (fraction % 10));
	return p;
}

static char *append_name(char *p)
{
	char c;

	for(int i = 0; i < T3_REPLAY_USER_NAME_LEN; i++) {
		c = replay_user_menu_header.name[i];
		*p++ = (((c == '\0') || (c == ' ')) ? ' ' : c);
	}
	return p;
}

static char *append_name_trimmed(char *p)
{
	char *first = p;

	p = append_name(p);
	while((p > first) && (p[-1] == ' ')) {
		p--;
	}
	return p;
}

static char *append_date(char *p)
{
	uint16_t dos_date = replay_user_menu_header.dos_date;
	uint16_t year = static_cast<uint16_t>(1980 + (dos_date >> 9));
	uint8_t month = static_cast<uint8_t>((dos_date >> 5) & 0x0F);
	uint8_t day = static_cast<uint8_t>(dos_date & 0x1F);

	if(dos_date == 0) {
		*p++ = '-';
		return p;
	}
	p = append_u8_2(p, month);
	*p++ = '-';
	p = append_u8_2(p, day);
	*p++ = '-';
	return append_u32(p, year);
}

static char *append_unknown_score(char *p)
{
	for(int i = 0; i < T3_REPLAY_USER_SCORE_DISPLAY_DIGITS; i++) {
		*p++ = '-';
	}
	return p;
}

static char *append_packed_score(char *p, const uint8_t far *score)
{
	uint8_t value;
	bool seen = false;
	int digit;

	for(digit = (T3_REPLAY_USER_SCORE_DIGITS - 1); digit >= 0; digit--) {
		value = score[digit / 2];
		if(digit & 1) {
			value >>= 4;
		}
		value &= 0x0F;
		if(value < 10) {
			if((value != 0) || seen) {
				seen = true;
				*p++ = static_cast<char>('0' + value);
			} else {
				*p++ = ' ';
			}
		} else {
			seen = true;
			*p++ = '?';
		}
	}
	*p++ = '0';
	return p;
}

static uint8_t playchar_id(uint8_t paletted)
{
	return ((paletted == 0) ? 0xFF : ((paletted - 1) / 2));
}

static const char *playchar_name(uint8_t paletted)
{
	switch(playchar_id(paletted)) {
	case PLAYCHAR_REIMU: return "Reimu";
	case PLAYCHAR_MIMA: return "Mima";
	case PLAYCHAR_MARISA: return "Marisa";
	case PLAYCHAR_ELLEN: return "Ellen";
	case PLAYCHAR_KOTOHIME: return "Kotohime";
	case PLAYCHAR_KANA: return "Kana";
	case PLAYCHAR_RIKAKO: return "Rikako";
	case PLAYCHAR_CHIYURI: return "Chiyuri";
	case PLAYCHAR_YUMEMI: return "Yumemi";
	default: return "-";
	}
}

static char *append_playchar_name(char *p, uint8_t paletted)
{
	return append_cstr(p, playchar_name(paletted));
}

static const char *rank_name(uint8_t rank)
{
	switch(rank) {
	case RANK_EASY: return "Easy";
	case RANK_NORMAL: return "Normal";
	case RANK_HARD: return "Hard";
	case RANK_LUNATIC: return "Lunatic";
	default: return "?";
	}
}

static const char *end_reason_name(void)
{
	if(replay_vs()) {
		return "Vs Mode";
	}
	switch(replay_user_menu_header.end_reason) {
	case RUER_COMPLETE: return "Clear";
	case RUER_GAME_OVER: return "Game Over";
	case RUER_MENU_RETURN: return "Menu Return";
	case RUER_INPUT_END: return "Input End";
	case RUER_PARTIAL: return "Partial";
	case RUER_ERROR: return "Error";
	default: return "None";
	}
}

static char *append_final_stage(char *p)
{
	uint8_t stage;

	if(replay_practice()) {
		*p++ = 'P';
		*p++ = static_cast<char>(
			'1' + replay_user_menu_header.scenario.practice.config.stage
		);
		*p++ = static_cast<char>(
			'1' + replay_user_menu_header.scenario.practice.config.round
		);
		return p;
	}
	if(replay_vs()) {
		return append_cstr(p, "VS");
	}
	if(replay_user_menu_header.end_reason == RUER_COMPLETE) {
		return append_cstr(p, "ALL");
	}
	if(
		summary_valid() &&
		(replay_user_menu_header.stage_reached_count != 0)
	) {
		stage = replay_user_menu_header.stage_reached_count;
		if(stage > T3_REPLAY_USER_STAGE_COUNT) {
			stage = T3_REPLAY_USER_STAGE_COUNT;
		}
		*p++ = static_cast<char>('0' + stage);
	} else {
		*p++ = '-';
	}
	return p;
}

static char *append_round_winner(char *p, uint8_t route_winner)
{
	switch(route_winner & 0x0F) {
	case 0: *p++ = '1'; break;
	case 1: *p++ = '2'; break;
	default: *p++ = '-'; break;
	}
	return p;
}

static int font_color(unsigned atrb)
{
	if(atrb == TX_BLACK) return 0;
	if(atrb == TX_CYAN) return 9;
	if(atrb == TX_YELLOW) return 12;
	return V_WHITE;
}

static int fixed_cell_width(unsigned atrb)
{
	if(atrb & REPLAY_FONT_FIXED_NAME) {
		return REPLAY_FONT_NAME_CELL_W;
	}
	return REPLAY_FONT_NUMERIC_CELL_W;
}

enum {
	SLOT_LIST_Y = 6,
	SLOT_PIXEL_TOP = 112,
	SLOT_PIXEL_H = 24,
	PIXEL_TOP_FLAG = 0x8000,
};

static unsigned font_top(unsigned y);
static unsigned slot_y(unsigned y);

static void field_put(screen_x_t left, unsigned y, char *p, unsigned atrb)
{
	unsigned count = (p - replay_menu_line);
	unsigned top = font_top(y);

	*p = '\0';
	if(atrb & REPLAY_FONT_FIXED_MASK) {
		replay_font_put_fixed_n(
			left, top, replay_menu_line, count,
			fixed_cell_width(atrb),
			font_color(atrb & 0xFF)
		);
	} else {
		menu_font_put(left, top, replay_menu_line, font_color(atrb));
	}
}

static void field_put_right(
	screen_x_t right, unsigned y, char *p, unsigned atrb
)
{
	char *first = replay_menu_line;
	unsigned top = font_top(y);

	*p = '\0';
	if(atrb & REPLAY_FONT_FIXED_MASK) {
		unsigned count = (p - replay_menu_line);
		int cell_w = fixed_cell_width(atrb);
		replay_font_put_fixed_n(
			(right - (count * cell_w)), top,
			replay_menu_line, count, cell_w, font_color(atrb & 0xFF)
		);
		return;
	}
	while((*first == ' ') && (first[1] != '\0')) {
		first++;
	}
	menu_font_put_right(right, top, first, font_color(atrb));
}

static void text_put(
	screen_x_t left, unsigned y, const char far *str, unsigned atrb
)
{
	menu_font_put(left, font_top(y), str, font_color(atrb));
}

static void stage_field_put(unsigned y, unsigned atrb)
{
	char *p = append_final_stage(replay_menu_line);

	field_put(
		STAGE_PIXEL_LEFT, y, p, (atrb | REPLAY_FONT_FIXED_NUMERIC)
	);
}

static void score_put(
	screen_x_t right,
	unsigned y,
	const uint8_t far *score,
	bool valid,
	unsigned atrb
)
{
	char *p = replay_menu_line;

	p = (valid ? append_packed_score(p, score) : append_unknown_score(p));
	field_put_right(right, y, p, atrb);
}

// Preserve the public slot renderer after replacing its numeric formatter.
#pragma codestring "\x90"

void far replay_font_slot_line_put(
	uint8_t slot, uint8_t sel, unsigned y, bool active, bool has_replay
)
{
	char *p;
	bool selected = ((slot == sel) && active);
	unsigned atrb = (selected ? TX_YELLOW : TX_WHITE);

	y = slot_y(y);

	if(selected) {
		menu_font_put(
			CURSOR_PIXEL_LEFT, font_top(y),
			">", REPLAY_FONT_SELECTED_COLOR
		);
	}
	p = append_u32(replay_menu_line, slot);
	field_put_right(
		(SLOT_PIXEL_LEFT + (2 * REPLAY_FONT_NUMERIC_CELL_W)),
		y, p, (atrb | REPLAY_FONT_FIXED_NUMERIC)
	);
	if(!has_replay) {
		p = append_cstr(replay_menu_line, "none");
		field_put(NAME_PIXEL_LEFT, y, p, atrb);
		return;
	}
	p = append_name(replay_menu_line);
	field_put(NAME_PIXEL_LEFT, y, p, (atrb | REPLAY_FONT_FIXED_NAME));
	score_put(
		SCORE_PIXEL_RIGHT, y, list_score(), summary_valid(),
		(atrb | REPLAY_FONT_FIXED_NUMERIC)
	);
	p = append_playchar_name(
		replay_menu_line, replay_user_menu_header.playchar_p1
	);
	field_put(CHAR_PIXEL_LEFT, y, p, atrb);
	text_put(
		RANK_PIXEL_LEFT, y, rank_name(replay_user_menu_header.rank), atrb
	);
	stage_field_put(y, atrb);
}

// Preserve the following public column renderer across slot-layout revisions.
#pragma codestring "\x90\x90\x90\x90"

void far replay_font_columns_put(bool clear)
{
	char *p;

	if(menu_font) {
		if(clear) {
			replay_menu_span_clear(LIST_LEFT, HEAD_Y, LIST_W);
		}
		text_put(SLOT_PIXEL_LEFT, HEAD_Y, "Slot", TX_CYAN);
		text_put(NAME_PIXEL_LEFT, HEAD_Y, "Name", TX_CYAN);
		menu_font_put_right(
			SCORE_PIXEL_RIGHT, (HEAD_Y * GLYPH_H),
			"Score", font_color(TX_CYAN)
		);
		text_put(CHAR_PIXEL_LEFT, HEAD_Y, "Character", TX_CYAN);
		text_put(RANK_PIXEL_LEFT, HEAD_Y, "Rank", TX_CYAN);
		text_put(STAGE_PIXEL_LEFT, HEAD_Y, "Stg", TX_CYAN);
		return;
	}
	p = replay_menu_line;
	*p++ = ' '; *p++ = 'S'; *p++ = 'l'; *p++ = ' ';
	*p++ = 'C'; *p++ = 'h'; *p++ = ' '; *p++ = 'R'; *p++ = ' ';
	*p++ = 'N'; *p++ = 'a'; *p++ = 'm'; *p++ = 'e';
	*p++ = ' '; *p++ = ' '; *p++ = ' '; *p++ = ' '; *p++ = ' ';
	*p++ = 'S'; *p++ = 'c'; *p++ = 'o'; *p++ = 'r'; *p++ = 'e';
	*p++ = ' '; *p++ = ' '; *p++ = ' '; *p++ = ' '; *p++ = ' ';
	*p++ = 'S'; *p++ = 't'; *p++ = 'g'; *p = '\0';
	replay_menu_line_put(LIST_LEFT, HEAD_Y, TX_CYAN);
}

static void slot_name_put(uint8_t slot, unsigned y)
{
	char *p = append_name_trimmed(replay_menu_line);

	field_put(DETAIL_PIXEL_LEFT, y, p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Slot ");
	p = append_u32(p, slot);
	field_put_right(DETAIL_PIXEL_RIGHT, y, p, TX_CYAN);
}

static void score_line_put(const char *label, unsigned y)
{
	char *p = append_cstr(replay_menu_line, label);

	field_put(DETAIL_PIXEL_LEFT, y, p, TX_WHITE);
	score_put(
		DETAIL_PIXEL_RIGHT, y, list_score(), summary_valid(),
		(TX_WHITE | REPLAY_FONT_FIXED_NUMERIC)
	);
}

static void date_line_put(unsigned y)
{
	char *p = append_cstr(replay_menu_line, "Date");

	field_put(DETAIL_PIXEL_LEFT, y, p, TX_WHITE);
	p = append_date(replay_menu_line);
	field_put_right(DETAIL_PIXEL_RIGHT, y, p, TX_WHITE);
}

static void slowdown_line_put(unsigned y)
{
	char *p = append_cstr(replay_menu_line, "Slowdown");
	uint32_t percent;

	field_put(DETAIL_PIXEL_LEFT, y, p, TX_WHITE);
	if(replay_user_menu_timed_frames == 0) {
		p = append_cstr(replay_menu_line, "-");
	} else {
		percent = (
			(replay_user_menu_slow_frames * 100UL) /
			replay_user_menu_timed_frames
		);
		if(percent > 100) {
			percent = 100;
		}
		p = append_u32(replay_menu_line, percent);
		*p++ = '%';
	}
	field_put_right(DETAIL_PIXEL_RIGHT, y, p, TX_WHITE);
}

static char *append_timer(char *p, uint32_t frames)
{
	uint32_t minutes = (frames / 3384UL);
	uint16_t remainder = static_cast<uint16_t>(frames % 3384UL);
	uint16_t centiseconds = static_cast<uint16_t>(
		(static_cast<uint32_t>(remainder) * 1000UL) / 564UL
	);

	p = append_u32(p, minutes);
	*p++ = ':';
	p = append_u8_2(p, static_cast<uint8_t>(centiseconds / 100));
	*p++ = '.';
	return append_u8_2(p, static_cast<uint8_t>(centiseconds % 100));
}

static screen_x_t detail_tab_put(
	screen_x_t left, const char far *label, bool selected
)
{
	menu_font_put(
		left, (DETAIL_Y * GLYPH_H), label,
		font_color(selected ? TX_YELLOW : TX_CYAN)
	);
	return static_cast<screen_x_t>(left + menu_font_width(label));
}

static void detail_tabs_put(uint8_t detail_page)
{
	screen_x_t left = DETAIL_ROUND_PIXEL_LEFT;

	left = detail_tab_put(left, "Splits", (detail_page == RDP_SPLITS));
	left = detail_tab_put(left, " / ", false);
	left = detail_tab_put(
		left, "Clears", (detail_page == RDP_CLEAR_BONUSES)
	);
	left = detail_tab_put(left, " / ", false);
	detail_tab_put(left, "Timers", (detail_page == RDP_TIMERS));
}

// Preserve all later replay-browser entry points after shortening the tabs.
#pragma codestring "\x90\x90"

static bool clear_bonus_valid(uint8_t stage)
{
	replay_user_stage_clear_bonus_t far *bonus = (
		&replay_user_menu_stage_clear_bonuses[stage]
	);

	for(uint8_t i = 0; i < T3_REPLAY_USER_PACKED_SCORE_SIZE; i++) {
		if(bonus->total[i] != 0) {
			return true;
		}
	}
	return (
		(bonus->max_combo != 0) ||
		(bonus->gauge_attacks != 0) ||
		(bonus->boss_attacks != 0) ||
		(bonus->boss_reversals != 0) ||
		(bonus->boss_panics != 0) ||
		(bonus->remaining_lives != 0)
	);
}

static void clear_bonus_score_put(uint8_t stage, unsigned y)
{
	bool valid = clear_bonus_valid(stage);

	score_put(
		DETAIL_STORY_SCORE_RIGHT, y,
		replay_user_menu_stage_clear_bonuses[stage].total, valid,
		(TX_WHITE | REPLAY_FONT_FIXED_NUMERIC)
	);
}

static void round_put(
	unsigned y,
	uint8_t round,
	replay_user_menu_round_split_t near *split,
	bool valid,
	bool selected
)
{
	unsigned atrb = (selected ? TX_YELLOW : TX_WHITE);
	char *p;

	if(selected) {
		text_put(DETAIL_ROUND_CURSOR_LEFT, y, ">", TX_YELLOW);
	}
	p = append_u32(replay_menu_line, (round + 1));
	field_put(DETAIL_ROUND_PIXEL_LEFT, y, p, atrb);
	score_put(
		DETAIL_ROUND_P1_SCORE_RIGHT, y,
		(valid ? split->score_p1 : NULL), valid,
		(atrb | REPLAY_FONT_FIXED_NUMERIC)
	);
	score_put(
		DETAIL_ROUND_P2_SCORE_RIGHT, y,
		(valid ? split->score_p2 : NULL), valid,
		(atrb | REPLAY_FONT_FIXED_NUMERIC)
	);
	p = replay_menu_line;
	p = (valid ? append_round_winner(p, split->route_winner) : append_cstr(p, "-"));
	field_put(DETAIL_ROUND_WINNER_LEFT, y, p, atrb);
}

static void round_heading_put(unsigned y)
{
	text_put(DETAIL_ROUND_PIXEL_LEFT, y, "Rd", TX_CYAN);
	menu_font_put_right(
		DETAIL_ROUND_P1_SCORE_RIGHT, (y * GLYPH_H),
		"P1 Score", font_color(TX_CYAN)
	);
	menu_font_put_right(
		DETAIL_ROUND_P2_SCORE_RIGHT, (y * GLYPH_H),
		"P2 Score", font_color(TX_CYAN)
	);
	text_put(DETAIL_ROUND_WINNER_LEFT, y, "W", TX_CYAN);
}

static uint8_t checkpoint_count(void)
{
	uint8_t count;

	if(replay_user_version_has_round_state(replay_user_menu_header.version)) {
		return replay_user_menu_summary_ext.checkpoint_count;
	}
	if(replay_user_menu_header.game_mode == GM_STORY) {
		count = replay_user_menu_header.stage_reached_count;
		return (
			(count > T3_REPLAY_USER_STAGE_COUNT) ?
				T3_REPLAY_USER_STAGE_COUNT : count
		);
	}
	return 1;
}

static uint8_t checkpoint_stage_round(uint8_t checkpoint)
{
	if(replay_user_version_has_round_state(replay_user_menu_header.version)) {
		return replay_user_menu_summary_ext.checkpoint_stage_round[checkpoint];
	}
	if(replay_user_menu_header.game_mode == GM_STORY) {
		return checkpoint;
	}
	return T3_REPLAY_USER_ROUND_STAGE_VS;
}

static void round_splits_legacy_put(bool focus);
static void round_splits_put(uint8_t selected, bool focus);
static void round_timers_put(uint8_t selected, bool focus);
static void clear_bonuses_put(bool show_unreached_opponents);

static void vs_put(uint8_t selected, bool focus, uint8_t detail_page)
{
	char *p;

	p = append_cstr(replay_menu_line, "Vs Mode ");
	if(replay_user_menu_header.game_mode == GM_VS_1P_CPU) {
		p = append_cstr(p, "1P-CPU");
	} else if(replay_user_menu_header.game_mode == GM_VS_1P_2P) {
		p = append_cstr(p, "1P-2P");
	} else {
		p = append_cstr(p, "CPU-CPU");
	}
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 1), p, TX_CYAN);
	score_line_put(
		((replay_user_menu_header.game_mode == GM_VS_1P_CPU) ?
			"P1 Score" : "Win Score"),
		(DETAIL_Y + 2)
	);
	date_line_put(DETAIL_Y + 3);
	p = append_cstr(replay_menu_line, rank_name(replay_user_menu_header.rank));
	*p++ = ' ';
	p = append_playchar_name(p, replay_user_menu_header.playchar_p1);
	p = append_cstr(p, " vs ");
	p = append_playchar_name(p, replay_user_menu_header.playchar_p2);
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 4), p, TX_WHITE);
	p = append_cstr(
		replay_menu_line,
		((replay_user_menu_header.autofire & 0x01) ?
			"AutofireOn" : "AutofireOff")
	);
	*p++ = ' ';
	p = append_cstr(
		p,
		((replay_user_menu_header.autofire & 0x02) ?
			"AutofireOn" : "AutofireOff")
	);
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 5), p, TX_WHITE);
	slowdown_line_put(DETAIL_Y + 6);
	detail_tabs_put(detail_page);
	if(detail_page == RDP_CLEAR_BONUSES) {
		clear_bonuses_put(false);
	} else if(detail_page == RDP_TIMERS) {
		round_timers_put(selected, focus);
	} else {
		round_splits_put(selected, focus);
	}
}

static uint8_t story_stage_checkpoint_count(uint8_t stage)
{
	uint8_t checkpoint;
	uint8_t count = 0;
	uint8_t checkpoints = checkpoint_count();

	for(checkpoint = 0; checkpoint < checkpoints; checkpoint++) {
		if((checkpoint_stage_round(checkpoint) & 0x0F) == stage) {
			count++;
		}
	}
	return count;
}

static uint8_t story_selected_row(uint8_t selected)
{
	uint8_t checkpoint;
	uint8_t checkpoints = checkpoint_count();
	uint8_t stage;
	uint8_t stage_count;
	uint8_t row = 0;

	for(stage = 0; stage < T3_REPLAY_USER_STAGE_COUNT; stage++) {
		stage_count = story_stage_checkpoint_count(stage);
		if(stage_count == 0) {
			row++;
			continue;
		}
		if(stage_count > 1) {
			row++;
		}
		for(checkpoint = 0; checkpoint < checkpoints; checkpoint++) {
			if((checkpoint_stage_round(checkpoint) & 0x0F) == stage) {
				if(checkpoint == selected) {
					return row;
				}
				row++;
			}
		}
	}
	return 0;
}

static void story_stage_row_put(
	uint8_t row,
	uint8_t stage,
	uint8_t checkpoint,
	uint8_t stage_count,
	bool selected,
	bool show_unreached_opponents
)
{
	char *p;
	unsigned atrb = (selected ? TX_YELLOW : TX_WHITE);
	bool reached = (stage_count != 0);

	if(selected) {
		text_put(DETAIL_STORY_CURSOR_LEFT, row, ">", TX_YELLOW);
	}
	p = append_u32(replay_menu_line, (stage + 1));
	field_put(
		DETAIL_STORY_PIXEL_LEFT, row, p,
		(atrb | REPLAY_FONT_FIXED_NUMERIC)
	);
	p = append_playchar_name(
		replay_menu_line, stage_opponent(stage, show_unreached_opponents)
	);
	field_put(DETAIL_STORY_OPPONENT_LEFT, row, p, atrb);
	if(stage_count > 1) {
		score_put(
			DETAIL_STORY_SCORE_RIGHT, row, NULL, false,
			(atrb | REPLAY_FONT_FIXED_NUMERIC)
		);
	} else {
		score_put(
			DETAIL_STORY_SCORE_RIGHT, row,
			replay_user_menu_header.scenario.story.stage_scores[stage],
			(summary_valid() && reached),
			(atrb | REPLAY_FONT_FIXED_NUMERIC)
		);
	}
	(void)checkpoint;
}

static void story_round_row_put(
	uint8_t row, uint8_t checkpoint, bool selected
)
{
	char *p;
	uint8_t stage_round = checkpoint_stage_round(checkpoint);
	replay_user_menu_round_split_t near *split = round_split_at(
		(stage_round & 0x0F), (stage_round >> 4)
	);
	unsigned atrb = (selected ? TX_YELLOW : TX_WHITE);

	if(selected) {
		text_put(DETAIL_STORY_CURSOR_LEFT, row, ">", TX_YELLOW);
	}
	p = append_cstr(replay_menu_line, "Round ");
	p = append_u32(p, ((stage_round >> 4) + 1));
	field_put(DETAIL_STORY_ROUND_LEFT, row, p, atrb);
	score_put(
		DETAIL_STORY_SCORE_RIGHT, row,
		((split != NULL) ? split->score_p1 : NULL), (split != NULL),
		(atrb | REPLAY_FONT_FIXED_NUMERIC)
	);
}

static bool round_timer_frames(
	uint8_t stage, uint8_t round, uint32_t& frames
)
{
	int index = round_split_index_at(stage, round);

	if(index < 0) {
		frames = 0;
		return false;
	}
	frames = replay_user_menu_round_real_frames[index];
	return true;
}

static void timer_put(
	screen_x_t right, unsigned y, uint32_t frames, bool valid, unsigned atrb
)
{
	char *p;
	unsigned count;
	int width;

	if(!valid) {
		p = append_cstr(replay_menu_line, "-");
		field_put_right(right, y, p, atrb);
		return;
	}
	p = append_timer(replay_menu_line, frames);
	*p = '\0';
	count = (p - replay_menu_line);
	width = (
		((count - 2) * REPLAY_FONT_NUMERIC_CELL_W) +
		menu_font_width(":.")
	);
	replay_font_put_fixed_n(
		(right - width), (y * GLYPH_H), replay_menu_line, count, 0,
		font_color(atrb)
	);
}

static uint32_t story_stage_timer(uint8_t stage, bool& valid)
{
	uint32_t total = 0;
	uint32_t frames;
	uint8_t checkpoint;
	uint8_t stage_round;

	valid = false;
	for(checkpoint = 0; checkpoint < checkpoint_count(); checkpoint++) {
		stage_round = checkpoint_stage_round(checkpoint);
		if((stage_round & 0x0F) != stage) {
			continue;
		}
		if(round_timer_frames(stage, (stage_round >> 4), frames)) {
			total += frames;
			valid = true;
		}
	}
	return total;
}

static void story_timer_stage_row_put(
	uint8_t row, uint8_t stage, bool selected,
	bool show_unreached_opponents
)
{
	char *p;
	bool valid;
	uint32_t frames = story_stage_timer(stage, valid);
	unsigned atrb = (selected ? TX_YELLOW : TX_WHITE);

	if(selected) {
		text_put(DETAIL_STORY_CURSOR_LEFT, row, ">", TX_YELLOW);
	}

	p = append_u32(replay_menu_line, (stage + 1));
	field_put(
		DETAIL_STORY_PIXEL_LEFT, row, p,
		(atrb | REPLAY_FONT_FIXED_NUMERIC)
	);
	p = append_playchar_name(
		replay_menu_line, stage_opponent(stage, show_unreached_opponents)
	);
	field_put(DETAIL_STORY_OPPONENT_LEFT, row, p, atrb);
	timer_put(DETAIL_STORY_SCORE_RIGHT, row, frames, valid, atrb);
}

static void story_timer_round_row_put(
	uint8_t row, uint8_t checkpoint, bool selected
)
{
	char *p;
	uint8_t stage_round = checkpoint_stage_round(checkpoint);
	uint32_t frames;
	bool valid = round_timer_frames(
		(stage_round & 0x0F), (stage_round >> 4), frames
	);
	unsigned atrb = (selected ? TX_YELLOW : TX_WHITE);

	if(selected) {
		text_put(DETAIL_STORY_CURSOR_LEFT, row, ">", TX_YELLOW);
	}
	p = append_cstr(replay_menu_line, "Round ");
	p = append_u32(p, ((stage_round >> 4) + 1));
	field_put(DETAIL_STORY_ROUND_LEFT, row, p, atrb);
	timer_put(DETAIL_STORY_SCORE_RIGHT, row, frames, valid, atrb);
}

static void story_timers_put(
	uint8_t selected, bool focus, bool show_unreached_opponents
)
{
	uint8_t checkpoint;
	uint8_t checkpoints = checkpoint_count();
	uint8_t stage;
	uint8_t stage_count;
	uint8_t physical = 0;
	uint8_t selected_row = story_selected_row(selected);
	uint8_t top = (
		(selected_row >= DETAIL_SPLIT_ROWS_VISIBLE) ?
			(selected_row - (DETAIL_SPLIT_ROWS_VISIBLE - 1)) : 0
	);
	uint8_t row;

	text_put(DETAIL_STORY_PIXEL_LEFT, (DETAIL_Y + 2), "St", TX_CYAN);
	text_put(
		DETAIL_STORY_OPPONENT_LEFT, (DETAIL_Y + 2), "Opponent", TX_CYAN
	);
	menu_font_put_right(
		DETAIL_STORY_SCORE_RIGHT, ((DETAIL_Y + 2) * GLYPH_H),
		"Time", font_color(TX_CYAN)
	);
	for(stage = 0; stage < T3_REPLAY_USER_STAGE_COUNT; stage++) {
		stage_count = story_stage_checkpoint_count(stage);
		if((stage_count == 0) || (stage_count > 1)) {
			if((physical >= top) &&
			   (physical < (top + DETAIL_SPLIT_ROWS_VISIBLE))) {
				row = (DETAIL_SPLIT_ROWS_Y + physical - top);
				story_timer_stage_row_put(
					row, stage, false, show_unreached_opponents
				);
			}
			physical++;
		}
		for(checkpoint = 0; checkpoint < checkpoints; checkpoint++) {
			if((checkpoint_stage_round(checkpoint) & 0x0F) != stage) {
				continue;
			}
			if((physical >= top) &&
			   (physical < (top + DETAIL_SPLIT_ROWS_VISIBLE))) {
				row = (DETAIL_SPLIT_ROWS_Y + physical - top);
				if(stage_count == 1) {
					story_timer_stage_row_put(
						row, stage,
						(focus && (checkpoint == selected)),
						show_unreached_opponents
					);
				} else {
					story_timer_round_row_put(
						row, checkpoint,
						(focus && (checkpoint == selected))
					);
				}
			}
			physical++;
		}
	}
}

static void clear_bonus_stage_row_put(
	uint8_t row, uint8_t stage, bool show_unreached_opponents
)
{
	char *p = append_u32(replay_menu_line, (stage + 1));

	field_put(
		DETAIL_STORY_PIXEL_LEFT, row, p,
		(TX_WHITE | REPLAY_FONT_FIXED_NUMERIC)
	);
	p = append_playchar_name(
		replay_menu_line, stage_opponent(stage, show_unreached_opponents)
	);
	field_put(DETAIL_STORY_OPPONENT_LEFT, row, p, TX_WHITE);
	clear_bonus_score_put(stage, row);
}

static void clear_bonuses_put(bool show_unreached_opponents)
{
	uint8_t stage;

	text_put(DETAIL_STORY_PIXEL_LEFT, (DETAIL_Y + 2), "St", TX_CYAN);
	text_put(
		DETAIL_STORY_OPPONENT_LEFT, (DETAIL_Y + 2), "Opponent", TX_CYAN
	);
	menu_font_put_right(
		DETAIL_STORY_SCORE_RIGHT, ((DETAIL_Y + 2) * GLYPH_H),
		"Bonus", font_color(TX_CYAN)
	);
	if(replay_practice()) {
		stage = replay_user_menu_header.scenario.practice.config.stage;
		clear_bonus_stage_row_put(
			DETAIL_SPLIT_ROWS_Y, stage, show_unreached_opponents
		);
		return;
	}
	if(replay_vs()) {
		text_put(
			DETAIL_STORY_PIXEL_LEFT, DETAIL_SPLIT_ROWS_Y,
			"No stage clear bonuses.", TX_WHITE
		);
		return;
	}
	for(stage = 0; stage < T3_REPLAY_USER_STAGE_COUNT; stage++) {
		clear_bonus_stage_row_put(
			(DETAIL_SPLIT_ROWS_Y + stage), stage, show_unreached_opponents
		);
	}
}

static void story_splits_put(
	uint8_t selected, bool focus, bool show_unreached_opponents
)
{
	uint8_t checkpoint;
	uint8_t checkpoints = checkpoint_count();
	uint8_t stage;
	uint8_t stage_count;
	uint8_t physical = 0;
	uint8_t selected_row = story_selected_row(selected);
	uint8_t top = (
		(selected_row >= DETAIL_SPLIT_ROWS_VISIBLE) ?
			(selected_row - (DETAIL_SPLIT_ROWS_VISIBLE - 1)) : 0
	);
	uint8_t row;

	text_put(DETAIL_STORY_PIXEL_LEFT, (DETAIL_Y + 2), "St", TX_CYAN);
	text_put(
		DETAIL_STORY_OPPONENT_LEFT, (DETAIL_Y + 2), "Opponent", TX_CYAN
	);
	menu_font_put_right(
		DETAIL_STORY_SCORE_RIGHT, ((DETAIL_Y + 2) * GLYPH_H),
		"Score", font_color(TX_CYAN)
	);
	for(stage = 0; stage < T3_REPLAY_USER_STAGE_COUNT; stage++) {
		stage_count = story_stage_checkpoint_count(stage);
		if(stage_count == 0) {
			if((physical >= top) &&
			   (physical < (top + DETAIL_SPLIT_ROWS_VISIBLE))) {
				row = (DETAIL_SPLIT_ROWS_Y + physical - top);
				story_stage_row_put(
					row, stage, 0, 0, false, show_unreached_opponents
				);
			}
			physical++;
			continue;
		}
		if(stage_count > 1) {
			if((physical >= top) &&
			   (physical < (top + DETAIL_SPLIT_ROWS_VISIBLE))) {
				row = (DETAIL_SPLIT_ROWS_Y + physical - top);
				story_stage_row_put(
					row, stage, 0, stage_count, false,
					show_unreached_opponents
				);
			}
			physical++;
		}
		for(checkpoint = 0; checkpoint < checkpoints; checkpoint++) {
			if((checkpoint_stage_round(checkpoint) & 0x0F) != stage) {
				continue;
			}
			if((physical >= top) &&
			   (physical < (top + DETAIL_SPLIT_ROWS_VISIBLE))) {
				row = (DETAIL_SPLIT_ROWS_Y + physical - top);
				if(stage_count == 1) {
					story_stage_row_put(
						row, stage, checkpoint, stage_count,
						(focus && (checkpoint == selected)),
						show_unreached_opponents
					);
				} else {
					story_round_row_put(
						row, checkpoint,
						(focus && (checkpoint == selected))
					);
				}
			}
			physical++;
		}
	}
}

static void story_put(
	uint8_t selected, bool focus, uint8_t detail_page,
	bool show_unreached_opponents
)
{
	char *p = append_cstr(replay_menu_line, "Story Mode - ");

	p = append_cstr(p, end_reason_name());
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 1), p, TX_CYAN);
	score_line_put("Final Score", (DETAIL_Y + 2));
	date_line_put(DETAIL_Y + 3);
	p = append_cstr(replay_menu_line, rank_name(replay_user_menu_header.rank));
	*p++ = ' ';
	p = append_playchar_name(p, replay_user_menu_header.playchar_p1);
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 4), p, TX_WHITE);
	p = append_cstr(
		replay_menu_line,
		((replay_user_menu_header.autofire & 0x01) ?
			"AutofireOn" : "AutofireOff")
	);
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 5), p, TX_WHITE);
	slowdown_line_put(DETAIL_Y + 6);
	detail_tabs_put(detail_page);
	if(detail_page == RDP_CLEAR_BONUSES) {
		clear_bonuses_put(show_unreached_opponents);
	} else if(detail_page == RDP_TIMERS) {
		story_timers_put(selected, focus, show_unreached_opponents);
	} else {
		story_splits_put(selected, focus, show_unreached_opponents);
	}
}

static const char *practice_timer_name(void)
{
	uint8_t timer = replay_user_menu_header.scenario.practice.config.cpu_timer;

	if(timer == PRACTICE_CPU_TIMER_VS_DEFAULT) return "VS Default";
	if(timer == PRACTICE_CPU_TIMER_STORY_NATIVE) return "Story Native";
	return "Infinite";
}

static void practice_put(
	uint8_t selected, bool focus, uint8_t detail_page
)
{
	char *p = append_cstr(replay_menu_line, "Practice Mode");

	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 1), p, TX_CYAN);
	score_line_put("Final Score", (DETAIL_Y + 2));
	date_line_put(DETAIL_Y + 3);
	p = append_cstr(replay_menu_line, rank_name(replay_user_menu_header.rank));
	*p++ = ' ';
	p = append_playchar_name(p, replay_user_menu_header.playchar_p1);
	p = append_cstr(p, " vs ");
	p = append_playchar_name(p, replay_user_menu_header.playchar_p2);
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 4), p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Stage ");
	p = append_u32(
		p, (replay_user_menu_header.scenario.practice.config.stage + 1)
	);
	p = append_cstr(p, " Round ");
	p = append_u32(
		p, (replay_user_menu_header.scenario.practice.config.round + 1)
	);
	if(replay_user_menu_header.scenario.practice.config.round == 5) *p++ = '+';
	p = append_cstr(p, " Start");
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 5), p, TX_WHITE);
	slowdown_line_put(DETAIL_Y + 6);
	if(!focus) {
		text_put(
			(DETAIL_PIXEL_LEFT - 16), DETAIL_PRACTICE_SETTINGS_Y,
			">", TX_YELLOW
		);
	}
	text_put(
		DETAIL_PIXEL_LEFT, DETAIL_PRACTICE_SETTINGS_Y,
		"View Settings", (focus ? TX_WHITE : TX_YELLOW)
	);
	detail_tabs_put(detail_page);
	if(detail_page == RDP_CLEAR_BONUSES) {
		clear_bonuses_put(false);
	} else if(detail_page == RDP_TIMERS) {
		round_timers_put(selected, focus);
	} else {
		round_splits_put(selected, focus);
	}
}

// Preserve the following public replay-font entry points across menu revisions.
#pragma codestring "\x90"

void far replay_font_detail_put(
	uint8_t slot,
	uint8_t checkpoint_sel,
	bool checkpoint_focus,
	uint8_t detail_page,
	bool show_unreached_opponents
)
{
	slot_name_put(slot, DETAIL_Y);
	if(replay_practice()) {
		practice_put(checkpoint_sel, checkpoint_focus, detail_page);
	} else if(replay_vs()) {
		vs_put(checkpoint_sel, checkpoint_focus, detail_page);
	} else {
		story_put(
			checkpoint_sel, checkpoint_focus, detail_page,
			show_unreached_opponents
		);
	}
}

void far replay_font_practice_settings_modal_put(void)
{
	char *p;
	const replay_user_practice_t near& cfg = (
		replay_user_menu_header.scenario.practice.config
	);
	unsigned y = 8;

	grcg_setcolor(GC_RMW, 0);
	grcg_boxfill(72, 104, 568, 296);
	grcg_off();

	p = append_cstr(replay_menu_line, "Life Stock ");
	if(cfg.stock == T3_PRACTICE_STOCK_VS_RULES) {
		p = append_cstr(p, "VS Rules");
	} else {
		p = append_u32(p, cfg.stock);
		p = append_cstr(p, " (");
		p = append_u32(p, cfg.extends_gained);
		p = append_cstr(p, " Extends Gained)");
	}
	field_put(96, y++, p, TX_WHITE);
	p = append_cstr(replay_menu_line, "CPU Timer: ");
	p = append_cstr(p, practice_timer_name());
	field_put(96, y++, p, TX_WHITE);
	p = append_cstr(replay_menu_line, "CPU Safety: ");
	p = append_u32(p, cfg.initial_cpu_safety_frames);
	field_put(96, y++, p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Speeds: ");
	p = append_q4(p, cfg.round_speed);
	p = append_cstr(p, " / ");
	p = append_q4(p, cfg.bullet_speed);
	field_put(96, y++, p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Spell Rank: P1 ");
	p = append_u32(p, cfg.p1_spell);
	p = append_cstr(p, " / CPU ");
	p = append_u32(p, cfg.cpu_spell);
	field_put(96, y++, p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Start Gauge: P1 ");
	p = append_u32(p, cfg.p1_gauge);
	p = append_cstr(p, " / CPU ");
	p = append_u32(p, cfg.cpu_gauge);
	field_put(96, y++, p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Boss Rank: ");
	p = append_u32(p, (cfg.boss_level + 1));
	field_put(96, y++, p, TX_WHITE);
	p = append_cstr(replay_menu_line, "CPU Damage: ");
	p = append_u32(p, cfg.cpu_damage);
	field_put(96, y, p, TX_WHITE);
}

void far replay_font_detail_empty_put(uint8_t slot)
{
	char *p = append_cstr(replay_menu_line, "Slot ");

	p = append_u32(p, slot);
	field_put(DETAIL_PIXEL_LEFT, DETAIL_Y, p, TX_CYAN);
	text_put(
		DETAIL_PIXEL_LEFT, (DETAIL_Y + 2), "No replay header found.", TX_WHITE
	);
}

void pascal far replay_font_put_fixed_n(
	int left,
	int top,
	const char far *str,
	unsigned count,
	int cell_w,
	int color
)
{
	int glyph_left;

	while(count && *str) {
		glyph_left = left;
		if(*str == '1') {
			glyph_left += REPLAY_FONT_ONE_INSET;
		}
		if(*str == 'I') {
			glyph_left += (
				(REPLAY_FONT_NAME_CELL_W - REPLAY_FONT_CAPITAL_I_ADVANCE) / 2
			);
		}
		menu_font_put_n(glyph_left, top, str, 1, color);
		if(cell_w != 0) {
			left += cell_w;
		} else if((*str >= '0') && (*str <= '9')) {
			left += REPLAY_FONT_NUMERIC_CELL_W;
		} else {
			left += menu_font_width_n(str, 1);
		}
		str++;
		count--;
	}
}

void far replay_font_diagnostic_line_put(
	unsigned y, uint8_t label, uint32_t value
)
{
	char *p = replay_menu_line;
	uint32_t near *pairs = reinterpret_cast<uint32_t near *>(p);

	switch(label) {
	case RFD_SAMPLES:
		pairs[0] = 0x706D6153UL; // "Samp"
		pairs[1] = 0x3A73656CUL; // "les:"
		p += 8;
		break;
	case RFD_FRAMES:
		pairs[0] = 0x6D617246UL; // "Fram"
		pairs[1] = 0x003A7365UL; // "es:"
		p += 7;
		break;
	case RFD_BYTES:
		pairs[0] = 0x65747942UL; // "Byte"
		pairs[1] = 0x00003A73UL; // "s:"
		p += 6;
		break;
	default:
		pairs[0] = 0x3A474E52UL; // "RNG:"
		p += 4;
		break;
	}
	*p++ = ' ';
	p = append_u32(p, value);
	field_put(DETAIL_PIXEL_LEFT, y, p, TX_WHITE);
}

static unsigned font_top(unsigned y)
{
	return (
		(y & PIXEL_TOP_FLAG) ? (y & ~PIXEL_TOP_FLAG) : (y * GLYPH_H)
	);
}

static unsigned slot_y(unsigned y)
{
	return static_cast<unsigned>(
		PIXEL_TOP_FLAG |
		(SLOT_PIXEL_TOP + ((y - SLOT_LIST_Y) * SLOT_PIXEL_H))
	);
}

static void round_splits_put(uint8_t selected, bool focus)
{
	uint8_t checkpoint;
	uint8_t count = checkpoint_count();
	uint8_t stage_round;
	replay_user_menu_round_split_t near *split;

	round_heading_put(DETAIL_Y + 2);
	if(!replay_user_version_has_round_state(replay_user_menu_header.version)) {
		round_splits_legacy_put(focus);
		return;
	}
	for(checkpoint = 0; checkpoint < count; checkpoint++) {
		stage_round = checkpoint_stage_round(checkpoint);
		split = round_split_at(
			(stage_round & 0x0F), (stage_round >> 4)
		);
		round_put(
			(DETAIL_SPLIT_ROWS_Y + checkpoint),
			(stage_round >> 4), split, (split != NULL),
			(focus && (checkpoint == selected))
		);
	}
}

static void round_timers_put(uint8_t selected, bool focus)
{
	uint8_t checkpoint;
	uint8_t count = checkpoint_count();
	uint8_t stage_round;
	uint32_t frames;
	bool valid;
	unsigned atrb;
	char *p;

	text_put(DETAIL_ROUND_PIXEL_LEFT, (DETAIL_Y + 2), "Rd", TX_CYAN);
	menu_font_put_right(
		DETAIL_ROUND_P2_SCORE_RIGHT, ((DETAIL_Y + 2) * GLYPH_H),
		"Time", font_color(TX_CYAN)
	);
	for(checkpoint = 0; checkpoint < count; checkpoint++) {
		stage_round = checkpoint_stage_round(checkpoint);
		valid = round_timer_frames(
			(stage_round & 0x0F), (stage_round >> 4), frames
		);
		atrb = (
			(focus && (checkpoint == selected)) ? TX_YELLOW : TX_WHITE
		);
		if(focus && (checkpoint == selected)) {
			text_put(
				DETAIL_ROUND_CURSOR_LEFT,
				(DETAIL_SPLIT_ROWS_Y + checkpoint), ">", TX_YELLOW
			);
		}
		p = append_u32(replay_menu_line, ((stage_round >> 4) + 1));
		field_put(
			DETAIL_ROUND_PIXEL_LEFT,
			(DETAIL_SPLIT_ROWS_Y + checkpoint), p, atrb
		);
		timer_put(
			DETAIL_ROUND_P2_SCORE_RIGHT,
			(DETAIL_SPLIT_ROWS_Y + checkpoint), frames, valid, atrb
		);
	}
}

static void round_splits_legacy_put(bool focus)
{
	uint8_t row;
	uint8_t round = (
		replay_practice() ?
			replay_user_menu_header.scenario.practice.config.round : 0
	);
	uint8_t rows = (replay_practice() ? 5 : 3);
	replay_user_menu_round_split_t near *split;

	for(row = 0; row < rows; row++, round++) {
		split = round_split(round);
		if(split == NULL) {
			break;
		}
		round_put(
			(DETAIL_SPLIT_ROWS_Y + row), round, split, true,
			(focus && (row == 0))
		);
	}
}

enum {
	REPLAY_FONT_PAGE_ROWS = 10,
	REPLAY_FONT_PAGE_COUNT = 10,
	REPLAY_FONT_SLOT_COUNT = (
		REPLAY_FONT_PAGE_ROWS * REPLAY_FONT_PAGE_COUNT
	),
};

uint8_t far replay_font_page_top(uint8_t selected)
{
	return static_cast<uint8_t>(
		(selected / REPLAY_FONT_PAGE_ROWS) * REPLAY_FONT_PAGE_ROWS
	);
}

uint8_t far replay_font_page_left(uint8_t selected)
{
	return static_cast<uint8_t>(
		(selected < REPLAY_FONT_PAGE_ROWS) ?
			(selected + REPLAY_FONT_SLOT_COUNT - REPLAY_FONT_PAGE_ROWS) :
			(selected - REPLAY_FONT_PAGE_ROWS)
	);
}

uint8_t far replay_font_page_right(uint8_t selected)
{
	return static_cast<uint8_t>(
		(selected >= (REPLAY_FONT_SLOT_COUNT - REPLAY_FONT_PAGE_ROWS)) ?
			(selected - REPLAY_FONT_SLOT_COUNT + REPLAY_FONT_PAGE_ROWS) :
			(selected + REPLAY_FONT_PAGE_ROWS)
	);
}

void far replay_font_page_put(uint8_t selected, unsigned y)
{
	char *p = append_cstr(replay_menu_line, "Page ");
	unsigned len;
	uint8_t page = ((selected / REPLAY_FONT_PAGE_ROWS) + 1);

	*p++ = static_cast<char>('0' + (page / 10));
	*p++ = static_cast<char>('0' + (page % 10));
	*p++ = '/';
	p = append_u32(p, REPLAY_FONT_PAGE_COUNT);
	*p = '\0';
	if(menu_font) {
		menu_font_put_centered(
			(RES_X / 2), 356, replay_menu_line, V_WHITE
		);
		return;
	}
	len = static_cast<unsigned int>(p - replay_menu_line);
	text_putsa(((text_width() - len) / 2), y, replay_menu_line, TX_WHITE);
}

static void save_japanese_put(
	int left, int top, int color, const shiftjis_t far *text
)
{
	graph_putsa_fx(
		left, top, (color | FX_WEIGHT_NORMAL), text
	);
}

static const shiftjis_t far SAVE_JP_TEXT[] =
	"\x82\xB1\x82\xCC\x83\x58\x83\x8D\x83\x62\x83\x67\x82\xF0"
	"\x8F\xE3\x8F\x91\x82\xAB\x82\xB5\x82\xDC\x82\xB7\x82\xA9"
	"\x81\x48\0"
	"\x83\x8A\x83\x76\x83\x8C\x83\x43\x82\xF0\x95\xDB\x91\xB6"
	"\x82\xB5\x82\xDC\x82\xB7\x82\xA9\x81\x48\0"
	"\x95\xDB\x91\xB6\x82\xF0\x82\xE2\x82\xDF\x82\xDC\x82\xB7"
	"\x82\xA9\x81\x48\0"
	"\x82\xCD\x82\xA2\0"
	"\x82\xA2\x82\xA2\x82\xA6\0"
	"\x95\xDB\x91\xB6\x82\xB5\x82\xDC\x82\xB5\x82\xBD";

#define SAVE_JP_OVERWRITE (&SAVE_JP_TEXT[0])
#define SAVE_JP_SAVE      (&SAVE_JP_TEXT[31])
#define SAVE_JP_QUIT      (&SAVE_JP_TEXT[56])
#define SAVE_JP_YES       (&SAVE_JP_TEXT[75])
#define SAVE_JP_NO        (&SAVE_JP_TEXT[80])
#define SAVE_JP_COMPLETE  (&SAVE_JP_TEXT[87])

static void save_japanese_question_put(uint8_t question)
{
	const shiftjis_t far *text;
	unsigned len;

	if(question == RFSQ_SAVE) {
		text = SAVE_JP_SAVE;
		len = 24;
	} else if(question == RFSQ_OVERWRITE) {
		text = SAVE_JP_OVERWRITE;
		len = 30;
	} else {
		text = SAVE_JP_QUIT;
		len = 18;
	}
	save_japanese_put(
		((RES_X - (len * GLYPH_HALF_W)) / 2),
		(11 * GLYPH_H), 9, text
	);
}

static void save_japanese_choices_put(bool selected_yes)
{
	save_japanese_put(
		(33 * GLYPH_HALF_W), (13 * GLYPH_H),
		(selected_yes ? REPLAY_FONT_SELECTED_COLOR : V_WHITE), SAVE_JP_YES
	);
	save_japanese_put(
		(45 * GLYPH_HALF_W), (13 * GLYPH_H),
		(selected_yes ? V_WHITE : REPLAY_FONT_SELECTED_COLOR), SAVE_JP_NO
	);
}

static void save_english_put(int left, unsigned y, int color, unsigned atrb)
{
	if(menu_font) {
		menu_font_put(
			left, (y * GLYPH_H), replay_menu_line, color
		);
	} else {
		text_putsa(
			(left / GLYPH_HALF_W), y, replay_menu_line, atrb
		);
	}
}

static void save_english_put_centered(
	unsigned y, int color, unsigned atrb, char *p
)
{
	unsigned len = static_cast<unsigned>(p - replay_menu_line);

	*p = '\0';
	if(menu_font) {
		menu_font_put_centered(
			(RES_X / 2), (y * GLYPH_H), replay_menu_line, color
		);
	} else {
		text_putsa(
			((text_width() - len) / 2), y, replay_menu_line, atrb
		);
	}
}

void far replay_font_save_dialog_put(
	uint8_t question, uint8_t slot, bool selected_yes
)
{
	char *p = replay_menu_line;

	if(!language_is_english()) {
		save_japanese_question_put(question);
		save_japanese_choices_put(selected_yes);
		return;
	}
	if(question == RFSQ_OVERWRITE) {
		p = append_cstr(p, "Overwrite Slot ");
		p = append_u32(p, slot);
	} else if(question == RFSQ_SAVE) {
		p = append_cstr(p, "Save Replay");
	} else {
		p = append_cstr(p, "Quit saving replay");
	}
	*p++ = '?';
	save_english_put_centered(11, 9, TX_CYAN, p);
	p = append_cstr(replay_menu_line, "Yes");
	*p = '\0';
	save_english_put(
		(33 * GLYPH_HALF_W), 13,
		(selected_yes ? REPLAY_FONT_SELECTED_COLOR : V_WHITE),
		(selected_yes ? TX_YELLOW : TX_WHITE)
	);
	p = append_cstr(replay_menu_line, "No");
	*p = '\0';
	save_english_put(
		(45 * GLYPH_HALF_W), 13,
		(selected_yes ? V_WHITE : REPLAY_FONT_SELECTED_COLOR),
		(selected_yes ? TX_WHITE : TX_YELLOW)
	);
}

void far replay_font_save_complete_put(void)
{
	char *p = replay_menu_line;

	if(language_is_english()) {
		p = append_cstr(p, "Saved. Press any key.");
		save_english_put_centered(22, 13, TX_CYAN, p);
		return;
	}
	save_japanese_put(
		((RES_X - (12 * GLYPH_HALF_W)) / 2),
		(22 * GLYPH_H), 9, SAVE_JP_COMPLETE
	);
}

// Keep the patch-owned segment phase stable for following OP contributions.
#pragma codestring "\x90"
