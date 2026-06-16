#pragma option -zCmain_10_TEXT -zPmain_10

#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th02/snd/snd.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/sprite16.hpp"

extern "C" uint8_t kotohime_chargeshot[];
extern "C" uint8_t pid_PID_so_attack;
extern "C" uint16_t word_1FE6A;

struct player_stuff_t {
	uint8_t unused_0[0x18];
	uint16_t gauge_charged;
	uint8_t unused_1[0x66];
};

extern player_stuff_t players[PLAYER_COUNT];

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

extern "C" void pascal far chargeshot_update_kotohime(void)
{
	word_1FE6A = reinterpret_cast<uint16_t>(
		kotohime_chargeshot + (pid_current * 8)
	);
	_BX = word_1FE6A;
	if(reinterpret_cast<uint8_t near *>(_BX)[0] == 0) {
		goto ret;
	}
	players[pid_current].gauge_charged = 0;
	_BX = word_1FE6A;
	_AX = *reinterpret_cast<subpixel_t near *>(_BX + 6);
	asm { add [bx+4], ax; }
	if(*reinterpret_cast<subpixel_t near *>(_BX + 4) > -0x100) {
		goto accelerate;
	}
	reinterpret_cast<uint8_t near *>(_BX)[0] = 0;
	return;

accelerate:
	_BX = word_1FE6A;
	*reinterpret_cast<subpixel_t near *>(_BX + 6) -= 2;

ret:
}

#pragma warn -aus
extern "C" void near kotohime_chargeshot_1C1E9(void)
{
	screen_x_t left;
	screen_y_t top;
	sprite16_offset_t sprite_offset;

	sprite_offset = (
		pid_PID_so_attack + ((56 * ROW_SIZE) + (64 / BYTE_DOTS))
	);
	_BX = word_1FE6A;
	left = (playfield_fg_x_to_screen(
		*reinterpret_cast<subpixel_t near *>(_BX + 2),
		pid_current
	) - 48);
	_BX = word_1FE6A;
	_AX = *reinterpret_cast<subpixel_t near *>(_BX + 4);
	asm { sar ax, 4; }
	_AX += 8;
	top = _AX;
	sprite16_put(left, _AX, sprite_offset);
}
