// TH02 Stage 4 actor checkpoint and clean-Practice ownership. This patch-only
// segment follows the earlier patch tails and leaves every native contribution
// byte-stable.
#pragma option -zCT2S4ACT_TEXT -G-

#include "platform.h"
#include "pc98.h"
#include "th01/rank.h"
#include "th02/core/globals.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/s4_actor.hpp"
#include "th02/main/bg_particle.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/explode.hpp"
#include "th02/main/laser.hpp"
#include "th02/main/midboss/midboss.hpp"

extern "C" int patnum_2064E;
extern "C" uint8_t boss_phase;
extern "C" bool boss_hit_flash;
extern "C" uint8_t boss_rank_param[5];
extern "C" uint8_t bomb_damage_frame_mask;
extern "C" int boss_pos_x;
extern "C" int boss_pos_y;

extern "C" int marisa_intro_direction;
extern "C" int16_t midboss4_defeat_frame;
extern "C" screen_y_t midboss4_pellet_top;
extern "C" bool16 midboss4_active;
extern "C" uint8_t midboss4_pattern;
extern "C" uint8_t midboss4_patterns_seen;

extern "C" int marisa_defeat_frame;
extern "C" int marisa_intro_step;
extern "C" int8_t marisa_pattern;
extern "C" int8_t marisa_pattern_side;
extern "C" uint8_t marisa_star_drift_angle;
extern "C" uint8_t marisa_patterns_seen;
extern "C" int8_t marisa_orbless_patterns_seen;
extern "C" uint8_t marisa_rounds_done;
extern "C" int8_t marisa_bg_particle_col_i;
extern "C" uint8_t marisa_damage_multiplier;
extern "C" bool marisa_spray_is_first_run;
extern "C" uint8_t angle_26D7F;
extern "C" uint8_t angle_26D80;
extern "C" uint8_t angle_26D87;
extern "C" uint8_t angle_26D88;
extern "C" int8_t marisa_swoop_direction;
extern "C" screen_x_t marisa_swoop_center_x;
extern "C" screen_y_t marisa_swoop_center_y;
extern "C" uint8_t marisa_swoop_angle;
extern "C" uint8_t marisa_volleys_fired;
extern "C" uint8_t marisa_orb_volley_angle[MARISA_ORB_COUNT];
extern "C" int marisa_orb_kill_frame[MARISA_ORB_COUNT];
extern "C" vram_y_t stage4_tile_top;

static const int MIDBOSS4_FIRST_SCROLL_STEP = 944;
static const int MIDBOSS4_SECOND_SCROLL_STEP = 1632;
static const int MARISA_PATNUM = 128;

static bool16 near th02_s4_direction_valid(int direction)
{
	return ((direction >= -1) && (direction <= 1));
}

static bool16 near th02_s4_topleft_valid(const screen_point_t& topleft)
{
	return (
		(topleft.x >= (PLAYFIELD_LEFT - MARISA_W)) &&
		(topleft.x <= PLAYFIELD_RIGHT) &&
		(topleft.y >= -256) &&
		(topleft.y < (RES_Y + 256))
	);
}

static bool16 near th02_s4_midboss_state_validate(
	const th02_s4_midboss_state_t *state
)
{
	return (
		(state != 0) &&
		th02_s4_topleft_valid(state->topleft) &&
		(state->intro_step >= 0) &&
		(state->intro_step <= 1) &&
		th02_s4_direction_valid(state->intro_direction) &&
		(state->defeat_frame >= 0) &&
		(state->defeat_frame < 64) &&
		(state->pellet_top >= -256) &&
		(state->pellet_top < (RES_Y + 256)) &&
		((state->active == false) || (state->active == true)) &&
		(state->pattern < 5) &&
		(state->patterns_seen <= 12)
	);
}

