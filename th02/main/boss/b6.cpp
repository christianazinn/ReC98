/// Extra Stage boss - Evil Eye Σ
/// -----------------------------
/// Her pattern runner, her per-frame update, her fight-init and her defeat, in
/// dump order. The defeat is also the last thing MAIN.EXE does on an Extra
/// Stage clear: it commits the run to the resident structure and launches
/// MAINE.EXE over this process. This is the bottom of th02_main.asm's
/// BOSS_5_TEXT contribution, and the rest of her -- her twelve patterns, her
/// expanding-blast pool, two movement helpers, her hittest wrapper and her
/// defeat animation -- is still above it in that dump.
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
// -G because sigma_update()'s prolog is `push bp; mov bp, sp; sub sp, 2`
// rather than an `enter 2, 0` (kb/codegen/0011). It is the only function here
// with a local at all: sigma_166DE(), sigma_init() and sigma_end() have ZERO,
// so their prologs are a bare `push bp; mov bp, sp` under either setting and
// the flag does not move them.
//
// `[measured 2026-08-22]` This line read "No -G either" for one parcel, and
// correctly so - -G- is this project's default and the two zero-local bodies
// did not need it. IT WENT RED THE MOMENT A BODY WITH A LOCAL LANDED HERE, at
// instruction 0, for 2 bytes. A per-object flag is a property of the object's
// CURRENT contents, so re-derive it from the incoming body's own prolog on
// every lift into an existing object; the flag that was right for the object
// yesterday is not evidence about today.
#pragma option -zCBOSS_5_TEXT -zPmain_03 -G

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
// And these for sigma_update(). th02/main/stage/stage.hpp before
// th02/main/stage/callback.hpp is this project's rule, but callback.hpp is not
// needed here: this file only *defines* two of the slots' functions.
#include "th02/main/frames.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/bg_particle.hpp"
#include "th02/main/boss/bosses.hpp"
// And these three for her final phase's patterns, appended for the same reason
// as the run above. th02/main/bullet/bullet.hpp is the one that needed a
// decision: it has NO include guard, which is why every earlier parcel in this
// file spelled a bullet symbol out by hand instead of including it
// (bullets_clear() above). It is included ONCE, here, because these patterns
// need two function declarations and four `bullet_group_or_special_motion_t`
// enumerators from it and spelling an enum out by hand is not a saving. Nothing
// else in this TU's include graph reaches it, so the single inclusion is safe;
// a second `#include` of it anywhere in this object is not.
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/player/player.hpp"
#include "th02/math/randring.hpp"
// And these two for phase 7's patterns. Both are unguarded, and both are
// nothing but declarations -- four lines and fourteen -- so unlike
// bullet.hpp above they are idempotent and a second inclusion would cost
// nothing. Included rather than spelled out because vector2_between_plus() has
// eight parameters, two of them references, and hand-copying that signature is
// how a `far`/`near` mismatch gets in.
#include "th02/hardware/pages.hpp"
#include "th02/math/vector.hpp"
// And this one for phase 5 pattern 1. Guarded, and nothing but macros, one
// struct, two extern declarations and five function declarations -- so unlike
// th02/main/bullet/bullet.hpp above it is inert and idempotent, and unlike
// lasers_add() it is not worth hand-copying a four-parameter `pascal`
// signature whose `int patnum_base` is load-bearing (see that header: the
// widened formal is what lets Turbo C++ pack the last two arguments into one
// 32-bit push). It also already documents this proc as the one laser spawner
// in the binary that writes no [laser_wait_frames] of its own.
#include "th02/main/laser.hpp"
// And this one for phase 5 pattern 0, which mirrors a 16x16 bullet around
// the right edge of the playfield and needs the sprite width to do it. Two
// `#define`s and nothing else -- unguarded, but a redefinition to the same
// token sequence is legal, so a second inclusion anywhere in this object
// would still be inert. th02/main/boss/b4.cpp and th02/main/midboss/m4.cpp
// reach it the same way.
#include "th02/sprites/bullet16.h"
// And this one for phase 3 pattern 0, the only pattern in her fight that draws
// into VRAM itself. Guarded, and one enum of one enumerator.
#include "th02/v_colors.hpp"

// th02/main/dialog/dialog.hpp declares every dialog_script_* function but not
// this one, which is how th02/main/boss/b3.cpp, th02/main/boss/b4.cpp and
// th02/main/boss/b5.cpp all already declare it.
void near dialog_pre(void);

// th02/hardware/input.hpp, spelled out here rather than included for the same
// reason th02/main/boss/b5.cpp spells out snd_kaja_interrupt(): that header
// has no include guard and defines fourteen unused `input_t` constants for the
// sake of one call.
void key_delay(void);

// th02/snd/snd.h, spelled out for the same reason and in exactly the form
// th02/main/boss/b5m.cpp already uses: that header has no include guard and
// pulls in libs/kaja/kaja.h, game/pf.h and defconv.h for the sake of one call.
extern "C" void __cdecl snd_se_play(int new_se);

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

// The binary-wide boss alive/defeated flag, and the point shottype B's homing
// shots aim at, set by whichever boss is on screen. All three are published in
// th02_main.asm under these names and have no header; b3.cpp, b4.cpp, b5m.cpp
// and th02/main/enemy/update.cpp all declare them exactly this way.
//
// [boss_phase] is NOT a per-boss progression counter - see [sigma_phase] below.
extern "C" uint8_t boss_phase;
extern "C" int boss_pos_x;
extern "C" int boss_pos_y;

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

// Still ASM, above this object's contribution in BOSS_5_TEXT: blits Sigma as
// her two side-by-side 64-pixel-wide cels, at [patnum_2064E] and
// [patnum_2064E] + 1, from [sigma_topleft]. It carries no state of its own.
//
// `[measured 2026-08-22]` THIS COMMENT USED TO SAY "the six pattern functions
// plus sigma_15907() also call it", and NOT ONE PATTERN FUNCTION DOES. The
// dump holds exactly seven call sites: one in sigma_15907() and SIX inside
// sigma_15A25(), the defeat animation, which blits her at three cels through
// three separate stages of it. Attributing a call count to a group of
// functions is not the same as attributing each call to its enclosing proc,
// and only the second is a measurement. `near` resolves to the same
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
// record's cel only when `(record->frame & this) == 0`. sigma_init() sets 7;
// six of her pattern functions set 3.
//
// `[measured 2026-08-22]` The array is her 16-slot pool of expanding circular
// blasts, so the cel this gates is a blast's 8-frame telegraph, drawn together
// with a warning ring that shrinks 64 -> 0 on the same tick. That makes the
// name right rather than merely mechanical - see state/notes/sigma_update.md,
// which characterises the whole pool. The name is kept as it stands because
// the fuller spelling would run into kb/codegen/0060's 32-character cliff.
extern "C" uint8_t sigma_cel_interval_mask;

// Where her patterns fire from: her sprite's top-left plus 60 on both axes.
// sigma_update() re-derives both every frame and immediately copies them into
// [boss_pos_x] / [boss_pos_y], so this pair is also what shottype B's homing
// shots aim at while she is on screen.
//
// `[measured]` 60 is NOT half of either sprite extent - she is 128x64 from
// [sigma_topleft] - and her hittest is centred on +36/+32 instead
// (sigma_15907), so this offset gets its own name for the same reason
// MARISA_CENTER_OFFSET does (th02/main/boss/b4.hpp): ZUN's notions of a boss's
// centre do not agree with each other. Sigma-exclusive; all 13 read sites in
// th02_main.asm are inside a sigma_* proc, and every one of them is a bullet,
// laser or blast origin.
extern "C" screen_x_t sigma_center_x;
extern "C" screen_y_t sigma_center_y;
static const pixel_t SIGMA_CENTER_OFFSET = 60;

// Which of the current phase's patterns runs this frame. sigma_166DE() cycles
// it and wraps it at the count sigma_update() passes in; every even phase
// resets it to 0. Same role as [mima_pattern] (th02/main/boss/b5m.cpp) and
// [marisa_pattern] (b4.hpp), and like them it is NOT [sigma_phase].
extern "C" uint8_t sigma_pattern;

// `[measured]` Raised by sigma_166DE() on the frame [sigma_pattern] wraps back
// to 0, and cleared by every phase change - and then read by NOTHING, in this
// dump or in any decompiled C++. A dead store, spelled the way
// [mima_orbs_gone_unused] (th02/main/boss/b5m.cpp) and
// [stones_phase_frame_unused] (b3.cpp) already spell theirs.
extern "C" uint8_t sigma_pattern_looped_unused;

// The patterns of the phase she is in. sigma_update() installs two or three of
// them at every even phase and sigma_166DE() calls one per frame.
//
// `[measured]` THE EXTENT IS EXACTLY THREE SLOTS, and the _BSS layout is what
// proves it rather than any access: the three words are gapless and
// [sigma_phase_damage_max] begins immediately after the third, so a fourth
// slot has nowhere to live. Only the phases that pass 3 install slot [2]; the
// ones that pass 2 leave whatever the previous group put there, and it is
// never called.
//
// `_func` on the slot rather than on the function, the convention
// th02/main/stage/callback.hpp spells out for [boss_update_func].
static const int SIGMA_PATTERN_SLOTS = 3;

// The dump's pattern procs are `near` and parameterless, and their
// kb/codegen/0123 aliases publish in Borland's __cdecl decoration, so
// platform.h's nearfunc_t_near cannot type this array: that typedef is
// `pascal`, which would ask the linker for an upper-cased symbol instead
// (kb/codegen/0086). For a parameterless call the two conventions emit
// identical bytes; only the decoration differs.
typedef void (near *near sigma_pattern_func_t)(void);
extern "C" sigma_pattern_func_t sigma_pattern_func[SIGMA_PATTERN_SLOTS];

// How much [boss_damage] the current phase ends at. sigma_166DE() is the only
// reader. [mima_phase_damage_max] (th02/main/boss/b5m.cpp) is the same-binary
// precedent for both the role and the name.
extern "C" int sigma_phase_damage_max;

// The radius of the outer of the two expanding dot-square rings
// sigma_update() blits behind her, the inner one being this radius plus 128,
// both around a fixed (224, 200). Near-twin of [mima_ring_radius]
// (th02/main/boss/b5m.cpp), which is the same effect at the same centre.
extern "C" uint8_t sigma_ring_radius;

// The signed horizontal step sigma_move_sweep() latches on frame 50 and then
// applies for three legs. [marisa_velocity_x] / [marisa_velocity_y]
// (th02/main/boss/b4.hpp) is the structural twin, down to the latch-once-from-a-
// player-compare rule, and `velocity_x` is the house concept
// (enemy_t::velocity_x, ITEM_MISS_VELOCITY_X_CENTER). NOT a `_dx` suffix: in
// this tree `dx` means the DX register (grcg_off_clobbering_dx).
// `[measured]` Signed: every read is `mov al` + `cbw`.
// Sigma-exclusive AND helper-exclusive -- all four references in th02_main.asm
// were inside sigma_165A5, so this lift held the last of them and IDA's name is
// RETIRED rather than aliased, the way [mima_velocity_y] in that same _BSS block
// already is.
//
// `_x` although the sweep has no vertical component, because its twin
// latches the same shape and DOES move her on both axes; the pair reads as a
// pair now that it has landed, directly below.
extern "C" int8_t sigma_sweep_velocity_x;

