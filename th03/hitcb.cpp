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

extern "C" void pascal far reimu_10BFE(uint16_t slot);
extern "C" void pascal far mima_FB46(uint16_t slot);
extern "C" void pascal far marisa_F5AF(uint16_t slot);
extern "C" void pascal far ellen_113E2(uint16_t slot);
extern "C" void pascal far kotohime_11A6D(uint16_t slot);
extern "C" void pascal far kana_12BFB(uint16_t slot);
extern "C" void pascal far rikako_1334D(uint16_t slot);
extern "C" void pascal far chiyuri_1219D(uint16_t slot);
extern "C" void pascal far yumemi_102C8(uint16_t slot);

extern "C" void pascal far exatt_add_reimu(
	subpixel_t center_x, subpixel_t center_y, pid_t pid
);
extern "C" void pascal far exatt_add_mima(
	subpixel_t center_x, subpixel_t center_y, pid_t pid
);
extern "C" void pascal far exatt_add_marisa(
	subpixel_t center_x, subpixel_t center_y, pid_t pid
);
extern "C" void pascal far exatt_add_ellen(
	subpixel_t center_x, subpixel_t center_y, pid_t pid
);
extern "C" void pascal far exatt_add_kotohime(
	subpixel_t center_x, subpixel_t center_y, pid_t pid
);
extern "C" void pascal far exatt_add_kana(
	subpixel_t center_x, subpixel_t center_y, pid_t pid
);
extern "C" void pascal far exatt_add_rikako(
	subpixel_t center_x, subpixel_t center_y, pid_t pid
);
extern "C" void pascal far exatt_add_chiyuri(
	subpixel_t center_x, subpixel_t center_y, pid_t pid
);
void pascal far exatt_add_yumemi(
	subpixel_t center_x, subpixel_t center_y, pid_t pid
);

void far exatt_update_reimu(void);
void far exatt_render_reimu(void);
void far exatt_update_mima(void);
void far exatt_render_mima(void);
void far exatt_update_marisa(void);
void far exatt_render_marisa(void);
void far exatt_update_ellen(void);
void far exatt_render_ellen(void);
void far exatt_update_kotohime(void);
void far exatt_render_kotohime(void);
void far exatt_update_kana(void);
void far exatt_render_kana(void);
void far exatt_update_rikako(void);
void far exatt_render_rikako(void);
void far exatt_update_chiyuri(void);
void far exatt_render_chiyuri(void);
void far exatt_update_yumemi(void);
void far exatt_render_yumemi(void);

extern "C" void pascal far chargeshot_add_reimu(
	Subpixel center_x, Subpixel center_y
);
extern "C" void pascal far chargeshot_update_reimu(void);
uint8_t far chargeshot_hittest_reimu(void);
extern "C" void pascal far chargeshot_render_reimu(void);
extern "C" void pascal far chargeshot_add_mima(
	Subpixel center_x, Subpixel center_y
);
extern "C" void pascal far chargeshot_update_mima(void);
uint8_t far chargeshot_hittest_mima(void);
extern "C" void pascal far chargeshot_render_mima(void);
extern "C" void pascal far CHARGESHOT_ADD_MARISA(
	Subpixel center_x, Subpixel center_y
);
extern "C" void pascal far chargeshot_update_marisa(void);
uint8_t far chargeshot_hittest_marisa(void);
extern "C" void pascal far chargeshot_render_marisa(void);
extern "C" void pascal far chargeshot_add_ellen(
	Subpixel center_x, Subpixel center_y
);
extern "C" void pascal far chargeshot_update_ellen(void);
uint8_t far chargeshot_hittest_ellen(void);
extern "C" void pascal far chargeshot_render_ellen(void);
extern "C" void pascal far chargeshot_add_kotohime(
	Subpixel center_x, Subpixel center_y
);
extern "C" void pascal far chargeshot_update_kotohime(void);
uint8_t far chargeshot_hittest_kotohime(void);
extern "C" void pascal far chargeshot_render_kotohime(void);
extern "C" void pascal far chargeshot_add_kana(
	Subpixel center_x, Subpixel center_y
);
extern "C" void pascal far chargeshot_update_kana(void);
uint8_t far chargeshot_hittest_kana(void);
extern "C" void pascal far chargeshot_render_kana(void);
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

