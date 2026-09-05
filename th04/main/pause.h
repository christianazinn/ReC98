// Returns 1 if the game should quit.
// `near` and `extern "C"` preserve the ABI and undecorated linker name of the
// body now defined in th04/main/pause.cpp.
extern "C" int near pause(void);
