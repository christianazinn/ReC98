#pragma option -zCPLAYER_M_TEXT -zPmain_01 -G-

#include "libs/master.lib/pc98_gfx.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/round.hpp"

extern "C" unsigned char byte_20E3D;

extern "C" void near sub_C2F9(void)
{
	register int left;
	register int top;
	int frame_mod_32;

	left = ((byte_20E3D * PLAYFIELD_W_BORDERED) + 96);
	super_put(left, 40, 18);
	super_put((left + 64), 40, 19);

	left = (((1 - byte_20E3D) * PLAYFIELD_W_BORDERED) + 96);
	frame_mod_32 = (round_or_result_frame & 31);
	if(frame_mod_32 < 16) {
		top = (frame_mod_32 + 32);
	} else {
		top = (64 - frame_mod_32);
	}
	super_put(left, top, 20);
	super_put((left + 64), top, 21);
}