extern "C" void pascal far gba_gauge_pattern_pellet_reimu(void);
extern "C" void pascal far gba_gauge_pattern_bullet_reimu(void);
extern "C" void pascal far gba_gauge_pattern_pellet_mima(void);
extern "C" void pascal far gba_gauge_pattern_bullet_mima(void);
extern "C" void pascal far gba_gauge_pattern_pellet_marisa(void);
extern "C" void pascal far gba_gauge_pattern_bullet_marisa(void);
extern "C" void pascal far gba_gauge_pattern_pellet_ellen(void);
extern "C" void pascal far gba_gauge_pattern_bullet_ellen(void);
extern "C" void pascal far GBA_GAUGE_PATTERN_PELLET_KOTOHIM(void);
extern "C" void pascal far GBA_GAUGE_PATTERN_BULLET_KOTOHIM(void);
extern "C" void pascal far GBA_GAUGE_PATTERN_PELLET_KANA(void);
extern "C" void pascal far GBA_GAUGE_PATTERN_BULLET_KANA(void);
extern "C" void pascal far gba_gauge_pattern_pellet_rikako(void);
extern "C" void pascal far gba_gauge_pattern_bullet_rikako(void);
extern "C" void pascal far gba_gauge_pattern_pellet_chiyuri(void);
extern "C" void pascal far gba_gauge_pattern_bullet_chiyuri(void);
extern "C" void pascal far gba_gauge_pattern_pellet_yumemi(void);
extern "C" void pascal far gba_gauge_pattern_bullet_yumemi(void);

extern "C" void pascal far gba_boss_update_reimu(void);
extern "C" void pascal far gba_boss_render_reimu(void);
extern "C" void pascal far gba_boss_update_mima(void);
extern "C" void pascal far gba_boss_render_mima(void);
extern "C" void pascal far gba_boss_update_marisa(void);
extern "C" void pascal far gba_boss_render_marisa(void);
extern "C" void pascal far gba_boss_update_ellen(void);
extern "C" void pascal far gba_boss_render_ellen(void);
extern "C" void pascal far gba_boss_update_kotohime(void);
extern "C" void pascal far gba_boss_render_kotohime(void);
extern "C" void pascal far gba_boss_update_kana(void);
extern "C" void pascal far gba_boss_render_kana(void);
extern "C" void pascal far gba_boss_update_rikako(void);
extern "C" void pascal far gba_boss_render_rikako(void);
extern "C" void pascal far gba_boss_update_chiyuri(void);
extern "C" void pascal far gba_boss_render_chiyuri(void);
extern "C" void pascal far gba_boss_update_yumemi(void);
extern "C" void pascal far gba_boss_render_yumemi(void);

extern "C" void pascal far reimu_1508C(void);
extern "C" void pascal far sub_1501E(void);
extern "C" void pascal far mima_17043(void);
extern "C" void pascal far ellen_185AB(void);
extern "C" void pascal far reimu_bomb(void);
extern "C" void pascal far mima_bomb(void);
extern "C" void pascal far marisa_bomb(void);
extern "C" void pascal far ellen_bomb(void);
extern "C" void pascal far kotohime_bomb(void);
extern "C" void pascal far kana_bomb(void);
extern "C" void pascal far rikako_bomb(void);
extern "C" void pascal far chiyuri_bomb(void);
extern "C" void pascal far yumemi_bomb(void);

extern "C" void pascal near hyper_reimu(void);
extern "C" void pascal near hyper_mima(void);
extern "C" void pascal near hyper_marisa(void);
extern "C" void pascal near hyper_ellen(void);
extern "C" void pascal near hyper_kotohime(void);
extern "C" void pascal near hyper_kana(void);
extern "C" void pascal near hyper_rikako(void);
extern "C" void pascal near hyper_chiyuri(void);
extern "C" void pascal near hyper_yumemi(void);
extern "C" void pascal near hyper_standby(void);

extern "C" void pascal far sub_D092(void);
extern "C" void pascal far sub_D135(void);
extern "C" void pascal far sub_D05D(void);
extern "C" void pascal far sub_D2E8(void);
extern "C" void pascal far sub_D340(void);