// And that twin's own step, latched by sigma_move_weave() on the same frame from
// the same comparison and applied for three legs. One pixel where the sweep gets
// two, and this is the one of the pair that also moves her vertically -- by a
// literal 1 in either direction rather than through a second latched byte, so
// there is no `_y` member to name.
//
// `[measured]` Signed, on `mov al` + `cbw` at all three reads; Sigma-exclusive
// and helper-exclusive, all four references in th02_main.asm -- one write on the
// latch frame and one read on each of the three legs -- having been inside the
// helper itself, so this lift held the last of them and IDA's name is RETIRED
// rather than aliased.
extern "C" int8_t sigma_weave_velocity_x;

// Phase 7 pattern 0's two streams of expanding blasts. `[measured]`
// sigma_16421() is the sole owner of all five of these -- every reference in
// th02_main.asm was inside it -- so all five are renames rather than
// kb/codegen/0123 aliases, and none of them needed a ruling about any other
// pattern. th02/main/boss/b5m.cpp keeps [point_26CD6] under IDA's spelling for
// the same role precisely because its two callers SHARE it and naming it would
// have needed that ruling; this one does not.
//
// [sigma_stream_velocity] is the 48-pixel step from her centre toward the
// player, computed once by vector2_between_plus() on the aim frame, and then
// added to the first stream's spawn point every time it fires.
//
// `stream` after th02/main/boss/b5m.cpp's "Her two pellet streams", which is the
// same shape one boss earlier: a muzzle that walks one step at a time.
// The word "trail" was rejected: th01/main/boss/b15m.cpp already spends it on a
// position HISTORY behind one moving sprite (TRAIL_COUNT and its three slots),
// and these are a series of independent spawns instead. Symbol-clean, meaning
// already taken.
//
// [sigma_stream_mirror_velocity_x] is 31 characters as an object symbol, one
// under kb/codegen/0060's 32-character cliff, so this family cannot grow a
// longer member.
// [sigma_stream_mirror_velocity_x] is its negation, held in its own slot rather
// than negated at each use because that is what the original stores.
extern "C" screen_point_t sigma_stream_velocity;
extern "C" int sigma_stream_mirror_velocity_x;

// The two streams' current spawn points. `[measured]` The y is SHARED -- both
// walk down the same rows, and only their x diverges, which is why there is no
// mirrored y.
//
// `mirror` for the derived one, after STONES_LASER_MIRROR
// (th02/main/boss/b3.cpp), which is the same game's word for the same
// "symmetric pair" situation. NOT `symmetric`: this codebase uses that word for
// the pair as a whole (BG_2_SPREAD_HORIZONTALLY_SYMMETRIC) and never to
// distinguish one member of it.
extern "C" screen_x_t sigma_stream_x;
extern "C" screen_y_t sigma_stream_y;
extern "C" screen_x_t sigma_stream_mirror_x;

// How much sigma_1566F() shrinks an expanding blast's lethal square relative to
// the circle it draws. `[measured]` SIGNED, and every read is `mov al` + `cbw`:
// a positive value insets the hitbox, a negative one GROWS it past the drawn
// radius, and the lethal shape is an axis-aligned square of half-extent
// `radius - this` against the player's 32x32 box even though the visual is a
// disc.
//
// This is the ONE symbol in Sigma's _BSS that more than one proc touches, which
// is why it keeps a kb/codegen/0123 alias instead of being renamed: still-ASM
// sigma_1566F() holds all four reads, and sigma_15E84() and sigma_15F95() write
// it too.
//
// ZUN quirk, and it is load-bearing: sigma_init() never initialises this, and
// neither phase 7's second pattern nor any of phase 9's three write it, so from
// the moment sigma_16421() runs, every later blast in the fight inherits its
// -4. Preserve that on a match branch.
extern "C" int8_t sigma_blast_hitbox_margin;

// The angle sigma_16650() spawns its 8-way ring of 16x16 balls at, stepped by 8
// per ring. `[measured]` All five references were inside sigma_16650, so this
// name is retired rather than aliased too.
//
// `[measured 2026-08-22]` The shorter spelling without `ball` is not merely
// vaguer, it COLLIDES: this file already holds SIGMA_RING_ANGLE_STEP and
// SIGMA_RING_ANGLE_STEP_REDUCED, and `<x>_ring_angle` + `<X>_RING_ANGLE_STEP` is
// an established pair idiom here (stage3_ring_angle in
// th02/main/stage/stages.cpp, mima_ring_radius in b5m.cpp). Those two constants
// belong to [sigma_ring_radius] above -- the two dot-square background rings
// sigma_update() blits at a fixed centre, which have no angle of their own -- so
// the short name would have falsely paired with them. Naming the projectile is
// what keeps the two apart, and mima_spiral_angle, mima_ray_angle,
// mima_stream_angle and mima_fan_angle are the <boss>_<what-rotates>_angle
// family it joins.
//
// THAT COLLISION IS INVISIBLE TO A CASE-SENSITIVE CENSUS, which is how it was
// nearly missed: TASM runs under `/mx` and resolves symbols case-insensitively
// while publishing whatever the directive spells, so a name census in this
// project has to be case-insensitive to be an instrument at all.
extern "C" uint8_t sigma_ball_ring_angle;

static const screen_x_t SIGMA_RING_CENTER_X = 224;
static const screen_y_t SIGMA_RING_CENTER_Y = 200;
static const int SIGMA_RING_COUNT = 2;
static const int SIGMA_RING_DISTANCE = 128;
static const int SIGMA_RING_RADIUS_STEP = 8;
static const vc_t SIGMA_RING_COL = 4;

// `[measured]` Where Sigma differs from Mima: Mima skips her rings entirely if
// [reduce_effects] is set, Sigma coarsens hers from a 2-step walk of the
// 256-step circle to a 16-step one instead, so the ring stays but is drawn
// from an eighth as many dots.
static const int SIGMA_RING_ANGLE_STEP = 2;
static const int SIGMA_RING_ANGLE_STEP_REDUCED = 16;

// Frames phase 0 holds before the first pattern group is installed.
static const int SIGMA_INTRO_FRAMES = 50;

// [boss_damage] each of the first four pattern groups ends at.
static const int SIGMA_PHASE_DAMAGE = 1800;

// `[measured]` And the fifth's, which is DEAD: phase 9's own defeat test fires
// at SIGMA_DEFEAT_DAMAGE, well below this, so sigma_166DE() can never reach
// this threshold and phase 10 does not exist.
static const int SIGMA_FINAL_PHASE_DAMAGE = 5000;

static const int SIGMA_DEFEAT_DAMAGE = 1300;
static const long SIGMA_DEFEAT_SCORE = 300000;
/// -----------------

/// Her still-ASM code, all of it above this object's contribution in
/// BOSS_5_TEXT, and all of it `near` because it is in this very segment.
/// -------------------------------------------------
/// EVERY ONE OF THESE KEEPS THE DUMP'S ADDRESS-SUFFIXED SPELLING, and that is
/// a decision rather than an omission. An address-suffixed hand name is not an
/// IDA placeholder (tools/re/naming_precheck.py's pattern is keyed on IDA's
/// own kind prefixes, and `sigma_15D56` matches none of them), and naming a
/// pattern body means reading it and ruling on the whole table at once - which
/// is what th02/main/boss/b3.cpp, b4.cpp and b5m.cpp all already say for
/// stones', Marisa's and Mima's patterns in this same binary. sigma_put()
/// above was renamed only because the parcel that moved its caller read its
/// twelve-instruction body.
///
/// state/notes/sigma_update.md characterises all of them from measurement and
/// is the map for that naming round. In short: 1566F is the 16-slot
/// expanding-blast pool's per-frame update, render and hittest; 15907 is her
/// own hittest plus the blit; 15A25 is the defeat animation; and the twelve
/// below are the patterns themselves. FOUR more procs are missing from this
/// list on purpose: the dump publishes no alias for the blast pool's spawn or
/// for the helper of sigma_bg_render(), and nothing in this object calls the
/// second of those. Publishing one is part of whichever parcel first needs it --
/// and the two movement helpers that used to be named here are C++ now, both of
/// them plain `static` functions below.

extern "C" void near sigma_1566F(void);
extern "C" void near sigma_15907(void);
extern "C" bool16 near sigma_15A25(void);


// And the one still-ASM proc in her chain that the dump kept PRIVATE and this
// object nevertheless has to reach: the spawn for her 16-slot pool of expanding
// circular blasts at 0x254EC. Bounds-checks the point against the playfield,
// takes the first free record, seeds it and plays SE 9.
//
// **RETURNS `true` WHEN THE REQUEST WAS REJECTED**, for either reason -- the
// point outside `0 < x < 444` and `0 < y < 400`, or every record already in use.
// That inverts the polarity every other `bool16` in this tree uses, and the
// project's habit is to carry such an inversion in the doc comment rather than
// in the name (midboss3_invalidate() does the same thing in this same binary),
// which is why this sentence is shouting instead of the identifier. Seven of the
// eight call sites ignore the value; sigma_16421 is the one that reads it, and
// restarts its pattern when both of its two spawns are refused.
//
// Plural pool noun plus `_add`, which is the house shape by sixteen precedents
// to one: bullets_add_pellet, items_add, lasers_add, sparks_add, enemies_add,
// b4balls_add, swords_add, cheetos_add. (`shot_add` is the lone singular, and
// `_spawn` is never a verb in this tree.)
//
// `pascal`, which is not a style choice but what the original's `retn 6` says,
// and therefore what its new alias in the dump has to publish: Borland decorates
// a `pascal` `extern "C"` name in UPPER CASE with no leading underscore, so the
// dump grew `public SIGMA_BLASTS_ADD`; the lower-case underscore-prefixed form
// every other alias in that dump uses would not have resolved
// (kb/codegen/0086, kb/codegen/0027; `public MPN_PUT_8` in th04_main.asm and
// `public MPN_FREE` in th05_main.asm are the in-tree idiom).
//
// [radius_max] is written verbatim into the record and is where each pattern's
// blast stops growing -- see SIGMA_BLAST_RADIUS_MAX below for why it is an
// argument rather than a constant.
extern "C" bool16 pascal near sigma_blasts_add(
	screen_x_t x, screen_y_t y, int radius_max
);
/// -------------------------------------------------

/// Constants her patterns share
/// ----------------------------
/// These live above the phase groups because the groups below reach across each
/// other for them, and a `static const` emits nothing, so the placement is free.

// The frame both movement helpers hold until, and the frame on which each
// latches its direction. `[measured]` The same 50 in both, and the same 50
// SIGMA_INTRO_FRAMES uses for phase 0 -- but those are different counters
// (phase 0 tests `>`, these test `<` and `==`), so this is its own constant.
static const int SIGMA_MOVE_HOLD_FRAMES = 50;

// The radius most of her patterns cap their blasts at. `[measured]` NOT a
// property of the pool: sigma_155C5's third argument is written straight into
// the record and its eight call sites pass three different values -- 0x40 from
// sigma_15E84, 0x18 twice from sigma_15F95, and this one everywhere else.
static const int SIGMA_BLAST_RADIUS_MAX = 0x30;

// What sigma_15F95() and sigma_16421() inset the blast hitbox by while they
// run, and the only value either of them ever writes there. `[measured
// 2026-08-22]` MOVED UP HERE from phase 7's own block by the phase 3 parcel,
// which is the second group to need it: a `static const` emits nothing, so a
// constant two groups share belongs above both of them and the move is free.
static const int8_t SIGMA_BLAST_HITBOX_MARGIN_WIDE = -4;

