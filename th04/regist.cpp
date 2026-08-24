// The merged product has one shared score-data implementation, so this host
// contains only the registration routines lifted from the MAINE root dump.
#pragma option -zCSCORE_TEXT -zPgroup_01

#include "th04/formats/scoredat/scoredat.hpp"
#include "th04/gaiji/gaiji.h"

extern unsigned char rank;

bool pascal near hiscore_scoredat_load_for(playchar_t pc)
{
	return scoredat_load(pc, static_cast<rank_t>(rank));
}

void near hiscore_scoredat_save(void)
{
	scoredat_save(playchar, static_cast<rank_t>(rank));
}

#include "th04/hiscore/regist_enter.cpp"
#define stage_put regist_stage_put
#include "th04/hiscore/regist_view.cpp"
#undef stage_put
#include "th04/hiscore/regist_menu.cpp"
#include "th04/hiscore/regist_unblit.cpp"
