#pragma option -zCmain_11_TEXT -zPmain_11

#include "platform.h"
#include "th01/math/subpixel.hpp"

extern "C" uint8_t pid_PID_current;
extern "C" uint8_t rikako_chargeshot_frames[];
extern "C" uint8_t rikako_chargeshot_nodes[];
extern "C" Subpixel rikako_chargeshot_origin_x[];
extern "C" Subpixel rikako_chargeshot_origin_y[];
extern "C" uint16_t rikako_chargeshot_radius[];
extern "C" uint8_t rikako_chargeshot_state[];

extern "C" void far sub_1C40A(void)
{
	rikako_chargeshot_state[0] = 0;
	rikako_chargeshot_state[1] = 0;
}

extern "C" void pascal far chargeshot_add_rikako(
	Subpixel center_x, Subpixel center_y
)
{
	_DI = center_x.v;
	rikako_chargeshot_state[pid_PID_current] = 1;
	rikako_chargeshot_frames[pid_PID_current] = 0;
	rikako_chargeshot_radius[pid_PID_current] = 0x80;
	_SI = reinterpret_cast<uint16_t>(
		rikako_chargeshot_nodes + (pid_PID_current * (4 * 6))
	);
	_CX = 0;
	goto node_loop_check;

node_loop:
	*reinterpret_cast<subpixel_t near *>(_SI) = _DI;
	*reinterpret_cast<Subpixel near *>(_SI + 2) = center_y;
	_AL = _CL;
	_AL <<= 6;
	_AL += 0x20;
	reinterpret_cast<uint8_t near *>(_SI)[4] = _AL;
	_CX++;
	_SI += 6;

node_loop_check:
	asm { cmp cx, 4; }
	asm { jl node_loop; }

	rikako_chargeshot_origin_x[pid_PID_current].v = _DI;
	rikako_chargeshot_origin_y[pid_PID_current] = center_y;
}
