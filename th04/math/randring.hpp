#ifndef TH04_MATH_RANDRING_HPP
#define TH04_MATH_RANDRING_HPP

// Guarded for the same reason th04/main/player/shot.hpp is, and stated there
// in full: that header pulls this one, so any object that includes both it
// and this file directly expands every `inline` below twice, and Turbo C++
// 4.02 rejects the second expansion outright. Ordering the includes so only
// one of them wins is not an invariant across hosts; a guard is.
// Byte-inert: this file only declares.

#include "th03/math/randring.hpp"

uint16_t pascal near randring1_next16_mod(uint16_t divisor);

/// Constrained random numbers
/// --------------------------
/// These limit the next random number between [min] inclusive and [max]
/// exclusive, via either AND or MOD.

inline uint8_t randring1_next8_mod_ge_lt(uint8_t min, uint8_t max) {
	return (static_cast<int8_t>(min) >= 0)
		? (min + randring1_next16_mod(max - min))
		: (randring1_next16_mod(max + -static_cast<int8_t>(min)) - (-min));
}

inline int16_t randring1_next16_mod_ge_lt(int16_t min, int16_t max) {
	return (min + randring1_next16_mod(max - min));
}

inline subpixel_t randring1_next16_mod_ge_lt_sp(float min, float max) {
	return randring1_next16_mod_ge_lt(to_sp(min), to_sp(max));
}
/// --------------------------
#endif /* TH04_MATH_RANDRING_HPP */
