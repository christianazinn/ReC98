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
extern farfunc_t_near farfp_20F24;
extern "C" void pascal far sub_D1E7(void);
extern "C" void pascal far sub_D3F9(void);

extern "C" void pascal far sub_D05D(void)
{
	angle_2142C = 0x40;
	_SI = FP_OFF(byte_20F2C);
	_DI = 0;
	goto d05d_loop_test;
d05d_loop:
	_asm {
		push 	ds
		lea 	ax, [si+4]
		push 	ax
		push 	ds
		lea 	ax, [si+6]
		push 	ax
		push 	64
		mov 	al, [si+8]
		mov 	ah, 0
		push 	ax
		call 	far ptr vector2
	}
	_DI++;
	_SI += sizeof(d3f9_rec_t);
d05d_loop_test:
	if(static_cast<int16_t>(_DI) < 0x50) {
		goto d05d_loop;
	}
}

extern "C" void pascal far sub_D092(void)
{
	angle_2142C = 0x40;
	_SI = FP_OFF(byte_20F2C);
	_DI = 0;
	goto d092_loop_test;
d092_loop:
	_AX = irand();
	_BX = (640 * 16);
	_asm {
		cwd
		idiv 	bx
		mov 	[si], dx
	}
	_AX = irand();
	_BX = (400 * 16);
	_asm {
		cwd
		idiv 	bx
		mov 	[si+2], dx
		mov 	word ptr [si+4], 0
		mov 	word ptr [si+6], 10h
		mov 	byte ptr [si+8], 20h
		mov 	word ptr [si+0Ah], 210Eh
	}
	_AX = irand();
	_asm {
		test 	al, 1
		jz   	d092_sprite_offset_set
		add 	word ptr [si+0Ah], 280h
d092_sprite_offset_set:
	}
	_AX = irand();
	_BX = 4000;
	_asm {
		cwd
		idiv 	bx
		mov 	[si+0Ch], dx
	}
	_DI++;
	_SI += sizeof(d3f9_rec_t);
d092_loop_test:
	if(static_cast<int16_t>(_DI) < 0x30) {
		goto d092_loop;
	}
}

extern "C" void near sub_D0FA(void)
{
	_asm {
		mov 	ax, [di]
		add 	ax, [di+4]
		cmp 	ax, 0080h
		jnb 	d0fa_x_above_min
		add 	ax, 2700h
		jmp 	d0fa_x_done
d0fa_x_above_min:
		cmp 	ax, 2780h
		jbe 	d0fa_x_done
		sub 	ax, 2700h
d0fa_x_done:
		mov 	[di], ax
		mov 	ax, [di+2]
		add 	ax, [di+6]
		cmp 	ax, 00E0h
		jnb 	d0fa_y_above_min
		add 	ax, 1720h
		jmp 	d0fa_y_done
d0fa_y_above_min:
		cmp 	ax, 1800h
		jbe 	d0fa_y_done
		sub 	ax, 1720h
d0fa_y_done:
		mov 	[di+2], ax
	}
}

extern "C" void pascal far sub_D135(void)
{
	sprite16_put_size.w.v = (16 / 16);
	sprite16_put_size.h = 3;
	sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
	sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	grcg_setcolor(GC_RMW, 10);
	_ES = SEG_PLANE_B;
	angle_2142C = 0x40;
	_DI = FP_OFF(byte_20F2C);
	_SI = 0;
d135_grcg_loop:
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
		goto d135_grcg_loop;
	}
	grcg_off();
	egc_on();
d135_sprite_loop:
	_AX = (irand() & 1);
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
		call 	far ptr sprite16_put
	}
	_DI += sizeof(d3f9_rec_t);
	_SI++;
	if(static_cast<uint16_t>(_SI) < 0x30) {
		goto d135_sprite_loop;
	}
	if(round_frame > 128) {
		farfp_20F24 = sub_D1E7;
	}
	egc_off();
}

extern "C" void pascal far sub_D1E7(void)
{
	int frame_mod_4096;
	char should_vector;

	frame_mod_4096 = (round_or_result_frame & 4095);
	if(frame_mod_4096 < 1024) {
		should_vector = false;
	} else if(frame_mod_4096 < 1280) {
		if((frame_mod_4096 & 7) == 0) {
			goto d1e7_vector_inc;
		}
		goto d1e7_vector_flag_ready;
	} else if(frame_mod_4096 < 2048) {
		should_vector = false;
	} else if(frame_mod_4096 < 2304) {
		if((frame_mod_4096 & 3) == 0) {
			goto d1e7_vector_dec;
		}
		goto d1e7_vector_flag_ready;
	} else if((frame_mod_4096 >= 3072) && (frame_mod_4096 < 4064)) {
		if((frame_mod_4096 & 127) >= 64) {
			goto d1e7_vector_dec;
		}
d1e7_vector_inc:
		angle_2142C++;
		goto d1e7_vector_set_flag;
d1e7_vector_dec:
		angle_2142C--;
d1e7_vector_set_flag:
		should_vector = true;
	} else {
		should_vector = false;
	}

d1e7_vector_flag_ready:
	_DI = FP_OFF(byte_20F2C);
	_SI = 0;
d1e7_vector_loop:
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
	if(static_cast<uint16_t>(_SI) < 0x30) {
		goto d1e7_vector_loop;
	}
	sprite16_put_size.w.v = (16 / 16);
	sprite16_put_size.h = 3;
	sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
	sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	grcg_setcolor(GC_RMW, 10);
	_ES = SEG_PLANE_B;
	_DI = FP_OFF(byte_20F2C);
	_SI = 0;
d1e7_grcg_loop:
		sub_D031();
		_DI += sizeof(d3f9_rec_t);
		_SI++;
	if(static_cast<uint16_t>(_SI) < 0x18) {
		goto d1e7_grcg_loop;
	}
	grcg_off();
	egc_on();
d1e7_sprite_loop:
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
	if(static_cast<uint16_t>(_SI) < 0x30) {
		goto d1e7_sprite_loop;
	}
	egc_off();
}

extern "C" void pascal far sub_D2E8(void)
{
	angle_2142C = 0x40;
	_SI = FP_OFF(byte_20F2C);
	_DI = 0;
	goto loop_test;
loop:
	_AX = irand();
	_BX = (640 * 16);
	_asm {
		cwd
		idiv 	bx
		mov 	[si], dx
	}
	_AX = irand();
	_BX = (400 * 16);
	_asm {
		cwd
		idiv 	bx
		mov 	[si+2], dx
		mov 	word ptr [si+4], 0
		mov 	word ptr [si+6], 10h
		mov 	byte ptr [si+8], 10h
	}
	if(static_cast<int16_t>(_DI) < 0x28) {
		_asm { mov word ptr [si+0Ah], 233Eh }
	} else {
		_asm { mov word ptr [si+0Ah], 20BEh }
	}
	_DI++;
	_SI += sizeof(d3f9_rec_t);
loop_test:
	if(static_cast<int16_t>(_DI) < 0x32) {
		goto loop;
	}
}

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
