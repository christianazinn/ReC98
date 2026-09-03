#pragma option -zCLANGUAGE_MAIN_TEXT -zPLANGUAGE_MAIN_TEXT

#include "th03/language.hpp"
#include "th03/langmain.hpp"

extern "C" char near aGAUGE_ATTACK_TIMES[];
extern "C" char near aBOSS_ATTACK_TIMES[];
extern "C" char near aBOSS_REVERSAL_TIMES[];
extern "C" char near aBOSS_PANIC_TIMES[];
extern "C" char near aPLAYER_REM[];

static const char far RESULT_GAUGE_EN[] =
	"\x82\x66\x82\x60\x82\x74\x82\x66\x82\x64\x81\x40"
	"\x82\x60\x82\x73\x82\x6A\x81\x40\x81\x40\x81\x40\x81\x7E";
static const char far RESULT_BOSS_EN[] =
	"\x82\x61\x82\x6E\x82\x72\x82\x72\x81\x40"
	"\x82\x60\x82\x73\x82\x6A\x81\x40\x81\x40\x81\x40\x81\x40\x81\x7E";
static const char far RESULT_FLIP_EN[] =
	"\x82\x61\x82\x6E\x82\x72\x82\x72\x81\x40"
	"\x82\x65\x82\x6B\x82\x68\x82\x6F\x81\x40\x81\x40\x81\x40\x81\x7E";
static const char far RESULT_PANIC_EN[] =
	"\x82\x61\x82\x6E\x82\x72\x82\x72\x81\x40"
	"\x82\x6F\x82\x60\x82\x6D\x82\x68\x82\x62\x81\x40\x81\x40\x81\x7E";
static const char far RESULT_LIVES_EN[] =
	"\x82\x6B\x82\x68\x82\x75\x82\x64\x82\x72\x81\x40"
	"\x82\x6B\x82\x64\x82\x65\x82\x73\x81\x40\x81\x40\x81\x7E";

static void language_main_copy(char near *dst, const char far *src)
{
	do {
		*dst = *src;
		dst++;
	} while(*src++ != '\0');
}

void far language_main_apply(void)
{
	if(!language_is_english()) {
		return;
	}
	language_main_copy(aGAUGE_ATTACK_TIMES, RESULT_GAUGE_EN);
	language_main_copy(aBOSS_ATTACK_TIMES, RESULT_BOSS_EN);
	language_main_copy(aBOSS_REVERSAL_TIMES, RESULT_FLIP_EN);
	language_main_copy(aBOSS_PANIC_TIMES, RESULT_PANIC_EN);
	language_main_copy(aPLAYER_REM, RESULT_LIVES_EN);
}

// Keep the following score and compiler-runtime segments at their accepted phase.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90"
