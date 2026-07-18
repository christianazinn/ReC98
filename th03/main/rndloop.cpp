#pragma option -zCPLAYFLD_TEXT -zPmain_01

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "pc98.h"
#include "th02/hardware/pages.hpp"
#include "th02/math/randring.hpp"
#include "th03/hardware/palette.hpp"
#include "th03/main/bullet/bullet.hpp"
#include "th03/main/defeat.hpp"
#include "th03/main/difficul.hpp"
#include "th03/main/enemy/enemy.hpp"
#include "th03/main/enemy/fireball.hpp"
#include "th03/main/hitcirc.hpp"
#include "th03/main/player/bomb.hpp"
#include "th03/main/player/combo.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/exatt.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/replay_build.hpp"
#include "th03/main/replay.hpp"
#include "th03/main/round.hpp"
#include "th03/practice.hpp"
#include "th03/resident.hpp"
#include "th03/snd/snd.h"

extern player_stuff_t p1;
extern player_stuff_t p2;

extern farfunc_t_near callback_205CE[PLAYER_COUNT];
extern farfunc_t_near bomb_func[PLAYER_COUNT];
extern farfunc_t_near farfp_20F24;
extern nearfunc_t_near fp_1FBC0;
extern nearfunc_t_near fp_1E6EA;

extern uint16_t word_1E6E8;
extern uint8_t byte_1FBC3;
extern uint8_t byte_23AF9;
extern uint8_t byte_23AFA;
extern uint8_t byte_23B00;

extern "C" void near collmap_reset(void);
extern "C" void near playfield_rows_fill_288(void);
extern "C" void near sub_BB12(void);
extern "C" void near sub_BE5D(void);
extern "C" void near sub_C2F9(void);
extern "C" void near sub_C7A5(void);
extern "C" void near sub_C830(void);
extern "C" void near sub_C8C4(void);
extern "C" void near sub_D52E(void);
void near hitcircles_render(void);
void bullets_render(void);
extern "C" void pascal near player_update(
	input_t input, player_stuff_t near *player
);
extern "C" void pascal near sub_B4A8(void);
extern "C" void pascal near sub_E3F2(void);

extern "C" void pascal far SUB_CA3C(void);
extern "C" void pascal far SUB_CB81(uint8_t pid);
extern "C" void pascal far SUB_C9FE(uint8_t pid);
extern "C" void pascal far SUB_CEB2(void);
extern "C" void pascal far SUB_CEE0(void);

void pascal near resident_score_last_update(int pid);

#define nopcall_noarg(func) { \
	_asm { nop; push cs; call near ptr func; } \
}

#define nopcall_uint8(func, value) { \
	__emit__(0x6A, value); \
	_asm { nop; push cs; call near ptr func; } \
}

// Reclaim the inert NOP byte for the late replay overlay call.
#define overlay_balanced_call_noarg(func) { \
	_asm { push cs; call near ptr func; } \
}
#define overlay_balanced_call_uint8(func, value) { \
	__emit__(0x6A, value); \
	_asm { push cs; call near ptr func; } \
}

#define return_2() { \
	_asm { mov al, 2; pop bp; ret; } \
}

