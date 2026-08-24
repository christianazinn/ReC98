/// Uncompressed 16-color 16x16 image format with palette, used for map tiles
/// -------------------------------------------------------------------------
#pragma option -zC_TEXT -zPTEXT -2

#include <dos.h>
#include "decomp.hpp"
#include "planar.h"
#include "th02/formats/tile.hpp"
#include "th04/formats/mpn.hpp"

// One [TILE_W]-pixel line of a single bitplane of a .MPN image.
typedef dots_t(TILE_W) mpn_line_dots_t;

static const int MPN_IMAGE_BITS = 7;
static const int MPN_SLOT_BITS = 6;

// [_SI] walks the G plane first and then the E plane. B and R are two and one
// complete planes behind it, respectively.
#define mpn_line(plane) \
	reinterpret_cast<const mpn_line_dots_t __ds *>(_SI)[TILE_H * plane]

#define mpn_slot_at_bx \
	(*reinterpret_cast<const mpn_t near *>( \
		reinterpret_cast<const uint8_t near *>(mpn_slots) + _BX \
	))

void pascal mpn_put_8(screen_x_t left, vram_y_t top, int slot, int image)
{
	static_assert((1 << MPN_IMAGE_BITS) == sizeof(mpn_image_t));
	static_assert((1 << MPN_SLOT_BITS) == sizeof(mpn_t));

	// ES cycles through VRAM planes, so the image source has to live in DS:SI.
	asm { push ds; }

	_DI = vram_offset_shift_fast(left, top);

	_BX = slot;
	_BX <<= MPN_SLOT_BITS;
	_AX = image;
	if(_AX > mpn_slot_at_bx.count) {
		goto done;
	}

	_AX <<= MPN_IMAGE_BITS;
	_SI = _AX;
	_SI += (sizeof(mpn_plane_t) * 2);
	_DX = FP_SEG(mpn_slot_at_bx.images);
	asm { mov ds, dx; }

	_FS = SEG_PLANE_B;
	_GS = SEG_PLANE_R;
	_ES = SEG_PLANE_G;
	_CX = TILE_H;
	color_planes: {
		_AX = mpn_line(-2);
		// Turbo C++'s inline assembler cannot spell FS/GS prefixes, and its
		// pseudoregister dereferences emit the wrong bytes (codegen.hpp).
		__emit__(0x64, 0x89, 0x05); // mov fs:[di], ax
		_AX = mpn_line(-1);
		__emit__(0x65, 0x89, 0x05); // mov gs:[di], ax
		asm { movsw; }
		_DI += (ROW_SIZE - sizeof(mpn_line_dots_t));
		asm { loop color_planes; }
	}

	_DI -= (TILE_H * ROW_SIZE);
	_ES = SEG_PLANE_E;
	_CX = TILE_H;
	alpha_plane: {
		asm { movsw; }
		_DI += (ROW_SIZE - sizeof(mpn_line_dots_t));
		asm { loop alpha_plane; }
	}

	done:
	asm { pop ds; }
}

// The original compiler object ended the odd-length body with one NOP before
// the next word-aligned master.lib object.
#pragma codestring "\x90"

#undef mpn_slot_at_bx
#undef mpn_line
