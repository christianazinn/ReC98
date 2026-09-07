#ifndef TH01_BPRSTATE_HPP
#define TH01_BPRSTATE_HPP
#include "platform.h"

// C-linkage views of existing storage exported in the three owner CPP files.
// These declarations allocate nothing. Layout/order is the owner's, not wire.
struct t1bp_hit_t { bool16 invincible; int16_t frame; };
struct t1bp_el_hit_t { int16_t frame; bool16 invincible; };
struct t1bp_el_phase_t { int16_t pattern; bool16 teleport_done; };
struct t1bp_sar_phase_t { int16_t pattern; int16_t done; int16_t until_next; };
struct t1bp_kon_phase_t { int16_t pattern; int16_t done; };
extern "C" {
	extern int16_t t1bp_el_form;
	extern t1bp_el_hit_t t1bp_el_hit;
	extern t1bp_el_phase_t t1bp_el_phase;
	extern int16_t t1bp_el_vx;
	extern int16_t t1bp_el_vy;
	extern bool t1bp_el_hp_done;
	extern int16_t t1bp_el_pattern;
	extern bool16 t1bp_sar_invincible;
	extern int16_t t1bp_sar_hit_frame;
	extern int16_t t1bp_sar_ring;
	extern bool t1bp_sar_hp_done;
	extern t1bp_sar_phase_t t1bp_sar_phase;
	extern int16_t t1bp_sar_pattern;
	extern t1bp_hit_t t1bp_kon_hit;
	extern int16_t t1bp_kon_prev;
	extern bool t1bp_kon_hp_done;
	extern t1bp_kon_phase_t t1bp_kon_phase;
	extern int16_t t1bp_kon_pattern;
}
#endif
