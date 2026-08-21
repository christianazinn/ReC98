/// Stage 1 Boss - Orange: the fight's own update function
/// ------------------------------------------------------
/// (#included from th04/main_033.cpp. main_033_TEXT has no other C++
/// contribution, and TLINK lays a segment's contributions out in link order
/// with the root dump first, so this object lands at the segment's tail by
/// construction — which is where this function already was.
/// kb/codegen/0112 + 0114.)
///
/// orange_bg_render() and orange_fg_render() are th04/main/boss/bg.cpp and
/// th04/main/boss/render.cpp, both in main_01 and both different objects.

// Declared FAR here, and only here: th04/main/boss/bosses.hpp declares the
// same function `near`, which is what it is, and that header is deliberately
// not included. A near reference under this object's `-zPmain_03` frames its
// offset on main_03, and orange_bg_render() lives in main_01.
// kb/codegen/0162.
void pascal far orange_bg_render(void);
/// ---------

/// State
/// -----
// The two `boss_statebyte` slots Orange uses, spelled the way th04_main.asm's
// own `boss_statebyte_t` overlay already names them.
#define patterns_done     boss_statebyte[15]
#define pattern_num_prev  boss_statebyte[14]
/// -----

/// Constants
/// ---------
static const int ORANGE_HP = 3050;

// Orange hovers around this point during her last two patterns, nudged back
// with a one-pixel dead zone on each axis.
static const subpixel_t ORANGE_HOVER_X = TO_SP(192);
static const subpixel_t ORANGE_HOVER_Y = TO_SP(80);
/// ---------

// Nudges Orange back towards ([ORANGE_HOVER_X], [ORANGE_HOVER_Y]). Written as
// a macro rather than a function because the original inlines it into both of
// the phases that use it, in full, twice.
#define orange_hover() { \
	if(boss.pos.cur.x.v < (ORANGE_HOVER_X - TO_SP(1))) { \
		boss.pos.velocity.x.v = 24; \
	} else if(boss.pos.cur.x.v > (ORANGE_HOVER_X + TO_SP(1))) { \
		boss.pos.velocity.x.v = -24; \
	} \
	if(boss.pos.cur.y.v < (ORANGE_HOVER_Y - TO_SP(1))) { \
		boss.pos.velocity.y.v = 12; \
	} else if(boss.pos.cur.y.v > (ORANGE_HOVER_Y + TO_SP(1))) { \
		boss.pos.velocity.y.v = -12; \
	} \
	boss.pos.update_seg3(); \
}

/// Orange's patterns
/// -----------------
/// All seven of these sat directly above orange_update() in ZUN's object, and
/// every one of them is reached from it and from nowhere else.

// Included by four of the six patterns below, and the only thing that moves
// Orange during her fight: a 70-frame flight to a random point in the upper
// playfield, opened by a three-circle gather stack and closed by a shrinking
// circle, then 16 frames of standing still before the pattern may fire.
//
// The return value is that schedule: 0 while she is still travelling, 1 while
// the pattern must not fire yet, and 2 once it may.
static int near orange_195E4(void)
{
	subpixel_t x;
	subpixel_t y;

	gather_add_only_3stack((boss.phase_frame - 70), 7, 6);
	if(boss.phase_frame < 16) {
		return 1;
	}
	if(boss.phase_frame == 16) {
		x = (randring2_next16_mod(TO_SP(320)) + TO_SP(32));
		y = (randring2_next16_mod(TO_SP(96)) + TO_SP(64));
		boss.pos.velocity.x.v = ((x - boss.pos.cur.x.v) / TO_SP(4));
		boss.pos.velocity.y.v = ((y - boss.pos.cur.y.v) / TO_SP(4));
		gather_template.radius.v = TO_SP(96);
		gather_template.ring_points = 8;
	}
	// One chain and one trailing `return 0`, not two early ones: the
	// original emits the zero epilogue ONCE, at the end, and jumps to it from
	// both of the first two arms.
	if(boss.phase_frame < 70) {
		boss.pos.update_seg3();
	} else if(boss.phase_frame == 70) {
		circles_add_shrinking(boss.pos.cur.x.v, boss.pos.cur.y.v);
		circles_color = V_WHITE;
	} else if(boss.phase_frame < 86) {
		return 1;
	} else {
		return 2;
	}
	return 0;
}

