#pragma codeseg RANDRING_FILL_TEXT main_01

#include "libs/master.lib/master.hpp"
#include "th02/math/randring.hpp"

void near randring_fill(void)
{
	register int i;

	i = 0;
	do {
		randring[i] = irand();
		i++;
	} while(i < RANDRING_SIZE);
}
