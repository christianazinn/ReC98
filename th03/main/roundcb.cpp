#pragma option -zCPLAYFLD_TEXT -zPmain_01

#include "platform.h"
#include "th03/hardware/input.h"
#include "th03/main/demo.h"

extern nearfunc_t_near fp_1FBC0;

extern uint8_t byte_1FBC3;
extern uint8_t byte_23B00;

extern "C" void pascal near sub_B4A8(void);

extern "C" void near demo_round_update(void)
{
	if(demo_frame < DEMO_N) {
		demo_frame++;
	} else if(demo_frame == DEMO_N) {
		fp_1FBC0 = sub_B4A8;
	}
	if(byte_1FBC3 != 0) {
		byte_23B00 = 1;
	}
	if(input_sp != INPUT_NONE) {
		byte_23B00 = 1;
	}
}

extern "C" void near round_mode_update_none(void)
{
}
