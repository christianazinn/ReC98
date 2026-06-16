#pragma option -zCmain_07_TEXT -zPmain_07

#include "platform.h"

extern "C" uint8_t chiyuri_chargeshot_nodes[];

extern "C" void far sub_1B260(void)
{
	register uint8_t near *node;

	node = chiyuri_chargeshot_nodes;
	_AX = 0;
	goto node_loop_test;

node_loop:
	node[0] = 0;
	_AX++;

node_loop_test:
	if(static_cast<int>(_AX) < 16) {
		goto node_loop;
	}
}
