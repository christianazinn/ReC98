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
extern "C" const char aMaine_1[];
#define MAINE_FN aMaine_1
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
