// TH02 public later-boss Practice constructors. This isolated tail
// deliberately follows every retained patch contribution and owns no DATA.
#pragma option -zCT2LBP_TEXT -G-

#include "platform.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/math/polar.hpp"
#include "th02/core/globals.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/math/randring.hpp"
#include "th02/main/bg_particle.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/laser.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/s4_actor.hpp"
#include "th02/main/s5_actor.hpp"
#include "th02/main/s6_actor.hpp"
#include "th02/main/later_boss_practice.hpp"
#include "th02/v_colors.hpp"

extern "C" int patnum_2064E;
extern "C" int boss_pos_x;
extern "C" int boss_pos_y;

extern "C" void far marisa_bg_render(void);

extern "C" int mima_phase;
extern "C" int mima_pattern;
extern "C" int mima_patterns_this_phase;
extern "C" int mima_velocity_y;
extern "C" int mima_phase_damage_max;
extern "C" int mima_patterns_max;
extern "C" int mima_pattern_count;
extern "C" int mima_patterns_until_vulnerable;
extern "C" bool mima_all_patterns;
extern "C" uint8_t mima_bg_ring_radius;
extern "C" uint8_t mima_bg_circle_radius;
extern "C" uint8_t mima_bg_ring_col_head;
extern "C" uint8_t mima_bg_ring_col_tail;
extern "C" uint8_t mima_bg_circle_col;
extern "C" uint8_t mima_bg_ring_phase;
extern "C" uint8_t mima_ring_radius;
extern "C" uint8_t mima_bg_circle_radius_base;
extern "C" uint8_t mima_bg_circle_pulse_frame;
extern "C" void far mima_bg_render(void);
extern "C" const char mima2_bft[];

extern screen_point_t sigma_topleft;
extern "C" uint8_t sigma_phase;
extern "C" uint8_t sigma_pattern;
extern "C" uint8_t sigma_pattern_looped_unused;
extern "C" int sigma_phase_damage_max;
extern "C" uint8_t sigma_cel_interval_mask;
extern "C" int8_t sigma_blast_hitbox_margin;
extern "C" uint8_t sigma_ring_radius;
extern uint32_t sigma_frame;
extern "C" void far sigma_bg_render(void);
extern "C" void near sigma_15D56(void);
extern "C" void near sigma_15E84(void);
extern "C" void near sigma_15F6F(void);
extern "C" void near sigma_15F95(void);
extern "C" void near sigma_16176(void);
extern "C" void near sigma_1619C(void);
extern "C" void near sigma_162D3(void);

typedef void (near *near t2lbp_sigma_pattern_func_t)(void);
extern "C" t2lbp_sigma_pattern_func_t sigma_pattern_func[3];

// b6.cpp keeps this private because only Sigma's own updater normally owns it.
// A clean Phase 1 has no active blast, so construct the source-defined empty
// roster fieldwise instead of relying on the process-zero BSS accident.
#pragma pack(push, 1)
struct t2lbp_sigma_blast_t {
	uint8_t state;
	uint8_t state_frame;
	screen_x_t center_x;
	screen_y_t center_y;
	int radius_max;
	uint8_t patnum;
	uint8_t radius;
};
#pragma pack(pop)

typedef char t2lbp_sigma_blast_size_check[
	(sizeof(t2lbp_sigma_blast_t) == 10) ? 1 : -1
];

extern "C" t2lbp_sigma_blast_t near sigma_blasts[16];

extern Palette8 __cdecl Palettes;
extern void far pascal palette_show(void);

static void near t2lbp_pages_restore(void)
{
	page_front = 1;
	page_back = 0;
	boss_left_on_back_page = &boss_left_on_page[page_back];
	boss_top_on_back_page = &boss_top_on_page[page_back];
	graph_accesspage(page_back);
}

// randring2_next8() is a near leaf in an original segment. The tail cannot
// reach it without a 64 KiB fixup, so retain its source operation against the
// named ring owner rather than crossing a code-segment boundary.
static uint8_t near t2lbp_randring2_next8(void)
{
	return randring[randring_p++];
}

