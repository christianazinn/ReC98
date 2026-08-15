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

// Erases the stage title from TRAM at [stage_frame] == 160, then disables
// itself.
extern void (far *stage_title_unput)(void);

extern void (far *enemies_invalidate)(void);
extern void (far *enemies_update_and_render)(void);

// Per-stage foreground/background effects. TH04 and TH05 declare the same pair
// of per-stage slots as stage_invalidate / stage_render in
// th04/main/stage/stage.hpp; TH02 runs one combined update-and-render pass,
// hence the second name.
extern void (far *stage_invalidate)(void);
extern void (far *stage_update_and_render)(void);

// Only installed for Stage 4 and Extra. What they render is not evidenced,
// hence the neutral names. [static]
extern void (far *farfp_23A72)(void);
extern void (far *farfp_23A76)(void);

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
