// Stage 4 Boss Reimu's half of main_036_TEXT, as its OWN object, linked
// immediately ahead of th04/main_036.cpp.
//
// WHY A SECOND OBJECT RATHER THAN A SECOND #include IN THAT ONE, and it is
// `[measured]`, not a preference: **the padding in front of the two generated
// switch tables in this segment proves that ZUN compiled the two fights
// separately.** `#pragma option -a2` can only align a table UP to an even
// offset in its own object, so a table that ends up at an ODD object offset is
// unreachable by any alignment setting. Taking the segment as one object,
// based at reimu_1E917 = 0x1E917:
//
//   reimu_1EA4B's table   0x1EB09 -> offset 0x01F2   even, consistent
//   gengetsu_1F903's table 0x1F96A -> offset 0x1053  ODD, impossible
//
// Split at gengetsu_1F8EE = 0x1F8EE, each table is in a different object and
// each one lands where alignment can put it:
//
//   reimu_1EA4B    natural 0x01F1 (odd) -> padded to 0x01F2 in THIS object
//   gengetsu_1F903 natural 0x007B (odd) -> padded to 0x007C in main_036.cpp's
//
// which is exactly the parity th04/main/boss/bx2_upd.cpp's own `-a2` was
// measured at when Gengetsu's chain landed. Merging the two would silently
// drop Gengetsu's pad, and the one-byte-short object is how this was found:
// the instruction-level diff put the first divergence at a table address one
// byte early, 0xbed9 where the original has 0xbeda.
// kb/codegen/0157's "What it licensed" is the same fix for TH02's m4.cpp.
//
// The segment name therefore cannot come from Turbo C++'s basename default
// (kb/codegen/0105) and is named explicitly; `-zPmain_03` for the same reason
// th04/main_036.cpp gives, since reimu_update() has two dense `cs:` tables.
#pragma option -zCmain_036_TEXT -zPmain_03

#include "platform.h"
#include "pc98.h"
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
#include "th04/main/homing.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/player/player.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/tile/bb.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/b4r.hpp"

#include "th04/main/boss/b4r_upd.cpp"
