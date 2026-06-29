#pragma codeseg mainl_03_TEXT group_01

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "shiftjis.hpp"
#include "th01/hardware/grppsafx.h"
#include "th02/v_colors.hpp"
#include "th02/score.h"
#include "x86real.h"

extern const shiftjis_t* VERDICT_PLAYCHARS[];
extern const shiftjis_t* VERDICT_RANKS[];
extern const shiftjis_t* VERDICT_NUMBERS[];
extern const shiftjis_t VERDICT_POINT[];

extern unsigned char continues_used;
extern unsigned char rank;
extern unsigned char score[];
extern unsigned char skill;
extern unsigned char staffroll_verdict_playchar;

void near staffroll_verdict_overlay_put(void)
{
	#define left       	_SI
	#define digit      	_DI

	int digits_left;
	int digit_seen;

	graph_putsa_fx(
		352, 174, (V_WHITE | FX_WEIGHT_BOLD),
		VERDICT_PLAYCHARS[staffroll_verdict_playchar]
	);
	graph_putsa_fx(360, 199, (V_WHITE | FX_WEIGHT_BOLD), VERDICT_RANKS[rank]);

	left = 408;
	digit_seen = false;
	digits_left = SCORE_DIGITS;
	while(digits_left > 0) {
		digit = score[digits_left];
		if(!digit_seen) {
			if(digit != 0) {
				left -= (digits_left * 8);
				digit_seen = true;
			}
		}
		if(digit_seen) {
			graph_putsa_fx(
				left, 224, (V_WHITE | FX_WEIGHT_BOLD), VERDICT_NUMBERS[digit]
			);
			left += 16;
		}
		digits_left--;
	}

	digit = continues_used;
	graph_putsa_fx(left, 224, (V_WHITE | FX_WEIGHT_BOLD), VERDICT_NUMBERS[digit]);
	graph_putsa_fx(408, 248, (V_WHITE | FX_WEIGHT_BOLD), VERDICT_NUMBERS[digit]);

	digit = (skill / 100);
	left = 408;
	digit_seen = false;
	if(digit != 0) {
		left -= 16;
		digit_seen = true;
		graph_putsa_fx(
			left, 291, (V_WHITE | FX_WEIGHT_BOLD), VERDICT_NUMBERS[digit]
		);
		left += 16;
	}

	digit = ((skill % 100) / 10);
	if(digit != 0) {
		if(!digit_seen) {
			digit_seen = true;
			left -= 8;
		}
	}
	if(digit_seen) {
		graph_putsa_fx(
			left, 291, (V_WHITE | FX_WEIGHT_BOLD), VERDICT_NUMBERS[digit]
		);
		left += 16;
	}

	digit = (skill % 10);
	graph_putsa_fx(left, 291, (V_WHITE | FX_WEIGHT_BOLD), VERDICT_NUMBERS[digit]);
	// TCC emits `mov ax, si; add ax, 16` for this argument otherwise.
	asm {
		lea 	ax, [si + 16];
		push	ax;
		db  	066h, 068h, 02Fh, 000h, 023h, 001h;
		push	ds;
		push	offset VERDICT_POINT;
		call	far ptr graph_putsa_fx;
	}

	#undef digit
	#undef left
}

#pragma codeseg
