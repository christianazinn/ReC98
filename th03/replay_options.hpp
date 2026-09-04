#ifndef TH03_REPLAY_OPTIONS_HPP
#define TH03_REPLAY_OPTIONS_HPP

#include "platform.h"
#include "th03/resident.hpp"
#include "th03/snd/options.hpp"

// Zero keeps replay recording enabled for every configuration written before
// this option was introduced.
#define T3_CFG_REPLAY_RECORDING_DISABLED 0x40
#define T3_REPLAY_RECORDING_RES_INDEX (T3_SND_MMD_HANDOFF_RES_INDEX + 1)

#if (T3_REPLAY_RECORDING_RES_INDEX >= 198)
#error Replay recording state exceeds resident scratch space
#endif

inline bool replay_recording_enabled(void)
{
	return (
		static_cast<uint8_t>(
			resident->unused_3[T3_REPLAY_RECORDING_RES_INDEX]
		) != 0
	);
}

inline void replay_recording_enabled_set(bool enabled)
{
	resident->unused_3[T3_REPLAY_RECORDING_RES_INDEX] = enabled;
}

#endif /* TH03_REPLAY_OPTIONS_HPP */
