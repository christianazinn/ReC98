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

#pragma codeseg
