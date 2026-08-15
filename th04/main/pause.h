// Returns 1 if the game should quit.
// `near` and `extern "C"` because th04/main/pause.asm defines it as a near
// proc under the undecorated `_pause`.
extern "C" int near pause(void);
