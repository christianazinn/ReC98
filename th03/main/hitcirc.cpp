#include "libs/master.lib/pc98_gfx.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th02/main/execl.hpp"
#include "th03/core/initexit.h"
#include "th03/formats/pi.hpp"
#include "th03/math/randring.hpp"
#include "th03/main/round.hpp"
#include "th03/main/hitbox.hpp"
#include "th03/main/hitcirc.hpp"
#include "th03/main/v_colors.hpp"
#include "th03/sprites/main_s16.hpp"
#include <process.h>
#include <stddef.h>

extern nearfunc_t_near fp_1FBC0;
extern "C" int trapezoid_hmask;
extern "C" uint8_t byte_1FBC2;
extern "C" uint8_t byte_1FBC3;
extern "C" int word_1FBC4;
extern "C" int word_1FBC6;
extern "C" int word_1FBC8;
extern "C" int word_1FBCA;
extern "C" int word_1FBCC;
extern "C" int word_1FBCE;
extern "C" int word_1FBD0;
extern "C" int word_1FBD2;
extern "C" uint16_t wordmask_1DB0C[];

extern "C" void near playfield_rows_fill_288(void);
extern "C" void pascal near sub_B4A3(void);

#pragma option -k-
extern "C" void far sub_B39E(void)
{
	playfield_rows_fill_288();
}
#pragma option -k.

#pragma option -k-
extern "C" void pascal near sub_B3A2(void)
{
	__emit__(0x57);                   // push di
	__emit__(0x56);                   // push si
	__emit__(0xB8, 0xF0, 0xFF);       // mov ax, 0FFF0h
	__emit__(0xBA, 0xA0, 0x04);       // mov dx, EGC_ACTIVEPLANEREG
	__emit__(0xEF);                   // out dx, ax
	__emit__(0xB8, 0xFF, 0x00);       // mov ax, 00FFh
	__emit__(0xBA, 0xA2, 0x04);       // mov dx, EGC_READPLANEREG
	__emit__(0xEF);                   // out dx, ax
	__emit__(0xB8, 0x00, 0x31);       // mov ax, EGC copy mode
	__emit__(0xBA, 0xA4, 0x04);       // mov dx, EGC_MODE_ROP_REG
	__emit__(0xEF);                   // out dx, ax
	__emit__(0xB8, 0xFF, 0xFF);       // mov ax, 0FFFFh
	__emit__(0xBA, 0xA8, 0x04);       // mov dx, EGC_MASKREG
	__emit__(0xEF);                   // out dx, ax
	__emit__(0xB8, 0x00, 0x00);       // mov ax, 0
	__emit__(0xBA, 0xAC, 0x04);       // mov dx, EGC_ADDRRESSREG
	__emit__(0xEF);                   // out dx, ax
	__emit__(0xB8, 0x0F, 0x00);       // mov ax, 0Fh
	__emit__(0xBA, 0xAE, 0x04);       // mov dx, EGC_BITLENGTHREG
	__emit__(0xEF);                   // out dx, ax
	__emit__(0x8C, 0xDA);             // mov dx, ds
	__emit__(0xBF, 0x32, 0x02);       // mov di, ((7 * ROW_SIZE) + (16 / BYTE_DOTS))
	__emit__(0xBE, 0xD8, 0x40);       // mov si, ((207 * ROW_SIZE) + (320 / BYTE_DOTS))
	__emit__(0xB8, 0xC0, 0xAB);       // mov ax, 0ABC0h
	__emit__(0x8E, 0xC0);             // mov es, ax
	__emit__(0x8E, 0xD8);             // mov ds, ax
	__emit__(0x90);                   // nop
	__emit__(0xB9, 0x12, 0x00);       // mov cx, 12h
	__emit__(0xA5);                   // movsw
	__emit__(0x26, 0x89, 0x45, 0x26); // mov es:[di+26h], ax
	__emit__(0xE2, 0xF9);             // loop -7
	__emit__(0x83, 0xEE, 0x74);       // sub si, 74h
	__emit__(0x83, 0xEF, 0x74);       // sub di, 74h
	__emit__(0x7D, 0xEE);             // jge -18
	__emit__(0x8E, 0xDA);             // mov ds, dx
	__emit__(0x5E);                   // pop si
	__emit__(0x5F);                   // pop di
}
#pragma option -k.

#pragma codestring "\x90"

