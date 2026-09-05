#ifndef TH04_MAIN_PHASE_HPP
#define TH04_MAIN_PHASE_HPP

// Common midboss and boss phases
static const unsigned char PHASE_HP_FILL = 0;
static const unsigned char PHASE_BOSS_ENTRANCE_BB = 1;
#if (GAME == 5)
static const unsigned char PHASE_BOSS_EXPLODE_SMALL = 253;
static const unsigned char PHASE_BOSS_EXPLODE_BIG = 254;
#endif
static const unsigned char PHASE_EXPLODE_BIG = 254;
static const unsigned char PHASE_NONE = 255;

// Guarded because th05/shot_inv.cpp now reaches this file twice: once from the
// sub_12017() lift at its front, and once through th04/main/boss/boss.hpp on
// the way to the objects behind it. A second expansion rejects every
// `static const` above (kb/codegen/0129). Byte-inert.
#endif