// Two rings of two, from a starting angle that is either straight right or
// straight left, sweeping in the matching direction.
static void near orange_19686(void)
{
	if(orange_195E4() != 2) {
		return;
	}
	if(boss.phase_frame == 86) {
		boss.phase_state.patterns_seen = randring2_next16_and(1);
		boss.angle = ((boss.phase_state.patterns_seen == 0) ? 0x00 : 0x80);

		// …and the sweep direction is parked in the same byte the coin flip
		// came out of.
		boss.phase_state.patterns_seen = (
			(boss.phase_state.patterns_seen == 0) ? 0x0B : -0x0B
		);
	}
	if(stage_frame_mod2 == 0) {
		bullet_template.spawn_type = BST_PELLET;
		bullet_template.origin.x.v = boss.pos.cur.x.v;
		bullet_template.origin.y.v = boss.pos.cur.y.v;
		bullet_template.group = BG_RING;
		bullet_template.count = 2;
		bullet_template.angle = boss.angle;
		bullet_template.speed.v = (TO_SP(1) + 14);
		bullets_add_regular();
		bullet_template.angle += 5;
		bullet_template.speed.v = (TO_SP(1) + 4);
		bullets_add_regular();
		boss.angle += boss.phase_state.patterns_seen;
		snd_se_play(9);
	}
	if(boss.phase_frame >= 118) {
		boss.mode = 0;
	}
}

// Two aimed spreads of three, 0x40 apart, speeding up on every volley.
static void near orange_19720(void)
{
	if(orange_195E4() != 2) {
		return;
	}
	if(boss.phase_frame == 86) {
		boss.angle = iatan2(
			(player_pos.cur.y.v - boss.pos.cur.y.v),
			(player_pos.cur.x.v - boss.pos.cur.x.v)
		);
		bullet_template.speed.v = TO_SP(1);
	}
	if(stage_frame_mod4 == 0) {
		bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
		bullet_template.patnum = PAT_BULLET16_N_OUTLINED_BALL_WHITE;
		bullet_template.origin.x.v = boss.pos.cur.x.v;
		bullet_template.origin.y.v = boss.pos.cur.y.v;
		bullet_template.group = BG_SPREAD;
		bullet_template.count = 3;
		bullet_template.delta.spread_angle = 0x0C;
		bullet_template.special_motion = BSM_NONE;
		bullet_template.angle = (boss.angle - 0x20);
		bullet_template_tune();
		bullets_add_special();
		bullet_template.angle += 0x40;
		bullets_add_special();
		snd_se_play(3);
		bullet_template.speed.v += 6;
	}
	if(boss.phase_frame >= 118) {
		boss.mode = 0;
	}
}

// One aimed 16-bullet ring every 8 frames, and nothing else.
static void near orange_197BB(void)
{
	if(orange_195E4() != 2) {
		return;
	}
	if(stage_frame_mod8 == 0) {
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.patnum = PAT_BULLET16_N_OUTLINED_BALL_WHITE;
		bullet_template.origin.x.v = boss.pos.cur.x.v;
		bullet_template.origin.y.v = boss.pos.cur.y.v;
		bullet_template.group = BG_RING_AIMED;
		bullet_template.count = 16;
		bullet_template.speed.v = TO_SP(2);
		bullet_template.angle = 0;
		bullet_template_tune();
		bullets_add_regular();
		snd_se_play(9);
	}
	if(boss.phase_frame >= 118) {
		boss.mode = 0;
	}
}

// Two 8-bullet rings, one to each side of her, rotating by 8 every volley.
static void near orange_19814(void)
{
	if(orange_195E4() != 2) {
		return;
	}
	if(stage_frame_mod8 == 0) {
		bullet_template.spawn_type = BST_PELLET;
		bullet_template.origin.x.v = (boss.pos.cur.x.v - TO_SP(32));
		bullet_template.origin.y.v = boss.pos.cur.y.v;
		bullet_template.group = BG_RING;
		bullet_template.count = 8;
		bullet_template.speed.v = (TO_SP(1) + 14);
		bullet_template.angle += 8;
		bullet_template_tune();
		bullets_add_regular();
		bullet_template.origin.x.v += TO_SP(64);
		bullets_add_regular();
		snd_se_play(9);
	}
	if(boss.phase_frame >= 118) {
		boss.mode = 0;
	}
}

