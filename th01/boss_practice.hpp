#ifndef TH01_BOSS_PRACTICE_HPP
#define TH01_BOSS_PRACTICE_HPP

#include "platform.h"
#include "th01/replay_format.hpp"

// Literal stage/route IDs are the stable replay format, not memory addresses.
inline uint8_t t1boss_practice_first(uint8_t scene, uint8_t route)
{
	if(scene == 0) return T1RPBPT_SINGYOKU_FIRST_COMBAT;
	if(scene == 1) return route ? T1RPBPT_MIMA_FIRST_COMBAT : T1RPBPT_YUUGENMAGAN_PHASE_1;
	if(scene == 2) return route ? T1RPBPT_KIKURI_PHASE_2 : T1RPBPT_ELIS_PHASE_1;
	return route ? T1RPBPT_KONNGARA_PHASE_1 : T1RPBPT_SARIEL_PHASE_1;
}

inline uint8_t t1boss_practice_last(uint8_t scene, uint8_t route)
{
	if(scene == 0) return T1RPBPT_SINGYOKU_PHASE_2;
	if(scene == 1) return route ? T1RPBPT_MIMA_PHASE_3 : T1RPBPT_YUUGENMAGAN_PHASE_13;
	if(scene == 2) return route ? T1RPBPT_KIKURI_PHASE_6 : T1RPBPT_ELIS_PHASE_5;
	return route ? T1RPBPT_KONNGARA_PHASE_7 : T1RPBPT_SARIEL_FORM_2;
}

inline bool16 t1boss_practice_target_valid(uint8_t target, int8_t stage, int8_t route)
{
	if((stage < 4) || (stage > 19) || ((stage % 5) != 4) ||
	   (route < 0) || (route > 1) || ((stage == 4) && route)) return false;
	uint8_t first = t1boss_practice_first(stage / 5, route);
	uint8_t last = t1boss_practice_last(stage / 5, route);
	if((first == T1RPBPT_SINGYOKU_FIRST_COMBAT) ||
	   (first == T1RPBPT_MIMA_FIRST_COMBAT)) return ((target == first) || (target == last));
	return ((target >= first) && (target <= last));
}

// Zero represents Boss Start in the Start Point cycle.
inline uint8_t t1boss_practice_step(uint8_t target, uint8_t scene, uint8_t route, int delta)
{
	uint8_t first = t1boss_practice_first(scene, route);
	uint8_t last = t1boss_practice_last(scene, route);
	if(!target) return (delta > 0) ? first : last;
	if(delta > 0) return (target == last) ? 0 : ((first <= 2) ? last : (target + 1));
	return (target == first) ? 0 : ((first <= 2) ? first : (target - 1));
}

inline uint8_t t1boss_practice_phase(uint8_t target)
{
	if(target <= 2) return 1;
	if(target == T1RPBPT_SINGYOKU_PHASE_2) return 2;
	if(target <= T1RPBPT_YUUGENMAGAN_PHASE_13) return (1 + (target - T1RPBPT_YUUGENMAGAN_PHASE_1) * 2);
	if(target == T1RPBPT_MIMA_PHASE_3) return 3;
	if(target == T1RPBPT_KIKURI_PHASE_2) return 2;
	if(target <= T1RPBPT_KIKURI_PHASE_6) return (4 + target - T1RPBPT_KIKURI_PHASE_4);
	if(target <= T1RPBPT_ELIS_PHASE_5) return (1 + (target - T1RPBPT_ELIS_PHASE_1) * 2);
	if(target == T1RPBPT_SARIEL_FORM_2) return 100;
	if(target <= T1RPBPT_SARIEL_PHASE_7) return (1 + (target - T1RPBPT_SARIEL_PHASE_1) * 2);
	return (1 + (target - T1RPBPT_KONNGARA_PHASE_1) * 2);
}

bool16 t1boss_practice_construct(uint8_t target);

#endif /* TH01_BOSS_PRACTICE_HPP */