extern "C" uint8_t pascal near sub_9778(void)
{
	goto loop_test;

frame:
	collmap_reset();
	bullet_template_reset_stuff();
	pid_current = 0;
	pid.so_attack = SO_ATTACK_P1;
	exatt_funcs[0].update();
	gba_boss_update[0]();
	callback_205CE[0]();
	pid_current = 1;
	pid.so_attack = SO_ATTACK_P2;
	exatt_funcs[1].update();
	gba_boss_update[1]();
	callback_205CE[1]();
	hitcircles_update();
	shots_update();
	pid_current = 0;
	pid.so_attack = SO_ATTACK_P1;
	chargeshot_update[0]();
	pid_current = 1;
	pid.so_attack = SO_ATTACK_P2;
	chargeshot_update[1]();
	sub_BB12();
	enemy_formations_update();
	bullets_update();
	enemies_update();
	fireballs_update();
	pid_current = 0;
	pid.so_attack = SO_ATTACK_P1;
	gba_gauge_pattern_pellet[0]();
	gba_gauge_pattern_bullet[0]();
	pid_current = 1;
	pid.so_attack = SO_ATTACK_P2;
	gba_gauge_pattern_pellet[1]();
	gba_gauge_pattern_bullet[1]();
	overlay_balanced_call_noarg(SUB_CEB2);
	input_mode();
	replay_frame_io();
	pid.current = 0;
	player_update(input_mp_p1, &p1);
	pid.current = 1;
	player_update(input_mp_p2, &p2);
	snd_se_update();
	asm {
		call	far ptr replay_pause_request_poll
		jnc	pause_done
	}
	sub_C7A5();
pause_done:

	overlay_balanced_call_noarg(SUB_CA3C);
	nopcall_uint8(SUB_CB81, 0);
	nopcall_uint8(SUB_CB81, 1);
	farfp_20F24();
	if(byte_23AFA != 0) {
		if((round_or_result_frame & 1) == 0) {
			goto frame_counters;
		}
	}

	egc_on();
	pid_current = 0;
	pid.so_attack = SO_ATTACK_P1;
	bomb_func[0]();
	pid_current = 1;
	pid.so_attack = SO_ATTACK_P2;
	bomb_func[1]();
	pid_current = 0;
	pid.so_attack = SO_ATTACK_P1;
	gba_boss_render[0]();
	pid_current = 1;
	pid.so_attack = SO_ATTACK_P2;
	gba_boss_render[1]();
	shots_render();
	enemies_render();
	fireballs_hittest_and_render();
	hitcircles_render();
	pid_current = 0;
	pid.so_attack = SO_ATTACK_P1;
	exatt_funcs[0].render();
	chargeshot_render[0]();
	pid_current = 1;
	pid.so_attack = SO_ATTACK_P2;
	exatt_funcs[1].render();
	chargeshot_render[1]();
	pid.current = 0;
	player_render(&p1);
	pid.current = 1;
	player_render(&p2);
	egc_off();
	pid.current = 0;
	player_overlay_render(&p1);
	pid.current = 1;
	player_overlay_render(&p2);
	overlay_balanced_call_noarg(SUB_CEE0);
	bullets_render();
	if(defeat_flag == DF_BANNER) {
		sub_C2F9();
		if(
			(resident->game_mode == GM_STORY) ||
			practice_resident_uses_stock()
		) {
			goto story_mode;
		}
		if(p1.rounds_won >= 2) {
			goto round_end_result;
		}
		if(p2.rounds_won >= 2) {
			goto round_end_result;
		}
		if(round_or_result_frame == 160) {
			goto start_result_trapezoid;
		}
		_asm {
			cmp	byte ptr byte_1FBC3, 0;
			jz	after_defeat;
			jmp	return_1;
		}

round_end_result:
		sub_E3F2();
		if(round_or_result_frame == word_1E6E8) {
			goto start_result_trapezoid;
		}
		if(byte_1FBC3 == 0) {
			goto after_defeat;
		}
		resident_score_last_update(resident->pid_winner);
		return_2();

story_mode:
		if(resident->pid_winner == 0) {
			goto round_end_result;
		}
		if(round_or_result_frame != 160) {
			goto story_after_frame_160;
		}

start_result_trapezoid:
		fp_1FBC0 = sub_B4A8;
		goto after_defeat;

story_after_frame_160:
		if(byte_1FBC3 == 0) {
			goto after_defeat;
		}
		if(resident->story_lives != 0) {
			resident->story_lives--;
		} else {
			goto story_gameover;
		}

return_1:
		return 1;

story_gameover:
		resident_score_last_update(0);
		return_2();
	}

after_defeat:
	sub_C830();
	sub_C8C4();
	sub_D52E();
	overlay_balanced_call_uint8(SUB_C9FE, 0);
	overlay_balanced_call_uint8(SUB_C9FE, 1);
	sub_BE5D();
	combos_update_and_render();
	fp_1FBC0();
	fp_1E6EA();
	replay_overlay_put();
	if(byte_23AFA == 0) {
		while(byte_23AF9 > vsync_Count1) {
		}
		vsync_Count1 = 0;
		byte_23AF9 = 1;
	} else {
		if((round_or_result_frame & 1) == 0) {
			vsync_Count1 = 0;
			while(byte_23AF9 > vsync_Count1) {
			}
			byte_23AF9 = 2;
		}
	}
	if(palette_changed != false) {
		palette_show();
		palette_changed = false;
	}
	graph_accesspage(page_front);
	graph_showpage(page_back);
	page_front = _AL;
	page_back ^= 1;
	grcg_setcolor(GC_RMW, 0);
	_BX = ((183 * ROW_SIZE) + (16 / BYTE_DOTS));
	playfield_rows_fill_288();
	grcg_setcolor(GC_RMW, 1);
	_BX = ((183 * ROW_SIZE) + (336 / BYTE_DOTS));
	playfield_rows_fill_288();
	grcg_off();

frame_counters:
	round_frame++;
	round_or_result_frame++;
	_AL = static_cast<uint8_t>(round_frame);
	_AL &= 0x0F;
	round_frame_mod16 = _AL;
	_AL &= 7;
	round_frame_mod8 = _AL;
	_AL &= 3;
	round_frame_mod4 = _AL;
	_AL &= 1;
	round_frame_mod2 = _AL;
	if((round_or_result_frame & 63) == 0) {
		if(round_speed < ROUND_SPEED_MAX) {
			round_speed++;
		}
		if((round_or_result_frame & 1023) == 0) {
			randring_fill();
			if(p1.hit_damage_next < HIT_DAMAGE_MAX) {
				p1.hit_damage_next++;
			}
			if(p2.hit_damage_next < HIT_DAMAGE_MAX) {
				p2.hit_damage_next++;
			}
		}
		if(round_or_result_frame < 2000) {
			p1.cpu_frame = 0;
			p2.cpu_frame = 0;
		}
	}

loop_test:
	if(byte_23B00 == 0) {
		goto frame;
	}
	return 0;
}

#undef nopcall_uint8
#undef nopcall_noarg
#undef overlay_balanced_call_uint8
#undef overlay_balanced_call_noarg
#undef return_2
