// TH04-specific boss declarations.

#include "pc98.h"

/// Backdrops
/// ---------

// Boss-specific [boss_backdrop_colorfill] functions.
void pascal near orange_backdrop_colorfill(void);
void pascal near kurumi_backdrop_colorfill(void);
void pascal near elly_backdrop_colorfill(void);
void pascal near reimu_marisa_backdrop_colorfill(void);
void pascal near yuuka5_backdrop_colorfill(void);
void pascal near mugetsu_gengetsu_backdrop_colorfill(void);
/// ---------

// Orange's fight. orange_fg_render() is already C++
// (th04/main/boss/render.cpp); the other two are still in th04_main.asm, which
// publishes them under their mangled UPPER-case spelling for this header.
void pascal near orange_bg_render(void);
void pascal near orange_fg_render(void);
void pascal  far orange_update(void);

// Kurumi's fight. kurumi_fg_render() is already C++
// (th04/main/boss/render.cpp); the other two are still in th04_main.asm, which
// publishes them under their mangled UPPER-case spelling for this header.
void pascal near kurumi_bg_render(void);
void pascal near kurumi_fg_render(void);
void pascal  far kurumi_update(void);

// Elly's fight. elly_fg_render() is already C++ (th04/main/boss/b3_fg.cpp);
// the other two are still in th04_main.asm, which publishes them under their
// mangled UPPER-case spelling for this header.
void pascal near elly_bg_render(void);
void pascal near elly_fg_render(void);
void pascal  far elly_update(void);

void pascal near reimu_marisa_bg_render(void);
void pascal near reimu_fg_render(void);
void pascal  far reimu_update(void);
void pascal near marisa_fg_render(void);
void pascal  far marisa_update(void);
void pascal near yuuka5_bg_render(void);
void pascal near yuuka5_fg_render(void);
void pascal  far yuuka5_update(void);

// Yuuka's Stage 6 fight. All three functions are C++: the background in
// th04/main/boss/bg.cpp, the foreground in th04/main/boss/b6_fg.cpp, and the
// update function in th04/main/boss/b6_upd.cpp.
void pascal near yuuka6_bg_render(void);
void pascal near yuuka6_fg_render(void);
void pascal  far yuuka6_update(void);

void pascal near mugetsu_gengetsu_bg_render(void);
void pascal near mugetsu_fg_render(void);
void pascal  far mugetsu_update(void);

static const pixel_t GENGETSU_W = 96;
static const pixel_t GENGETSU_H = 96;

void pascal near gengetsu_fg_render(void);
void pascal  far gengetsu_update(void);
