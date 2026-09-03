#ifndef TH03_SND_OPTIONS_HPP
#define TH03_SND_OPTIONS_HPP

#include "platform.h"
#include "th03/language.hpp"
#include "th03/resident.hpp"

extern bool snd_fm_possible;

// Keeps YUME.CFG byte-compatible with every earlier replay-patch version.
// Zero in the previously unused high bit therefore retains the old behavior.
#define T3_CFG_BGM_MODE_MASK 0x7F
#define T3_CFG_SE_DISABLED 0x80

#define T3_SND_SE_RES_INDEX (T3_LANGUAGE_RES_INDEX + 1)

#if (T3_SND_SE_RES_INDEX >= 198)
#error Sound-effect state exceeds resident scratch space
#endif

inline bool th03_snd_se_enabled(void)
{
	return (
		static_cast<uint8_t>(resident->unused_3[T3_SND_SE_RES_INDEX]) != 0
	);
}

inline void th03_snd_se_enabled_set(bool enabled)
{
	resident->unused_3[T3_SND_SE_RES_INDEX] = enabled;
	if(!enabled) {
		snd_fm_possible = false;
	}
}

inline void th03_snd_cfg_unpack(uint8_t packed_bgm_mode)
{
	resident->bgm_mode = (packed_bgm_mode & T3_CFG_BGM_MODE_MASK);
	th03_snd_se_enabled_set((packed_bgm_mode & T3_CFG_SE_DISABLED) == 0);
}

inline uint8_t th03_snd_cfg_pack(void)
{
	return (
		resident->bgm_mode |
		(th03_snd_se_enabled() ? 0 : T3_CFG_SE_DISABLED)
	);
}

inline void th03_snd_process_apply(void)
{
	if(!th03_snd_se_enabled()) {
		snd_fm_possible = false;
	}
}

void far th03_snd_process_init(void);
#if (BINARY == 'M')
void far th03_snd_process_adopt_mainl(void);
#endif
#if (BINARY == 'L')
extern "C" void far th03_snd_mainl_play(void);
#endif
void far th03_snd_se_toggle(void);

#endif /* TH03_SND_OPTIONS_HPP */