// Phase 3's one pattern, and the only one that does not use orange_195E4():
// she bounces between the playfield's sides on a sine-wave vertical, spraying
// randomly-angled bullets from either side of herself.
static void near orange_19878(void)
{
	if(boss.phase_frame == 1) {
		boss.pos.velocity.x.v = (
			(boss.pos.cur.x.v < TO_SP(192)) ? TO_SP(1) : -TO_SP(1)
		);
		boss.angle = 0;
	}
	boss.pos.velocity.y.v = polar(0, TO_SP(1), SinTable8[boss.angle]);
	if(boss.pos.cur.y.v >= TO_SP(96)) {
		boss.pos.velocity.y.v = -TO_SP(1);
	}
	if(boss.pos.cur.y.v <= TO_SP(48)) {
		boss.pos.velocity.y.v = TO_SP(1);
	}
	boss.angle += 2;

	// The returned X, which is what makes this an `unsigned` comparison.
	boss.pos.update_seg3();
	if((_AX <= TO_SP(32)) || (_AX >= TO_SP(352))) {
		// kb/codegen/0053's shape, the other way round: the constant goes
		// into AX and the memory word is the multiplicand, which is the
		// one-operand `IMUL`. A plain `-1 * x` folds to the three-operand
		// form instead.
		_AX = -1;
		_asm imul word ptr [boss+8]
		boss.pos.velocity.x.v = _AX;
	}

	if((stage_frame_mod4 == 0) && (
		(rank != RANK_EASY) || (stage_frame_mod8 != 0)
	)) {
		bullet_template.patnum = PAT_BULLET16_N_OUTLINED_BALL_WHITE;
		bullet_template.origin.x.v = (boss.pos.cur.x.v - TO_SP(32));
		bullet_template.origin.y.v = boss.pos.cur.y.v;
		bullet_template.group = BG_RANDOM_ANGLE;
		bullet_template.count = 1;
		if(boss.hp <= 700) {
			if(rank < RANK_LUNATIC) {
				bullet_template.count = 2;
			} else {
				bullet_template.count = 4;
			}
		}
		bullet_template.speed.v = TO_SP(2);

		// Each side independently picks pellets or 16×16 balls.
		bullet_template.spawn_type = ((randring2_next16_and(1) == 0)
			? BST_PELLET
			: BST_BULLET16_CLOUD_FORWARDS
		);
		bullet_template.angle = randring2_next16();
		bullet_template_tune();
		bullets_add_regular();

		bullet_template.origin.x.v += TO_SP(64);
		bullet_template.spawn_type = ((randring2_next16_and(1) == 0)
			? BST_PELLET
			: BST_BULLET16_CLOUD_FORWARDS
		);
		bullet_template.angle = randring2_next16();
		bullets_add_regular();
	}
}

