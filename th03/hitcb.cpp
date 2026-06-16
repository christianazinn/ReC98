#pragma option -zCHITCIRC_TEXT -zPmain_01

#include "th03/common.h"
#include "th03/main/player/ch_shot.hpp"
#include "th03/main/player/exatt.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/player/stuff.hpp"

extern farfunc_t_near callback_205CE[PLAYER_COUNT];
extern farfunc_t_near bomb_func[PLAYER_COUNT];
extern farfunc_t_near farfp_20F20;
extern farfunc_t_near farfp_20F24;
extern farfunc_t_near farfp_20F28;

extern "C" void pascal near sub_A4A1(void);
extern "C" void pascal near sub_A4C3(int pid, int char_id);

extern "C" void pascal far rikako_1334D(uint16_t slot);
extern "C" void pascal far chiyuri_1219D(uint16_t slot);
extern "C" void pascal far yumemi_102C8(uint16_t slot);

extern "C" void pascal far exatt_add_rikako(
	subpixel_t center_x, subpixel_t center_y, pid_t pid
);
extern "C" void pascal far exatt_add_chiyuri(
	subpixel_t center_x, subpixel_t center_y, pid_t pid
);
void pascal far exatt_add_yumemi(
	subpixel_t center_x, subpixel_t center_y, pid_t pid
);

void far exatt_update_rikako(void);
void far exatt_render_rikako(void);
void far exatt_update_chiyuri(void);
void far exatt_render_chiyuri(void);
void far exatt_update_yumemi(void);
void far exatt_render_yumemi(void);

extern "C" void pascal far chargeshot_add_rikako(
	Subpixel center_x, Subpixel center_y
);
extern "C" void pascal far chargeshot_update_rikako(void);
uint8_t far chargeshot_hittest_rikako(void);
extern "C" void pascal far chargeshot_render_rikako(void);
extern "C" void pascal far chargeshot_add_chiyuri(
	Subpixel center_x, Subpixel center_y
);
extern "C" void pascal far chargeshot_update_chiyuri(void);
uint8_t far chargeshot_hittest_chiyuri(void);
extern "C" void pascal far chargeshot_render_chiyuri(void);
extern "C" void pascal far chargeshot_add_yumemi(
	Subpixel center_x, Subpixel center_y
);
extern "C" void pascal far chargeshot_update_yumemi(void);
uint8_t far chargeshot_hittest_yumemi(void);
extern "C" void pascal far chargeshot_render_yumemi(void);

extern "C" void pascal far gba_gauge_pattern_pellet_rikako(void);
extern "C" void pascal far gba_gauge_pattern_bullet_rikako(void);
extern "C" void pascal far gba_gauge_pattern_pellet_chiyuri(void);
extern "C" void pascal far gba_gauge_pattern_bullet_chiyuri(void);
extern "C" void pascal far gba_gauge_pattern_pellet_yumemi(void);
extern "C" void pascal far gba_gauge_pattern_bullet_yumemi(void);

extern "C" void pascal far gba_boss_update_rikako(void);
extern "C" void pascal far gba_boss_render_rikako(void);
extern "C" void pascal far gba_boss_update_chiyuri(void);
extern "C" void pascal far gba_boss_render_chiyuri(void);
extern "C" void pascal far gba_boss_update_yumemi(void);
extern "C" void pascal far gba_boss_render_yumemi(void);

extern "C" void pascal far sub_1501E(void);
extern "C" void pascal far rikako_bomb(void);
extern "C" void pascal far chiyuri_bomb(void);
extern "C" void pascal far yumemi_bomb(void);

extern "C" void pascal near hyper_rikako(void);
extern "C" void pascal near hyper_chiyuri(void);
extern "C" void pascal near hyper_yumemi(void);
extern "C" void pascal near hyper_standby(void);

extern "C" void pascal far sub_D092(void);
extern "C" void pascal far sub_D135(void);
extern "C" void pascal far sub_D05D(void);
extern "C" void pascal far sub_D2E8(void);
extern "C" void pascal far sub_D340(void);

extern "C" void pascal near set_callbacks_rikako(int pid)
{
	register int si = pid;

	sub_A4C3(si, 0x0D);
	if(si == 0) {
		exatt_funcs[0].add = exatt_add_rikako;
		exatt_funcs[0].update = exatt_update_rikako;
		exatt_funcs[0].render = exatt_render_rikako;
		players[0].chargeshot_add = chargeshot_add_rikako;
		chargeshot_update[0] = chargeshot_update_rikako;
		chargeshot_render[0] = chargeshot_render_rikako;
		chargeshot_hittest[0] = chargeshot_hittest_rikako;
		gba_gauge_pattern_pellet[0] = gba_gauge_pattern_pellet_rikako;
		gba_gauge_pattern_bullet[0] = gba_gauge_pattern_bullet_rikako;
		gba_boss_update[0] = gba_boss_update_rikako;
		gba_boss_render[0] = gba_boss_render_rikako;
		callback_205CE[0] = sub_1501E;
		bomb_func[0] = rikako_bomb;
		players[0].hyper_func = hyper_rikako;
		players[0].hyper = hyper_standby;
	} else {
		exatt_funcs[1].add = exatt_add_rikako;
		exatt_funcs[1].update = exatt_update_rikako;
		exatt_funcs[1].render = exatt_render_rikako;
		players[1].chargeshot_add = chargeshot_add_rikako;
		chargeshot_update[1] = chargeshot_update_rikako;
		chargeshot_render[1] = chargeshot_render_rikako;
		chargeshot_hittest[1] = chargeshot_hittest_rikako;
		gba_gauge_pattern_pellet[1] = gba_gauge_pattern_pellet_rikako;
		gba_gauge_pattern_bullet[1] = gba_gauge_pattern_bullet_rikako;
		gba_boss_update[1] = gba_boss_update_rikako;
		gba_boss_render[1] = gba_boss_render_rikako;
		callback_205CE[1] = sub_1501E;
		bomb_func[1] = rikako_bomb;
		players[1].hyper_func = hyper_rikako;
		players[1].hyper = hyper_standby;
		sub_A4A1();
	}
	farfp_20F20 = sub_D092;
	farfp_20F24 = sub_D135;
	farfp_20F28 = sub_D05D;
	rikako_1334D(si);
}

