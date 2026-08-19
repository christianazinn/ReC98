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
/// ## Only TH05 compiles this file today, and the TH04 half is PRE-WIRED
///
/// th04_main.asm's SHOT_INV_TEXT region is held by another lane's open
/// parcel claim, so TH04 still `include`s the module
/// and this body is currently reached only through th05/main010.cpp. Nothing
/// here is TH05-shaped: the `#if (GAME == 4)` arm below is the TH04 body,
/// already written and already correct.
///
/// **Finish it as an ADOPTION, never as a second implementation.** The
/// campaign has already paid for the alternative once: two lanes wrote rival
/// wirings for enemies_invalidate(), and the two could not be merged into a
/// third arrangement because their include topologies double-defined the body.
/// The remaining TH04 step is exactly three things:
///
/// 1. `#include "th04/main/player/bb_playchar_put.cpp"` at the top of
///    th04/shot_inv.cpp, ahead of th04/main/player/shots_inv.cpp;
/// 2. replace `include th04/main/player/bb_playchar_put.asm` in
///    th04_main.asm's `SHOT_INV_TEXT` with a one-line comment, and delete the
///    module;
/// 3. remove `#include "th02/v_colors.hpp"` from
///    th04/main/player/shots_inv.cpp and update the "pulled in exactly once
///    from here" comment there — that header is unguarded, this file takes it
///    over, and TH04's object is the only one where both are present.

#if (GAME == 5)
	#include "th05/playchar.h"
#else
	#include "th04/playchar.h"
#endif

// For V_WHITE in the TH04 arm. Unguarded, and NOT in TH05's host object
// closure, so it is listed unconditionally — but see step 3 above: the day
// TH04 adopts this file, that game's object reaches it twice and
// th04/main/player/shots_inv.cpp has to stop including it.
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
