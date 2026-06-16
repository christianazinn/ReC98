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
#include "th03/main/playfld.hpp"
#include "th03/snd/snd.h"

enum {
	WF_PORTRAIT = 1,
	WF_FLASH_WHITE = 2,
	WF_FLASH_RED = 3,
};

extern "C" uint8_t near warning_flag[];

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

#undef hud_static_gauge_levels_put_nop
