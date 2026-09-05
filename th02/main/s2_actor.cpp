// TH02 Stage 2 actor checkpoint and clean-Practice ownership. This patch-only
// segment follows the Stage 1 actor substrate and has no caller yet.
#pragma option -zCT2S2ACT_TEXT -G-

#include "platform.h"
#include "pc98.h"
#include "th01/rank.h"
#include "th02/core/globals.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/frames.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/s2_actor.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/explode.hpp"

extern "C" int patnum_2064E;
extern "C" uint8_t boss_phase;
extern "C" uint8_t boss_rank_param[5];

extern "C" int16_t bg_flash_frame;
extern "C" bool16 midboss2_active;
extern "C" int16_t midboss2_defeat_frame;

extern "C" uint8_t meira_phase;
extern "C" uint8_t meira_pattern;
extern "C" int meira_defeat_frame;
extern "C" bool16 meira_afterimages_active;
extern "C" screen_x_t meira_dash_origin_x;
extern "C" screen_y_t meira_dash_origin_y;
extern "C" screen_x_t meira_dash_target_x;
extern "C" screen_y_t meira_dash_target_y;
extern "C" int meira_dash_step;
extern "C" screen_x_t near meira_afterimage_left[PAGE_COUNT][
	TH02_S2_MEIRA_AFTERIMAGE_SLOTS
];
extern "C" screen_y_t near meira_afterimage_top[PAGE_COUNT][
	TH02_S2_MEIRA_AFTERIMAGE_SLOTS
];
extern "C" uint8_t meira_player_is_right;
extern "C" uint8_t meira_slash_trail_patnum;
extern "C" int16_t meira_slash_trail_frames;
extern "C" uint8_t meira_slash_burst_i;
extern "C" uint8_t meira_burst_group;
extern "C" uint8_t meira_burst_speed;
extern "C" subpixel_t meira_252E8;
extern "C" subpixel_t meira_252EA;
extern "C" th02_s2_meira_slash_state_t near meira_slashes[
	TH02_S2_MEIRA_SLASH_COUNT
];

static const int MIDBOSS2_DEFEAT_FRAMES = 64;
static const int MEIRA_DEFEAT_FRAMES = 96;
static const int MEIRA_DASH_STEPS = 64;
static const int MEIRA_DASH_STEP_POST_LANDING = (MEIRA_DASH_STEPS + 1);
static const int MEIRA_SLASH_TRAIL_FRAMES_MAX = 0x50;
static const main_patnum_t MEIRA_PATNUM = 141;
static const main_patnum_t MEIRA_SLASH_TRAIL_PATNUM_SLASH = 120;
static const main_patnum_t MEIRA_SLASH_TRAIL_PATNUM_BACKSLASH = 121;

static bool16 near th02_s2_bool_valid(bool16 value)
{
	return ((value == false) || (value == true));
}

static bool16 near th02_s2_screen_point_valid(screen_x_t x, screen_y_t y)
{
	return ((x >= 0) && (x < RES_X) && (y >= 0) && (y < RES_Y));
}

static bool16 near th02_s2_slash_state_validate(
	const th02_s2_meira_slash_state_t *slash
)
{
	if(slash == 0) {
		return false;
	}
	if(
		(slash->flag != F_FREE) &&
		(slash->flag != F_ALIVE) &&
		(slash->flag != F_REMOVE)
	) {
		return false;
	}
	if(slash->flag == F_FREE) {
		// Native removal leaves the rest of the record untouched.
		return true;
	}
	if(!th02_s2_screen_point_valid(slash->left, slash->top)) {
		return false;
	}
	if(slash->flag == F_REMOVE) {
		return true;
	}
	return (
		(slash->trail_patnum == MEIRA_SLASH_TRAIL_PATNUM_SLASH) ||
		(slash->trail_patnum == MEIRA_SLASH_TRAIL_PATNUM_BACKSLASH)
	);
}

static bool16 near th02_s2_midboss_state_validate(
	const th02_s2_midboss_state_t *state
)
{
	return (
		(state != 0) &&
		(state->defeat_frame >= 0) &&
		(state->defeat_frame < MIDBOSS2_DEFEAT_FRAMES) &&
		th02_s2_bool_valid(state->active)
	);
}