bool16 far th02_s4_midboss_state_capture(th02_s4_midboss_state_t *state)
{
	th02_s4_midboss_state_t captured;

	captured.topleft = marisa_topleft;
	captured.intro_step = marisa_intro_step;
	captured.intro_direction = marisa_intro_direction;
	captured.defeat_frame = midboss4_defeat_frame;
	captured.pellet_top = midboss4_pellet_top;
	captured.active = midboss4_active;
	captured.pattern = midboss4_pattern;
	captured.patterns_seen = midboss4_patterns_seen;
	if(!th02_s4_midboss_state_validate(&captured) || (state == 0)) {
		return false;
	}
	*state = captured;
	return true;
}

bool16 far th02_s4_midboss_state_apply(
	const th02_s4_midboss_state_t *state
)
{
	if(!th02_s4_midboss_state_validate(state)) {
		return false;
	}
	marisa_topleft = state->topleft;
	marisa_intro_step = state->intro_step;
	marisa_intro_direction = state->intro_direction;
	midboss4_defeat_frame = state->defeat_frame;
	midboss4_pellet_top = state->pellet_top;
	midboss4_active = state->active;
	midboss4_pattern = state->pattern;
	midboss4_patterns_seen = state->patterns_seen;
	return true;
}

static bool16 near th02_s4_pattern_valid(int8_t pattern)
{
	return (
		((pattern >= -3) && (pattern <= 6)) ||
		(pattern == MP_UNSTARTED)
	);
}

static bool16 near th02_s4_marisa_state_validate(
	const th02_s4_marisa_state_t *state
)
{
	int i;
	int page;

	if(
		(state == 0) ||
		!th02_s4_topleft_valid(state->topleft) ||
		(state->velocity_x < -1) ||
		(state->velocity_x > 1) ||
		(state->velocity_y < -1) ||
		(state->velocity_y > 1) ||
		(state->intro_step < 0) ||
		(state->intro_step > 2) ||
		!th02_s4_direction_valid(state->intro_direction) ||
		!th02_s4_pattern_valid(state->pattern) ||
		!((state->pattern_side == -1) || (state->pattern_side == 1)) ||
		!((state->star_drift_angle == 2) ||
		  (state->star_drift_angle == 0xFE)) ||
		(state->patterns_seen > MARISA_PATTERNS_PER_ROUND) ||
		(state->orbless_patterns_seen < 0) ||
		(state->orbless_patterns_seen > 2) ||
		(state->rounds_done > MARISA_ROUNDS) ||
		(state->defeat_frame < 0) ||
		(state->defeat_frame >= MARISA_DEFEAT_FRAMES) ||
		(state->damage_multiplier > 1) ||
		(state->bg_particle_col_i < 0) ||
		(state->bg_particle_col_i >= MARISA_BG_PARTICLE_COLS_COUNT) ||
		!((state->spray_is_first_run == false) ||
		  (state->spray_is_first_run == true)) ||
		!th02_s4_direction_valid(state->swoop_direction) ||
		(state->swoop_center_x < (PLAYFIELD_LEFT - MARISA_SWOOP_RADIUS)) ||
		(state->swoop_center_x >= (PLAYFIELD_RIGHT + MARISA_SWOOP_RADIUS)) ||
		(state->swoop_center_y < -MARISA_SWOOP_RADIUS) ||
		(state->swoop_center_y >= (RES_Y + MARISA_SWOOP_RADIUS)) ||
		(state->orb_flag_sum < 0) ||
		(state->orb_flag_sum > (MARISA_ORB_COUNT * MOF_REMOVED))
	) {
		return false;
	}
	for(i = 0; i < MARISA_ORB_COUNT; i++) {
		if(
			(state->orb_flag[i] < MOF_ALIVE) ||
			(state->orb_flag[i] > MOF_REMOVED) ||
			(state->orb_damage[i] < 0) ||
			!((state->orb_hit_flash[i] == false) ||
			  (state->orb_hit_flash[i] == true)) ||
			(state->orb_kill_frame[i] < 0) ||
			(state->orb_kill_frame[i] >= MARISA_ORB_KILL_FRAMES) ||
			(state->orb_radius[i] < 0) ||
			(state->orb_radius[i] >= 512) ||
			(state->orb_angle_delta[i] < -4) ||
			(state->orb_angle_delta[i] > 4)
		) {
			return false;
		}
		for(page = 0; page < PAGE_COUNT; page++) {
			if(
				(state->orb_left_on_page[page][i] < -256) ||
				(state->orb_left_on_page[page][i] >= (RES_X + 256)) ||
				(state->orb_top_on_page[page][i] < -256) ||
				(state->orb_top_on_page[page][i] >= (RES_Y + 256))
			) {
				return false;
			}
		}
	}
	return true;
}

