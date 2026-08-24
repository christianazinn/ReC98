#include "th04/main/player/shot.hpp"

#define SHOT_FLAG_AND_AGE(shot) \
	(*reinterpret_cast<unsigned near *>((shot) + 0))
#define SHOT_POS_CUR(shot) \
	(*reinterpret_cast<unsigned long near *>((shot) + 2))
#define SHOT_POS_VELOCITY(shot) \
	(*reinterpret_cast<unsigned long near *>((shot) + 10))

static const unsigned long SHOT_VELOCITY_UP_12 = 0xFF400000UL;

// Searches for a free shot slot from [shot_ptr] onwards and returns it, or
// nullptr if all remaining slots have already been used this frame.
Shot near* near shots_add(void)
{
	_AX = 0;
loop:
	if(static_cast<unsigned char>(shot_last_id) >= SHOT_COUNT) {
		goto ret;
	}
	_BX = reinterpret_cast<int>(shot_ptr);
	shot_ptr++;
	if(reinterpret_cast<Shot near *>(_BX)->flag == SF_FREE) {
		goto found;
	}
	shot_last_id++;
	goto loop;

found:
	SHOT_FLAG_AND_AGE(_BX) = (SF_ALIVE | (0 << 8));
	SHOT_POS_CUR(_BX) = *reinterpret_cast<unsigned long *>(&player_pos.cur);
	SHOT_POS_VELOCITY(_BX) = SHOT_VELOCITY_UP_12;
	_AX = _BX;

ret:
	return reinterpret_cast<Shot near *>(_AX);
}
