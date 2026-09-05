// TH02-specific boss declarations.

#include "platform.h"

// Total amount of frames spent in the Evil Eye Σ fight, excluding the defeat
// animation.
extern uint32_t sigma_frame;

// Resets every piece of boss, midboss and stone state that survives a stage
// transition. stage_init() calls this once per stage. Defined at the top of
// th02/main/bullet/bullet.cpp because that is the object ZUN put it in.
//
// The plural is deliberate and is NOT a typo for TH04's singular boss_reset()
// (th04/main/boss/reset.cpp), which resets the boss alone. This one resets
// stone, midboss *and* boss state, so the two functions differ by one letter
// across two games while doing measurably different jobs. Do not "correct"
// either name into the other.
void bosses_reset(void);
