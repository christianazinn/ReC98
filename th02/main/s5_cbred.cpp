/* TH02 Stage 5 Mima semantic callback and redraw checkpoint codecs. */

#pragma option -zCT2S5CBRD_TEXT -G-

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/resident.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/null.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/midboss/midboss.hpp"
#include "th02/main/player/bomb.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/stage/callback.hpp"
#include "th02/main/s5_cbred.hpp"

#define T2S5CB_CALLBACK_PROFILE_OFFSET 0
#define T2S5CB_LASER_INVALIDATE_OFFSET 0
#define T2S5CB_LASER_RENDER_OFFSET 1
#define T2S5CB_ENEMY_INVALIDATE_OFFSET 2
#define T2S5CB_ENEMY_RENDER_OFFSET 3
#define T2S5CB_COMMON_DISABLED 0
#define T2S5RD_RECIPE_OFFSET 0

typedef char th02_s5_mima_callback_wire_size_check[
	((T2S5CB_CALLBACK_PROFILE_OFFSET + 1) ==
	 TH02_S5_MIMA_CALLBACK_WIRE_SIZE) ? 1 : -1
];
typedef char th02_s5_mima_redraw_wire_size_check[
	((T2S5RD_RECIPE_OFFSET + 1) == TH02_S5_MIMA_REDRAW_WIRE_SIZE) ? 1 : -1
];

extern "C" bool16 (far *stage_loop_func)(void);
bool16 stage_loop(void);

extern "C" void far mima_init(void);
extern "C" void far mima_end(void);
extern "C" void far mima_bg_render(void);
extern "C" stage_progression_t far mima_update(void);
extern "C" bool16 far stage_should_end(void);
extern "C" void far nullfunc_void_2(void);
extern "C" const char mima1_bft[];
extern "C" const char mima2_bft[];
extern "C" const char aStage3_b_btt[];
extern "C" char aMima_m[];
extern "C" void far boss_bgm_load(char *fn);

extern "C" bool16 pascal near bomb_reimu_a(void);
extern "C" bool16 pascal near bomb_reimu_b(void);
extern "C" bool16 pascal near bomb_reimu_c(void);

static bool16 near t2s5cb_bomb_callback_valid(uint8_t shottype)
{
	switch(shottype) {
	case 0:
		return (playchar_bomb_func == bomb_reimu_a);
	case 1:
		return (playchar_bomb_func == bomb_reimu_b);
	case 2:
		return (playchar_bomb_func == bomb_reimu_c);
	default:
		return false;
	}
}

static bool16 near t2s5cb_native_table_valid(uint8_t shottype)
{
	return (
		(stage_loop_func == stage_loop) &&
		(boss_init == mima_init) && (boss_end == mima_end) &&
		(boss_bg_render_func == mima_bg_render) &&
		(boss_update_func == mima_update) &&
		(boss_bg_render == mima_bg_render) &&
		(boss_update == mima_update) &&
		(stage_should_end_func == stage_should_end) &&
		(boss_activate_if_scroll_done_func == nullfunc_void) &&
		(stage_title_unput_func == nullfunc_void) &&
		(stage_invalidate == nullfunc_void) &&
		(stage_update_and_render == nullfunc_void) &&
		(midboss_invalidate == nullfunc_false) &&
		(midboss_update_and_render == nullfunc_void) &&
		(lasers_invalidate_func == nullfunc_void) &&
		(lasers_update_and_render_func == nullfunc_void) &&
		(enemies_invalidate_func == nullfunc_void_2) &&
		(enemies_update_and_render_func == nullfunc_void_2) &&
		t2s5cb_bomb_callback_valid(shottype)
	);
}

bool16 far th02_s5_mima_callback_wire_valid(
	const uint8_t far *wire, uint16_t wire_size
)
{
	return (
		(wire != 0) && (wire_size == TH02_S5_MIMA_CALLBACK_WIRE_SIZE) &&
		(wire[T2S5CB_CALLBACK_PROFILE_OFFSET] <= T2RECP_EX_SIGMA)
	);
}

