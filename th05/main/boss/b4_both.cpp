/// Stage 4 Boss - Mai & Yuki (both)
/// --------------------------------

#pragma option -zCB4_UPDATE_TEXT -zPmain_03

#include "th04/math/vector.hpp"
#include "th04/math/randring.hpp"
#include "th05/sprites/main_pat.h"
#include "th05/main/boss/boss.hpp"
#include "th05/main/boss/impl.hpp"

#define mai boss
#define yuki boss2

extern y_direction_t mai_flystep_random_next_y_direction;
extern y_direction_t yuki_flystep_random_next_y_direction;

// Still ZUN's assembly in th05_main.asm's B4_UPDATE_TEXT, reached through the
// zero-byte `public` alias in front of the dump's own label (kb/codegen/0081:
// `extern "C"` + `pascal` mangles to the all-uppercase undecorated name, so
// `public MAI_YUKI_1A3EF` over a lowercase `proc` costs no bytes). Left at its
// IDA placeholder spelling on purpose: it is NOT a tail — main_035_TEXT calls
// it twice more — so it cannot be lifted alongside the function below, and
// this campaign does not name a body it is not lifting.
//
// It is yuki's boss_hittest_shots_damage(): same three parameters and same
// return, but it sets [shot_hitbox_center] from [yuki.pos.cur] explicitly (the
// shape midboss_hittest_shots_damage() uses) and never calls
// boss_hittest_player().
extern "C" int pascal near mai_yuki_1A3EF(
	subpixel_t radius_x, subpixel_t radius_y, int se_on_hit
);

// Game logic
// ----------

/// The two-boss shot hittest
/// -------------------------
/// Mai and Yuki are hittable at the same time and end their phases
/// independently, so this cannot return a `bool` the way boss_hittest_shots()
/// does. It returns *which* of the two ended its phase this frame — 0 for
/// neither, 1 for Mai, 2 for Yuki — and its one caller subtracts 1 to get the
/// [boss2_phase_state] that selects the explosion. [inferred from that call
/// site; the dump left this function unnamed]
///
/// Mai goes through the shared boss_hittest_shots(), because `mai` IS [boss].
/// Yuki needs the separate helper above for the same work against [boss2].
/// The hitbox radius is the (W/2) - (W/8) idiom that TH05's Extra midboss also
/// uses — 24 of BOSS_W's 64, NOT `BOSS_W / 2`, which would be 32.

unsigned char near mai_yuki_hittest_shots(void)
{
	if(boss_hittest_shots()) {
		return 1;
	}
	yuki.damage_this_frame = mai_yuki_1A3EF(
		to_sp((BOSS_W / 2) - (BOSS_W / 8)),
		to_sp((BOSS_W / 2) - (BOSS_W / 8)),
		4
	);
	yuki.hp -= yuki.damage_this_frame;
	if(yuki.hp <= yuki.phase_end_hp) {
		return 2;
	}
	return 0;
}
/// -------------------------

// See boss_flystep_random().
bool pascal near flystep_random(
	boss_stuff_t near &boss, int frame, y_direction_t near &next_y_direction
)
{
	flystep_random_for(
		boss,
		next_y_direction,
		6.25f,
		48,
		to_sp(BOSS_W / 2),
		to_sp(PLAYFIELD_W - (BOSS_W / 2)),
		to_sp(24.0f),
		to_sp(128.0f),
		PAT_B4_LEFT,
		PAT_B4_RIGHT,
		PAT_B4_STILL,
		frame
	);
}

bool pascal near mai_yuki_flystep_random(int frame)
{
	/*  */ flystep_random(mai,  frame, mai_flystep_random_next_y_direction);
	return flystep_random(yuki, frame, yuki_flystep_random_next_y_direction);
}
// ----------
