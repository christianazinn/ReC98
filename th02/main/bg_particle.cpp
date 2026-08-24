/* ReC98
 * -----
 * TH02's boss background particles, together with the dot-square primitive
 * that all of the game's boss background effects blit their points with. ZUN's
 * object for this code segment held both these and the sparks, which is why
 * they are compiled into the same translation unit here — see th02/spark.cpp.
 */

// The original's prologs are `push bp; mov bp, sp; sub sp, N`, which is -G.
// The spark code that follows in this object was built without it and emits
// ENTER, so the option has to be turned back off at the end of this file.
// (kb/codegen/0011)
#pragma option -G

#include <stddef.h>
#include "platform.h"
#include "pc98.h"
#include "planar.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/bg_particle.hpp"

// ZUN landmine: Just like sparks_add(), bg_particles_update_and_render()
// insists on calling this function using a `near` call with the current code
// segment. This will cause a fixup overflow if this function is ever further
// than 64 KiB away from the call site. Just use the declaration from the
// regular header instead. (See th02/main/spark.cpp, 690 bytes further down in
// the original binary, which does the exact same thing.)
void pascal near vector2(int&, int&, unsigned char, int);

extern "C" bool reduce_effects;

// Write-only. The first is seeded to -1 by bg_particles_reset() and copied
// into [_2] once per frame, right next to a copy of [bg_particle_col] into
// [_3]; nothing ever reads any of the three. The first is `_BSS` right after
// [bg_particle_unput_col], [_2] and [_3] are `_BSS` right after
// [dot_square_top]. ZUN bloat.
// The first is unnumbered on purpose, exactly as th02/main/scroll.cpp:25-27
// spells its own three: a `_1` would imply a contiguous family that the
// layout above does not have.
extern int8_t bg_particle_unused;
extern uint8_t bg_particle_unused_2;
extern uint8_t bg_particle_unused_3;

// ZUN bloat: `p = &bg_particles[0];`
#define unneeded_copy	reinterpret_cast<bg_particle_t near *>(_AX)

// ZUN bloat: Pointer arithmetic is faster with Turbo C++ 4.0J, but assumes
// the original structure layout. (Same macro as in th02/main/spark.cpp.)
#define screen_topleft_bytewise(p, offset) \
	reinterpret_cast<SPPoint near *>( \
		(reinterpret_cast<uint8_t near *>(p->screen_topleft) + offset) \
	)

void far bg_particles_reset(void)
{
	// ZUN bloat: The loop counter is the byte offset into the array rather
	// than a slot index, which is why this needs the pseudo-register.
	for(
		_BX = 0;
		static_cast<int16_t>(_BX) < static_cast<int>(sizeof(bg_particles));
		_BX += sizeof(bg_particle_t)
	) {
		reinterpret_cast<bg_particle_t near *>(
			reinterpret_cast<uint8_t near *>(bg_particles) + _BX
		)->flag = F_FREE;
	}
	bg_particle_speed_initial = 32;
	bg_particle_speed_delta = 1;
	bg_particle_angle_delta = 0;
	bg_particle_col = V_WHITE;
	bg_particle_unput_col = 0;
	bg_particle_unused = -1;
	bg_particle_edge_step = 32;
}

void pascal far bg_particles_add(
	screen_x_t left, screen_y_t top, unsigned char angle
)
{
	register bg_particle_t near *p;

	#define subpixel_left	_CX
	#define subpixel_top 	_BX

	subpixel_left = left;
	subpixel_top = top;
	TO_SP_INPLACE(subpixel_left);
	TO_SP_INPLACE(subpixel_top);

	p = unneeded_copy = bg_particles;
	for(_DX = 0; static_cast<int16_t>(_DX) < BG_PARTICLE_COUNT; _DX++, p++) {
		if(p->flag != F_FREE) {
			continue;
		}
		p->flag = F_ALIVE;
		p->angle = angle;
		p->screen_topleft[0].x.v = subpixel_left;
		p->screen_topleft[1].x.v = subpixel_left;
		p->screen_topleft[0].y.v = subpixel_top;
		p->screen_topleft[1].y.v = subpixel_top;
		p->edge = 1;
		p->speed.v = bg_particle_speed_initial;
		break;
	}

	#undef subpixel_top
	#undef subpixel_left
}

#pragma codestring "\x90"