// The two values [sigma_cel_interval_mask] ever takes: 7 for a blast telegraph
// that advances every 8th frame, 3 for every 4th.
static const uint8_t SIGMA_CEL_INTERVAL_SLOW = 7;
static const uint8_t SIGMA_CEL_INTERVAL_FAST = 3;
/// ----------------------------

/// Phase 1's first two patterns
/// ---------------------------
/// [sigma_phase] 1's slots 0 and 1, and with them all twelve of her patterns.
/// Slot 0 is the only pattern in her fight that moves her WITHOUT either
/// movement helper: it carries its own three-leg schedule inline, on its own
/// frames, with a 4x dash on the middle leg.

// Her sprite's extents, which slot 0 needs because it fires from a random column
// across her whole width and from her bottom edge. `[measured]` The file's own
// comment on SIGMA_CENTER_OFFSET already records 128x64 from [sigma_topleft];
// this is the same measurement spelled as constants, and the mask below is
// SIGMA_W - 1 rather than a bare 0x7F because 128 is a real extent rather than a
// convenient power of two.
static const pixel_t SIGMA_W = 128;
static const pixel_t SIGMA_H = 64;

// The signed horizontal step slot 0 latches on its own hold frame, third member
// of the [sigma_sweep_velocity_x] / [sigma_weave_velocity_x] family and latched
// by the same comparison against the playfield centre.
//
// Named for the ONE leg that makes it distinctive, which is the middle one:
// legs 1 and 3 drift her by this at 1x, and leg 2 dashes her BACK at 4x. Two of
// the three legs are therefore not a dash, and this sentence rather than the
// identifier is where that lives -- the same way [sigma_blasts_add]'s inverted
// return is carried in prose above.
//
// `[measured]` Signed, on `mov al` + `cbw` at all three reads.
extern "C" int8_t sigma_dash_velocity_x;

// Half the angular width of the pellet spray slot 0's last stage fires, in
// 256ths of a turn, and it WIDENS BY ONE EVERY FRAME of that stage rather than
// per volley -- the counter runs every frame and only the volley is gated.
//
// `[measured]` The window is `[SIGMA_SPRAY_ANGLE_CENTER - this,
// SIGMA_SPRAY_ANGLE_CENTER + this - 1]`, from
// `(randring2_next8() % (this * 2)) + (SIGMA_SPRAY_ANGLE_CENTER - this)`. It is
// also zeroed on the hold frame, 320 frames before the stage that reads it.
//
// "spray" is attested for a boss pellet stream in this same game family --
// MIMA_SPRAY_LAST_FRAME and [marisa_spray_is_first_run] -- and TH01 has
// pattern_pellet_arcs_at_expanding_random_angles for this exact effect. NOT
// `spread`: th04/main/bullet/types.h spends `spread_angle` on the angle BETWEEN
// adjacent bullets of a group, which is a different quantity.
extern "C" uint8_t sigma_spray_half_angle;

// Slot 0's own schedule. `[measured]` Its hold is 20 frames rather than the
// helpers' 50, and every one of these frames is its own: the drift out ends at
// 148, the 4x dash back at 212, the second drift at 340, and the spray stage
// runs from there to 440.
static const int SIGMA_DASH_HOLD_FRAMES = 20;
static const int SIGMA_DASH_LEG_1_END = 148;
static const int SIGMA_DASH_LEG_2_END = 212;
static const int SIGMA_DASH_LEG_3_END = 340;
static const int SIGMA_SPRAY_PAST_LAST_FRAME = 440;
static const int SIGMA_DASH_SPEED_MULTIPLIER = 4;

// SE 3 every 4th frame for the whole pattern, hold included, and it is the same
// SE the blast pool plays when a telegraph finishes -- so this pattern ticks
// audibly all the way through.
static const int SIGMA_DASH_SE_INTERVAL = 4;

// Straight down, and the centre the spray widens around.
static const uint8_t SIGMA_SPRAY_ANGLE_CENTER = 0x40;

// One pellet on every ODD frame of the three drift legs, and three per volley on
// every 4th frame of the spray stage.
static const int SIGMA_SPRAY_VOLLEY_PELLETS = 3;

// The random speed jitter both stages add to their pellet speed.
static const int SIGMA_SPRAY_SPEED_BASE = ((1 << 4) + 14);
static const uint8_t SIGMA_SPRAY_SPEED_JITTER_MASK = 0x1F;

/// Phase 1 pattern 0: her own three-leg walk -- drift out, dash back at 4x,
/// drift out again -- dropping one pellet from a random column on every odd
/// frame, and then 100 frames of three-pellet volleys through a spray that
/// widens by one angle unit per frame.
extern "C" void near sigma_15D56(void)
{
	// The volley counter, `register` because the original keeps it in SI. Only
	// one register local here against sigma_15F95()'s two, and the frame is
	// `sub sp, 2` for the one-byte angle below either way.
	register int i;

	// The spray's angle for this volley, and a local for the same reason
	// sigma_15F95()'s antipode is: the original computes it into [bp-1] once and
	// then reads it back inside the loop.
	uint8_t angle;

	if(boss_phase_frame < SIGMA_DASH_HOLD_FRAMES) {
		return;
	}
	if(boss_phase_frame == SIGMA_DASH_HOLD_FRAMES) {
		sigma_spray_half_angle = 0;

		// A conditional EXPRESSION and not an `if`/`else` with a store in each
		// arm, for the 2 bytes both movement helpers spell out at their own copy
		// of this line -- and latched AWAY from the player, same as those two.
		sigma_dash_velocity_x = ((player_topleft.x < PLAYER_LEFT_START)
			? 1
			: -1
		);
	}

	// `%` and not `& 3`, and the original's `cwd` + `idiv` is what says so.
	if((boss_phase_frame % SIGMA_DASH_SE_INTERVAL) == 0) {
		snd_se_play(3);
	}

	// The pellet spawn is written out in all three legs and `-O` cross-jumps the
	// three identical copies into ONE, which is what the original has: three
	// `test`s, three `jz`s, and a single call sequence they all reach. The odd
	// frames are the same beat in every leg, so a reader would factor this out
	// below the chain -- and that compiles to ONE `test` instead of three.
	if(boss_phase_frame < SIGMA_DASH_LEG_1_END) {
		*boss_left_on_back_page += sigma_dash_velocity_x;
		if((boss_phase_frame & 1) != 0) {
			bullets_add_pellet(
				(randring2_next8_and(SIGMA_W - 1) + sigma_topleft.x),
				(sigma_topleft.y + SIGMA_H),
				SIGMA_SPRAY_ANGLE_CENTER,
				BG_1,
				(randring2_next8_and(SIGMA_SPRAY_SPEED_JITTER_MASK) +
					SIGMA_SPRAY_SPEED_BASE)
			);
		}
	} else if(boss_phase_frame < SIGMA_DASH_LEG_2_END) {
		*boss_left_on_back_page -= (
			sigma_dash_velocity_x * SIGMA_DASH_SPEED_MULTIPLIER
		);
		if((boss_phase_frame & 1) != 0) {
			bullets_add_pellet(
				(randring2_next8_and(SIGMA_W - 1) + sigma_topleft.x),
				(sigma_topleft.y + SIGMA_H),
				SIGMA_SPRAY_ANGLE_CENTER,
				BG_1,
				(randring2_next8_and(SIGMA_SPRAY_SPEED_JITTER_MASK) +
					SIGMA_SPRAY_SPEED_BASE)
			);
		}
	} else if(boss_phase_frame < SIGMA_DASH_LEG_3_END) {
		*boss_left_on_back_page += sigma_dash_velocity_x;
		if((boss_phase_frame & 1) != 0) {
			bullets_add_pellet(
				(randring2_next8_and(SIGMA_W - 1) + sigma_topleft.x),
				(sigma_topleft.y + SIGMA_H),
				SIGMA_SPRAY_ANGLE_CENTER,
				BG_1,
				(randring2_next8_and(SIGMA_SPRAY_SPEED_JITTER_MASK) +
					SIGMA_SPRAY_SPEED_BASE)
			);
		}
	} else if(boss_phase_frame < SIGMA_SPRAY_PAST_LAST_FRAME) {
		// She stops moving for this stage. The widening and the angle roll
		// happen EVERY frame; only the volley is gated to every 4th, so 3 of
		// every 4 rolls are thrown away.
		sigma_spray_half_angle++;
		angle = ((randring2_next8() % (sigma_spray_half_angle * 2)) +
			(SIGMA_SPRAY_ANGLE_CENTER - sigma_spray_half_angle)
		);
		if((boss_phase_frame & 3) != 0) {
			return;
		}
		for(i = 0; i < SIGMA_SPRAY_VOLLEY_PELLETS; i++) {
			bullets_add_pellet(
				(randring2_next8_and(SIGMA_W - 1) + sigma_topleft.x),
				(sigma_topleft.y + SIGMA_H),
				angle,
				BG_1,
				(randring2_next8_and(SIGMA_SPRAY_SPEED_JITTER_MASK) +
					SIGMA_SPRAY_SPEED_BASE)
			);
		}
	} else {
		boss_phase_frame = 0;
	}
}

// What sigma_15E84() insets the blast hitbox by, and it is the sibling
// SIGMA_BLAST_HITBOX_MARGIN_WIDE needed: this is the ONE writer in her whole
// fight that makes the lethal square SMALLER than the drawn circle. The other
// two writers set the negative value, which grows it past what the player can
// see -- and phases 7 and 9 write it at all, so they inherit whichever of the
// two ran last.
static const int8_t SIGMA_BLAST_HITBOX_MARGIN_NARROW = 4;

// Where slot 1 drops its two blasts, and both at the same y. `[measured]` The
// two x values are 128 and 320, which are NOT symmetric about the playfield
// centre (PLAYFIELD_LEFT is 16 and PLAYFIELD_W is 384, so the centre column is
// 208): the pair is off to the left by 80 pixels.
static const screen_x_t SIGMA_TRAP_1_X = 128;
static const screen_x_t SIGMA_TRAP_2_X = 320;
static const screen_y_t SIGMA_TRAP_Y = 320;

// And its own frames. The two blasts land 20 frames apart, the aimed spread runs
// for the 19 frames after that, and the pattern restarts past 200.
static const int SIGMA_TRAP_1_FRAME = 100;
static const int SIGMA_TRAP_2_FRAME = 120;
static const int SIGMA_TRAP_SPREAD_FIRST_FRAME = 130;
static const int SIGMA_TRAP_SPREAD_PAST_LAST = 150;
static const int SIGMA_TRAP_SPREAD_INTERVAL = 4;
static const int SIGMA_TRAP_PAST_LAST_FRAME = 200;

// The radius these two stop growing at, and the largest any of her blasts
// reaches -- SIGMA_BLAST_RADIUS_MAX above is 0x30 and phase 3's are 0x18.
static const int SIGMA_TRAP_BLAST_RADIUS_MAX = 0x40;

