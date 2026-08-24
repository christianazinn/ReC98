#pragma option -zCPLAYER_M_TEXT -zPmain_01 -G-

#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "libs/sprite16/sprite16.h"
#include "th03/common.h"
#include "th03/hardware/palette.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/round.hpp"
#include "th03/main/sprite16.hpp"
#include "codegen.hpp"
#include "th03/math/polar.hpp"
#include "th03/math/vector.hpp"

struct cee0_rec_t {
	uint8_t type;
	uint8_t age;
	screen_x_t x;
	vram_y_t y;
	uint8_t radius;
	int8_t radius_delta;
	uint8_t angle;
	uint8_t unused_9;
};

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

static const int WARNING_FLASH_RED_FRAMES = 30;
enum {
	WF_NONE = 0,
	WF_PORTRAIT = 1,
	WF_FLASH_RED = 3,
	WF_FLASH_RED_END = (WF_FLASH_RED + WARNING_FLASH_RED_FRAMES),
};

extern "C" cee0_rec_t near byte_20EA6[];
extern "C" uint8_t near byte_20E98[];
extern "C" uint8_t near byte_20E9A[];
extern "C" uint8_t near byte_20F1E;
extern "C" d3f9_rec_t near byte_20F2C[];
extern "C" uint8_t near warning_flag[];
extern "C" uint8_t near angle_2142C;
extern "C" char asc_1DD5A[];
extern "C" uint16_t far randring_far_next16_raw(void);

extern farfunc_t_near farfp_20F24;
extern "C" void pascal far sub_A3A8(uint8_t pid);
extern "C" void pascal far SUB_A3A8(uint8_t pid);
extern "C" void pascal near SUB_CACB(uint8_t pid, int color);
extern "C" void pascal far TEXT_PUTSA(
	unsigned x, unsigned y, const char far *str, unsigned atrb
);
extern "C" void pascal far sub_D1E7(void);
extern "C" void pascal far sub_D3F9(void);

extern "C" void pascal far SUB_CB81(uint8_t pid)
{
	uint8_t frame[4];

#define cb81_pid_other frame[1]
#define cb81_left      (*reinterpret_cast<uint16_t *>(&frame[2]))
#define cb81_byte_20E98_store_AL() { \
	_asm { \
		mov	dl, byte ptr [bp+6]; \
		mov	dh, 0; \
		db	0x8B, 0xDA; \
		mov	byte ptr byte_20E98[bx], al; \
	} \
}
#define cb81_gba_active_store_AL() { \
	_asm { \
		mov	dl, byte ptr [bp+6]; \
		mov	dh, 0; \
		db	0x8B, 0xDA; \
		mov	gba_flag_active[bx], al; \
	} \
}
#define cb81_palette_other_r_store_AL() { \
	_asm { \
		mov	dl, byte ptr [bp-3]; \
		mov	dh, 0; \
	} \
	imul_reg_to_reg(_DX, _DX, sizeof(RGB8)); \
	_asm { \
		db	0x8B, 0xDA; \
		mov	byte ptr Palettes[bx], al; \
	} \
}
#define cb81_call_sub_A3A8() { \
	_asm { \
		push	word ptr [bp-3]; \
		nop; \
		push	cs; \
		call	near ptr SUB_A3A8; \
	} \
}
#define cb81_call_sub_CACB() { \
	_asm { \
		push	word ptr [bp+6]; \
		push	si; \
		call	near ptr SUB_CACB; \
	} \
}
#define cb81_text_putsa_clear() { \
	_asm { \
		push	word ptr [bp-2]; \
		push	si; \
		push	ds; \
		push	offset asc_1DD5A; \
		push	TX_WHITE; \
		call	far ptr TEXT_PUTSA; \
	} \
}

	_AL = 1;
	_AL -= pid;
	cb81_pid_other = _AL;
	if(warning_flag[pid] < WF_FLASH_RED) {
		goto gba_palette_ramp;
	}
	if(warning_flag[pid] == WF_FLASH_RED) {
		byte_20E98[pid] = 0;
	}
	if(warning_flag[pid] >= WF_FLASH_RED_END) {
		goto warning_end;
	}
	if(warning_flag[pid] < (WF_FLASH_RED + (WARNING_FLASH_RED_FRAMES / 2))) {
		_AL = byte_20E98[pid];
		_AL += 13;
	} else {
		_AL = byte_20E98[pid];
		_AL += -13;
	}
	cb81_byte_20E98_store_AL();
	warning_flag[pid]++;
	_SI = 1;
	if(warning_flag[pid] & WF_PORTRAIT) {
		if(gba_flag_next[pid] == GBAF_GAUGE_PELLET_INIT) {
			_SI = TX_CYAN;
			goto warning_portrait_reset;
		}
		if(gba_flag_next[pid] == GBAF_GAUGE_BULLET_INIT) {
			_SI = TX_MAGENTA;
			goto warning_portrait_reset;
		}
		_SI = TX_RED;
warning_portrait_reset:
		cb81_call_sub_A3A8();
		goto warning_put;
	}
	if(gba_flag_next[pid] == GBAF_GAUGE_PELLET_INIT) {
		goto palette_blue;
	}
	if(gba_flag_next[pid] == GBAF_GAUGE_BULLET_INIT) {
		Palettes[cb81_pid_other].c.r = 120;
palette_blue:
		Palettes[cb81_pid_other].c.b = 120;
		goto palette_done;
	}
	Palettes[cb81_pid_other].c.r = 120;
