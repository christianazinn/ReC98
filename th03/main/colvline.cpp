#pragma option -zCCOLLMAP_TEXT -zPmain_04 -G -k-

#include "decomp.hpp"
#include "platform/x86real/flags.hpp"
#include "th01/math/subpixel.hpp"
#include "th03/main/collmap.hpp"
#include "th03/main/playfld.hpp"

void collmap_set_vline()
{
	#define pattern          	static_cast<uint8_t>(_AH)
	#define first_bit        	static_cast<uint8_t>(_CL)
	#define first_bit_wide   	static_cast<uint16_t>(_CX)
	#define rows_from_bottom 	_DX
	#define collmap_p        	reinterpret_cast<uint8_t near *>(_BX)

	_AX = collmap_topleft.x.v;
	_DX = collmap_topleft.y.v;

	__emit__(0x0B); __emit__(0xC0); // or ax, ax
	if(FLAGS_SIGN) {
		goto ret;
	}
	if(static_cast<int16_t>(_AX) >= (PLAYFIELD_W << SUBPIXEL_BITS)) {
		goto ret;
	}
	__emit__(0x0B); __emit__(0xD2); // or dx, dx
	if(FLAGS_SIGN) {
		goto ret;
	}
	if(static_cast<int16_t>(_DX) >= (PLAYFIELD_H << SUBPIXEL_BITS)) {
		goto ret;
	}

	_BX = 0;
	if(collmap_pid != 0) {
		_BX = COLLMAP_SIZE;
	}
	_asm { add	bx, offset collmap; }

	_asm { sar	ax, (SUBPIXEL_BITS + COLLMAP_TILE_W_BITS); }
	_asm { sar	dx, (SUBPIXEL_BITS + COLLMAP_TILE_H_BITS); }

	first_bit_wide = _AX;
	_asm { sar	ax, 3; }
	first_bit_wide &= (8 - 1);
	_BX += _DX;
	_asm {
		mov	ah, COLLMAP_H;
		mul	ah;
	}
	_BX += _AX;

	_AX = COLLMAP_H;
	_AX -= _DX;
	rows_from_bottom = _AX;
	pattern = 0x80;
	pattern >>= first_bit;
	_CX = collmap_tile_h.v;
	__emit__(0x90);

row_loop:
	*collmap_p |= pattern;
	_BX++;
	rows_from_bottom--;
	asm { loopne	row_loop; }

ret:
	#undef collmap_p
	#undef rows_from_bottom
	#undef first_bit_wide
	#undef first_bit
	#undef pattern
}

#pragma codestring "\x90"