// Phase 4's one pattern: a single stream that grows from one arm to six as the
// phase goes on, with a gather and a shrinking circle at four landmark frames
// each.
static void near orange_1998B(void)
{
	if(
		(boss.phase_frame == 96) || (boss.phase_frame == 160) ||
		(boss.phase_frame == 224) || (boss.phase_frame == 288)
	) {
		gather_template.center.x.v = bullet_template.origin.x.v;
		gather_template.center.y.v = bullet_template.origin.y.v;
		gather_add_only();
	}
	if(
		(boss.phase_frame == 112) || (boss.phase_frame == 160) ||
		(boss.phase_frame == 240) || (boss.phase_frame == 304)
	) {
		circles_add_shrinking(
			bullet_template.origin.x.v, bullet_template.origin.y.v
		);
		circles_color = V_WHITE;
	}
	if(stage_frame_mod4 != 0) {
		return;
	}
	bullet_template.spawn_type = BST_PELLET;
	bullet_template.group = BG_SINGLE;
	boss.angle -= 7;
	bullet_template.speed.v = TO_SP(2);
	bullet_template.angle = boss.angle;
	bullet_template_tune();
	bullets_add_regular_fixedspeed();
	if(boss.phase_frame < 128) {
		return;
	}
	if(boss.phase_frame >= 192) {
		if(boss.phase_frame >= 256) {
			// A `goto` and not a nested `if`, because the original emits this
			// arm AFTER the three shorter ones and jumps forward into it --
			// `jge`, where an inline block would be `jl` around it.
			if(boss.phase_frame >= 320) {
				goto six_arms;
			}
			bullet_template.angle += 0x40;
			bullets_add_regular_fixedspeed();
		}
		bullet_template.angle += 0x40;
		bullets_add_regular_fixedspeed();
	}
	bullet_template.angle += 0x40;
	bullets_add_regular_fixedspeed();
	return;

six_arms:
	bullet_template.angle += 0x40;
	bullets_add_regular_fixedspeed();
	bullet_template.angle += 0x40;
	bullets_add_regular_fixedspeed();
	bullet_template.angle += 0x40;
	bullets_add_regular_fixedspeed();

	// The last two arms are a different bullet, faster and mirrored.
	bullet_template.speed.v += TO_SP(1);
	bullet_template.spawn_type = BST_BULLET16;
	bullet_template.patnum = PAT_BULLET16_N_OUTLINED_BALL_WHITE;
	bullet_template.angle = (-bullet_template.angle - 0x20);
	bullets_add_regular_fixedspeed();
	bullet_template.angle += 0x80;
	bullets_add_regular_fixedspeed();
}
/// -----------------

