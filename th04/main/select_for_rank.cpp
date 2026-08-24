#include "x86real.h"
#include "th04/main/rank.hpp"

int pascal select_for_rank(
	int for_easy, int for_normal, int for_hard, int for_lunatic
)
{
	// At entry, the four Pascal parameters occupy SS:[SP+10] through
	// SS:[SP+4], in rank order. Read the selected word without a stack frame.
	_AL = rank;
	_AH ^= _AH;
	_AX += _AX;
	_BX = 10;
	_BX -= _AX;
	_BX += _SP;
	return peek(_SS, _BX);
}
