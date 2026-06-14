#pragma codeseg mainl_03_TEXT group_01

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "pc98.h"
#include "x86real.h"

extern int staffroll_frame;
extern bool staffroll_cdg_put_alpha;
extern bool staffroll_cdg_alpha;
extern int staffroll_cdg_speed;
extern int staffroll_cdg_frames;
extern screen_y_t staffroll_cdg_y_stop;
extern screen_y_t staffroll_cdg_y_start_off;
extern unsigned char staffroll_cdg_aux_slot;
extern page_t page_back;
extern screen_y_t stf_center_y_on_page[PAGE_COUNT];

void near staffroll_blue_plane_clear(void);
void near staffroll_flakes_tick(void);
bool16 pascal near staffroll_phase_done(
	uint16_t measure_threshold, int frame_threshold
);

extern "C" {
void pascal near cdg_unput_for_upwards_motion_e_8(
	screen_x_t center_x, vram_y_t center_y, int slot
);
void pascal near cdg_put_dissolve_e_8(
	screen_x_t center_x, vram_y_t center_y, int slot, int strength
);
}

void pascal near staffroll_cdg_slide_up(int slot_arg)
{
	#define slot    	_DI
	#define strength	_SI

	slot = slot_arg;
	staffroll_blue_plane_clear();
	if(staffroll_frame <= staffroll_cdg_frames) {
		cdg_unput_for_upwards_motion_e_8(
			(RES_X / 2), stf_center_y_on_page[page_back], slot
		);
		if(stf_center_y_on_page[page_back] > staffroll_cdg_y_stop) {
			stf_center_y_on_page[page_back] -= staffroll_cdg_speed;
			if(stf_center_y_on_page[page_back] < staffroll_cdg_y_stop) {
				stf_center_y_on_page[page_back] = staffroll_cdg_y_stop;
			}
		}

		strength = (7 - (staffroll_frame / (staffroll_cdg_frames / 8)));
		if(static_cast<int16_t>(strength) < 0) {
			strength = 0;
		}
		if(staffroll_cdg_put_alpha) {
			if(staffroll_cdg_aux_slot != 0) {
				cdg_put_dissolve_e_8(504, 200, staffroll_cdg_aux_slot, strength);
				staffroll_cdg_alpha = true;
			}
		}
		cdg_put_dissolve_e_8(
			(RES_X / 2), stf_center_y_on_page[page_back], slot, strength
		);
		staffroll_cdg_alpha = false;
	}

	#undef strength
	#undef slot
}

void pascal near staffroll_cdg_slide_out(int slot_arg)
{
	#define slot    	_DI
	#define strength	_SI

	slot = slot_arg;
	staffroll_blue_plane_clear();
	if(staffroll_frame <= 0xA1) {
		if(staffroll_frame < 0xA0) {
			cdg_unput_for_upwards_motion_e_8(
				(RES_X / 2), stf_center_y_on_page[page_back], slot
			);
			stf_center_y_on_page[page_back]--;
			strength = (staffroll_frame / 20);
			if(static_cast<int16_t>(strength) > 7) {
				strength = 7;
			}
			if(staffroll_cdg_put_alpha) {
				if(staffroll_cdg_aux_slot != 0) {
					cdg_put_dissolve_e_8(504, 200, staffroll_cdg_aux_slot, strength);
					staffroll_cdg_alpha = true;
				}
			}
			cdg_put_dissolve_e_8(
				(RES_X / 2), stf_center_y_on_page[page_back], slot, strength
			);
			staffroll_cdg_alpha = false;
		} else {
			grcg_setcolor(GC_RMW, 0);
			grcg_byteboxfill_x(
				(8 / 8), 8, ((RES_X - 1 - 8) / 8), (RES_Y - 1 - 8)
			);
			grcg_off();
		}
	}

	#undef strength
	#undef slot
}

void pascal near staffroll_cdg_dissolve_in(
	int slot, screen_y_t center_y, screen_x_t center_x
)
{
	#define strength _SI

	if(staffroll_frame <= 0xA0) {
		strength = (7 - (staffroll_frame / 20));
		if(static_cast<int16_t>(strength) < 0) {
			strength = 0;
		}
		cdg_put_dissolve_e_8(slot, center_y, center_x, strength);
	}

	#undef strength
}

void pascal near staffroll_cdg_slide_cycle(
	int slot_arg, int frame_threshold_in, int frame_threshold_out
)
{
	#define slot _SI

	slot = slot_arg;
	if(staffroll_cdg_speed == 2) {
		stf_center_y_on_page[page_back] = 264;
		stf_center_y_on_page[page_back ^ 1] = 263;
	} else {
		stf_center_y_on_page[page_back] = (280 - staffroll_cdg_y_start_off);
		stf_center_y_on_page[page_back ^ 1] = (
			280 - staffroll_cdg_y_start_off
		);
	}

	staffroll_frame = 0;
	do {
		staffroll_cdg_slide_up(slot);
		staffroll_flakes_tick();
		staffroll_frame++;
	} while(!staffroll_phase_done(frame_threshold_in, 0x100));

	staffroll_frame = 0;
	do {
		staffroll_cdg_slide_out(slot);
		staffroll_flakes_tick();
		staffroll_frame++;
	} while(!staffroll_phase_done(frame_threshold_out, 0x100));

	grcg_setcolor(GC_RMW, 0);
	grcg_byteboxfill_x((8 / 8), 8, ((RES_X - 1 - 8) / 8), (RES_Y - 1 - 8));
	grcg_off();
	staffroll_flakes_tick();
	grcg_setcolor(GC_RMW, 0);
	grcg_byteboxfill_x((8 / 8), 8, ((RES_X - 1 - 8) / 8), (RES_Y - 1 - 8));
	grcg_off();
	staffroll_flakes_tick();

	#undef slot
}

#pragma codeseg
