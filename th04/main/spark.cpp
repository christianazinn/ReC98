/// Spark rendering and lifecycle
/// -----------------------------

#include "platform.h"
#include "pc98.h"
#include "x86real.h"
#include "th02/sprites/sparks.h"
#include "th04/hardware/grcg.hpp"
#include "th04/main/spark.hpp"
#include "th04/main/tile/tile.hpp"
#include "libs/master.lib/master.hpp"

// See tile.hpp for the original packed-structure parameter variant.
extern "C" void pascal near tiles_invalidate_around(const SPPoint center);

// AX and DX are Turbo C++'s two representable __fastcall parameters. The
// sprite ID is a third register parameter in CL, outside that ABI.
extern "C" void __fastcall near spark_render(screen_x_t, vram_y_t)
{
	_SI = _AX;
	_BX = _DX;
	asm { sar ax, 3; }
	asm { shl dx, 6; }
	_AX += _DX;
	asm { shr dx, 2; }
	_AX += _DX;
	_DI = _AX;
	_SI &= BYTE_MASK;
	_AX = _SI;
	_SI <<= 7;
	_SI += reinterpret_cast<uint16_t>(sSPARKS);
	_CX &= (SPARK_CELS - 1);
	_CX <<= 4;
	_SI += _CX;

	if(static_cast<unsigned int>(_BX) <= (RES_Y - SPARK_H)) {
		_CX = SPARK_H;
		_BX ^= _BX;
		goto blit_loop;
	}
	_CX = RES_Y;
	_CX -= _BX;
	_BX = SPARK_H;
	_BX -= _CX;

blit_loop:
	asm { lodsw; }
	if(_AH != 0) {
		*reinterpret_cast<dots16_t __es *>(_DI) = _AX;
		goto next_row;
	}
	if(_AL != 0) {
		*reinterpret_cast<dots8_t __es *>(_DI) = _AL;
	}

next_row:
	_DI += ROW_SIZE;
	asm { loop blit_loop; }
	if(_BX == 0) {
		return;
	}
	_DI -= PLANE_SIZE;
	asm { xchg bx, cx; }
	goto blit_loop;
}

#pragma codestring "\x90"

extern "C" void near sparks_update(void)
{
	register spark_t near *spark;
	register int i;

	i = SPARK_COUNT;
	spark = sparks;
	do {
		if(spark->flag == F_FREE) {
			goto next;
		}
		if(spark->flag != F_ALIVE) {
			spark->flag = F_FREE;
			goto next;
		}

		spark->center.update_seg1();
		_AX += TO_SP(SPARK_W / 2);
		if(static_cast<unsigned int>(_AX) >= TO_SP(PLAYFIELD_W + SPARK_W)) {
			goto remove;
		}
		_DX += TO_SP(SPARK_H / 2);
		if(static_cast<unsigned int>(_DX) < TO_SP(PLAYFIELD_H + SPARK_H)) {
			goto age;
		}
	remove:
		spark->flag = F_REMOVE;
		goto next;

	age:
		spark->center.velocity.y.v++;
		spark->age++;
		if(spark->age > 40) {
			// Keep this store distinct from the identical one above. Otherwise,
			// -O cross-jumps both branches into one shared assignment.
			__emit__(0xC6, 0x04, F_REMOVE); // mov byte ptr [si], F_REMOVE
		}

	next:
		spark++;
	} while(--i > 0);
}

extern "C" void near sparks_render(void)
{
	register spark_t near *spark;
	register int i;

	grcg_setcolor_direct(12);
	_ES = SEG_PLANE_B;
	i = SPARK_COUNT;
	spark = sparks;
	do {
		if(spark->flag == F_ALIVE) {
			_AX = spark->center.cur.y.v;
			_AX += TO_SP(PLAYFIELD_TOP - (SPARK_H / 2));
			_AX = scroll_subpixel_y_to_vram_seg1(_AX);
			_DX = _AX;
			_AX = spark->center.cur.x.v;
			_AX += TO_SP(PLAYFIELD_LEFT - (SPARK_W / 2));
			asm { sar ax, 4; }
			_CL = spark->age;
			spark_render(_AX, _DX);
		}
		spark++;
	} while(--i > 0);
}

#pragma codestring "\x90"

void near sparks_invalidate(void)
{
	register spark_t near *spark;
	register int i;

	reinterpret_cast<uint32_t &>(tile_invalidate_box) = (
		SPARK_H | (static_cast<uint32_t>(SPARK_W) << 16)
	);
	i = SPARK_COUNT_TH04;
	spark = sparks;
	do {
		if(spark->flag != F_FREE) {
			tiles_invalidate_around(spark->center.prev);
		}
		spark++;
	} while(--i > 0);
}

extern "C" void near sparks_init(void)
{
	register spark_t near *spark;
	register int i;

	spark = sparks;
	i = SPARK_COUNT_TH04;
	do {
		reinterpret_cast<uint8_t &>(spark->angle) = irand();
		spark++;
	} while(--i != 0);
	reinterpret_cast<uint8_t &>(spark_ring_offset) = 0;
}
