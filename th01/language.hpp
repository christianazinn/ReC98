#ifndef TH01_LANGUAGE_HPP
#define TH01_LANGUAGE_HPP

#include "platform.h"

// Patch-owned presentation preference. It deliberately stays outside replay,
// resident, and score-file state.
enum t1_language_preference_t {
	T1LANG_JAPANESE = 0,
	T1LANG_ENGLISH = 1,
};

// Patch settings share T1LANG.CFG. Zero in the recording bit deliberately
// keeps captures enabled for configurations written before this option.
#define T1_SETTINGS_LANGUAGE_MASK 0x01
#define T1_SETTINGS_REPLAY_RECORDING_DISABLED 0x40
#define T1_SETTINGS_KNOWN_MASK ( \
	T1_SETTINGS_LANGUAGE_MASK | T1_SETTINGS_REPLAY_RECORDING_DISABLED \
)

// Each executable validates the on-disk preference before its first
// language-dependent presentation. Invalid files always select Japanese.
void far t1_language_load(void);
t1_language_preference_t far t1_language_get(void);
bool far t1_replay_recording_enabled(void);

#endif /* TH01_LANGUAGE_HPP */
