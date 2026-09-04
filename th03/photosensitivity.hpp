#ifndef TH03_PHOTOSENSITIVITY_HPP
#define TH03_PHOTOSENSITIVITY_HPP

#include "platform.h"
#include "th03/snd/options.hpp"

#define T3_CFG_LANGUAGE_MASK 0x3F
#define T3_CFG_PHOTOSENSITIVITY 0x80
#define T3_PHOTOSENSITIVITY_RES_INDEX (T3_SND_SE_RES_INDEX + 1)

#if (T3_PHOTOSENSITIVITY_RES_INDEX >= 198)
#error Photosensitivity state exceeds resident scratch space
#endif

inline bool photosensitivity_enabled(void)
{
	return (
		static_cast<uint8_t>(
			resident->unused_3[T3_PHOTOSENSITIVITY_RES_INDEX]
		) != 0
	);
}

inline void photosensitivity_enabled_set(bool enabled)
{
	resident->unused_3[T3_PHOTOSENSITIVITY_RES_INDEX] = enabled;
}

#endif /* TH03_PHOTOSENSITIVITY_HPP */
