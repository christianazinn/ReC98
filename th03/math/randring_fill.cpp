#pragma codeseg RANDRING_FILL_TEXT main_01

#include "libs/master.lib/master.hpp"
#include "th02/math/randring.hpp"

#if (GAME >= 4)
extern uint16_t randring_p;
#else
extern uint8_t randring_p;
#endif

#pragma option -k-

void near randring_fill(void)
{
	register int i;

	i = (RANDRING_SIZE - 1);
	do {
		randring[i] = irand();
	} while(--i >= 0);
	randring_p = 0;
}

#pragma option -k.
#pragma codeseg
