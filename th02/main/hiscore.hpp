/// MAIN.EXE's score file accesses outside the registration menu
/// ------------------------------------------------------------
/// Defined in th02/main/hiscore.cpp, which is compiled as part of
/// th02/regist_m.cpp rather than on its own (kb/codegen/0112) — so this header
/// only ever declares, and the definitions depend on the include order that
/// wrapper sets up.

#ifndef TH02_MAIN_HISCORE_HPP
#define TH02_MAIN_HISCORE_HPP

// Loads the score file, then seeds the in-game high score display from its top
// entry.
void far hiscore_get(void);

// Records that the main 5 stages have been cleared with the shot type the
// player is currently using. Called from ASM.
void far scoredat_cleared_set(void);

// Records that the Extra Stage has been cleared with the shot type the player
// is currently using. Called from ASM.
void far scoredat_extra_cleared_set(void);

#endif
/// ------------------------------------------------------------
