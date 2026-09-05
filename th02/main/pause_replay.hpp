#ifndef TH02_MAIN_PAUSE_REPLAY_HPP
#define TH02_MAIN_PAUSE_REPLAY_HPP

#include "platform.h"

// The protected native pause_menu() is an equal-size near wrapper around this
// patch-owned far tail.
bool16 far t2pause_menu(void);

#endif