// Every admitted Marisa round begins with the same first regular-pattern
// state. The only round-owned fields are the completed-round count and the
// damage multiplier; th02_s4_marisa_clean_init() owns every persistent actor,
// callback, palette, and page-backed field before this adds the live orbs.
static void near t2lbp_marisa_regular_construct(
	uint8_t rounds_done, uint8_t damage_multiplier
)
{
	int i;

	th02_s4_marisa_clean_init();
	marisa_intro_step = 2;
	marisa_rounds_done = rounds_done;
	marisa_damage_multiplier = damage_multiplier;
	marisa_orbless_patterns_seen = 0;
	marisa_pattern = ((t2lbp_randring2_next8() % 5) + 1);
	marisa_patterns_seen = 1;
	boss_phase_frame = 1;
	marisa_orb_flag_sum = 0;
	boss_pos_x = -1;
	boss_pos_y = -1;
	for(i = 0; i < MARISA_ORB_COUNT; i++) {
		marisa_orb_flag[i] = MOF_ALIVE;
		marisa_orb_damage[i] = 0;
		marisa_orb_hit_flash[i] = false;
		marisa_orb_kill_frame[i] = 0;
		marisa_orb_radius[i] = 54;
		marisa_orb_angle[i] = (i << 6);
		marisa_orb_angle_delta[i] = 2;
		marisa_orb_left_on_page[0][i] = polar_x_fast(
			(marisa_topleft.x + 32), marisa_orb_radius[i], marisa_orb_angle[i]
		);
		marisa_orb_left_on_page[1][i] = marisa_orb_left_on_page[0][i];
		marisa_orb_top_on_page[0][i] = polar_y_fast(
			(marisa_topleft.y + 32), marisa_orb_radius[i], marisa_orb_angle[i]
		);
		marisa_orb_top_on_page[1][i] = marisa_orb_top_on_page[0][i];
	}
}

static void near t2lbp_marisa_orbs_put(void)
{
	int alive[MARISA_ORB_COUNT + 1];
	int from;
	int i;
	int line_count;
	int to;

	for(i = 0, line_count = 0; i < MARISA_ORB_COUNT; i++) {
		if(marisa_orb_flag[i] == MOF_ALIVE) {
			alive[line_count] = i;
			line_count++;
		}
	}
	alive[line_count] = alive[0];
	grcg_setcolor(GC_RMW, 3);
	for(i = 0; i < line_count; i++) {
		from = alive[i];
		to = alive[i + 1];
		grcg_line(
			(*marisa_orb_left_on_back_page[from] + 16),
			(*marisa_orb_top_on_back_page[from] + 16),
			(*marisa_orb_left_on_back_page[to] + 16),
			(*marisa_orb_top_on_back_page[to] + 16)
		);
	}
	grcg_off();
	for(i = 0; i < MARISA_ORB_COUNT; i++) {
		if(marisa_orb_flag[i] == MOF_ALIVE) {
			super_put_rect(
				*marisa_orb_left_on_back_page[i],
				*marisa_orb_top_on_back_page[i],
				(MARISA_ORB_PATNUM + i)
			);
		}
	}
}

static void near t2lbp_marisa_redraw_both(void)
{
	int page;

	for(page = 0; page < PAGE_COUNT; page++) {
		page_back = page;
		page_front = (1 - page);
		graph_accesspage(page_back);
		marisa_bg_render();
		super_put_rect(marisa_topleft.x, marisa_topleft.y, patnum_2064E);
		super_put_rect(
			(marisa_topleft.x + 48), marisa_topleft.y, (patnum_2064E + 1)
		);
		t2lbp_marisa_orbs_put();
	}
	t2lbp_pages_restore();
	for(page = 0; page < MARISA_ORB_COUNT; page++) {
		marisa_orb_left_on_back_page[page] =
			&marisa_orb_left_on_page[page_back][page];
		marisa_orb_top_on_back_page[page] =
			&marisa_orb_top_on_page[page_back][page];
	}
}

