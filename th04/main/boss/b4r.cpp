/// Stage 4 Boss - Reimu
/// --------------------

#include "th04/main/boss/b4r.hpp"

#define ORB_W        	REIMU_ORB_W
#define ORB_H        	REIMU_ORB_H
#define ORB_COUNT    	REIMU_ORB_COUNT
#define orb_flag_t   	reimu_orb_flag_t
#define orb_t        	reimu_orb_t
#define orbs         	reimu_orbs

// State
// -----

unsigned char reimu_pattern8_angle = 0x00;
int8_t reimu_bg_pulse_direction = false;
// -----

void pascal near orbs_add_moving(void)
;
void pascal near orbs_add_spinning(unsigned char angle_offset, int count)
;
