/// Extra Stage boss - Evil Eye Σ
/// -----------------------------
/// Her defeat, which is also the last thing MAIN.EXE does on an Extra Stage
/// clear: it commits the run to the resident structure and launches
/// MAINE.EXE over this process. This is the bottom of th02_main.asm's
/// BOSS_5_TEXT contribution, and everything else of hers -- sigma_init(),
/// sigma_update() and her patterns -- is still above it in that dump.
///
/// THIS IS ITS OWN OBJECT, AND THAT IS THE WHOLE POINT OF THE FILE. The
/// obvious host for a BOSS_5_TEXT tail lift is
/// th02/main/enemy/update.cpp next door, which is the C++ object that
/// already picks the segment up at 0FAC:6FA6 -- but that route is parity
/// locked. update.cpp emits enemy_run()'s three generated jump tables under
/// -a2, and Turbo C++'s OBJ writer pads a generated table exactly when its
/// natural OBJECT-LOCAL offset comes out odd, so a prefix of odd length
/// prepended there deletes the pad and shifts every byte after it
/// (kb/codegen/0119, kb/codegen/0096). sigma_end() is 0x45 bytes -- odd --
/// and the two procs above it in the dump do not fix that: +sigma_init()
/// 0xB6 = 0xFB is still odd, and only +sigma_update() 0x227 = 0x322 turns
/// even. So the prepend route cannot take Sigma a proc at a time at all.
///
/// A NEW OBJECT sidesteps all of it, and does so measurably rather than by
/// argument. `[measured 2026-08-21]` Every BOSS_5_TEXT contribution in
/// obj/th02/main.map carries ACBP=28, i.e. BYTE segment alignment, so TLINK
/// inserts nothing between contributions and the odd 0x356B the root now
/// ends at costs nothing. This object emits no generated table and no -a2
/// data of its own, so it has no internal parity to protect either. And
/// because it is exactly the 0x45 bytes the root gave up, both objects after
/// it start at the same segment offsets they did before: update.cpp still at
/// 0FAC:6FA6 with its 0xF13, th02/boss_5.cpp still at 0FAC:7EB9 with its
/// 0x203A and its own pad intact.
///
/// The Tupfile.lua line therefore has to sit BETWEEN th02/dialog.cpp and
/// th02/main/enemy/update.cpp: TLINK lays a segment's contributions out in
/// link order, th02_main.asm is the first object it is handed, so that slot
/// is the 0FAC:6FA6 seam.
///
/// A later lift out of the same block goes at the TOP of this file, and its
/// length is free -- see the paragraph above for why. Do not fold this into
/// th02/main/enemy/update.cpp or th02/boss_5.cpp to save the Tupfile.lua
/// line; both of those have -a2 data whose pads depend on their own object's
/// prefix.

// -zC because the segment name would otherwise come from this file's own
// basename and be B6_TEXT (kb/codegen/0105). -zPmain_03 for the two near calls
// that leave this segment: dialog_pre() is at 0FAC:3320 in DIALOG_TEXT and
// stage_extra_clear_bonus_animate() at 0FAC:0352 in main_03_TEXT, and both are
// only reachable near because BOSS_5_TEXT is in the same group as they are.
// No -a2: `[measured]` nothing here emits a generated jump table, so there is
// no alignment to pin, and pinning one is what would re-roll the segment.
// No -G either: sigma_end() has ZERO locals, so its prolog is a bare
// `push bp; mov bp, sp` under both settings, and -G- is the toolchain default
// this project builds with (kb/codegen/0011 -- derive it from the target's own
// prolog, never from a neighbouring TU; th02/boss_5.cpp next door needs -G for
// mima_update()'s `sub sp, 2` and this file must not copy that).
#pragma option -zCBOSS_5_TEXT -zPmain_03

#include "platform.h"
#include "th02/resident.hpp"
#include "th02/core/globals.hpp"
#include "th02/main/execl.hpp"
#include "th02/main/hiscore.hpp"
#include "th02/main/stage/bonus.hpp"
#include "th02/main/dialog/dialog.hpp"

// th02/main/dialog/dialog.hpp declares every dialog_script_* function but not
// this one, which is how th02/main/boss/b3.cpp, th02/main/boss/b4.cpp and
// th02/main/boss/b5.cpp all already declare it.
void near dialog_pre(void);

// th02/hardware/input.hpp, spelled out here rather than included for the same
// reason th02/main/boss/b5.cpp spells out snd_kaja_interrupt(): that header
// has no include guard and defines fourteen unused `input_t` constants for the
// sake of one call.
void key_delay(void);

// "maine", as it already exists in the root ASM's _DATA. A C++ string literal
// would add a second copy rather than reuse the one this call site owns, so
// the existing label is referenced directly - the same thing
// th02/main/boss/b5.cpp does for `aMaine_0` and th04/main/end.cpp for its own
// four copies. `aMaine` is the dump's own spelling and is not an IDA
// placeholder: PLACEHOLDER_RE's string-auto pattern needs an underscore and a
// letter after it.
extern "C" const char aMaine[];

/// Her defeat, and the end of MAIN.EXE on an Extra Stage clear. Installed as
/// [boss_end] by stage_init() (th02/main/stage/init.cpp), which is why it is
/// `far`.
extern "C" void far sigma_end(void)
{
	dialog_pre();
	dialog_script_generic_part_animate(DS_POSTBOSS);
	stage_extra_clear_bonus_animate();
	key_delay();

	// 127 is STAGE_ALL, from th02/formats/scoredat/scoredat.hpp. That header
	// cannot be included here: it re-includes the unguarded th02/score.h.
	resident->stage = 127;

	// ZUN quirk: The number of continues is *added* to the score rather than
	// stored beside it, which is what makes the resident score a decimal digit
	// wider than the in-game one. MAINE.EXE splits the two apart again. The
	// same line, for the same reason, ends the 5-stage run in
	// th02/main/boss/b5.cpp's mima_19C1D().
	resident->score = ((score * 10) + resident->continues_used);

	scoredat_extra_cleared_set();
	GameExecl(aMaine);
}
