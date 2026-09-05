/* TH01 patch-owned FUUIN registration language presentation. */

#pragma option -zCT1LANG_FUUIN_TEXT -G-

#include "th01/langfuu.hpp"

#include "th01/hardware/graph.h"
#include "th01/hardware/grppsafx.h"
#include "th01/language.hpp"
#include "th01/rank.h"

#define T1LANG_FUUIN_REGIST_TEXT_CAPACITY 18
#define T1LANG_FUUIN_REGIST_TITLE_CAPACITY 40

static bool t1lang_fuuin_regist_english_build(
	shiftjis_t *out, t1lang_fuuin_regist_text_t text
)
{
	shiftjis_t *p = out;
	#define T1LF_PUTC(c) *p++ = (c)
	#define T1LF_SPACE() T1LF_PUTC(' ')
	#define T1LF_WORD1(a) T1LF_PUTC(a)
	#define T1LF_WORD2(a, b) \
		*reinterpret_cast<uint16_t *>(p) = static_cast<uint16_t>( \
			static_cast<uint8_t>(a) | \
			(static_cast<uint16_t>(static_cast<uint8_t>(b)) << 8) \
		); \
		p += 2
	#define T1LF_WORD3(a, b, c) T1LF_WORD2(a, b); T1LF_WORD1(c)
	#define T1LF_WORD4(a, b, c, d) T1LF_WORD2(a, b); T1LF_WORD2(c, d)
	#define T1LF_FULLWIDTH_SPACE() T1LF_WORD2(0x81, 0x40)

	switch(text) {
	case T1LFRT_ALPHABET_END:
		T1LF_WORD2('E', 'd');
		break;
	case T1LFRT_HEADER_PLACE:
		T1LF_FULLWIDTH_SPACE(); T1LF_WORD4('L', 'e', 'g', 'a');
		T1LF_WORD2('c', 'y');
		break;
	case T1LFRT_HEADER_NAME:
		T1LF_FULLWIDTH_SPACE(); T1LF_SPACE(); T1LF_WORD4('N', 'a', 'm', 'e');
		break;
	case T1LFRT_HEADER_SCORE:
		T1LF_FULLWIDTH_SPACE(); T1LF_WORD4('H', 'i', 'g', 'h'); T1LF_SPACE();
		T1LF_WORD4('S', 'c', 'o', 'r'); T1LF_WORD1('e');
		break;
	case T1LFRT_HEADER_STAGE_ROUTE:
		T1LF_SPACE(); T1LF_SPACE(); T1LF_WORD4('S', 't', 'a', 'g');
		T1LF_WORD1('e'); T1LF_SPACE(); T1LF_WORD1('-'); T1LF_SPACE();
		T1LF_WORD4('R', 'o', 'u', 't'); T1LF_WORD1('e'); T1LF_SPACE();
		break;
	case T1LFRT_PLACE_0:
		T1LF_FULLWIDTH_SPACE(); T1LF_SPACE(); T1LF_WORD3('G', 'o', 'd');
		break;
	case T1LFRT_PLACE_1:
		T1LF_SPACE(); T1LF_WORD4('A', 't', 'a', 'v');
		T1LF_WORD3('a', 'k', 'a'); T1LF_SPACE(); T1LF_SPACE();
		break;
	case T1LFRT_PLACE_2:
		T1LF_FULLWIDTH_SPACE(); T1LF_WORD4('D', 'e', 'i', 't');
		T1LF_WORD1('y');
		break;
	case T1LFRT_PLACE_3:
		T1LF_FULLWIDTH_SPACE(); T1LF_SPACE(); T1LF_WORD3('F', 'a', 'e');
		break;
	case T1LFRT_PLACE_4:
		T1LF_FULLWIDTH_SPACE(); T1LF_SPACE(); T1LF_WORD3('I', 'm', 'p');
		break;
	case T1LFRT_PLACE_5:
		T1LF_SPACE(); T1LF_WORD4('I', 'm', 'm', 'o');
		T1LF_WORD4('r', 't', 'a', 'l'); T1LF_SPACE();
		break;
	case T1LFRT_PLACE_6:
		T1LF_FULLWIDTH_SPACE(); T1LF_SPACE(); T1LF_WORD3('E', 'l', 'f');
		break;
	case T1LFRT_PLACE_7:
		T1LF_SPACE(); T1LF_SPACE(); T1LF_WORD4('S', 'e', 'e', 'r');
		break;
	case T1LFRT_PLACE_8:
		T1LF_SPACE(); T1LF_WORD4('M', 'e', 'd', 'i'); T1LF_WORD2('u', 'm');
		break;
	case T1LFRT_PLACE_9:
		T1LF_SPACE(); T1LF_SPACE(); T1LF_WORD4('P', 'u', 'p', 'i');
		T1LF_WORD1('l');
		break;
	case T1LFRT_STAGE_MAKAI:
		T1LF_WORD4('M', 'k', 'a', 'i');
		break;
	case T1LFRT_STAGE_JIGOKU:
		T1LF_WORD4('H', 'e', 'l', 'l');
		break;
	default:
		return false;
	}
	*p = '\0';

	#undef T1LF_FULLWIDTH_SPACE
	#undef T1LF_WORD4
	#undef T1LF_WORD3
	#undef T1LF_WORD2
	#undef T1LF_WORD1
	#undef T1LF_SPACE
	#undef T1LF_PUTC
	return true;
}

