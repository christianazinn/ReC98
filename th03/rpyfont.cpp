#pragma option -zCRPYFONT_TEXT -zPRPYFONT_TEXT -dc

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/rank.h"
#include "th02/v_colors.hpp"
#include "th03/common.h"
#include "th03/menu_font.hpp"
#include "th03/opfont.hpp"
#include "th03/practice.hpp"
#include "th03/replay_format.hpp"
#include "th03/rpyfont.hpp"
#include <stddef.h>

enum {
	LIST_LEFT = 4,
	LIST_W = 72,
	HEAD_Y = 5,
	DETAIL_Y = 5,
	LIST_PIXEL_LEFT = (LIST_LEFT * GLYPH_HALF_W),
	CURSOR_PIXEL_LEFT = (LIST_PIXEL_LEFT + 8),
	SLOT_PIXEL_LEFT = 64,
	CHAR_PIXEL_LEFT = 112,
	RANK_PIXEL_LEFT = 150,
	NAME_PIXEL_LEFT = 184,
	SCORE_PIXEL_RIGHT = 480,
	STAGE_PIXEL_LEFT = 512,
	DETAIL_PIXEL_LEFT = 48,
	DETAIL_SPLIT_CURSOR_LEFT = 336,
	DETAIL_SPLIT_PIXEL_LEFT = 352,
	DETAIL_SPLIT_OPPONENT_LEFT = 384,
	DETAIL_SPLIT_SCORE_RIGHT = 600,
	DETAIL_ROUND_P1_SCORE_RIGHT = 456,
	DETAIL_ROUND_P2_SCORE_RIGHT = 568,
	DETAIL_ROUND_WINNER_LEFT = 584,
	REPLAY_FONT_FIXED_NUMERIC = 0x100,
	REPLAY_FONT_FIXED_NAME = 0x200,
	REPLAY_FONT_FIXED_MASK = (
		REPLAY_FONT_FIXED_NUMERIC | REPLAY_FONT_FIXED_NAME
	),
	REPLAY_FONT_SELECTED_COLOR = 12,
};

extern char replay_menu_line[81];
extern replay_user_header_t replay_user_menu_header;
extern replay_user_summary_ext_t replay_user_menu_summary_ext;
extern replay_user_snapshot_t replay_user_menu_snapshot;

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

