/// Extra Stage midboss, update function
/// ------------------------------------
/// TWO bodies, like th04/main/midboss/mx.cpp's renderer, but they share
/// nothing: TH04's Extra midboss is a scripted fly-by in ENM_POS_TEXT, TH05's
/// is a two-phase danmaku fight in BX_UPDATE_TEXT. Only TH05's is decompiled
/// so far; TH04's keeps the `#else` branch free for the parcel that lifts it.

#if (GAME == 5)

// This translation unit is shared with th05/main/boss/bx.cpp (kb/codegen/0112),
// so every header below must be included exactly once across both files.
// th04/main/gather.hpp brings in th04/main/bullet/bullet.hpp, and
// th04/main/boss/boss.hpp brings in th04/main/phase.hpp; neither of those two
// has an include guard, so neither may be listed here as well.
#include "th04/snd/snd.h"
#include "th04/main/pattern.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/gather.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/spark.hpp"
#include "th04/main/item/item.hpp"
#include "th04/main/midboss/midboss.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/main/hud/hud.hpp"
#include "th05/sprites/main_pat.h"

// Constants
// ---------

static const pixel_t MIDBOSSX_W = 64;
static const pixel_t MIDBOSSX_H = 64;
static const int HP_TOTAL = 3000;
// ---------

// Still ZUN's assembly in th05_main.asm's BX_UPDATE_TEXT, reached through the
// zero-byte `public` aliases in front of the dump's own labels
// (kb/codegen/0123). All five are deliberately left at their IDA placeholder
// spellings: this campaign only *defers to* names that already exist for the
// same routine elsewhere, and none of these has one. Naming them is a review
// round's call, and the evidence each one would be named from is written out
// below and in state/notes/_midbossx_update_qv.md.
// -----

extern "C" {

// [frame] is relative to the start of the current phase.
//
// Sets [midboss.sprite] from [frame] (221 while [frame] < 0 or during the
// 30…39 window, 222/223 alternating during the 0…29 approach, 220 from 40 on),
// steps [midboss.pos] along [midboss.angle] at a [frame]-dependent speed for
// [frame]s 0…29, and plays se 8 on [frame] 0 and se 15 on [frame] 30. From
// [frame] 40 on it calls the current pattern and returns *its* result; every
// earlier path returns false.
bool pascal near sub_1E556(int frame);

// The pattern that phase 1 starts from. Its address is taken here and stored
// into [sub_1E556]'s callback; the body stays in the dump.
bool near sub_1E60E(void);

}

// The pattern [sub_1E556] calls once the approach is over. Initialised to
// sub_1E5FC in the dump's own data.
extern pattern_oneshot_func_t off_2285A;

// The pattern picked on every phase-1 cycle, indexed by [boss_statebyte[12]]
// (0 before the score bonus below, 1 after) and by the low bit of the cycle
// counter [boss_statebyte[14]]. Same shape as SHINKI_PATTERNS_PHASE_2_3 in
// th05/main/boss/b6.cpp — but note that the column index is spelled `% 2`
// rather than `& 1`, because [boss_statebyte] promotes to a *signed* int and
// only the remainder emits the original's `cwd`/`idiv` pair.
extern const pattern_oneshot_func_t off_2285C[2][2];

// [midboss.angle] for each of the first 8 phase-1 cycles, indexed by
// [boss_statebyte[14]] & 7. Cycle 8 onwards would read 0 twice before the
// timeout condition ends the fight at cycle 20 — except that the fight is
// unwinnable that long, see below.
extern const unsigned char byte_22868[8];
// -----

// [boss_statebyte] slots used here. Not #defined to names: [12] is both the
// "score bonus already given" latch and the pattern table's row index, [14] is
// both the angle table's index and the cycle counter the timeout condition
// reads, and [15] is written (0 in phase 0, 1 on every phase-1 cycle) but
// never read in this function or anywhere else in BX_UPDATE_TEXT.

void pascal midbossx_update(void)
{
	bullet_template.origin = midboss.pos.cur;
	gather_template.center = midboss.pos.cur;

	midboss.phase_frame++;

	switch(midboss.phase) {
	case 0:
		midboss_hittest_shots_invincible(
			TO_SP((MIDBOSSX_W / 2) - (MIDBOSSX_W / 8)),
			TO_SP((MIDBOSSX_H / 2) - (MIDBOSSX_H / 8))
		);
		midboss.angle = 0x40;
		if(sub_1E556(midboss.phase_frame)) {
			midboss.phase++;
			midboss.phase_frame = 0;
			midboss.angle = 0x00;
			boss_statebyte[15] = 0;
			boss_statebyte[14] = 0;
			boss_statebyte[12] = 0;
			off_2285A = sub_1E60E;
		}
		break;

	case 1:
		if(sub_1E556(midboss.phase_frame - 64)) {
			if((boss_statebyte[12] == 0) && (midboss.hp < 1000)) {
				midboss_score_bonus(10);
				bullets_clear();
				snd_se_play(15);
				boss_statebyte[12]++;
			}
			boss_statebyte[15] = 1;
			midboss.phase_frame = 0;
			midboss.angle = byte_22868[boss_statebyte[14] & 7];
			boss_statebyte[14]++;
			off_2285A = off_2285C[boss_statebyte[12]][boss_statebyte[14] % 2];
		}
		midboss_hittest_shots(
			TO_SP((MIDBOSSX_W / 2) - (MIDBOSSX_W / 8)),
			TO_SP((MIDBOSSX_H / 2) - (MIDBOSSX_H / 8))
		);

		// Timeout condition
		if(boss_statebyte[14] < 20) {
			if(midboss.hp > 0) {
				break;
			}
			bullet_zap.active = true;
			midboss_score_bonus(30);
			items_add(midboss.pos.cur.x, midboss.pos.cur.y, IT_1UP);
		}
		midboss.phase = PHASE_EXPLODE_BIG;
		midboss.sprite = PAT_ENEMY_KILL;
		midboss.phase_frame = 0;
		sparks_add_circle(
			midboss.pos.cur.x, midboss.pos.cur.y, TO_SP(MIDBOSSX_W / 8), 48
		);
		snd_se_play(12);
		break;

	default:
		midboss_defeat_update();
		hud_hp_update_and_render(midboss.hp, HP_TOTAL);
		return;
	}

	hud_hp_update_and_render(midboss.hp, HP_TOTAL);
	homing_target.x = midboss.pos.cur.x;
	homing_target.y = midboss.pos.cur.y;
}

#endif
