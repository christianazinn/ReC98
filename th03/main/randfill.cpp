#pragma option -zCHITCIRC_TEXT -zPmain_01 -k-

#include "libs/master.lib/master.hpp"
#include "th02/math/randring.hpp"

void near randring_fill(void)
{
	register int i;

	i = (RANDRING_SIZE - 1);
	do {
		randring[i] = irand();
	} while(--i >= 0);
}