static bool16 near th02_s2_meira_state_validate(
	const th02_s2_meira_state_t *state
)
{
	int page;
	int slash_i;

	if(
		(state == 0) ||
		(state->phase > 2) ||
		((state->phase == 0) && (state->pattern > 3)) ||
		((state->phase == 1) && (state->pattern > 2)) ||
		((state->phase == 2) && (state->pattern > 1)) ||
		(state->defeat_frame < 0) ||
		(state->defeat_frame >= MEIRA_DEFEAT_FRAMES) ||
		!th02_s2_bool_valid(state->afterimages_active) ||
		!th02_s2_screen_point_valid(
			state->dash_origin_x, state->dash_origin_y
		) ||
		(state->dash_target_x < PLAYFIELD_LEFT) ||
		(state->dash_target_x > (PLAYFIELD_RIGHT - 64)) ||
		(state->dash_target_y < PLAYFIELD_TOP) ||
		(state->dash_target_y > (PLAYFIELD_TOP + 255)) ||
		(state->dash_step < 0) ||
		(state->dash_step > MEIRA_DASH_STEP_POST_LANDING) ||
		(state->player_is_right > 1) ||
		(
			(state->slash_trail_patnum != 0) &&
			(state->slash_trail_patnum != MEIRA_SLASH_TRAIL_PATNUM_SLASH) &&
			(state->slash_trail_patnum != MEIRA_SLASH_TRAIL_PATNUM_BACKSLASH)
		) ||
		(state->slash_trail_frames < 0) ||
		(state->slash_trail_frames > MEIRA_SLASH_TRAIL_FRAMES_MAX)
	) {
		return false;
	}
	for(page = 0; page < PAGE_COUNT; page++) {
		for(slash_i = 0; slash_i < TH02_S2_MEIRA_AFTERIMAGE_SLOTS; slash_i++) {
			if(!th02_s2_screen_point_valid(
				state->afterimage_left[page][slash_i],
				state->afterimage_top[page][slash_i]
			)) {
				return false;
			}
		}
	}
	for(slash_i = 0; slash_i < TH02_S2_MEIRA_SLASH_COUNT; slash_i++) {
		if(!th02_s2_slash_state_validate(&state->slashes[slash_i])) {
			return false;
		}
	}
	return true;
}

bool16 far th02_s2_midboss_state_capture(th02_s2_midboss_state_t *state)
{
	th02_s2_midboss_state_t captured;

	captured.defeat_frame = midboss2_defeat_frame;
	captured.active = midboss2_active;
	if(!th02_s2_midboss_state_validate(&captured) || (state == 0)) {
		return false;
	}
	state->defeat_frame = captured.defeat_frame;
	state->active = captured.active;
	return true;
}

bool16 far th02_s2_midboss_state_apply(
	const th02_s2_midboss_state_t *state
)
{
	if(!th02_s2_midboss_state_validate(state)) {
		return false;
	}
	midboss2_defeat_frame = state->defeat_frame;
	midboss2_active = state->active;
	return true;
}

