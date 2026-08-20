/// Playchar bomb animation blitter
/// -------------------------------
/// One name, one body. TH04 picks the fill color from the playchar first;
/// TH05 has no such branch, which is the only `#if` in the function and the
/// only difference between the two dumps' copies (35 bytes against 19).
///
/// This replaces the hand-written module of the same name, which is the tail
/// `include` of TWO root contributions at once — th04_main.asm's
/// `SHOT_INV_TEXT` and th05_main.asm's `mai_TEXT`, where in BOTH cases it is
/// the *entire* remaining contribution. The C++ object that already follows in
/// each segment #includes this file at its FRONT, so every byte keeps its
/// address: no carve, no new segment name, no group-list edit and no
/// Tupfile.lua line (kb/codegen/0112 + 0114). Both wrappers carry their own
/// `-zC`/`-zP` pragmas rather than delegating them to the file they include,
/// so 0112's trap 0 does not arise on either side.
///
/// ## BOTH games compile this file
///
/// TH05 reaches it through th05/main010.cpp and TH04 through th04/shot_inv.cpp;
/// the hand-written module both dumps used to `include` is gone from each.
///
/// **It was finished as an ADOPTION, never as a second implementation**, and
/// that is the durable half of this note. The campaign has already paid for the
/// alternative once: two lanes wrote rival wirings for enemies_invalidate(), and
/// the two could not be merged into a third arrangement because their include
/// topologies double-defined the body. So the second game's step was three
/// mechanical things — include this file ahead of the segment's other half,
/// replace the dump's `include` with a comment and delete the module, and hand
/// the unguarded th02/v_colors.hpp over from
/// th04/main/player/shots_inv.cpp — and nothing was rewritten.

#if (GAME == 5)
	#include "th05/playchar.h"
#else
	#include "th04/playchar.h"
#endif

// For V_WHITE in the TH04 arm. Unguarded, and NOT in TH05's host object
// closure, so it is listed unconditionally — and it is listed HERE rather than
// in th04/main/player/shots_inv.cpp, which shares TH04's object with this file
// and would otherwise reach the same unguarded header twice.
#include "th02/v_colors.hpp"

// For bb_tiles8_t only. Unguarded, and in neither host's include closure.
#include "th04/formats/bb.h"

// NOT th04/main/tile/bb.hpp, which would be the natural home for the three
// declarations below: it is unguarded AND brings the unguarded
// th02/formats/tile.hpp, which th04/shot_inv.cpp's other half already reaches
// — a collision the TH04 adoption would hit on day one.
// Repeating the declarations is what th04/main/execl.cpp and
// th04/main/player/bombupd.cpp already do for their halves of this same
// subsystem.
//
// [tiles_bb_put_raw] keeps C++ linkage, because the original calls the
// MANGLED `@tiles_bb_put_raw$qi`; bb.hpp's tiles_bb_put() macro declares it
// the same way. [bb_playchar_seg] needs no `extern "C"`: Borland does not
// mangle variable names, and th04/formats/bb_playchar[bss].asm already
// publishes the undecorated `_bb_playchar_seg`.
extern unsigned char tiles_bb_col;
extern bb_tiles8_t __seg *tiles_bb_seg;
extern bb_tiles8_t __seg *bb_playchar_seg;
void pascal near tiles_bb_put_raw(int cel);

// `extern "C"`, and `pascal` with it, because the module this replaces
// published this name undecorated and upper-cased (kb/codegen/0123). The
// prototype the one caller uses is still the local one in
// th04/main/player/bombupd.cpp.
extern "C" void pascal near bb_playchar_put(int cel)
{
	#if (GAME == 4)
		// A ternary rather than an `if`/`else` pair of stores: the original
		// computes the color in AL down both arms and stores it once
		// afterwards, which two separate `mov mem, imm` stores cannot produce.
		//
		// The `2` is left as a literal because it is not V_*-named anywhere in
		// the tree; th02/v_colors.hpp declares only V_WHITE.
		tiles_bb_col = ((playchar == PLAYCHAR_REIMU) ? V_WHITE : 2);
	#endif

	// bb.hpp's tiles_bb_put() macro spelled out, for the reason above.
	tiles_bb_seg = bb_playchar_seg;
	tiles_bb_put_raw(cel);
}
