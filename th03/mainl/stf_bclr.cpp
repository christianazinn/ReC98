#pragma codeseg mainl_03_TEXT group_01

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "pc98.h"
#include "platform/x86real/pc98/keyboard.hpp"
#include "th01/math/subpixel.hpp"
#include "th03/fast_forward.hpp"
#include "th03/math/vector.hpp"
#include "th03/resident.hpp"
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
extern bool staffroll_cdg_put_alpha;
extern int staffroll_frame;
extern bool staffroll_flake_reset_pending;
extern page_t page_back;

static bool near staffroll_fast_forward_held(void)
{
	if(!resident->unused_3[T3_RES_FAST_FORWARD_STAFF_UNLOCKED_INDEX]) {
		return false;
	}
	return ((peekb(0, KEYGROUP_5) & K5_Z) != 0);
}

static void near staffroll_fast_forward_wait_skip(void)
{
	uint8_t phase;

	if(!staffroll_fast_forward_held()) {
		resident->unused_3[T3_RES_FAST_FORWARD_STAFF_PHASE_INDEX] = 0;
		return;
	}
	phase = resident->unused_3[T3_RES_FAST_FORWARD_STAFF_PHASE_INDEX];
	phase++;
	if(phase >= T3_STAFF_FAST_FORWARD_RATE) {
		resident->unused_3[T3_RES_FAST_FORWARD_STAFF_PHASE_INDEX] = 0;
		return;
	}
	resident->unused_3[T3_RES_FAST_FORWARD_STAFF_PHASE_INDEX] = phase;
	vsync_Count1 = 1;
}

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
	if(staffroll_fast_forward_held()) {
		if(staffroll_frame > frame_threshold) {
			goto phase_done;
		}
		goto phase_not_done;
	}
	if(!snd_active) {
		if(staffroll_frame > frame_threshold) {
			goto phase_done;
		}
		goto phase_not_done;
	}
	_AH = KAJA_GET_SONG_MEASURE;
	if(snd_bgm_is_fm()) {
		geninterrupt(PMD);
	} else {
		// ED.M retains the regular four-quarter-note measure used by MMD.
		_DX = (MMD_TICKS_PER_QUARTER_NOTE * 4);
		geninterrupt(MMD);
	}
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

void near staffroll_flakes_tick(void)
{
	flakes_spawn();
	flakes_update();
	flakes_render();
	if(staffroll_flake_reset_pending) {
		if(vsync_Count1 > 1) {
			staffroll_cdg_put_alpha = false;
			staffroll_flake_count = 0x32;
			staffroll_flake_reset_pending = false;
		}
	}
	staffroll_fast_forward_wait_skip();
	while(vsync_Count1 == 0) {
	}
	vsync_Count1 = 0;
	graph_showpage(page_back);
	_AL = 1;
	_AL -= page_back;
	page_back = _AL;
	graph_accesspage(_AL);
}

#pragma codeseg
