/// TH02-specific stage callbacks
/// -----------------------------
/// Stage 3's [stage_invalidate] and [stage_update_and_render], which
/// stage_init()'s `case 2:` installs and stage_loop() then calls once per
/// frame for the rest of the stage - boss fight included, since that slot is
/// never cleared for Stage 3.
///
/// The effect is a ring of 64 dots expanding out of the middle of the
/// playfield, over a palette colour 0 that walks red -> green -> blue ->
/// magenta and back, on a 200-frame cycle that restarts forever. It is what
/// th02/main/bgm_show.cpp's `stage_id == 2` arm blacks out again, and what
/// [stage3_effect_frame] - reset from bosses_reset() in
/// th02/main/bullet/bullet.cpp - is the timeline of.
///
/// These two are the WHOLE of what th02_main.asm used to contribute to
/// DIALOG_TEXT, so that contribution is now empty and this translation unit
/// leads the segment, ahead of th02/main/midboss/m3.cpp. (kb/codegen/0099)
///
/// This is the TH02 counterpart of th04/main/stage/stages.cpp and
/// th05/main/stage/stages.cpp. Unlike those two it carries no `stages.hpp`:
/// th02/main/stage/init.cpp declares all five stages' effect functions inline
/// beside each other, and a header for two of the five would split a list
/// that reads better whole.

// -zC, because the segment name would otherwise come from this file's own
// basename (kb/codegen/0105). -G, because the prologs are
// `push bp; mov bp, sp` and `push bp; mov bp, sp; sub sp, 4` rather than an
// `ENTER` (kb/codegen/0011). No -a2: `[measured]` nothing here emits a
// generated jump table.
#pragma option -zCDIALOG_TEXT -zPmain_03 -G

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/slowdown.hpp"
#include "th02/main/tile/tile.hpp"

// th02/snd/snd.h, declared here rather than included, the way
// th02/main/midboss/m3.cpp and th02/main/laser.cpp already declare it: snd.h
// has no include guard and pulls in three more unguarded headers for the sake
// of one function.
extern "C" void __cdecl snd_se_play(int new_se);

// Not declared in libs/master.lib/pc98_gfx.hpp, which only has the
// graph_scrollup() convenience wrapper around it. Declared exactly the way
// th02/main/explode.cpp already declares it, which is also the object that
// makes the same three-argument call for the boss explosion's flash.
extern "C" {
	void MASTER_RET graph_scroll(unsigned line1, unsigned adr1, unsigned adr2);
}

// The dots in the ring, and the angle between two of them. 64 × 4 is exactly
// one revolution of master.lib's byte-wide angle, so the ring closes.
static const int STAGE3_RING_DOTS = 64;
static const uint8_t STAGE3_RING_ANGLE_STEP = 4;

// State
// -----

// The effect's own frame counter, and the only thing it runs on. Counts up
// from 0, does nothing until 168, and is snapped back to 200 at 1000 so that
// the four colour phases repeat for the rest of the stage. Zeroed by
// bosses_reset(); see th02/main/bullet/bullet.cpp for why it lives there.
extern "C" uint16_t stage3_effect_frame;

// How far the ring has expanded, 0 to 200, restarted at every colour change.
extern "C" int16_t stage3_ring_radius;

// The angle the ring's first dot is drawn at, which walks one step per frame
// and reverses direction with every colour phase - so the whole ring
// counter-rotates as it grows.
extern "C" uint8_t stage3_ring_angle;

// Where the ring expands from: the middle of the playfield, and the middle of
// the 400-line screen. ZUN bloat: written every frame between 168 and 200 and
// never anything but these two constants.
extern "C" screen_x_t stage3_ring_center_x;
extern "C" screen_y_t stage3_ring_center_y;

// Every dot's unwrapped position, kept per page so that stage3_invalidate()
// can mark the previous frame's dots on the page it is about to draw again.
// `[measured]` One 512-byte extent, not two arrays: IDA split it in two at
// the first reference, 2 bytes and then the remaining 510, but the stride
// from one dot to the next is 4, so the two coordinates are interleaved.
// (kb/codegen/0123)
extern "C" screen_point_t stage3_ring_dot[PAGE_COUNT][STAGE3_RING_DOTS];
// -----