palette_done:
	palette_changed = true;

warning_put:
	cb81_call_sub_CACB();
	goto gba_palette_ramp;

warning_end:
	cb81_left = 4;
	if(cb81_pid_other == 1) {
		cb81_left += 0x28;
	}
	_DI = 0;
	_SI = 0x0B;
	goto clear_loop_test;
clear_loop:
	cb81_text_putsa_clear();
	_DI++;
	_SI++;
clear_loop_test:
	if(static_cast<uint16_t>(_DI) < 6) {
		goto clear_loop;
	}
	cb81_call_sub_A3A8();
	warning_flag[pid] = WF_NONE;
	_AL = gba_flag_next[pid];
	cb81_gba_active_store_AL();
	byte_20E98[pid] = 0;
	byte_20E9A[pid] = 0;

gba_palette_ramp:
	if(gba_flag_active[pid] == GBAF_NONE) {
		goto end;
	}
	if(byte_20E9A[pid] == 0) {
		_AL = byte_20E98[pid];
		_AL += 4;
		cb81_byte_20E98_store_AL();
		if(byte_20E98[pid] >= 144) {
			byte_20E9A[pid] = 1;
		}
		goto palette_ramp_color_set;
	}
	_AL = byte_20E98[pid];
	_AL += -4;
	cb81_byte_20E98_store_AL();
	if(byte_20E98[pid] <= 0) {
		byte_20E9A[pid] = 0;
	}

palette_ramp_color_set:
	_AL = byte_20E98[pid];
	cb81_palette_other_r_store_AL();
	palette_changed = true;

end:
#undef cb81_text_putsa_clear
#undef cb81_call_sub_CACB
#undef cb81_call_sub_A3A8
#undef cb81_palette_other_r_store_AL
#undef cb81_gba_active_store_AL
#undef cb81_byte_20E98_store_AL
#undef cb81_left
#undef cb81_pid_other
}

extern "C" void pascal far SUB_CDBD(
	subpixel_t x, subpixel_t y, uint16_t pid
)
{
	register cee0_rec_t near *slot;

	byte_20F1E++;
	if(byte_20F1E >= 12) {
		byte_20F1E = 0;
	}
	slot = &byte_20EA6[byte_20F1E];
	slot->type = 1;
	slot->age = 0;
	slot->x = playfield_fg_x_to_screen(x, pid);
	slot->y = ((y >> 4) + 16);
	slot->radius = 8;
	slot->radius_delta = 8;
}

extern "C" void pascal far SUB_CE0C(
	subpixel_t x, subpixel_t y, uint16_t pid
)
{
	register cee0_rec_t near *slot;

	byte_20F1E++;
	if(byte_20F1E >= 12) {
		byte_20F1E = 0;
	}
	slot = &byte_20EA6[byte_20F1E];
	slot->type = 1;
	slot->age = 0;
	slot->x = playfield_fg_x_to_screen(x, pid);
	slot->y = ((y >> 4) + 16);
	slot->radius = 0x88;
	slot->radius_delta = -8;
}

extern "C" void pascal far SUB_CE5B(
	subpixel_t x, subpixel_t y, uint16_t pid
)
{
	register cee0_rec_t near *slot;

	byte_20F1E++;
	if(byte_20F1E >= 12) {
		byte_20F1E = 0;
	}
	slot = &byte_20EA6[byte_20F1E];
	slot->type = 2;
	slot->age = 0;
	slot->x = playfield_fg_x_to_screen(x, pid);
	slot->y = ((y >> 4) + 16);
	slot->radius = 162;
	slot->radius_delta = -10;
	slot->angle = randring_far_next16_raw();
}

