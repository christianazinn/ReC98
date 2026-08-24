#include "platform.h"
#include "x86real.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/egc.hpp"
#include "th02/main/playfld.hpp"
#include "th04/main/tile/tile.hpp"
#include "th01/hardware/egc_impl.hpp"

#pragma option -k-

void pascal near tiles_render_all(void)
{
	egc_start_copy_noframe();
	_DI = ((RES_Y - TILE_H) * ROW_SIZE) + PLAYFIELD_VRAM_LEFT;
	_BX = FP_OFF(&tile_ring[TILES_Y - 1][0]);
	_ES = SEG_PLANE_B;

render_row:
	_DL = TILES_X;
render_tile:
	_SI = *reinterpret_cast<vram_offset_t near *>(_BX);
	_CX = TILE_H;
	asm { nop; }
render_line:
	_AX = *reinterpret_cast<uint16_t __es *>(_SI);
	*reinterpret_cast<uint16_t __es *>(_DI) = _AX;
	_SI += ROW_SIZE;
	_DI += ROW_SIZE;
	asm { loop render_line; }
	_DI -= ((TILE_H * ROW_SIZE) - TILE_VRAM_W);
	_BX += sizeof(tile_ring[0][0]);
	asm {
		dec dl;
		jnz render_tile;
	}
	_BX -= ((TILES_MEMORY_X + TILES_X) * sizeof(tile_ring[0][0]));
	_DI -= ((TILE_H * ROW_SIZE) + (TILE_VRAM_W * TILES_X));
	asm { jge render_row; }
	egc_off();
}

void near egc_start_copy_noframe(void)
{
	_outportb_(0x7C, GC_OFF);
	_outportb_(0x6A, 7);
	_outportb_(0x6A, 5);
	_outportb_(0x7C, GC_TDW);
	_outportb_(0x6A, 6);
	egc_setup_copy();
}

#if (GAME == 4)
#pragma codestring "\x90"
#endif
#pragma option -k.
