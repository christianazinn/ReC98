#include "pc98.h"
#include "th03/main/score.hpp"
#include "libs/master.lib/pc98_gfx.hpp"

extern "C" uint16_t near FIVE_DIGIT_POWERS_OF_10[];
extern "C" unsigned char score[];
extern "C" unsigned char temp_lebcd[];
extern "C" void pascal near SUB_D50E(void);

void pascal far score_add(uint16_t score_delta, bool pid)
{
	asm {
		mov 	bx, offset temp_lebcd
		mov 	word ptr [bx+6], 0
		mov 	byte ptr [bx+5], 0

		mov 	si, offset FIVE_DIGIT_POWERS_OF_10
		add 	bx, 4
		mov 	cx, score_delta

	score_to_bcd:
		db  	08Bh, 0C1h
		db  	033h, 0D2h
		div 	word ptr [si]
		db  	08Bh, 0CAh
		mov 	[bx], al
		dec 	bx
		add 	si, 2
		cmp 	word ptr [si], 1
		ja  	short score_to_bcd
		mov 	[bx], cl
		mov 	si, offset score
		mov 	ch, pid
		db  	00Ah, 0EDh
		jz  	short p1
		add 	si, SCORE_DIGITS

	p1:
		mov 	cx, SCORE_DIGITS - 1
		db  	033h, 0C0h

	add_next_digit_to_score:
		mov 	al, [bx]
		add 	al, [si]
		aaa
		mov 	[si], al
		inc 	bx
		inc 	si
		add 	[si], ah
		mov 	ah, 0
		loop	add_next_digit_to_score
		mov 	al, [bx]
		add 	[si], al
	}
}

void pascal far hud_dynamic_5_digit_points_put(
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
		call	near ptr SUB_D50E
		add 	cx, 8
		db  	033h, 0C0h
		call	near ptr SUB_D50E
	}

	grcg_off();
}

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
