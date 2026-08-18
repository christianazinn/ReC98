/// Scrolling the tile ring
/// -----------------------
/// Advances the map cursor by one tile row whenever [scroll_line] has crossed
/// into a new one, refills the [tile_ring] row it exposed from [map_seg], and
/// then EGC-copies the lines that scrolled in since the VRAM page currently
/// being drawn was last rendered.
///
/// (TH04: #included from th04/main/tile/mpn_load.cpp, which is itself
/// #included from the th04/map.cpp object wrapper. That is kb/codegen/0129's
/// host-source form, not kb/codegen/0112's wrapper form, and the count that
/// decides it is 2: of the headers this file needs that th04/map.cpp's
/// translation unit already provides — th04/main/tile/tile.hpp and
/// th04/formats/map.hpp — *both* are unguarded, so including either here
/// would break the TU. This form costs 0 header edits. Including this file
/// from the wrapper instead would have needed a guard on tile.hpp, or
/// mpn_load.cpp's #include moved up into the wrapper.
///
/// TH05 puts the same function in a different segment — STD_TEXT rather than
/// END_TEXT — so it is #included from th05/formats/std.cpp instead, ahead of
/// std_load(). Same kb/codegen/0129 host-source form, and the host provides
/// th04/main/tile/tile.hpp and th04/main/stage/stage.hpp, both unguarded.
/// TH05's copy was th05_main.asm's sub_BD20 (a placeholder name that no
/// longer exists in that dump).
///
/// Because this file shares a translation unit with mpn_load.cpp and
/// th04/formats/map.cpp, its file-scope names are NOT file-local. The one
/// macro below is #undef'd at the end.

#include "x86real.h"
// egc_off(); the host TU only has it on the TH04 side. Guarded, so the
// #include is a no-op there.
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/egc.hpp"
#include "th04/formats/std.hpp"
#include "th04/main/scroll.hpp"

// th04/formats/map.hpp is included further down this same translation unit, by
// th04/formats/map.cpp, and it has no include guard (kb/codegen/0112 trap 2).
// Only [map_seg]'s segment is needed here, and a repeated `extern` declaration
// is legal where a repeated definition is not — so forward-declare the pointee
// and re-declare the pointer with exactly map.hpp's type.
struct map_section_tiles_t;
extern map_section_tiles_t __seg* map_seg;

// Indexes [TILE_SECTION_OFFSETS] with a byte offset, the way the original
// does: the section ID is doubled in place rather than scaled by the index.
#define TILE_SECTION_OFFSETS_bytewise \
	reinterpret_cast<const uint8_t __ds *>(TILE_SECTION_OFFSETS)