bool16 far th02_s2_meira_state_capture(th02_s2_meira_state_t *state)
{
	th02_s2_meira_state_t captured;
	int page;
	int slash_i;

	captured.phase = meira_phase;
	captured.pattern = meira_pattern;
	captured.defeat_frame = meira_defeat_frame;
	captured.afterimages_active = meira_afterimages_active;
	captured.dash_origin_x = meira_dash_origin_x;
	captured.dash_origin_y = meira_dash_origin_y;
	captured.dash_target_x = meira_dash_target_x;
	captured.dash_target_y = meira_dash_target_y;
	captured.dash_step = meira_dash_step;
	for(page = 0; page < PAGE_COUNT; page++) {
		for(slash_i = 0; slash_i < TH02_S2_MEIRA_AFTERIMAGE_SLOTS; slash_i++) {
			captured.afterimage_left[page][slash_i] =
				meira_afterimage_left[page][slash_i];
			captured.afterimage_top[page][slash_i] =
				meira_afterimage_top[page][slash_i];
		}
	}
	captured.player_is_right = meira_player_is_right;
	captured.slash_trail_patnum = meira_slash_trail_patnum;
	captured.slash_trail_frames = meira_slash_trail_frames;
	captured.slash_burst_i = meira_slash_burst_i;
	captured.burst_group = meira_burst_group;
	captured.burst_speed = meira_burst_speed;
	captured.ramp_speed_a = meira_252E8;
	captured.ramp_speed_b = meira_252EA;
	for(slash_i = 0; slash_i < TH02_S2_MEIRA_SLASH_COUNT; slash_i++) {
		captured.slashes[slash_i].flag = meira_slashes[slash_i].flag;
		captured.slashes[slash_i].group = meira_slashes[slash_i].group;
		captured.slashes[slash_i].left = meira_slashes[slash_i].left;
		captured.slashes[slash_i].top = meira_slashes[slash_i].top;
		captured.slashes[slash_i].angle = meira_slashes[slash_i].angle;
		captured.slashes[slash_i].age = meira_slashes[slash_i].age;
		captured.slashes[slash_i].speed = meira_slashes[slash_i].speed;
		captured.slashes[slash_i].trail_patnum =
			meira_slashes[slash_i].trail_patnum;
		captured.slashes[slash_i].trail_frames =
			meira_slashes[slash_i].trail_frames;
	}
	if(!th02_s2_meira_state_validate(&captured) || (state == 0)) {
		return false;
	}
	state->phase = captured.phase;
	state->pattern = captured.pattern;
	state->defeat_frame = captured.defeat_frame;
	state->afterimages_active = captured.afterimages_active;
	state->dash_origin_x = captured.dash_origin_x;
	state->dash_origin_y = captured.dash_origin_y;
	state->dash_target_x = captured.dash_target_x;
	state->dash_target_y = captured.dash_target_y;
	state->dash_step = captured.dash_step;
	for(page = 0; page < PAGE_COUNT; page++) {
		for(slash_i = 0; slash_i < TH02_S2_MEIRA_AFTERIMAGE_SLOTS; slash_i++) {
			state->afterimage_left[page][slash_i] =
				captured.afterimage_left[page][slash_i];
			state->afterimage_top[page][slash_i] =
				captured.afterimage_top[page][slash_i];
		}
	}
	state->player_is_right = captured.player_is_right;
	state->slash_trail_patnum = captured.slash_trail_patnum;
	state->slash_trail_frames = captured.slash_trail_frames;
	state->slash_burst_i = captured.slash_burst_i;
	state->burst_group = captured.burst_group;
	state->burst_speed = captured.burst_speed;
	state->ramp_speed_a = captured.ramp_speed_a;
	state->ramp_speed_b = captured.ramp_speed_b;
	for(slash_i = 0; slash_i < TH02_S2_MEIRA_SLASH_COUNT; slash_i++) {
		state->slashes[slash_i].flag = captured.slashes[slash_i].flag;
		state->slashes[slash_i].group = captured.slashes[slash_i].group;
		state->slashes[slash_i].left = captured.slashes[slash_i].left;
		state->slashes[slash_i].top = captured.slashes[slash_i].top;
		state->slashes[slash_i].angle = captured.slashes[slash_i].angle;
		state->slashes[slash_i].age = captured.slashes[slash_i].age;
		state->slashes[slash_i].speed = captured.slashes[slash_i].speed;
		state->slashes[slash_i].trail_patnum =
			captured.slashes[slash_i].trail_patnum;
		state->slashes[slash_i].trail_frames =
			captured.slashes[slash_i].trail_frames;
	}
	return true;
}

bool16 far th02_s2_meira_state_apply(const th02_s2_meira_state_t *state)
{
	int page;
	int slash_i;

	if(!th02_s2_meira_state_validate(state)) {
		return false;
	}
	meira_phase = state->phase;
	meira_pattern = state->pattern;
	meira_defeat_frame = state->defeat_frame;
	meira_afterimages_active = state->afterimages_active;
	meira_dash_origin_x = state->dash_origin_x;
	meira_dash_origin_y = state->dash_origin_y;
	meira_dash_target_x = state->dash_target_x;
	meira_dash_target_y = state->dash_target_y;
	meira_dash_step = state->dash_step;
	for(page = 0; page < PAGE_COUNT; page++) {
		for(slash_i = 0; slash_i < TH02_S2_MEIRA_AFTERIMAGE_SLOTS; slash_i++) {
			meira_afterimage_left[page][slash_i] =
				state->afterimage_left[page][slash_i];
			meira_afterimage_top[page][slash_i] =
				state->afterimage_top[page][slash_i];
		}
	}
	meira_player_is_right = state->player_is_right;
	meira_slash_trail_patnum = state->slash_trail_patnum;
	meira_slash_trail_frames = state->slash_trail_frames;
	meira_slash_burst_i = state->slash_burst_i;
	meira_burst_group = state->burst_group;
	meira_burst_speed = state->burst_speed;
	meira_252E8 = state->ramp_speed_a;
	meira_252EA = state->ramp_speed_b;
	for(slash_i = 0; slash_i < TH02_S2_MEIRA_SLASH_COUNT; slash_i++) {
		meira_slashes[slash_i].flag = state->slashes[slash_i].flag;
		meira_slashes[slash_i].group = state->slashes[slash_i].group;
		meira_slashes[slash_i].left = state->slashes[slash_i].left;
		meira_slashes[slash_i].top = state->slashes[slash_i].top;
		meira_slashes[slash_i].angle = state->slashes[slash_i].angle;
		meira_slashes[slash_i].age = state->slashes[slash_i].age;
		meira_slashes[slash_i].speed = state->slashes[slash_i].speed;
		meira_slashes[slash_i].trail_patnum =
			state->slashes[slash_i].trail_patnum;
		meira_slashes[slash_i].trail_frames =
			state->slashes[slash_i].trail_frames;
	}
	return true;
}

