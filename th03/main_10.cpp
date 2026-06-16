#pragma option -zCmain_10_TEXT -zPmain_10

#include "platform.h"
#include "th02/snd/snd.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/player/cur.hpp"

extern "C" uint8_t kotohime_chargeshot[];
extern "C" uint16_t word_1FE6A;

extern "C" void far sub_1C158(void)
{
	kotohime_chargeshot[0] = 0;
	kotohime_chargeshot[8] = 0;
}

extern "C" void pascal far chargeshot_add_kotohime(
	Subpixel center_x, Subpixel center_y
)
{
	word_1FE6A = reinterpret_cast<uint16_t>(
		kotohime_chargeshot + (pid.current * 8)
	);
	_BX = word_1FE6A;
	reinterpret_cast<uint8_t near *>(_BX)[0] = 1;
	reinterpret_cast<uint8_t near *>(_BX)[1] = 0;
	*reinterpret_cast<Subpixel near *>(_BX + 2) = center_x;
	*reinterpret_cast<Subpixel near *>(_BX + 4) = center_y;
	*reinterpret_cast<subpixel_t near *>(_BX + 6) = -0x10;
	snd_se_play(6);
}