// Marks the tile under every dot of the previous frame's ring for redrawing.
// One tile each: the rect is 1×2 pixels, which is smaller than the dot the
// renderer actually sets, and smaller than a tile in both directions.
// Installed into [stage_invalidate] by stage_init().
extern "C" void far stage3_invalidate(void)
{
	register int i;

	for(i = 0; i < STAGE3_RING_DOTS; i++) {
		tiles_invalidate_rect(
			stage3_ring_dot[page_back][i].x,
			stage3_ring_dot[page_back][i].y,
			1,
			2
		);
	}
}


// One frame of the effect: the 32-frame strobe and hardware-scroll handoff
// that starts it at frame 168, then the colour phase for wherever
// [stage3_effect_frame] has got to, and finally the ring itself.
// Installed into [stage_update_and_render] by stage_init().
extern "C" void far stage3_update_and_render(void)
{
	int x;
	uint8_t angle;
	register screen_y_t y;
	// Reused for the hardware-scroll parity below, which is the same
	// double duty th02/main/explode.cpp's copy of that call gives it.
	int i;

	stage3_effect_frame++;
	if(stage3_effect_frame >= 168) {
		if(stage3_effect_frame == 168) {
			snd_se_play(14);
		} else if(stage3_effect_frame < 200) {
			if(stage3_effect_frame <= 180) {
				palette_settone((((stage3_effect_frame & 1) * 70) + 100));
			}
			i = (stage3_effect_frame & 1);
			graph_scroll(
				(RES_Y - scroll_line), ((scroll_line * 40) + i), i
			);
			slowdown_factor = 3;
			stage3_ring_center_x = 224;
			stage3_ring_center_y = 200;
			stage3_ring_radius = 0;
			stage3_ring_angle = 0;
		} else if(stage3_effect_frame < 400) {
			if(stage3_effect_frame == 200) {
				slowdown_factor = 1;
			}
			Palettes[0].c.r = (200 - (stage3_effect_frame >> 1));
			Palettes[0].c.g = 0;
			Palettes[0].c.b = 0;
			stage3_ring_radius = (stage3_effect_frame - 200);
			stage3_ring_angle++;
		} else if(stage3_effect_frame < 600) {
			Palettes[0].c.r = 0;
			Palettes[0].c.g = (44 - (stage3_effect_frame >> 1));
			Palettes[0].c.b = 0;
			stage3_ring_radius = (stage3_effect_frame - 400);
			stage3_ring_angle--;
		} else if(stage3_effect_frame < 800) {
			Palettes[0].c.r = 0;
			Palettes[0].c.g = 0;
			Palettes[0].c.b = (144 - (stage3_effect_frame >> 1));
			stage3_ring_radius = (stage3_effect_frame - 600);
			stage3_ring_angle++;
		} else if(stage3_effect_frame < 1000) {
			Palettes[0].c.r = (244 - (stage3_effect_frame >> 1));
			Palettes[0].c.g = 0;
			Palettes[0].c.b = (244 - (stage3_effect_frame >> 1));
			stage3_ring_radius = (stage3_effect_frame - 800);
			stage3_ring_angle--;
		} else {
			stage3_effect_frame = 200;
		}
		palette_show();
		egc_off();
		grcg_setcolor(GC_RMW, 7);
		for(
			i = 0, angle = stage3_ring_angle;
			i < STAGE3_RING_DOTS;
			i++, angle = (angle + STAGE3_RING_ANGLE_STEP)
		) {
			// The same `(radius * table) >> 8` shape th02/main/boss/b4.cpp's
			// orbs use, and Turbo C++ 4.0J widens both sides of it to 32 bits
			// the same way.
			x = (
				(
					((long)(stage3_ring_radius) * CosTable8[angle]) >> 8
				) + stage3_ring_center_x
			);
			y = (
				(
					((long)(stage3_ring_radius) * SinTable8[angle]) >> 8
				) + stage3_ring_center_y
			);
			stage3_ring_dot[page_back][i].x = x;
			stage3_ring_dot[page_back][i].y = y;

			// Stored unwrapped, drawn wrapped: stage3_invalidate() re-derives
			// the tile from the stored pair on a later frame, when
			// [scroll_line] will have moved on.
			y += scroll_line;
			if(y >= RES_Y) {
				y -= RES_Y;
			}
			grcg_pset(x, y);
		}
		grcg_off();
	}
}
