#pragma option -zCPLAYER_M_TEXT -zPmain_01 -G-

#include "codegen.hpp"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/frmdelay.h"
#include "th03/common.h"
#include "th03/formats/mrs.hpp"
#include "th03/hardware/input.h"
#include "th03/main/hud/static.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/playfld.hpp"
#include "th03/snd/snd.h"

enum {
	WF_PORTRAIT = 1,
	WF_FLASH_WHITE = 2,
	WF_FLASH_RED = 3,
};

extern "C" uint8_t near warning_flag[];
extern "C" char gbWARNING_1[];
extern "C" char gbWARNING_2[];
extern "C" char gbWARNING_3[];
extern "C" char gpYOU_ARE_FORCED_TO_EVADE_FROM[];
extern "C" char gpGAUGE_ATTACK_LEVEL[];
extern "C" char gpBOSS_ATTACK_LEVEL[];
extern "C" char gpYOUR_LIFE_IS_IN_PERIL_BE_CAREFUL[];

extern "C" void near sub_C7A5(void);
extern "C" void pascal near SUB_CACB(uint8_t pid, int color);

#define hud_static_gauge_levels_put_nop(pid) { \
	__emit__(0x6A, pid); \
	_asm { nop; push cs; call near ptr hud_static_gauge_levels_put; } \
}

extern "C" void pascal far SUB_C9FE(uint8_t mrs_slot)
{
	if(warning_flag[mrs_slot] != WF_PORTRAIT) {
		return;
	}
	_SI = PLAYFIELD_LEFT;
	if(mrs_slot != 0) {
		_SI += PLAYFIELD_W_BORDERED;
	}
	mrs_put_8(_SI, PLAYFIELD_TOP, mrs_slot);
	warning_flag[mrs_slot] = WF_FLASH_WHITE;
}

extern "C" void pascal far SUB_CA3C(void)
{
	if((warning_flag[0] != WF_FLASH_WHITE) && (warning_flag[1] != WF_FLASH_WHITE)) {
		return;
	}
	snd_se_reset();
	snd_se_play(19);
	snd_se_update();
	hud_static_gauge_levels_put_nop(0);
	hud_static_gauge_levels_put_nop(1);
	if(warning_flag[0] == WF_FLASH_WHITE) {
		warning_flag[0] = WF_FLASH_RED;
		SUB_CACB(0, TX_WHITE);
	}
	if(warning_flag[1] == WF_FLASH_WHITE) {
		warning_flag[1] = WF_FLASH_RED;
		SUB_CACB(1, TX_WHITE);
	}
	_SI = 0;
	goto loop_test;
loop:
	input_reset_sense_key_held();
	if(input_sp & INPUT_CANCEL) {
		sub_C7A5();
	}
	frame_delay(1);
	_AX = _SI;
	_AX &= 1;
	imul_reg_to_reg(_AX, _AX, 50);
	_AX += 100;
	PaletteTone = _AX;
	palette_show();
	_SI++;
loop_test:
	if(static_cast<int16_t>(_SI) < 27) {
		goto loop;
	}
}

extern "C" void pascal near SUB_CACB(uint8_t pid, int color)
{
	register int left;
	register int atrb;

	atrb = color;
	left = 4;
	if(pid == 0) {
		left += 0x28;
	}
	gaiji_putsa(left, 11, gbWARNING_1, atrb);
	gaiji_putsa(left, 12, gbWARNING_2, atrb);
	gaiji_putsa(left, 13, gbWARNING_3, atrb);
	left += 4;
	gaiji_putsa(left, 14, gpYOU_ARE_FORCED_TO_EVADE_FROM, atrb);
	if(gba_flag_next[pid] == GBAF_BOSS) {
		goto boss_attack;
	}
	gaiji_putsa((left + 2), 15, gpGAUGE_ATTACK_LEVEL, atrb);
	gaiji_putca((left + 0x13), 15, (gba_gauge_level[pid] + 0x1F), TX_WHITE);
	goto end;

boss_attack:
	gaiji_putsa((left + 2), 15, gpBOSS_ATTACK_LEVEL, atrb);
	gaiji_putca((left + 0x13), 15, (gba_boss_level + 0x1F), TX_WHITE);

end:
	gaiji_putsa(left, 16, gpYOUR_LIFE_IS_IN_PERIL_BE_CAREFUL, atrb);
}

#undef hud_static_gauge_levels_put_nop