extern "C" void pascal near set_callbacks_chiyuri(int pid)
{
	register int si = pid;

	sub_A4C3(si, 0x0F);
	if(si == 0) {
		exatt_funcs[0].add = exatt_add_chiyuri;
		exatt_funcs[0].update = exatt_update_chiyuri;
		exatt_funcs[0].render = exatt_render_chiyuri;
		players[0].chargeshot_add = chargeshot_add_chiyuri;
		chargeshot_update[0] = chargeshot_update_chiyuri;
		chargeshot_render[0] = chargeshot_render_chiyuri;
		chargeshot_hittest[0] = chargeshot_hittest_chiyuri;
		gba_gauge_pattern_pellet[0] = gba_gauge_pattern_pellet_chiyuri;
		gba_gauge_pattern_bullet[0] = gba_gauge_pattern_bullet_chiyuri;
		gba_boss_update[0] = gba_boss_update_chiyuri;
		gba_boss_render[0] = gba_boss_render_chiyuri;
		callback_205CE[0] = sub_1501E;
		bomb_func[0] = chiyuri_bomb;
		players[0].hyper_func = hyper_chiyuri;
		players[0].hyper = hyper_standby;
	} else {
		exatt_funcs[1].add = exatt_add_chiyuri;
		exatt_funcs[1].update = exatt_update_chiyuri;
		exatt_funcs[1].render = exatt_render_chiyuri;
		players[1].chargeshot_add = chargeshot_add_chiyuri;
		chargeshot_update[1] = chargeshot_update_chiyuri;
		chargeshot_render[1] = chargeshot_render_chiyuri;
		chargeshot_hittest[1] = chargeshot_hittest_chiyuri;
		gba_gauge_pattern_pellet[1] = gba_gauge_pattern_pellet_chiyuri;
		gba_gauge_pattern_bullet[1] = gba_gauge_pattern_bullet_chiyuri;
		gba_boss_update[1] = gba_boss_update_chiyuri;
		gba_boss_render[1] = gba_boss_render_chiyuri;
		callback_205CE[1] = sub_1501E;
		bomb_func[1] = chiyuri_bomb;
		players[1].hyper_func = hyper_chiyuri;
		players[1].hyper = hyper_standby;
		sub_A4A1();
	}
	farfp_20F20 = sub_D2E8;
	farfp_20F24 = sub_D340;
	farfp_20F28 = sub_D05D;
	chiyuri_1219D(si);
}

extern "C" void pascal near set_callbacks_yumemi(int pid)
{
	register int si = pid;

	sub_A4C3(si, 0x11);
	if(si == 0) {
		exatt_funcs[0].add = exatt_add_yumemi;
		exatt_funcs[0].update = exatt_update_yumemi;
		exatt_funcs[0].render = exatt_render_yumemi;
		players[0].chargeshot_add = chargeshot_add_yumemi;
		chargeshot_update[0] = chargeshot_update_yumemi;
		chargeshot_render[0] = chargeshot_render_yumemi;
		chargeshot_hittest[0] = chargeshot_hittest_yumemi;
		gba_gauge_pattern_pellet[0] = gba_gauge_pattern_pellet_yumemi;
		gba_gauge_pattern_bullet[0] = gba_gauge_pattern_bullet_yumemi;
		gba_boss_update[0] = gba_boss_update_yumemi;
		gba_boss_render[0] = gba_boss_render_yumemi;
		callback_205CE[0] = sub_1501E;
		bomb_func[0] = yumemi_bomb;
		players[0].hyper_func = hyper_yumemi;
		players[0].hyper = hyper_standby;
	} else {
		exatt_funcs[1].add = exatt_add_yumemi;
		exatt_funcs[1].update = exatt_update_yumemi;
		exatt_funcs[1].render = exatt_render_yumemi;
		players[1].chargeshot_add = chargeshot_add_yumemi;
		chargeshot_update[1] = chargeshot_update_yumemi;
		chargeshot_render[1] = chargeshot_render_yumemi;
		chargeshot_hittest[1] = chargeshot_hittest_yumemi;
		gba_gauge_pattern_pellet[1] = gba_gauge_pattern_pellet_yumemi;
		gba_gauge_pattern_bullet[1] = gba_gauge_pattern_bullet_yumemi;
		gba_boss_update[1] = gba_boss_update_yumemi;
		gba_boss_render[1] = gba_boss_render_yumemi;
		callback_205CE[1] = sub_1501E;
		bomb_func[1] = yumemi_bomb;
		players[1].hyper_func = hyper_yumemi;
		players[1].hyper = hyper_standby;
		sub_A4A1();
	}
	farfp_20F20 = sub_D2E8;
	farfp_20F24 = sub_D340;
	farfp_20F28 = sub_D05D;
	yumemi_102C8(si);
}
