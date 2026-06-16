#pragma option -zCPLAYER_M_TEXT -zPmain_01 -G-

#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th03/common.h"
#include "th03/main/round.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/main/v_colors.hpp"

extern player_stuff_t p1;
extern player_stuff_t p2;

extern "C" void pascal near SUB_B3A2(void);
extern "C" void pascal near SUB_B398(void);
extern "C" void pascal near SUB_B3F6(void);

#define grcg_off_inline() { \
	_asm { db 0x31, 0xC0; out 0x7C, ax; } \
}

extern "C" void near sub_C830(void)
{
	register int col;

	egc_on();
	SUB_B3A2();
	egc_off();
	if((p1.hyper_active != 0) && (round_or_result_frame & 1)) {
		_AX = p1.gauge_avail;
		_AX >>= 10;
		col = _AX;
		if(col == 1) {
			col = 8;
		} else if(col == 2) {
			col = 0x0C;
		} else {
			col = 6;
		}
		grcg_setcolor(GC_RMW, col);
		_BX = ((7 * ROW_SIZE) + (16 / BYTE_DOTS));
		SUB_B398();
		grcg_off_inline();
	}
	if((p2.hyper_active != 0) && (round_or_result_frame & 1)) {
		_AX = p2.gauge_avail;
		_AX >>= 10;
		col = _AX;
		if(col == 1) {
			col = 8;
		} else if(col == 2) {
			col = 0x0C;
		} else {
			col = 6;
		}
		grcg_setcolor(GC_RMW, col);
		_BX = ((7 * ROW_SIZE) + (336 / BYTE_DOTS));
		SUB_B398();
		grcg_off_inline();
	}
}

extern "C" void near sub_C8C4(void)
{
	register int avail_p1;
	register int avail_p2;
	uint8_t col;
	int charged_p1;
	int charged_p2;

	charged_p1 = (p1.gauge_charged >> 4);
	avail_p1 = (p1.gauge_avail >> 4);
	charged_p2 = (p2.gauge_charged >> 4);
	avail_p2 = (p2.gauge_avail >> 4);
	avail_p1 -= charged_p1;
	if(avail_p1 > 0) {
		if((p1.hyper_active == 0) || (round_or_result_frame & 1)) {
			grcg_setcolor(GC_RMW, 9);
		} else {
			grcg_setcolor(GC_RMW, V_WHITE);
		}
		_DX = (charged_p1 + 0x20);
		_BX = avail_p1;
		SUB_B3F6();
	}
	avail_p2 -= charged_p2;
	if(avail_p2 > 0) {
		if((p2.hyper_active == 0) || (round_or_result_frame & 1)) {
			grcg_setcolor(GC_RMW, 9);
		} else {
			grcg_setcolor(GC_RMW, V_WHITE);
		}
		_DX = (charged_p2 + (PLAYFIELD_W_BORDERED + 32));
		_BX = avail_p2;
		SUB_B3F6();
	}
	_CX = charged_p1;
	if(_CX != 0) {
		if(_CX < 64) {
			col = V_WHITE;
		} else if(_CX < 128) {
			col = 0x0A;
		} else if(_CX < 192) {
			col = 8;
		} else if(_CX < 255) {
			col = 6;
		} else {
			col = 0x0C;
		}
		grcg_setcolor(GC_RMW, col);
		_DX = 0x20;
		_BX = _CX;
		SUB_B3F6();
	}
	_CX = charged_p2;
	if(_CX != 0) {
		if(_CX < 64) {
			col = V_WHITE;
		} else if(_CX < 128) {
			col = 0x0A;
		} else if(_CX < 192) {
			col = 8;
		} else if(_CX < 255) {
			col = 6;
		} else {
			col = 0x0C;
		}
		grcg_setcolor(GC_RMW, col);
		_DX = (PLAYFIELD_W_BORDERED + 32);
		_BX = _CX;
		SUB_B3F6();
	}
	grcg_off_inline();
}

#undef grcg_off_inline
