/// Extra Stage boss - Evil Eye Σ
/// -----------------------------
/// Her fight-init and her defeat, in dump order. The defeat is also the last
/// thing MAIN.EXE does on an Extra Stage clear: it commits the run to the
/// resident structure and launches MAINE.EXE over this process. This is the
/// bottom of th02_main.asm's BOSS_5_TEXT contribution, and the rest of her --
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
/// inserts nothing between contributions and the odd length the root now
/// ends at costs nothing. This object emits no generated table and no -a2
/// data of its own, so it has no internal parity to protect either. And
/// because it is exactly the bytes the root gave up, both objects after
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
///
/// `[measured 2026-08-22]` sigma_init() is the second lift to take that
/// paragraph up, and it settles "free" with a number instead of an argument:
/// obj_probe.py on the built obj/th02/b6.obj reported SEGDEF lengths
/// `0x45 0x0 0x0` before it, i.e. this object contributes to _TEXT and to
/// NOTHING ELSE -- no _DATA, no _BSS, no generated table. There is therefore
/// no parity ladder here at any depth, and the ladder quoted for the
/// update.cpp route above is that object's, not this segment's and not
/// Sigma's. Re-derive it from the HOST object's own PUBDEF/SEGDEF records
/// (kb/codegen/0160) every time the host changes; a ladder copied out of an
/// older note is a running sum of bodies that have already been lifted
/// somewhere else.

// -zC because the segment name would otherwise come from this file's own
// basename and be B6_TEXT (kb/codegen/0105). -zPmain_03 for the near calls
// that leave this segment: dialog_pre() is at 0FAC:3320 in DIALOG_TEXT and
// stage_extra_clear_bonus_animate() at 0FAC:0352 in main_03_TEXT, and
// sigma_init() adds boss_playfield_reset() in main_03__TEXT, three more
// DIALOG_TEXT entries and the bullets_clear() island in BULLET_TEXT -- all of
// them only reachable near because BOSS_5_TEXT is in the same group as they
// are.
// No -a2: `[measured]` nothing here emits a generated jump table, so there is
// no alignment to pin, and pinning one is what would re-roll the segment.
// No -G either: sigma_end() and sigma_init() both have ZERO locals, so both
// prologs are a bare
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
// APPENDED, not merged into the run above, and deliberately so: reordering an
// include is not a free edit in this codebase (kb/codegen/0011's neighbour
// trap), and sigma_end() below was already byte-exact against this exact
// prefix. Everything here is needed by sigma_init() only.
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/tile/tile.hpp"
#include "th02/main/boss/boss.hpp"

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

// "stage5b1.bft" and "stage5b2.bft", the two 64x64 halves Sigma is drawn as,
// and "boss5.m", the song her fight plays -- all three as they already exist
// in the root ASM's _DATA, for the same reason as [aMaine] above. sigma_init()
// held the ONLY reference either .bft label had, so IDA's string auto-name is
// retired for those two rather than aliased, exactly the way `_mima_bft` in
// that same _DATA block already is; `aBoss5_m` keeps IDA's spelling because it
// escapes PLACEHOLDER_RE, which is why th02/main/boss/b5.cpp keeps `aMima_m`
// and b4.cpp keeps `aBoss3_m` for this same call one and two bosses earlier.
extern "C" const char stage5b1_bft[];
extern "C" const char stage5b2_bft[];
extern "C" const char aBoss5_m[];

// The sprite the boss and midboss renderers blit. Declared exactly the way
// th02/main/boss/b4.cpp and b5.cpp already declare it; the address suffix is
// the dump's own published spelling and retiring it means ruling on all 67 of
// its reference sites at once, which is its own parcel.
extern "C" int patnum_2064E;

// th02/main/dialog/dialog.cpp. dialog.hpp declares neither of these two, which
// is how b3.cpp, b4.cpp and b5.cpp all already declare dialog_post().
void near dialog_post(void);
void near dialog_script_extra_pre_intro_animate(void);

// One of the four shared boss-entrance helpers in th02/main/boss/b4.cpp, which
// has no header; b5.cpp forward-declares it the same way. sigma_init() was the
// last caller th02_main.asm had for it, so this lift also retired the whole
// head-of-segment extrn declaration that dump used to reach it and
// mima_17E91() through. Neither of those two names exists in that file any
// more, which is why this comment does not spell either of them in the form a
// naming sweep would then look for.
extern "C" void near boss_playfield_reset(void);

// th02/main/player/shot.hpp, spelled out rather than included: that header
// pulls in th01/math/subpixel.hpp and th02/main/entity.hpp for the sake of one
// call, and this object's include list is load-bearing (see above).
void far shots_free_all(void);

// th02/main/bullet/bullet.cpp, reached through the island below rather than
// called, so only the symbol is needed. Spelled out rather than included
// because th02/main/bullet/bullet.hpp has no include guard.
extern "C" void bullets_clear(void);

// Still ASM. `[measured]` Stops the current KAJA song, snd_load()s [fn] over it
// with SND_LOAD_SONG, and starts it again; every boss init function in this
// binary switches its BGM through it. Declared identically in b3.cpp, b4.cpp,
// b5.cpp and th02/main/stage/init.cpp -- renaming it is a five-declaration
// refactor and therefore its own parcel, not this one.
extern "C" void far sub_13ABB(char *fn);

