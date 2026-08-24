#pragma option -zCBULLET_A_TEXT -zPmain_03
#pragma option -k-

#include "th04/main/bullet/bullet.hpp"

extern "C" void far bullets_add_regular_far(void)
{
	bullets_add_regular();
}

#pragma option -k.
