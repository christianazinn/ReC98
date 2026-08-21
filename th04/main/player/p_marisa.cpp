/// Marisa's shot control functions, shared parts and levels 0-1
/// ------------------------------------------------------------
/// The head of what th04_main.asm contributed to EXECL_TEXT: the two levels
/// that are the same for shottype A and B (SHOT_FUNCS_MARISA_A and
/// SHOT_FUNCS_MARISA_B both point their first two entries here), and the
/// option-laser starter that her eight shottype-A levels call. Both levels
/// are byte-for-byte the same construct as Reimu's
/// shot_reimu_l0() and shot_reimu_l1() in th04/main/player/shot_reimu.cpp,
/// with Marisa's sprite bank substituted -- the two shottypes only diverge
/// from level 2 onwards.
///
/// (#included from th04/p_marisa.cpp, the object that occupies the
/// kb/codegen/0080 P_MARISA_TEXT anchor carved off the HEAD of EXECL_TEXT's
/// root contribution. That anchor sits at the segment's HEAD where
/// th04/player_b.cpp's sits at its tail, so each further lift out of the
/// surviving EXECL_TEXT block APPENDS to the BOTTOM of this file rather than
/// prepending. kb/codegen 0080 + 0099 + 0114.)

#include "th03/math/randring.hpp"
#include "th04/main/player/shot.hpp"
#include "th04/sprites/main_pat.h"

extern "C" {

void pascal near shot_marisa_l0(void)
{
	Shot near *shot;

	shot_ptr = shots;
	shot_last_id = 0;
	if(( shot = shots_add() ) != nullptr) {
		shot->patnum_base = PAT_SHOT_MARISA;
		shot->damage = 10;
	}
}

void pascal near shot_marisa_l1(void)
{
	Shot near *shot;

	shot_ptr = shots;
	shot_last_id = 0;
	if(( shot = shots_add() ) != nullptr) {
		shot_velocity_set(
			&shot->pos.velocity, randring1_next8_ge_lt(-0x44, -0x3C)
		);
		shot->patnum_base = PAT_SHOT_MARISA;
		shot->damage = 10;
	}
}

}

// Starts the option laser if it is not already out, and spawns the pair of
// 16x16 ring shots that ride on top of it, one per option, on four out of
// every eight frames the laser is up. Called by Marisa's eight SHOTTYPE A
// levels and by nothing else in either game -- `[measured]` by a census, before
// the lift, of every call to it anywhere in th04_main.asm: eight, all inside
// shot_marisa_a_l2..a_l9. Shottype B never starts a laser. NOT `extern "C"`:
// the dump's own `public` spelled the C++-mangled name, so this definition
// has to stay mangled too.
void pascal near shot_laser_update(
	unsigned int frames, shot_laser_style_t style
)
{
	Shot near *shot;

	if(shot_laser_time == 0) {
		shot_laser_time = frames;
		shot_laser_style = style;
		// Four separate 16-bit copies, not two whole-point ones: under `-3`
		// Turbo C++ fuses an 8-byte-aligned 2x16-bit struct assignment into
		// `mov eax` / `mov dword ptr` and saves 12 bytes the original does not
		// save. Measured on the OBJ, kb/codegen/0160's instrument.
		shot_laser_bottomcenter.cur.x.v = player_option_pos_cur.x.v;
		shot_laser_bottomcenter.cur.y.v = player_option_pos_cur.y.v;
		shot_laser_bottomcenter.prev.x.v = player_option_pos_cur.x.v;
		shot_laser_bottomcenter.prev.y.v = player_option_pos_cur.y.v;
		shot_laser_ring_cycle = 0;
	}
	if(shot_laser_time >= (SHOT_LASER_COOLDOWN_FRAMES + 16)) {
		shot_laser_ring_cycle++;
		if(shot_laser_ring_cycle <= 4) {
			shot_ptr = shots;
			shot_last_id = 0;
			if(( shot = shots_add() ) != nullptr) {
				shot->patnum_base = PAT_SHOT_LASER_RING;
				shot->damage = 9;
				shot->pos.velocity.y.set(-18.0f);
				shot->pos.cur.x.v = (
					player_option_pos_cur.x.v - TO_SP(PLAYER_OPTION_DISTANCE)
				);
			}
			if(( shot = shots_add() ) != nullptr) {
				shot->patnum_base = PAT_SHOT_LASER_RING;
				shot->damage = 9;
				shot->pos.velocity.y.set(-18.0f);
				shot->pos.cur.x.v = (
					player_option_pos_cur.x.v + TO_SP(PLAYER_OPTION_DISTANCE)
				);
			}
		} else if(shot_laser_ring_cycle >= 8) {
			shot_laser_ring_cycle = 0;
		}
	}
}