void far t1lang_fuuin_regist_put(
	screen_x_t left, screen_y_t top, int col_and_fx,
	const shiftjis_t *japanese, t1lang_fuuin_regist_text_t text
)
{
	shiftjis_t english[T1LANG_FUUIN_REGIST_TEXT_CAPACITY];
	const shiftjis_t *selected = japanese;

	if((t1_language_get() == T1LANG_ENGLISH) &&
		t1lang_fuuin_regist_english_build(english, text)) {
		selected = english;
	}
	graph_putsa_fx(left, top, col_and_fx, selected);
}

static shiftjis_t *t1lang_fuuin_rank_english_build(shiftjis_t *p, int rank)
{
	switch(rank) {
	case RANK_EASY:
		*p++ = 0x81; *p++ = 0x40;
		*p++ = ' '; *p++ = ' '; *p++ = ' ';
		*p++ = 'E'; *p++ = 'a'; *p++ = 's'; *p++ = 'y';
		*p++ = ' '; *p++ = 0x81; *p++ = 0x40;
		break;
	case RANK_NORMAL:
		*p++ = 0x81; *p++ = 0x40; *p++ = ' ';
		*p++ = 'N'; *p++ = 'o'; *p++ = 'r'; *p++ = 'm';
		*p++ = 'a'; *p++ = 'l';
		*p++ = ' '; *p++ = 0x81; *p++ = 0x40;
		break;
	case RANK_HARD:
		*p++ = 0x81; *p++ = 0x40; *p++ = ' ';
		*p++ = 'H'; *p++ = 'a'; *p++ = 'r'; *p++ = 'd';
		*p++ = ' '; *p++ = 0x81; *p++ = 0x40;
		*p++ = 0x81; *p++ = 0x40;
		break;
	case RANK_LUNATIC:
		*p++ = ' '; *p++ = ' '; *p++ = ' ';
		*p++ = 'L'; *p++ = 'u'; *p++ = 'n'; *p++ = 'a';
		*p++ = 't'; *p++ = 'i'; *p++ = 'c';
		*p++ = ' '; *p++ = ' ';
		break;
	}
	return p;
}

bool far t1lang_fuuin_regist_title_put(
	screen_x_t left, screen_y_t top, int col_and_fx,
	const shiftjis_t *japanese_format, int rank
)
{
	shiftjis_t english[T1LANG_FUUIN_REGIST_TITLE_CAPACITY];
	shiftjis_t *p = english;

	if((t1_language_get() != T1LANG_ENGLISH) ||
		(rank < RANK_EASY) || (rank > RANK_LUNATIC)) {
		return false;
	}

	// The English v1.00 donor keeps the Japanese game title and replaces only
	// the registration subtitle and rank.
	while((*japanese_format != '\0') && (*japanese_format != 0x81)) {
		*p++ = *japanese_format++;
	}
	*p++ = 0x81; *p++ = 0x40;
	*p++ = 'H'; *p++ = 'e'; *p++ = 'r'; *p++ = 'o'; *p++ = ' ';
	*p++ = 'S'; *p++ = 'c'; *p++ = 'r'; *p++ = 'o'; *p++ = 'l'; *p++ = 'l';
	*p++ = ' ';
	p = t1lang_fuuin_rank_english_build(p, rank);
	*p = '\0';
	graph_putsa_fx(left, top, col_and_fx, english);
	return true;
}

