#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"

extern "C" uint16_t near FIVE_DIGIT_POWERS_OF_10[];
extern "C" void pascal near SUB_D50E(void);

extern "C" void pascal far sub_D668(
	screen_x_t left, vram_y_t top, uint16_t points, vc_t col
)
{
	grcg_setcolor(GC_RMW, col);

	_AX = top;
	_DX = _AX;
	_AX <<= 2;
	_AX += _DX;
	_AX += (SEG_PLANE_B + ((1 * ROW_SIZE) / 16));
	_ES = _AX;
	_CX = left;
	_SI = FP_OFF(FIVE_DIGIT_POWERS_OF_10);
	_DI = points;
	_BL = 0;

	asm {
		nop

	digit_loop:
		db  	08Bh, 0C7h
		db  	033h, 0D2h
		div 	word ptr [si]
		db  	08Bh, 0FAh
		db  	00Ah, 0D8h
		jz  	short digit_next
		call	near ptr SUB_D50E

	digit_next:
		add 	cx, 8
		add 	si, 2
		cmp 	word ptr [si], 1
		ja  	short digit_loop
		db  	08Bh, 0C7h
		db  	00Ah, 0D8h
		jz  	short digits_done
		call	near ptr SUB_D50E

	digits_done:
	}
}

#pragma codestring "\x90"
