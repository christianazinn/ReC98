#include "shiftjis.hpp"
#include "th02/main/language_presentation.hpp"

// This file is textually included by langm.cpp and therefore lives in MAIN's
// ungrouped patch tail. It intentionally keeps every translated byte in BSS:
// no initialized data is introduced ahead of the original runtime layout.
#pragma codeseg T2LANGMAINP_TEXT

#define T2_LANGUAGE_MAIN_BGM_COUNT 12
#define T2_LANGUAGE_MAIN_BGM_TITLE_CAPACITY 32
#define T2_LANGUAGE_MAIN_STAGE_COUNT 6
#define T2_LANGUAGE_MAIN_STAGE_TITLE_CAPACITY 40

extern "C" shiftjis_t near *BGM_TITLES[];
extern "C" shiftjis_t near *STAGE_TITLES[];
extern "C" uint8_t STAGE_TITLE_HALFLENGTHS[];

static bool t2_lmp_ready;
static shiftjis_t near *t2_language_main_jp_bgm_titles[T2_LANGUAGE_MAIN_BGM_COUNT];
static shiftjis_t near *t2_language_main_jp_stage_titles[T2_LANGUAGE_MAIN_STAGE_COUNT];
static uint8_t t2_language_main_jp_stage_title_halflengths[
	T2_LANGUAGE_MAIN_STAGE_COUNT
];
static shiftjis_t t2_language_main_en_bgm_titles[
	T2_LANGUAGE_MAIN_BGM_COUNT
][T2_LANGUAGE_MAIN_BGM_TITLE_CAPACITY];
static shiftjis_t t2_language_main_en_stage_titles[
	T2_LANGUAGE_MAIN_STAGE_COUNT
][T2_LANGUAGE_MAIN_STAGE_TITLE_CAPACITY];
static uint8_t t2_language_main_en_stage_title_halflengths[
	T2_LANGUAGE_MAIN_STAGE_COUNT
];

static void t2_language_main_bgm_title_build(int id, shiftjis_t near *p)
{
	#define P(c) (*p++ = static_cast<shiftjis_t>(c))
	switch(id) {
	case 0:
		P('H'); P('a'); P('k'); P('u'); P('r'); P('e'); P('i'); P(' ');
		P(0x81); P(0x60); P(' '); P('E'); P('a'); P('s'); P('t'); P('e');
		P('r'); P('n'); P(' '); P('W'); P('i'); P('n'); P('d'); P(' ');
		break;
	case 1:
		P('S'); P('h'); P('e'); P('\''); P('s'); P(' '); P('i'); P('n');
		P(' '); P('a'); P(' '); P('T'); P('e'); P('m'); P('p'); P('e');
		P('r'); P('!'); P('!'); P(' ');
		break;
	case 2:
		P('E'); P('n'); P('d'); P(' '); P('o'); P('f'); P(' '); P('D');
		P('a'); P('y'); P('l'); P('i'); P('g'); P('h'); P('t'); P(' ');
		break;
	case 3:
		P('P'); P('o'); P('w'); P('e'); P('r'); P(' '); P('o'); P('f');
		P(' '); P('D'); P('a'); P('r'); P('k'); P('n'); P('e'); P('s');
		P('s'); P(' ');
		break;
	case 4:
		P('W'); P('o'); P('r'); P('l'); P('d'); P(' '); P('o'); P('f');
		P(' '); P('E'); P('m'); P('p'); P('t'); P('y'); P(' '); P('D');
		P('r'); P('e'); P('a'); P('m'); P('s'); P(' ');
		break;
	case 5:
		P('B'); P('e'); P('t'); P(' '); P('o'); P('n'); P(' '); P('D');
		P('e'); P('a'); P('t'); P('h');
		break;
	case 6:
		P('H'); P('i'); P('m'); P('o'); P('r'); P('o'); P('g'); P('i');
		P(','); P(' '); P('B'); P('u'); P('r'); P('n'); P(' '); P('i');
		P('n'); P(' '); P('V'); P('i'); P('o'); P('l'); P('e'); P('t');
		break;
	case 7:
		P('L'); P('o'); P('v'); P('e'); P('-'); P('C'); P('o'); P('l');
		P('o'); P('u'); P('r'); P('e'); P('d'); P(' '); P('M'); P('a');
		P('g'); P('i'); P('c'); P(' ');
		break;
	case 8:
		P('A'); P(' '); P('P'); P('h'); P('a'); P('n'); P('t'); P('o');
		P('m'); P('\''); P('s'); P(' '); P('B'); P('o'); P('i'); P('s');
		P('t'); P('e'); P('r'); P('o'); P('u'); P('s'); P(' '); P('D');
		P('a'); P('n'); P('c'); P('e');
		break;
	case 9:
		P('C'); P('o'); P('m'); P('p'); P('l'); P('e'); P('t'); P('e');
		P(' '); P('D'); P('a'); P('r'); P('k'); P('n'); P('e'); P('s');
		P('s'); P(' ');
		break;
	case 10:
		P('E'); P('x'); P('t'); P('r'); P('a'); P(' '); P('L'); P('o');
		P('v'); P('e');
		break;
	default:
		P('T'); P('h'); P('e'); P(' '); P('T'); P('a'); P('n'); P('k');
		P(' '); P('G'); P('i'); P('r'); P('l'); P('\''); P('s'); P(' ');
		P('D'); P('r'); P('e'); P('a'); P('m'); P(' ');
		break;
	}
	P('\0');
	#undef P
}

