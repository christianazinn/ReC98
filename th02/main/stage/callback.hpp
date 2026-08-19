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
// __pascal callbacks (platform.h's farfunc_t_near): th02/main/null.asm
// publishes the defaults as `@nullfunc_void$qv` / `@nullfunc_false$qv` in
// lower case, which is Borland's __cdecl decoration (kb/codegen/0086).
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

extern void (far *enemies_invalidate)(void);
extern void (far *enemies_update_and_render)(void);

// Per-stage foreground/background effects. TH04 and TH05 declare the same pair
// of per-stage slots as stage_invalidate / stage_render in
// th04/main/stage/stage.hpp; TH02 runs one combined update-and-render pass,
// hence the second name.
extern void (far *stage_invalidate)(void);
extern void (far *stage_update_and_render)(void);

// The vertical boss lasers' per-frame pair. Only installed for Stage 4 and
// Extra, by lasers_callbacks_set() in th02/main/laser.cpp, which is the only
// writer of either slot in the whole binary and always installs exactly these
// two; stage_init() defaults both to nullfunc_void for every other stage.
// (That function was still ASM when this comment was first written, and was
// named for its dump placeholder; it has since been decompiled.)
// `_func` disambiguates the slot from the installed function, the way
// boss_bg_render_func does from boss_bg_render above.
extern void (far *lasers_invalidate_func)(void);
extern void (far *lasers_update_and_render_func)(void);

// Starts the boss fight once the map has been scrolled to its end, then
// disables itself.
extern void (far *boss_activate_if_scroll_done)(void);

// Returns `true` once the stage is over, which ends stage_loop().
// The slot's default installed function is `bool nullfunc_false(void)`, and
// the sibling slot above is a `bool` too, but this one has to be [bool16]:
// ZUN's code tests the result with `or ax, ax`, and a `bool` return compiles
// to `or al, al` — a real behavioral difference for any installed function
// that returns a nonzero high byte. (kb/codegen/0090)
extern bool16 (far *stage_should_end)(void);
