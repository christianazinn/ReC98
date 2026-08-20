/// Bullet and gather-circle tile invalidation
/// ------------------------------------------
/// Marks every background tile covered by a live bullet's or gather circle's
/// PREVIOUS position for redrawing, once per frame, ahead of the render pass
/// that draws them at their new one.
///
/// TH04 only, and that is a layout fact rather than a semantic one: the module
/// this replaces is the LAST emitting item of th04_main.asm's contribution to
/// TILE_TEXT, so this object's first function lands exactly where it was (no
/// carve, no new segment name, no group-list edit, no Tupfile.lua line;
/// kb/codegen 0098 + 0105 + 0112 + 0114). In th05_main.asm the same module sits
/// in the MIDDLE of SCORE_TEXT's include list, with two further modules and a
/// proc behind it, so TH05 keeps including the ASM and the module keeps its
/// `if GAME eq 5` arms.
///
/// This file is #included from th04/tile.cpp, ahead of th04/main/tile/tile.cpp,
/// under `#if (GAME != 5)` -- one wrapper serves both games there, and TH05's
/// object must not change.
///
/// It reaches NO header that th04/main/tile/tile.cpp also reaches unguarded.
/// Measured by simulating the guards over both closures: the only headers that
/// expand more than once in the combined translation unit are platform.h,
/// th02/sprites/cels.h and th04/sprites/cels.h, each of which already expands
/// 3-7 times in the object as it builds today, and th02/main/playfld.hpp, which
/// this file does not add to at all. th04/main/tile/tile.hpp is deliberately
/// NOT included: it has no include guard, tile.cpp includes it, and it declares
/// `static const` objects and an `inline` function that a second expansion
/// would reject (kb/codegen/0129). The two things this function needs from it
/// are spelled below instead -- one of them, tiles_invalidate_around(), is a
/// per-translation-unit declaration by that header's own design anyway.

#include "platform.h"
#include "pc98.h"
#include "th01/math/subpixel.hpp"
#include "th02/sprites/bullet16.h"
#include "th02/sprites/pellet.h"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/gather.hpp"

// th04/main/tile/tile.hpp's two declarations, see above.
// ---------------------------------------------------------------------
// Width and height, in screen pixels, of the box around the center passed to
// tiles_invalidate_around(). *Not* the radius.
extern point_t tile_invalidate_box;

// See th04/main/tile/tile.hpp for why this declaration is a per-TU choice.
// This is the SPPoint form, like th04/main/midboss/inv.cpp: the original
// pushes [pos.prev] as one dword rather than as two words.
extern "C" void pascal near tiles_invalidate_around(const SPPoint center);
// ---------------------------------------------------------------------

// A single 32-bit access to both halves of [tile_invalidate_box] at once. The
// original stores a folded constant into it with ONE `mov`, and doubles and
// halves both halves with ONE `shl`/`shr`; Turbo C++ has no store-merging pass,
// so no pair of assignments to .x and .y can produce any of the three. Same
// shape as the set_long() member SPPoint declares in th01/math/subpixel.hpp,
// reached through a cast because point_t is master.lib's plain struct with no
// such member.
//
// MODDERS: This aliases a two-`int` struct as a `uint32_t`. Assign .x and .y
// separately instead; the only thing lost is the original's instruction count.
//
// th04/main/enemy/inv.cpp carries its own copy of the store half. Promoting
// both into th04/main/tile/tile.hpp is the right home and is what
// state/notes/enemies_invalidate.md asks the parcel that lifts this module to
// do -- but that header has no include guard, so a caller can only reach it
// from a translation unit that does not already have it, and six live
// line-anchored citations point above the place a guard would have to go.
// Recorded rather than paid here; see state/notes/bullets_and_gather_invalidate.md.
#define tile_invalidate_box_set(w, h) \
	reinterpret_cast<uint32_t &>(tile_invalidate_box) = ( \
		(w) | (static_cast<uint32_t>(h) << 16) \
	)

