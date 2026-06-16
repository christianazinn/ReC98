#pragma option -zCPLAYER_M_TEXT -zPmain_01 -G-

#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th03/common.h"
#include "th03/main/round.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/math/vector.hpp"

struct d3f9_rec_t {
	Subpixel x;
	Subpixel y;
	Subpixel vx;
	Subpixel vy;
	uint8_t angle;
	uint8_t unused_9;
	sprite16_offset_t so;
	uint16_t unknown_C;
	uint16_t unused_E;
};

extern "C" d3f9_rec_t near byte_20F2C[];
extern "C" uint8_t near angle_2142C;

extern "C" void near sub_D031(void);
extern "C" void near sub_D0FA(void);
extern farfunc_t_near farfp_20F24;
extern "C" void pascal far sub_D3F9(void);

extern "C" void pascal far sub_D340(void)
{
	sprite16_put_size.w.v = (16 / 16);
	sprite16_put_size.h = 8;
	sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
	sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	grcg_setcolor(GC_RMW, 13);
	_ES = SEG_PLANE_B;
	angle_2142C = 0x40;
	_DI = FP_OFF(byte_20F2C);
	_SI = 0;
grcg_loop:
	_AX = (irand() & 1);
	_asm {
		add 	ax, [di+6]
		mov 	[di+6], ax
		mov 	[di+8], al
	}
	sub_D0FA();
	sub_D031();
	_DI += sizeof(d3f9_rec_t);
	_SI++;
	if(static_cast<uint16_t>(_SI) < 0x18) {
		goto grcg_loop;
	}
	grcg_off();
	egc_on();
sprite_loop:
	_AX = 0;
	if(static_cast<uint16_t>(_SI) < 0x28) {
		_AX = (irand() & 1);
	}
	_asm {
		add 	ax, [di+6]
		mov 	[di+6], ax
		mov 	[di+8], al
	}
	sub_D0FA();
	_asm {
		mov 	ax, [di]
		sar 	ax, 4
		push 	ax
		mov 	ax, [di+2]
		sar 	ax, 4
		push 	ax
		push 	word ptr [di+0Ah]
		call 	far ptr sprite16_put_noclip
	}
	_DI += sizeof(d3f9_rec_t);
	_SI++;
	if(static_cast<uint16_t>(_SI) < 0x32) {
		goto sprite_loop;
	}
	if(round_frame > 144) {
		farfp_20F24 = sub_D3F9;
	}
	egc_off();
}

extern "C" void pascal far sub_D3F9(void)
{
	int frame_mod_4096;
	char should_vector;

	frame_mod_4096 = (round_or_result_frame & 4095);
	if(frame_mod_4096 < 1024) {
		should_vector = false;
	} else if(frame_mod_4096 < 1280) {
		if((frame_mod_4096 & 7) == 0) {
			goto vector_inc;
		}
		goto vector_flag_ready;
	} else if(frame_mod_4096 < 2048) {
		should_vector = false;
	} else if(frame_mod_4096 < 2304) {
		if((frame_mod_4096 & 3) == 0) {
			goto vector_dec;
		}
		goto vector_flag_ready;
	} else if((frame_mod_4096 >= 3072) && (frame_mod_4096 < 4064)) {
		if((frame_mod_4096 & 127) >= 64) {
			goto vector_dec;
		}
vector_inc:
		angle_2142C++;
		goto vector_set_flag;
vector_dec:
		angle_2142C--;
vector_set_flag:
		should_vector = true;
	} else {
		should_vector = false;
	}

vector_flag_ready:
	_DI = FP_OFF(byte_20F2C);
	_SI = 0;
vector_loop:
		sub_D0FA();
		if(should_vector) {
			_asm {
				lea 	bx, [di+4]
				push 	ds
				push 	bx
				lea 	bx, [di+6]
				push 	ds
				push 	bx
				mov 	al, angle_2142C
				mov 	ah, 0
				push 	ax
				mov 	al, [di+8]
				push 	ax
				call 	far ptr vector2
			}
		}
		_DI += sizeof(d3f9_rec_t);
		_SI++;
	if(static_cast<uint16_t>(_SI) < 0x32) {
		goto vector_loop;
	}
	sprite16_put_size.w.v = (16 / 16);
	sprite16_put_size.h = 8;
	sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
	sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	grcg_setcolor(GC_RMW, 13);
	_ES = SEG_PLANE_B;
	_DI = FP_OFF(byte_20F2C);
	_SI = 0;
grcg_loop:
		sub_D031();
		_DI += sizeof(d3f9_rec_t);
		_SI++;
	if(static_cast<uint16_t>(_SI) < 0x18) {
		goto grcg_loop;
	}
	grcg_off();
	egc_on();
sprite_loop:
		_DX = 0;
		if(static_cast<uint16_t>(_SI) >= 0x28) {
			_DX = frame_mod_4096;
			_asm {
				db 	081h
				db 	0E2h
				db 	01Fh
				db 	000h
			}
			_DX >>= 3;
			asm { shl dx, 1; }
		}
		_asm {
			mov 	ax, [di]
			sar 	ax, 4
			push 	ax
			mov 	ax, [di+2]
			sar 	ax, 4
			push 	ax
			add 	dx, [di+0Ah]
			push 	dx
			call 	far ptr sprite16_put_noclip
		}
		_DI += sizeof(d3f9_rec_t);
		_SI++;
	if(static_cast<uint16_t>(_SI) < 0x32) {
		goto sprite_loop;
	}
	egc_off();
}