bool16 far th02_s4_marisa_state_capture(th02_s4_marisa_state_t *state)
{
	th02_s4_marisa_state_t captured;
	int i;
	int page;

	captured.topleft = marisa_topleft;
	captured.velocity_x = marisa_velocity_x;
	captured.velocity_y = marisa_velocity_y;
	captured.intro_step = marisa_intro_step;
	captured.intro_direction = marisa_intro_direction;
	captured.pattern = marisa_pattern;
	captured.pattern_side = marisa_pattern_side;
	captured.star_drift_angle = marisa_star_drift_angle;
	captured.patterns_seen = marisa_patterns_seen;
	captured.orbless_patterns_seen = marisa_orbless_patterns_seen;
	captured.rounds_done = marisa_rounds_done;
	captured.defeat_frame = marisa_defeat_frame;
	captured.damage_multiplier = marisa_damage_multiplier;
	captured.bg_particle_col_i = marisa_bg_particle_col_i;
	captured.spray_is_first_run = marisa_spray_is_first_run;
	captured.pattern_angle[0] = angle_26D7F;
	captured.pattern_angle[1] = angle_26D80;
	captured.pattern_angle[2] = angle_26D87;
	captured.pattern_angle[3] = angle_26D88;
	captured.swoop_direction = marisa_swoop_direction;
	captured.swoop_center_x = marisa_swoop_center_x;
	captured.swoop_center_y = marisa_swoop_center_y;
	captured.swoop_angle = marisa_swoop_angle;
	captured.volleys_fired = marisa_volleys_fired;
	for(i = 0; i < MARISA_ORB_COUNT; i++) {
		captured.orb_volley_angle[i] = marisa_orb_volley_angle[i];
		captured.orb_flag[i] = marisa_orb_flag[i];
		captured.orb_damage[i] = marisa_orb_damage[i];
		captured.orb_hit_flash[i] = marisa_orb_hit_flash[i];
		captured.orb_kill_frame[i] = marisa_orb_kill_frame[i];
		for(page = 0; page < PAGE_COUNT; page++) {
			captured.orb_left_on_page[page][i] =
				marisa_orb_left_on_page[page][i];
			captured.orb_top_on_page[page][i] =
				marisa_orb_top_on_page[page][i];
		}
		captured.orb_radius[i] = marisa_orb_radius[i];
		captured.orb_angle[i] = marisa_orb_angle[i];
		captured.orb_angle_delta[i] = marisa_orb_angle_delta[i];
	}
	captured.orb_flag_sum = marisa_orb_flag_sum;
	if(!th02_s4_marisa_state_validate(&captured) || (state == 0)) {
		return false;
	}
	*state = captured;
	return true;
}

