/* ReC98
 * -----
 * Head half of code segment #1's main_012_TEXT in TH04's MAIN.EXE
 */

// Y6_FG_TEXT is th04_main.asm's own new name for the head of what used to be
// main_012_TEXT's root contribution (kb/codegen/0080): Yuuka's Stage 6
// foreground renderer and the [custom_entities] overlay pass it calls.
// main_012_TEXT keeps the tail -- two `include`d modules and sub_11DE6 -- so
// th04/main_012.cpp, which already owned that segment's C++ contribution, is
// not re-pointed and every byte keeps its address.
//
// The `-zC` rather than a `#pragma codeseg` block inside the included file is
// deliberate, and kb/codegen/0155 is written about exactly this hazard:
// th04/main/boss/bosses.hpp declares yuuka6_fg_render(), and a declaration seen
// before a `codeseg` binds the function to the default segment -- the build
// links and runs, and only the map shows the body hundreds of bytes late. `-zC`
// applies before any code is generated, so it cannot be outrun by a
// declaration.
//
// `-zP` is required and not decorative: both bodies make same-group near calls
// out of this segment (explosions_small_update_and_render(),
// thicklasers_render(), grcg_setmode_rmw()), and a near reference only frames
// on the group base when the object names the group (kb/codegen/0104).
#pragma option -zCY6_FG_TEXT -zPmain_01

#include "th04/main/boss/b6_fg.cpp"
