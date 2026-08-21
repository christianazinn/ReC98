// The code segment name is left to Turbo C++'s basename default, which makes
// this object contribute to th04_main.asm's main_033_TEXT (kb/codegen/0105).
// That segment has no other contribution, so TLINK -- which lays a segment's
// contributions out in link order, with the root dump first -- puts this one
// at its tail by construction, which is where the function below already was.
// Its Tupfile.lua line is therefore append-anywhere. (kb/codegen/0112 + 0114.)
//
// `-zPmain_03` IS needed here, unlike th04/main_035.cpp: orange_update() has
// two dense `cs:` jump tables, and Turbo C++ frames those on the object's own
// declared group. Without the pragma they come out framed on main_033_TEXT
// itself, which is 0x2063 bytes below the group base, and every table word and
// every `jmp cs:` immediate is wrong. The one near function pointer the body
// stores into a main_01 target -- orange_bg_render() -- takes
// kb/codegen/0162's far declaration instead of dropping the pragma.
#pragma option -zPmain_03


// Neither included file is self-contained, exactly as th04/main_035.cpp's
// three are not: the headers they share live here, because most of them
// have no include guard and this object would otherwise expand them twice
// (kb/codegen/0129).
#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th02/main/player/player.hpp"
#include "th03/hardware/palette.hpp"
#include "th04/snd/snd.h"
#include "th04/sprites/main_pat.h"
#include "th04/math/randring.hpp"
#include "th04/main/bg.hpp"
#include "th04/main/circle.hpp"
#include "th04/main/gather.hpp"
#include "th04/main/frames.h"
#include "th04/main/player/player.hpp"
#include "th04/main/rank.hpp"
#include "th03/math/polar.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/spark.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/tile/bb.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/b2.cpp"

// Address order inside main_033_TEXT, which is what TLINK reproduces from
// the order of these #includes: Kurumi's fight, then Orange's.
#include "th04/main/boss/b2_updt.cpp"
#include "th04/main/boss/b1_updt.cpp"
