#include "planar.h"
#include "x86real.h"
#include "th04/main/item/splash.hpp"

extern "C" void __fastcall near item_splash_dot_render(
	screen_x_t /* x */, vram_y_t /* vram_y */
)
{
	// __fastcall already placed [x] in AX and [vram_y] in DX. Transforming
	// those registers in place avoids the stack frame emitted for an
	// equivalent vram_offset_mulshift() expression.
	_CX = _AX;
	asm { sar ax, BYTE_BITS; }
	_DX <<= 6;
	_AX += _DX;
	_DX = (static_cast<unsigned int>(_DX) >> 2);
	_AX += _DX;
	_BX = _AX;
	_AL = 0x80;
	_CL &= BYTE_MASK;
	_AL >>= _CL;
	*reinterpret_cast<dots8_t far *>(MK_FP(_ES, _BX)) = _AL;
}
