#ifndef TH01_T1KIK_HPP
#define TH01_T1KIK_HPP

// Private Kikuri natural/direct acceptance marker. The definitions are kept
// here rather than in the replay ABI so a default release build has no route
// to the witness. The profile selectors live in the existing format header,
// but this marker remains outside every serialized replay structure.
#include "th01/replay_format.hpp"
#ifndef T1KIK_TRACE
#define T1KIK_TRACE 0
#endif

#ifndef T1KIK_DIRECT_TRACE
#define T1KIK_DIRECT_TRACE 0
#endif

#ifndef T1KIK_NATURAL_TRACE
#define T1KIK_NATURAL_TRACE 0
#endif

#if T1KIK_TRACE

#include "platform.h"
#include "th01/hardware/graph.h"

// Invoked at the existing pre-input seam, after the native/direct first-combat
// setup and before input_sense() consumes the first ordinary input.
void t1kik_pre_input(uint8_t process_seq);

// Invoked immediately after input_sense() has updated its edge-detection
// state. It records the first resumed input without advancing gameplay.
void t1kik_post_input(uint8_t process_seq);

// Clears only profile-local tail state between REIIDEN processes. It is called
// at the established replay reset seam, before either one-shot arm.
void t1kik_trace_reset(void);

#if T1KIK_NATURAL_TRACE
// One-shot arm for the natural route. The profile is evidence-only: merely
// reaching Kikuri must not create a witness unless capture armed this route.
bool16 t1kik_natural_prepare(void);
#endif

#if T1KIK_DIRECT_TRACE
// One-shot arm for the direct owner path. It has no fallback: a missing arm
// means no marker and the caller must reject the profile handoff.
bool16 t1kik_direct_prepare(void);
#endif

// Graph wrappers record the hardware page ports in private far state. The
// normal page globals are not authoritative in the normal profile.
void t1kik_visible_page_set(page_t page);
uint8_t t1kik_visible_page_get(void);
void t1kik_accessed_page_set(page_t page);
uint8_t t1kik_accessed_page_get(void);

#endif

#endif /* TH01_T1KIK_HPP */
