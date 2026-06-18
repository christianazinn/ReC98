#pragma option -WX -zCSHARED -k-

#include "codegen.hpp"
#include "th03/formats/hfliplut.h"

extern "C" void hflip_lut_generate(void)
{
	// TCC would add its own DI save/restore if the body mentioned DI.
	__emit__(0x57);       // push di
	__emit__(0x33, 0xC0); // xor ax, ax
	__emit__(0xBF);       // mov di, offset hflip_lut
	asm { dw offset hflip_lut; }
	__emit__(0x32, 0xD2); // xor dl, dl
	__emit__(0xEB, 0x0B); // jmp short set_and_loop

	__emit__(0x32, 0xD2);             // xor dl, dl
	__emit__(0xB9, 0x08, 0x00);       // mov cx, 8
	__emit__(0xD0, 0xC0);             // rol al, 1
	__emit__(0xD0, 0xDA);             // rcr dl, 1
	__emit__(0xE2, 0xFA);             // loop generation_loop
	__emit__(0x88, 0x15);             // mov [di], dl
	__emit__(0x47);                   // inc di
	__emit__(0xFE, 0xC0);             // inc al
	__emit__(0x75, 0xEE);             // jnz short permutation_loop
	__emit__(0x5F);                   // pop di
}
#pragma option -k.
