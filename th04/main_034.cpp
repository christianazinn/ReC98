// The code segment name is left to Turbo C++'s basename default, which makes
// this object contribute to th04_main.asm's main_034_TEXT (kb/codegen/0105).
// That segment has no other contribution, so TLINK -- which lays a segment's
// contributions out in link order, with the root dump first -- puts this one
// at its tail by construction, which is where the function below already was.
// Its Tupfile.lua line is therefore append-anywhere. (kb/codegen/0112 + 0114.)
//
// `-zPmain_03` for the same reason th04/main_033.cpp gives: elly_update() has
// two dense `cs:` jump tables and one sparse value/jump pair, and Turbo C++
// frames those on the object's own declared group. The one near function
// pointer the body stores into a main_01 target -- elly_bg_render() -- takes
// kb/codegen/0162's far declaration instead of dropping the pragma.
#pragma option -zPmain_03

#include "th04/main/boss/b3_upd.cpp"
