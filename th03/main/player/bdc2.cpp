#pragma option -zCPLAYER_M_TEXT -zPmain_01 -G

#include "libs/master.lib/pc98_gfx.hpp"

#pragma warn -par
extern "C" void pascal near sub_BDC2(int x, int y, int patnum, int slot)
{
	asm {
		push	ds;
		mov 	ax, 0A800h;
		mov 	es, ax;
		mov 	cx, [bp+0Ah];
		db  	081h, 0E1h, 007h, 000h; // and cx, 7
		mov 	ax, [bp+0Ah];
		mov 	bx, [bp+8];
		sar 	ax, 3;
		shl 	bx, 6;
		db  	001h, 0D8h; // add ax, bx
		shr 	bx, 2;
		db  	001h, 0D8h; // add ax, bx
		db  	089h, 0C7h; // mov di, ax
		mov 	bx, [bp+6];
		shl 	bx, 1;
		add 	bx, offset super_patdata;
		mov 	ax, [bx];
		mov 	ds, ax;
		mov 	ax, [bp+4];
		shl 	ax, 2;
		add 	ax, 80h;
		db  	089h, 0C6h; // mov si, ax
		mov 	bx, 4;
	}

put_loop:
	asm {
		cmp 	word ptr [bp+0Ah], 0;
		jl  	put_next;
		cmp 	word ptr [bp+0Ah], 624;
		jge 	put_done;
		db  	030h, 0E4h; // xor ah, ah
		mov 	al, [si];
		ror 	ax, cl;
		mov 	es:[di], ax;
	}

put_next:
	asm {
		inc 	si;
		inc 	di;
		add 	word ptr [bp+0Ah], 8;
		dec 	bx;
		jnz 	put_loop;
	}

put_done:
	asm {
		pop 	ds;
	}
}
#pragma warn .par