static void near t2lbp_mima_phase1_construct(void)
{
	(void)th02_s5_mima_clean_init(T2S5_MIMA_BOSS_START);
	mima_phase = 1;
	mima_pattern = (t2lbp_randring2_next8() % 3);
	mima_patterns_this_phase = 0;
	mima_phase_damage_max = 600;
	mima_patterns_until_vulnerable = 2;
	mima_patterns_max = 10;
	mima_pattern_count = 3;
	mima_bg_ring_radius = 50;
	mima_bg_circle_radius = 33;
	mima_bg_circle_radius_base = 33;
	mima_bg_circle_pulse_frame = 0;
	Palettes[0].c.r = 50;
	Palettes[0].c.g = 0;
	Palettes[0].c.b = 50;
	palette_show();
}

static void near t2lbp_mima_put(void)
{
	super_put_rect(
		boss_left_on_page[page_back], boss_top_on_page[page_back], patnum_2064E
	);
	super_put_rect(
		(boss_left_on_page[page_back] + 48), boss_top_on_page[page_back],
		(patnum_2064E + 1)
	);
	super_put_rect(
		(boss_left_on_page[page_back] + 96), boss_top_on_page[page_back],
		(patnum_2064E + 2)
	);
}

static void near t2lbp_mima_redraw_both(void)
{
	uint8_t ring_phase = mima_bg_ring_phase;
	int page;

	for(page = 0; page < PAGE_COUNT; page++) {
		page_back = page;
		page_front = (1 - page);
		graph_accesspage(page_back);
		mima_bg_ring_phase = ring_phase;
		mima_bg_render();
		t2lbp_mima_put();
	}
	mima_bg_ring_phase = ring_phase;
	t2lbp_pages_restore();
}

static void near t2lbp_mima_phase3_construct(void)
{
	(void)th02_s5_mima_clean_init(T2S5_MIMA_BOSS_START);
	mima_phase = 3;
	mima_pattern = (t2lbp_randring2_next8() % 3);
	boss_phase_frame = 0;
	mima_patterns_this_phase = 0;
	mima_phase_damage_max = 700;
	mima_patterns_until_vulnerable = 2;
	mima_patterns_max = 12;
	mima_pattern_count = 3;
	mima_bg_ring_radius = 75;
	mima_bg_circle_radius = 50;
	mima_bg_ring_phase = 0;
	mima_ring_radius = 0;
	mima_bg_circle_radius_base = 50;
	mima_bg_circle_pulse_frame = 0;
	Palettes[0].c.r = 1;
	Palettes[0].c.g = 50;
	Palettes[0].c.b = 50;
	palette_show();
}

static void near t2lbp_mima_phase5_construct(void)
{
	(void)th02_s5_mima_clean_init(T2S5_MIMA_BOSS_START);
	mima_phase = 5;
	mima_pattern = (t2lbp_randring2_next8() % 3);
	boss_phase_frame = 0;
	mima_patterns_this_phase = 0;
	mima_phase_damage_max = 800;
	mima_patterns_until_vulnerable = 2;
	mima_patterns_max = 12;
	mima_pattern_count = 3;
	mima_bg_ring_radius = 100;
	mima_bg_circle_radius = 70;
	mima_bg_ring_phase = 0;
	mima_ring_radius = 0;
	mima_bg_circle_radius_base = 70;
	mima_bg_circle_pulse_frame = 0;
	Palettes[0].c.r = 50;
	Palettes[0].c.g = 1;
	Palettes[0].c.b = 1;
	palette_show();
}

