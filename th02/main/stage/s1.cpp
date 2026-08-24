/// Stage 1 scenery and the shared Stage 1/2 particle pool
/// ------------------------------------------------------
/// These are the first four procedures in th02_main.asm's BOSS_5_TEXT
/// contribution. The particle pool is shared with Stage 2; it is distinct
/// from the later 64-slot boss background-particle subsystem.

#pragma option -zCBOSS_5_TEXT -zPmain_03 -G

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/math/randring.hpp"
#include "th02/main/bg_particle.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/spark.hpp"
#include "th02/main/tile/tile.hpp"

static const int STAGE_SCENERY_PARTICLE_COUNT = 50;

enum stage_scenery_particle_flag_t {
	SSPF_FREE = 0,
	SSPF_ALIVE = 1,
	SSPF_REMOVE = 2,
};

struct stage_scenery_particle_t {
	uint8_t flag;
	uint8_t unused;
	screen_x_t left[PAGE_COUNT];
	screen_y_t top[PAGE_COUNT];
	int16_t speed;
};

extern "C" stage_scenery_particle_t stage_scenery_particles[
	STAGE_SCENERY_PARTICLE_COUNT
];

extern "C" int16_t bg_flash_frame;
extern "C" int16_t stage1_scenery_frame;
extern "C" bool reduce_effects;

void near stage_bg_flash_update(void);

void pascal near stage_scenery_particle_add(
	register screen_x_t left, screen_y_t top
)
{
	stage_scenery_particle_t far *p = stage_scenery_particles;
	register int i;
	static_assert(sizeof(stage_scenery_particle_t) == 12);

	for(i = 0; i < STAGE_SCENERY_PARTICLE_COUNT; i++, p++) {
		if(p->flag != SSPF_FREE) {
			continue;
		}
		p->flag = SSPF_ALIVE;
		p->left[0] = left;
		p->top[0] = top;
		p->left[1] = left;
		p->top[1] = top;
		p->speed = (randring2_next8_and(3) + 3);
		break;
	}
}

extern "C" void far stage_scenery_invalidate(void)
{
	stage_scenery_particle_t far *p = stage_scenery_particles;
	register int i;

	for(i = 0; i < STAGE_SCENERY_PARTICLE_COUNT; i++, p++) {
		if(p->flag == SSPF_FREE) {
			continue;
		}
		tiles_invalidate_rect(
			p->left[page_back], p->top[page_back], 2, 3
		);
		if(p->flag == SSPF_REMOVE) {
			p->flag = SSPF_FREE;
		} else {
			p->left[page_back] = p->left[page_front];
			p->top[page_back] = p->top[page_front];
		}
	}
}

void near stage_scenery_particles_render(void)
{
	int i;
	stage_scenery_particle_t far *p = stage_scenery_particles;
	register screen_x_t left;
	register screen_y_t top;

	for(i = 0; i < STAGE_SCENERY_PARTICLE_COUNT; i++, p++) {
		if(p->flag != SSPF_ALIVE) {
			continue;
		}
		p->left[page_back] -= p->speed;
		p->top[page_back] += p->speed;
		left = p->left[page_back];
		top = p->top[page_back];

		if(left < 32) {
			p->flag = SSPF_REMOVE;
		} else if((top < 384) && (left < 416)) {
			top += scroll_line;
			if(top >= RES_Y) {
				top -= RES_Y;
			}
			dot_square_left = left;
			dot_square_top = top;
			grcg_dot_square_put(2);
		}
	}
	grcg_off();
}

extern "C" void far stage1_update_and_render(void)
{
	RGB8 color;

	if((stage1_scenery_frame < 999) && (stage1_scenery_frame != 0)) {
		stage1_scenery_frame++;
		color.c.g = ((stage1_scenery_frame / 16) + 144);
		color.c.b = ((stage1_scenery_frame / 8) + 144);
		Palettes[8].c.r = 176;
		Palettes[8].c.g = color.c.g;
		Palettes[8].c.b = color.c.b;

		color.c.r = (160 - (stage1_scenery_frame / 16));
		color.c.g = (160 - (stage1_scenery_frame / 16));
		color.c.b = (255 - (stage1_scenery_frame / 4));
		Palettes[13].c.r = color.c.r;
		Palettes[13].c.g = color.c.g;
		Palettes[13].c.b = color.c.b;

		if(color.c.r <= 0x80) {
			stage1_scenery_frame = 1000;
			grc_setclip(
				PLAYFIELD_LEFT, 0, PLAYFIELD_RIGHT, (RES_Y - 1)
			);
			Palettes[14].c.r = 224;
			Palettes[14].c.g = 192;
			Palettes[14].c.b = 176;
			spark_accel_x.v = -1;
			bg_flash_frame = 1;
		}
		palette_show();
		return;
	}

	if(stage1_scenery_frame >= 1000) {
		stage_bg_flash_update();
		stage1_scenery_frame++;
		if(stage1_scenery_frame > ((reduce_effects * 2) + 1002)) {
			stage_scenery_particle_add(
				((randring2_next16() % 640) + 48), 16
			);
			stage1_scenery_frame = 1000;
		}
		egc_off();
		grcg_setcolor(GC_RMW, 8);
		stage_scenery_particles_render();
	}
}
