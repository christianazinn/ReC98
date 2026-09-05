#ifndef TH02_MATH_RANDRING_HPP
#define TH02_MATH_RANDRING_HPP

// Guarded for the same reason th04/main/player/shot.hpp is, and stated there
// in full: that header pulls this one, so any object that includes both it
// and this file directly expands every `inline` below twice, and Turbo C++
// 4.02 rejects the second expansion outright. Ordering the includes so only
// one of them wins is not an invariant across hosts; a guard is.
// Byte-inert: this file only declares.

#include "platform.h"
#include "th01/math/subpixel.hpp"

#define RANDRING_SIZE 256

extern uint8_t randring[RANDRING_SIZE];

void near randring_fill(void);

#if GAME == 2
extern uint8_t randring_p;

uint8_t near randring1_next8(void);
uint8_t pascal near randring1_next8_and(uint8_t mask);
uint16_t near randring1_next16(void);
uint8_t near randring2_next8(void);
uint8_t pascal near randring2_next8_and(uint8_t mask);
#endif

uint16_t near randring2_next16(void);

/// Constrained random numbers
/// --------------------------
/// These limit the next random number between [min] inclusive and [max]
/// exclusive, via either AND or MOD.

template <class T> inline bool is_range_a_power_of_two(T min, T max) {
	return (((max - min) & ((max - min) - 1)) == 0);
}

#if (GAME == 2)
inline uint8_t randring1_next8_ge_lt(uint8_t min, uint8_t max) {
	if(is_range_a_power_of_two(min, max)) {
		return (min + randring1_next8_and((max - min) - 1));
	}
	return (min + (randring1_next8() % (max - min)));
}

inline uint16_t randring1_next16_ge_lt(uint16_t min, uint16_t max) {
	if(is_range_a_power_of_two(min, max)) {
		return (min + (randring1_next16() & ((max - min) - 1)));
	}
	return (min + (randring1_next16() % (max - min)));
}

inline uint8_t randring2_next8_ge_lt(uint8_t min, uint8_t max) {
	if(is_range_a_power_of_two(min, max)) {
		return (min + randring2_next8_and((max - min) - 1));
	}
	return (min + (randring2_next8() % (max - min)));
}

inline subpixel_t randring2_next8_ge_lt_sp(float min, float max) {
	return randring2_next8_ge_lt(to_sp(min), to_sp(max));
}
#endif
/// --------------------------
#endif /* TH02_MATH_RANDRING_HPP */
