#pragma option -zCMAIN_010_TEXT -zPmain_01

#include "th03/main/player/bomb.hpp"
#include "codegen.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/main/hud/static.hpp"
#include "th02/snd/snd.h"
#include "x86real.h"

void near story_skill_decrement(void);
extern "C" void pascal far sub_CDBD(void);
extern "C" unsigned char pid_PID_current;

extern "C" void pascal near player_bomb(player_stuff_t near *player)
{
	register player_stuff_t near *p = player;

	if(bomb_flag[pid.current] != BF_INACTIVE) {
		return;
	}
	if(p->hyper_active != 0) {
		return;
	}
	if(p->bombs != 0) {
		bomb_flag[pid.current] = BF_PREPARING;
		p->bombs--;
		p->invincibility_time = BOMB_FRAMES;
		_asm {
			push	word ptr pid_PID_current
			nop
			push	cs
			call	near ptr hud_static_bombs_put
		}
	} else {
		if(p->gauge_avail < GAUGE_MAX) {
			return;
		}
		damage_all_on[pid.current] = true;
		snd_se_play(7);
		p->hyper_active = p->playchar_paletted.v;

		__emit__(0xFF, 0x34); // push word ptr [si]
		__emit__(0xFF, 0x74, 0x02); // push word ptr [si+2]
		_AX = pid.current;
		_asm {
			push	ax
			nop
			push	cs
			call	near ptr sub_CDBD
		}
	}
	story_skill_decrement();
}