void near tiles_scroll_and_egc_render(void)
{
	pixel_length_8_t lines_last_frame;

	if((scroll_lines_pending == 0) && (scroll_lines_prev_frame == 0)) {
		return;
	}
	if(scroll_speed == 0) {
		return;
	}

	// Kept in AX all the way down to the [tile_ring] row calculation below.
	_AX = scroll_line;
	_AX >>= TILE_BITS_H;

	if(_AX != tile_ring_row_filled) {
		tile_ring_row_filled = _AX;

		// Through BX rather than `_ES = FP_SEG(std_seg)`, which Turbo C++
		// lowers to a 4-byte `MOV ES, [std_seg]` — measured, 2 bytes short of
		// the original's `MOV BX, [std_seg]` + `MOV ES, BX`.
		_BX = FP_SEG(std_seg);
		_ES = _BX;

		// kb/codegen/0031: Turbo C++ widens a `signed char` before comparing
		// it, so `if(--tile_row_in_section < 0)` compiles to a
		// load/DEC/store/CBW/OR/JGE sequence instead of the original's fused
		// DEC on memory. Pin the decrement and its branch, and nothing else.
		asm {
			dec 	byte ptr tile_row_in_section;
			jns 	short tile_row_still_in_section;
		}
		{
			tile_row_in_section = (TILE_ROWS_PER_SECTION - 1);
			#if (GAME == 5)
				std_map_section_p++;
			#else
				std_map_section_id++;
			#endif
			std_scroll_speed++;
			_DL = *reinterpret_cast<subpixel_length_8_t __es *>(
				std_scroll_speed
			);
			scroll_speed.v = _DL;

			// End of the map: stop scrolling, and drop the lines that were
			// still pending. Reaching the boss is the caller's business.
			if(_DL == 0) {
				scroll_line = 0;
				scroll_lines_prev_frame = 0;
				scroll_lines_pending = 0;
				return;
			}
		}
tile_row_still_in_section:

		/// Refill the newly exposed [tile_ring] row from [map_seg].
		/// -------------------------------------------------------
		static_assert(sizeof(tile_ring[0]) == 64);
		// kb/codegen/0037: every register-to-register instruction in this block
		// is spelled in the assembler direction in the original, and Turbo C++
		// emits the compiler direction for the equivalent pseudo-register
		// assignment. Ordinary inline ASM gives the target with no byte pins.
		_AX <<= 6;
		_AX += FP_OFF(tile_ring);
		asm { mov	di, ax; }	// pseudo-register form: `8B F8`, target `89 C7`

		asm { xor	ax, ax; }	// pseudo-register form: `33 C0`, target `31 C0`
		_AL = tile_row_in_section;
		_AX <<= 6;

		#if (GAME == 5)
			_BX = std_map_section_p;
		#else
			_BX = std_map_section_id;
		#endif
		_BL = *reinterpret_cast<uint8_t __es *>(_BX);

		// `_BH = 0` emits a 2-byte `MOV BH, 0`: the same length as the
		// original's `XOR BH, BH`, but a different instruction, and not one of
		// the encoding pairs mzdiff --semantic treats as equal. Assigning the
		// byte into the whole of BX to make Turbo C++ zero-extend is worse
		// still, and that one was measured: it routes the load through AX
		// (`MOV AL, ES:[BX]` / `MOV AH, 0` / `MOV BX, AX`), which is 2 bytes
		// longer *and* wrong, because AX is live here.
		//
		// Self-XOR and self-add pseudo-register forms do give the right two
		// instructions, but in the compiler direction (`32 FF` / `02 DB`)
		// against the original's `30 FF` / `00 DB`. Inline ASM settles the
		// encoding as well as the instruction.
		asm { xor 	bh, bh; }

		// TH05 stores the section ID pre-doubled inside [std_seg]
		// (th04/formats/std.hpp), so only TH04 has to scale it here.
		#if (GAME != 5)
			asm { add 	bl, bl; }
		#endif

		_BX = *reinterpret_cast<const uint16_t near *>(
			&TILE_SECTION_OFFSETS_bytewise[_BX]
		);

		asm {
			mov 	si, ax;	// pseudo-register form: `8B F0`, target `89 C6`
			add 	si, bx;	// pseudo-register form: `03 F3`, target `01 DE`
		}

		asm {
			push	ds;
			pop 	es;
			push	ds;
		}
		_AX = FP_SEG(map_seg);
		asm { mov ds, ax; }
		_CX = TILES_X;
		asm {
			rep movsw;
			pop	ds;
		}
		/// -------------------------------------------------------
	}

	// The page being drawn was last drawn two frames ago, so it also missed
	// the previous frame's scroll and has to catch up on both.
	lines_last_frame = scroll_lines_prev_frame;
	scroll_lines_prev_frame = scroll_lines_pending;
	scroll_lines_pending += lines_last_frame;

	#if (GAME == 5)
		// Stage 6 (Shinki) scrolls no tiles: th05_main.asm's sub_10214, the
		// only caller, clears [scroll_active] on that stage two instructions
		// into its own body, immediately before it ends up here. So the test
		// below would already catch it, and this one looks redundant --
		// [inferred], not measured: [scroll_active] has other writers
		// (th04/main/player/bomb.cpp raises it again), and nothing rules out
		// one of them running on Stage 6 between the two tests.
		if(stage_id == 5) {
			scroll_lines_pending = 0;
			return;
		}
	#endif
	if(scroll_active == false) {
		scroll_lines_pending = 0;
		return;
	}
	egc_start_copy_noframe();
	tiles_egc_copy_scrolled_lines();
	scroll_lines_pending = 0;
	egc_off();
}

#undef TILE_SECTION_OFFSETS_bytewise
