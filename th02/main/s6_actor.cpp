// TH02 Extra Stage clean-Practice ownership. This patch-only segment follows
// every native contribution and the earlier actor tails.
#pragma option -zCT2S6ACT_TEXT -G-

#include "platform.h"
#include "pc98.h"
#include "th02/hardware/pages.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/boss/bosses.hpp"
#include "th02/main/s6_actor.hpp"

extern "C" int patnum_2064E;
extern "C" uint8_t boss_phase;
extern "C" bool boss_hit_flash;

extern screen_point_t sigma_topleft;
extern "C" uint8_t sigma_phase;
extern "C" uint8_t sigma_cel_interval_mask;
extern "C" screen_x_t sigma_center_x;
extern "C" screen_y_t sigma_center_y;
extern "C" uint8_t sigma_pattern;
extern "C" uint8_t sigma_pattern_looped_unused;
extern "C" int sigma_phase_damage_max;
extern "C" uint8_t sigma_ring_radius;
extern "C" int8_t sigma_blast_hitbox_margin;

void far th02_s6_sigma_clean_init(void)
{
	const screen_x_t initial_left = (
		PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 64
	);
	const screen_y_t initial_top = (PLAYFIELD_TOP + 32);

	boss_left_on_page[0] = initial_left;
	boss_left_on_page[1] = initial_left;
	boss_top_on_page[0] = initial_top;
	boss_top_on_page[1] = initial_top;
	boss_left_on_back_page = &boss_left_on_page[page_back];
	boss_top_on_back_page = &boss_top_on_page[page_back];
	boss_damage = 0;
	boss_phase = 0;
	boss_phase_frame = 0;
	boss_hit_flash = false;
	patnum_2064E = 128;

	sigma_topleft.x = initial_left;
	sigma_topleft.y = initial_top;
	sigma_center_x = (initial_left + 60);
	sigma_center_y = (initial_top + 60);
	sigma_phase = 0;
	sigma_pattern = 0;
	sigma_pattern_looped_unused = 0;
	sigma_phase_damage_max = 0;
	sigma_ring_radius = 0;
	sigma_cel_interval_mask = 7;
	sigma_blast_hitbox_margin = 0;
	sigma_frame = 0;
}
