#pragma codeseg mainl_03_TEXT group_01

#include "pc98.h"
#include "th03/formats/cdg.h"
#include "x86real.h"

extern "C" void pascal near cdg_unput_for_upwards_motion_e_8(
	screen_x_t center_x, vram_y_t center_y, int slot
)
{
	int w;
	int h;

	_DI = _DI;
	asm {
		mov 	bx, slot
		shl 	bx, 4
		mov 	ax, word ptr cdg_slots[bx+2]
		mov 	w, ax
		mov 	bx, slot
		shl 	bx, 4
		mov 	ax, word ptr cdg_slots[bx+4]
		mov 	h, ax
		mov 	ax, w
		cwd
		db  	02Bh, 0C2h
		sar 	ax, 1
		sub 	center_x, ax
		mov 	ax, h
		cwd
		db  	02Bh, 0C2h
		sar 	ax, 1
		add 	ax, -2
		add 	center_y, ax
		mov 	ax, center_x
		sar 	ax, 3
		mov 	dx, center_y
		shl 	dx, 6
		db  	003h, 0C2h
		mov 	dx, center_y
		shl 	dx, 4
		db  	003h, 0C2h
		db  	08Bh, 0F8h
		mov 	ax, SEG_PLANE_E
		mov 	es, ax
		mov 	dx, w
		shr 	dx, 4
		mov 	si, ROW_SIZE
		db  	029h, 0D6h
		db  	029h, 0D6h
		db  	031h, 0C0h
		db  	089h, 0D1h
		rep 	stosw
		db  	001h, 0F7h
		db  	089h, 0D1h
		rep 	stosw
		db  	001h, 0F7h
		db  	089h, 0D1h
		rep 	stosw
	}
}

#pragma codeseg