#define T1LFV_PUTC(c) *p++ = (c)
#define T1LFV_WORD1(a) T1LFV_PUTC(a)
#define T1LFV_WORD2(a, b) T1LFV_WORD1(a); T1LFV_WORD1(b)
#define T1LFV_WORD3(a, b, c) T1LFV_WORD2(a, b); T1LFV_WORD1(c)
#define T1LFV_WORD4(a, b, c, d) T1LFV_WORD2(a, b); T1LFV_WORD2(c, d)
#define T1LFV_PAIR(a, b) T1LFV_PUTC(a); T1LFV_PUTC(b)

static shiftjis_t *t1lang_fuuin_verdict_spaces(
	shiftjis_t *p, int halfwidth, int fullwidth
)
{
	while(halfwidth-- > 0) {
		*p++ = ' ';
	}
	while(fullwidth-- > 0) {
		*p++ = 0x81;
		*p++ = 0x40;
	}
	return p;
}

static shiftjis_t *t1lang_fuuin_verdict_range(
	shiftjis_t *p, int first_tens, int first_ones,
	int last_tens, int last_ones
)
{
	T1LFV_PAIR(0x81, 0x69);
	if(first_tens != 0) {
		T1LFV_PAIR(0x82, (0x4F + first_tens));
	}
	T1LFV_PAIR(0x82, (0x4F + first_ones));
	T1LFV_PAIR(0x81, 0x60);
	if(last_tens != 0) {
		T1LFV_PAIR(0x82, (0x4F + last_tens));
	}
	T1LFV_PAIR(0x82, (0x4F + last_ones));
	T1LFV_PAIR(0x81, 0x6A);
	return p;
}