extern "C" void pascal near set_callbacks_reimu(int pid)
{
	register int si = pid;

	sub_A4C3(si, 1);
	if(si == 0) {
		exatt_funcs[0].add = exatt_add_reimu;
		exatt_funcs[0].update = exatt_update_reimu;
		exatt_funcs[0].render = exatt_render_reimu;
		players[0].chargeshot_add = chargeshot_add_reimu;
		chargeshot_update[0] = chargeshot_update_reimu;
		chargeshot_render[0] = chargeshot_render_reimu;
		chargeshot_hittest[0] = chargeshot_hittest_reimu;
		gba_gauge_pattern_pellet[0] = gba_gauge_pattern_pellet_reimu;
		gba_gauge_pattern_bullet[0] = gba_gauge_pattern_bullet_reimu;
		gba_boss_update[0] = gba_boss_update_reimu;
		gba_boss_render[0] = gba_boss_render_reimu;
		callback_205CE[0] = reimu_1508C;
		bomb_func[0] = reimu_bomb;
		players[0].hyper_func = hyper_reimu;
		players[0].hyper = hyper_standby;
	} else {
		exatt_funcs[1].add = exatt_add_reimu;
		exatt_funcs[1].update = exatt_update_reimu;
		exatt_funcs[1].render = exatt_render_reimu;
		players[1].chargeshot_add = chargeshot_add_reimu;
		chargeshot_update[1] = chargeshot_update_reimu;
		chargeshot_render[1] = chargeshot_render_reimu;
		chargeshot_hittest[1] = chargeshot_hittest_reimu;
		gba_gauge_pattern_pellet[1] = gba_gauge_pattern_pellet_reimu;
		gba_gauge_pattern_bullet[1] = gba_gauge_pattern_bullet_reimu;
		gba_boss_update[1] = gba_boss_update_reimu;
		gba_boss_render[1] = gba_boss_render_reimu;
		callback_205CE[1] = reimu_1508C;
		bomb_func[1] = reimu_bomb;
		players[1].hyper_func = hyper_reimu;
		players[1].hyper = hyper_standby;
		sub_A4A1();
	}
	farfp_20F20 = sub_D092;
	farfp_20F24 = sub_D135;
	farfp_20F28 = sub_D05D;
	reimu_10BFE(si);
}

extern "C" void pascal near set_callbacks_mima(int pid)
{
	register int si = pid;

	sub_A4C3(si, 3);
	if(si == 0) {
		exatt_funcs[0].add = exatt_add_mima;
		exatt_funcs[0].update = exatt_update_mima;
		exatt_funcs[0].render = exatt_render_mima;
		players[0].chargeshot_add = chargeshot_add_mima;
		chargeshot_update[0] = chargeshot_update_mima;
		chargeshot_render[0] = chargeshot_render_mima;
		chargeshot_hittest[0] = chargeshot_hittest_mima;
		gba_gauge_pattern_pellet[0] = gba_gauge_pattern_pellet_mima;
		gba_gauge_pattern_bullet[0] = gba_gauge_pattern_bullet_mima;
		gba_boss_update[0] = gba_boss_update_mima;
		gba_boss_render[0] = gba_boss_render_mima;
		callback_205CE[0] = mima_17043;
		bomb_func[0] = mima_bomb;
		players[0].hyper_func = hyper_mima;
		players[0].hyper = hyper_standby;
	} else {
		exatt_funcs[1].add = exatt_add_mima;
		exatt_funcs[1].update = exatt_update_mima;
		exatt_funcs[1].render = exatt_render_mima;
		players[1].chargeshot_add = chargeshot_add_mima;
		chargeshot_update[1] = chargeshot_update_mima;
		chargeshot_render[1] = chargeshot_render_mima;
		chargeshot_hittest[1] = chargeshot_hittest_mima;
		gba_gauge_pattern_pellet[1] = gba_gauge_pattern_pellet_mima;
		gba_gauge_pattern_bullet[1] = gba_gauge_pattern_bullet_mima;
		gba_boss_update[1] = gba_boss_update_mima;
		gba_boss_render[1] = gba_boss_render_mima;
		callback_205CE[1] = mima_17043;
		bomb_func[1] = mima_bomb;
		players[1].hyper_func = hyper_mima;
		players[1].hyper = hyper_standby;
		sub_A4A1();
	}
	farfp_20F20 = sub_D092;
	farfp_20F24 = sub_D135;
	farfp_20F28 = sub_D05D;
	mima_FB46(si);
}

