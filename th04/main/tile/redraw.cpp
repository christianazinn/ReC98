#include "platform.h"
#include "x86real.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/egc.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/tile/tile.hpp"

extern bool halftiles_dirty[TILE_FLAGS_Y][TILES_MEMORY_X];
extern vram_offset_t tile_ring[TILES_Y][TILES_MEMORY_X];

#pragma option -k-

void pascal near tiles_redraw_invalidated(void)
{
	egc_start_copy_noframe();
	_ES = SEG_PLANE_B;
	_BX = FP_OFF(&halftiles_dirty[TILE_FLAGS_Y - 1][0]);
	_DI = ((RES_Y - TILE_FLAG_H) * ROW_SIZE) + PLAYFIELD_VRAM_LEFT;
	_DH = TILE_FLAGS_Y;
	_SI = (TILES_MEMORY_X * (TILES_Y - 1) * sizeof(tile_ring[0][0]));

start_row:
	_DL = TILES_X;

dirty:
	if(*reinterpret_cast<uint8_t near *>(_BX) != 0) {
		asm { push si; }
		*reinterpret_cast<uint8_t near *>(_BX) = 0;
		_SI = *reinterpret_cast<vram_offset_t near *>(
			FP_OFF(tile_ring) + _SI
		);
		asm {
			test dh, 2;
			jnz redraw;
		}
		_SI += (TILE_FLAG_H * ROW_SIZE);
redraw:
		_CX = TILE_FLAG_H;

blit_tile_redraw_lines:
		_AX = *reinterpret_cast<uint16_t __es *>(_SI);
		*reinterpret_cast<uint16_t __es *>(_DI) = _AX;
		_SI += ROW_SIZE;
		_DI += ROW_SIZE;
		asm { loop blit_tile_redraw_lines; }
		_DI -= (TILE_FLAG_H * ROW_SIZE);
		asm { pop si; }
	}

	_DI += sizeof(tile_ring[0][0]);
	_SI += sizeof(tile_ring[0][0]);
	_BX++;
	asm {
		dec dl;
		jnz dirty;
	}
	asm {
		test dh, 1;
		jnz previous_row;
	}
	_SI += (TILES_MEMORY_X * sizeof(tile_ring[0][0]));
previous_row:
	_SI -= (
		(TILES_MEMORY_X * sizeof(tile_ring[0][0])) +
		(TILES_X * sizeof(tile_ring[0][0]))
	);
	asm { dec dh; }
	_BX -= (TILES_X + TILES_MEMORY_X);
	_DI -= (
		(TILES_X * sizeof(tile_ring[0][0])) +
		(TILE_FLAG_H * ROW_SIZE)
	);
	asm { jge start_row; }
	egc_off();
}

#pragma option -k.
