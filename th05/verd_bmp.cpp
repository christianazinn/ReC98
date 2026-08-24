/* ReC98
 * -----
 * The verdict screen's VRAM snapshots, in TH05's MAINE.EXE
 */

/// (These two were an `include`d ASM module under th05/end/, deleted by the
/// commit that added this file. It was the LAST and by then the ONLY emitting
/// item in th05_maine.asm's contribution to MAINE_01__TEXT, so with it gone
/// that contribution is ZERO bytes — as is every other CODE contribution that
/// dump makes. th05_maine.asm holds no code at all now, only _DATA and _BSS.
///
/// A NEW object rather than an #include into either neighbour, and the
/// Tupfile.lua line is POSITION-CRITICAL in both directions: the module sat
/// BETWEEN th05/space.cpp's contribution and th05/staffrol.cpp's, so the line
/// has to sit between th05_maine.asm and th05/staffrol.cpp. Listed anywhere
/// else it moves bodies in one of the two neighbours (kb/codegen 0112 + 0114),
/// and the link list already carries a POSITION-CRITICAL comment saying so for
/// the other two objects.
///
/// The three `db` direction pins the module carried are GONE, and that is the
/// point of the move rather than an accident of it. kb/codegen/0037 measured
/// that TASM and Turbo C++'s built-in inline assembler pick OPPOSITE encoding
/// directions for the same instruction: TASM assembles `xor si, si` to 33 F6,
/// the inline assembler to 31 F6. The original has 31 F6, so in a .asm file it
/// needed `db 031h, 0F6h` (parcel MATCH-TH05-DUMP-DIRECTION-BIT-PINS) and here
/// it needs nothing but the ordinary instruction.
///
/// Both functions are one inline-ASM island each, the same shape
/// th03/mainl/cdgunput.cpp uses: every instruction is ZUN's, the parameter is
/// reached by name so its [bp+4] displacement is the compiler's to choose, and
/// SI/DI are mentioned so Turbo C++ emits the `push si` / `push di` prologue
/// the original has (kb/codegen/0050).

#pragma option -zCMAINE_01__TEXT -zPgroup_01

#include "platform.h"
#include "pc98.h"
#include "x86real.h"
#include "planar.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th05/staff.hpp"

/// master.lib's GRCG_OFF_CLOBBERING macro, which spills the port number to DX
/// rather than using the 8-bit immediate form. Spelled the same way
/// th04/end/staff_dissolve.cpp and th04/main/stage/loop.cpp spell it.
#define grcg_off_clobbering_dx() outportb(0x7C, GC_OFF)

extern "C" void pascal near verdict_bitmap_snap(size_t bitmap_offset)
{
	// DS is switched to the E plane for the whole copy and restored at the
	// end. The heap segment is the destination, starting at [bitmap_offset].
	asm {
		push	ds;
		mov 	es, verdict_bitmap;
		mov 	ax, SEG_PLANE_E;
		mov 	ds, ax;
		xor 	si, si;
		mov 	di, bitmap_offset;
		mov 	ax, VERDICT_SCREEN_H;
	}
snap_loop:
	asm {
		mov 	cx, (VERDICT_BITMAP_VRAM_W / 2);
		rep 	movsw;
		add 	si, (ROW_SIZE - VERDICT_BITMAP_VRAM_W);
		dec 	ax;
		jnz 	snap_loop;
		pop 	ds;
	}
}

extern "C" void pascal near verdict_bitmap_put(size_t bitmap_offset)
{
	// ZUN quirk: the clear loop below `rep stosw`s whatever AX happens to
	// hold when grcg_setcolor() returns. In TDW mode the written data is
	// ignored and the tile registers supply the color, so the garbage never
	// reaches VRAM -- but nothing here sets AX, and that is the original's
	// shape rather than a transcription gap.
	grcg_setcolor(GC_TDW, 1);
	asm {
		// The module spelled this segment GRAM_400, which is an ASM-only
		// equate in libs/master.lib/macros.inc with no C++ counterpart. Its
		// value is 0xA800, i.e. SEG_PLANE_B, which is how th03/mainl/stf_bclr.cpp
		// spells the identical instruction pair.
		mov 	ax, SEG_PLANE_B;
		mov 	es, ax;
		xor 	di, di;
		mov 	dx, VERDICT_SCREEN_H;
	}
clear_loop:
	asm {
		mov 	cx, (VERDICT_BITMAP_VRAM_W / 2);
		rep 	stosw;
		add 	di, (ROW_SIZE - VERDICT_BITMAP_VRAM_W);
		dec 	dx;
		jnz 	clear_loop;
	}

	// 13 is the dump's own bare literal and is NOT V_WHITE, which is 15.
	// Nothing in either game names this color, so it stays a literal, the way
	// th02's twelve bare snd_se_play() operands do.
	grcg_setcolor(GC_RMW, 13);
	asm {
		xor 	di, di;
		mov 	si, bitmap_offset;
		mov 	dx, VERDICT_SCREEN_H;
		push	ds;
		mov 	ds, verdict_bitmap;
	}
put_loop:
	asm {
		mov 	cx, (VERDICT_BITMAP_VRAM_W / 2);
		rep 	movsw;
		add 	di, (ROW_SIZE - VERDICT_BITMAP_VRAM_W);
		dec 	dx;
		jnz 	put_loop;
		pop 	ds;
	}
	grcg_off_clobbering_dx();
}
