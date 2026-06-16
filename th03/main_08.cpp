#pragma option -zCmain_08_TEXT -zPmain_08

#include "platform.h"

extern "C" uint8_t ellen_chargeshot_nodes[];
extern "C" uint16_t word_1F868;

extern "C" void far sub_1B653(void)
{
	word_1F868 = reinterpret_cast<uint16_t>(ellen_chargeshot_nodes);
	_AX = 0;
	goto node_loop_test;

node_loop:
	_BX = word_1F868;
	reinterpret_cast<uint8_t near *>(_BX)[0] = 0;
	_AX++;
	word_1F868 += 0x0C;

node_loop_test:
	if(static_cast<int>(_AX) < 0x40) {
		goto node_loop;
	}
}
