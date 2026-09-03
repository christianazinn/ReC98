#pragma option -zCLANGUAGE_MAINL_TEXT -zPLANGUAGE_MAINL_TEXT

#include "shiftjis.hpp"
#include "th03/language.hpp"
#include "th03/lngml.hpp"

extern const shiftjis_t* CHAR_TITLE[];
extern const shiftjis_t* CHAR_NAME[];
extern const shiftjis_t* REGIST_PLAYCHARS[];

static const shiftjis_t far TITLE1_REIMU[] = "Miko Who Protects Dream    ";
static const shiftjis_t far TITLE2_REIMU[] = "and Tradition";
static const shiftjis_t far TITLE1_MIMA[] = "Spirit Who Left Fate to Eter";
static const shiftjis_t far TITLE2_MIMA[] = " nal Dream";
static const shiftjis_t far TITLE1_MARISA[] = "   A Being Made of Magic    ";
static const shiftjis_t far TITLE2_MARISA[] = " and Red Dreams";
static const shiftjis_t far TITLE1_ELLEN[] = "Hardworking Witch Who Dreams";
static const shiftjis_t far TITLE2_ELLEN[] = "   of Love";
static const shiftjis_t far TITLE1_KOTOHIME[] = "Princess Dreaming of Beauty ";
static const shiftjis_t far TITLE2_KOTOHIME[] = "in Danmaku";
static const shiftjis_t far TITLE1_KANA[] = " Poltergeist Maiden Who Has ";
static const shiftjis_t far TITLE2_KANA[] = "Lost Her Dreams  ";
static const shiftjis_t far TITLE1_RIKAKO[] = "     Scientist Searching    ";
static const shiftjis_t far TITLE2_RIKAKO[] = "    For Dreams";
static const shiftjis_t far TITLE1_CHIYURI[] = "Resident of Fantasy Who Runs";
static const shiftjis_t far TITLE2_CHIYURI[] = "  Through Time ";
static const shiftjis_t far TITLE1_YUMEMI[] = "       Fantasy Legend       ";
static const shiftjis_t far TITLE2_YUMEMI[] = "            ";

#define NAME_BAR "\x81\x5C"
static const shiftjis_t far NAME_REIMU[] =
	"     " NAME_BAR " Reimu Hakurei " NAME_BAR "    ";
static const shiftjis_t far NAME_MIMA[] =
	"         " NAME_BAR " Mima " NAME_BAR "         ";
static const shiftjis_t far NAME_MARISA[] =
	"    " NAME_BAR " Marisa Kirisame " NAME_BAR "   ";
static const shiftjis_t far NAME_ELLEN[] =
	"        " NAME_BAR " Ellen " NAME_BAR "         ";
static const shiftjis_t far NAME_KOTOHIME[] =
	"       " NAME_BAR " Kotohime " NAME_BAR "       ";
static const shiftjis_t far NAME_KANA[] =
	"     " NAME_BAR " Kana Anaberal " NAME_BAR "    ";
static const shiftjis_t far NAME_RIKAKO[] =
	"    " NAME_BAR " Rikako Asakura " NAME_BAR "    ";
static const shiftjis_t far NAME_CHIYURI[] =
	" " NAME_BAR " Chiyuri Kitashirakawa " NAME_BAR;
static const shiftjis_t far NAME_YUMEMI[] =
	"    " NAME_BAR " Yumemi Okazaki " NAME_BAR "    ";

static const shiftjis_t far * const far TITLE1_EN[] = {
	TITLE1_REIMU, TITLE1_MIMA, TITLE1_MARISA, TITLE1_ELLEN,
	TITLE1_KOTOHIME, TITLE1_KANA, TITLE1_RIKAKO, TITLE1_CHIYURI,
	TITLE1_YUMEMI,
};
static const shiftjis_t far * const far TITLE2_EN[] = {
	TITLE2_REIMU, TITLE2_MIMA, TITLE2_MARISA, TITLE2_ELLEN,
	TITLE2_KOTOHIME, TITLE2_KANA, TITLE2_RIKAKO, TITLE2_CHIYURI,
	TITLE2_YUMEMI,
};
static const shiftjis_t far * const far NAME_EN[] = {
	NAME_REIMU, NAME_MIMA, NAME_MARISA, NAME_ELLEN, NAME_KOTOHIME,
	NAME_KANA, NAME_RIKAKO, NAME_CHIYURI, NAME_YUMEMI,
};

static const shiftjis_t far REGIST_NONE_EN[] = "  No Entry! ";
static const shiftjis_t far REGIST_REIMU_EN[] = "  Reimu     ";
static const shiftjis_t far REGIST_MIMA_EN[] = "  Mima      ";
static const shiftjis_t far REGIST_MARISA_EN[] = "  Marisa    ";
static const shiftjis_t far REGIST_ELLEN_EN[] = "  Ellen     ";
static const shiftjis_t far REGIST_KOTOHIME_EN[] = "  Kotohime  ";
static const shiftjis_t far REGIST_KANA_EN[] = "  Kana      ";
static const shiftjis_t far REGIST_RIKAKO_EN[] = "  Rikako    ";
static const shiftjis_t far REGIST_CHIYURI_EN[] = "  Chiyuri   ";
static const shiftjis_t far REGIST_YUMEMI_EN[] = "  Yumemi    ";
static const shiftjis_t far * const far REGIST_EN[] = {
	REGIST_NONE_EN, REGIST_REIMU_EN, REGIST_MIMA_EN, REGIST_MARISA_EN,
	REGIST_ELLEN_EN, REGIST_KOTOHIME_EN, REGIST_KANA_EN, REGIST_RIKAKO_EN,
	REGIST_CHIYURI_EN, REGIST_YUMEMI_EN,
};

void far language_mainl_apply(void)
{
	int i;
	if(!language_is_english()) {
		return;
	}
	for(i = 0; i < 9; i++) {
		CHAR_TITLE[i * 2] = TITLE1_EN[i];
		CHAR_NAME[i * 2] = NAME_EN[i];
	}
	for(i = 0; i < 10; i++) {
		REGIST_PLAYCHARS[i] = REGIST_EN[i];
	}
}

const shiftjis_t far * far language_mainl_title2(int char_id)
{
	return TITLE2_EN[char_id];
}

// Keep the following score and compiler-runtime segments at their accepted phase.
#pragma codestring "\x90\x90\x90\x90\x90\x90"