void pascal far grcg_dot_square_put(int edge)
{
	// Rows still to be blitted, counted down from [edge]. An earlier candidate
	// used `edge_left` (`ec571c60:th02/main/bg_particle.cpp:115`), but every
	// other `edge` in the tree means "boundary of a region"
	// (x_edge_offset, bullet_bounce_edge_t, …), which would read this as the
	// square's *left* edge.
	#define rows_remaining	_SI

	#define vo           	_DI
	#define first_bit    	_BX
	#define dots_mirrored	_DX

	int dots;

	_ES = SEG_PLANE_B;
	rows_remaining = edge;
	vo = vram_offset_shift_fast(dot_square_left, dot_square_top);
	first_bit = dot_square_left;

	// ZUN bloat: Turbo C++ 4.0J would otherwise only ever generate an 8-bit
	// AND. (Same as in bomb_circle_point_put(), except that this one is the
	// sign-extended 3-byte encoding, which Turbo Assembler does not pick for
	// `and bx, BYTE_MASK`.)
	asm { db 083h, 0E3h, BYTE_MASK; } // and bx, BYTE_MASK

	// ZUN bloat: A manual 16-bit right rotation, exactly as in
	// bomb_circle_point_put(). Turbo C++ 4.0J has intrinsics for that, which
	// compile into a single ROR instruction.
	asm { mov	al, DOT_SQUARE_ROWS[si]; }
	_AH = 0;
	dots = _AX;
	_CL = _BL;
	_AX >>= _CL;
	_CL = (sizeof(dots16_t) * BYTE_DOTS);
	_CL -= _BL;
	dots_mirrored = dots;
	dots_mirrored <<= _CL;
	_AX |= dots_mirrored;

	while(static_cast<int16_t>(rows_remaining) > 0) {
		*reinterpret_cast<dots16_t __es *>(vo) = _AX;
		vo += ROW_SIZE;
		rows_remaining--;
	}

	#undef dots_mirrored
	#undef first_bit
	#undef vo
	#undef rows_remaining
}

void pascal near grcg_dot_square_unput(int edge)
{
	#define rows_remaining	_SI
	#define vo            	_BX

	rows_remaining = edge;
	vo = vram_offset_shift_fast(dot_square_left, dot_square_top);
	while(static_cast<int16_t>(rows_remaining) > 0) {
		*reinterpret_cast<dots16_t __es *>(vo) = 0xFFFF;
		vo += ROW_SIZE;
		rows_remaining--;
	}

	#undef vo
	#undef rows_remaining
}

#pragma codestring "\x90"

// Unblits every live particle from the back page, then retires the ones that
// were marked F_REMOVE during the previous frame. Also carries each surviving
// particle's front-page position over to the back page, which is what
// bg_particles_update_and_render() then adds this frame's motion to.
void far bg_particles_invalidate(void)
{
	register bg_particle_t near *p;
	register int i;

	_ES = SEG_PLANE_B;
	grcg_setcolor(GC_RMW, bg_particle_unput_col);
	p = unneeded_copy = bg_particles;
	for(i = 0; i < BG_PARTICLE_COUNT; i++, p++) {
		if(p->flag == F_FREE) {
			continue;
		}
		_BX = (page_back * sizeof(SPPoint));
		dot_square_left = screen_topleft_bytewise(p, _BX)->x.to_pixel();
		dot_square_top = screen_topleft_bytewise(p, _BX)->y.to_pixel();

		// With [reduce_effects], every particle is only unblitted from every
		// other page, which halves its effective frame rate.
		if(!reduce_effects || (page_back == (i & 1))) {
			grcg_dot_square_unput(p->edge);
		}
		if(p->flag == F_REMOVE) {
			p->flag = F_FREE;
			continue;
		}

		// ZUN bloat:
		// 	p->screen_topleft[!page_front] = p->screen_topleft[page_front];
		_BX = (page_front * sizeof(SPPoint));
		_AX = screen_topleft_bytewise(p, _BX)->x.v;
		_CX = screen_topleft_bytewise(p, _BX)->y.v;
		_BX ^= sizeof(SPPoint);
		screen_topleft_bytewise(p, _BX)->x.v = _AX;
		screen_topleft_bytewise(p, _BX)->y.v = _CX;
	}
	grcg_off();
}

#pragma codestring "\x90"

