#pragma option -zCPLAYER_M_TEXT -zPmain_01 -G-

#include "decomp.hpp"
#include "libs/master.lib/master.hpp"
#include "th03/main/hud/start.hpp"
#include "th03/math/polar.hpp"
#include "th03/math/randring.hpp"
#include "th03/resident.hpp"
#include "x86real.h"

struct hud_start_anim_t {
	int x;
	int y;
	int v;
	unsigned char angle;
	unsigned char active;
	signed char angle_delta_first;
	signed char angle_delta_second;
};

extern "C" hud_start_anim_t byte_207E4[];
extern "C" hud_start_anim_t near *word_20CE4;
extern "C" unsigned char byte_20CE6;
extern "C" int word_20CE8;
extern "C" int word_20CEA;
extern "C" int x_20CEC;

#define irand_mod3_to(slot) { \
	_AX = irand(); \
	_BX = 3; \
	asm { cwd; idiv bx; } \
	(slot) = _DL; \
}

extern "C" void near sub_BB12(void)
{
	register int i;
	register int x;
	int delta;
	signed char angle_delta;

	if(hud_start_flag == HSF_INIT) {
		word_20CE8 = 0;
		word_20CE4 = byte_207E4;
		x = 0xFEE0;
		delta = 84;
		i = 0;
		goto banner_init_test;

banner_init_loop:
		word_20CE4->x = x;
		word_20CE4->y = delta;
		word_20CE4->v = 8;
		word_20CE4->active = 1;
		i++;
		word_20CE4++;
		delta++;
		x += 8;

banner_init_test:
		if(i < 0x10) {
			goto banner_init_loop;
		}
		byte_20CE6 = 0;
		if(resident->game_mode == GM_STORY) {
			word_20CEA = 0x16;
		} else {
			word_20CEA = 0x1A;
		}
		hud_start_flag = HSF_ACTIVE;
		x_20CEC = 0;
	}

	if(byte_20CE6 == 0) {
		word_20CE4 = byte_207E4;
		i = 0;
		goto banner_slide_test;

banner_slide_loop:
		if(word_20CE4->active == 0) {
			goto banner_slide_next;
		}
		word_20CE4->x += word_20CE4->v;
		if(word_20CE4->active != 1) {
			goto banner_not_state_1;
		}
		asm {
			cmp 	word ptr [bx], 60h;
			jl  	short banner_slide_next;
			mov 	byte ptr [bx+7], 2;
			jmp 	short banner_slide_next;
		}

banner_not_state_1:
		if(word_20CE4->x > 0x40) {
			goto banner_decelerate;
		}
		asm {
			mov 	word ptr [bx], 40h;
			mov 	byte ptr [bx+7], 0;
			db  	00Bh, 0F6h; // or si, si
			jnz 	short banner_slide_next;
			mov 	byte_20CE6, 1;
			jmp 	short banner_slide_next;
		}

banner_decelerate:
		word_20CE4->v--;

banner_slide_next:
		i++;
		word_20CE4++;

banner_slide_test:
		if(i < 0x10) {
			goto banner_slide_loop;
		}
		optimization_barrier();
		__emit__(0xE9, 0xD2, 0x01);
	}

	if(byte_20CE6 == 1) {
		word_20CE8++;
		if(word_20CE8 > 0x56) {
			byte_20CE6++;
		}
		goto ret;
	}

	if(byte_20CE6 == 2) {
		word_20CE4 = byte_207E4;
		x = (96 << 4);
		i = 0;
		goto particle_left_test;

particle_left_loop:
		word_20CE4->x = x;
		word_20CE4->y = (randring1_next16_and(511) + (168 << 4));
		word_20CE4->angle = randring1_next16();
		irand_mod3_to(word_20CE4->angle_delta_first);
		irand_mod3_to(word_20CE4->angle_delta_second);
		word_20CE4->active = 1;
		i++;
		word_20CE4++;
		x += 0x20;

particle_left_test:
		if(i < 0x40) {
			goto particle_left_loop;
		}
		x = (416 << 4);
		i = 0;
		goto particle_right_test;

particle_right_loop:
		word_20CE4->x = x;
		word_20CE4->y = (randring1_next16_and(511) + (168 << 4));
		word_20CE4->angle = randring1_next16();
		irand_mod3_to(word_20CE4->angle_delta_first);
		irand_mod3_to(word_20CE4->angle_delta_second);
		word_20CE4->active = 1;
		i++;
		word_20CE4++;
		x += 0x20;

particle_right_test:
		if(i < 0x40) {
			goto particle_right_loop;
		}
		byte_20CE6++;
		word_20CE8 = 0;
		goto ret;
	}

	if(byte_20CE6 != 3) {
		goto done;
	}

	word_20CE8++;
	if(word_20CE8 >= 0x40) {
		byte_20CE6++;
	}
	word_20CE4 = byte_207E4;
	i = 0;
	goto particle_update_test;

particle_update_loop:
	if(word_20CE4->active == 0) {
		goto particle_update_next;
	}
	x = polar(0, 80, CosTable8[word_20CE4->angle]);
	delta = polar(0, 80, SinTable8[word_20CE4->angle]);
	word_20CE4->x += x;
	word_20CE4->y += delta;
	asm {
		cmp 	word ptr [bx], 0;
		jl  	short particle_deactivate;
		cmp 	word ptr [bx], (624 shl 4);
		jge 	short particle_deactivate;
		cmp 	word ptr [bx+2], 0;
		jl  	short particle_deactivate;
		cmp 	word ptr [bx+2], (384 shl 4);
		jl  	short particle_in_bounds;
	}

particle_deactivate:
	word_20CE4->active = 0;
	goto particle_update_next;

particle_in_bounds:
	if(word_20CE8 < 0x20) {
		angle_delta = word_20CE4->angle_delta_first;
	} else {
		angle_delta = word_20CE4->angle_delta_second;
	}
	angle_delta--;
	_AL = word_20CE4->angle;
	_DL = angle_delta;
	_DL <<= 2;
	_AL += _DL;
	word_20CE4->angle = _AL;
	goto particle_update_next;

particle_update_next:
	i++;
	word_20CE4++;

particle_update_test:
	if(i < 0x80) {
		goto particle_update_loop;
	}
	optimization_barrier();
	goto ret;

done:
	hud_start_flag = HSF_DONE;
ret:
	;
}
