#pragma option -zCHITCIRC_TEXT -zPmain_01

#include "libs/master.lib/master.hpp"
#include "th02/math/randring.hpp"

extern uint8_t randring_p;

#pragma option -k-

void near randring_fill(void)
{
	register int i;

	i = (RANDRING_SIZE - 1);
	do {
		randring[i] = irand();
	} while(--i >= 0);
}

uint16_t near randring1_next16(void)
{
	__emit__(0x32, 0xFF); // xor bh, bh
	_BL = randring_p;
	_asm { add	bx, offset randring; }
	randring_p++;
	return *reinterpret_cast<uint16_t near *>(_BX);
}

#pragma codestring "\x90"

#pragma option -k.

uint16_t pascal near randring1_next16_and(uint16_t mask)
{
	__emit__(0x32, 0xFF); // xor bh, bh
	_BL = randring_p;
	_asm { add	bx, offset randring; }
	randring_p++;
	return (*reinterpret_cast<uint16_t near *>(_BX) & mask);
}
