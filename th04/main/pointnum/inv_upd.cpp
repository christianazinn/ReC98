/// Point-number initialization, invalidation, and update
/// ----------------------------------------------------
/// Shared by TH04 and TH05. The games only differ in the number of yellow
/// slots and in TH04's extra width and center correction for the "x2" suffix.

#include "platform.h"
#include "x86real.h"
#include "decomp.hpp"
#include "th02/main/entity.hpp"

// Including th04/sprites/pointnum.h here would collide its sprite-cel
// POINTNUM_COUNT with the slot count in pointnum.hpp.
#define POINTNUM_W 8
#define POINTNUM_H 8

#include "th04/main/pointnum/pointnum.hpp"
#include "th04/main/tile/tile.hpp"

// See tile.hpp for the two original parameter-list variants. This module uses
// separate Y and X coordinates, in that order.
extern "C" void pascal near tiles_invalidate_around(
	subpixel_t center_y, subpixel_t center_x
);

#pragma option -k-

void pascal near pointnums_init(void)
{
	pointnum_white_p = 0;
	pointnum_yellow_p = 0;
}

// The included ASM word-aligned the next function with a NOP.
#pragma codestring "\x90"

void near pointnums_invalidate(void)
{
	register pointnum_t near *pointnum;
	register int i;

	tile_invalidate_box.y = POINTNUM_H;
	pointnum = pointnums;
	i = POINTNUM_COUNT;
	do {
		if(pointnum->flag != F_FREE) {
			tile_invalidate_box.x = pointnum->width;
			_AX = pointnum->center_cur.x.v;
#if GAME == 4
			if(pointnum->times_2) {
				_AX += to_sp(POINTNUM_TIMES_2_W / 2);
			}
#endif
			tiles_invalidate_around(pointnum->center_prev_y.v, _AX);
		}
		pointnum++;
	} while(--i);
}

#if GAME == 4
// TH04's extra "x2" branch makes this function odd-sized.
#pragma codestring "\x90"
#endif

void pascal near pointnums_update(void)
{
	register pointnum_t near *pointnum;

	pointnum_first_yellow_alive = 0;
	_BX = FP_OFF(pointnums_alive);
	pointnum = pointnums;
	_DI = POINTNUM_COUNT;

	loop:
		if(pointnum->flag == F_FREE) {
			goto next;
		}
		if(
			reinterpret_cast<unsigned char &>(pointnum->flag) == F_REMOVE
		) {
			pointnum->flag = F_FREE;
			goto next;
		}

		_CL = pointnum->age;
		_AX = pointnum->center_cur.y.v;
		pointnum->center_prev_y.v = _AX;
		if(_CL >= POINTNUM_POPUP_FRAMES) {
			_AX -= to_sp(POINTNUM_POPUP_DISTANCE / POINTNUM_POPUP_FRAMES);
		}
		pointnum->center_cur.y.v = _AX;
		if(static_cast<signed int>(_AX) <= to_sp(-POINTNUM_H / 2)) {
			pointnum->flag = F_REMOVE;
		} else {
			_CL++;
			pointnum->age = _CL;
			if(_CL <= POINTNUM_FRAMES) {
				goto not_remove;
			}

			// A regular store is cross-jumped with the identical store above.
			// Hiding this one from -O also keeps [pointnum] allocated to SI.
			__emit__(0xC6, 0x04, F_REMOVE); // mov byte ptr [si], F_REMOVE
			goto next;

			not_remove: {
				*reinterpret_cast<pointnum_t near *near *>(_BX) = pointnum;
				if(
					(_DI <= POINTNUM_YELLOW_COUNT) &&
					!pointnum_first_yellow_alive
				) {
					pointnum_first_yellow_alive = pointnum;
				}
				_BX += sizeof(pointnum_t near *);
			}
		}

	next:
		pointnum++;
		asm { dec di; jnz loop; }
	*reinterpret_cast<pointnum_t near *near *>(_BX) = 0;
}

#if GAME == 5
// TH05's shorter invalidation body puts this odd-sized function at an even
// address, so the included ASM emitted one final alignment NOP.
#pragma codestring "\x90"
#endif

#pragma option -k.
