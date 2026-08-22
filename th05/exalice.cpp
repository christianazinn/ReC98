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

// exalice_update() joins this object and this SEGMENT, rather than getting a
// `#pragma codeseg main_036_TEXT` and an object of its own, and both halves of
// that are forced rather than chosen.
//
// The same object, because it calls exalice_hittest() above through `push cs` +
// a near `call`: kb/codegen/0116 measures that a same-segment far call is 4
// bytes from inside the caller's own object and 5 -- TLINK's `nop`-padded
// rewrite -- from outside it, and a second object put that `nop` in front of
// all four of those call sites.
//
// The same segment, because `push cs` is only correct when the callee shares
// CS, which Turbo C++ will only assume within one segment. BX_TEXT and
// main_036_TEXT are two names for parts of ONE segment of ZUN's -- the
// kb/codegen/0080 carve created the split -- they are adjacent and byte-aligned
// in group main_03, and main_036_TEXT's root contribution is now empty, so
// every byte still lands at its original address: this object fills
// 0x1E8DA..0x1F6AC and th05/main_036.cpp's midboss5 half picks up at 0x1F6AD.
// The carve's own boundary therefore no longer means anything, and this is the
// arrangement that reproduces the original's call encodings.
//
// bx_upd.cpp also carries NO `#pragma option -a2`: this object's offset for
// exalice_update() is odd, so -a2 would pad each of its three jump tables, and
// the original has no pad in front of any of them.
#include "th05/main/boss/bx_upd.cpp"