/// Phase 1 pattern 1: two big slow blasts dropped at fixed points near the
/// bottom of the playfield 20 frames apart, then five frames of a medium aimed
/// spread. She does not move at all.
extern "C" void near sigma_15E84(void)
{
	if(boss_phase_frame < SIGMA_TRAP_1_FRAME) {
		return;
	}

	// Two arms whose only difference is the x, and `-O` cross-jumps everything
	// they share -- the y, the radius and the call -- into one copy, which is
	// what the original has: each arm pushes only its own x and then jumps to a
	// single shared tail that pushes the other two arguments as one packed
	// 32-bit immediate and makes the call. Written as two plain arms; the merge
	// is the compiler's.
	if(boss_phase_frame == SIGMA_TRAP_1_FRAME) {
		sigma_cel_interval_mask = SIGMA_CEL_INTERVAL_SLOW;
		sigma_blast_hitbox_margin = SIGMA_BLAST_HITBOX_MARGIN_NARROW;
		sigma_blasts_add(SIGMA_TRAP_1_X, SIGMA_TRAP_Y,
			SIGMA_TRAP_BLAST_RADIUS_MAX
		);
	} else if(boss_phase_frame == SIGMA_TRAP_2_FRAME) {
		sigma_blasts_add(SIGMA_TRAP_2_X, SIGMA_TRAP_Y,
			SIGMA_TRAP_BLAST_RADIUS_MAX
		);
	} else if(
		(boss_phase_frame > SIGMA_TRAP_SPREAD_FIRST_FRAME) &&
		(boss_phase_frame < SIGMA_TRAP_SPREAD_PAST_LAST) &&
		((boss_phase_frame % SIGMA_TRAP_SPREAD_INTERVAL) == 0)
	) {
		bullets_add_pellet(
			sigma_center_x,
			sigma_center_y,
			0,
			BG_5_SPREAD_MEDIUM_AIMED,
			((7 << 4) + 8)
		);
	}
	if(boss_phase_frame > SIGMA_TRAP_PAST_LAST_FRAME) {
		boss_phase_frame = 0;
	}
}
/// ---------------------------

/// The movement helper phases 1, 3, 5 and 7 share, and phase 1's third pattern
/// ---------------------------------------------------------------------------
/// The helper is the twin of sigma_move_sweep() at the bottom of this file: the
/// same 50-frame hold, the same latch-once-on-frame-50 rule, the same
/// zero-[boss_phase_frame] handshake with sigma_166DE(). It walks her one pixel
/// per frame instead of two, over legs at different frames, and it is the one of
/// the pair that moves her on BOTH axes.
///
/// "weave" is a COINAGE and is named as one. Unlike "sweep", which
/// th04/main/boss/b4r_upd.cpp attests as a boss-motion noun, it occurs nowhere
/// else in this tree. The attested alternative was rejected rather than
/// overlooked: "drift" is already spent in this same binary, on Marisa's own
/// drift helper in th02/main/boss/b4.cpp and on three `bullet_special` fields,
/// and three legs on two axes is not a drift.
///
/// `[measured 2026-08-22]` **AND IT IS `static` NOW, WHICH IS THE POINT OF THIS
/// PARCEL.** The phase 7 group had to publish it as a kb/codegen/0123 alias
/// because its four callers were spread one per phase over 1, 3, 5 and 7, so no
/// shorter group could take it along. Phases 5 and 3 then reached it for free.
/// This parcel lifts its LAST ASM caller, sigma_15F6F() below, so the alias has
/// no remaining reader and goes out of the dump with the body -- and the helper
/// becomes exactly the plain `static` that sigma_move_sweep() has been since its
/// own group landed. That is the whole arc of "when a shared helper cannot ride
/// along, the FIRST group that needs it pays once for all of them": one publish,
/// paid once, spent three times, then retired by the group that empties it.

// One pixel per frame, against sigma_move_sweep()'s two.
static const pixel_t SIGMA_WEAVE_SPEED = 1;

// `[measured]` Three legs and then a reset, at frames the sweep does not share.
// Legs 1 and 3 both step her x by +[sigma_weave_velocity_x] and differ only in
// the SIGN of their one-pixel y step, so -O cannot cross-jump them the way it
// merges the sweep's identical legs 1 and 3 -- which is why this body is 0x78
// against that one's 0x61 for the same shape.
static const int SIGMA_WEAVE_LEG_1_END = 114;
static const int SIGMA_WEAVE_LEG_2_END = 242;
static const int SIGMA_WEAVE_LEG_3_END = 306;

static bool16 near sigma_move_weave(void)
{
	if(boss_phase_frame < SIGMA_MOVE_HOLD_FRAMES) {
		return true;
	}
	if(boss_phase_frame == SIGMA_MOVE_HOLD_FRAMES) {
		// A conditional EXPRESSION and not an `if`/`else` with a store in each
		// arm, for the same 2 bytes sigma_move_sweep() spells out at its own
		// copy of this line: the original loads each candidate into AL, jumps to
		// a join, and stores AL to this byte ONCE.
		//
		// `[measured]` And the latch is AWAY from the player: she gets the
		// POSITIVE step when the player is to the LEFT of the playfield centre.
		sigma_weave_velocity_x = ((player_topleft.x < PLAYER_LEFT_START)
			? SIGMA_WEAVE_SPEED
			: -SIGMA_WEAVE_SPEED
		);
	}

	// `++` and `--` on the page-indexed y rather than `+= 1` / `-= 1`:
	// kb/codegen/0094's first discriminator, and the original takes the
	// dedicated `inc`/`dec word ptr [bx]` forms. The x steps go through the
	// latched byte, so they are ordinary `add`/`sub` of a widened AL.
	if(boss_phase_frame < SIGMA_WEAVE_LEG_1_END) {
		*boss_left_on_back_page += sigma_weave_velocity_x;
		(*boss_top_on_back_page)++;
	} else if(boss_phase_frame < SIGMA_WEAVE_LEG_2_END) {
		*boss_left_on_back_page -= sigma_weave_velocity_x;
	} else if(boss_phase_frame < SIGMA_WEAVE_LEG_3_END) {
		*boss_left_on_back_page += sigma_weave_velocity_x;
		(*boss_top_on_back_page)--;
	} else {
		// What sigma_166DE() reads as "the pattern finished its loop". The phase
		// counter is untouched.
		boss_phase_frame = 0;
	}
	return false;
}

/// Phase 1 pattern 2: the weave, with a 32-way ring at a random angle every 32nd
/// frame. Phase 3's pattern 1 is the same body with a 16-way ring every 8th
/// frame and a slower one.
extern "C" void near sigma_15F6F(void)
{
	if(sigma_move_weave()) {
		return;
	}
	if((boss_phase_frame & 0x1F) == 0) {
		bullets_add_pellet(
			sigma_center_x,
			sigma_center_y,
			randring2_next8(),
			BG_32_RING,
			((5 << 4) + 5)
		);
	}
}
/// ---------------------------------------------------------------------------

/// Phase 3's two patterns
/// ----------------------
/// [sigma_phase] 3's group. The first is the only pattern in her fight that
/// draws anything of its own into VRAM; the second is the shortest body in the
/// whole chain.

// Where phase 3 pattern 0 puts its circle, snapshotted ONCE from the player's
// centre on the aim frame and then never moved -- so dodging after that frame is
// what the pattern is about.
//
// "orbit" because the two blasts it spawns walk around this point at a fixed
// radius, and it is attested for exactly that in
// th05/main/midboss/m2_updt.cpp's `orbit_radius`. NOT `ring`: this file already
// spends that on [sigma_ring_radius] and its two SIGMA_RING_ANGLE_STEP
// constants, which are the dot-square background rings and a different effect
// entirely -- the same collision state/notes/sigma_update.md's census caught
// once already for her ball ring's angle.
extern "C" screen_x_t sigma_orbit_center_x;
extern "C" screen_y_t sigma_orbit_center_y;

// Where on that circle the next PAIR of blasts goes. `[measured]` A full-circle
// 0-255 byte read with `mov ah, 0`, so unsigned, and the pair is antipodal:
// the second blast is spawned at this plus half a turn.
extern "C" uint8_t sigma_orbit_angle;

// The radius of the circle, and it is ONE number doing two jobs: the outline is
// drawn with it and the blasts are placed on it, so what the player sees really
// is where the blasts will be. `[measured]` The drawn radius is the literal 112
// and the placement scale is 0x70; SIGMA_ORBIT_RADIUS is both.
static const int SIGMA_ORBIT_RADIUS = 112;

// The frame the circle is aimed and fixed on. Her fourth pattern to use 100 for
// that; see SIGMA_LASER_ARM_FRAME below.
static const int SIGMA_ORBIT_AIM_FRAME = 100;

// The blink runs from the aim frame to here, the blasts start here, and the
// ring joins in here -- the pattern layers itself on rather than switching
// between stages.
static const int SIGMA_ORBIT_BLINK_PAST_LAST = 130;
static const int SIGMA_ORBIT_BLAST_FIRST_FRAME = 150;
static const int SIGMA_ORBIT_RING_FIRST_FRAME = 200;
static const int SIGMA_ORBIT_PAST_LAST_FRAME = 450;

// The blink: drawn on `frame % 8 == 0` and erased on `frame % 8 == 2`, so it is
// on for two frames out of eight rather than half the time.
static const int SIGMA_ORBIT_BLINK_INTERVAL = 8;
static const int SIGMA_ORBIT_BLINK_ERASE_AT = 2;

// One antipodal PAIR of blasts every 12 frames, each capped well below
// SIGMA_BLAST_RADIUS_MAX -- these are the small ones.
static const int SIGMA_ORBIT_BLAST_INTERVAL = 12;
static const int SIGMA_ORBIT_BLAST_RADIUS_MAX = 0x18;

// Half of the 256-step circle. `[measured]` Added to a COPY of the angle rather
// than to the angle itself, which is why this pattern has a stack local at all.
static const int SIGMA_ORBIT_ANTIPODE = 0x80;

// An eighth of a turn BACKWARDS per pair, so the two blasts sweep clockwise
// around the circle and meet where they started after four pairs.
//
// `int` and not `uint8_t`, and negative rather than a `-=`: kb/codegen/0094's
// second discriminator says a byte-typed addend folds the step into
// `add mem, imm8` while an int-typed one forces the AL round trip the original
// takes, and 0094's scope note leaves `-=` unmeasured, so `+=` with a negative
// int is the spelling with evidence behind it.
static const int SIGMA_ORBIT_ANGLE_STEP = -0x10;

