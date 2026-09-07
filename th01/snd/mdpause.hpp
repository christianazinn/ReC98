#ifndef TH01_SND_MDPAUSE_HPP
#define TH01_SND_MDPAUSE_HPP
#include "platform.h"

// Dispatches to the configured backend; resume follows the accepted pause.
bool16 far t1_mdrv2_music_pause(void);
void far t1_mdrv2_music_resume(void);

#endif
