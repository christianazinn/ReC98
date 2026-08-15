/// Single-color playfield fills for boss backdrops
/// -----------------------------------------------
/// The three TH05-only fills that were the entire root contribution to
/// LASER_RH_TEXT. All of them fill whole playfield rows through the GRCG's TDW
/// mode, which is why they go through th04/hardware/grcg_fill_rows.asm rather
/// than any of master.lib's rectangle blitters.
///
/// shinki_stage_backdrop_colorfill() is a [boss_backdrop_colorfill] callback
/// and is therefore called with the GRCG already set up; the other two enable
/// and disable it themselves.

// No -zP here: LASER_RH_TEXT is already a member of group main_01 through
// th05_main.asm's own `group` directive, and a TU that merely contributes to
// an already-grouped segment inherits the membership (kb/codegen/0105).
// th05/laser_rh.cpp sets it for the whole TU instead, because Turbo C++
// rejects -zP once any code has been emitted and this file emits first.

#include "platform.h"
#include "pc98.h"
#include "x86real.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/main/playfld.hpp"
#include "th04/hardware/grcg.hpp"

#pragma option -k-

// Leaves the top 104 and the bottom 72 rows of the playfield filled, keeping
// the 192 rows in between for Shinki's stage backdrop image.
void pascal near shinki_stage_backdrop_colorfill(void)
{
	grcg_fill_playfield_rows_at(  0, 104);
	grcg_fill_playfield_rows_at(296,  72);
}

// Alignment padding between the two functions, in the original.
#pragma codestring "\x90"

/// The hand-rolled GRCG sequences below
/// ------------------------------------
/// Both remaining functions save DI *after* their port writes. Turbo C++ will
/// not do that: any inline-ASM mention of DI, and any use of the _DI
/// pseudoregister, makes it insert its own `PUSH DI` at the very top of the
/// function instead (kb/codegen/0050). So DI is kept entirely invisible to the
/// compiler here — the save, the restore and the `MOV DI, imm16` are all raw
/// bytes, which leaves the port writes ahead of the save where they belong.
///
/// The `XOR AL, AL`s are pinned to the `32 C0` spelling for the same reason
/// kb/codegen/0061 pins `31 C0`: both games' assemblers will otherwise pick the
/// equivalent `30 C0` direction bit and the function comes out the right length
/// but the wrong bytes.

#define PUSH_DI() _asm { db 0x57; }
#define POP_DI()  _asm { db 0x5F; }
#define MOV_DI(imm) __emit__( \
	0xBF, (uint8_t)((imm) & 0xFF), (uint8_t)(((imm) >> 8) & 0xFF) \
)

// GRCG on, in TDW mode, with all four tile registers zeroed. Interrupts are
// restored to their previous state rather than unconditionally reenabled,
// which is why this is neither grcg_setmode_tdw() nor
// grcg_setcolor_direct_constant(0).
#define GRCG_TDW_COL_0_NOINT() _asm { \
	pushf; \
	cli; \
	mov 	al, GC_TDW; \
	out 	0x7C, al; \
	/* grcg_setcolor_direct(0), with all four tile register writes */ \
	/* sharing the single zeroed AL. */ \
	mov 	dx, 0x7E; \
	db  	0x32, 0xC0; /* XOR AL, AL */ \
	out 	dx, al; \
	out 	dx, al; \
	out 	dx, al; \
	out 	dx, al; \
	popf; \
}

// GRCG off, in the four-byte inline spelling the target uses rather than a call
// to master.lib's GRCG_OFF (kb/codegen/0061).
#define GRCG_OFF_INLINE() _asm { \
	db  	0x32, 0xC0; /* XOR AL, AL */ \
	out 	0x7C, al; \
}

// As grcg_fill_playfield_rows_at(), but keeping DI away from the compiler.
#define grcg_fill_playfield_rows_at_hidden_di(y, num_rows) { \
	_ES = (SEG_PLANE_B + ((((y) + PLAYFIELD_TOP) * ROW_SIZE) / 16)); \
	MOV_DI((((num_rows) - 1) * ROW_SIZE) + PLAYFIELD_VRAM_LEFT); \
	grcg_fill_playfield_rows(); \
}

// Clears the entire playfield to color 0.
void near boss_bg_fill_col_0(void)
{
	GRCG_TDW_COL_0_NOINT();
	PUSH_DI();
	grcg_fill_playfield_rows_at_hidden_di(0, PLAYFIELD_H);
	GRCG_OFF_INLINE();
	POP_DI();
}

// Same, but stopping 128 rows short of the bottom, which is where Shinki's
// type D backdrop image goes.
void near shinki_bg_type_d_colorfill(void)
{
	GRCG_TDW_COL_0_NOINT();
	PUSH_DI();
	grcg_fill_playfield_rows_at_hidden_di(0, 240);
	GRCG_OFF_INLINE();
	POP_DI();
}