static void t2_language_main_stage_title_build(int id, shiftjis_t near *p)
{
	#define P(c) (*p++ = static_cast<shiftjis_t>(c))
	switch(id) {
	case 0:
		P('P'); P('u'); P('r'); P('p'); P('l'); P('e'); P(' '); P('F');
		P('i'); P('e'); P('l'); P('d'); P(' '); P(0x81); P(0x60); P(' ');
		P('P'); P('u'); P('r'); P('p'); P('l'); P('e'); P(' '); P('D');
		P('a'); P('w'); P('n');
		break;
	case 1:
		P('R'); P('a'); P('i'); P('j'); P('u'); P('u'); P(' '); P(0x81);
		P(0x60); P(' '); P('M'); P('i'); P('d'); P('n'); P('i'); P('g');
		P('h'); P('t'); P(' '); P('R'); P('a'); P('i'); P('n'); P('s');
		P('t'); P('o'); P('r'); P('m');
		break;
	case 2:
		P('S'); P('c'); P('a'); P('r'); P('l'); P('e'); P('t'); P(' ');
		P('D'); P('r'); P('e'); P('a'); P('m');
		break;
	case 3:
		P('E'); P('v'); P('i'); P('l'); P(' '); P('S'); P('p'); P('i');
		P('r'); P('i'); P('t'); P(' '); P(0x81); P(0x60); P(' '); P('R');
		P('e'); P('v'); P('e'); P('n'); P('g'); P('e'); P('f'); P('u');
		P('l'); P(' '); P('G'); P('h'); P('o'); P('s'); P('t');
		break;
	case 4:
		P('E'); P('a'); P('s'); P('t'); P('e'); P('r'); P('n'); P(' ');
		P('S'); P('e'); P('a'); P('l'); P('i'); P('n'); P('g'); P(' ');
		P('o'); P('f'); P(' '); P('a'); P(' '); P('D'); P('e'); P('m');
		P('o'); P('n'); P(','); P(' '); P('a'); P('n'); P('d'); P('.');
		P('.'); P('.');
		break;
	default:
		P('A'); P(' '); P('D'); P('i'); P('f'); P('f'); P('e'); P('r');
		P('e'); P('n'); P('t'); P(' '); P('S'); P('k'); P('y'); P(' ');
		P(0x81); P(0x60); P(' '); P('f'); P('o'); P('r'); P(' '); P('L');
		P('u'); P('n'); P('a'); P('t'); P('i'); P('c'); P(' '); P('G');
		P('a'); P('m'); P('e'); P('r'); P('s');
		break;
	}
	P('\0');
	#undef P
}

static uint8_t t2_language_main_title_halflen(const shiftjis_t near *text)
{
	uint8_t width = 0;

	while(*text) {
		uint8_t c = static_cast<uint8_t>(*text++);
		width++;
		if(
			((c >= 0x81) && (c <= 0x9F)) ||
			((c >= 0xE0) && (c <= 0xFC))
		) {
			text++;
			width++;
		}
	}
	return ((width + 1) / 2);
}

static void t2_lmp_init(void)
{
	int i;

	if(t2_lmp_ready) {
		return;
	}
	for(i = 0; i < T2_LANGUAGE_MAIN_BGM_COUNT; i++) {
		t2_language_main_jp_bgm_titles[i] = BGM_TITLES[i];
		t2_language_main_bgm_title_build(i, t2_language_main_en_bgm_titles[i]);
	}
	for(i = 0; i < T2_LANGUAGE_MAIN_STAGE_COUNT; i++) {
		t2_language_main_jp_stage_titles[i] = STAGE_TITLES[i];
		t2_language_main_jp_stage_title_halflengths[i] = STAGE_TITLE_HALFLENGTHS[i];
		t2_language_main_stage_title_build(i, t2_language_main_en_stage_titles[i]);
		t2_language_main_en_stage_title_halflengths[i] = (
			t2_language_main_title_halflen(t2_language_main_en_stage_titles[i])
		);
	}
	t2_lmp_ready = true;
}

void far t2_language_main_presentation_apply(bool english_bft_loaded)
{
	int i;

	t2_lmp_init();
	for(i = 0; i < T2_LANGUAGE_MAIN_BGM_COUNT; i++) {
		BGM_TITLES[i] = (
			english_bft_loaded ?
			t2_language_main_en_bgm_titles[i] :
			t2_language_main_jp_bgm_titles[i]
		);
	}
	for(i = 0; i < T2_LANGUAGE_MAIN_STAGE_COUNT; i++) {
		STAGE_TITLES[i] = (
			english_bft_loaded ?
			t2_language_main_en_stage_titles[i] :
			t2_language_main_jp_stage_titles[i]
		);
		STAGE_TITLE_HALFLENGTHS[i] = (
			english_bft_loaded ?
			t2_language_main_en_stage_title_halflengths[i] :
			t2_language_main_jp_stage_title_halflengths[i]
		);
	}
}

#pragma codeseg
