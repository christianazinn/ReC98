#pragma option -zCPLAYFLD_TEXT -zPmain_01

#include "libs/master.lib/pc98_gfx.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th03/main/hud/static.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/main/round.hpp"
#include "th03/resident.hpp"
#include "th02/snd/snd.h"

extern unsigned char score[];
extern farfunc_t_near farfp_20F28;

extern "C" void pascal near hyper_standby(void);
extern "C" void pascal far sub_A38E(void);
extern "C" void pascal near sub_9EBF(void);

extern "C" void pascal near sub_A21F(void)
{
	grcg_setcolor(GC_RMW, 2);
	grc_setclip(0, 0, (RES_X - 1), (SPRITE16_RES_Y - 1));
	graph_accesspage(1);
	grcg_fill();
	graph_accesspage(0);
	grcg_fill();
	graph_showpage(1);
	grcg_off();
	round_id++;
	sub_9EBF();
	farfp_20F28();
	players[0].hyper = hyper_standby;
	players[1].hyper = hyper_standby;
	snd_se_reset();
	_asm { nop; push cs; call near ptr hud_wipe; }
	_asm { nop; push cs; call near ptr sub_A38E; }
	_asm { nop; push cs; call near ptr hud_static_put; }
}

void pascal near resident_score_last_update(int pid)
{
	for(int digit = 0; digit < SCORE_DIGITS; digit++) {
		resident->score_last[pid].digits[digit] = score[
			(pid * SCORE_DIGITS) + digit
		];
		resident->score_last[1 - pid].digits[digit] = 0;
	}
}
