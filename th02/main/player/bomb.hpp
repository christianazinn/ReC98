#include "platform.h"
#include "pc98.h"

static const unsigned char BOMB_CIRCLE_FRAMES = 32;

extern bool bombing;

#if (GAME == 2)
// TH04 and TH05 include this header for [BOMB_CIRCLE_FRAMES] and [bombing],
// but redeclare everything below with their own types in
// th04/main/player/bomb.hpp.

// Frame counter for the bomb animation, restarted at 0 for both the initial
// circle and the shottype-specific animation that follows it.
extern int bomb_frame;

// The expanding ring of particles that every bomb starts with.
extern point_t bomb_circle_center;
extern int bomb_circle_frame;
extern bool16 bomb_circle_done;

// The [resident->shottype]-specific bomb animation, selected by bomb_load().
// Returns true once the bomb is over.
typedef bool16 (near pascal *near playchar_bomb_func_t)(void);
extern playchar_bomb_func_t playchar_bomb_func;

// Loads BOMBS.BFT and the graphics for [resident->shottype]'s bomb, and points
// [playchar_bomb_func] at that shottype's animation. gameplay_init() calls
// this once per run.
void near bomb_load(void);

// Frees everything bomb_load() allocated. GameExecl() calls this at the end of
// a run.
void near bomb_free(void);

// Resets the per-stage bomb-use counter and any bomb still in progress — not
// the remaining [bombs], which is why this is singular like every other
// scalar-state reset in the tree (scroll_reset(), score_reset(),
// player_reset()) rather than plural like the array ones (bullets_reset(),
// items_reset(), bg_particles_reset()).
// stage_init() calls this once per stage.
void near bomb_reset(void);

// Marks the tiles under the expanding bomb circle for redrawing.
// stage_loop() calls this once per frame, together with the other
// *_invalidate() functions.
void near bomb_invalidate(void);
#endif

// Drops a bomb, if possible.
// Before the lift, TH02's root dump published this one with __pascal *and*
// `extern "C"` name decoration as `public PLAYER_BOMB`
// (`fc5d259e:th02_main.asm:3362`), not `@PLAYER_BOMB$QV`, so the lift has to
// keep both. See kb/codegen/0086.
extern "C" void pascal near player_bomb(void);

void near bomb_update_and_render(void);
