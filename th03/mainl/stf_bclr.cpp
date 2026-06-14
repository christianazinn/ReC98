#pragma codeseg mainl_03_TEXT group_01

#include "pc98.h"

static const unsigned int FLAKE_COUNT = 80;

struct flake_t {
	unsigned char alive;
	char unused[15];
};

extern flake_t near flakes[FLAKE_COUNT];

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

#pragma codeseg
