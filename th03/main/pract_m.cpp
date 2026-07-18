#pragma option -zCPRACTICEM_TEXT

#include "th01/rank.h"
#include "th03/main/difficul.hpp"
#include "th03/main/player/cpu.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/main/round.hpp"
#include "th03/practice.hpp"

extern "C" uint8_t story_cpu_safety_frames[];

static uint16_t practice_vs_safety_frames(uint8_t rank)
{
	switch(rank) {
	case RANK_EASY:
		return 1000;
	case RANK_NORMAL:
		return 1900;
	case RANK_HARD:
		return 4000;
	default:
		return 0xFFFF;
	}
}

static uint16_t practice_story_safety_frames(
	uint8_t rank, uint8_t stage, uint8_t round
)
{
	unsigned long frames;

	if(round > 5) {
		round = 5;
	}
	frames = *reinterpret_cast<uint16_t near *>(
		&story_cpu_safety_frames[(stage * 12) + (round * 2)]
	);
	switch(rank) {
	case RANK_EASY:
		frames /= 3;
		break;
	case RANK_HARD:
		frames *= 2;
		break;
	case RANK_LUNATIC:
		frames = ((frames * 3) + 500);
		break;
	}
	if(frames > 0xFFFFUL) {
		frames = 0xFFFFUL;
	}
	return static_cast<uint16_t>(frames);
}

static uint16_t practice_safety_frames(
	uint8_t policy, uint8_t rank, uint8_t stage, uint8_t round
)
{
	if(policy == PRACTICE_CPU_TIMER_STORY_NATIVE) {
		return practice_story_safety_frames(rank, stage, round);
	}
	if(policy == PRACTICE_CPU_TIMER_INFINITE) {
		return 0xFFFF;
	}
	return practice_vs_safety_frames(rank);
}

void far practice_initial_apply(void)
{
	uint16_t safety;

	if(!practice_resident_active()) {
		return;
	}
	safety = practice_safety_frames(
		practice_resident_u8(T3_PRACTICE_RES_CPU_TIMER_INDEX),
		resident->rank,
		practice_resident_u8(T3_PRACTICE_RES_STAGE_INDEX),
		practice_resident_u8(T3_PRACTICE_RES_ROUND_INDEX)
	);
	players[0].cpu_safety_frames = safety;
	players[1].cpu_safety_frames = players[0].cpu_safety_frames;
	round_speed = practice_resident_u8(T3_PRACTICE_RES_ROUND_SPEED_INDEX);
	bullet_base_speed.v = practice_resident_u8(
		T3_PRACTICE_RES_BULLET_SPEED_INDEX
	);
	gba_gauge_level[0] = practice_resident_u8(T3_PRACTICE_RES_P1_SPELL_INDEX);
	gba_gauge_level[1] = practice_resident_u8(T3_PRACTICE_RES_CPU_SPELL_INDEX);
	gba_boss_level = practice_resident_u8(T3_PRACTICE_RES_BOSS_LEVEL_INDEX);
	cpu_hit_damage_additional = practice_resident_u8(
		T3_PRACTICE_RES_CPU_DAMAGE_INDEX
	);
}

void far practice_retry_apply(uint8_t p1_spell, uint8_t cpu_spell)
{
	uint8_t timer;
	uint16_t safety;

	if(!practice_resident_uses_stock()) {
		return;
	}
	timer = practice_resident_u8(T3_PRACTICE_RES_CPU_TIMER_INDEX);
	safety = practice_safety_frames(
		timer,
		resident->rank,
		practice_resident_u8(T3_PRACTICE_RES_STAGE_INDEX),
		round_id
	);
	players[0].cpu_safety_frames = safety;
	players[1].cpu_safety_frames = safety;
	gba_gauge_level[0] = p1_spell;
	gba_gauge_level[1] = cpu_spell;
	cpu_hit_damage_additional = practice_resident_u8(
		T3_PRACTICE_RES_CPU_DAMAGE_INDEX
	);
}

// Keep the following original segments at their pre-practice paragraph phase.
#pragma codeseg PLAYFLD_TEXT main_01
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codeseg

// Keep the compiler runtime segment at its accepted paragraph phase.
#pragma codestring "\x90\x90\x90\x90\x90"
