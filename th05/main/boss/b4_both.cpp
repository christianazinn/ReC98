/// Stage 4 Boss - Mai & Yuki (both)
/// --------------------------------
/// The `-zCB4_UPDATE_TEXT -zPmain_03` pragma this file used to carry now
/// lives in th05/boss_4.cpp, which compiles this file together with Alice's
/// update function (kb/codegen/0112 trap 0).

#include "th04/math/vector.hpp"
#include "th04/snd/snd.h"
// Also supplies th04/math/randring.hpp and th05/sprites/main_pat.h, which this
// file used to include directly. Neither has an include guard, so each can
// only come from one place; naming them here as well is a compile error, not a
// no-op. th04/main/boss/boss.cpp reaches both the same way.
#include "th04/main/player/shot.hpp"
// th05/main/boss/boss.hpp is no longer named here either: th05/boss_4.cpp now
// compiles th05/main/boss/b3.cpp ahead of this file, and that one reaches the
// header through th05/main/boss/bosses.hpp (kb/codegen 0129).
#include "th05/main/boss/impl.hpp"

#define mai boss
#define yuki boss2

extern y_direction_t mai_flystep_random_next_y_direction;
extern y_direction_t yuki_flystep_random_next_y_direction;

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
/// Yuki needs the separate function below for the same work against [boss2].
/// The hitbox radius is the (W/2) - (W/8) idiom that TH05's Extra midboss also
/// uses — 24 of BOSS_W's 64, NOT `BOSS_W / 2`, which would be 32.

// Yuki's boss_hittest_shots_damage(): the same three parameters, the same
// return value, and the same body apart from two differences that both follow
// from `yuki` being [boss2] rather than [boss]. It sets [shot_hitbox_center]
// from [yuki.pos.cur] — the shape midboss_hittest_shots_damage() uses — and it
// never calls boss_hittest_player(), because the Mai half of the same frame's
// hittest already did. [inferred; the dump left this function unnamed]
// Also called twice from ZUN's remaining assembly in main_035_TEXT, with the
// invincibility sound effect.
int pascal near yuki_hittest_shots_damage(
	subpixel_t radius_x, subpixel_t radius_y, int se_on_hit
)
{
	shots_hittest_against_boss = true;
	int ret = shots_hittest(yuki.pos.cur, radius_x, radius_y);
	if(ret) {
		snd_se_play(se_on_hit);
	}
	shots_hittest_against_boss = false;
	return ret;
}

unsigned char near mai_yuki_hittest_shots(void)
{
	if(boss_hittest_shots()) {
		return 1;
	}
	yuki.damage_this_frame = yuki_hittest_shots_damage(
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
