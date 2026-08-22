/// Stage 4 Boss - Yuki, solo
/// -------------------------
/// mai_update()'s mirror image, and the other half of th05_main.asm's
/// `setfarfp _boss_update` pair in mai_yuki_update(): this one runs when *Mai*
/// was defeated first, which is the branch that also copies [yuki.pos.cur]
/// into [boss] and loads _DM09.TX2. Whichever of the pair survives arrives in
/// [boss], so both functions drive [boss] and both share
/// [mai_yuki_pattern]; what differs is the cel block (PAT_YUKI rather than
/// PAT_MAI), 7900 HP rather than 7800, PAT_B4BALL_FIRE rather than
/// PAT_B4BALL_SNOW, and the phases that carry patterns -- 3, 5 and 9 here
/// against Mai's 3, 7 and 9.
///
/// (#included from th05/b4mai.cpp, ahead of th05/main/boss/b4_mai.cpp, which
/// owns the shared unguarded headers for the whole object (kb/codegen/0112
/// trap 0). This body was the last `proc` of th05_main.asm's contribution to
/// main_035_TEXT once Mai's block was lifted out from under it, and that object
/// is the segment's next contribution, so the lift lands exactly where the
/// root's block ended -- kb/codegen 0112 + 0114, no carve, no new segment, no
/// Tupfile.lua line.)
///
/// The object's `-a2` pad arithmetic differs from Mai's and is the one thing to
/// get right here: this body is 0x326 bytes, EVEN, so its own jump table lands
/// on an even object offset and takes NO pad -- which is what the original has.
/// It also shifts every table behind it by 0x33A, an even amount, so
/// mai_update()'s pad is unaffected (kb/codegen 0154 + 0157 + 0160).

// Yuki's HP thresholds, each spelled after the phase it ends, the way
// th05/main/boss/b4_mai.cpp spells Mai's.
static const int YUKI_HP_TOTAL = 7900;
static const int YUKI_HP_PHASE_3_END = 4600;
static const int YUKI_HP_PHASE_5_END = 3200;
static const int YUKI_HP_PHASE_7_END = 1200;
static const int YUKI_HP_PHASE_9_END = 0;

// [inferred] Yuki's half of the Stage 4 sprite block mirrors Mai's cel for
// cel: th05/sprites/main_pat.h names only Mai's first three (PAT_B4_STILL,
// PAT_B4_RIGHT, PAT_B4_LEFT, at PAT_MAI + 0..2) and this function assigns
// PAT_YUKI + 0..2 in exactly the same three places. The +8..+10 triple is the
// second still/right/left set, used from the solo fight onwards.
static const int PAT_YUKI_STILL = (PAT_YUKI + 0);
static const int PAT_YUKI_RIGHT = (PAT_YUKI + 1);
static const int PAT_YUKI_LEFT = (PAT_YUKI + 2);
static const int PAT_YUKI_SOLO_STILL = (PAT_YUKI + 8);
static const int PAT_YUKI_SOLO_RIGHT = (PAT_YUKI + 9);
static const int PAT_YUKI_SOLO_LEFT = (PAT_YUKI + 10);

// What this file still reaches in th05_main.asm
// --------------------------------------------

// Yuki's three pattern tables, the twins of MAI_PATTERNS_PHASE_3/_7/_9 in
// th05/main/boss/b4_mai.cpp and named on the same formula. PHASE_9 is the odd
// one out at FIVE entries, indexed `% 5` rather than `& 1`, and four of the
// five are the same function -- which is what sizes it, not the code.
extern "C" const pattern_oneshot_func_t YUKI_PATTERNS_PHASE_3[2];
extern "C" const pattern_oneshot_func_t YUKI_PATTERNS_PHASE_5[2];
extern "C" const pattern_oneshot_func_t YUKI_PATTERNS_PHASE_9[5];

// The first of Yuki's seven pattern bodies, and the only one left in
// th05_main.asm: it is 209 bytes, an ODD length, so it cannot be prepended to
// this object on its own without moving both jump tables' pads. It pairs with
// b4balls_update() above it (273 bytes) for a parcel that sums even.
// Address-suffixed for the reason th05/main/boss/b4_mai.cpp gives for
// mai_1BD2C() and its siblings.
extern "C" bool near yuki_1B557(void);

