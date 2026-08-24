/// Uncompressed 16-color 16×16 image format with palette, used for map tiles
/// -------------------------------------------------------------------------
/// -zC/-zP only take effect before any code is generated, so the segment has
/// to be named here rather than in the per-binary wrapper that includes this
/// file. (kb/codegen/0112)
#pragma option -zC_TEXT -zPTEXT -2

#include <dos.h>
#include "decomp.hpp"
#include "planar.h"
#include "th02/formats/tile.hpp"
#include "th02/formats/mpn.hpp"

// One [TILE_W]-pixel line of a single bitplane of a .MPN image.
typedef dots_t(TILE_W) mpn_line_dots_t;

static const int MPN_IMAGE_BITS = 7;

// [_SI] walks the B plane of the blitted image, one line per iteration. The
// other three planes are a whole plane – [TILE_H] lines – further apart each.
#define mpn_line(plane) \
	reinterpret_cast<const mpn_line_dots_t __ds *>(_SI)[TILE_H * plane]

void pascal mpn_put_8(screen_x_t left, vram_y_t top, int image)
{
	static_assert((1 << MPN_IMAGE_BITS) == sizeof(mpn_image_t));

	// The image data is reached through DS, not ES: ES cycles through the four
	// VRAM plane segments inside the loop. Turbo C++ won't save DS for a far
	// pointer on its own, so both halves of this one are spelled out by hand.
	asm { push ds; }

	_DI = top;
	_DI = vram_offset_shift_fast(left, _DI);

	// ZUN landmine: [image] is not restricted to ([mpn_count] + 1); see the
	// declaration in mpn.hpp and tile_area_init_and_put_both().
	_AX = image;
	_AX <<= MPN_IMAGE_BITS;
	_DX = FP_SEG(mpn_images);
	_BX = FP_OFF(mpn_images);
	_BX += _AX;
	asm { mov ds, dx; }
	_SI = _BX;

	_CX = TILE_H;
	line: {
		_ES = SEG_PLANE_B;
		*reinterpret_cast<mpn_line_dots_t __es *>(_DI) = mpn_line(0);
		_ES = SEG_PLANE_R;
		*reinterpret_cast<mpn_line_dots_t __es *>(_DI) = mpn_line(1);
		_ES = SEG_PLANE_G;
		*reinterpret_cast<mpn_line_dots_t __es *>(_DI) = mpn_line(2);
		_ES = SEG_PLANE_E;
		*reinterpret_cast<mpn_line_dots_t __es *>(_DI) = mpn_line(3);
		_DI += ROW_SIZE;
		_SI += (TILE_W / BYTE_DOTS);
		asm { loop line; }
	}

	asm { pop ds; }
}
