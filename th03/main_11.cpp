#pragma option -zCmain_11_TEXT -zPmain_11

#include "platform.h"

extern "C" uint8_t rikako_chargeshot_state[];

extern "C" void far sub_1C40A(void)
{
	rikako_chargeshot_state[0] = 0;
	rikako_chargeshot_state[1] = 0;
}
