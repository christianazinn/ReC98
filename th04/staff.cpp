// kb/codegen/0138: Turbo C++ 4.0J only accepts `-zP` (and, as this parcel
// measured, `-zC`) while the translation unit has emitted nothing at all —
// which th04/end/staff.cpp's `extern` declarations already violate. So both
// pragmas live in the wrapper, above every #include, exactly as 0138
// prescribes for a head-of-segment lift.
#pragma option -zCMAINE_01_TEXT
#pragma option -zPgroup_01

#include "th04/language_overlay.hpp"
#include "th04/end/staff.cpp"

// This object is the LAST one in TH04's MAINE.EXE link list, and until now it
// emitted nothing at all. Anything given a MAINE_01_TEXT contribution here
// therefore lands immediately after th04_maine.asm's, which is what makes the
// tail of that segment a free lift.
//
// Everything lifted out of that tail is included here in DUMP ORDER, i.e.
// each new lift goes ABOVE the ones already here, because the segment grows
// backwards from verdict_animate() into the hole the deletion leaves.
#include "th04/end/verdict_digits.cpp"
#include "th04/end/verdict_guts.cpp"
#include "th04/end/verdict_stats.cpp"
#include "th04/end/verdict_animate.cpp"
