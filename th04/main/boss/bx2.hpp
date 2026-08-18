#ifndef TH04_MAIN_BOSS_BX2_HPP
#define TH04_MAIN_BOSS_BX2_HPP

#include "th04/main/custom.hpp"

// Gengetsu's column bullet spawn lines
// ------------------------------------
// The Extra Stage's only use of the [custom_entities] block. Seeded on the
// first frame of the spawn phase, telegraphed by gengetsu_fg_render() as
// vertical lines for the 64 frames before the bullets come out of them.

#define GENGETSU_SPAWNCOLUMN_COUNT 16

struct gengetsu_spawncolumn_t {
	int8_t unused[2];
	PlayfieldPoint pos;
	int8_t padding[20];
};

#define gengetsu_spawncolumns \
	(reinterpret_cast<gengetsu_spawncolumn_t *>(custom_entities))
// ------------------------------------

// Amplitude of the sine wave that Gengetsu's two sprite halves are distorted
// along while she teleports. Nonzero for the whole of that animation, and the
// flag that gengetsu_fg_render() branches on to draw it.
// Should really have been signed.
extern uint8_t gengetsu_wave_amp;

extern Subpixel gengetsu_wave_target_x;

#endif /* TH04_MAIN_BOSS_BX2_HPP */