/// Phase 3 pattern 0: a 50-frame charge flicker, then a white outline circle
/// blinked at wherever the player was on the aim frame, and from 50 frames after
/// that a pair of antipodal expanding blasts walking around that circle every 12
/// frames, plus a 32-way ring on every 16th frame from frame 200.
extern "C" void near sigma_15F95(void)
{
	// The two blast coordinates, `register` because the original keeps them in
	// SI and DI -- which is worth 14 of this body's 0x1E1 bytes and is invisible
	// in the source's meaning, so it gets counted here. Passing the two
	// expressions straight into sigma_blasts_add() instead compiles to
	// `push ax` twice; the originals's registers cost `mov si, ax` + `mov di, ax`
	// + `push si` + `push ax` at each of the two spawns (8), `push si` +
	// `push di` + `pop di` + `pop si` around the body (4), and 1 byte each in the
	// two arms above that return early, because a four-instruction epilogue is
	// long enough that Turbo C++ jumps to the shared one rather than repeat it
	// (2). DI is then never READ: the peephole notices AX still holds it and
	// pushes AX, so the original carries a dead register store that only appears
	// if the source really has the variable.
	//
	// Declared before the byte below because SI goes to the first `register`
	// declaration; the byte's own [bp-1] slot is unaffected either way, since a
	// register variable takes no frame space.
	register screen_x_t blast_x;
	register screen_y_t blast_y;

	// The antipode of [sigma_orbit_angle], and a local because the original
	// computes it into [bp-1] and then reads it back for the second lookup
	// rather than recomputing it. Its declaration is what makes the frame
	// `sub sp, 2` for one byte.
	uint8_t angle_antipodal;

	if(boss_phase_frame < SIGMA_MOVE_HOLD_FRAMES) {
		return;
	}
	if(boss_phase_frame == SIGMA_MOVE_HOLD_FRAMES) {
		snd_se_play(9);
	}

	// The same charge flicker phase 5 and phase 7 open with.
	if(boss_phase_frame < SIGMA_ORBIT_AIM_FRAME) {
		patnum_2064E = ((page_back * 4) + 128);
		return;
	}
	if(boss_phase_frame == SIGMA_ORBIT_AIM_FRAME) {
		// +16 on BOTH axes, which is half her sprite's WIDTH twice over and not
		// the player's centre on y -- the same ZUN quirk phase 9's three
		// patterns spell out.
		sigma_orbit_center_x = player_center_x();
		sigma_orbit_center_y = (player_topleft.y + (PLAYER_W / 2));

		sigma_cel_interval_mask = SIGMA_CEL_INTERVAL_FAST;
		sigma_blast_hitbox_margin = SIGMA_BLAST_HITBOX_MARGIN_WIDE;
		sigma_orbit_angle = 0;
		patnum_2064E = 128;
	}

	// The blink, on both pages and therefore visible for two frames out of
	// eight. Two arms with two literal colours rather than one arm with a colour
	// variable: -O cross-jumps the shared tail down to one `call` and one
	// grcg_circle(), which is what the original has, while a variable would turn
	// the packed 32-bit argument push into a register push.
	if(boss_phase_frame < SIGMA_ORBIT_BLINK_PAST_LAST) {
		if((boss_phase_frame % SIGMA_ORBIT_BLINK_INTERVAL) == 0) {
			grcg_setcolor(GC_RMW, V_WHITE);
			grcg_circle(
				sigma_orbit_center_x, sigma_orbit_center_y, SIGMA_ORBIT_RADIUS
			);
		} else if(
			(boss_phase_frame % SIGMA_ORBIT_BLINK_INTERVAL) ==
			SIGMA_ORBIT_BLINK_ERASE_AT
		) {
			grcg_setcolor(GC_RMW, 0);
			grcg_circle(
				sigma_orbit_center_x, sigma_orbit_center_y, SIGMA_ORBIT_RADIUS
			);
		}
		grcg_off();
		return;
	}
	if(boss_phase_frame < SIGMA_ORBIT_PAST_LAST_FRAME) {
		// Held on page 0 only from here on, so it stops blinking and just sits
		// there for the rest of the pattern.
		if(page_back == 0) {
			grcg_setcolor(GC_RMW, V_WHITE);
			grcg_circle(
				sigma_orbit_center_x, sigma_orbit_center_y, SIGMA_ORBIT_RADIUS
			);
			grcg_off();
		}
		if(boss_phase_frame < SIGMA_ORBIT_BLAST_FIRST_FRAME) {
			return;
		}
		if((boss_phase_frame % SIGMA_ORBIT_BLAST_INTERVAL) == 0) {
			// The polar placement idiom this binary already matched at
			// th02/main/boss/b4.cpp for Marisa's swoop: the table entry is
			// widened to `long` FIRST, so the multiply is 32-bit and the shift
			// is an arithmetic one.
			blast_x = (((CosTable8[sigma_orbit_angle] *
				(long)(SIGMA_ORBIT_RADIUS)) >> 8) + sigma_orbit_center_x);
			blast_y = (((SinTable8[sigma_orbit_angle] *
				(long)(SIGMA_ORBIT_RADIUS)) >> 8) + sigma_orbit_center_y);
			sigma_blasts_add(blast_x, blast_y, SIGMA_ORBIT_BLAST_RADIUS_MAX);

			angle_antipodal = (sigma_orbit_angle + SIGMA_ORBIT_ANTIPODE);
			blast_x = (((CosTable8[angle_antipodal] *
				(long)(SIGMA_ORBIT_RADIUS)) >> 8) + sigma_orbit_center_x);
			blast_y = (((SinTable8[angle_antipodal] *
				(long)(SIGMA_ORBIT_RADIUS)) >> 8) + sigma_orbit_center_y);
			sigma_blasts_add(blast_x, blast_y, SIGMA_ORBIT_BLAST_RADIUS_MAX);
			sigma_orbit_angle += SIGMA_ORBIT_ANGLE_STEP;
		}
		if(boss_phase_frame < SIGMA_ORBIT_RING_FIRST_FRAME) {
			return;
		}
		if((boss_phase_frame & 0x0F) == 0) {
			bullets_add_pellet(
				sigma_center_x,
				sigma_center_y,
				randring2_next8(),
				BG_32_RING,
				(5 << 4)
			);
		}
	} else if(page_back == 0) {
		// The one erase that sticks, and the pattern's own restart.
		grcg_setcolor(GC_RMW, 0);
		grcg_circle(
			sigma_orbit_center_x, sigma_orbit_center_y, SIGMA_ORBIT_RADIUS
		);
		grcg_off();
		boss_phase_frame = 0;
	}
}

/// Phase 3 pattern 1: sigma_move_weave()'s walk with a 16-way ring at a random
/// angle every 8th frame, and nothing else. The shortest body in her chain.
extern "C" void near sigma_16176(void)
{
	if(sigma_move_weave()) {
		return;
	}
	if((boss_phase_frame & 7) == 0) {
		bullets_add_pellet(
			sigma_center_x,
			sigma_center_y,
			randring2_next8(),
			BG_16_RING,
			((3 << 4) + 2)
		);
	}
}
/// ----------------------

/// Phase 5's first pattern
/// -----------------------
/// [sigma_phase] 5's slot 0, and the other half of the phase sigma_162D3()
/// closes. This one moves nothing: she holds still for it, and the pattern is
/// carried entirely by a pair of bouncing 16x16 billiard balls walking inward
/// from the two edges of the playfield.

// The x of the LEFT ball of the mirrored pair, walked one bullet width to the
// right every 16 frames from PLAYFIELD_LEFT. The right ball's x is derived from
// it at the spawn site rather than stored, which is why only one of the two has
// a variable at all -- `mirror` is this tree's word for the derived member of a
// symmetric pair (after STONES_LASER_MIRROR in th02/main/boss/b3.cpp) and there
// is nothing here to name with it.
extern "C" screen_x_t sigma_billiard_left;

// Which of the two walks is on screen, 0 then 1, and it is also the SPRITE: the
// spawn adds it to PAT_BULLET16_BILLIARD_BALL_RED, whose successor in
// th02/sprites/main_pat.h is PAT_BULLET16_BILLIARD_BALL_PURPLE. So the first
// walk is red and the second purple, from one counter.
//
// "volley" rather than "sweep", and that is a collision the census caught rather
// than a preference: [sigma_sweep_velocity_x] already spends `sweep` on her
// horizontal MOVEMENT in phase 9, one variable away in the same _BSS block, and
// `<x>_sweep` twice in one boss with two unrelated meanings is exactly the trap
// state/notes/sigma_update.md's own naming census records for her ball ring's
// angle, where the shorter name it rejected would have paired itself with the
// SIGMA_RING_ANGLE_STEP constants of a completely different effect. `volley` is
// attested for a burst of shots in this tree by marisa_volleys_fired,
// kurumi_volley and KURUMI_VOLLEY_ARMS_MAX.
//
// `uint8_t` and not `int8_t`, which is measurable rather than a habit: the two
// reads are `mov al` + `mov ah, 0`, and the bound test is `cmp` + `jb`
// (kb/codegen/0029).
extern "C" uint8_t sigma_billiard_volley;

// The frame the walk is seeded on, and see SIGMA_LASER_ARM_FRAME above for why
// this is its own constant and not that one.
static const int SIGMA_BILLIARD_SEED_FRAME = 100;

// How many walks the pattern runs before restarting the phase's cycle. Also the
// number of billiard-ball sprites it steps through; see
// [sigma_billiard_volley].
static const int SIGMA_BILLIARD_VOLLEYS = 2;

/// Phase 5 pattern 0: a 50-frame charge flicker, then a mirrored pair of
/// bouncing billiard balls every 16 frames, walking inward from the playfield
/// edges -- and, on the single frame the left one reaches the playfield centre,
/// the five-laser volley her other phase-5 pattern fires from a table.
extern "C" void near sigma_1619C(void)
{
	if(boss_phase_frame < SIGMA_MOVE_HOLD_FRAMES) {
		return;
	}
	if(boss_phase_frame == SIGMA_MOVE_HOLD_FRAMES) {
		snd_se_play(9);
	}

	// The same charge flicker sigma_16421() opens with, down to the 128 base and
	// the missing restore -- the seed frame below is what puts [patnum_2064E]
	// back.
	if(boss_phase_frame < SIGMA_BILLIARD_SEED_FRAME) {
		patnum_2064E = ((page_back * 4) + 128);
		return;
	}
	if(boss_phase_frame == SIGMA_BILLIARD_SEED_FRAME) {
		patnum_2064E = 128;
		sigma_billiard_left = PLAYFIELD_LEFT;
		sigma_billiard_volley = 0;

		// So each ball bounces exactly TWICE before it expires
		// (th02/main/bullet/bullet.hpp: BSM_BOUNCE_* resets to BG_NONE after
		// [turns_max] + 1 turns). Global rather than per-bullet, and never
		// restored, so it is still 1 for whatever spawns next -- one of the two
		// reasons phase 5's patterns are not independent of each other.
		bullet_special.u3.turns_max = 1;
	}
	if((boss_phase_frame & 0x0F) != 0) {
		return;
	}
	bullets_add_16x16(
		sigma_billiard_left,
		(PLAYFIELD_TOP + 8),
		0x40,
		BSM_BOUNCE_LEFT_RIGHT_TOP_BOTTOM,
		static_cast<main_patnum_t>(
			PAT_BULLET16_BILLIARD_BALL_RED + sigma_billiard_volley
		),
		(4 << 4)
	);

	// The mirror, derived rather than stored. `[measured]` The reflection axis
	// is PLAYFIELD_RIGHT + BULLET16_W and not PLAYFIELD_RIGHT: the ball is
	// spawned by its LEFT edge, so mirroring the left edge of a 16-pixel sprite
	// needs the extra width to put its RIGHT edge where the left one started.
	bullets_add_16x16(
		((PLAYFIELD_RIGHT + BULLET16_W) - sigma_billiard_left),
		(PLAYFIELD_TOP + 8),
		0x40,
		BSM_BOUNCE_LEFT_RIGHT_TOP_BOTTOM,
		static_cast<main_patnum_t>(
			PAT_BULLET16_BILLIARD_BALL_RED + sigma_billiard_volley
		),
		(4 << 4)
	);
	sigma_billiard_left += BULLET16_W;

	// The five vertical lasers, on the ONE frame the left ball's x is exactly
	// the playfield centre -- an equality test on a walk that steps BULLET16_W
	// at a time, so it fires only because PLAYFIELD_W / 2 happens to be a
	// multiple of that step. The same five muzzle offsets sigma_162D3() reads
	// out of _SIGMA_LASER_X_OFFSETS, in the same order, but spelled as five
	// immediates here, so this half of phase 5 owes that _DATA template nothing.
	//
	// [laser_wait_frames] is a spawn-time template rather than an argument
	// (th02/main/laser.hpp), so each write covers every lasers_add() after it
	// until the next one: 0x20 for the first, 0x30 for the second and third,
	// 0x64 for the fourth and fifth. `[measured]` The 0x10 at the end is NOT the
	// value lasers_reset() restores by coincidence -- it is the same 16 -- but
	// this pattern restores it by hand and sigma_162D3() writes the variable
	// nowhere at all, so the four lasers IT spawns charge for whatever this
	// left behind. Bare literals rather than named constants, which is how
	// th02/main/boss/b3.cpp already spells the stones' six writes of the same
	// variable in this same binary.
	if(sigma_billiard_left == (PLAYFIELD_LEFT + (PLAYFIELD_W / 2))) {
		laser_wait_frames = 0x20;
		lasers_add((sigma_topleft.x + 60), sigma_center_y, 1, 0x6F);
		laser_wait_frames = 0x30;
		lasers_add((sigma_topleft.x + 44), sigma_center_y, 1, 0x6F);
		lasers_add((sigma_topleft.x + 76), sigma_center_y, 1, 0x6F);
		laser_wait_frames = 0x64;
		lasers_add((sigma_topleft.x + 28), sigma_center_y, 1, 0x6F);
		lasers_add((sigma_topleft.x + 92), sigma_center_y, 1, 0x6F);
		laser_wait_frames = 0x10;
	}
	if(sigma_billiard_left >= (PLAYFIELD_RIGHT + BULLET16_W)) {
		sigma_billiard_left = PLAYFIELD_LEFT;

		// `++` and not `+= 1`: kb/codegen/0094's first discriminator, and the
		// original takes the dedicated `inc mem` form.
		sigma_billiard_volley++;
		if(sigma_billiard_volley >= SIGMA_BILLIARD_VOLLEYS) {
			boss_phase_frame = 0;
		}
	}
}
/// -----------------------

