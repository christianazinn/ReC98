#pragma option -zCmain_06_TEXT -zPmain_06

#include "th01/math/subpixel.hpp"
#include "th03/main/player/cur.hpp"

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
