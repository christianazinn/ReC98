#ifndef TH03_T3CASE_BUILD_HPP
#define TH03_T3CASE_BUILD_HPP

#include "th03/t3case.hpp"

// Which branch's verifier this build is. Provenance only; playback behavior
// never depends on it. This is the single file that differs between the
// master-lineage and Anniversary-lineage verifier branches.
#define T3CASE_PRODUCER T3CASE_PRODUCER_MASTER_MOD

#endif /* TH03_T3CASE_BUILD_HPP */
