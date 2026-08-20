#include "th02/main/player/bomb.hpp"

// Frames after a hit during which a bomb still takes the death back. Added on
// top of MISS_ANIM_FRAMES when [miss_time] is set, so a [miss_time] above
// MISS_ANIM_FRAMES means the miss animation has not started yet.
// (th04/shared.inc)
static const uint8_t DEATHBOMB_WINDOW = 8;

extern bool bombing_disabled;
extern unsigned char bomb_frame;

#if (GAME == 4)
// Frames of invincibility granted by dropping a bomb. TH04-only: TH05 picks
// this per playchar instead (BOMB_INVINCIBILITY_* in
// th04/main/player/bomb.cpp), so an unguarded 255 here would be a constant
// named after a quantity TH05 does not have.
static const uint8_t BOMB_INVINCIBILITY_FRAMES = 255;
#endif

#if (GAME == 4)
// Pointless indirection to player_bomb().
extern nearfunc_t_near player_bomb_func;
#endif

// Character-specific bomb update and render functions
// ---------------------------------------------------

extern nearfunc_t_near playchar_bomb_func;

// `extern "C"`, because every dump that publishes these spells them
// UNDECORATED and all-uppercase (kb/codegen/0086) — the form th05_main.asm
// still carries for its own, unrelated pair of playchar bomb functions. Once a
// TH04 body is lifted, its own `public` line goes with it, and what pins the
// undecorated name from then on is the `procdesc pascal near` the lift leaves
// behind for main()'s `mov _playchar_bomb_func, offset bomb_…` site.
//
// Neither declaration carried `extern "C"` until TH04's bomb_marisa() became
// the first C++ definition either name had ever had. Nothing had graded them
// before that — no translation unit in the tree defined or called either
// function — and Turbo C++ rejected the body outright with "'pascal
// bomb_marisa()' was previously declared with the language 'C++'".
extern "C" {
	void pascal near bomb_reimu(void);
	void pascal near bomb_marisa(void);
}
// ---------------------------------------------------

#if (GAME == 4)
// Handles the BB?.BB animation, scroll deactivation, common palette
// manipulation, and calls [playchar_bomb_func].
void near bomb_update_and_render(void);
#endif

// player_bomb() is declared in th02/main/player/bomb.hpp above, for all games
// and with the `extern "C"` linkage the dumps require (kb/codegen/0086). Both
// dumps now spell it `PLAYER_BOMB procdesc pascal near` (th04_main.asm:6014,
// th05_main.asm:1290) — the `public PLAYER_BOMB` lines that used to carry the
// requirement were deleted when these bodies were lifted. What still pins the
// undecorated name is the ASM references to it: `offset player_bomb` at
// th04_main.asm:386 and :404. TH05's dump reference was a call inside
// sub_1214A, and it went to C++ with that body on 2026-08-19, so th05_main.asm
// no longer holds one at all.
//
// The re-declaration that used to sit here dropped that `extern "C"`, which
// only ever compiled because th02's declaration came first. TH04's own note
// about it: player_bomb() also cancels a death when called during the
// deathbomb window — see th04/main/player/bomb.cpp.