#pragma option -k-
extern "C" void pascal near sub_B3F6(void)
{
	__emit__(0x57);             // push di
	__emit__(0x56);             // push si
	__emit__(0xBF, 0xF0, 0x00); // mov di, (3 * ROW_SIZE)
	__emit__(0xB8, 0xCA, 0xAB); // mov ax, 0ABCAh
	__emit__(0x8E, 0xC0);       // mov es, ax
	__emit__(0xBE, 0x50, 0x00); // mov si, 50h
	__emit__(0x8B, 0xC2);       // mov ax, dx
	__emit__(0x25, 0x0F, 0x00); // and ax, 0Fh
	__emit__(0x03, 0xD8);       // add bx, ax
	__emit__(0xC1, 0xEA, 0x04); // shr dx, 4
	__emit__(0xD1, 0xE2);       // shl dx, 1
	__emit__(0x03, 0xFA);       // add di, dx
	__emit__(0x8B, 0xC3);       // mov ax, bx
	__emit__(0x83, 0xE3, 0x0F); // and bx, 0Fh
	__emit__(0xD1, 0xE3);       // shl bx, 1
	__emit__(0xC1, 0xE8, 0x04); // shr ax, 4
	asm { jz short gauge_bar_partial_word; }
	__emit__(0x8B, 0xD0);       // mov dx, ax
	__emit__(0xD1, 0xE0);       // shl ax, 1
	__emit__(0x03, 0xF0);       // add si, ax
	__emit__(0x33, 0xC0);       // xor ax, ax
	__emit__(0xF7, 0xD0);       // not ax
	__emit__(0x90);             // nop
	__emit__(0x8B, 0xCA);       // mov cx, dx
	__emit__(0xF3, 0xAB);       // rep stosw
	__emit__(0x2B, 0xFE);       // sub di, si
	__emit__(0x8B, 0xCA);       // mov cx, dx
	__emit__(0xF3, 0xAB);       // rep stosw
	__emit__(0x2B, 0xFE);       // sub di, si
	__emit__(0x8B, 0xCA);       // mov cx, dx
	__emit__(0xF3, 0xAB);       // rep stosw
	__emit__(0x2B, 0xFE);       // sub di, si
	__emit__(0x8B, 0xCA);       // mov cx, dx
	__emit__(0xF3, 0xAB);       // rep stosw
	__emit__(0x0B, 0xDB);       // or bx, bx
	asm { jz short gauge_bar_done; }
	__emit__(0x81, 0xC7, 0xF0, 0x00); // add di, (3 * ROW_SIZE)
gauge_bar_partial_word:
	_asm { mov ax, wordmask_1DB0C[bx]; }
gauge_bar_partial_loop:
	__emit__(0xAB);             // stosw
	__emit__(0x83, 0xEF, 0x52); // sub di, 52h
	asm { jge short gauge_bar_partial_loop; }
gauge_bar_done:
	__emit__(0x5E);             // pop si
	__emit__(0x5F);             // pop di
}
#pragma option -k.

#pragma codestring "\x90"

extern "C" void pascal near sub_B60A(void);

int pascal GameExecl(const char *binary_fn)
{
	pi_free(0);
	super_free();
	graph_hide();
	text_clear();
	gaiji_restore();
	game_exit();
	__emit__(0x66, 0x6A, 0x00); // pushd 0
	__emit__(0x66, 0xFF, 0x76, 0x06); // pushd dword ptr [bp+binary_fn]
	_asm {
		push	cs;
	}
	__emit__(0x68); // push offset _MERGED_FN
	__emit__(0xB9, 0x1D);
	_asm {
		push	cs;
	}
	__emit__(0x68); // push offset _MERGED_FN
	__emit__(0xB9, 0x1D);
	_asm {
		call	far ptr execl;
		add	sp, 16;
	}
}

#pragma codestring "anniv\x00\x90\x90\x90\x90"

void pascal near sub_B4A3(void)
{
}