// --------------------------------------------

// [inferred] PAT_YUKI + 12, the one cel of Yuki's block that
// th05/main/boss/b4_solo_fg.cpp animates over four frames -- the twin of
// PAT_MAI_ANIMATED in th05/main/boss/b4_mai.cpp. Every one of the six patterns
// below switches her to it, and b4_solo_fg.cpp spells the same number 208.
static const int PAT_YUKI_ANIMATED = (PAT_YUKI + 12);

/// Danmaku patterns
/// ----------------
/// Six of Yuki's seven, in their original address order, which is also the
/// order YUKI_PATTERNS_PHASE_5 and _9 index them in. All are
/// pattern_oneshot_func_t; yuki_1B973() is the exception that never returns
/// `true`, exactly as Mai's phase-5 laser pattern never does, because its
/// phase ends on the HP check in yuki_update() instead.
///
/// Four of the six close with the same tail: an exact [boss.phase_frame]
/// equality that hands the phase back, and then a random flight step that only
/// starts once the phase has seen a few patterns. **Note the odd one out** --
/// yuki_1B8C8() gates that step on `> 2` where every other body in both halves
/// of this fight uses `>= 2`.
/// ----------------

// Phase 3, pattern B: one random-width, random-speed spread every other frame,
// walking around by 6 units a shot.
bool near yuki_1B628(void)
{
	if(boss.phase_frame == 8) {
		bullet_template.angle = 0x80;
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.group = BG_SPREAD;
		boss.sprite = PAT_YUKI_ANIMATED;
		bullet_template.patnum = PAT_BULLET16_N_BALL_RED;
	} else if(boss.phase_frame > 8) {
		if((boss.phase_frame % 2) == 0) {
			bullet_template.speed.v = (randring2_next16_and(0x1F) + 8);
			bullet_template.spread = randring2_next16_and(7);
			bullet_template.spread_angle_delta = 6;
			bullet_template_tune();
			bullets_add_regular();
			bullet_template.angle += 6;
			snd_se_play(3);
		}
		if(boss.phase_frame == 256) {
			boss.phase_frame = 0;
			boss.mode = 0;
			return true;
		}
		if(
			(boss.phase_state.patterns_seen >= 2) && (boss.phase_frame >= 64)
		) {
			boss_flystep_random(boss.phase_frame % 64);
		}
	}
	return false;
}

// Phase 5, pattern A: one 24-pellet ring every 16 frames, at a random angle
// and a random speed.
bool near yuki_1B6C4(void)
{
	if(boss.phase_frame == 8) {
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.group = BG_RING;
		bullet_template.patnum = 0;
		bullet_template.spread = 24;
		bullet_template_tune();
		boss.sprite = PAT_YUKI_ANIMATED;
	} else if(boss.phase_frame > 8) {
		if((boss.phase_frame % 16) == 0) {
			bullet_template.angle = randring2_next16();
			bullet_template.speed.v = (
				randring2_next16_and(0x1F) + to_sp8(2.0f)
			);
			bullets_add_regular();
			snd_se_play(15);
		}
		if(boss.phase_frame == 160) {
			boss.phase_frame = 0;
			boss.mode = 0;
			return true;
		}
		if(
			(boss.phase_state.patterns_seen >= 2) && (boss.phase_frame >= 64)
		) {
			boss_flystep_random((boss.phase_frame % 64) - 32);
		}
	}
	return false;
}

