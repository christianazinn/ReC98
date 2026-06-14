#pragma codeseg mainl_03_TEXT group_01

#include "pc98.h"

void near staffroll_blue_plane_clear(void)
{
	asm {
		db  	057h
		mov 	ax, SEG_PLANE_B
		mov 	es, ax
		db  	031h, 0C0h
		db  	089h, 0C7h
		mov 	cx, (PLANE_SIZE / 2)
		rep 	stosw
		db  	05Fh
	}
}

#pragma codeseg
