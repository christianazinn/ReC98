#include "platform/x86real/pc98/grcg.hpp"
#include "th04/hardware/grcg.hpp"

void near grcg_setmode_rmw(void)
{
	_outportb_(0x7C, 0xC1);
}
#pragma codestring "\x90"

void near grcg_setmode_tdw(void)
{
	_outportb_(0x7C, GC_TDW);
}
#pragma codestring "\x90"
