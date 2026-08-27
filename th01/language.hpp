#ifndef TH01_LANGUAGE_HPP
#define TH01_LANGUAGE_HPP

#include "platform.h"

// Patch-owned presentation preference. It deliberately stays outside replay,
// resident, and score-file state.
enum t1_language_preference_t {
	T1LANG_JAPANESE = 0,
	T1LANG_ENGLISH = 1,
};

// Each executable validates the on-disk preference before its first
// language-dependent presentation. Invalid files always select Japanese.
void far t1_language_load(void);
t1_language_preference_t far t1_language_get(void);

#endif /* TH01_LANGUAGE_HPP */
