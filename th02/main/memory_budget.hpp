#ifndef TH02_MAIN_MEMORY_BUDGET_HPP
#define TH02_MAIN_MEMORY_BUDGET_HPP

#include "platform.h"

// Initializes MAIN's hardware and packfile state after allocating a checked,
// bounded DOS heap. This is a patch-owned replacement for the stock fixed-size
// call so the original initmain contribution remains in place.
int far t2replay_game_init_main(void);

// OP.EXE grew with the title/replay surfaces as well. It uses the same bounded
// DOS admission policy, preserving the stock 256,000-byte request whenever
// the resident environment permits it.
int far t2replay_game_init_op(void);

// Number of paragraphs still available to master.lib's managed heap. This is
// exposed only for private admission diagnostics; gameplay never branches on
// it.
uint16_t far t2replay_game_heap_available_paras(void);

#endif /* TH02_MAIN_MEMORY_BUDGET_HPP */
