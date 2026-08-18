/// The verdict screen's entry point
/// --------------------------------
/// Shows `ude.pi`, fades it in, and hands over to the screen body, which is
/// still ASM. `[measured]` This was the LAST proc of th04_maine.asm's
/// MAINE_01_TEXT contribution, and th04/staff.cpp is the LAST object in TH04's
/// MAINE.EXE link list — an otherwise declarations-only translation unit whose
/// only trace in the map was an empty, auto-named `STAFF_TEXT` segment. Giving
/// it a `-zCMAINE_01_TEXT -zPgroup_01` body therefore appends to that segment
/// immediately behind the dump's contribution, i.e. exactly at `0A05:20A8`:
/// a tail lift with no carve, no new segment, no group-list edit and no
/// Tupfile.lua line. It is the mirror of the head lift that
/// th05/regist.cpp — also a declarations-only TU in the right slot — gave the
/// glyph ball parcel, and it is the position every further TH04 MAINE lift
/// should now come out of, because MAINE_01_TEXT's tail is the only free end
/// this dump has left.
/// kb/codegen/0138: the `-zCMAINE_01_TEXT` / `-zPgroup_01` pair cannot live
/// here — th04/end/staff.cpp's declarations precede this file in the same
/// translation unit and Turbo C++ rejects both options once anything has been
/// seen. They sit at the top of the th04/staff.cpp wrapper instead.

#include "th02/hardware/frmdelay.h"
/// th04/hardware/input.h has no include guard, and th04/end/verdict_stats.cpp
/// — which now precedes this file in the same translation unit — already needs
/// it. Including it again is 15 "initialized more than once" errors, so this
/// file relies on its host, which is the idiom for every other .cpp fragment
/// in the chain.
#include "th03/formats/pi.hpp"
#include "th04/end/verdict.hpp"
#include "libs/master.lib/pc98_gfx.hpp"

/// `[measured]` `ude.pi` stays a `_DATA` byte of the root dump rather than
/// becoming a literal of this translation unit, for the reason
/// th04/hiscore/regist_menu.cpp documents at length for `hi01.pi`: the build
/// compiles with `-d`, and moving a string out of the dump's `_DATA`
/// contribution shifts every following byte of it. Published there as
/// `_ude_pi` (kb/codegen/0123), next to `_hi01_pi` and `_scnum2_bft`.
extern const char ude_pi[];

/// The body of the verdict screen: twelve labels and their values, the [skill]
/// computation and its clamp, the `_ude.txt` verdict line, then the wait and
/// the fade-out. Now C++ too, at `0A05:1B31` — th04/end/verdict_stats.cpp,
/// which th04/staff.cpp includes immediately ahead of this file so that the
/// two contributions stay in dump order.
extern "C" void near verdict_stats_put_and_wait(void);

void near verdict_animate(void)
{
	palette_settone(0);
	graph_accesspage(1);
	pi_load(0, ude_pi);
	pi_palette_apply(0);
	pi_put_8(0, 0, 0);
	pi_free(0);
	graph_copy_page(0);
	palette_black_in(4);
	verdict_stats_put_and_wait();
}
