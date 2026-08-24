/* ReC98
 * -----
 * TH02's per-stage callback slots: the far function pointers that
 * stage_init() installs once per stage and stage_loop() calls once per frame.
 *
 * Include this *after* th02/main/stage/stage.hpp, which defines the
 * [stage_progression_t] that boss_update() returns. Neither header has an
 * include guard.
 */

#include "platform.h"

// Every function TH02 installs into these is __cdecl, unlike TH03-TH05's
// __pascal callbacks (platform.h's farfunc_t_near). Before MAIN_01___TEXT went
// fully C++, the default symbols in the module included at
// `6654b796:th02_main.asm:323` used Borland's lower-case __cdecl decoration
// (kb/codegen/0086); that is also what
// th02/main/bgm_show.cpp's definitions emit now.
//
// For a parameterless call the two conventions emit identical bytes, so
// declaring these as farfunc_t_near also builds, links, and matches — which is
// exactly why the two translation units below disagreed about them for three
// parcels without anything catching it. Only this spelling can be assigned to
// without a cast, and it is the one the ASM actually publishes, so it is the
// one that belongs in a header both TUs share.

extern void (far *boss_bg_render)(void);
extern stage_progression_t (far *boss_update)(void);
extern void (far *boss_bg_render_func)(void);
extern stage_progression_t (far *boss_update_func)(void);
extern void (far *boss_init)(void);
extern void (far *boss_end)(void);

// The slot for stage_title_unput(), which erases the stage title from TRAM at
// [stage_frame] == 160 and then disables itself by nulling this pointer.
// `_func` on the slot rather than on the function, exactly as stage_loop_func
// does for stage_loop() (th02/main/stage/init.cpp) — the same convention the
// lasers pair below already spells out.
extern void (far *stage_title_unput_func)(void);

// The slot for enemies_invalidate(), which is th02/main/enemy/enemies.cpp
// under th02/boss_5.cpp. `_func` for the same reason [stage_title_unput_func]
// has it: a function of the bare name now exists, and with C linkage on both
// there is no spelling of the pair that does not collide -- the slot's alias
// in th02_main.asm and the function's own `public` would both be
// `_enemies_invalidate`. TH04 and TH05 spell their own function of this name
// the same way (th04/main/enemy/inv.cpp).
//
// [enemies_update_and_render_func] took the suffix one parcel later, for
// exactly the reason predicted here: its function is now C++ too, in
// th02/main/enemy/update.cpp.
extern void (far *enemies_invalidate_func)(void);
extern void (far *enemies_update_and_render_func)(void);

// Per-stage foreground/background effects. TH04 and TH05 declare the same pair
// of per-stage slots as stage_invalidate / stage_render in
// th04/main/stage/stage.hpp; TH02 runs one combined update-and-render pass,
// hence the second name.
extern void (far *stage_invalidate)(void);
extern void (far *stage_update_and_render)(void);

// The vertical boss lasers' per-frame pair. Only installed for Stage 4 and
// Extra, by lasers_callbacks_set() in th02/main/laser.cpp, which is the only
// writer that installs the REAL pair -- it always installs exactly these two --
// while th02/main/stage/init.cpp writes both slots with nullfunc_void for
// every other stage. The scope limiter matters: without it the sentence
// contradicts its own next clause.
// (That function was still ASM when this comment was first written, and was
// named for its dump placeholder; it has since been decompiled.)
// `_func` disambiguates the slot from the installed function.
//
// THE SUFFIX MEANS TWO DIFFERENT THINGS IN THIS HEADER, so read it per slot
// rather than by pattern. On this pair, on [stage_title_unput_func],
// [boss_activate_if_scroll_done_func] and [stage_should_end_func], `_func`
// separates the slot from a function of the same name. On
// [boss_bg_render_func] / [boss_update_func] it does not: those are STAGED
// values that stage_init() picks per stage and that
// boss_activate_if_scroll_done() promotes into the bare [boss_bg_render] /
// [boss_update] on the frame the fight starts, so there both names are slots.
extern void (far *lasers_invalidate_func)(void);
extern void (far *lasers_update_and_render_func)(void);

// The slot for boss_activate_if_scroll_done(), which starts the boss fight
// once the map has been scrolled to its end and then nulls this pointer. The
// bare name belongs to the function, which is now C++ in
// th02/main/bgm_show.cpp.
//
// This one identifier is 33 characters, and Turbo C++ only sees the first 32,
// so the external it emits is `_boss_activate_if_scroll_done_fun` and that is
// the spelling th02_main.asm has to publish (kb/codegen/0060). Renaming
// anything here means re-checking that truncation.
extern void (far *boss_activate_if_scroll_done_func)(void);

// The slot for stage_should_end(), whose `true` ends stage_loop().
// boss_activate_if_scroll_done() installs the real one for the duration of the
// boss fight; the rest of the time it holds nullfunc_false(), which defaults
// straight in — no cast, because that function returns [bool16] too.
//
// The slot is [bool16] rather than `bool` because ZUN's code tests the result
// with `or ax, ax`, and a `bool` return compiles to `or al, al` — a real
// behavioral difference for any installed function that returns a nonzero
// high byte. (kb/codegen/0090). nullfunc_false()'s own body agrees: it zeroes
// the whole of AX. TWO slots still need the cast, not the one this comment
// used to claim: [midboss_invalidate], which is byte-wide for its own reasons,
// and [boss_update], which returns [stage_progression_t]. Both casts are in
// th02/main/stage/init.cpp; [stage_should_end_func] is the only one of the
// three that takes nullfunc_false() unqualified.
extern bool16 (far *stage_should_end_func)(void);
