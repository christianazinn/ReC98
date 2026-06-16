#pragma option -zCmain_10_TEXT -zPmain_10

#include "platform.h"

extern "C" uint8_t kotohime_chargeshot[];

extern "C" void far sub_1C158(void)
{
	kotohime_chargeshot[0] = 0;
	kotohime_chargeshot[8] = 0;
}
