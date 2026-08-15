/// Ending launcher
/// ---------------
/// One name, one body. The TH04 and TH05 regions are textually identical apart
/// from which of the dump's `'maine'` copies they point at.

#pragma option -zPmain_01

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/main/execl.hpp"
#include "th04/snd/snd.h"
#include "th04/end/end.h"
#if (GAME == 5)
#include "th05/resident.hpp"
#else
#include "th04/resident.hpp"
#endif
#include "th04/main/end.hpp"

// "maine", as it already exists in the root ASM's _DATA. Each binary carries
// one copy of the string per GameExecl("maine") call site, at consecutive
// 6-byte offsets — four in TH04, three in TH05. A C++ string literal here
// would add a further copy rather than reuse the one this function owns, so
// the existing label is referenced directly, the same thing
// th03/main/entry.cpp does for aOp and arg0.
#if (GAME == 5)
extern "C" const char aMaine_0[];
#define MAINE_FN aMaine_0
#else
extern "C" const char aMaine[];
extern "C" const char aMaine_0[];
extern "C" const char aMaine_1[];
#define MAINE_FN aMaine_1
#endif

#if (GAME != 5)
/// Ending selection
/// ----------------
/// One shape, two bodies, and they are the same source with two constants
/// swapped: the two 43-byte regions of the original differ in exactly 5 bytes
/// (`kb/codegen/0115`), which are the [end_sequence] value, the
/// [end_type_ascii] digit, and the `'maine'` copy each one launches through.
/// TH05 has neither — its single end_game() sets no [end_type_ascii], because
/// TH05's resident structure has no such field.

void end_game_good(void)
{
	resident->end_sequence = ES_GOOD;
	resident->end_type_ascii = '0';
	snd_kaja_func(KAJA_SONG_FADE, 4);
	palette_black_out(16);

	// Same linker-relaxed far call as end_extra() below. (kb/codegen 0014)
	_asm {
		push	ds;
		push	offset aMaine;
		nop;
		push	cs;
		call	near ptr GameExecl;
	}
}

void end_game_bad(void)
{
	resident->end_sequence = ES_BAD;
	resident->end_type_ascii = '1';
	snd_kaja_func(KAJA_SONG_FADE, 4);
	palette_black_out(16);

	_asm {
		push	ds;
		push	offset aMaine_0;
		nop;
		push	cs;
		call	near ptr GameExecl;
	}
}
/// ----------------
#endif

void end_extra(void)
{
	resident->end_sequence = ES_EXTRA;
	snd_kaja_func(KAJA_SONG_FADE, 4);
	palette_black_out(16);

	// The original reaches GameExecl() through the linker-relaxed
	// `nop; push cs; call near ptr` form, which no plain C++ far call
	// reproduces even within one group. Upstream's th04/main/demo.hpp carries
	// the identical workaround, with a TODO saying the same thing.
	// (kb/codegen 0014)
	_asm {
		push	ds;
		push	offset MAINE_FN;
		nop;
		push	cs;
		call	near ptr GameExecl;
	}
}
