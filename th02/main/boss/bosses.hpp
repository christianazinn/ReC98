// TH02-specific boss declarations.

#include "platform.h"

// Total amount of frames spent in the Evil Eye Σ fight, excluding the defeat
// animation.
extern uint32_t sigma_frame;

// Resets every piece of boss, midboss and stone state that survives a stage
// transition. stage_init() calls this once per stage. Defined at the top of
// th02/main/bullet/bullet.cpp because that is the object ZUN put it in.
void bosses_reset(void);