bool16 far th02_s5_mima_callback_wire_agree(
	uint8_t header_profile, const uint8_t far *callback_wire,
	const uint8_t far *laser_wire, const uint8_t far *enemy_wire
)
{
	return (
		(callback_wire != 0) && (laser_wire != 0) && (enemy_wire != 0) &&
		(header_profile == T2RECP_S5_MIMA) &&
		(callback_wire[T2S5CB_CALLBACK_PROFILE_OFFSET] == header_profile) &&
		(laser_wire[T2S5CB_LASER_INVALIDATE_OFFSET] ==
		 T2S5CB_COMMON_DISABLED) &&
		(laser_wire[T2S5CB_LASER_RENDER_OFFSET] == T2S5CB_COMMON_DISABLED) &&
		(enemy_wire[T2S5CB_ENEMY_INVALIDATE_OFFSET] ==
		 T2S5CB_COMMON_DISABLED) &&
		(enemy_wire[T2S5CB_ENEMY_RENDER_OFFSET] == T2S5CB_COMMON_DISABLED)
	);
}

bool16 far th02_s5_mima_callback_wire_capture(
	uint8_t far *wire, uint16_t wire_size, uint8_t header_profile,
	uint8_t shottype, const uint8_t far *laser_wire,
	const uint8_t far *enemy_wire
)
{
	if(
		(wire == 0) || (wire_size != TH02_S5_MIMA_CALLBACK_WIRE_SIZE) ||
		(header_profile != T2RECP_S5_MIMA) ||
		!t2s5cb_native_table_valid(shottype)
	) {
		return false;
	}
	wire[T2S5CB_CALLBACK_PROFILE_OFFSET] = header_profile;
	return (
		th02_s5_mima_callback_wire_valid(wire, wire_size) &&
		th02_s5_mima_callback_wire_agree(
			header_profile, wire, laser_wire, enemy_wire
		)
	);
}

bool16 far th02_s5_mima_redraw_wire_valid(
	const uint8_t far *wire, uint16_t wire_size
)
{
	return (
		(wire != 0) && (wire_size == TH02_S5_MIMA_REDRAW_WIRE_SIZE) &&
		(wire[T2S5RD_RECIPE_OFFSET] == T2RERR_NATIVE_ONE_FRAME_REVEAL)
	);
}

bool16 far th02_s5_mima_redraw_wire_capture(
	uint8_t far *wire, uint16_t wire_size
)
{
	if((wire == 0) || (wire_size != TH02_S5_MIMA_REDRAW_WIRE_SIZE)) {
		return false;
	}
	wire[T2S5RD_RECIPE_OFFSET] = T2RERR_NATIVE_ONE_FRAME_REVEAL;
	return th02_s5_mima_redraw_wire_valid(wire, wire_size);
}

#if T2REPLAY_EXACT_APPLY
#define T2S5CB_MIMA_PHASE_OFFSET 20

static int16_t near t2s5cb_wire_i16(
	const uint8_t far *wire, uint16_t offset
)
{
	return static_cast<int16_t>(
		static_cast<uint16_t>(wire[offset]) |
		(static_cast<uint16_t>(wire[offset + 1]) << 8)
	);
}

bool16 far th02_s5_mima_resources_prepare(
	const uint8_t far *actor_stage_wire
)
{
	int phase;
	int patterns;

	if(actor_stage_wire == 0) {
		return false;
	}
	phase = t2s5cb_wire_i16(actor_stage_wire, T2S5CB_MIMA_PHASE_OFFSET);
	super_clean(128, 192);
	super_patnum = 128;
	if(phase >= 7) {
		patterns = super_entry_bfnt(mima2_bft);
	} else {
		patterns = super_entry_bfnt(mima1_bft);
		if((patterns > 0) && (super_entry_bfnt(aStage3_b_btt) <= 0)) {
			patterns = 0;
		}
	}
	if(
		(patterns <= 0) || (super_patsize[128] == 0) ||
		(super_patsize[129] == 0) || (super_patsize[130] == 0)
	) {
		return false;
	}
	boss_bgm_load(aMima_m);
	return true;
}