extern "C" void pascal near set_callbacks_marisa(int pid)
{
	register int si = pid;

	sub_A4C3(si, 5);
	if(si == 0) {
		exatt_funcs[0].add = exatt_add_marisa;
		exatt_funcs[0].update = exatt_update_marisa;
		exatt_funcs[0].render = exatt_render_marisa;
		players[0].chargeshot_add = CHARGESHOT_ADD_MARISA;
		chargeshot_update[0] = chargeshot_update_marisa;
		chargeshot_render[0] = chargeshot_render_marisa;
		chargeshot_hittest[0] = chargeshot_hittest_marisa;
		gba_gauge_pattern_pellet[0] = gba_gauge_pattern_pellet_marisa;
		gba_gauge_pattern_bullet[0] = gba_gauge_pattern_bullet_marisa;
		gba_boss_update[0] = gba_boss_update_marisa;
		gba_boss_render[0] = gba_boss_render_marisa;
		callback_205CE[0] = sub_1501E;
		bomb_func[0] = marisa_bomb;
		players[0].hyper_func = hyper_marisa;
		players[0].hyper = hyper_standby;
	} else {
		exatt_funcs[1].add = exatt_add_marisa;
		exatt_funcs[1].update = exatt_update_marisa;
		exatt_funcs[1].render = exatt_render_marisa;
		players[1].chargeshot_add = CHARGESHOT_ADD_MARISA;
		chargeshot_update[1] = chargeshot_update_marisa;
		chargeshot_render[1] = chargeshot_render_marisa;
		chargeshot_hittest[1] = chargeshot_hittest_marisa;
		gba_gauge_pattern_pellet[1] = gba_gauge_pattern_pellet_marisa;
		gba_gauge_pattern_bullet[1] = gba_gauge_pattern_bullet_marisa;
		gba_boss_update[1] = gba_boss_update_marisa;
		gba_boss_render[1] = gba_boss_render_marisa;
		callback_205CE[1] = sub_1501E;
		bomb_func[1] = marisa_bomb;
		players[1].hyper_func = hyper_marisa;
		players[1].hyper = hyper_standby;
		sub_A4A1();
	}
	farfp_20F20 = sub_D2E8;
	farfp_20F24 = sub_D340;
	farfp_20F28 = sub_D05D;
	marisa_F5AF(si);
}

extern "C" void pascal near set_callbacks_ellen(int pid)
{
	register int si = pid;

	sub_A4C3(si, 7);
	if(si == 0) {
		exatt_funcs[0].add = exatt_add_ellen;
		exatt_funcs[0].update = exatt_update_ellen;
		exatt_funcs[0].render = exatt_render_ellen;
		players[0].chargeshot_add = chargeshot_add_ellen;
		chargeshot_update[0] = chargeshot_update_ellen;
		chargeshot_render[0] = chargeshot_render_ellen;
		chargeshot_hittest[0] = chargeshot_hittest_ellen;
		gba_gauge_pattern_pellet[0] = gba_gauge_pattern_pellet_ellen;
		gba_gauge_pattern_bullet[0] = gba_gauge_pattern_bullet_ellen;
		gba_boss_update[0] = gba_boss_update_ellen;
		gba_boss_render[0] = gba_boss_render_ellen;
		callback_205CE[0] = ellen_185AB;
		bomb_func[0] = ellen_bomb;
		players[0].hyper_func = hyper_ellen;
		players[0].hyper = hyper_standby;
	} else {
		exatt_funcs[1].add = exatt_add_ellen;
		exatt_funcs[1].update = exatt_update_ellen;
		exatt_funcs[1].render = exatt_render_ellen;
		players[1].chargeshot_add = chargeshot_add_ellen;
		chargeshot_update[1] = chargeshot_update_ellen;
		chargeshot_render[1] = chargeshot_render_ellen;
		chargeshot_hittest[1] = chargeshot_hittest_ellen;
		gba_gauge_pattern_pellet[1] = gba_gauge_pattern_pellet_ellen;
		gba_gauge_pattern_bullet[1] = gba_gauge_pattern_bullet_ellen;
		gba_boss_update[1] = gba_boss_update_ellen;
		gba_boss_render[1] = gba_boss_render_ellen;
		callback_205CE[1] = ellen_185AB;
		bomb_func[1] = ellen_bomb;
		players[1].hyper_func = hyper_ellen;
		players[1].hyper = hyper_standby;
		sub_A4A1();
	}
	farfp_20F20 = sub_D2E8;
	farfp_20F24 = sub_D340;
	farfp_20F28 = sub_D05D;
	ellen_113E2(si);
}