static replay_user_round_split_t near *round_split(uint8_t round)
{
	uint8_t i;
	uint8_t stage = (
		replay_practice() ?
			replay_user_menu_header.scenario.practice.config.stage :
			T3_REPLAY_USER_ROUND_STAGE_VS
	);
	replay_user_round_split_t near *split;

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
	replay_user_round_split_t near *split;
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

static char *append_packed_score(char *p, const uint8_t near *score)
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

static char *append_playchar_pair(char *p, uint8_t paletted)
{
	const char *name = playchar_name(paletted);

	*p++ = name[0];
	if(name[1] != '\0') {
		*p++ = name[1];
	}
	return p;
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

static char rank_initial(uint8_t rank)
{
	const char *name = rank_name(rank);
	return name[0];
}

static const char *game_mode_name(uint8_t game_mode)
{
	switch(game_mode) {
	case GM_STORY: return "Story";
	case GM_VS_1P_CPU: return "VS 1P-CPU";
	case GM_VS_1P_2P: return "VS 1P-2P";
	case GM_VS_CPU_CPU: return "VS CPU-CPU";
	default: return "Unknown";
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

static char *append_story_lives(char *p)
{
	if(
		summary_valid() &&
		(replay_user_menu_header.final_story_lives !=
		 T3_REPLAY_USER_SUMMARY_UNKNOWN)
	) {
		return append_u32(p, replay_user_menu_header.final_story_lives);
	}
	*p++ = '-';
	*p++ = '-';
	return p;
}

static char *append_stage(char *p, uint8_t stage)
{
	if(stage == STAGE_ALL) {
		return append_cstr(p, "All");
	}
	if(stage == STAGE_NONE) {
		return append_cstr(p, "--");
	}
	return append_u32(p, (stage + 1));
}

static char *append_final_stage(char *p)
{
	uint8_t stage;

	if(replay_practice()) {
		*p++ = 'P';
		return p;
	}
	if(replay_vs()) {
		return append_cstr(p, "VS");
	}
	if(replay_user_menu_header.end_reason == RUER_COMPLETE) {
		return append_cstr(p, "All");
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

static void field_put(screen_x_t left, unsigned y, char *p, unsigned atrb)
{
	unsigned count = (p - replay_menu_line);

	*p = '\0';
	if(atrb & REPLAY_FONT_FIXED_MASK) {
		replay_font_put_fixed_n(
			left, (y * GLYPH_H), replay_menu_line, count,
			fixed_cell_width(atrb),
			font_color(atrb & 0xFF)
		);
	} else {
		menu_font_put(left, (y * GLYPH_H), replay_menu_line, font_color(atrb));
	}
}

static void field_put_right(
	screen_x_t right, unsigned y, char *p, unsigned atrb
)
{
	char *first = replay_menu_line;

	*p = '\0';
	if(atrb & REPLAY_FONT_FIXED_MASK) {
		unsigned count = (p - replay_menu_line);
		int cell_w = fixed_cell_width(atrb);
		replay_font_put_fixed_n(
			(right - (count * cell_w)), (y * GLYPH_H),
			replay_menu_line, count, cell_w, font_color(atrb & 0xFF)
		);
		return;
	}
	while((*first == ' ') && (first[1] != '\0')) {
		first++;
	}
	menu_font_put_right(right, (y * GLYPH_H), first, font_color(atrb));
}

static void text_put(
	screen_x_t left, unsigned y, const char far *str, unsigned atrb
)
{
	menu_font_put(left, (y * GLYPH_H), str, font_color(atrb));
}

static void score_put(
	screen_x_t right,
	unsigned y,
	const uint8_t near *score,
	bool valid,
	unsigned atrb
)
{
	char *p = replay_menu_line;

	p = (valid ? append_packed_score(p, score) : append_unknown_score(p));
	field_put_right(right, y, p, atrb);
}

// Keep the public replay-list renderers at their accepted entry offsets.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90"

void far replay_font_slot_line_put(
	uint8_t slot, uint8_t sel, unsigned y, bool active, bool has_replay
)
{
	char *p;
	bool selected = ((slot == sel) && active);
	unsigned atrb = (selected ? TX_YELLOW : TX_WHITE);

	if(selected) {
		menu_font_put(
			CURSOR_PIXEL_LEFT, (y * GLYPH_H),
			">", REPLAY_FONT_SELECTED_COLOR
		);
	}
	p = append_u8_2(replay_menu_line, slot);
	field_put(SLOT_PIXEL_LEFT, y, p, (atrb | REPLAY_FONT_FIXED_NUMERIC));
	if(!has_replay) {
		p = append_cstr(replay_menu_line, "none");
		field_put(NAME_PIXEL_LEFT, y, p, atrb);
		return;
	}
	p = append_playchar_pair(
		replay_menu_line, replay_user_menu_header.playchar_p1
	);
	field_put(CHAR_PIXEL_LEFT, y, p, atrb);
	p = replay_menu_line;
	*p++ = rank_initial(replay_user_menu_header.rank);
	field_put(RANK_PIXEL_LEFT, y, p, atrb);
	p = append_name(replay_menu_line);
	field_put(NAME_PIXEL_LEFT, y, p, (atrb | REPLAY_FONT_FIXED_NAME));
	score_put(
		SCORE_PIXEL_RIGHT, y, list_score(), summary_valid(),
		(atrb | REPLAY_FONT_FIXED_NUMERIC)
	);
	p = append_final_stage(replay_menu_line);
	field_put(STAGE_PIXEL_LEFT, y, p, atrb);
}

void far replay_font_columns_put(bool clear)
{
	char *p;

	if(menu_font) {
		if(clear) {
			replay_menu_span_clear(LIST_LEFT, HEAD_Y, LIST_W);
		}
		text_put(SLOT_PIXEL_LEFT, HEAD_Y, "Slot", TX_CYAN);
		text_put(CHAR_PIXEL_LEFT, HEAD_Y, "Ch", TX_CYAN);
		text_put(RANK_PIXEL_LEFT, HEAD_Y, "R", TX_CYAN);
		text_put(NAME_PIXEL_LEFT, HEAD_Y, "Name", TX_CYAN);
		menu_font_put_right(
			SCORE_PIXEL_RIGHT, (HEAD_Y * GLYPH_H),
			"Score", font_color(TX_CYAN)
		);
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

static void common_put(uint8_t slot, unsigned y)
{
	char *p = append_cstr(replay_menu_line, "Slot ");

	p = append_u8_2(p, slot);
	field_put(DETAIL_PIXEL_LEFT, y, p, TX_CYAN);
	p = append_cstr(replay_menu_line, "Status: ");
	p = append_cstr(p, (replay_practice() ? "Practice" : end_reason_name()));
	field_put(DETAIL_PIXEL_LEFT, (y + 1), p, TX_CYAN);
	p = append_cstr(replay_menu_line, "Name: ");
	p = append_name(p);
	field_put(DETAIL_PIXEL_LEFT, (y + 2), p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Date: ");
	p = append_date(p);
	field_put(DETAIL_PIXEL_LEFT, (y + 3), p, TX_WHITE);
}

static void diagnostics_put(unsigned y, bool combine_last)
{
	char *p = append_cstr(replay_menu_line, "Samples: ");

	p = append_u32(p, replay_user_menu_header.sample_count);
	field_put(DETAIL_PIXEL_LEFT, y, p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Frames: ");
	p = append_u32(p, replay_user_menu_header.final_frame_count);
	field_put(DETAIL_PIXEL_LEFT, (y + 1), p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Bytes: ");
	p = append_u32(p, replay_user_menu_header.input_size);
	if(combine_last) {
		p = append_cstr(p, "  RNG: ");
		p = append_u32(p, replay_user_menu_header.resident_rand);
		field_put(DETAIL_PIXEL_LEFT, (y + 2), p, TX_WHITE);
		return;
	}
	field_put(DETAIL_PIXEL_LEFT, (y + 2), p, TX_WHITE);
	p = append_cstr(replay_menu_line, "RNG: ");
	p = append_u32(p, replay_user_menu_header.resident_rand);
	field_put(DETAIL_PIXEL_LEFT, (y + 3), p, TX_WHITE);
}

static void round_put(
	unsigned y,
	uint8_t round,
	replay_user_round_split_t near *split,
	bool valid,
	unsigned atrb
)
{
	char *p = append_u32(replay_menu_line, (round + 1));

	field_put(DETAIL_SPLIT_PIXEL_LEFT, y, p, atrb);
	score_put(
		DETAIL_ROUND_P1_SCORE_RIGHT, y,
		(valid ? split->score_p1 : NULL), valid, atrb
	);
	score_put(
		DETAIL_ROUND_P2_SCORE_RIGHT, y,
		(valid ? split->score_p2 : NULL), valid, atrb
	);
	p = replay_menu_line;
	p = (valid ? append_round_winner(p, split->route_winner) : append_cstr(p, "-"));
	field_put(DETAIL_ROUND_WINNER_LEFT, y, p, atrb);
}

static void round_heading_put(unsigned y)
{
	text_put(DETAIL_SPLIT_PIXEL_LEFT, y, "Rd", TX_CYAN);
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

static void vs_put(void)
{
	char *p;
	uint8_t round;
	replay_user_round_split_t near *split;
	bool valid = summary_valid();

	p = append_cstr(
		replay_menu_line,
		((replay_user_menu_header.game_mode == GM_VS_1P_CPU) ?
			"P1 Score: " : "Win Score: ")
	);
	p = (valid ? append_packed_score(p, list_score()) : append_unknown_score(p));
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 4), p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Mode: ");
	p = append_cstr(p, game_mode_name(replay_user_menu_header.game_mode));
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 5), p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Rank: ");
	p = append_cstr(p, rank_name(replay_user_menu_header.rank));
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 6), p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Autofire P1: ");
	p = append_cstr(
		p, ((replay_user_menu_header.autofire & 0x01) ? "On" : "Off")
	);
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 7), p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Autofire P2: ");
	p = append_cstr(
		p, ((replay_user_menu_header.autofire & 0x02) ? "On" : "Off")
	);
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 8), p, TX_WHITE);
	p = append_cstr(replay_menu_line, "P1: ");
	p = append_cstr(p, playchar_name(replay_user_menu_header.playchar_p1));
	if(replay_user_menu_header.is_cpu_p1) p = append_cstr(p, " CPU");
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 9), p, TX_WHITE);
	p = append_cstr(replay_menu_line, "P2: ");
	p = append_cstr(p, playchar_name(replay_user_menu_header.playchar_p2));
	if(replay_user_menu_header.is_cpu_p2) p = append_cstr(p, " CPU");
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 10), p, TX_WHITE);
	diagnostics_put((DETAIL_Y + 12), false);
	text_put(DETAIL_SPLIT_PIXEL_LEFT, DETAIL_Y, "Round Splits", TX_CYAN);
	round_heading_put(DETAIL_Y + 2);
	for(round = 0; round < 3; round++) {
		split = round_split(round);
		round_put((DETAIL_Y + 3 + round), round, split, (split != NULL), TX_WHITE);
	}
}

static void story_put(
	uint8_t stage_sel, bool stage_focus, bool show_unreached_opponents
)
{
	char *p;
	uint8_t stage;
	unsigned atrb;
	bool valid = summary_valid();

	p = append_cstr(replay_menu_line, "Final Score: ");
	p = (
		valid ?
			append_packed_score(p, replay_user_menu_header.final_score) :
			append_unknown_score(p)
	);
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 4), p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Lives: ");
	p = append_story_lives(p);
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 5), p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Mode: ");
	p = append_cstr(p, game_mode_name(replay_user_menu_header.game_mode));
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 6), p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Rank: ");
	p = append_cstr(p, rank_name(replay_user_menu_header.rank));
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 7), p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Start Stage: ");
	p = append_stage(p, replay_user_menu_header.story_stage);
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 8), p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Autofire: ");
	p = append_cstr(
		p, ((replay_user_menu_header.autofire & 0x01) ? "On" : "Off")
	);
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 9), p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Player: ");
	p = append_cstr(p, playchar_name(replay_user_menu_header.playchar_p1));
	if(replay_user_menu_header.is_cpu_p1) p = append_cstr(p, " CPU");
	field_put(DETAIL_PIXEL_LEFT, (DETAIL_Y + 10), p, TX_WHITE);
	diagnostics_put((DETAIL_Y + 12), false);
	text_put(DETAIL_SPLIT_PIXEL_LEFT, DETAIL_Y, "Stage Splits", TX_CYAN);
	text_put(DETAIL_SPLIT_PIXEL_LEFT, (DETAIL_Y + 2), "St", TX_CYAN);
	text_put(DETAIL_SPLIT_OPPONENT_LEFT, (DETAIL_Y + 2), "Op", TX_CYAN);
	menu_font_put_right(
		DETAIL_SPLIT_SCORE_RIGHT, ((DETAIL_Y + 2) * GLYPH_H),
		"Score", font_color(TX_CYAN)
	);
	for(stage = 0; stage < T3_REPLAY_USER_STAGE_COUNT; stage++) {
		atrb = (
			(stage_focus && (stage == stage_sel)) ? TX_YELLOW : TX_WHITE
		);
		p = replay_menu_line;
		*p++ = static_cast<char>('1' + stage);
		field_put(
			DETAIL_SPLIT_PIXEL_LEFT, (DETAIL_Y + 3 + stage), p, atrb
		);
		p = append_playchar_pair(
			replay_menu_line,
			stage_opponent(stage, show_unreached_opponents)
		);
		field_put(
			DETAIL_SPLIT_OPPONENT_LEFT, (DETAIL_Y + 3 + stage), p, atrb
		);
		score_put(
			DETAIL_SPLIT_SCORE_RIGHT, (DETAIL_Y + 3 + stage),
			replay_user_menu_header.scenario.story.stage_scores[stage],
			(valid && (stage < replay_user_menu_header.stage_reached_count)),
			(atrb | REPLAY_FONT_FIXED_NUMERIC)
		);
	}
	if(stage_focus) {
		menu_font_put(
			DETAIL_SPLIT_CURSOR_LEFT,
			((DETAIL_Y + 3 + stage_sel) * GLYPH_H),
			">", REPLAY_FONT_SELECTED_COLOR
		);
	}
}

