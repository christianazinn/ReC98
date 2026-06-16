#pragma option -zCmain_09_TEXT -zPmain_09

#include "platform.h"

extern "C" uint8_t kana_chargeshot_state[];

extern "C" void far sub_1BC4D(void)
{
	kana_chargeshot_state[0] = 0;
	kana_chargeshot_state[1] = 0;
}
