#pragma option -zCmain_08_TEXT -zPmain_08

#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/player/cur.hpp"

extern "C" uint8_t ellen_chargeshot_nodes[];
extern "C" uint16_t word_1F868;

extern "C" uint16_t far randring_far_next16_raw(void);

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

extern "C" void pascal far chargeshot_add_ellen(
	Subpixel center_x, Subpixel center_y
)
{
	int i;

	word_1F868 = reinterpret_cast<uint16_t>(
		ellen_chargeshot_nodes + (pid.current * (32 * 12))
	);
	i = 0;
	goto node_loop_test;

node_loop:
	_BX = word_1F868;
	_AL = reinterpret_cast<uint8_t near *>(&i)[0];
	reinterpret_cast<uint8_t near *>(_BX)[0] = _AL;
	reinterpret_cast<uint8_t near *>(_BX)[1] = 0;
	*reinterpret_cast<Subpixel near *>(_BX + 4) = center_x;
	*reinterpret_cast<Subpixel near *>(_BX + 6) = center_y;
	_AX = randring_far_next16_raw();
	_BX = word_1F868;
	reinterpret_cast<uint8_t near *>(_BX)[2] = _AL;
	reinterpret_cast<uint8_t near *>(_BX)[3] = 0x50;
	i += 2;
	word_1F868 += 0x0C;

node_loop_test:
	if(i < 0x40) {
		goto node_loop;
	}
}

extern "C" void pascal far ellen_hyper_1B6CA(
	Subpixel center_x, Subpixel center_y
)
{
	register int i;

	word_1F868 = reinterpret_cast<uint16_t>(
		ellen_chargeshot_nodes + (pid.current * (32 * 12))
	);
	i = 0;
	goto node_loop_test;

node_loop:
	_BX = word_1F868;
	if(reinterpret_cast<uint8_t near *>(_BX)[0] != 0) {
		goto next;
	}
	_BX = word_1F868;
	reinterpret_cast<uint8_t near *>(_BX)[0] = 1;
	reinterpret_cast<uint8_t near *>(_BX)[1] = 0;
	*reinterpret_cast<Subpixel near *>(_BX + 4) = center_x;
	*reinterpret_cast<Subpixel near *>(_BX + 6) = center_y;
	_AX = randring_far_next16_raw();
	_BX = word_1F868;
	reinterpret_cast<uint8_t near *>(_BX)[2] = _AL;
	reinterpret_cast<uint8_t near *>(_BX)[3] = 0x50;
	goto ret;

next:
	i++;
	word_1F868 += 0x0C;

node_loop_test:
	if(i < 0x20) {
		goto node_loop;
	}

ret:
}
