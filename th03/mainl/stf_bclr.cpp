#pragma codeseg mainl_03_TEXT group_01

#include "libs/master.lib/master.hpp"
#include "pc98.h"
#include "th01/math/subpixel.hpp"
#include "th03/math/vector.hpp"
#include "th03/sprites/flake.h"

static const unsigned int FLAKE_COUNT = 80;
static const subpixel_t FLAKE_LEFT_MAX = TO_SP(RES_X - FLAKE_W);
static const subpixel_t FLAKE_TOP_MAX = TO_SP(RES_Y - FLAKE_H);

struct flake_t {
	bool alive;
	char padding_1;
	Subpixel left;
	Subpixel top;
	SPPoint velocity;
	int cel;
	char padding_2[4];
};

extern flake_t near flakes[FLAKE_COUNT];
extern unsigned char staffroll_flake_count;
extern int staffroll_frame;

void near staffroll_blue_plane_clear(void)
{
	asm {
		db  	057h
		mov 	ax, SEG_PLANE_B
		mov 	es, ax
		db  	031h, 0C0h
		db  	089h, 0C7h
		mov 	cx, (PLANE_SIZE / 2)
		rep 	stosw
		db  	05Fh
	}
}

void near flakes_reset(void)
{
	register flake_t near *flake = flakes;
	for(register int i = 0; i < FLAKE_COUNT; i++, flake++) {
		flake->alive = false;
	}
}

void near flakes_spawn(void)
{
	unsigned char angle;
	unsigned char length;
	register flake_t near *flake = flakes;
	for(register int i = 0; staffroll_flake_count > i; i++, flake++) {
		if(!flake->alive && ((i * 8) <= staffroll_frame)) {
			flake->alive = true;
			if(i & 3) {
				flake->left.v = (irand() % FLAKE_LEFT_MAX);
				flake->top.v = 0;
			} else {
				flake->left.v = FLAKE_LEFT_MAX;
				flake->top.v = (irand() % FLAKE_TOP_MAX);
			}

			angle = ((irand() % 0x20) + 0x50);
			length = ((irand() % TO_SP(4)) + TO_SP(3));
			flake->cel = (irand() & (FLAKE_CELS - 1));
			vector2(flake->velocity.x.v, flake->velocity.y.v, angle, length);
		}
	}
}

void near flakes_update(void)
{
	register flake_t near *flake = flakes;
	for(register int i = 0; staffroll_flake_count > i; i++, flake++) {
		if(flake->alive) {
			flake->alive = true;
			flake->left.v += flake->velocity.x.v;
			flake->top.v += flake->velocity.y.v;
			if(flake->left.v <= 0) {
				flake->left.v += FLAKE_LEFT_MAX;
			}
			if(flake->top.v >= FLAKE_TOP_MAX) {
				flake->top.v -= FLAKE_TOP_MAX;
			}
		}
	}
}

#pragma codeseg
