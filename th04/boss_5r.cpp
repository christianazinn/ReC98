// ZUN's object for this code segment held yuuka5_fg_render() followed by
// kurumi_backdrop_colorfill(). (kb/codegen/0112: that an original object held
// several unrelated sources.)
//
// BOTH are C++ now. This comment used to say the second one was "the same
// hand-written shape as" the fill module that opened CIRCLE_TEXT and had to
// stay in the dump; that was wrong twice over, and it stopped a lift for as
// long as it stood. The module it appealed to is th04/hardware/fillm64.cpp
// today, so neither half of the claim survives.
// A GRCG_FILL_PLAYFIELD_ROWS pair whose callee takes its arguments in ES:DI is
// exactly what th04/hardware/grcg.hpp's grcg_fill_playfield_rows_at() macro
// compiles to, register-passed interface and all —
// TH05's byte-identical-apart-from-operands twin had already been matched from
// exactly that macro. It now lives in th04/main/boss/colorfill.cpp, and
// `main_TEXT` is a zero-length segment.
//
// The group pragma lives here rather than in the included file: it only takes
// effect before any code is generated. (kb/codegen/0112, trap 0)
#pragma option -zPmain_01

#include "th04/main/boss/b5r.cpp"
