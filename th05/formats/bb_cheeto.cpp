/// Cheeto bullet sprites: loading
/// ------------------------------
/// TH05 only. Included from th05/bbcheeto.cpp, which binds this function to
/// END_EXT_H_TEXT. The separate hand-written cheeto_put() blitter remains
/// included from th05_main.asm in the immediately following segment.

#include "th04/formats/bb.h"
#include "x86real.h"

extern "C" const char near BB_CHEETO_FN[];
extern "C" bb_tiles8_t near bb_cheeto[BB_SIZE];

#pragma option -k-

extern "C" void pascal near bb_cheeto_load(void)
{
	// DOS file open
	_AX = 0x3D01;
	_DX = FP_OFF(BB_CHEETO_FN);
	geninterrupt(0x21);
	_BX = _AX; // ZUN landmine: No error handling.

	// DOS file read
	_AH = 0x3F;
	_DX = FP_OFF(bb_cheeto);
	_CX = BB_SIZE;
	geninterrupt(0x21);

	// DOS file close
	_AH = 0x3E;
	geninterrupt(0x21);
}