// Phase 5, pattern B: an aimed 5-bullet spread plus a five-ball fan every 32
// frames, behind a gathering animation.
bool near yuki_1B754(void)
{
	if(boss.phase_frame < 32) {
		gather_add_only_3stack((boss.phase_frame - 16), 9, 8);
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.group = BG_SPREAD_AIMED;
		bullet_template.patnum = PAT_BULLET16_N_BALL_RED;
		bullet_template.set_spread(5, 12);
		bullet_template.angle = 0x00;
		bullet_template_tune();
		boss.sprite = PAT_YUKI_ANIMATED;
		b4ball_template.speed.set(4.0f);
	} else {
		if((boss.phase_frame % 32) == 0) {
			bullet_template.speed.v = (
				randring2_next16_and(0x1F) + to_sp8(1.0f)
			);
			bullets_add_regular();
			b4ball_template.angle = player_angle_from(
				b4ball_template.origin.x, b4ball_template.origin.y
			);
			b4balls_add();
			b4ball_template.angle -= 12;
			b4balls_add();
			b4ball_template.angle -= 12;
			b4balls_add();
			b4ball_template.angle += 36;
			b4balls_add();
			b4ball_template.angle += 12;
			b4balls_add();
			snd_se_play(15);
		}
		if(boss.phase_frame == 160) {
			boss.phase_frame = 0;
			boss.mode = 0;
			return true;
		}
		if(
			(boss.phase_state.patterns_seen >= 2) && (boss.phase_frame >= 64)
		) {
			boss_flystep_random((boss.phase_frame % 64) - 32);
		}
	}
	return false;
}

// Phase 9, pattern A: a four-way ball cross every 4 frames, the whole cross
// rotating by [boss_statebyte[13]] a time and reversing when the pattern ends.
bool near yuki_1B832(void)
{
	if(boss.phase_frame < 32) {
		gather_add_only_3stack((boss.phase_frame - 16), 9, 8);
		boss.sprite = PAT_YUKI_ANIMATED;
		b4ball_template.speed.set(2.0f);
	} else {
		if(boss.phase_frame == 32) {
			snd_se_play(15);
		}
		if((boss.phase_frame % 4) == 0) {
			b4balls_add();
			b4ball_template.angle += 0x40;
			b4balls_add();
			b4ball_template.angle += 0x40;
			b4balls_add();
			b4ball_template.angle += 0x40;
			b4balls_add();
			b4ball_template.angle += 0x40;
			b4ball_template.angle += boss_statebyte[13];
		}
		if(boss.phase_frame == 64) {
			boss.phase_frame = 0;
			boss.mode = 0;
			boss_statebyte[13] = -boss_statebyte[13];
			return true;
		}
	}
	return false;
}

// Phase 9, pattern B, and four of YUKI_PATTERNS_PHASE_9's five entries: one
// stack every other frame, sweeping in a direction that reverses each run.
bool near yuki_1B8C8(void)
{
	if(boss.phase_frame == 24) {
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.group = BG_STACK;
		bullet_template.patnum = PAT_BULLET16_V_RED;
		bullet_template.set_stack(8, 0.375f);
		bullet_template.angle = 0x10;
		bullet_template.speed.set(2.0f);
		bullet_template_tune();
		boss.sprite = PAT_YUKI_ANIMATED;
	} else if(boss.phase_frame >= 24) {
		if((boss.phase_frame % 2) == 0) {
			bullet_template.angle = boss_statebyte[15];
			bullets_add_regular();
			boss_statebyte[15] += boss_statebyte[14];
			snd_se_play(15);
		}
		if(boss.phase_frame == 64) {
			boss_statebyte[14] = -boss_statebyte[14];
			if(boss_statebyte[14] < 0x7F) {
				boss_statebyte[15] = 0x10;
			} else {
				boss_statebyte[15] = 0x70;
			}
			boss.phase_frame = 0;
			boss.mode = 0;
			return true;
		}

		// [measured] `> 2`, where every other pattern body in this fight uses
		// `>= 2` in the same position. Preserved as ZUN wrote it: the effect is
		// that this one pattern's random flight step only starts one pattern
		// later than the others'.
		if(boss.phase_state.patterns_seen > 2) {
			boss_flystep_random((boss.phase_frame % 64) - 25);
		}
	}
	return false;
}

