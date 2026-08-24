/// Stage 2 scenery
/// ---------------
/// The shared Stage 1/2 lightning flash and Stage 2's per-frame scenery
/// callback. These are the last two procedures in th02_main.asm's
/// BOSS_5_TEXT contribution, immediately before th02/main/midboss/m2.cpp.

#pragma option -zCBOSS_5_TEXT -zPmain_03 -G

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/math/subpixel.hpp"
#include "th02/math/randring.hpp"
#include "th02/main/bg_particle.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/spark.hpp"

extern "C" int16_t bg_flash_frame;

// The next frame on which the shared lightning flash begins.
extern "C" uint16_t bg_flash_next_frame;

// Stage 2's own scenery clock and one-shot setup flag.
extern "C" uint16_t stage2_scenery_frame;
extern "C" uint8_t stage2_scenery_setup;

void pascal near stage_scenery_particle_add(screen_x_t left, screen_y_t top);
void near stage_scenery_particles_render(void);

void near stage_bg_flash_update(void)
{
	if(bg_flash_frame != 0) {
		if(bg_flash_frame == 1) {
			bg_flash_next_frame = 100;
		}
		bg_flash_frame++;
		if(
			(bg_flash_frame >= bg_flash_next_frame) &&
			(bg_flash_frame < (bg_flash_next_frame + 8))
		) {
			if(bg_flash_frame & 1) {
				PaletteTone = 140;
			} else {
				PaletteTone = 100;
			}
			palette_show();
			if(bg_flash_frame == (bg_flash_next_frame + 7)) {
				PaletteTone = 100;
				palette_show();
				bg_flash_next_frame += ((randring2_next16() & 1023) + 20);
			}
		}
	}
}

extern "C" void far stage2_update_and_render(void)
{
	if(stage2_scenery_setup != 0) {
		grc_setclip(
			PLAYFIELD_LEFT, 0, PLAYFIELD_RIGHT, (RES_Y - 1)
		);
		stage2_scenery_setup = 0;
		spark_accel_x.v = -1;
	}

	stage2_scenery_frame++;
	if((stage2_scenery_frame & 1) == 0) {
		stage_scenery_particle_add(
			((randring2_next16() % 640) + 48), 16
		);
	}

	stage_bg_flash_update();
	egc_off();
	grcg_setcolor(GC_RMW, 3);
	stage_scenery_particles_render();
}
