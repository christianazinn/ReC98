/* TH02 Stage 5 Mima semantic callback and redraw checkpoint codecs. */

#pragma option -zCT2S5CBRD_TEXT -G-

#include "platform.h"
#include "th02/resident.hpp"
#include "th02/main/null.hpp"
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
