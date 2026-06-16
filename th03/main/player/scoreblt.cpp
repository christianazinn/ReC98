#pragma option -zCPLAYER_M_TEXT -zPmain_01 -G

#include "th03/sprites/score.hpp"

#pragma option -k-
extern "C" void pascal near SUB_D50E(void)
{
	__emit__(0x56);             // push si
	__emit__(0x57);             // push di
	__emit__(0xB4, 0x00);       // mov ah, 0
	__emit__(0xBE);             // mov si, offset sSCORE_FONT
	asm { dw offset sSCORE_FONT; }
	__emit__(0xC1, 0xE0, 0x03); // shl ax, 3
	__emit__(0x03, 0xF0);       // add si, ax
	__emit__(0x8B, 0xC1);       // mov ax, cx
	__emit__(0xC1, 0xF8, 0x03); // sar ax, 3
	__emit__(0x05, 0xE0, 0x01); // add ax, (6 * ROW_SIZE)
	__emit__(0x8B, 0xF8);       // mov di, ax
digit_blit_next:
	__emit__(0xA4);             // movsb
	__emit__(0x83, 0xEF, 0x51); // sub di, (ROW_SIZE + 1)
	asm { jnb short digit_blit_next; }
	__emit__(0x5F);             // pop di
	__emit__(0x5E);             // pop si
}
#pragma option -k.

#pragma codestring "\x90"