bool16 far th02_s4_marisa_state_apply(const th02_s4_marisa_state_t *state)
{
	int i;
	int page;

	if(!th02_s4_marisa_state_validate(state)) {
		return false;
	}
	marisa_topleft = state->topleft;
	marisa_velocity_x = state->velocity_x;
	marisa_velocity_y = state->velocity_y;
	marisa_intro_step = state->intro_step;
	marisa_intro_direction = state->intro_direction;
	marisa_pattern = state->pattern;
	marisa_pattern_side = state->pattern_side;
	marisa_star_drift_angle = state->star_drift_angle;
	marisa_patterns_seen = state->patterns_seen;
	marisa_orbless_patterns_seen = state->orbless_patterns_seen;
	marisa_rounds_done = state->rounds_done;
	marisa_defeat_frame = state->defeat_frame;
	marisa_damage_multiplier = state->damage_multiplier;
	marisa_bg_particle_col_i = state->bg_particle_col_i;
	marisa_spray_is_first_run = state->spray_is_first_run;
	angle_26D7F = state->pattern_angle[0];
	angle_26D80 = state->pattern_angle[1];
	angle_26D87 = state->pattern_angle[2];
	angle_26D88 = state->pattern_angle[3];
	marisa_swoop_direction = state->swoop_direction;
	marisa_swoop_center_x = state->swoop_center_x;
	marisa_swoop_center_y = state->swoop_center_y;
	marisa_swoop_angle = state->swoop_angle;
	marisa_volleys_fired = state->volleys_fired;
	for(i = 0; i < MARISA_ORB_COUNT; i++) {
		marisa_orb_volley_angle[i] = state->orb_volley_angle[i];
		marisa_orb_flag[i] = static_cast<marisa_orb_flag_t>(state->orb_flag[i]);
		marisa_orb_damage[i] = state->orb_damage[i];
		marisa_orb_hit_flash[i] = state->orb_hit_flash[i];
		marisa_orb_kill_frame[i] = state->orb_kill_frame[i];
		for(page = 0; page < PAGE_COUNT; page++) {
			marisa_orb_left_on_page[page][i] =
				state->orb_left_on_page[page][i];
			marisa_orb_top_on_page[page][i] =
				state->orb_top_on_page[page][i];
		}
		marisa_orb_left_on_back_page[i] =
			&marisa_orb_left_on_page[page_back][i];
		marisa_orb_top_on_back_page[i] =
			&marisa_orb_top_on_page[page_back][i];
		marisa_orb_radius[i] = state->orb_radius[i];
		marisa_orb_angle[i] = state->orb_angle[i];
		marisa_orb_angle_delta[i] = state->orb_angle_delta[i];
	}
	marisa_orb_flag_sum = state->orb_flag_sum;
	return true;
}

bool16 far th02_s4_field_state_capture(th02_s4_field_state_t *state)
{
	if((state == 0) || (stage4_tile_top >= RES_Y)) {
		return false;
	}
	state->tile_top = stage4_tile_top;
	return true;
}

bool16 far th02_s4_field_state_apply(const th02_s4_field_state_t *state)
{
	if((state == 0) || (state->tile_top >= RES_Y)) {
		return false;
	}
	stage4_tile_top = state->tile_top;
	return true;
}

static void near th02_s4_rank_params_set(void)
{
	if(rank != RANK_EASY) {
		boss_rank_param[0] = BG_5_SPREAD_MEDIUM_AIMED;
		boss_rank_param[1] = BG_4_SPREAD_MEDIUM_AIMED;
		boss_rank_param[2] = BG_2_SPREAD_MEDIUM;
		boss_rank_param[3] = 4;
	} else {
		boss_rank_param[0] = BG_5_SPREAD_WIDE_AIMED;
		boss_rank_param[1] = BG_4_SPREAD_WIDE_AIMED;
		boss_rank_param[2] = BG_1;
		boss_rank_param[3] = 2;
	}
}

bool16 far th02_s4_midboss_clean_init(
	th02_s4_midboss_clean_target_t target
)
{
	screen_x_t initial_left;
	int direction;

	switch(target) {
	case T2S4_MIDBOSS_FIRST:
		initial_left = (PLAYFIELD_LEFT + 32);
		direction = 1;
		midboss_scroll_step = MIDBOSS4_FIRST_SCROLL_STEP;
		patnum_2064E = 150;
		break;
	case T2S4_MIDBOSS_SECOND:
		initial_left = (PLAYFIELD_RIGHT - 32 - 64);
		direction = -1;
		midboss_scroll_step = MIDBOSS4_SECOND_SCROLL_STEP;
		patnum_2064E = 149;
		break;
	default:
		return false;
	}

	boss_left_on_page[0] = initial_left;
	boss_left_on_page[1] = initial_left;
	boss_top_on_page[0] = (PLAYFIELD_TOP - 32);
	boss_top_on_page[1] = (PLAYFIELD_TOP - 32);
	boss_left_on_page[page_back] += (direction << 3);
	boss_top_on_page[page_back] += 2;
	boss_left_on_back_page = &boss_left_on_page[page_back];
	boss_top_on_back_page = &boss_top_on_page[page_back];
	marisa_topleft.x = *boss_left_on_back_page;
	marisa_topleft.y = *boss_top_on_back_page;
	boss_damage = 0;
	boss_phase = 0;
	boss_phase_frame = 1;
	boss_hit_flash = false;
	marisa_intro_step = 0;
	marisa_intro_direction = direction;
	midboss4_defeat_frame = 0;
	midboss4_pellet_top = (marisa_topleft.y + 48);
	midboss4_pattern = 0;
	midboss4_patterns_seen = 0;
	midboss4_active = true;
	boss_pos_x = (marisa_topleft.x + 24);
	boss_pos_y = (marisa_topleft.y + 24);
	lasers_callbacks_set();
	return true;
}

