#pragma codeseg mainl_03_TEXT group_01

#include "libs/master.lib/master.hpp"
#include "pc98.h"
#include "th01/math/subpixel.hpp"
#include "th03/math/vector.hpp"
#include "th03/sprites/flake.h"
#include "th03/snd/snd.h"
#include "x86real.h"

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

void pascal near flake_put(screen_x_t left, screen_y_t top, int cel);

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

void near flakes_render(void)
{
	register flake_t near *flake = flakes;
	for(register int i = 0; staffroll_flake_count > i; i++, flake++) {
		if(flake->alive) {
			flake_put(
				TO_PIXEL(flake->left.v), TO_PIXEL(flake->top.v), flake->cel
			);
		}
	}
}

bool16 pascal near staffroll_phase_done(
	uint16_t measure_threshold, int frame_threshold
)
{
	if(!snd_active) {
		if(staffroll_frame > frame_threshold) {
			goto phase_done;
		}
		goto phase_not_done;
	}
	_AH = KAJA_GET_SONG_MEASURE;
	asm { int 60h; }
	if(_AX < measure_threshold) {
		goto phase_not_done;
	}
	if(staffroll_frame <= 0xC0) {
		goto phase_not_done;
	}
phase_done:
	_AX = true;
	goto phase_return;
phase_not_done:
	_AX = false;
phase_return:
	return _AX;
}

#pragma codeseg
