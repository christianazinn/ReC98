#pragma codeseg HITBOX_TEXT

#include "platform.h"
#include "th02/snd/snd.h"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/stuff.hpp"

extern "C" uint8_t byte_1FDEA;
extern "C" uint8_t byte_1FE1C;
extern "C" uint8_t near *word_1FE4E;

extern "C" void far sub_142D0(void)
{
	byte_1FDEA = 0;
	byte_1FE1C = 0;
}

extern "C" void pascal far chargeshot_add_marisa(
	Subpixel center_x, Subpixel center_y
)
{
	register int i;

	word_1FE4E = (&byte_1FDEA + (pid.current * 0x32));
	word_1FE4E[0] = 1;
	word_1FE4E[1] = 0;
	players[pid.current].shot_active = SA_DISABLED;
	i = 0;
	while(i < 12) {
		reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i] = center_x;
		reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[i].v = (
			center_y.v - 0x100
		);
		i++;
	}
	snd_se_play(6);
}