bool far t1lang_fuuin_verdict_english_build(
	shiftjis_t *out, t1lang_fuuin_verdict_text_t text
)
{
	shiftjis_t *p = out;

	switch(text) {
	case T1LFVT_RANK:
		T1LFV_WORD4('R', 'a', 'n', 'k');
		p = t1lang_fuuin_verdict_spaces(p, 2, 10);
		T1LFV_WORD4(' ', ' ', '%', 's');
		break;
	case T1LFVT_SCORE_HIGHEST:
		T1LFV_WORD4('S', 'c', 'o', 'r'); T1LFV_WORD1('e');
		p = t1lang_fuuin_verdict_spaces(p, 9, 6);
		T1LFV_WORD4(' ', ' ', '%', '7'); T1LFV_WORD2('l', 'u');
		break;
	case T1LFVT_SCORE:
		T1LFV_WORD4('H', 'i', ' ', 'S');
		T1LFV_WORD4('c', 'o', 'r', 'e');
		p = t1lang_fuuin_verdict_spaces(p, 0, 9);
		T1LFV_WORD4(' ', ' ', '%', '7'); T1LFV_WORD2('l', 'u');
		break;
	case T1LFVT_CONTINUES:
		T1LFV_WORD4('C', 'o', 'n', 't');
		T1LFV_WORD4('i', 'n', 'u', 'e');
		T1LFV_WORD4('s', ' ', 'U', 's');
		T1LFV_WORD2('e', 'd');
		p = t1lang_fuuin_verdict_spaces(p, 4, 0);
		break;
	case T1LFVT_SHRINE:
		T1LFV_PAIR(0x81, 0x40); T1LFV_WORD4('S', 'h', 'r', 'i');
		T1LFV_WORD2('n', 'e');
		p = t1lang_fuuin_verdict_spaces(p, 0, 3);
		p = t1lang_fuuin_verdict_spaces(p, 4, 0);
		p = t1lang_fuuin_verdict_range(p, 0, 1, 0, 5);
		T1LFV_WORD1(' '); T1LFV_PAIR(0x81, 0x40); T1LFV_WORD1(' ');
		T1LFV_WORD4('%', '3', 'l', 'u');
		break;
	case T1LFVT_MAKAI_1:
		T1LFV_PAIR(0x81, 0x40); T1LFV_WORD4('G', 'a', 't', 'e');
		p = t1lang_fuuin_verdict_spaces(p, 2, 3);
		p = t1lang_fuuin_verdict_spaces(p, 4, 0);
		p = t1lang_fuuin_verdict_range(p, 0, 6, 1, 0);
		T1LFV_WORD2(' ', ' '); T1LFV_WORD4('%', '3', 'l', 'u');
		break;
	case T1LFVT_JIGOKU_1:
		T1LFV_PAIR(0x81, 0x40);
		T1LFV_WORD4('W', 'a', 'y', 's');
		T1LFV_WORD4('i', 'd', 'e', ' ');
		T1LFV_WORD4('S', 'h', 'r', 'i'); T1LFV_WORD2('n', 'e');
		T1LFV_WORD2(' ', ' ');
		p = t1lang_fuuin_verdict_range(p, 0, 6, 1, 0);
		T1LFV_WORD2(' ', ' '); T1LFV_WORD4('%', '3', 'l', 'u');
		break;
	case T1LFVT_MAKAI_2:
		T1LFV_WORD2(' ', ' '); T1LFV_WORD4('V', 'i', 'n', 'a');
		T1LFV_WORD4(' ', 'R', 'u', 'i'); T1LFV_WORD2('n', 's');
		p = t1lang_fuuin_verdict_spaces(p, 6, 0);
		p = t1lang_fuuin_verdict_range(p, 1, 1, 1, 5);
		T1LFV_WORD4('%', '3', 'l', 'u');
		break;
	case T1LFVT_JIGOKU_2:
		T1LFV_PAIR(0x81, 0x40);
		T1LFV_WORD4('C', 'e', 's', 's'); T1LFV_WORD4('p', 'o', 'o', 'l');
		T1LFV_PAIR(0x81, 0x40); T1LFV_PAIR(0x81, 0x40);
		p = t1lang_fuuin_verdict_spaces(p, 4, 0);
		p = t1lang_fuuin_verdict_range(p, 1, 1, 1, 5);
		T1LFV_WORD4('%', '3', 'l', 'u');
		break;
	case T1LFVT_MAKAI_3:
		T1LFV_WORD2(' ', ' '); T1LFV_WORD4('F', 'a', 'l', 'l');
		T1LFV_WORD4('e', 'n', ' ', 'T');
		T1LFV_WORD4('e', 'm', 'p', 'l'); T1LFV_WORD1('e');
		p = t1lang_fuuin_verdict_spaces(p, 3, 0);
		p = t1lang_fuuin_verdict_range(p, 1, 5, 2, 0);
		T1LFV_WORD4('%', '3', 'l', 'u');
		break;
	case T1LFVT_JIGOKU_3:
		T1LFV_PAIR(0x81, 0x40); T1LFV_WORD4('Q', 'u', 'i', 'e');
		T1LFV_WORD4('t', ' ', 'T', 'e');
		T1LFV_WORD4('m', 'p', 'l', 'e');
		p = t1lang_fuuin_verdict_spaces(p, 4, 0);
		p = t1lang_fuuin_verdict_range(p, 1, 5, 2, 0);
		T1LFV_WORD4('%', '3', 'l', 'u');
		break;
	case T1LFVT_MAKAI_TOTAL:
		T1LFV_WORD2(' ', ' '); T1LFV_WORD4('M', 'a', 'k', 'a');
		T1LFV_WORD4('i', ' ', 'T', 'o');
		T1LFV_WORD3('t', 'a', 'l');
		p = t1lang_fuuin_verdict_spaces(p, 3, 6);
		T1LFV_WORD4(' ', ' ', '%', '5'); T1LFV_WORD2('l', 'u');
		break;
	case T1LFVT_JIGOKU_TOTAL:
		T1LFV_PAIR(0x81, 0x40); T1LFV_WORD4('H', 'e', 'l', 'l');
		T1LFV_WORD4(' ', 'T', 'o', 't'); T1LFV_WORD2('a', 'l');
		p = t1lang_fuuin_verdict_spaces(p, 4, 7);
		T1LFV_WORD4('%', '5', 'l', 'u');
		break;
	case T1LFVT_HEADING:
		T1LFV_WORD1(' ');
		T1LFV_PAIR(0x81, 0x9A); T1LFV_PAIR(0x81, 0x9A);
		T1LFV_PAIR(0x81, 0x9A);
		T1LFV_WORD4('A', 's', 's', 'e');
		T1LFV_WORD4('s', 's', 'm', 'e'); T1LFV_WORD2('n', 't');
		T1LFV_PAIR(0x81, 0x9A); T1LFV_PAIR(0x81, 0x9A);
		T1LFV_PAIR(0x81, 0x9A); T1LFV_WORD1(' ');
		break;
	default:
		return false;
	}
	*p = '\0';
	return true;
}