// Still ASM, above sigma_update() in BOSS_5_TEXT: blits Sigma as her two
// side-by-side 64-pixel-wide cels, at [patnum_2064E] and [patnum_2064E] + 1,
// from [sigma_topleft]. It carries no state of its own, and the six pattern
// functions plus sigma_15907() also call it. `near` resolves to the same
// 3-byte relative near call the original encodes, because it is in this very
// segment; the dump publishes it under this name through a kb/codegen/0123
// alias.
extern "C" void near sigma_put(void);

/// Sigma's own state
/// -----------------

// Top-left corner of her sprite, in screen space, snapshotted from
// [boss_left_on_back_page] / [boss_top_on_back_page] at the top of every
// sigma_update() and seeded here. Every blit, hitbox and bullet origin in her
// still-ASM code takes .x and .y directly.
//
// `[measured]` Unlike [marisa_topleft] (th02/main/boss/b4.hpp), which the
// stage-4 midboss shares, this one really is exclusive: every reference to the
// address in th02_main.asm is inside a `sigma_*` proc, and no other entity
// touches it.
extern screen_point_t sigma_topleft;

// Which of the ten steps of her fight she is in. NOT [boss_phase], which is
// the binary-wide alive/defeated flag and stays 0 for all ten of these -- the
// same distinction th02/main/boss/b3.cpp draws for [stones_phase] and b5m.cpp
// for [mima_phase], and this is the third holder of that shape in this binary.
//
// `[measured]` sigma_update() advances it through 0..9; the even steps are
// one-frame setups that install the next set of pattern functions, the odd
// ones run them. Every access in the dump is an equality test, so nothing
// there says whether the slot is signed.
extern "C" uint8_t sigma_phase;

// ANDed with a per-slot frame counter to decide whether that slot advances its
// cel this frame, so 7 means every 8th frame and 3 means every 4th.
//
// `[measured]` sigma_1566F() holds the only read: it walks a 16-entry array of
// 10-byte records at 0x254EC and, for each one still in state 1, advances the
// record's cel only when `(record->frame & this) == 0`. That array itself is
// still un-analysed, which is why this name describes the gate rather than the
// thing being gated. sigma_init() sets 7; six of her pattern functions set 3.
extern "C" uint8_t sigma_cel_interval_mask;
/// -----------------

/// Her fight-init. Installed as [boss_init] by stage_init()
/// (th02/main/stage/init.cpp), which is why it is `far`. mima_init() in
/// th02/main/boss/b5.cpp is the fuller version of the same sequence; Sigma
/// skips the entrance animation entirely and fades straight in.
extern "C" void far sigma_init(void)
{
	boss_playfield_reset();
	tile_mode = TM_NONE;
	dialog_pre();
	super_clean(128, 192);
	super_patnum = 128;
	super_entry_bfnt(stage5b1_bft);
	super_entry_bfnt(stage5b2_bft);
	grc_setclip(PLAYFIELD_LEFT, 0, PLAYFIELD_RIGHT, (RES_Y - 1));
	dialog_script_extra_pre_intro_animate();

	// Both pages get the constant rather than page 1 getting a copy of page 0,
	// which is what mima_init() does for the same pair.
	boss_left_on_page[0] = (PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 64);
	boss_left_on_page[1] = (PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 64);
	sigma_topleft.x = (PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 64);
	boss_top_on_page[0] = (PLAYFIELD_TOP + 32);
	boss_top_on_page[1] = (PLAYFIELD_TOP + 32);
	sigma_topleft.y = (PLAYFIELD_TOP + 32);

	boss_damage = 0;
	boss_phase_frame = 0;
	patnum_2064E = 128;
	sigma_phase = 0;
	sigma_cel_interval_mask = 7;

	// bullets_clear() is far and lands in this same physical group, so the
	// original reaches it through the linker-relaxed `nop; push cs; call near
	// ptr` island that no plain C++ far call reproduces. (kb/codegen/0083)
	// It takes no arguments, and nothing above it has a deferred __cdecl
	// cleanup for the island to have to reach backwards over -- every call so
	// far is `pascal` and cleans itself - so this is stones_12778()'s shape
	// rather than marisa_init()'s.
	__emit__(0x90);	// nop
	__emit__(0x0E);	// push cs
	_asm { call near ptr bullets_clear; }

	shots_free_all();
	palette_white_out(1);
	sigma_put();

	// The same island again for sub_13ABB(), which does take an argument, so
	// its far pointer and its __cdecl cleanup are hand-spelled with it -
	// statement for statement th02/main/boss/b3.cpp's stones_init(), down to
	// the `add sp, 4`. palette_white_out() above is `pascal`, so again nothing
	// is deferred into this one.
	__emit__(0x1E);	// push ds
	_asm { push offset aBoss5_m; }
	__emit__(0x90);	// nop
	__emit__(0x0E);	// push cs
	_asm { call near ptr sub_13ABB; }
	__emit__(0x83, 0xC4, 0x04);	// add sp, 4

	palette_white_in(1);
	dialog_script_generic_part_animate(DS_PREBOSS);
	dialog_post();
}

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
