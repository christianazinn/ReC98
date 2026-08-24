/// Allocating a shot slot
/// ----------------------
/// TH05 only. TH04's function of the same name is a different body: it walks
/// [shot_ptr] through memory and counts [shot_last_id], reloading BX from the
/// global on every iteration. It now compiles from
/// th04/main/player/shots_add.cpp.
///
/// This one keeps the cursor in BX for the whole loop and writes it back to
/// [shot_ptr] only on success, which is why it needs the `_BX`
/// pseudo-register rather than an ordinary local. `[measured 2026-08-23]`
/// `tcc -S` over four ordinary C++ spellings of a loop-carried near pointer --
/// plain, `register`, with SI and DI contended, and this function's exact
/// control flow -- puts it in SI 4/4 and never in BX, with the `push si` /
/// `pop si` that implies. Borland's 16-bit code generator has only SI and DI as
/// register-variable candidates, so BX is not reachable from a declaration; it
/// is reachable from `_BX`, which is a code-generator feature and not inline
/// assembly (`tools/pi-audit/carve_free_tails.py` records the same fact as an
/// ADVISORY signal, because the sparse-`switch` dispatcher emits a BX cursor
/// too).
///
/// The loop is spelled with `goto` on purpose. The `while` form compiles to a
/// rotated loop -- Turbo jumps to a bottom test and falls into the body -- and
/// this function tests the bound at the TOP, before the increment. Same
/// measurement: the `while` spelling came out 0x3B and the `goto` spelling
/// 0x37, which is the original.
///
/// `#pragma option -k-` for the whole object, because the original establishes
/// no frame at all: it has no locals to home and every value it touches is
/// either a register or memory through BX.

#include "th05/main/player/shot.hpp"

// Both merged stores below cover two adjacent members, which is why they go
// through a cast rather than through the struct: Turbo C++ 4.02 has no peephole
// that fuses two byte assignments into one word store, and the original stores
// one word in each case.
//
// [flag] and [age] are adjacent `char`s at offset 0, and [patnum_base] and
// [type] are adjacent `char`s at offset 14. ZUN initialises both pairs
// together, so a shot is born F_ALIVE with age 0 and PAT_SHOT with type 0.
#define SHOT_FLAG_AND_AGE(shot) (*reinterpret_cast<unsigned near *>((shot) + 0))
#define SHOT_PATNUM_AND_TYPE(shot) \
	(*reinterpret_cast<unsigned near *>((shot) + 14))

// [pos].cur and [pos].velocity are both a pair of 16-bit subpixels, and the
// original copies each as one 32-bit unit -- the first from [player_pos].cur
// through EAX, the second from a 32-bit immediate whose halves are
// (velocity.x = 0) and (velocity.y = -12 pixels). A `long` assignment is what
// emits the operand-size prefix; two subpixel assignments emit two word moves.
#define SHOT_POS_CUR(shot) \
	(*reinterpret_cast<unsigned long near *>((shot) + 2))
#define SHOT_POS_VELOCITY(shot) \
	(*reinterpret_cast<unsigned long near *>((shot) + 10))

static const unsigned long SHOT_VELOCITY_UP_12 = 0xFF400000UL;
static const int SHOT_PATNUM_BASE = 20;

// Searches for a free shot slot after [shot_ptr] and returns it, or nullptr if
// there is none. ZUN landmine: The increment happens after the bound check but
// before the flag test. shot_cycle_init(), the cursor's only initializer,
// points at [shots][0], and no path wraps the cursor, so slot 0 is never
// allocated. If the cursor reaches the final slot, the pre-increment check
// also permits the one-past-end element to be read and potentially initialized
// in the 72-byte padding after [shots].
Shot near* near shots_add(void)
{
	_AX = 0;
	_BX = reinterpret_cast<int>(shot_ptr);
loop:
	if(_BX >= reinterpret_cast<int>(&shots[SHOT_COUNT])) {
		goto ret;
	}
	_BX += sizeof(Shot);
	if(reinterpret_cast<Shot near *>(_BX)->flag != F_FREE) {
		goto loop;
	}
	SHOT_FLAG_AND_AGE(_BX) = (F_ALIVE | (0 << 8));
	SHOT_POS_CUR(_BX) = *reinterpret_cast<unsigned long *>(&player_pos.cur);
	SHOT_POS_VELOCITY(_BX) = SHOT_VELOCITY_UP_12;
	SHOT_PATNUM_AND_TYPE(_BX) = SHOT_PATNUM_BASE;
	_AX = _BX;
	_BX += sizeof(Shot);
	shot_ptr = reinterpret_cast<Shot near *>(_BX);
ret:
	return reinterpret_cast<Shot near *>(_AX);
}

// The module's own `even`, which put a single `nop` between this function and
// th04/main/player/shot_velocity.asm. The body is 0x37, so the pad is what
// takes the carved head to the 0x38 that keeps SCORE_TEXT reopening at an even
// address. kb/codegen/0119: a `#pragma option -a2` would not produce it,
// because this object emits no aligned data at all.
#pragma codestring "\x90"
