#include "th04/main/player/bomb.hpp"

// TH05's other two playchar bomb drivers, alongside the bomb_reimu() and
// bomb_marisa() th04/main/player/bomb.hpp declares for both games.
//
// `extern "C"`, for the reason that header spells out at length: every dump
// that publishes one of these four spells it UNDECORATED and all-uppercase
// (kb/codegen/0086), and Turbo C++ rejects the body outright once a C++-linkage
// declaration has been seen first.
extern "C" {
	void pascal near bomb_mima(void);
	void pascal near bomb_yuuka(void);
}
