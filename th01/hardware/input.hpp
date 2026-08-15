#include "libs/master.lib/master.hpp"
#include "platform.h"

/// Shared input macros
extern bool input_shot;
extern bool input_ok;

#define input_func_bool(var) { var = true; } else { var = false; }

// REIIDEN.EXE and FUUIN.EXE
// -------------------------

// Hey, at least two inputs merged into a single variable! It's a start.
enum input_lr_t {
	INPUT_NONE = 0,
	INPUT_RIGHT = 1,
	INPUT_LEFT = 2,
	INPUT_RIGHT_LEFT = 3,
	INPUT_LEFT_RIGHT = 3,

	_input_lr_t_FORCE_INT16 = 0x7FFF
};

extern bool input_up;
extern bool input_down;
extern uint8_t input_lr; // input_lr_t
extern bool input_strike;
extern bool input_mem_enter;
extern bool input_mem_leave;
extern bool paused;
extern bool input_bomb;

// Updates all input-related variables if the held state of their associated
// keys changed compared to the last input.
void input_sense(bool16 reset_repeat);

// Resets all input-related variables, then updates them according to the
// keyboard state.
void input_reset_sense(void);

// ORACLE-TH01 (mod branch only, -DT1CASE on the REIIDEN branch alone —
// Tupfile.lua:381). input_reset_sense() calls input_sense(true), whose early
// return happens BEFORE t1case_frame_io() and therefore consumes no case
// record. All eight call sites are consequently invisible to every one of the
// case's three cursors, and W3.1 step 4a measured two runs holding different
// reset counts at an identical cursor. A bare counter proved that; naming
// WHICH site fired needs the id, and the id cannot be recovered from inside
// input_sense() without reading a return address — which is a layout address
// and therefore a stop condition (TXSPLIT_CONTRACT.md 7).
//
// So the id is passed in at the call site. This is diagnostic only: it reaches
// T1DIAG.TXT and never the T1SPLIT row, which is byte-compared across
// lineages and must not grow.
#define T1RS_SITE_LIFE_LOOP     1 // th01/main_01.cpp, per life-loop iteration
#define T1RS_SITE_PAUSE_MENU    2 // th01/main/hud/menu.cpp, non-quit exit
#define T1RS_SITE_CONTINUE_MENU 3 // th01/main/hud/menu.cpp
#define T1RS_SITE_ROUTE_SELECT  4 // th01/main/boss/defeat.cpp, SinGyoku
#define T1RS_SITE_STAGE_BONUS   5 // th01/main/bonus.cpp, non-boss stage clear
#define T1RS_SITE_TOTLE         6 // th01/main/bonus.cpp, boss stage clear
#define T1RS_SITE_REGIST_NAME   7 // th01/hiscore/regist.cpp, name entry
#define T1RS_SITE_REGIST_MENU   8 // th01/hiscore/regist.cpp, after scoredat_load()
#define T1RS_SITES              9

#ifdef T1CASE
extern "C" void far t1case_reset_site(int site);

#define input_reset_sense_at(site) { \
	t1case_reset_site(site); \
	input_reset_sense(); \
}
#else
#define input_reset_sense_at(site) { \
	input_reset_sense(); \
}
#endif

// Resets just menu-related inputs.
inline void input_reset_menu_related(void) {
	input_lr = INPUT_NONE;
	input_shot = false;
	input_ok = false;
}

#define input_func_flag(var, flag) { var |= flag; } else { var &= ~flag; }

#define input_onchange(prev_slot, cur_sensed, if_pressed) \
	if(input_prev[prev_slot] != (cur_sensed)) { \
		if(cur_sensed) if_pressed \
	} \
	input_prev[prev_slot] = (cur_sensed);

#define input_onchange_bool(prev_slot, var, cur_sensed) \
	input_onchange(prev_slot, cur_sensed, input_func_bool(var))

#define input_onchange_flag(prev_slot, var, flag, cur_sensed) \
	input_onchange(prev_slot, cur_sensed, input_func_flag(var, flag))

#define input_onchange_2( \
	prev_slot_1, prev_slot_2, cur_sensed_1, cur_sensed_2, if_pressed \
) \
	if( \
		input_prev[prev_slot_1] != (cur_sensed_1) || \
		input_prev[prev_slot_2] != (cur_sensed_2) \
	) { \
		if(cur_sensed_1 || (cur_sensed_2)) if_pressed \
	} \
	input_prev[prev_slot_1] = (cur_sensed_1); \
	input_prev[prev_slot_2] = (cur_sensed_2);

#define input_onchange_bool_2( \
	prev_slot_1, prev_slot_2, var, cur_sensed_1, cur_sensed_2 \
) \
	input_onchange_2(prev_slot_1, prev_slot_2, \
		cur_sensed_1, cur_sensed_2, input_func_bool(var) \
	)

#define input_onchange_flag_2( \
	prev_slot_1, prev_slot_2, var, flag, cur_sensed_1, cur_sensed_2 \
) \
	input_onchange_2(prev_slot_1, prev_slot_2,  \
		cur_sensed_1, cur_sensed_2, input_func_flag(var, flag) \
	)

// ORACLE-TH01 (mod branch only): the replay injector's seam. Redirecting the
// call site rather than replacing master.lib's `_key_sense` symbol keeps
// th01_reiiden.asm — an original segment contribution — untouched, and lets
// ZUN's edge-detection logic, [input_prev] and the [input_bomb] double-tap
// derivation run completely unmodified. REIIDEN.EXE only; FUUIN keeps the
// stock routine.
#if defined(T1CASE) && (BINARY == 'M')
	int far t1case_key_sense(int keygroup);
	#define input_key_sense(group) t1case_key_sense(group)
#else
	#define input_key_sense(group) key_sense(group)
#endif

#define input_pause_ok_sense(prev_slot_esc, prev_slot_ok, group0, group3) \
	group0 = input_key_sense(0); \
	group3 = input_key_sense(3); \
	group0 |= input_key_sense(0); \
	group3 |= input_key_sense(3); \
	input_onchange(prev_slot_esc, (group0 & K0_ESC), { \
		paused = (1 - paused); \
	}) \
	input_onchange(prev_slot_ok, (group3 & K3_RETURN), { \
		if((paused == true) && (input_shot == true)) { \
			/**
			 * ZUN bloat: The fact that the Pause menu even writes to this \
			 * flag is completely disgusting. It doesn't even do anything \
			 * meaningful with it! \
			 */ \
			player_is_hit = true; \
		} \
		input_ok = true; \
	} else { \
		input_ok = false; \
	});
extern bool player_is_hit; // ZUN bloat: See above
// -------------------------