// The first regular state after Phase 6's completed charge. Keep the charge
// itself out of direct Practice, but retain each source-owned handoff field.
static void near t2lbp_mima_phase7_construct(void)
{
	(void)th02_s5_mima_clean_init(T2S5_MIMA_BOSS_START);
	mima_phase = 7;
	mima_pattern = (t2lbp_randring2_next8() & 7);
	boss_phase_frame = 0;
	mima_patterns_this_phase = 0;
	mima_phase_damage_max = 1500;
	mima_patterns_until_vulnerable = 3;
	mima_patterns_max = 200;
	mima_pattern_count = 9;
	mima_all_patterns = true;
	mima_bg_ring_radius = 125;
	mima_bg_circle_radius = 90;
	mima_bg_ring_col_head = 13;
	mima_bg_ring_col_tail = 2;
	mima_bg_circle_col = 3;
	mima_bg_ring_phase = 0;
	mima_ring_radius = 0;
	mima_bg_circle_radius_base = 90;
	mima_bg_circle_pulse_frame = 0;
	Palettes[0].c.r = 1;
	Palettes[0].c.g = 0;
	Palettes[0].c.b = 0;
	palette_show();
}

// The first ordinary state of Mima's winged form. mima_19C8D() reloads the
// second form immediately before mima_update() publishes this handoff. A
// direct Practice target has no preceding first-form position to inherit, so
// it starts at the source's first visible top row with the clean owner's
// canonical horizontal position and zero drift.
static void near t2lbp_mima_phase9_construct(void)
{
	(void)th02_s5_mima_clean_init(T2S5_MIMA_BOSS_START);
	super_clean(128, 192);
	super_patnum = 128;
	(void)super_entry_bfnt(mima2_bft);
	boss_top_on_page[0] = 64;
	boss_top_on_page[1] = 64;
	mima_velocity_y = 0;
	mima_phase = 9;
	mima_pattern = (t2lbp_randring2_next8() % 3);
	boss_phase_frame = 0;
	mima_patterns_this_phase = 0;
	mima_phase_damage_max = 1100;
	mima_patterns_until_vulnerable = 2;
	mima_patterns_max = 200;
	mima_pattern_count = 3;
	mima_all_patterns = false;
	mima_bg_ring_radius = 200;
	mima_bg_circle_radius = 150;
	mima_bg_ring_col_head = 13;
	mima_bg_ring_col_tail = 2;
	mima_bg_circle_col = 1;
	mima_bg_ring_phase = 0;
	mima_ring_radius = 0;
	mima_bg_circle_radius_base = 150;
	mima_bg_circle_pulse_frame = 0;
	Palettes[0].c.r = 1;
	Palettes[0].c.g = 0;
	Palettes[0].c.b = 0;
	palette_show();
}

static void near t2lbp_sigma_blasts_reset(void)
{
	int i;

	for(i = 0; i < 16; i++) {
		sigma_blasts[i].state = 0;
		sigma_blasts[i].state_frame = 0;
		sigma_blasts[i].center_x = 0;
		sigma_blasts[i].center_y = 0;
		sigma_blasts[i].radius_max = 0;
		sigma_blasts[i].patnum = 0;
		sigma_blasts[i].radius = 0;
	}
}

static void near t2lbp_sigma_phase1_construct(void)
{
	th02_s6_sigma_clean_init();
	sigma_phase = 1;
	sigma_pattern = 0;
	sigma_pattern_looped_unused = 0;
	boss_phase_frame = 0;
	sigma_pattern_func[0] = sigma_15D56;
	sigma_pattern_func[1] = sigma_15E84;
	sigma_pattern_func[2] = sigma_15F6F;
	sigma_phase_damage_max = 1800;
	t2lbp_sigma_blasts_reset();
	lasers_callbacks_set();
}

static void near t2lbp_sigma_put(void)
{
	super_put_rect(sigma_topleft.x, sigma_topleft.y, patnum_2064E);
	super_put_rect(
		(sigma_topleft.x + 48), sigma_topleft.y, (patnum_2064E + 1)
	);
}

static void near t2lbp_sigma_redraw_both(void)
{
	int page;

	for(page = 0; page < PAGE_COUNT; page++) {
		page_back = page;
		page_front = (1 - page);
		graph_accesspage(page_back);
		sigma_bg_render();
		t2lbp_sigma_put();
	}
	t2lbp_pages_restore();
}

