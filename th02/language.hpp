#ifndef TH02_LANGUAGE_HPP
#define TH02_LANGUAGE_HPP

#include "platform.h"

// Patch-owned presentation preference. It deliberately stays outside replay,
// resident, score-file, configuration, Practice, and simulation state.
enum t2_language_preference_t {
	T2LANG_JAPANESE = 0,
	T2LANG_ENGLISH = 1,
};

// Each executable can independently validate the on-disk preference before its
// first language-dependent presentation. Invalid files always select Japanese.
void far t2_language_load(void);
t2_language_preference_t far t2_language_get(void);

// Future OP presentation selection uses this transactional writer. No current
// surface invokes it, keeping this runtime parcel separate from UI routing.
bool far t2_language_set(t2_language_preference_t preference);

// T2EN.DAT is presentation-only. Its directory must have the exact audited
// shape before any later scoped packfile transaction may select English.
bool far t2_language_overlay_valid(void);
bool far t2_language_english_ready(void);

#endif /* TH02_LANGUAGE_HPP */