// The doubling and halving, on the other hand, CANNOT be written as C++, and
// that is measured rather than assumed. `tcc -S` was given six shapes for a
// 32-bit shift of a global -- `<<=` through a cast reference, through a raw
// pointer, on a plain `extern uint32_t`, on a `volatile` one, `= x << 1`, and
// `*= 2` -- and every one of them emits `mov eax, mem` / `shl eax, 1` /
// `mov mem, eax`, nine bytes where the original has five. The SAME shape on a
// 16-bit global emits `shl word ptr [mem], 1` directly, so the peephole exists
// and is limited by operand size: there is no C++ expression that reaches
// `66 D1 26`. Inline assembly is the only route, the way
// th02/main/tile/tile.cpp already reaches an instruction its C++ cannot.
//
// [measured] One `_asm` statement does NOT cost this function its register
// variables: with these two in place, the pointer still lands in SI and the
// counter in DI, frameless. kb/codegen/0152's note to budget for losing them
// does not hold for an `_asm` that names only globals.
// The `db 66h` is the operand-size prefix, hand-spelled because Turbo C++'s
// inline assembler refuses a dword-sized shift outright -- it reports an
// invalid combination of opcode and operands -- even under -3. The prefix
// upgrades the 16-bit form to the 32-bit one, and the OBJ then carries the
// original's `66 D1 26` / `66 D1 2E` verbatim. Same `_asm { ... }` shape that
// th04/main/entry.cpp's nopcall_same_group() uses.
//
// [measured] `tcc -S` is NOT a control for inline assembly. It printed a
// resolved `[_tile_invalidate_box]` operand and an `extrn` for it, while the
// real `-c` compile of that same source rejected the symbol: spell the C name
// WITHOUT the leading underscore, and grade inline asm with `-c`, never `-S`.
#define tile_invalidate_box_double() _asm { \
	db	66h; \
	shl	word ptr [tile_invalidate_box], 1; \
}

#define tile_invalidate_box_halve() _asm { \
	db	66h; \
	shr	word ptr [tile_invalidate_box], 1; \
}

// No parameters and no stack locals -- the three register variables are pushed
// either way and are not part of the frame -- and the original has no BP frame
// at all, so this one function needs -k- and the restore right after it
// (kb/codegen 0042 + 0149).
#pragma option -k-

void near bullets_and_gather_invalidate(void)
{
	// The two halves are separate blocks so that the bullet pointer's and the
	// gather pointer's lifetimes do not overlap. [measured] Turbo C++ 4.02
	// then gives BOTH of them SI and both counters DI, which is what the
	// original does -- it reloads SI with [gather_circles] and never spills.
	// Three register variables live at once would not fit, and a spilled one
	// would need the stack frame that -k- removes.
	{
		register bullet_t near *bullet;
		register int i;

		bullet = bullets;
		i = BULLET_COUNT;

		// While a bomb or a bullet-clearing item is decaying every bullet on
		// screen, the pellets are drawn at BULLET16 size too, so the whole
		// array takes a single pass with the larger box.
		if(!bullet_zap.active && !bullet_clear_time) {
			tile_invalidate_box_set(PELLET_W, PELLET_H);
			i = PELLET_COUNT;
			do {
				if(bullet->flag != F_FREE) {
					tiles_invalidate_around(bullet->pos.prev);
				}
				bullet++;
			} while(--i);
			i = BULLET16_COUNT;
		}

		tile_invalidate_box_set(BULLET16_W, BULLET16_H);
		do {
			if(bullet->flag != F_FREE) {
				// A grazed bullet is rendered with its graze halo, which is
				// twice as wide and tall.
				if(bullet->spawn_flag > BSF_GRAZED) {
					tile_invalidate_box_double();
					tiles_invalidate_around(bullet->pos.prev);
					tile_invalidate_box_halve();
				} else {
					tiles_invalidate_around(bullet->pos.prev);
				}
			}
			bullet++;
		} while(--i);
	}
	{
		register gather_t near *gather;
		register int i;

		gather = gather_circles;
		i = GATHER_COUNT;
		do {
			if(gather->flag != F_FREE) {
				// The box is the circle's diameter in pixels, plus one
				// GATHER_POINT sprite's worth of margin on each side.
				// Assigned through .x so that the two stores come out in
				// field order with no temporary; a named local would need
				// the stack frame that -k- removes.
				tile_invalidate_box.y = tile_invalidate_box.x = (
					(static_cast<unsigned>(gather->radius_cur.v) >> 3) +
					(GATHER_POINT_W * 2)
				);
				tiles_invalidate_around(gather->center.prev);
			}
			gather++;
		} while(--i);
	}
}

// The `even` that closed th04/main/bullets_gather_inv.asm. The body is 0x95
// bytes -- odd -- so it padded the module to a word boundary with one `nop`,
// and that byte belonged to the dump's contribution. With the module gone it
// has to come from here. A per-function funcdiff over the body reports
// IDENTICAL and stops one byte early; only the map's contribution row shows
// it. (kb/codegen/0111, and state/notes/enemies_invalidate.md, which paid for
// this lesson one parcel earlier.)
#pragma codestring "\x90"

#pragma option -k.
