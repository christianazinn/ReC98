#pragma option -zCLANGUAGE_OP_TEXT -zPLANGUAGE_OP_TEXT

#include "shiftjis.hpp"
#include "th03/language.hpp"
#include "th03/lngop.hpp"

extern const shiftjis_t* MUSIC_CHOICES[];

static const shiftjis_t far MUSIC_EN_01[] = "NO.1 Dream Transcending Space...";
static const shiftjis_t far MUSIC_EN_02[] = "NO.2    \x81\x40 \x81\x40Selection          ";
static const shiftjis_t far MUSIC_EN_03[] = "NO.3  Mystic Oriental Love C... ";
static const shiftjis_t far MUSIC_EN_04[] = "NO.4       Reincarnation        ";
static const shiftjis_t far MUSIC_EN_05[] = "NO.5         Dim. Dream         ";
static const shiftjis_t far MUSIC_EN_06[] = "NO.6 Tabula Rasa \x81\x60 The Empty...";
static const shiftjis_t far MUSIC_EN_07[] = "NO.7   \x81\x40 Maniacal Princess     ";
static const shiftjis_t far MUSIC_EN_08[] = "NO.8 Vanishing Dream \x81\x60 Lost... ";
static const shiftjis_t far MUSIC_EN_09[] = "NO.9 Visionary Game \x81\x60 Dream War";
static const shiftjis_t far MUSIC_EN_10[] = "NO.10   Decisive Magic Battle!  ";
static const shiftjis_t far MUSIC_EN_11[] = "NO.11    \x81\x40Sailor of Time       ";
static const shiftjis_t far MUSIC_EN_12[] = "NO.12    Strawberry Crisis!!    ";
static const shiftjis_t far MUSIC_EN_13[] = "NO.13 Non-Unified Magic World...";
static const shiftjis_t far MUSIC_EN_14[] = "NO.14   Love of Magical Chimes  ";
static const shiftjis_t far MUSIC_EN_15[] = "NO.15     Dream of Eternity     ";
static const shiftjis_t far MUSIC_EN_16[] = "NO.16      Eastern Blue Sky     ";
static const shiftjis_t far MUSIC_EN_17[] = "NO.17     Eternal Full Moon     ";
static const shiftjis_t far MUSIC_EN_18[] = "NO.18       Maple Dream...      ";
static const shiftjis_t far MUSIC_EN_19[] = "NO.19 Ghostly Person's Holiday  ";

static const shiftjis_t far * const far MUSIC_EN[] = {
	MUSIC_EN_01, MUSIC_EN_02, MUSIC_EN_03, MUSIC_EN_04, MUSIC_EN_05,
	MUSIC_EN_06, MUSIC_EN_07, MUSIC_EN_08, MUSIC_EN_09, MUSIC_EN_10,
	MUSIC_EN_11, MUSIC_EN_12, MUSIC_EN_13, MUSIC_EN_14, MUSIC_EN_15,
	MUSIC_EN_16, MUSIC_EN_17, MUSIC_EN_18, MUSIC_EN_19,
};
static const shiftjis_t far * far music_japanese[19];
static bool16 far music_japanese_captured;

void far language_op_apply(void)
{
	int i;
	if(!music_japanese_captured) {
		for(i = 0; i < 19; i++) {
			music_japanese[i] = MUSIC_CHOICES[i];
		}
		music_japanese_captured = true;
	}
	for(i = 0; i < 19; i++) {
		MUSIC_CHOICES[i] = (
			language_is_english() ? MUSIC_EN[i] : music_japanese[i]
		);
	}
}

void far language_op_toggle(void)
{
	if(language_is_english()) {
		language_resident_set(LANGUAGE_JAPANESE);
	} else if(language_overlay_available()) {
		language_resident_set(LANGUAGE_ENGLISH);
	} else {
		return;
	}
	language_op_apply();
}

// Keep the following patch and compiler-runtime segments at their accepted phase.
#pragma codestring "\x90\x90"
