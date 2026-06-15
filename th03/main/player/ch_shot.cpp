#pragma codeseg HITBOX_TEXT

#include "platform.h"

extern "C" uint8_t byte_1FDEA;
extern "C" uint8_t byte_1FE1C;

extern "C" void far sub_142D0(void)
{
	byte_1FDEA = 0;
	byte_1FE1C = 0;
}
