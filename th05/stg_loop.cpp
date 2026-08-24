#include "th04/main/stage/loop.cpp"

// These roots immediately followed stage_loop() in ZUN's original object.
// Keeping the chain in one translation unit preserves the odd prefix parity
// that makes -a2 emit stage_setup()'s one-byte pre-table pad (kb/codegen/0154).
#include "th04/main/stage/gameplay_init.cpp"
#include "th04/main/stage/setup_main.cpp"