extern "C" void pascal near sub_B4A8(void)
{
	register int si;

	if(byte_1FBC2 == 0) {
		word_1FBC4 = 303;
		word_1FBC6 = 382;
		word_1FBC8 = 0x10;
		word_1FBCA = 382;
		word_1FBCC = 0x10;
		word_1FBCE = 0x10;
		word_1FBD0 = 303;
		word_1FBD2 = 0x10;
	}

	grc_setclip(8, 0, 623, 191);
	word_1FBC4 -= 0x12;
	word_1FBCA -= 0x17;
	word_1FBCC += 0x12;
	word_1FBD2 += 0x17;
	grcg_setcolor(GC_RMW, 2);
	trapezoid_hmask = 0x5555;

	si = 0;
	goto first_loop_test;
first_loop:
		grcg_triangle(
			(si + 303), 8,
			(si + 303), 191,
			(word_1FBC4 + si), (word_1FBC6 / 2)
		);
		grcg_triangle(
			(si + 16), 8,
			(si + 16), 191,
			(word_1FBCC + si), (word_1FBCE / 2)
		);
		si += 320;
first_loop_test:
	if(si <= 320) {
		goto first_loop;
	}

	trapezoid_hmask = 0xAAAA;
	si = 0;
	goto second_loop_test;
second_loop:
		grcg_triangle(
			(si + 16), 191,
			(si + 303), 191,
			(word_1FBC8 + si), (word_1FBCA / 2)
		);
		grcg_triangle(
			(si + 16), 8,
			(si + 303), 8,
			(word_1FBD0 + si), (word_1FBD2 / 2)
		);
		si += 320;
second_loop_test:
	if(si <= 320) {
		goto second_loop;
	}

	byte_1FBC2++;
	if(byte_1FBC2 >= 0x10) {
		_BX = ((183 * ROW_SIZE) + (16 / BYTE_DOTS));
		_asm { nop; push cs; call near ptr sub_B39E; }
		_BX = ((183 * ROW_SIZE) + (336 / BYTE_DOTS));
		_asm { nop; push cs; call near ptr sub_B39E; }
		byte_1FBC3 = 1;
		byte_1FBC2 = 0;
		fp_1FBC0 = reinterpret_cast<nearfunc_t_near>(0x1F2A);
	}

	trapezoid_hmask = 0xFFFF;
	grcg_off();
	grc_setclip(0, 0, (RES_X - 1), (SPRITE16_RES_Y - 1));
}

extern "C" void pascal near sub_B60A(void)
{
	register int si;

	if(byte_1FBC2 == 0) {
		word_1FBC4 = 303;
		word_1FBC6 = 0x10;
		word_1FBC8 = 303;
		word_1FBCA = 382;
		word_1FBCC = 0x10;
		word_1FBCE = 382;
		word_1FBD0 = 0x10;
		word_1FBD2 = 0x10;
	}

	grc_setclip(16, 0, 623, 191);
	word_1FBC6 += 0x17;
	word_1FBC8 -= 0x12;
	word_1FBCE -= 0x17;
	word_1FBD0 += 0x12;
	grcg_setcolor(GC_RMW, 2);

	si = 0;
	goto triangle_loop_test;
triangle_loop:
		grcg_triangle(
			(si + 16), 191,
			(si + 303), 191,
			(word_1FBC4 + si), (word_1FBC6 / 2)
		);
		grcg_triangle(
			(si + 16), 8,
			(si + 16), 191,
			(word_1FBC8 + si), (word_1FBCA / 2)
		);
		grcg_triangle(
			(si + 16), 8,
			(si + 303), 8,
			(word_1FBCC + si), (word_1FBCE / 2)
		);
		grcg_triangle(
			(si + 303), 8,
			(si + 303), 191,
			(word_1FBD0 + si), (word_1FBD2 / 2)
		);
		si += 320;
triangle_loop_test:
	if(si <= 320) {
		goto triangle_loop;
	}

	byte_1FBC2++;
	if(byte_1FBC2 >= 0x10) {
		byte_1FBC2 = 0;
		fp_1FBC0 = reinterpret_cast<nearfunc_t_near>(0x1DC3);
	}

	grcg_off();
	grc_setclip(0, 0, (RES_X - 1), (SPRITE16_RES_Y - 1));
}

static const pixel_t HITCIRCLE_W = 48;
static const pixel_t HITCIRCLE_H = 48;
static const uvram_byte_amount_t HITCIRCLE_VRAM_W = (HITCIRCLE_W / BYTE_DOTS);

static const int HITCIRCLE_CELS = 4;
static const unsigned int HITCIRCLE_FRAMES_PER_CEL = 4;
static const int HITCIRCLE_FRAMES = (HITCIRCLE_FRAMES_PER_CEL * HITCIRCLE_CELS);

static const int HITCIRCLE_ENEMY_COUNT = 12;

// ZUN quirk: Should *maybe* be [PLAYER_COUNT]? If both players get hit within
// [HITCIRCLE_FRAMES], the earlier animation is cut off. It's too obvious of an
// oversight to classify it as a bug, though. The sudden stop of the animation
// could also be supposed to telegraph that the other player got hit, without
// having to peek over to the other playfield.
static const int HITCIRCLE_PLAYER_COUNT = 1;