void far th02_s4_marisa_clean_init(void)
{
	int i;
	int page;
	const screen_x_t initial_left =
		(PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - (MARISA_W / 2));
	const screen_y_t initial_top = (PLAYFIELD_TOP + 48);

	boss_left_on_page[0] = initial_left;
	boss_left_on_page[1] = initial_left;
	boss_top_on_page[0] = initial_top;
	boss_top_on_page[1] = initial_top;
	boss_left_on_back_page = &boss_left_on_page[page_back];
	boss_top_on_back_page = &boss_top_on_page[page_back];
	marisa_topleft.x = initial_left;
	marisa_topleft.y = initial_top;
	marisa_velocity_x = 0;
	marisa_velocity_y = 0;
	patnum_2064E = MARISA_PATNUM;
	boss_damage = 0;
	boss_phase = 0;
	boss_phase_frame = 0;
	boss_hit_flash = false;
	marisa_intro_step = 0;
	marisa_intro_direction = 0;
	marisa_pattern = 0;
	marisa_pattern_side = 1;
	marisa_star_drift_angle = 2;
	marisa_patterns_seen = 0;
	marisa_orbless_patterns_seen = 0;
	marisa_rounds_done = 0;
	marisa_defeat_frame = 0;
	marisa_damage_multiplier = 0;
	marisa_bg_particle_col_i = 0;
	marisa_spray_is_first_run = true;
	angle_26D7F = 0;
	angle_26D80 = 0;
	angle_26D87 = 0;
	angle_26D88 = 0;
	marisa_swoop_direction = 0;
	marisa_swoop_center_x = initial_left;
	marisa_swoop_center_y = initial_top;
	marisa_swoop_angle = 0;
	marisa_volleys_fired = 0;
	marisa_orb_flag_sum = (MARISA_ORB_COUNT * MOF_REMOVED);
	for(i = 0; i < MARISA_ORB_COUNT; i++) {
		marisa_orb_volley_angle[i] = 0;
		marisa_orb_flag[i] = MOF_REMOVED;
		marisa_orb_damage[i] = 0;
		marisa_orb_hit_flash[i] = false;
		marisa_orb_kill_frame[i] = 0;
		marisa_orb_radius[i] = 0;
		marisa_orb_angle[i] = 0;
		marisa_orb_angle_delta[i] = 0;
		for(page = 0; page < PAGE_COUNT; page++) {
			marisa_orb_left_on_page[page][i] = (initial_left + 32);
			marisa_orb_top_on_page[page][i] = (initial_top + 32);
		}
		marisa_orb_left_on_back_page[i] =
			&marisa_orb_left_on_page[page_back][i];
		marisa_orb_top_on_back_page[i] =
			&marisa_orb_top_on_page[page_back][i];
	}
	bomb_damage_frame_mask = 1;
	boss_explode_angle_offset = 0xE0;
	bg_particle_col = 9;
	bg_particle_edge_step = 24;
	bg_particle_speed_initial = 24;
	bg_particle_speed_delta = 4;
	th02_s4_rank_params_set();
	lasers_callbacks_set();
}
