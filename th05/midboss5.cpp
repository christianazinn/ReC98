// The segment pragma lives here rather than in either included file, because
// it only takes effect before any code is generated. (kb/codegen/0112, trap 0)
#pragma option -zCmain_0_TEXT

// EX-Alice's custom-bullet callback was the ONLY proc of th05_main.asm's
// main_0_TEXT root contribution, which the dump now contributes zero bytes
// to, so it belongs at the very front of this object (kb/codegen/0114).
#include "th05/main/boss/bx_custombullets.cpp"
#include "th05/main/boss/bx_fg.cpp"
#include "th05/main/midboss/m5.cpp"