extern "C" void pascal near set_callbacks_kotohime(int pid)
{
	register int si = pid;

	sub_A4C3(si, 9);
	if(si == 0) {
		exatt_funcs[0].add = exatt_add_kotohime;
		exatt_funcs[0].update = exatt_update_kotohime;
		exatt_funcs[0].render = exatt_render_kotohime;
		players[0].chargeshot_add = chargeshot_add_kotohime;
		chargeshot_update[0] = chargeshot_update_kotohime;
		chargeshot_render[0] = chargeshot_render_kotohime;
		chargeshot_hittest[0] = chargeshot_hittest_kotohime;
		gba_gauge_pattern_pellet[0] = GBA_GAUGE_PATTERN_PELLET_KOTOHIM;
		gba_gauge_pattern_bullet[0] = GBA_GAUGE_PATTERN_BULLET_KOTOHIM;
		gba_boss_update[0] = gba_boss_update_kotohime;
		gba_boss_render[0] = gba_boss_render_kotohime;
		callback_205CE[0] = sub_1501E;
		bomb_func[0] = kotohime_bomb;
		players[0].hyper_func = hyper_kotohime;
		players[0].hyper = hyper_standby;
	} else {
		exatt_funcs[1].add = exatt_add_kotohime;
		exatt_funcs[1].update = exatt_update_kotohime;
		exatt_funcs[1].render = exatt_render_kotohime;
		players[1].chargeshot_add = chargeshot_add_kotohime;
		chargeshot_update[1] = chargeshot_update_kotohime;
		chargeshot_render[1] = chargeshot_render_kotohime;
		chargeshot_hittest[1] = chargeshot_hittest_kotohime;
		gba_gauge_pattern_pellet[1] = GBA_GAUGE_PATTERN_PELLET_KOTOHIM;
		gba_gauge_pattern_bullet[1] = GBA_GAUGE_PATTERN_BULLET_KOTOHIM;
		gba_boss_update[1] = gba_boss_update_kotohime;
		gba_boss_render[1] = gba_boss_render_kotohime;
		callback_205CE[1] = sub_1501E;
		bomb_func[1] = kotohime_bomb;
		players[1].hyper_func = hyper_kotohime;
		players[1].hyper = hyper_standby;
		sub_A4A1();
	}
	farfp_20F20 = sub_D2E8;
	farfp_20F24 = sub_D340;
	farfp_20F28 = sub_D05D;
	kotohime_11A6D(si);
}

extern "C" void pascal near set_callbacks_kana(int pid)
{
	register int si = pid;

	sub_A4C3(si, 0x0B);
	if(si == 0) {
		exatt_funcs[0].add = exatt_add_kana;
		exatt_funcs[0].update = exatt_update_kana;
		exatt_funcs[0].render = exatt_render_kana;
		players[0].chargeshot_add = chargeshot_add_kana;
		chargeshot_update[0] = chargeshot_update_kana;
		chargeshot_render[0] = chargeshot_render_kana;
		chargeshot_hittest[0] = chargeshot_hittest_kana;
		gba_gauge_pattern_pellet[0] = GBA_GAUGE_PATTERN_PELLET_KANA;
		gba_gauge_pattern_bullet[0] = GBA_GAUGE_PATTERN_BULLET_KANA;
		gba_boss_update[0] = gba_boss_update_kana;
		gba_boss_render[0] = gba_boss_render_kana;
		callback_205CE[0] = sub_1501E;
		bomb_func[0] = kana_bomb;
		players[0].hyper_func = hyper_kana;
		players[0].hyper = hyper_standby;
	} else {
		exatt_funcs[1].add = exatt_add_kana;
		exatt_funcs[1].update = exatt_update_kana;
		exatt_funcs[1].render = exatt_render_kana;
		players[1].chargeshot_add = chargeshot_add_kana;
		chargeshot_update[1] = chargeshot_update_kana;
		chargeshot_render[1] = chargeshot_render_kana;
		chargeshot_hittest[1] = chargeshot_hittest_kana;
		gba_gauge_pattern_pellet[1] = GBA_GAUGE_PATTERN_PELLET_KANA;
		gba_gauge_pattern_bullet[1] = GBA_GAUGE_PATTERN_BULLET_KANA;
		gba_boss_update[1] = gba_boss_update_kana;
		gba_boss_render[1] = gba_boss_render_kana;
		callback_205CE[1] = sub_1501E;
		bomb_func[1] = kana_bomb;
		players[1].hyper_func = hyper_kana;
		players[1].hyper = hyper_standby;
		sub_A4A1();
	}
	farfp_20F20 = sub_D2E8;
	farfp_20F24 = sub_D340;
	farfp_20F28 = sub_D05D;
	kana_12BFB(si);
}

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