extern "C" void pascal far sub_CEB2(void)
{
	register cee0_rec_t near *slot;

	slot = byte_20EA6;
	_DX = 0;
	goto loop_test;
loop:
	if(slot->type != 0) {
		slot->age++;
		if(slot->age > 0x10) {
			slot->type = 0;
		}
		_AL = slot->radius_delta;
		slot->radius += _AL;
	}
	_DX++;
	slot++;
loop_test:
	if(static_cast<int>(_DX) < 12) {
		goto loop;
	}
}

extern "C" void pascal far sub_CEE0(void)
{
	uint8_t frame[0x14];
	register cee0_rec_t near *slot;
	register int j;

#define cee0_angle  frame[1]
#define cee0_y(i)   (*reinterpret_cast<vram_y_t *>(&frame[2 + ((i) * 2)]))
#define cee0_x(i)   (*reinterpret_cast<screen_x_t *>(&frame[10 + ((i) * 2)]))
#define cee0_i      (*reinterpret_cast<int *>(&frame[18]))

	slot = byte_20EA6;
	grcg_setcolor(GC_RMW, 13);
	cee0_i = 0;
	goto cee0_loop_test;
cee0_loop:
	if(slot->type == 0) {
		goto cee0_next;
	}
	if(slot->x < (RES_X / 2)) {
		grc_setclip(16, 8, 303, 191);
	} else {
		grc_setclip(336, 8, 623, 191);
	}
	if(slot->type == 1) {
		grcg_circle(slot->x, (slot->y >> 1), slot->radius);
		goto cee0_next;
	}
	if((cee0_i % 2) == 0) {
		_AL = 8;
	} else {
		_AL = -8;
	}
	_AL += slot->angle;
	slot->angle = _AL;
	j = 0;
	_AL = slot->angle;
	goto cee0_vertex_test;
cee0_vertex_loop:
	cee0_x(j) = polar(slot->x, slot->radius, CosTable8[cee0_angle]);
	cee0_y(j) = (polar(slot->y, slot->radius, SinTable8[cee0_angle]) / 2);
	j++;
	_AL = cee0_angle;
	_AL += 0x40;
cee0_vertex_test:
	cee0_angle = _AL;
	if(j < 4) {
		goto cee0_vertex_loop;
	}
	grcg_line(cee0_x(0), cee0_y(0), cee0_x(1), cee0_y(1));
	grcg_line(cee0_x(1), cee0_y(1), cee0_x(2), cee0_y(2));
	grcg_line(cee0_x(2), cee0_y(2), cee0_x(3), cee0_y(3));
	grcg_line(cee0_x(3), cee0_y(3), cee0_x(0), cee0_y(0));

cee0_next:
	cee0_i++;
	slot++;
cee0_loop_test:
	if(cee0_i < 12) {
		goto cee0_loop;
	}
	asm {
		db 	0x31, 0xC0; // XOR AX, AX
		out	0x7C, ax;
	}
	grc_setclip(0, 0, (RES_X - 1), (SPRITE16_RES_Y - 1));

#undef cee0_i
#undef cee0_x
#undef cee0_y
#undef cee0_angle
}

extern "C" void near sub_D031(void)
{
	_asm {
		mov 	ax, [di]
		mov 	bx, [di+2]
		sar 	ax, 4
		db 	089h
		db 	0C1h
		sar 	ax, 3
		db 	081h
		db 	0E3h
		db 	0E0h
		db 	0FFh
		shl 	bx, 1
		db 	001h
		db 	0D8h
		shr 	bx, 2
		db 	001h
		db 	0C3h
		and 	cl, 7
		mov 	ax, 11000000b
		ror 	ax, cl
		mov 	es:[bx], ax
	}
}

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
	asm { xor	si, si; } // _SI = 0;
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
	asm { xor	si, si; } // _SI = 0;
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
	asm { xor	si, si; } // _SI = 0;
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
	asm { xor	si, si; } // _SI = 0;
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
	asm { xor	ax, ax; } // _AX = 0;
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
	asm { xor	si, si; } // _SI = 0;
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
	asm { xor	si, si; } // _SI = 0;
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
		asm { xor	dx, dx; } // _DX = 0;
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
