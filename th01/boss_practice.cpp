// Public TH01 boss-phase Practice dispatch. Boss-owned state remains in each
// boss translation unit; this object only validates identity and dispatches.

#pragma option -zCT1BPRAC_TEXT -G-
#pragma codeseg T1BPRAC_TEXT

#include "th01/boss_practice.hpp"
#include "th01/replay_format.hpp"
#include "th01/resident.hpp"
#include "th01/main/boss/boss.hpp"
#include "th01/main/boss/b05.hpp"
#include "th01/main/boss/b10m.hpp"
#include "th01/main/boss/b10j.hpp"
#include "th01/main/boss/b15j.hpp"
#include "th01/main/boss/b15m.hpp"
#include "th01/main/boss/b20m.hpp"
#include "th01/main/boss/b20j.hpp"
#include "th01/main/stage/stages.hpp"
#include "th01/main/debug.hpp"

extern int8_t boss_id;


bool16 t1boss_practice_construct(uint8_t target)
{
	if(!resident || (frame_since_start_of_binary != 0) || !t1boss_practice_target_valid(
		target, resident->stage_id, resident->route
	)) {
		return false;
	}
	switch(boss_id) {
	case BID_SINGYOKU: return t1boss_singyoku_practice_construct(target);
	case BID_YUUGENMAGAN: return t1boss_yuugenmagan_practice_construct(target);
	case BID_MIMA: return t1boss_mima_practice_construct(target);
	case BID_KIKURI: return t1boss_kikuri_practice_construct(target);
	case BID_ELIS: return t1boss_elis_practice_construct(target);
	case BID_SARIEL: return t1boss_sariel_practice_construct(target);
	case BID_KONNGARA: return t1boss_konngara_practice_construct(target);
	}
	return false;
}

#pragma codeseg