void far th02_s2_midboss_clean_init(void)
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
	midboss2_defeat_frame = 0;
	midboss2_active = true;
	bg_flash_frame = 1;
}

bool16 far th02_s2_meira_clean_init(th02_s2_meira_clean_target_t target)
{
	int initial_damage;
	int i;
	int page;
	const screen_x_t initial_left = (PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 32);
	const screen_y_t initial_top = (PLAYFIELD_TOP + 32);

	switch(target) {
	case T2S2_MEIRA_PHASE_1:
		initial_damage = 0;
		break;
	case T2S2_MEIRA_PHASE_2:
		initial_damage = 701;
		break;
	case T2S2_MEIRA_PHASE_3:
		initial_damage = 1501;
		break;
	default:
		return false;
	}

	boss_left_on_page[0] = initial_left;
	boss_left_on_page[1] = initial_left;
	boss_top_on_page[0] = initial_top;
	boss_top_on_page[1] = initial_top;
	boss_left_on_back_page = &boss_left_on_page[page_back];
	boss_top_on_back_page = &boss_top_on_page[page_back];
	boss_damage = initial_damage;
	boss_phase = 0;
	boss_phase_frame = 0;
	patnum_2064E = MEIRA_PATNUM;
	boss_explode_angle_offset = 0;
	bullet_special.u3.turns_max = 2;
	meira_phase = target;
	meira_pattern = 0;
	meira_defeat_frame = 0;
	meira_afterimages_active = false;
	meira_dash_origin_x = initial_left;
	meira_dash_origin_y = initial_top;
	meira_dash_target_x = initial_left;
	meira_dash_target_y = initial_top;
	meira_dash_step = 0;
	for(page = 0; page < PAGE_COUNT; page++) {
		for(i = 0; i < TH02_S2_MEIRA_AFTERIMAGE_SLOTS; i++) {
			meira_afterimage_left[page][i] = initial_left;
			meira_afterimage_top[page][i] = initial_top;
		}
	}
	meira_player_is_right = 0;
	meira_slash_trail_patnum = 0;
	meira_slash_trail_frames = 0;
	meira_slash_burst_i = 0;
	meira_burst_group = 0;
	meira_burst_speed = 0;
	meira_252E8 = 0;
	meira_252EA = 0;
	for(i = 0; i < TH02_S2_MEIRA_SLASH_COUNT; i++) {
		meira_slashes[i].flag = F_FREE;
		meira_slashes[i].group = 0;
		meira_slashes[i].left = 0;
		meira_slashes[i].top = 0;
		meira_slashes[i].angle = 0;
		meira_slashes[i].age = 0;
		meira_slashes[i].speed = 0;
		meira_slashes[i].trail_patnum = 0;
		meira_slashes[i].trail_frames = 0;
	}
	if(rank != RANK_EASY) {
		boss_rank_param[0] = BG_1_RANDOM_ANGLE;
		boss_rank_param[1] = BG_5_SPREAD_WIDE;
		boss_rank_param[2] = BG_5_SPREAD_MEDIUM_AIMED;
	} else {
		boss_rank_param[0] = BG_1_RANDOM_ANGLE;
		boss_rank_param[1] = BG_3_SPREAD_WIDE;
		boss_rank_param[2] = BG_3_SPREAD_MEDIUM_AIMED;
	}
	return true;
}
