#ifndef TH03_LANGUAGE_HPP
#define TH03_LANGUAGE_HPP

#include "platform.h"
#include "th03/fast_forward.hpp"
#include "th03/resident.hpp"

#define T3_LANGUAGE_RES_INDEX 183
#define T3_LANGUAGE_OVERLAY_FN "TH3EN.DAT"

#if (T3_LANGUAGE_RES_INDEX <= T3_RES_FAST_FORWARD_STAFF_PHASE_INDEX)
#error Language state overlaps fast-forward state
#endif
#if (T3_LANGUAGE_RES_INDEX >= 198)
#error Language state exceeds resident scratch space
#endif

enum language_t {
	LANGUAGE_JAPANESE = 0,
	LANGUAGE_ENGLISH = 1,
	LANGUAGE_COUNT,
};

inline language_t language_resident(void)
{
	if(!resident) {
		return LANGUAGE_JAPANESE;
	}
	return static_cast<language_t>(
		static_cast<uint8_t>(resident->unused_3[T3_LANGUAGE_RES_INDEX])
	);
}

inline void language_resident_set(language_t language)
{
	resident->unused_3[T3_LANGUAGE_RES_INDEX] = static_cast<uint8_t>(language);
}

inline bool language_is_english(void)
{
	return (language_resident() == LANGUAGE_ENGLISH);
}

bool16 far language_overlay_available(void);
bool16 far language_archive_begin_if_translated(const char far *fn);
void far language_archive_end(bool16 switched);
int far language_pi_load(int slot, const char far *fn);

#endif /* TH03_LANGUAGE_HPP */