/// Phase 5's second pattern
/// ------------------------
/// [sigma_phase] 5's slot 1, over sigma_move_weave()'s four-leg walk: two laser
/// volleys through five fixed muzzles, with a bouncing billiard fan between
/// them.

// The five muzzle x offsets from her sprite's left edge, walked one per laser.
//
// THE TEMPLATE STAYS IN THE ROOT ASM'S _DATA, and this is kb/codegen/0084's
// situation with the escape it usually leaves open closed off. It is the
// initializer of a LOCAL aggregate, so Turbo C++ would re-emit it into this
// object's own _DATA and copy it in from there -- except that `[measured
// 2026-08-22]` EVERY C++ OBJECT IN TH02 MAIN CONTRIBUTES ZERO BYTES TO _DATA:
// obj/th02/main.map gives th02_main.asm the whole `1DA7:0090 13D2` and each of
// the objects after it `1DA7:1462 0000`. So the first object to emit any would
// start at 1DA7:1462, while this template lives at 1DA7:13AE -- 0xB4 earlier,
// inside the dump's own contribution, where giving up its ten bytes would shift
// every later _DATA byte. Re-deriving this object's _TEXT parity
// (kb/codegen/0160) says nothing about that: it is a _DATA placement, not an
// alignment.
//
// So the storage keeps its address in the dump, gains a publish alias there,
// and is copied in through the ordinary struct assignment below.
//
// SCREAMING_CASE because it is const data, which is how _GAME_CLEAR_CONSTANTS
// and _EXTRA_CLEAR_FLAGS are already published in that same _DATA block, four
// labels above this one. sigma_1619C, still ASM one proc up, fires the same five
// offsets from five separate immediates rather than from this table.
static const int SIGMA_LASER_COUNT = 5;

struct sigma_laser_x_offsets_t {
	pixel_t offset[SIGMA_LASER_COUNT];
};
extern "C" const sigma_laser_x_offsets_t SIGMA_LASER_X_OFFSETS;

// Which muzzle the next laser of the current volley comes out of. `[measured]`
// A SIGNED byte, and the array index is where it shows: all four reads are
// `mov al` + `cbw`, not `mov ah, 0`.
extern "C" int8_t sigma_laser_i;

// The frame the muzzle walk is re-armed on, before the first volley.
//
// `[measured]` 100 is also SIGMA_STREAM_AIM_FRAME (phase 7) and the setup frame
// of sigma_15F95 and sigma_1619C, so four of her patterns share the number --
// but folding them into one constant is a ruling over five call sites and three
// of them are still ASM. Made once, when the last of those lands here.
static const int SIGMA_LASER_ARM_FRAME = 100;

// The six frames the two volleys fire on, and the counts are not laid out the
// way the code makes them look: 1, 2 and 2 lasers at 114, 118 and 122, then the
// same counts again at 242, 246 and 250. `[measured]` The arms for 118 and 246
// FALL THROUGH into the one-laser arm instead of duplicating it, and 122 and 250
// share one body outright, so ten lasers come out of four lasers_add() call
// sites.
static const int SIGMA_LASER_VOLLEY_1_FRAME_1 = 114;
static const int SIGMA_LASER_VOLLEY_1_FRAME_2 = 118;
static const int SIGMA_LASER_VOLLEY_1_FRAME_3 = 122;
static const int SIGMA_LASER_VOLLEY_2_FRAME_1 = 242;
static const int SIGMA_LASER_VOLLEY_2_FRAME_2 = 246;
static const int SIGMA_LASER_VOLLEY_2_FRAME_3 = 250;

// `[measured]` This pattern writes [laser_wait_frames] NOWHERE, so its ten
// lasers charge for however long the last entity to touch that variable left
// behind -- and th02/main/laser.hpp's own census names this proc as the one
// spawner that does exactly that. On any normal run the previous writer is
// sigma_1619C, phase 5's OTHER pattern, which leaves 0x10 behind after setting
// 0x20, 0x30, 0x30, 0x64 and 0x64 for its own five. A ZUN quirk to preserve,
// and the reason the two halves of phase 5 are not independent even though they
// share no variable of their own.
static const int SIGMA_LASER_ACTIVE_FRAMES = 1;
static const int SIGMA_LASER_PATNUM_BASE = 0x6F;

// The bouncing billiard fan between the volleys: six balls an eighth of a turn
// apart, alternating two sprites. `[measured]` The loop bound is an UNSIGNED
// byte compare (`cmp` + `jb` against -10h), so 0xF0 is one step past the last
// angle rather than a negative one.
static const int SIGMA_FAN_FIRST_FRAME = 120;
static const int SIGMA_FAN_LAST_FRAME = 240;
static const uint8_t SIGMA_FAN_ANGLE_FIRST = 0x90;
static const uint8_t SIGMA_FAN_ANGLE_PAST_LAST = 0xF0;
// `int` and not the `uint8_t` this really is, and that is kb/codegen/0094's
// second discriminator rather than a typo: for a compound assignment Turbo C++
// folds a BYTE-typed addend into `add mem, imm8` and forces an AL round trip
// for an int-typed one, and the original takes the round trip. Measured here
// for a STACK LOCAL, which 0094 had only measured for a global and a struct
// member; the fold cost 4 of this body's 0x14E bytes on the first screen.
static const int SIGMA_FAN_ANGLE_STEP = 0x10;
static const int SIGMA_FAN_PATNUMS = 2;

/// Phase 5 pattern 1: two laser volleys through five fixed muzzles with a
/// bouncing billiard fan between them, over sigma_move_weave()'s four-leg walk.
extern "C" void near sigma_162D3(void)
{
	// Declared in the original's own order, which is what puts the ten-byte
	// array at the TOP of the frame (kb/codegen/0010). [i] is last and is a
	// plain `int` rather than a `register` one, because the original keeps it at
	// [bp-0Eh]: SI and DI are both spoken for by the struct copy.
	sigma_laser_x_offsets_t laser_x_offsets;
	uint8_t angle;
	int i;

	// A plain copy assignment, and it has to be the FIRST statement: the
	// original does the copy before anything else, in the SI / DI / ES / CX
	// order that kb/codegen/0109's table attributes to exactly this -- `-G` plus
	// a whole-struct assignment, which is also the one of that table's four
	// orders no `decomp.hpp` macro spells. `-G` was already set on this object
	// for sigma_update(), so nothing had to change for it. `push ss` rather than
	// `push ds` for the ES half only because the DESTINATION is a stack local.
	laser_x_offsets = SIGMA_LASER_X_OFFSETS;

	if(sigma_move_weave()) {
		return;
	}
	if(boss_phase_frame == SIGMA_LASER_ARM_FRAME) {
		sigma_laser_i = 0;
		return;
	}

	// ZUN wrote six arms and -O cross-jumped the duplicates, so the two
	// two-laser arms share one body and the two one-laser arms share another.
	// Written as the `goto`s -O chose rather than as six plain arms, in ZUN's
	// frame order, so that every `cmp` lands where the original has it -- the
	// same hand-merge sigma_update() needed three parcels earlier, and for the
	// same reason: an arm that only falls through cannot be spelled any other
	// way.
	if(boss_phase_frame == SIGMA_LASER_VOLLEY_1_FRAME_1) {
		goto laser_1;
	} else if(boss_phase_frame == SIGMA_LASER_VOLLEY_1_FRAME_2) {
		goto laser_2;
	} else if(boss_phase_frame == SIGMA_LASER_VOLLEY_1_FRAME_3) {
		goto laser_2_and_rearm;
	} else if(boss_phase_frame == SIGMA_LASER_VOLLEY_2_FRAME_1) {
		goto laser_1;
	} else if(boss_phase_frame == SIGMA_LASER_VOLLEY_2_FRAME_2) {
laser_2:
		lasers_add(
			(laser_x_offsets.offset[sigma_laser_i++] + sigma_topleft.x),
			sigma_center_y,
			SIGMA_LASER_ACTIVE_FRAMES,
			SIGMA_LASER_PATNUM_BASE
		);
laser_1:
		lasers_add(
			(laser_x_offsets.offset[sigma_laser_i++] + sigma_topleft.x),
			sigma_center_y,
			SIGMA_LASER_ACTIVE_FRAMES,
			SIGMA_LASER_PATNUM_BASE
		);
		return;
	} else if(boss_phase_frame == SIGMA_LASER_VOLLEY_2_FRAME_3) {
laser_2_and_rearm:
		lasers_add(
			(laser_x_offsets.offset[sigma_laser_i++] + sigma_topleft.x),
			sigma_center_y,
			SIGMA_LASER_ACTIVE_FRAMES,
			SIGMA_LASER_PATNUM_BASE
		);

		// NO post-increment on this one, so the fifth muzzle is the last index
		// the volley reaches and the re-arm below is what makes the second
		// volley start from 0 again.
		lasers_add(
			(laser_x_offsets.offset[sigma_laser_i] + sigma_topleft.x),
			sigma_center_y,
			SIGMA_LASER_ACTIVE_FRAMES,
			SIGMA_LASER_PATNUM_BASE
		);
		sigma_laser_i = 0;
		return;
	}
	if(
		(boss_phase_frame > SIGMA_FAN_FIRST_FRAME) &&
		(boss_phase_frame < SIGMA_FAN_LAST_FRAME) &&
		((boss_phase_frame & 0x0F) == 0)
	) {
		angle = SIGMA_FAN_ANGLE_FIRST;
		i = 0;
		while(angle < SIGMA_FAN_ANGLE_PAST_LAST) {
			bullets_add_16x16(
				sigma_center_x,
				sigma_center_y,
				angle,
				BSM_BOUNCE_LEFT_RIGHT_TOP_BOTTOM,
				static_cast<main_patnum_t>(
					PAT_BULLET16_BILLIARD_BALL_RED + (i % SIGMA_FAN_PATNUMS)
				),
				((3 << 4) + 12)
			);
			angle += SIGMA_FAN_ANGLE_STEP;
			i++;
		}
	}
}
/// ------------------------