static bool16 near t2s5cb_loaded_resources_valid(
	uint8_t shottype, const uint8_t far *actor_stage_wire
)
{
	return (
		(actor_stage_wire != 0) &&
		(super_patsize[128] != 0) && (super_patsize[129] != 0) &&
		(super_patsize[130] != 0) &&
		(stage_loop_func == stage_loop) &&
		(boss_init == mima_init) && (boss_end == mima_end) &&
		(boss_bg_render_func == mima_bg_render) &&
		(boss_update_func == mima_update) &&
		t2s5cb_bomb_callback_valid(shottype)
	);
}

bool16 far th02_s5_mima_callback_redraw_prepare(
	th02_s5_callback_redraw_plan_t *plan,
	const uint8_t far *callback_wire,
	const uint8_t far *redraw_wire,
	const uint8_t far *actor_stage_wire,
	const uint8_t far *laser_wire,
	const uint8_t far *enemy_wire,
	uint8_t header_profile,
	uint8_t shottype
)
{
	if(
		(plan == 0) ||
		!th02_s5_mima_callback_wire_valid(
			callback_wire, TH02_S5_MIMA_CALLBACK_WIRE_SIZE
		) ||
		!th02_s5_mima_redraw_wire_valid(
			redraw_wire, TH02_S5_MIMA_REDRAW_WIRE_SIZE
		) ||
		!th02_s5_mima_callback_wire_agree(
			header_profile, callback_wire, laser_wire, enemy_wire
		) ||
		!t2s5cb_loaded_resources_valid(shottype, actor_stage_wire)
	) {
		return false;
	}
	plan->callback_profile = callback_wire[T2S5CB_CALLBACK_PROFILE_OFFSET];
	plan->redraw_recipe = redraw_wire[T2S5RD_RECIPE_OFFSET];
	plan->sprite_form = static_cast<uint8_t>(
		t2s5cb_wire_i16(actor_stage_wire, T2S5CB_MIMA_PHASE_OFFSET) >= 7
	);
	return true;
}

void far th02_s5_mima_callback_commit_prepared(
	const th02_s5_callback_redraw_plan_t *plan
)
{
	(void)plan;
	stage_loop_func = stage_loop;
	boss_init = mima_init;
	boss_end = mima_end;
	boss_bg_render_func = mima_bg_render;
	boss_update_func = mima_update;
	boss_bg_render = mima_bg_render;
	boss_update = mima_update;
	stage_should_end_func = stage_should_end;
	boss_activate_if_scroll_done_func = nullfunc_void;
	stage_title_unput_func = nullfunc_void;
	stage_invalidate = nullfunc_void;
	stage_update_and_render = nullfunc_void;
	midboss_invalidate = nullfunc_false;
	midboss_update_and_render = nullfunc_void;
	lasers_invalidate_func = nullfunc_void;
	lasers_update_and_render_func = nullfunc_void;
	enemies_invalidate_func = nullfunc_void_2;
	enemies_update_and_render_func = nullfunc_void_2;
}

void far th02_s5_mima_redraw_commit_prepared(
	const th02_s5_callback_redraw_plan_t *plan,
	uint8_t captured_back_page
)
{
	int page;

	(void)plan;
	for(page = 0; page < PAGE_COUNT; page++) {
		graph_accesspage(page);
		graph_clear();
	}
	grcg_setcolor(GC_RMW, 11);
	grc_setclip(PLAYFIELD_RIGHT, 0, (RES_X - 1), (RES_Y - 1));
	graph_accesspage(0);
	grcg_fill();
	graph_accesspage(1);
	grcg_fill();
	grcg_off();
	grc_setclip(PLAYFIELD_LEFT, 0, PLAYFIELD_RIGHT, (RES_Y - 1));
	graph_accesspage(captured_back_page);
}
#endif
