/// Tile-based .BB animation rendering and invalidation
/// --------------------------------------------------

#include "platform.h"
#include "pc98.h"
#include "x86real.h"
#include "th04/formats/bb.h"
#include "th04/main/playfld.hpp"
#include "th04/main/tile/bb.hpp"
#include "libs/master.lib/pc98_gfx.hpp"

#if (GAME == 5)
	#pragma codeseg END_EXT_B_TEXT main_01
#else
	#pragma codeseg CIRCLE_B_TEXT main_01
#endif

// See tile.hpp for the original packed-structure parameter variant.
extern point_t tile_invalidate_box;
extern "C" void pascal near tiles_invalidate_around(const SPPoint center);

// master.lib's GRCG_OFF_CLOBBERING macro spills the port number to DX.
#define grcg_off_clobbering_dx() outportb(0x7C, GC_OFF)

void pascal near tiles_bb_put_raw(int cel)
{
	uint8_t bb_shiftreg;
	uint8_t row_tiles_left;
	volatile screen_x_t left;
	volatile uvram_y_t top;

	grcg_setcolor(GC_TDW, tiles_bb_col);
	_AX = reinterpret_cast<seg_t>(tiles_bb_seg);
	_FS = _AX;
	_DI = cel;
	_DI <<= 7;
	top = PLAYFIELD_TOP;

row_loop:
		row_tiles_left = TILES_X;
		left = PLAYFIELD_LEFT;

bb_byte_loop:
			// Turbo C++ emits 0x46 instead of the FS prefix 0x64 for memory
			// accesses through the _FS pseudoregister (codegen.hpp).
			__emit__(0x64, 0x8A, 0x05); // mov al, fs:[di]
			bb_shiftreg = _AL;

next_tile_in_row:
				if(bb_shiftreg & 0x80) {
					_AX = left;
					_DX = top;
					#if (GAME == 5)
					if(scroll_active) {
						_DX += scroll_line;
					}
					#else
					_DX += scroll_line;
					#endif
					if(static_cast<vram_y_t>(_DX) >= RES_Y) {
						_DX -= RES_Y;
					}
					grcg_tile_bb_put_8(_AX, _DX);
				}
		bb_shiftreg <<= 1;
		left += TILE_W;
		row_tiles_left--;
		asm { jz short row_next; }
		if(row_tiles_left & 7) {
			goto next_tile_in_row;
		}
		_DI++;
		goto bb_byte_loop;

row_next:
		// The final byte ends without the increment above; the second skipped
		// byte represents the eight unused tiles in this 32-tile-wide format.
		_DI += 2;
		top += TILE_H;
	if(top < PLAYFIELD_BOTTOM) {
		goto row_loop;
	}
	grcg_off_clobbering_dx();
}

#if (GAME == 4)
// The shared ASM module aligned the following function in TH04 only.
#pragma codestring "\x90"
#endif

void pascal near tiles_bb_invalidate_raw(int cel)
{
	uint8_t bb_shiftreg;
	uint8_t row_tiles_remaining;
	SPPoint tile_center;

	reinterpret_cast<uint32_t &>(tile_invalidate_box) = (2 | (2UL << 16));

	// ZUN bloat: Every caller passes [bb_boss_seg], which this body reads
	// directly. The macro's preceding [tiles_bb_seg] store has no effect here.
	_AX = reinterpret_cast<seg_t>(bb_boss_seg);
	_FS = _AX;
	_DI = cel;
	_DI <<= 7;
	tile_center.y.v = TO_SP(TILE_H / 2);

invalidate_row_loop:
		tile_center.x.v = TO_SP(TILE_W / 2);
		row_tiles_remaining = TILES_X;

invalidate_bb_byte_loop:
			__emit__(0x64, 0x8A, 0x05); // mov al, fs:[di]
			bb_shiftreg = _AL;

invalidate_next_tile_in_row:
				// Zero bits expose the stage background and must be redrawn.
				if(bb_shiftreg & 0x80) {
					goto invalidate_skip_tile;
				}
				tiles_invalidate_around(tile_center);

invalidate_skip_tile:
		bb_shiftreg <<= 1;
		tile_center.x.v += TO_SP(TILE_W);
		row_tiles_remaining--;
		asm { jz short invalidate_row_next; }
		if(row_tiles_remaining & 7) {
			goto invalidate_next_tile_in_row;
		}
		_DI++;
		goto invalidate_bb_byte_loop;

invalidate_row_next:
		_DI += 2;
		tile_center.y.v += TO_SP(TILE_H);
	if(static_cast<uint16_t>(tile_center.y.v) < TO_SP(PLAYFIELD_H)) {
		goto invalidate_row_loop;
	}
}

#undef grcg_off_clobbering_dx