bool far t1lang_fuuin_verdict_title_english_build(
	shiftjis_t *out, int group, int level
)
{
	shiftjis_t *p = out;
	const int title = ((group * 6) + level);

	if((group < 0) || (group >= 3) || (level < 0) || (level >= 6)) {
		return false;
	}
	switch(title) {
	case 0:
		T1LFV_PAIR(0x81, 0x40); T1LFV_WORD4('M', 'o', 'n', 'k');
		T1LFV_WORD2('e', 'y'); T1LFV_WORD2(' ', ' '); T1LFV_PAIR(0x81, 0x40);
		break;
	case 1: T1LFV_WORD4('H', 'o', 'm', 'i'); T1LFV_WORD4('n', 'o', 'i', 'd'); break;
	case 2: T1LFV_PAIR(0x81, 0x40); T1LFV_WORD4('H', 'u', 'm', 'a'); T1LFV_WORD1('n'); break;
	case 3: T1LFV_WORD4('S', 'p', 'i', 'r'); T1LFV_WORD2('i', 't'); break;
	case 4: T1LFV_PAIR(0x81, 0x40); T1LFV_WORD4('D', 'e', 'i', 't'); T1LFV_WORD2('y', ' '); break;
	case 5: T1LFV_PAIR(0x81, 0x40); T1LFV_WORD4('T', 'i', 't', 'a'); T1LFV_WORD1('n'); break;
	case 6: T1LFV_WORD1(' '); T1LFV_WORD4('C', 'h', 'i', 'l'); T1LFV_WORD4('d', ' ', 'G', 'a'); T1LFV_WORD4('m', 'e', 'r', ' '); T1LFV_WORD1(' '); break;
	case 7: T1LFV_PAIR(0x81, 0x40); T1LFV_WORD4('L', 'o', 'w', ' '); T1LFV_WORD4('S', 'c', 'o', 'r'); T1LFV_WORD4('e', 'r', ' ', ' '); break;
	case 8: T1LFV_PAIR(0x81, 0x40); T1LFV_WORD4('M', 'i', 'd', ' '); T1LFV_WORD4('S', 'c', 'o', 'r'); T1LFV_WORD4('e', 'r', ' ', ' '); T1LFV_WORD2(' ', ' '); break;
	case 9: T1LFV_PAIR(0x81, 0x40); T1LFV_WORD4('H', 'i', 'g', 'h'); T1LFV_WORD4(' ', 'S', 'c', 'o'); T1LFV_WORD4('r', 'e', 'r', ' '); break;
	case 10: T1LFV_PAIR(0x81, 0x40); T1LFV_WORD4('S', 'u', 'p', 'e'); T1LFV_WORD4('r', ' ', 'G', 'a'); T1LFV_WORD4('m', 'e', 'r', ' '); T1LFV_WORD2(' ', ' '); break;
	case 11: T1LFV_WORD4('P', 'e', 'e', 'r'); T1LFV_WORD4('l', 'e', 's', 's'); T1LFV_WORD4(' ', 'G', 'a', 'm'); T1LFV_WORD2('e', 'r'); break;
	case 12: T1LFV_WORD4('M', 'o', 'l', 'd'); T1LFV_WORD4('y', ' ', 'O', 'r'); T1LFV_WORD4('a', 'n', 'g', 'e'); break;
	case 13: T1LFV_WORD1(' '); T1LFV_WORD4('C', 'l', 'a', 'i'); T1LFV_WORD4('r', 'v', 'o', 'i'); T1LFV_WORD4('y', 'a', 'n', 't'); T1LFV_WORD1(' '); break;
	case 14: T1LFV_WORD4('G', 'o', 'l', 'd'); T1LFV_WORD4(' ', 'A', 'p', 'p'); T1LFV_WORD2('l', 'e'); break;
	case 15: T1LFV_WORD4('L', 'o', 't', 'u'); T1LFV_WORD4('s', ' ', 'L', 'e'); T1LFV_WORD2('a', 'f'); break;
	case 16: T1LFV_WORD4('S', 'a', 'c', 'r'); T1LFV_WORD4('e', 'd', ' ', 'S'); T1LFV_WORD4('c', 'r', 'o', 'l'); T1LFV_WORD2('l', ' '); break;
	case 17: T1LFV_WORD1(' '); T1LFV_WORD4('A', 'm', 'b', 'r'); T1LFV_WORD4('o', 's', 'i', 'a'); T1LFV_WORD1(' '); break;
	}
	*p = '\0';
	return true;
}

