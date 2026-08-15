#include "th02/hardware/input.hpp"

#define DEMO_N 7000 /* ZUN symbol [MAGNet2010] */

extern int demo_frame;
extern input_t *DemoBuf; /* ZUN symbol [MAGNet2010] */

// Allocates [DemoBuf] and reads the recording selected by
// [resident->demo_num] into it. Called from main() through a same-code-group
// `nopcall` alias.
void demo_load(void);

// Replays one frame of [DemoBuf] into [key_det]. Called once per frame from
// stage_loop(), through a same-code-group `nopcall` alias.
extern "C" void pascal DemoPlay(void); /* ZUN symbol [MAGNet2010] */