/// Phase 7's two patterns
/// ----------------------
/// [sigma_phase] 7's group. Both open by delegating to sigma_move_weave(), the
/// still-ASM helper above, and doing nothing until it says her entrance hold is
/// over.

// The frame sigma_16421() aims its streams on, and the length of the step it
// aims with.
static const int SIGMA_STREAM_AIM_FRAME = 100;
static const int SIGMA_STREAM_VELOCITY_LENGTH = 48;

// How far each stream's spawn point is nudged horizontally after every step.
//
// `[measured 2026-08-22]` AND THE TWO STREAMS ARE NOT MIRROR IMAGES OF EACH
// OTHER HERE, which is the surprise in this pattern and the reason this comment
// exists. For the same comparison against the player's x, the first stream is
// nudged TOWARD the player and the mirrored one AWAY from it -- `add` against
// `sub` in the dump, at both of the four sites. So the pair is mirrored only in
// its initial velocity; one homes and one flees, and the figure they draw is
// not symmetric at all. `mirror` is kept because the velocity really is the
// `neg` of the first, and the asymmetry is carried here rather than in the
// name.
static const pixel_t SIGMA_STREAM_CHASE_STEP = 16;

/// Phase 7 pattern 0: a 50-frame charge flicker, then two streams of expanding
/// blasts walking outward from her centre every 8 frames -- one chasing the
/// player and one fleeing, see SIGMA_STREAM_CHASE_STEP -- plus a 16-way ring at
/// a random angle on the same beat.
extern "C" void near sigma_16421(void)
{
	// `bool16` and not `bool`, because it holds this function's own copy of
	// sigma_blasts_add()'s inverted return and the original's local is a word.
	bool16 stream_rejected;

	if(boss_phase_frame < SIGMA_MOVE_HOLD_FRAMES) {
		return;
	}
	if(boss_phase_frame == SIGMA_MOVE_HOLD_FRAMES) {
		snd_se_play(9);
	}

	// The charge flicker: 128 on one page and 132 on the other, so she strobes
	// at the frame rate rather than animating. 128 is the base [patnum_2064E]
	// sigma_init() sets, and this arm restores nothing -- the aim frame below
	// puts it back.
	if(boss_phase_frame < SIGMA_STREAM_AIM_FRAME) {
		patnum_2064E = ((page_back * 4) + 128);
		return;
	}
	if(boss_phase_frame == SIGMA_STREAM_AIM_FRAME) {
		vector2_between_plus(
			sigma_center_x,
			sigma_center_y,
			player_center_x(),
			(player_topleft.y + (PLAYER_W / 2)),
			0,
			sigma_stream_velocity.x,
			sigma_stream_velocity.y,
			SIGMA_STREAM_VELOCITY_LENGTH
		);
		sigma_stream_mirror_velocity_x = -sigma_stream_velocity.x;
		patnum_2064E = 128;

		// Chained, and in this direction: the original loads her centre ONCE
		// and stores it to the first stream and then to the mirror, which is
		// what one expression assigned through two slots compiles to. Two
		// statements would load it twice.
		sigma_stream_mirror_x = sigma_stream_x = sigma_center_x;

		sigma_blast_hitbox_margin = SIGMA_BLAST_HITBOX_MARGIN_WIDE;
		sigma_cel_interval_mask = SIGMA_CEL_INTERVAL_FAST;
		sigma_stream_y = sigma_center_y;
	}
	if((boss_phase_frame & 7) == 0) {
		// BOTH spawns always happen; only the restart is conditional. The
		// second one's result is tested first because it is the one still in AX
		// -- which is why the first needs the local at all.
		stream_rejected = sigma_blasts_add(
			sigma_stream_x, sigma_stream_y, SIGMA_BLAST_RADIUS_MAX
		);
		if(sigma_blasts_add(
			sigma_stream_mirror_x, sigma_stream_y, SIGMA_BLAST_RADIUS_MAX
		) && stream_rejected) {
			// Both refused, so the pool is full or both points left the
			// playfield: restart the pattern rather than keep walking off
			// screen.
			boss_phase_frame = 0;
		}
		sigma_stream_x += sigma_stream_velocity.x;
		sigma_stream_mirror_x += sigma_stream_mirror_velocity_x;
		sigma_stream_y += sigma_stream_velocity.y;
		if(player_center_x() > sigma_stream_x) {
			sigma_stream_x += SIGMA_STREAM_CHASE_STEP;
		} else if(player_center_x() < sigma_stream_x) {
			sigma_stream_x -= SIGMA_STREAM_CHASE_STEP;
		}
		if(player_center_x() > sigma_stream_mirror_x) {
			sigma_stream_mirror_x -= SIGMA_STREAM_CHASE_STEP;
		} else if(player_center_x() < sigma_stream_mirror_x) {
			sigma_stream_mirror_x += SIGMA_STREAM_CHASE_STEP;
		}
		bullets_add_pellet(
			sigma_center_x,
			sigma_center_y,
			randring2_next8(),
			BG_16_RING,
			((3 << 4) + 12)
		);
	}
}

/// Phase 7 pattern 1: a blast on the player on the frames halfway through each
/// 64-frame cycle, and a 32-way ring at a random angle on the cycle boundary.
extern "C" void near sigma_16555(void)
{
	if(sigma_move_weave()) {
		return;
	}
	if(boss_phase_frame == SIGMA_MOVE_HOLD_FRAMES) {
		sigma_cel_interval_mask = SIGMA_CEL_INTERVAL_SLOW;
	}
	if((boss_phase_frame & 0x3F) == 32) {
		sigma_blasts_add(
			player_center_x(),
			(player_topleft.y + (PLAYER_W / 2)),
			SIGMA_BLAST_RADIUS_MAX
		);
	}
	if((boss_phase_frame & 0x3F) == 0) {
		bullets_add_pellet(
			sigma_center_x,
			sigma_center_y,
			randring2_next8(),
			BG_32_RING,
			((3 << 4) + 12)
		);
	}
}
/// ----------------------

/// Her final phase's three patterns, and the movement they share
/// -------------------------------------------------------------
/// [sigma_phase] 9's group, installed together by sigma_update() below. All
/// three open by delegating to the same movement helper and doing nothing until
/// it says she has finished her entrance hold, then each adds its own bullets on
/// its own frame cadence.

// Phase 9's sweep: 2 pixels per frame, latched signed once and then added,
// subtracted and added again by the three legs below.
static const pixel_t SIGMA_SWEEP_SPEED = 2;
static const int SIGMA_SWEEP_LEG_1_END = 130;
static const int SIGMA_SWEEP_LEG_2_END = 290;
static const int SIGMA_SWEEP_LEG_3_END = 370;

/// Phase 9's shared movement, and the answer "not yet" for the 50 frames before
/// it starts.
///
/// `static` and unnamed in the dump on purpose: this is the one proc in Sigma's
/// chain whose every caller is in this group, so it needs no kb/codegen/0123
/// alias and no `public` -- taking it in the same parcel as its three callers is
/// strictly cheaper than lifting them without it.
///
/// Named boss-plus-move-plus-qualifier, after shinki_move_float()
/// (th05/main/boss/b6.cpp) and midboss1_move() (th05/main/midboss/m1.cpp); the
/// qualifier is "sweep" after reimu_sweep_angle_delta
/// (th04/main/boss/b4r_upd.cpp), which establishes it as a boss-motion noun
/// here. An axis-letter suffix has no precedent in this tree, and dropping the
/// verb loses what every other movement helper carries.
///
/// `[measured]` sigma_move_weave(), at the top of this file, is its twin for
/// phases 1, 3, 5 and 7: same hold, same latch-once-on-frame-50 rule, but 1
/// pixel per frame and a vertical component on two of its three legs. It got the
/// same shape with a different qualifier when it landed, which is why this one
/// is not simply the unqualified move.
///
/// The direction is latched AWAY from the player, not toward: she gets the
/// positive step when the player is left of centre. Returning `true` while
/// holding is what lets each pattern open with a bare early-out.
static bool16 near sigma_move_sweep(void)
{
	if(boss_phase_frame < SIGMA_MOVE_HOLD_FRAMES) {
		return true;
	}
	if(boss_phase_frame == SIGMA_MOVE_HOLD_FRAMES) {
		// A conditional EXPRESSION, not an `if`/`else` with a store in each
		// arm, and that is 2 bytes rather than a style question: the original
		// loads each candidate into AL, jumps to a join, and stores AL to this
		// byte ONCE -- ten bytes, and what an assignment from a single
		// expression compiles to. An `if`/`else` puts an immediate store in
		// each arm instead, which is twelve. `tcc -S` reported exactly that
		// difference before the first full build of this parcel.
		sigma_sweep_velocity_x = ((player_topleft.x < PLAYER_LEFT_START)
			? SIGMA_SWEEP_SPEED
			: -SIGMA_SWEEP_SPEED
		);
	}

	// Legs 1 and 3 are the same statement, and -O cross-jumps them into one
	// copy by itself -- unlike the three arms of sigma_update()'s phase
	// dispatch, which it could not merge because those are inline-ASM islands
	// it cannot read. Written as four plain arms here, and the merge is the
	// compiler's.
	if(boss_phase_frame < SIGMA_SWEEP_LEG_1_END) {
		*boss_left_on_back_page += sigma_sweep_velocity_x;
	} else if(boss_phase_frame < SIGMA_SWEEP_LEG_2_END) {
		*boss_left_on_back_page -= sigma_sweep_velocity_x;
	} else if(boss_phase_frame < SIGMA_SWEEP_LEG_3_END) {
		*boss_left_on_back_page += sigma_sweep_velocity_x;
	} else {
		// And THIS is what sigma_166DE() below reads as "the pattern finished
		// its loop". The phase counter is untouched.
		boss_phase_frame = 0;
	}
	return false;
}

/// Phase 9 pattern 0: a blast on the player every 64 frames, and an ultrawide
/// aimed spread every 8.
extern "C" void near sigma_16606(void)
{
	if(sigma_move_sweep()) {
		return;
	}
	if(boss_phase_frame == SIGMA_MOVE_HOLD_FRAMES) {
		sigma_cel_interval_mask = SIGMA_CEL_INTERVAL_SLOW;
	}
	if((boss_phase_frame & 0x3F) == 0) {
		// `[measured]` +16 on BOTH axes, and PLAYER_H is 48, so the second one
		// is NOT the player's centre -- player_center_y()
		// (th02/main/player/player.hpp) would be +24. ZUN used half the WIDTH
		// on both axes. Spelled out rather than routed through that helper so
		// the quirk is visible at every one of the three call sites below.
		sigma_blasts_add(
			player_center_x(),
			(player_topleft.y + (PLAYER_W / 2)),
			SIGMA_BLAST_RADIUS_MAX
		);
	}
	if((boss_phase_frame & 7) == 0) {
		bullets_add_pellet(
			sigma_center_x,
			sigma_center_y,
			0,
			BG_2_SPREAD_ULTRAWIDE_AIMED,
			((4 << 4) + 6)
		);
	}
}