// Phase 7: an accelerating random-angle cloud every 4 frames, with the flight
// step changing shape halfway through. The one Yuki pattern that never returns
// `true` -- yuki_update()'s own HP check ends this phase.
bool near yuki_1B973(void)
{
	if(boss.phase_frame == 8) {
		bullet_template.spawn_type = BST_NO_DECELERATE;
		bullet_template.group = BG_RANDOM_ANGLE;
		bullet_template.special_motion = BSM_SPEEDUP;
		bullet_special.speed_delta.v = 1;
		bullet_template.patnum = PAT_BULLET16_V_RED;
		bullet_template.spread = ((rank * 2) + 4);
		boss.sprite = PAT_YUKI_ANIMATED;
		bullet_template.speed.set(2.0f);
	} else if(boss.phase_frame > 8) {
		if((boss.phase_frame % 4) == 0) {
			bullets_add_special();
		}
		if(boss.phase_frame >= 128) {
			if(boss.phase_frame < 320) {
				boss_flystep_random((boss.phase_frame % 64) - 32);
			} else {
				boss_flystep_random(boss.phase_frame % 64);
			}
		}
	}
	return false;
}
/// ----------------

#pragma option -a2

// `extern "C"` + `pascal`: the module published the undecorated upper-case
// `YUKI_UPDATE`, and th05_main.asm's mai_yuki_update() resolves its
// `setfarfp _boss_update` against that spelling (kb/codegen 0081 + 0102).
extern "C" void pascal yuki_update(void)
{
	homing_target = boss.pos.cur;
	bullet_template.origin = boss.pos.cur;
	gather_template.center = boss.pos.cur;
	b4ball_template.origin = boss.pos.cur;
	boss.phase_frame++;

	switch(boss.phase) {
	case PHASE_HP_FILL:
		if(boss.phase_frame == 1) {
			boss.hp = YUKI_HP_TOTAL;
			boss.phase_end_hp = YUKI_HP_PHASE_3_END;
			gather_template.radius.set(BOSS_W / 1.0f);
			gather_template.angle_delta = 0x02;
			gather_template.ring_points = 8;
			boss.sprite = PAT_YUKI_STILL;
			boss_sprite_left = PAT_YUKI_LEFT;
			boss_sprite_right = PAT_YUKI_RIGHT;
			boss_sprite_stay = PAT_YUKI_STILL;
			b4ball_template.patnum_tiny_base = PAT_B4BALL_FIRE;
		}
		boss_hittest_shots_invincible();

		// Timeout condition
		if(boss.phase_frame >= 64) {
			// Next phase
			boss.phase++;
			boss.phase_frame = 0;
			snd_se_play(13);
			bg_render_bombing_func = mai_yuki_bg_render;
		}
		break;

	case PHASE_BOSS_ENTRANCE_BB:
		boss_hittest_shots_invincible();

		// Timeout condition
		if(boss.phase_frame >= 64) {
			// Next phase
			boss.sprite = PAT_YUKI_SOLO_STILL;
			boss.phase++;
			boss.phase_frame = 0;
			boss_custombullets_render = b4balls_render;
		}
		break;

	case 2:
		boss_hittest_shots_invincible();
		if(boss_flystep_towards(to_sp(PLAYFIELD_W / 2), to_sp(96.0f))) {
			// Next phase
			boss.sprite = PAT_YUKI_SOLO_STILL;
			boss.phase++;
			boss.phase_frame = 0;
			boss.mode = 1;
			boss.phase_state.patterns_seen = 0;

			// [inferred] The flight target for the phase below, one boss
			// height under the point Yuki is standing at now. [yuki] is the
			// same structure as [boss2], so this is writing to the slot the
			// defeated Mai left -- not to Yuki's own.
			yuki.pos.cur.x.v = boss.pos.cur.x.v;
			yuki.pos.cur.y.v = (boss.pos.cur.y.v + to_sp(16.0f));

			circles_add_growing(boss.pos.cur.x, boss.pos.cur.y);
			boss_explode_small(ET_VERTICAL);
			mai_yuki_pattern = yuki_1B557;
			boss_sprite_left = PAT_YUKI_SOLO_LEFT;
			boss_sprite_right = PAT_YUKI_SOLO_RIGHT;
			boss_sprite_stay = PAT_YUKI_SOLO_STILL;
		}
		break;

	case 3:
		switch(boss.mode) {
		case 0:
			if(boss_flystep_towards(yuki.pos.cur.x, yuki.pos.cur.y)) {
				yuki.pos.cur.x.v = boss.pos.cur.x.v;
				yuki.pos.cur.y.v = to_sp(96.0f);
				boss.phase_frame = 0;
				boss.mode++;
				boss.phase_state.patterns_seen++;

				// Timeout condition
				if(boss.phase_state.patterns_seen >= 12) {
					goto phase_3_timed_out;
				}
				mai_yuki_pattern = YUKI_PATTERNS_PHASE_3[
					boss.phase_state.patterns_seen & 1
				];
			}
			break;

		case 1:
			mai_yuki_pattern();
			break;
		}
		if(boss_hittest_shots()) {
			boss_score_bonus(10);
phase_3_timed_out:
			// Next phase
			boss_phase_next(ET_NW_SE, YUKI_HP_PHASE_5_END);
		}
		break;

	case 4:
		boss_hittest_shots();
		if(boss_flystep_towards(to_sp(PLAYFIELD_W / 2), to_sp(96.0f))) {
			// Next phase
			boss.phase++;
			boss.phase_frame = 0;
			mai_yuki_pattern = yuki_1B6C4;
			boss.mode = 1;
		}
		break;

	case 5:
		switch(boss.mode) {
		case 0:
			if(boss_flystep_towards(boss.pos.cur.x, to_sp(96.0f))) {
				boss.phase_frame = 0;
				boss.mode++;
				boss.phase_state.patterns_seen++;

				// Timeout condition
				if(boss.phase_state.patterns_seen >= 24) {
					goto phase_5_timed_out;
				}
				mai_yuki_pattern = YUKI_PATTERNS_PHASE_5[
					boss.phase_state.patterns_seen & 1
				];
			}
			break;

		case 1:
			mai_yuki_pattern();
			break;
		}
		if(boss_hittest_shots()) {
			boss_score_bonus(10);
phase_5_timed_out:
			// Next phase
			boss_phase_next(ET_SW_NE, YUKI_HP_PHASE_7_END);
		}
		break;

	case 6:
		boss_hittest_shots();
		if(boss_flystep_towards(to_sp(PLAYFIELD_W / 2), to_sp(96.0f))) {
			// Next phase
			boss.phase++;
			boss.phase_frame = 0;
		}
		break;

	case 7:
		yuki_1B973();
		if(boss.phase_frame < 2000) {
			if(!boss_hittest_shots()) {
				break;
			}
			boss_score_bonus(10);
		}

		// Next phase
		boss_phase_next(ET_HORIZONTAL, YUKI_HP_PHASE_9_END);
		break;

	case 8:
		boss_hittest_shots();
		if(boss_flystep_towards(to_sp(PLAYFIELD_W / 2), to_sp(96.0f))) {
			// Next phase
			boss.phase++;
			boss.phase_frame = 0;
			mai_yuki_pattern = yuki_1B832;
			boss.mode = 1;

			// [inferred] Read by the phase-9 patterns, which are still
			// assembly; left as raw indices for the reason
			// th05/main/boss/b5.cpp gives for Yumeko's.
			boss_statebyte[14] = 8;
			boss_statebyte[15] = 0x10;
			boss_statebyte[13] = -8;
		}
		break;

	case 9:
		switch(boss.mode) {
		case 0:
			// The only pattern phase in either of the pair's update functions
			// that advances on a plain frame count rather than on a flight
			// step returning true.
			if(boss.phase_frame >= 8) {
				boss.phase_frame = 0;
				boss.mode++;
				boss.phase_state.patterns_seen++;

				// Timeout condition
				if(boss.phase_state.patterns_seen >= 36) {
					boss.phase_state.defeat_bonus = false;
					goto yuki_defeated;
				}
				mai_yuki_pattern = YUKI_PATTERNS_PHASE_9[
					boss.phase_state.patterns_seen % 5
				];
			}
			break;

		case 1:
			mai_yuki_pattern();
			break;
		}
		if(boss_hittest_shots()) {
			boss.phase_state.defeat_bonus = true;
yuki_defeated:
			boss_explode_small(ET_VERTICAL);
			boss.phase_frame = 0;
			boss.phase = PHASE_BOSS_EXPLODE_SMALL;
			b4balls_reset();
			boss_custombullets_render = nullfunc_near;
		}
		break;

	default:
		boss_defeat_update(50);
		return;
	}

	b4balls_update();
	hud_hp_update_and_render(boss.hp, YUKI_HP_TOTAL);
}

#pragma option -a1
