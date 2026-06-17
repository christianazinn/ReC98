#pragma option -zCPLAYER_M_TEXT -zPmain_01 -G

#include "x86real.h"

#pragma warn -par
extern "C" void pascal near sub_BE2A(int x, int y)
{
	asm {
		mov 	ax, [bp+6];
		mov 	bx, [bp+4];
		sar 	ax, 7;
		shr 	bx, 5;
		shl 	bx, 6;
		db  	001h, 0D8h; // add ax, bx
		shr 	bx, 2;
		db  	001h, 0D8h; // add ax, bx
		db  	089h, 0C7h; // mov di, ax
	}
	asm {
		mov 	cx, [bp+6];
		shr 	cx, 4;
		db  	081h, 0E1h, 007h, 000h; // and cx, 7
	}
	_BX = 0xC0;
	asm {
		ror 	bx, cl;
		mov 	es:[di], bx;
	}
}
#pragma warn .par