static const int HITCIRCLE_COUNT = (
	HITCIRCLE_ENEMY_COUNT + HITCIRCLE_PLAYER_COUNT
);

struct HitCircle {
	uint8_t age; // Alive if ≥1.
	pid_t pid;
	screen_point_t topleft;

	void spawn(const pid2& pid_) {
		age = 1;
		pid = pid_;
	}

	sprite16_offset_t cel(void) const {
		// ZUN bloat: (SO_HITCIRCLE + (
		// 	((age - 1) / HITCIRCLE_FRAMES_PER_CEL) * HITCIRCLE_VRAM_W)
		// ). 1 SHR, 1 IMUL. This compiles to both those *and* a bunch of MOVs
		// and ADDs in between...
		static_assert(HITCIRCLE_VRAM_W == 6);
		_BX = (age - 1);
		_BX /= HITCIRCLE_FRAMES_PER_CEL;
		_BX *= 2;
		return (SO_HITCIRCLE + _BX + _BX + _BX);
	}
};

// ZUN bloat: Merge into HitCircle::spawn().
#define hitcircle_set_topleft(p, center_x, center_y, pid) { \
	p.topleft.x = ( \
		playfield_fg_x_to_screen(center_x, pid) - (HITCIRCLE_W / 2) \
	); \
	p.topleft.y = (TO_PIXEL(center_y) + PLAYFIELD_TOP - (HITCIRCLE_H / 2)); \
}

extern HitCircle hitcircles[HITCIRCLE_COUNT];

void pascal hitcircles_enemy_add(
	subpixel_t center_x, subpixel_t center_y, pid2 pid
)
{
	#define last_id hitcircles_enemy_last_id
	extern uint8_t last_id;
	last_id++;
	if(last_id >= HITCIRCLE_ENEMY_COUNT) {
		last_id = 0;
	}

	HitCircle near& p = hitcircles[last_id];
	p.spawn(pid);

	if(!hitcircles_enemy_add_do_not_randomly_center_within_hitbox) {
		center_x = (
			hitbox.origin.topleft.x.v +
			randring_far_next16_mod(hitbox.radius.x * 2)
		);
		center_y = (
			hitbox.origin.topleft.y.v +
			randring_far_next16_mod(hitbox.radius.y * 2)
		);
	}
	hitcircle_set_topleft(p, center_x, center_y, pid);

	#undef last_id
}

void pascal hitcircles_player_add(
	subpixel_t center_x, subpixel_t center_y, pid2 pid
)
{
	HitCircle near& p = hitcircles[HITCIRCLE_ENEMY_COUNT];
	p.spawn(pid);
	hitcircle_set_topleft(p, center_x, center_y, pid);
}

void near hitcircles_update(void)
{
	HitCircle near* p = hitcircles;
	int i = 0;
	while(i < HITCIRCLE_COUNT) {
		if(p->age) {
			p->age++;

			// Not off-by-one, since [age] starts at 1.
			if(p->age > HITCIRCLE_FRAMES) {
				p->age = 0;
			}
		}
		i++;
		p++;
	}
}

void near hitcircles_render(void)
{
	const HitCircle near* p = hitcircles;

	sprite16_mono_(true);

	// ZUN bloat
	_DX = round_or_result_frame;
	asm { and dx, 1 };
	_DX += V_YELLOW_DARK;
	sprite16_mono_color(_DX);

	sprite16_put_size.set(HITCIRCLE_W, HITCIRCLE_H);

	int i = 0;
	while(i < HITCIRCLE_ENEMY_COUNT) {
		if(p->age) {
			sprite16_clip_set_for_pid(p->pid);
			sprite16_offset_t so = p->cel();
			sprite16_put(p->topleft.x, p->topleft.y, so);
		}
		i++;
		p++;
	}

	// Player circle
	static_assert(HITCIRCLE_PLAYER_COUNT == 1);
	_DX = V_WHITE;
	sprite16_mono_color(_DX);
	if(p->age) {
		sprite16_clip_set_for_pid(p->pid);
		sprite16_offset_t so = p->cel();
		sprite16_put(p->topleft.x, p->topleft.y, so);
	}

	// Turn mono mode back off. The macro is expanded here because the
	// original zeroes DX in the assembler direction (`31 D2`) -
	// kb/codegen/0037 - and every other call site of it either passes a
	// nonzero value or is already exact, so the macro itself stays as it is.
	asm { xor	dx, dx; }
	_AH = SPRITE16_SET_MONO;
	geninterrupt(SPRITE16);
}
