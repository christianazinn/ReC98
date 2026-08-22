/* ReC98
 * -----
 * Head half of code segment #3's main_036_TEXT in TH05's MAIN.EXE
 */

// BX_TEXT is th05_main.asm's own new name for the head of what used to be
// main_036_TEXT's root contribution (kb/codegen/0080): EX-Alice's fifteen
// uncharacterised movement and pattern bodies, and the phase-transition helper
// below them. main_036_TEXT keeps the middle -- exalice_update() and the three
// jump tables its `switch` statements compile to -- and POINTNUM_TEXT the tail,
// so th05/main_036.cpp, which already owns the middle block's C++
// contribution, is not re-pointed and every byte keeps its address.
//
// This is BX_TEXT's FIRST C++ object. Until it existed the block's own tail was
// an `include`, which is what made every proc in here unliftable: there was no
// C++ contribution for a lift to grow backwards into. There is now, at exactly
// the address the included module used to occupy, so the fifteen procs above
// are ordinary kb/codegen 0099 + 0114 prepends into the front of the file this
// object compiles.
//
// The `-zC` rather than a `#pragma codeseg` block inside the included file is
// deliberate, and kb/codegen/0155 is written about exactly this hazard:
// th05/main/boss/bx.cpp declares exalice_phase_next(), and a declaration seen
// before a `codeseg` binds the function to the default segment -- the build
// links and runs, and only the map shows the body hundreds of bytes late.
// `-zC` applies before any code is generated, so it cannot be outrun by a
// declaration.
//
// `-zP` is required and not decorative: exalice_phase_next() makes same-group
// near calls out of this segment -- boss_explode_small() and boss_items_drop()
// -- and a near reference only frames on the group base when the object names
// the group (kb/codegen/0104).
#pragma option -zCBX_TEXT -zPmain_03

#include "th05/main/boss/bx_updt.cpp"
