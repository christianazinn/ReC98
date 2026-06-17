#pragma option -zCHITCIRC_TEXT -zPmain_01

#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th03/hardware/palette.hpp"

extern "C" Palette8 palette_1F2F4;

extern "C" void pascal far SUB_A3A8(uint8_t pid)
{
	_asm {
		mov	ax, ds;
		mov	es, ax;
		db	33h, 0DBh; // xor bx, bx
		mov	al, [bp+6];
		db	0Ah, 0C0h; // or al, al
		jz	short a3a8_pid_0;
		mov	bl, 3;

a3a8_pid_0:
		lea	di, Palettes[bx];
		lea	si, palette_1F2F4[bx];
		movsw;
		movsb;
		mov	palette_changed, true;
	}
}

#pragma codestring "\x90"

extern "C" void pascal far _sub_A3D2(uint8_t pid, uint8_t value)
{
	_asm {
		mov	cx, 3;
		db	33h, 0DBh; // xor bx, bx
		mov	al, [bp+6];
		db	0Ah, 0C0h; // or al, al
		jz	short a3d2_pid_0;
		mov	bl, 3;

a3d2_pid_0:
		lea	di, Palettes[bx];
		mov	al, [bp+8];

a3d2_loop:
		mov	[di], al;
		inc	di;
		loop	a3d2_loop;
		mov	palette_changed, true;
	}
}
