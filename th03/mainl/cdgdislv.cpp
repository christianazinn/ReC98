#pragma codeseg mainl_03_TEXT group_01

#include "pc98.h"
#include "th03/formats/cdg.h"
#include "x86real.h"

extern bool staffroll_cdg_put_alpha;
extern bool staffroll_cdg_alpha;

extern "C" uint16_t CDG_DISSOLVE_PATTERN[];

extern "C" void pascal near cdg_put_dissolve_e_8(
	screen_x_t center_x, vram_y_t center_y, int slot, int strength
)
{
	int p_offset;
	int w;
	int h;

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
		sub 	center_y, ax
		cmp 	staffroll_cdg_alpha, 0
		jz  	put_noalpha
		cmp 	staffroll_cdg_put_alpha, 0
		jnz 	put_alpha

	put_noalpha:
		push	center_x
		push	center_y
		push	slot
		call	far ptr cdg_put_noalpha_8
		jmp 	dissolve_prepare

	put_alpha:
		push	center_x
		push	center_y
		push	slot
		call	far ptr cdg_put_8

	dissolve_prepare:
		db  	083h, 066h, 004h, 007h
		cmp 	strength, 0
		jz  	dissolve_done
		mov 	bx, 16
		mov 	ax, w
		cwd
		idiv	bx
		mov 	w, ax
		mov 	ax, center_y
		add 	h, ax
		mov 	ax, center_x
		sar 	ax, 3
		mov 	dx, center_y
		shl 	dx, 6
		db  	003h, 0C2h
		mov 	dx, center_y
		shl 	dx, 4
		db  	003h, 0C2h
		mov 	p_offset, ax
		mov 	ax, SEG_PLANE_E
		mov 	es, ax
		mov 	bx, strength
		shl 	bx, 3
		add 	bx, offset CDG_DISSOLVE_PATTERN
		mov 	dx, center_y

	dissolve_next_row:
		db  	089h, 0D6h
		db  	081h, 0E6h, 003h, 000h
		shl 	si, 1
		mov 	ax, [bx+si]
		not 	ax
		mov 	di, p_offset
		mov 	cx, w

	dissolve_apply:
		and 	es:[di], ax
		add 	di, 2
		loop	dissolve_apply
		add 	p_offset, ROW_SIZE
		inc 	dx
		cmp 	dx, h
		jb  	dissolve_next_row

	dissolve_done:
	}
}

#pragma codeseg