static int t1lang_fuuin_verdict_digits(unsigned long value)
{
	int digits = 1;
	while(value >= 10) {
		value /= 10;
		digits++;
	}
	return digits;
}

static pixel_t t1lang_fuuin_verdict_number_w(
	t1lang_fuuin_verdict_text_t text, unsigned long value, int field_digits
)
{
	shiftjis_t english[40];
	int digits = t1lang_fuuin_verdict_digits(value);
	if(digits < field_digits) {
		digits = field_digits;
	}
	if(!t1lang_fuuin_verdict_english_build(english, text)) {
		return 0;
	}
	// Every donor numeric conversion is four halfwidth format characters.
	return (
		text_extent_fx(0, english) - (4 * GLYPH_HALF_W) +
		(digits * GLYPH_HALF_W)
	);
}

static void t1lang_fuuin_verdict_max_assign(pixel_t& maximum, pixel_t value)
{
	if(value > maximum) {
		maximum = value;
	}
}

pixel_t far t1lang_fuuin_verdict_max_w(
	const shiftjis_t *rank, unsigned long score_highest, unsigned long score,
	const int32_t *continues_per_scene, unsigned long continues_total,
	bool16 makai_route
)
{
	shiftjis_t english[40];
	pixel_t maximum = 0;

	if(t1lang_fuuin_verdict_english_build(english, T1LFVT_RANK)) {
		maximum = (
			text_extent_fx(0, english) - (2 * GLYPH_HALF_W) +
			text_extent_fx(0, rank)
		);
	}
	t1lang_fuuin_verdict_max_assign(maximum,
		t1lang_fuuin_verdict_number_w(T1LFVT_SCORE_HIGHEST, score_highest, 7)
	);
	t1lang_fuuin_verdict_max_assign(maximum,
		t1lang_fuuin_verdict_number_w(T1LFVT_SCORE, score, 7)
	);
	t1lang_fuuin_verdict_max_assign(maximum,
		t1lang_fuuin_verdict_number_w(
			T1LFVT_SHRINE, static_cast<unsigned long>(continues_per_scene[0]), 3
		)
	);
	t1lang_fuuin_verdict_max_assign(maximum,
		t1lang_fuuin_verdict_number_w(
			makai_route ? T1LFVT_MAKAI_1 : T1LFVT_JIGOKU_1,
			static_cast<unsigned long>(continues_per_scene[1]), 3
		)
	);
	t1lang_fuuin_verdict_max_assign(maximum,
		t1lang_fuuin_verdict_number_w(
			makai_route ? T1LFVT_MAKAI_2 : T1LFVT_JIGOKU_2,
			static_cast<unsigned long>(continues_per_scene[2]), 3
		)
	);
	t1lang_fuuin_verdict_max_assign(maximum,
		t1lang_fuuin_verdict_number_w(
			makai_route ? T1LFVT_MAKAI_3 : T1LFVT_JIGOKU_3,
			static_cast<unsigned long>(continues_per_scene[3]), 3
		)
	);
	t1lang_fuuin_verdict_max_assign(maximum,
		t1lang_fuuin_verdict_number_w(
			makai_route ? T1LFVT_MAKAI_TOTAL : T1LFVT_JIGOKU_TOTAL,
			continues_total, 5
		)
	);
	return maximum;
}

#undef T1LFV_PAIR
#undef T1LFV_WORD4
#undef T1LFV_WORD3
#undef T1LFV_WORD2
#undef T1LFV_WORD1
#undef T1LFV_PUTC

#pragma codeseg
