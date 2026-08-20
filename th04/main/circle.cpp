#include "th04/main/circle.hpp"
#include "th04/hardware/grcg.hpp"
#include "th04/main/playfld.hpp"
#include "th02/main/entity.hpp"
#include "libs/master.lib/pc98_gfx.hpp"

// More like 17 though, due to the quirks documented below.
static const int CIRCLE_FRAMES = 16;

// circle_t, CIRCLE_COUNT and [circles] moved to th04/main/circle.hpp, which
// this file already includes: stage_state_reset() needs sizeof(circles) and is
// a second translation unit.

#define circle_init(p, center_x, center_y, radius_delta_) { \
	p->flag = F_ALIVE; \
	p->age = 0; \
	p->center.x = (PLAYFIELD_LEFT + (center_x / SUBPIXEL_FACTOR)); \
	p->center.y = (PLAYFIELD_TOP  + (center_y / SUBPIXEL_FACTOR)); \
	p->radius_cur = ( \
		4 + ((radius_delta_ > 0) ? 0 : (CIRCLE_FRAMES * -(radius_delta_))) \
	); \
	p->radius_delta = radius_delta_; \
}

void pascal circles_add_growing(subpixel_t center_x, subpixel_t center_y)
{
	circle_t near *p;
	int i;
	for((p = circles, i = 0); i < CIRCLE_COUNT; (i++, p++)) {
		if(p->flag != F_FREE) {
			continue;
		}
		circle_init(p, center_x, center_y, 8);
		break;
	}
}

void pascal circles_add_shrinking(subpixel_t center_x, subpixel_t center_y)
{
	circle_t near *p;
	int i;
	for((p = circles, i = 0); i < CIRCLE_COUNT; (i++, p++)) {
		if(p->flag != F_FREE) {
			continue;
		}
		circle_init(p, center_x, center_y, -8);
		break;
	}
}

void near circles_update(void)
{
	circle_t near *p;
	int i;
	for((p = circles, i = 0); i < CIRCLE_COUNT; (i++, p++)) {
		if(p->flag == F_REMOVE) {
			p->flag = F_FREE;
		}
		if(p->flag != F_ALIVE) {
			continue;
		}

		// ZUN quirk: This runs before [boss_update] or the bomb update/render
		// function. Any circles spawned there will therefore bypass this
		// update on their first frame and render at their initial radius.
		p->radius_cur += p->radius_delta;
		p->age++;
		if(p->age > CIRCLE_FRAMES) {
			// ZUN quirk: Deferring the removal until the next update means
			// that this circle will still be rendered on this frame.
			p->flag = F_REMOVE;
		}
	}
}

void near circles_render(void)
{
	grcg_setcolor_direct(circles_color);
	circle_t near *p;
	int i;
	for((p = circles, i = 0); i < CIRCLE_COUNT; (i++, p++)) {
		if(p->flag != F_ALIVE) {
			continue;
		}
		grcg_circle(p->center.x, p->center.y, p->radius_cur);
	}
}

// THIS FILE IS SHARED. th04/circle.cpp and th05/circle.cpp both #include it,
// so anything added here is compiled into BOTH binaries. TH05 has no Elly, and
// its CIRCLE_TEXT root contribution is zero bytes -- an unguarded definition
// put a segment TH05's dump never declares into its MAIN_01 and the group blew
// past 64K at link time. Everything Elly-specific is therefore guarded, the
// th04-only header include included.
#if (GAME == 4)

// elly_backdrop_colorfill() belongs to CIRCLE_TEXT's HEAD, not its tail: in the
// original it sits 0x4F6 bytes into th04_main.asm's root contribution, with
// 0x77E bytes of root ASM still following it. A plain definition here would
// therefore land after the entire root contribution, 0x77E bytes late.
// th04_main.asm splits the segment at that point instead (kb/codegen 0080,
// head-rename form), and this function is emitted into the head from THIS
// source rather than from an object of its own.
//
// `#pragma codeseg` is what makes that free. kb/codegen 0080 assumes the head
// gets a new object declaring `-zC<HEAD>`, which would mean a new Tupfile.lua
// line -- and Tupfile.lua is a whole-file claim that some other lane usually
// holds. th03/main/enemy/enemy.cpp and th02/end/end.cpp show that one object
// can contribute to several code segments instead. The group has to be named:
// the call to grcg_fill_playfield_rows() below is NEAR and its callee lives in
// main_013_TEXT, so the two segments must share main_01.
//
// AND THIS FILE MUST NOT INCLUDE th04/main/boss/bosses.hpp, which is where
// elly_backdrop_colorfill() is declared for its caller in
// th04/main/stage/setup.cpp. `#pragma codeseg` binds a function to a segment at
// its FIRST DECLARATION, not at its definition. With that header included, the
// declaration is read while the default segment is still in force, the
// definition below is emitted into CIRCLE_TEXT no matter what pragma precedes
// it, and the object contributes a zero-length CIRCLE_A_TEXT -- which links,
// runs, and is wrong by 0x770 bytes. Measured by bisection over this file: the
// single edit that moves the function is dropping that include.
//
// That is also what th04/main/hud/overlay.hpp means by "Needs to be here to get
// the effect of `#pragma codeseg`", and why th04/main/scroll.hpp wraps a
// DECLARATION rather than a definition. Declaring elly here instead would work
// too; it is not done because bosses.hpp is the header its caller reads, and
// two declarations of one function in two segments is the next lane's bug.
//
// `-k-`: the original has no stack frame at all -- `push di` is the first
// instruction, not `push bp`. Bracketed rather than left on, because a #pragma
// option keeps applying to everything generated after it and the four circle
// functions below are already matched WITH frames (kb/codegen 0011).
#pragma option -k-
#pragma codeseg CIRCLE_A_TEXT main_01

// Elly's fight fills the playfield from row 112 down. The single fill is what
// distinguishes this from the rest of the [boss_backdrop_colorfill] family;
// kurumi_backdrop_colorfill() in th04/main/boss/colorfill.cpp is the same shape
// with two, and byte-comparing the two originals is what established this one
// as compiler output before a build was spent on it (kb/codegen 0115).
void pascal near elly_backdrop_colorfill(void)
{
	grcg_fill_playfield_rows_at(112, 256);
}

#pragma codeseg
#pragma option -k
#endif