static void near t2lbp_sigma_phase3_construct(void)
{
	th02_s6_sigma_clean_init();
	sigma_phase = 3;
	sigma_pattern = 0;
	sigma_pattern_looped_unused = 0;
	boss_phase_frame = 0;
	sigma_pattern_func[0] = sigma_15F95;
	sigma_pattern_func[1] = sigma_16176;
	sigma_pattern_func[2] = sigma_15F6F;
	sigma_phase_damage_max = 1800;
	sigma_cel_interval_mask = 7;
	sigma_blast_hitbox_margin = 4;
	sigma_ring_radius = 0;
	sigma_frame = 0;
	t2lbp_sigma_blasts_reset();
	lasers_callbacks_set();
}

static void near t2lbp_sigma_phase5_construct(void)
{
	th02_s6_sigma_clean_init();
	sigma_phase = 5;
	sigma_pattern = 0;
	sigma_pattern_looped_unused = 0;
	boss_phase_frame = 0;
	sigma_pattern_func[0] = sigma_1619C;
	sigma_pattern_func[1] = sigma_162D3;
	sigma_pattern_func[2] = sigma_15F6F;
	sigma_phase_damage_max = 1800;
	sigma_cel_interval_mask = 7;
	sigma_blast_hitbox_margin = 0;
	sigma_ring_radius = 0;
	sigma_frame = 0;
	t2lbp_sigma_blasts_reset();
	lasers_callbacks_set();
}

bool16 far th02_later_boss_clean_init(th02_later_boss_target_t target)
{
	switch(target) {
	case T2LBPT_MARISA_PHASE1:
		t2lbp_marisa_regular_construct(0, 0);
		t2lbp_marisa_redraw_both();
		return true;
	case T2LBPT_MIMA_PHASE1:
		t2lbp_mima_phase1_construct();
		t2lbp_mima_redraw_both();
		return true;
	case T2LBPT_SIGMA_PHASE1:
		t2lbp_sigma_phase1_construct();
		t2lbp_sigma_redraw_both();
		return true;
	case T2LBPT_MARISA_ROUND2:
		t2lbp_marisa_regular_construct(1, 0);
		t2lbp_marisa_redraw_both();
		return true;
	case T2LBPT_MIMA_PHASE3:
		t2lbp_mima_phase3_construct();
		t2lbp_mima_redraw_both();
		return true;
	case T2LBPT_SIGMA_PHASE3:
		t2lbp_sigma_phase3_construct();
		t2lbp_sigma_redraw_both();
		return true;
	case T2LBPT_MARISA_ROUND3:
		t2lbp_marisa_regular_construct(2, 1);
		t2lbp_marisa_redraw_both();
		return true;
	case T2LBPT_MARISA_ROUND4:
		t2lbp_marisa_regular_construct(3, 1);
		t2lbp_marisa_redraw_both();
		return true;
	case T2LBPT_MARISA_ROUND5:
		t2lbp_marisa_regular_construct(4, 1);
		t2lbp_marisa_redraw_both();
		return true;
	case T2LBPT_MARISA_ROUND6:
		t2lbp_marisa_regular_construct(5, 1);
		t2lbp_marisa_redraw_both();
		return true;
	case T2LBPT_MARISA_ROUND7:
		t2lbp_marisa_regular_construct(6, 1);
		t2lbp_marisa_redraw_both();
		return true;
	case T2LBPT_MIMA_PHASE5:
		t2lbp_mima_phase5_construct();
		t2lbp_mima_redraw_both();
		return true;
	case T2LBPT_SIGMA_PHASE5:
		t2lbp_sigma_phase5_construct();
		t2lbp_sigma_redraw_both();
		return true;
	case T2LBPT_MIMA_PHASE7:
		t2lbp_mima_phase7_construct();
		t2lbp_mima_redraw_both();
		return true;
	case T2LBPT_MIMA_PHASE9:
		t2lbp_mima_phase9_construct();
		t2lbp_mima_redraw_both();
		return true;
	default:
		return false;
	}
}
