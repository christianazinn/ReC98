#pragma option -zCmain_04_TEXT -zPmain_04

#include "th03/math/randring.hpp"

extern uint8_t randring_p;

#pragma option -k-

uint16_t near randring2_next16(void)
{
	__emit__(0x32, 0xFF); // xor bh, bh
	_BL = randring_p;
	_asm { add	bx, offset randring; }
	randring_p++;
	return *reinterpret_cast<uint16_t near *>(_BX);
}

#pragma codestring "\x90"

#pragma option -k.

uint16_t pascal near randring2_next16_and(uint16_t mask)
{
	__emit__(0x32, 0xFF); // xor bh, bh
	_BL = randring_p;
	_asm { add	bx, offset randring; }
	randring_p++;
	return (*reinterpret_cast<uint16_t near *>(_BX) & mask);
}

uint16_t pascal near randring2_next16_mod(uint16_t divisor)
{
	__emit__(0x32, 0xFF); // xor bh, bh
	_BL = randring_p;
	_asm { add	bx, offset randring; }
	randring_p++;
	return (*reinterpret_cast<uint16_t near *>(_BX) % divisor);
}

#pragma option -k-

uint16_t far randring_far_next16(void)
{
	__emit__(0x32, 0xFF); // xor bh, bh
	_BL = randring_p;
	_asm { add	bx, offset randring; }
	randring_p++;
	return *reinterpret_cast<uint16_t near *>(_BX);
}

#pragma codestring "\x90"

#pragma option -k.

uint16_t pascal far randring_far_next16_and(uint16_t mask)
{
	__emit__(0x32, 0xFF); // xor bh, bh
	_BL = randring_p;
	_asm { add	bx, offset randring; }
	randring_p++;
	return (*reinterpret_cast<uint16_t near *>(_BX) & mask);
}

uint16_t pascal far randring_far_next16_mod(uint16_t divisor)
{
	__emit__(0x32, 0xFF); // xor bh, bh
	_BL = randring_p;
	_asm { add	bx, offset randring; }
	randring_p++;
	return (*reinterpret_cast<uint16_t near *>(_BX) % divisor);
}