void pascal far orange_update(void)
{
	int i;

	gather_template.center.x.v = boss.pos.cur.x.v;
	gather_template.center.y.v = boss.pos.cur.y.v;

	switch(boss.phase) {
	case 0:
		if(boss.phase_frame == 0) {
			boss.hp = ORANGE_HP;
			boss.phase_end_hp = 1950;
			Palettes[0].c.r = 0;
			Palettes[0].c.g = 0;
			Palettes[0].c.b = 96;
			palette_changed = true;
		}
		boss.phase_frame++;
		if(boss.phase_frame == 192) {
			// The entrance gather: one huge ring that shrinks onto her for
			// the rest of the phase.
			boss.sprite += 2;
			snd_se_play(8);
			gather_template.center.x.v = (boss.pos.cur.x.v + TO_SP(8));
			gather_template.center.y.v = (boss.pos.cur.y.v - TO_SP(40));
			gather_template.radius.v = TO_SP(320);
			gather_template.ring_points = 32;
			gather_template.angle_delta = 3;
			gather_template.col = 7;
		} else if(boss.phase_frame > 320) {
			if(boss.phase_frame == 336) {
				gather_template.col = 6;
			}
			if((boss.phase_frame & 7) == 0) {
				gather_add_only();
			}
			if(boss.phase_frame >= 352) {
				boss.phase++;
				boss.phase_frame = 0;
				snd_se_play(13);
				patterns_done = 0;
				pattern_num_prev = -1;
				_asm mov word ptr bg_render_bombing_func, offset orange_bg_render
				tiles_bb_col = 0;
			}
		}
		boss_hittest_shots_damage(TO_SP(16), TO_SP(16), 10);
		break;

	case 1:
		boss.phase_frame++;
		if(boss.phase_frame >= 32) {
			// Back to the small gather rings the patterns use…
			gather_template.radius.v = TO_SP(64);
			gather_template.angle_delta = 2;
			gather_template.ring_points = 8;

			boss.phase = 2;
			boss.phase_frame = 0;
			boss.mode = 0;
			boss.sprite += 2;

			// …and one three-ring opening volley, each ring slower than the
			// one before it.
			bullet_template.spawn_type = BST_PELLET;
			bullet_template.origin.x.v = boss.pos.cur.x.v;
			bullet_template.origin.y.v = boss.pos.cur.y.v;
			bullet_template.group = BG_RING;
			bullet_template.count = 16;
			bullet_template.speed.v = TO_SP(4);
			bullet_template.angle = 0;
			bullet_template_tune();
			for(i = 0; i < 3; i++) {
				bullets_add_regular();
				bullet_template.speed.v -= TO_SP(1);
			}
			snd_se_play(6);
		}
		boss_hittest_shots_damage(TO_SP(16), TO_SP(16), 10);
		break;

	case 2:
		switch(boss.mode) {
		case 0:
			boss.phase_frame = 0;

			// Rerolled until it differs from the last one.
			do {
				// kb/codegen/0032: the original increments the returned byte
				// in AL and stores it, where `and(3) + 1` widens to `INC AX`.
				_AL = randring2_next16_and(3);
				_AL++;
				boss.mode = _AL;
			} while(pattern_num_prev == boss.mode);
			pattern_num_prev = boss.mode;

			patterns_done++;
			if(patterns_done >= 16) {
				goto phase_2_over;
			}
			break;
		case 1:
			orange_19686();
			break;
		case 2:
			orange_19720();
			break;
		case 3:
			orange_197BB();
			break;
		case 4:
			orange_19814();
			break;
		}
		if(!boss_hittest_shots()) {
			break;
		}
		boss_score_bonus(5);
phase_2_over:
		boss_phase_next(ET_NW_SE, 450);
		Palettes[0].c.r = 112;
		Palettes[0].c.b = 112;
		palette_changed = true;
		break;

	case 3:
		switch(boss.mode) {
		case 0:
			if(boss.phase_frame > 128) {
				boss.phase_frame = 0;
				boss.mode = 1;
			}
			break;
		case 1:
			orange_19878();
			break;
		}
		if(boss.phase_frame <= 1500) {
			if(!boss_hittest_shots()) {
				break;
			}
			boss_score_bonus(5);
		}
		boss_phase_next(ET_NW_SE, 0);
		boss.sprite += 4;
		Palettes[0].c.r = 144;
		Palettes[0].c.b = 32;
		palette_changed = true;
		gather_template.col = 9;
		break;

	case 4:
		bullet_template.origin.x.v = (boss.pos.cur.x.v + TO_SP(8));
		bullet_template.origin.y.v = (boss.pos.cur.y.v - TO_SP(16));
		switch(boss.mode) {
		case 0:
			if(boss.phase_frame == 96) {
				gather_template.center.x.v = bullet_template.origin.x.v;
				gather_template.center.y.v = bullet_template.origin.y.v;
				gather_add_only();
			}
			if(boss.phase_frame == 112) {
				circles_add_shrinking(
					bullet_template.origin.x.v, bullet_template.origin.y.v
				);
				circles_color = V_WHITE;
			}
			if(boss.phase_frame > 128) {
				boss.phase_frame = 0;
				boss.mode = 1;
			}
			orange_hover();
			break;
		case 1:
			orange_1998B();
			break;
		}
		if(boss.phase_frame <= 600) {
			if(!boss_hittest_shots()) {
				break;
			}
		}

		// The defeat bonus is the one thing that distinguishes killing Orange
		// from surviving her: the timeout takes the same branch.
		if(boss.phase_frame <= 600) {
			boss.phase_state.defeat_bonus = true;
		} else {
			boss.phase_state.defeat_bonus = false;
		}
		boss_explode_small(ET_HORIZONTAL);
		boss.phase++;
		boss.phase_frame = 0;
		boss.mode = 0;
		sparks_add_circle(
			boss.pos.cur.x, boss.pos.cur.y, TO_SP(8), 48
		);
		break;

	case 5:
		orange_hover();
		boss.phase_frame++;
		if(boss.phase_frame == 16) {
			boss_explode_small(ET_VERTICAL);
		}
		if(boss.phase_frame == 32) {
			boss_defeat_explode_big(ET_CIRCLE, 10);
			snd_se_play(12);

			// Only two of the three components, like Marisa's.
			Palettes[0].c.r = 0;
			Palettes[0].c.b = 0;
			palette_changed = true;
			player_invincibility_time = BOSS_DEFEAT_INVINCIBILITY_FRAMES;
		}
		break;

	default:
		boss_defeat_update();
		return;
	}

	homing_target.x.v = boss.pos.cur.x.v;
	homing_target.y.v = boss.pos.cur.y.v;
	hud_hp_update_and_render(boss.hp, ORANGE_HP);
}
/// ------------------------------------------------------
