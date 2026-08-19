// Guarded because th04/main_035.cpp now includes it once for
// th04/formats/bb_boss.cpp and th04/main/stage/setup.cpp includes it again
// (kb/codegen/0129: at a collision set of ONE header, guarding is the cheap
// fix; it was eleven that made guarding the wrong answer). Byte-inert: nothing
// here is conditional on anything but GAME, which is fixed per build.
#ifndef TH04_FORMATS_BB_H
#define TH04_FORMATS_BB_H

#include "pc98.h"

#define BB_SIZE 2048

// Bitmap format, storing 1-bit values for 8 tiles in one byte.
typedef uint8_t bb_tiles8_t;

// Blocky boss entrance animations
// -------------------------------

extern bb_tiles8_t __seg *bb_boss_seg;

// Loads the .BB file with the given name into memory, and sets [bb_boss_seg]
// to the newly allocated segment. Does not attempt to free [bb_boss_seg], and
// will leak memory if it is not a nullptr.
void pascal near bb_boss_load(const char far *fn);

// Frees any previously allocated [bb_boss_seg].
#if (GAME == 5)
void near bb_boss_free(void);
#else
void far bb_boss_free(void);
#endif
// -------------------------------

/// Text dissolve circles
/// ---------------------

#define BB_TXT_W 32
#define BB_TXT_H 32
#define BB_TXT_VRAM_W (BB_TXT_W / BYTE_DOTS)

#define BB_TXT_IN_SPRITE 16
#define BB_TXT_IN_CELS 8
#define BB_TXT_OUT_SPRITE 0
#define BB_TXT_OUT_CELS 16

// One cel, as stored in the .BB files and blitted from there.
typedef bb_tiles8_t bb_txt_cel_t[BB_TXT_H][BB_TXT_VRAM_W];

// TXT.BB and TXT2.BB, loaded back to back into one allocation so that the
// sprite index runs straight across both. The constants above are what fix
// this layout, and they are consistent with the sizes the loader passes:
// BB_TXT_OUT_CELS cels of BB_TXT_H * BB_TXT_VRAM_W bytes is exactly BB_SIZE,
// BB_TXT_IN_CELS is BB_SIZE / 2, and BB_TXT_IN_SPRITE * sizeof(bb_txt_cel_t)
// is the offset the second read starts at.
struct bb_txt_t {
	bb_txt_cel_t out[BB_TXT_OUT_CELS];
	bb_txt_cel_t in[BB_TXT_IN_CELS];
};

extern bb_txt_t __seg *bb_txt_seg;

// Allocates [bb_txt_seg] and loads both files into it. Does not free a
// previous allocation, and will leak it if there is one.
extern "C" void pascal near bb_txt_load(void);

// Frees [bb_txt_seg], if allocated.
extern "C" void pascal near bb_txt_free(void);

// Puts the given TXT*.BB sprite at (⌊left/8⌋*8, top). Assumptions:
// • ES is already set to the beginning of a VRAM segment
// • The GRCG is active, and set to the intended color
#define bb_txt_put_8(left, top, sprite) \
	_CX = sprite; \
	bb_txt_put_8_raw(left, top);
void __fastcall near bb_txt_put_8_raw(uscreen_x_t left, uvram_y_t top);
/// ---------------------

#endif /* TH04_FORMATS_BB_H */
