#ifndef TH02_BOMB_BFT_HPP
#define TH02_BOMB_BFT_HPP

#include "libs/master.lib/master.hpp"

// Allocates the original 15 masks and a fully covered final cel. The original
// BOMBS.BFT ends with that same coverage, but frame 31 reads one cel past it.
// The extra mask repairs only drawing; bomb timing and gameplay stay intact.
void __seg * MASTER_RET bomb_bft_alloc(unsigned bytesize);

#endif
