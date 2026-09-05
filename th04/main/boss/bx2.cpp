/// Extra Stage Boss #2 - Gengetsu
/// ------------------------------

#include "th04/main/boss/bx2.hpp"

// Coordinates
// -----------

static const pixel_t WAVE_TARGET_MARGIN = (PLAYFIELD_W / 12);
// -----------

#define wave_amp     	gengetsu_wave_amp
#define wave_target_x	gengetsu_wave_target_x
uint8_t wave_amp = 0x00;
Subpixel wave_target_x;