static void practice_put(void)
{
	char *p;
	uint8_t attempt;
	uint8_t round;
	uint8_t attempts = (
		(replay_user_menu_header.scenario.practice.config.stock ==
		 T3_PRACTICE_STOCK_VS_RULES) ?
			3 : (replay_user_menu_header.scenario.practice.config.stock + 1)
	);
	replay_user_round_split_t near *split;
	bool valid = summary_valid();

	p = append_cstr(replay_menu_line, "Final Score: ");
	p = (
		valid ?
			append_packed_score(p, replay_user_menu_header.final_score) :
			append_unknown_score(p)
	);
	field_put(DETAIL_PIXEL_LEFT, 8, p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Rank: ");
	p = append_cstr(p, rank_name(replay_user_menu_header.rank));
	field_put(DETAIL_PIXEL_LEFT, 9, p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Preset: ");
	p = append_cstr(p, (
		(replay_user_menu_header.scenario.practice.config.preset ==
		 PRACTICE_PRESET_VS_DEFAULT) ? "VS Default" : "Story Native"
	));
	field_put(DETAIL_PIXEL_LEFT, 10, p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Players: ");
	p = append_cstr(p, playchar_name(replay_user_menu_header.playchar_p1));
	p = append_cstr(p, " / ");
	p = append_cstr(p, playchar_name(replay_user_menu_header.playchar_p2));
	field_put(DETAIL_PIXEL_LEFT, 11, p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Start: Stage ");
	p = append_u32(
		p, (replay_user_menu_header.scenario.practice.config.stage + 1)
	);
	p = append_cstr(p, " / Round ");
	p = append_u32(
		p, (replay_user_menu_header.scenario.practice.config.round + 1)
	);
	if(replay_user_menu_header.scenario.practice.config.round == 5) *p++ = '+';
	field_put(DETAIL_PIXEL_LEFT, 12, p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Stock: ");
	if(
		replay_user_menu_header.scenario.practice.config.stock ==
		T3_PRACTICE_STOCK_VS_RULES
	) {
		p = append_cstr(p, "VS Rules");
	} else {
		p = append_u32(p, replay_user_menu_header.scenario.practice.config.stock);
	}
	p = append_cstr(p, " / Extends ");
	if(
		replay_user_menu_header.scenario.practice.config.preset ==
		PRACTICE_PRESET_VS_DEFAULT
	) {
		p = append_cstr(p, "--");
	} else {
		p = append_u32(
			p, replay_user_menu_header.scenario.practice.config.extends_gained
		);
	}
	field_put(DETAIL_PIXEL_LEFT, 13, p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Timer: ");
	if(
		replay_user_menu_header.scenario.practice.config.cpu_timer ==
		PRACTICE_CPU_TIMER_VS_DEFAULT
	) {
		p = append_cstr(p, "VS Default");
	} else if(
		replay_user_menu_header.scenario.practice.config.cpu_timer ==
		PRACTICE_CPU_TIMER_STORY_NATIVE
	) {
		p = append_cstr(p, "Story Native");
	} else {
		p = append_cstr(p, "Infinite");
	}
	field_put(DETAIL_PIXEL_LEFT, 14, p, TX_WHITE);
	p = append_cstr(replay_menu_line, "CPU Safety: ");
	p = append_u32(
		p,
		replay_user_menu_header.scenario.practice.config.initial_cpu_safety_frames
	);
	field_put(DETAIL_PIXEL_LEFT, 15, p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Speeds: ");
	p = append_q4(p, replay_user_menu_header.scenario.practice.config.round_speed);
	p = append_cstr(p, " / ");
	p = append_q4(p, replay_user_menu_header.scenario.practice.config.bullet_speed);
	field_put(DETAIL_PIXEL_LEFT, 16, p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Spells: P1 ");
	p = append_u32(p, replay_user_menu_header.scenario.practice.config.p1_spell);
	p = append_cstr(p, " / CPU ");
	p = append_u32(p, replay_user_menu_header.scenario.practice.config.cpu_spell);
	field_put(DETAIL_PIXEL_LEFT, 17, p, TX_WHITE);
	p = append_cstr(replay_menu_line, "Boss Level: ");
	p = append_u32(
		p, (replay_user_menu_header.scenario.practice.config.boss_level + 1)
	);
	field_put(DETAIL_PIXEL_LEFT, 18, p, TX_WHITE);
	p = append_cstr(replay_menu_line, "CPU Dmg: ");
	p = append_u32(p, replay_user_menu_header.scenario.practice.config.cpu_damage);
	field_put(DETAIL_PIXEL_LEFT, 19, p, TX_WHITE);
	diagnostics_put(20, true);
	text_put(DETAIL_SPLIT_PIXEL_LEFT, DETAIL_Y, "Round Splits", TX_CYAN);
	round_heading_put(DETAIL_Y + 2);
	for(attempt = 0; attempt < 5; attempt++) {
		round = static_cast<uint8_t>(
			replay_user_menu_header.scenario.practice.config.round + attempt
		);
		split = round_split(round);
		round_put(
			(DETAIL_Y + 3 + attempt), round, split,
			((attempt < attempts) && (split != NULL)), TX_WHITE
		);
	}
}

void far replay_font_detail_put(
	uint8_t slot,
	uint8_t stage_sel,
	bool stage_focus,
	bool show_unreached_opponents
)
{
	if(replay_practice()) {
		common_put(slot, 4);
		practice_put();
	} else if(replay_vs()) {
		common_put(slot, DETAIL_Y);
		vs_put();
	} else {
		common_put(slot, DETAIL_Y);
		story_put(stage_sel, stage_focus, show_unreached_opponents);
	}
}

void far replay_font_detail_empty_put(uint8_t slot)
{
	char *p = append_cstr(replay_menu_line, "Slot ");

	p = append_u8_2(p, slot);
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
		menu_font_put_n(glyph_left, top, str, 1, color);
		left += cell_w;
		str++;
		count--;
	}
}

// Keep the following patch-owned segment at its accepted paragraph phase.
#pragma codestring "\x90\x90\x90\x90\x90\x90"
