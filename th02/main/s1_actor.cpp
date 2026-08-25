// TH02 Stage 1 actor checkpoint and clean-Practice ownership. This patch-only
// segment follows every native, replay, and common Practice segment.
#pragma option -zCT2S1ACT_TEXT -G-

#include "platform.h"
#include "pc98.h"
#include "th01/rank.h"
#include "th02/core/globals.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/frames.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/s1_actor.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/explode.hpp"
#include "th02/main/laser.hpp"

extern "C" int patnum_2064E;
extern "C" uint8_t boss_phase;
extern "C" uint8_t boss_rank_param[5];

extern "C" int16_t stage1_scenery_frame;
extern "C" int16_t midboss1_defeat_frame;
extern "C" bool16 midboss1_active;
extern "C" int16_t midboss1_patnum;

extern "C" screen_point_t rika_topleft;
extern "C" int16_t rika_move_state;
extern "C" int16_t rika_defeat_frame;
extern "C" uint8_t rika_pattern_angle;
extern "C" uint16_t rika_pattern_frame;

static const int MIDBOSS1_PATNUM = 148;
static const int MIDBOSS1_PATNUM_ALT = 149;
static const int RIKA_PATNUM = 150;

static bool16 near th02_s1_midboss_state_validate(
	const th02_s1_midboss_state_t *state
)
{
	return (
		(state != 0) &&
		(state->defeat_frame >= 0) &&
		(state->defeat_frame < 64) &&
		(
			(state->patnum == MIDBOSS1_PATNUM) ||
			(state->patnum == MIDBOSS1_PATNUM_ALT)
		) &&
		((state->active == false) || (state->active == true))
	);
}

static bool16 near th02_s1_rika_state_validate(
	const th02_s1_rika_state_t *state
)
{
	return (
		(state != 0) &&
		(state->topleft.x >= 0) &&
		(state->topleft.x <= (RES_X - 64)) &&
		(state->topleft.y >= 0) &&
		(state->topleft.y < RES_Y) &&
		(state->move_state >= 0) &&
		(state->move_state <= 2) &&
		(state->defeat_frame >= 0) &&
		(state->defeat_frame < 80) &&
		(state->pattern_angle < 0x80)
	);
}

bool16 far th02_s1_midboss_state_capture(th02_s1_midboss_state_t *state)
{
	th02_s1_midboss_state_t captured;

	captured.defeat_frame = midboss1_defeat_frame;
	captured.patnum = midboss1_patnum;
	captured.active = midboss1_active;
	if(!th02_s1_midboss_state_validate(&captured) || (state == 0)) {
		return false;
	}
	*state = captured;
	return true;
}

bool16 far th02_s1_midboss_state_apply(
	const th02_s1_midboss_state_t *state
)
{
	if(!th02_s1_midboss_state_validate(state)) {
		return false;
	}
	midboss1_defeat_frame = state->defeat_frame;
	midboss1_patnum = state->patnum;
	midboss1_active = state->active;
	return true;
}

bool16 far th02_s1_rika_state_capture(th02_s1_rika_state_t *state)
{
	th02_s1_rika_state_t captured;

	captured.topleft = rika_topleft;
	captured.move_state = rika_move_state;
	captured.defeat_frame = rika_defeat_frame;
	captured.pattern_angle = rika_pattern_angle;
	captured.pattern_frame = rika_pattern_frame;
	if(!th02_s1_rika_state_validate(&captured) || (state == 0)) {
		return false;
	}
	*state = captured;
	return true;
}

bool16 far th02_s1_rika_state_apply(const th02_s1_rika_state_t *state)
{
	if(!th02_s1_rika_state_validate(state)) {
		return false;
	}
	rika_topleft = state->topleft;
	rika_move_state = state->move_state;
	rika_defeat_frame = state->defeat_frame;
	rika_pattern_angle = state->pattern_angle;
	rika_pattern_frame = state->pattern_frame;
	return true;
}

void far th02_s1_midboss_clean_init(void)
{
	boss_left_on_page[0] = (PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 32);
	boss_left_on_page[1] = (PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 32);
	boss_top_on_page[0] = (PLAYFIELD_TOP - 32);
	boss_top_on_page[1] = (PLAYFIELD_TOP - 32);
	boss_left_on_back_page = &boss_left_on_page[page_back];
	boss_top_on_back_page = &boss_top_on_page[page_back];
	boss_damage = 0;
	boss_phase = 0;
	boss_phase_frame = 1;
	midboss1_defeat_frame = 0;
	midboss1_patnum = MIDBOSS1_PATNUM;
	midboss1_active = true;
	stage1_scenery_frame = 1;
}

bool16 far th02_s1_rika_clean_init(th02_s1_rika_clean_target_t target)
{
	int initial_damage;
	int initial_left;
	int initial_top;

	switch(target) {
	case T2S1_RIKA_START:
		initial_damage = 0;
		break;
	case T2S1_RIKA_DAMAGE_700:
		initial_damage = 700;
		break;
	case T2S1_RIKA_DAMAGE_1400:
		initial_damage = 1400;
		break;
	default:
		return false;
	}

	initial_left = (PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 32);
	initial_top = (48 + scroll_line);
	if(initial_top >= RES_Y) {
		initial_top -= RES_Y;
	}
	boss_left_on_page[0] = initial_left;
	boss_left_on_page[1] = initial_left;
	boss_top_on_page[0] = 48;
	boss_top_on_page[1] = 48;
	boss_left_on_back_page = &boss_left_on_page[page_back];
	boss_top_on_back_page = &boss_top_on_page[page_back];
	boss_damage = initial_damage;
	boss_phase = 0;
	boss_phase_frame = 0;
	patnum_2064E = RIKA_PATNUM;
	boss_explode_angle_offset = 0x20;
	rika_topleft.x = initial_left;
	rika_topleft.y = initial_top;
	rika_move_state = 0;
	rika_defeat_frame = 0;
	rika_pattern_angle = 0;
	rika_pattern_frame = 0;

	if(rank != RANK_EASY) {
		boss_rank_param[0] = BG_3_SPREAD_MEDIUM_AIMED;
		boss_rank_param[1] = BG_5_SPREAD_WIDE;
		boss_rank_param[2] = BG_1;
		boss_rank_param[3] = BG_2_SPREAD_MEDIUM;
		boss_rank_param[4] = 3;
	} else {
		boss_rank_param[0] = BG_1_AIMED;
		boss_rank_param[1] = BG_5_SPREAD_WIDE;
		boss_rank_param[2] = BG_1;
		boss_rank_param[3] = BG_1;
		boss_rank_param[4] = 7;
	}
	lasers_callbacks_set();
	return true;
}
