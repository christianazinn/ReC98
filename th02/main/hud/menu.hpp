#include "platform.h"

// Runs the pause menu, blocking the caller until the player either resumes
// the game or confirms quitting. Returns `true` if the stage should be quit.
bool16 near pause_menu(void);
