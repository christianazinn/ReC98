// TH02 packfile declarations.

#include "libs/master.lib/master.hpp"

// Names and ABIs retained from master.lib's hand-written PFOPEN.ASM. [pf_t]
// is the segment of its separately allocated PFILE structure.
typedef void __seg *pf_t;

extern "C" {
pf_t MASTER_RET pfopen(
	const char MASTER_PTR *parfile, const char MASTER_PTR *file
);

// ReC98's semantic alias for master.lib's private `SEEK` label. [header]
// points into PFOPEN's stack frame; both filename pointers follow the active
// memory model, which is far in every TH02 binary.
int pascal near pf_header_seek(
	void __ss *header,
	pf_t pf,
	const char MASTER_PTR *parfile,
	const char MASTER_PTR *file
);
}

static inline void game_pfopen(void) {
	extern const char pf_fn[];
	pfkey = 0x12;
	pfstart(pf_fn);
}