/// Phase 9 pattern 1: an 8-way ring of 16x16 balls every 8 frames, rotating an
/// eighth of a turn per ring.
extern "C" void near sigma_16650(void)
{
	if(sigma_move_sweep()) {
		return;
	}
	if(boss_phase_frame == SIGMA_MOVE_HOLD_FRAMES) {
		sigma_ball_ring_angle = 0;
	}
	if((boss_phase_frame & 7) == 0) {
		bullets_add_16x16(
			sigma_center_x,
			sigma_center_y,
			sigma_ball_ring_angle,
			BG_8_RING,
			PAT_BULLET16_BALL,
			(5 << 4)
		);

		// 8 of the 256-step circle, i.e. exactly one gap of the 8-way ring
		// divided by four, so the ring only lines up with itself every 4th
		// spawn. th02/main/boss/b3.cpp's stones_11BFE() steps its own ring
		// angle by the same 8 after the same call.
		sigma_ball_ring_angle += 8;
	}
}

/// Phase 9 pattern 2: a blast on the player every 32 frames, and a 16-way ring
/// at a random angle on the frames exactly between them.
extern "C" void near sigma_1668E(void)
{
	if(sigma_move_sweep()) {
		return;
	}
	if(boss_phase_frame == SIGMA_MOVE_HOLD_FRAMES) {
		sigma_cel_interval_mask = SIGMA_CEL_INTERVAL_SLOW;
	}
	if((boss_phase_frame & 0x1F) == 0) {
		sigma_blasts_add(
			player_center_x(),
			(player_topleft.y + (PLAYER_W / 2)),
			SIGMA_BLAST_RADIUS_MAX
		);
	}

	// Interleaved with the blast rather than sharing its test: 16 is half of
	// 0x20, so the ring always fires on the frame midway between two blasts.
	if((boss_phase_frame & 0x1F) == 16) {
		bullets_add_pellet(
			sigma_center_x,
			sigma_center_y,
			randring2_next8(),
			BG_16_RING,
			((3 << 4) + 12)
		);
	}
}
/// -------------------------------------------------------------

/// Her pattern runner: one of the current phase's [sigma_pattern_func] slots
/// per frame, then the two counters that end the pattern and end the phase.
/// Reached only from sigma_update() below, and `far` only because the original
/// declared it that way -- nothing outside this segment ever called it.
///
/// [pattern_count] is how many of the three slots the phase installed, and it
/// is PASSED rather than read off a global, which is what makes the two-pattern
/// and three-pattern phases share one runner. mima_18A1B()
/// (th02/main/boss/b5m.cpp) is the same shape one boss earlier.
extern "C" void far sigma_166DE(int pattern_count)
{
	// `switch` on the global with NO source local, because this function has
	// zero stack locals -- its prolog is a bare `push bp; mov bp, sp` -- and a
	// selector local would store the value a second time beside Turbo C++'s own
	// hidden switch temp (kb/codegen/0076). Three dense cases is under the
	// threshold for a generated table, so this emits a contiguous run of
	// compares and no `-a2` data; that is what keeps this object's SEGDEF at
	// `len 0x0` for `_DATA` and leaves the next lift into it free.
	//
	// And a `switch` rather than the shorter `sigma_pattern_func[sigma_pattern]()`,
	// which is NOT what the original does: it holds three separate `call word
	// ptr` instructions through the three slots by name. A computed index would
	// have emitted one indexed indirect call through a register.
	switch(sigma_pattern) {
	case 0:
		sigma_pattern_func[0]();
		break;
	case 1:
		sigma_pattern_func[1]();
		break;
	case 2:
		sigma_pattern_func[2]();
		break;
	}

	// `[measured]` NOT "the phase just started": each pattern's own movement
	// helper zeroes [boss_phase_frame] when its fixed frame schedule wraps, so
	// this fires once per completed pattern loop and the whole block below is
	// the end-of-pattern bookkeeping.
	if(boss_phase_frame == 0) {
		sigma_pattern++;
		if(sigma_pattern >= pattern_count) {
			sigma_pattern = 0;
			sigma_pattern_looped_unused = 1;
		}

		// Nested inside the same test rather than beside it, which is what puts
		// the damage compare after the wrap and lets both reach the single exit.
		if(boss_damage >= sigma_phase_damage_max) {
			boss_damage = 0;
			sigma_phase++;
			sigma_pattern = 0;
			sigma_pattern_looped_unused = 0;
		}
	}
}

// sigma_166DE() is `far` and lands in this very segment, so TLINK relaxed the
// original's plain `call` to it into `push cs; call near ptr` -- four bytes,
// and with NO `nop`, because the dump reaches it with a bare `call` rather
// than through ReC98.inc's `nopcall` macro, which is what adds that byte. No
// C++ far call emits either form (kb/codegen/0083), and inline ASM cannot see
// C++ expressions, so the argument and the __cdecl cleanup are spelled with
// it (kb/codegen/0122).
//
// __emit__() for the push rather than `_asm { push n }`, because the inline
// assembler is free to pick the 3-byte `68 imm16` form; and this names no
// register, so [i] in sigma_update() stays in SI, exactly as
// nopcall_same_group() leaves mima_update()'s.
#define call_same_group_cdecl_1(func, arg) { \
	__emit__(0x6A, arg); \
	__emit__(0x0E); \
	_asm { call near ptr func; } \
	__emit__(0x83, 0xC4, 0x02); \
}

/// Her per-frame update. Installed into [boss_update] by stage_init()
/// (th02/main/stage/init.cpp). mima_update() in th02/main/boss/b5m.cpp is the
/// same shape one boss earlier, down to the dot-square-ring loop.
///
/// Through an `int` rather than [stage_progression_t]: -b- sizes a three-value
/// enum as a `char`, and the original returns in AX. mima_update() and
/// marisa_update() are declared the same way.
extern "C" int far sigma_update(void)
{
	// [radius] first: Turbo C++ allocates stack locals in declaration order,
	// and it is the original's only one, at [bp-1]. (kb/codegen/0010)
	unsigned char radius;
	register int i;

	sigma_topleft.x = *boss_left_on_back_page;
	sigma_topleft.y = *boss_top_on_back_page;
	sigma_center_x = (sigma_topleft.x + SIGMA_CENTER_OFFSET);
	sigma_center_y = (sigma_topleft.y + SIGMA_CENTER_OFFSET);
	boss_phase_frame++;
	boss_pos_x = sigma_center_x;
	boss_pos_y = sigma_center_y;
	if((stage_frame & 1) == 0) {
		sigma_ring_radius += SIGMA_RING_RADIUS_STEP;
		grcg_setcolor(GC_RMW, SIGMA_RING_COL);

		// [i] is initialized ahead of [radius], and the increment is a
		// statement of its own rather than a loop-header increment, for the
		// reason mima_update() spells out for the identical loop: that is what
		// puts `inc si` ahead of the radius update and lets -O merge both
		// stores to [radius] into the shared test block.
		i = 0;
		radius = sigma_ring_radius;
		while(i < SIGMA_RING_COUNT) {
			dot_square_ring_put(
				SIGMA_RING_CENTER_X,
				SIGMA_RING_CENTER_Y,
				radius,
				((reduce_effects *
					(SIGMA_RING_ANGLE_STEP_REDUCED - SIGMA_RING_ANGLE_STEP)
				) + SIGMA_RING_ANGLE_STEP)
			);
			i++;
			radius += SIGMA_RING_DISTANCE;
		}
		grcg_off();
	}
	if(boss_phase) {
		if(sigma_15A25()) {
			return SP_CLEAR;
		}
	} else {
		if(sigma_phase == 0) {
			if(boss_phase_frame > SIGMA_INTRO_FRAMES) {
				sigma_pattern = 0;
				boss_phase_frame = 0;
				sigma_phase++;
				sigma_pattern_looped_unused = 0;
				sigma_pattern_func[0] = sigma_15D56;
				sigma_pattern_func[1] = sigma_15E84;
				sigma_pattern_func[2] = sigma_15F6F;
				sigma_phase_damage_max = SIGMA_PHASE_DAMAGE;
			}
		} else if(sigma_phase == 1) {
			call_same_group_cdecl_1(sigma_166DE, 3);
		} else if(sigma_phase == 2) {
			sigma_pattern = 0;
			boss_phase_frame = 0;
			sigma_phase++;
			sigma_pattern_looped_unused = 0;
			sigma_pattern_func[0] = sigma_15F95;
			sigma_pattern_func[1] = sigma_16176;
			sigma_phase_damage_max = SIGMA_PHASE_DAMAGE;
		// THE THREE `goto`s BELOW ARE A MERGE -O CANNOT PERFORM ITSELF, and
		// they are the shape of this parcel's one build cycle.
		//
		// ZUN wrote ten plain arms, three of which run two patterns and are
		// therefore identical; -O cross-jumped those three into one copy, kept
		// the LAST of them, and reached it with two `je short`s from the
		// earlier two. `[measured 2026-08-22]` Written as ten plain arms here,
		// -O leaves all three copies standing and the function comes out
		// 0x23F instead of 0x227: the bodies are inline-ASM islands
		// (kb/codegen/0083), and the cross-jump optimiser will not merge
		// blocks it cannot read. So the merge is spelled by hand, in the same
		// direction -O chose - into the last of the three - and the arms stay
		// in ZUN's order so that every `cmp` lands where the original has it.
		} else if(sigma_phase == 3) {
			goto run_two_patterns;
		} else if(sigma_phase == 4) {
			sigma_pattern = 0;
			boss_phase_frame = 0;
			sigma_phase++;
			sigma_pattern_looped_unused = 0;
			sigma_pattern_func[0] = sigma_1619C;
			sigma_pattern_func[1] = sigma_162D3;
			sigma_phase_damage_max = SIGMA_PHASE_DAMAGE;
		} else if(sigma_phase == 5) {
			goto run_two_patterns;
		} else if(sigma_phase == 6) {
			sigma_pattern = 0;
			boss_phase_frame = 0;
			sigma_phase++;
			sigma_pattern_looped_unused = 0;
			sigma_pattern_func[0] = sigma_16421;
			sigma_pattern_func[1] = sigma_16555;
			sigma_phase_damage_max = SIGMA_PHASE_DAMAGE;
		} else if(sigma_phase == 7) {
run_two_patterns:
			call_same_group_cdecl_1(sigma_166DE, 2);
		} else if(sigma_phase == 8) {
			sigma_pattern = 0;
			boss_phase_frame = 0;
			sigma_phase++;
			sigma_pattern_looped_unused = 0;
			sigma_pattern_func[0] = sigma_16606;
			sigma_pattern_func[1] = sigma_16650;
			sigma_pattern_func[2] = sigma_1668E;
			sigma_phase_damage_max = SIGMA_FINAL_PHASE_DAMAGE;
		} else if(sigma_phase == 9) {
			call_same_group_cdecl_1(sigma_166DE, 3);
			if(boss_damage >= SIGMA_DEFEAT_DAMAGE) {
				boss_phase = 1;
				score += SIGMA_DEFEAT_SCORE;
				boss_phase_frame = 0;
			}
		}

		// Again, and from the same two pointers: the pattern that just ran may
		// have moved her.
		sigma_topleft.x = *boss_left_on_back_page;
		sigma_topleft.y = *boss_top_on_back_page;
		sigma_15907();
		sigma_frame++;
	}
	sigma_1566F();
	return SP_BOSS;
}

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
