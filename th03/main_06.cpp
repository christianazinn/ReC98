#pragma option -zCmain_06_TEXT -zPmain_06

#include "th01/math/subpixel.hpp"
#include "th03/main/difficul.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/playfld.hpp"
#include "th03/math/randring.hpp"
#include "th03/math/vector.hpp"

extern "C" uint8_t exatt_buffers[];
extern "C" uint16_t word_2028A;

extern "C" uint8_t near sub_1A1A7(void)
{
	register uint8_t near *slot;

	slot = reinterpret_cast<uint8_t near *>(word_2028A);
	_DX = *reinterpret_cast<subpixel_t near *>(slot + 2);
	_DX += *reinterpret_cast<subpixel_t near *>(slot + 6);
	_BX = word_2028A;
	if(reinterpret_cast<uint8_t near *>(_BX)[0x10] != 0) {
		goto other_side;
	}
	if(
		*reinterpret_cast<subpixel_t near *>(slot + 0x0C) >
		static_cast<int>(_DX)
	) {
		goto move;
	}

done:
	*reinterpret_cast<subpixel_t near *>(slot + 2) = (
		*reinterpret_cast<subpixel_t near *>(slot + 0x0A)
	);
	slot[0] = 3;
	_AL = 1;
	_AL -= pid_current;
	slot[0x10] = _AL;
	_AL = 1;
	goto ret;

other_side:
	if(
		*reinterpret_cast<subpixel_t near *>(slot + 0x0C) >=
		static_cast<int>(_DX)
	) {
		goto done;
	}

move:
	*reinterpret_cast<subpixel_t near *>(slot + 2) = _DX;
	*reinterpret_cast<subpixel_t near *>(slot + 4) += (
		*reinterpret_cast<subpixel_t near *>(slot + 8)
	);
	_AL = 0;

ret:
	return _AL;
}

extern "C" void pascal near sub_1A1ED(
	subpixel_t x,
	subpixel_t y1,
	subpixel_t target_x,
	subpixel_t target_y,
	pid_t pid,
	int velocity_base
)
{
	register uint8_t near *slot;
	register screen_x_t target_x_screen;

	target_x_screen = target_x;
	slot = reinterpret_cast<uint8_t near *>(word_2028A);
	slot[0] = 2;
	slot[1] = 0;
	*reinterpret_cast<subpixel_t near *>(slot + 2) = x;
	*reinterpret_cast<subpixel_t near *>(slot + 4) = y1;
	slot[0x10] = pid;
	x = playfield_fg_x_to_screen(x, pid);
	*reinterpret_cast<subpixel_t near *>(slot + 0x0A) = target_x_screen;
	target_x_screen = playfield_fg_x_to_screen(target_x_screen, (1 - pid));
	vector2_between_plus(
		(x << 4),
		y1,
		(target_x_screen << 4),
		target_y,
		0,
		*reinterpret_cast<int near *>(slot + 6),
		*reinterpret_cast<int near *>(slot + 8),
		(velocity_base + (static_cast<int>(round_speed) / 4))
	);
	*reinterpret_cast<subpixel_t near *>(slot + 0x0C) = screen_x_to_playfield(
		target_x_screen, pid
	);
}

extern "C" void pascal far exatt_add_reimu(
	subpixel_t center_x, subpixel_t center_y, pid_t pid
)
{
	register uint8_t near *slot;
	register int i;

	_AL = pid;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	slot = reinterpret_cast<uint8_t near *>(_AX);

	i = 0;
	goto loop_test;
loop:
	if(slot[0] == 0) {
		word_2028A = reinterpret_cast<uint16_t>(slot);
		sub_1A1ED(
			center_x,
			center_y,
			randring_far_next16_mod(PLAYFIELD_W << 4),
			randring_far_next16_and(2047),
			pid,
			0x5A
		);
		return;
	}
	i++;
	slot += 0x20;
loop_test:
	if(i < 8) {
		goto loop;
	}
}
