#ifndef TH04_ORACLE_BUILD_HPP
#define TH04_ORACLE_BUILD_HPP

// Per-lineage build identity for the TH04/TH05 oracle mod. This is the ONLY
// file that is expected to differ between the branches that carry the verifier
// (master-based, Anniversary-based, ...), mirroring TH03's
// `th03/t3case_build.hpp`. Everything else is shared verbatim.

#include "th04/main/oracle.hpp"

#define ORACLE_PRODUCER ORACLE_PRODUCER_GAME_MOD

// Whether this lineage may WRITE a case, as opposed to only playing one back.
//
// Only the byte-identical lineage records. A case is a statement about what
// ZUN's demo does, so exactly one lineage gets to make it; every other lineage
// is a consumer whose job is to reproduce that case's trace. Compiling the
// recorder out of the others is stronger than merely never configuring it,
// because it makes "the Anniversary build wrote this corpus" impossible rather
// than merely unlikely.
#define ORACLE_RECORD_SUPPORTED 1

// First four bytes of the ReC98 commit this branch is based on, as ASCII, in
// file order. `harness/main` @ 92a49130 -> '9','2','a','4'.
//
// Written as an expression rather than a table so that the module still
// contributes zero initialized data.
#define ORACLE_COMMIT_ASCII(c0, c1, c2, c3) ( \
	(static_cast<uint32_t>(c0)) | \
	(static_cast<uint32_t>(c1) << 8) | \
	(static_cast<uint32_t>(c2) << 16) | \
	(static_cast<uint32_t>(c3) << 24) \
)
#define ORACLE_SOURCE_COMMIT ORACLE_COMMIT_ASCII('9', '2', 'a', '4')

#endif /* TH04_ORACLE_BUILD_HPP */