void far bg_particles_update_and_render(void)
{
	register bg_particle_t near *p;
	register int edge_threshold;
	int i;
	int delta_x;
	int delta_y;

	grcg_setcolor(GC_RMW, bg_particle_col);
	bg_particle_unused_2 = bg_particle_unused;
	bg_particle_unused_3 = bg_particle_col;
	p = unneeded_copy = bg_particles;
	for(i = 0; i < BG_PARTICLE_COUNT; i++, p++) {
		if(p->flag != F_ALIVE) {
			continue;
		}
		p->speed.v += bg_particle_speed_delta;
		p->angle += bg_particle_angle_delta;

		// ZUN bloat: vector2(delta_x, delta_y, p->angle, p->speed.v);
		// [angle] is read as a word, which pushes the low byte of the
		// following per-page X coordinate along with it. Only AL is used by
		// the callee, so the garbage is harmless.
		static_assert(offsetof(bg_particle_t, angle) == 0x01);
		static_assert(offsetof(bg_particle_t, speed) == 0x0B);
		asm {
			push	ss;
			lea 	ax, delta_x;
			push	ax;
			push	ss;
			lea 	ax, delta_y;
			push	ax;
			db  	0FFh, 074h, 001h; // push word ptr [si+1]
			db  	08Ah, 044h, 00Bh; // mov al, [si+0Bh]
			mov 	ah, 0;
			push	ax;
			nop;
			push	cs;
			call	near ptr vector2;
		}

		// ZUN bloat: The two pixel coordinates stay in DX and AX so that the
		// clipping test below needs no reload, which is only reproducible by
		// naming both registers.
		#define screen_left	static_cast<screen_x_t>(_DX)
		#define screen_top 	static_cast<screen_y_t>(_AX)

		// ZUN bloat:
		// 	screen_topleft_bytewise(p, _BX)->x.v += delta_x;
		// 	screen_topleft_bytewise(p, _BX)->y.v += delta_y;
		// Turbo C++ 4.0J adds through AX, which would leave the pixel
		// coordinates below in the wrong registers.
		static_assert(offsetof(bg_particle_t, screen_topleft) == 0x02);
		static_assert(sizeof(SPPoint) == 4);
		_BX = (page_back * sizeof(SPPoint));
		_DX = delta_x;
		asm { db 001h, 050h, 002h; } // add [bx+si+2], dx
		_DX = delta_y;
		asm { db 001h, 050h, 004h; } // add [bx+si+4], dx
		_DX = screen_topleft_bytewise(p, _BX)->x.v;
		screen_left >>= SUBPIXEL_BITS;
		dot_square_left = screen_left;
		_AX = screen_topleft_bytewise(p, _BX)->y.v;
		screen_top >>= SUBPIXEL_BITS;
		dot_square_top = screen_top;

		// Same four conditions as playfield_clip_topleft_small(), but with
		// the right edge tested before the left one.
		if(
			playfield_clip_right_small(screen_left, BG_PARTICLE_W) ||
			playfield_clip_left_small(screen_left, BG_PARTICLE_W) ||
			playfield_clip_top_small(screen_top, BG_PARTICLE_H) ||
			playfield_clip_bottom_small(screen_top, BG_PARTICLE_H)
		) {
			p->flag = F_REMOVE;
			continue;
		}

		// The faster a particle is, the larger its square. The loop stops one
		// step above 0, so [edge] never falls below 1.
		// ZUN bloat: edge_threshold = (edge_step * DOT_SQUARE_EDGE_MAX);
		// Spelling the multiplication out as a power of two plus one more step
		// is what keeps Turbo C++ 4.0J from emitting a single IMUL.
		edge_threshold = (
			(bg_particle_edge_step * (DOT_SQUARE_EDGE_MAX - 1)) +
			bg_particle_edge_step
		);
		_BL = DOT_SQUARE_EDGE_MAX;
		while(edge_threshold > bg_particle_edge_step) {
			if(p->speed.v > edge_threshold) {
				break;
			}
			edge_threshold -= bg_particle_edge_step;
			_BL--;
		}
		p->edge = _BL;

		if(!reduce_effects || (page_back == (i & 1))) {
			grcg_dot_square_put(p->edge);
		}

		#undef screen_top
		#undef screen_left
	}
	grcg_off();
}

#undef screen_topleft_bytewise
#undef unneeded_copy

#pragma option -G-
