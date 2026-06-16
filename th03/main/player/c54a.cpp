#pragma option -zCPLAYER_M_TEXT -zPmain_01 -G-

#include "th03/main/player/stuff.hpp"

extern "C" void pascal near sub_C568(player_stuff_t near *player);
extern "C" void pascal near sub_C433(player_stuff_t near *player);

extern "C" void pascal near sub_C54A(player_stuff_t near *player)
{
	register player_stuff_t near *p = player;

	if(p->cpu_frame <= p->cpu_safety_frames) {
		sub_C568(p);
	} else {
		sub_C433(p);
	}
}
