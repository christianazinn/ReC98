#include "platform.h"

static const unsigned char BOMB_CIRCLE_FRAMES = 32;

extern bool bombing;

// Drops a bomb, if possible.
// Still ASM in TH02's root dump, which publishes it with __pascal *and*
// `extern "C"` name decoration (`public PLAYER_BOMB`, not
// `@PLAYER_BOMB$QV`). See kb/codegen/0086.
extern "C" void pascal near player_bomb(void);

void near bomb_update_and_render(void);
