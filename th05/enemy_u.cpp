/* ReC98
 * -----
 * Enemy updating, at the front of code segment #3's 2nd part in TH05's
 * MAIN.EXE
 */

// enemies_update() was the whole of th05_main.asm's remaining contribution to
// main_032_TEXT, so its bytes are the FIRST ones in that segment. TLINK lays a
// segment's contributions out in link order with the root dump first, and the
// root's contribution is ZERO bytes now, so this object only lands on the
// segment start if its Tupfile.lua line sits ABOVE th05/gather.cpp's -- which
// it does. (kb/codegen 0112 + 0114)
//
// A NEW object rather than an #include at the front of th05/gather.cpp, and
// that is a measured choice, not a stylistic one. Hoisting this body's headers
// to the front of gather.cpp changes the codegen of two functions FURTHER DOWN
// that same translation unit: th01/math/subpixel.hpp on its own is enough to
// turn boss_explode_small()'s switch dispatch from
// `mov ax,[bp+4]` / `dec ax` / `mov bx,ax` into `mov bx,[bp+4]` / `dec bx`,
// two bytes shorter. That header is GUARDED and was already in the object --
// only the position of its first expansion changed. An empty #include in the
// same slot leaves all 17 pre-existing procs byte-identical, so the trigger is
// the header and not the edit. A separate translation unit cannot perturb
// another one at all, which is the whole reason this route is cheaper than
// auditing which header may be hoisted past which function.
//
// The segment has to be named explicitly: the basename default would open an
// ENEMY_U_TEXT of its own (kb/codegen/0105), the same reason th05/main031.cpp
// gives. `-zP` is here because this object DOES need the group -- the
// hand-spelled `call near ptr sparks_add_random` in the body reaches
// SPARK_A_TEXT, a different segment of main_03, and th05/bullet_u.cpp carries
// the identical island and the identical pragma for the identical reason.
// The group pragma lives here rather than in the included file: it only takes
// effect before any code is generated (kb/codegen/0112 trap 0;
// kb/codegen/0138).
#pragma option -zCmain_032_TEXT -zPmain_03

#include "th05/main/enemy/update.cpp"
