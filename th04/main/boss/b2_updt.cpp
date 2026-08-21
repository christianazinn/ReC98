/// Stage 2 Boss - Kurumi: the fight's own update function
/// ------------------------------------------------------
/// (#included from th04/main_033.cpp, AHEAD of th04/main/boss/b1_updt.cpp,
/// which is this function's original address order: ZUN's object for
/// main_033_TEXT held Kurumi's fight and then Orange's.)

/// Still ASM
/// ---------
// Kurumi's patterns and the two helpers they share, all of them in this same
// segment and all private to ZUN's object, so each needed a zero-byte `label`
// alias in th04_main.asm to become linkable (kb/codegen/0123). The
// address-suffixed names are the dump's own; naming them belongs to whoever
// lifts them.
extern "C" {
	// Runs on every frame of the two phases that let Kurumi move freely.
	void near kurumi_18A79(void);

	// The between-patterns filler both pattern phases open with.
	void near kurumi_18B68(void);

	// Phase 2's three patterns…
	void near kurumi_18BE6(void);
	void near kurumi_18C76(void);
	void near kurumi_18D04(void);

	// …phase 3's one…
	void near kurumi_18DB6(void);

	// …phase 4's three…
	void near kurumi_18E43(void);
	void near kurumi_18EE7(void);
	void near kurumi_18F8B(void);

}

// Written 0 exactly once, here, and read by nothing in any of the five
// binaries. ZUN bloat, and the reason it keeps an address-suffixed name.
// **A naming round is owed** if a reader ever turns up.
extern "C" unsigned char kurumi_259F1;

// Declared FAR here, and only here: th04/main/boss/bosses.hpp declares the
// same function `near`, which is what it is, and that header is deliberately
// not included by this object. A near reference under `-zPmain_03` frames its
// offset on main_03, and kurumi_bg_render() lives in main_01.
// kb/codegen/0162.
void pascal far kurumi_bg_render(void);
/// ---------

/// Constants
/// ---------
static const int KURUMI_HP = 4800;

// The number of pattern cycles each of the two pattern phases runs before it
// ends on its own.
static const int KURUMI_CYCLES_MAX = 10;

// Cap on the number of arms the ring volley between two patterns fires.
static const int KURUMI_VOLLEY_ARMS_MAX = 5;
/// ---------

// Byte-for-byte the same nudge Orange's phases 4 and 5 use
// (th04/main/boss/b1_updt.cpp), down to the one-pixel dead zone and both
// speeds — but the two files are two objects' worth of ZUN source, so it is
// written out in each rather than shared.
#define kurumi_hover() { \
	if(boss.pos.cur.x.v < TO_SP(191)) { \
		boss.pos.velocity.x.v = 24; \
	} else if(boss.pos.cur.x.v > TO_SP(193)) { \
		boss.pos.velocity.x.v = -24; \
	} \
	if(boss.pos.cur.y.v < TO_SP(79)) { \
		boss.pos.velocity.y.v = 12; \
	} else if(boss.pos.cur.y.v > TO_SP(81)) { \
		boss.pos.velocity.y.v = -12; \
	} \
	boss.pos.update_seg3(); \
}

// The ring volley that opens every pattern cycle: up to five nested rings of
// 22, each one faster and rotated against the one before it.
#define kurumi_volley(angle_delta) { \
	bullet_template.spawn_type = BST_PELLET; \
	bullet_template.origin.x.v = boss.pos.cur.x.v; \
	bullet_template.origin.y.v = boss.pos.cur.y.v; \
	bullet_template.patnum = PAT_BULLET16_N_OUTLINED_BALL_BLUE; \
	bullet_template.group = BG_RING; \
	bullet_template.count = 22; \
	bullet_template.angle = randring2_next16(); \
	bullet_template_tune(); \
	bullet_template.speed.v = TO_SP(1); \
	arms = boss.phase_state.patterns_seen; \
	if(arms > KURUMI_VOLLEY_ARMS_MAX) { \
		arms = KURUMI_VOLLEY_ARMS_MAX; \
	} \
	i = 0; \
	while(i < arms) { \
		bullets_add_regular_fixedspeed(); \
		i++; \
		bullet_template.speed.v += 8; \
		bullet_template.angle += angle_delta; \
	} \
}

/// Kurumi's phase 5 pattern
/// ------------------------
// The four `boss_statebyte` slots it uses, spelled the way th04_main.asm's own
// `boss_statebyte_t` overlay already names them.
#define stack_right_angle  boss_statebyte[15]
#define stack_left_angle   boss_statebyte[14]
#define stacks_fired       boss_statebyte[13]
#define spread_interval    boss_statebyte[0]

// A re-seed of the two stack angles, which happens on the pattern's first
// frame and again after every tenth stack.
#define kurumi_stacks_reseed() { \
	stack_right_angle = (-0x20 - randring2_next16_and(0xF)); \
	/* kb/codegen/0032: the original ADDS the negative constant to the */ \
	/* returned byte in AL, where `- 0x60` is a `SUB` of the positive one. */ \
	_AL = randring2_next16_and(0xF); \
	_AL += -0x60; \
	stack_left_angle = _AL; \
	stacks_fired = 0; \
}

// Two streams of forward-cloud bullets from either side of her, their angles
// walking apart by 0x10 every eighth frame, plus an aimed five-spread on an
// interval the rank picks.
static void near kurumi_1905A(void)
{
	if(boss.phase_frame < 16) {
		return;
	}
	if(boss.phase_frame == 16) {
		kurumi_stacks_reseed();
		bullet_template.patnum = PAT_BULLET16_D_BLUE;
	}
	if((boss.phase_frame % 8) == 0) {
		bullet_template.speed.v = TO_SP(1);
		stack_right_angle += 0x10;
		stack_left_angle -= 0x10;
		stacks_fired++;
		if(stacks_fired > 10) {
			kurumi_stacks_reseed();
		}
		snd_se_play(15);
	}
	bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
	bullet_template.group = BG_SINGLE;
	bullet_template.origin.y.v = (boss.pos.cur.y.v - TO_SP(10));
	bullet_template.origin.x.v = (boss.pos.cur.x.v + TO_SP(12));
	bullet_template.angle = stack_right_angle;
	bullets_add_regular_fixedspeed();
	bullet_template.origin.x.v -= TO_SP(24);
	bullet_template.angle = stack_left_angle;
	bullets_add_regular_fixedspeed();
	bullet_template.speed.v += 10;

	if((boss.phase_frame % spread_interval) == 0) {
		bullet_template.group = BG_SPREAD_AIMED;
		bullet_template.count = 5;
		bullet_template.delta.spread_angle = 9;
		bullet_template.angle = 0;
		bullet_template.spawn_type = BST_PELLET;

		// kb/codegen/0032: the original adds the minimum to the returned byte
		// in AL and then stores it.
		_AL = randring2_next16_and(0xF);
		_AL += TO_SP(2);
		bullet_template.speed.v = _AL;

		bullet_template.special_motion = BSM_NONE;
		bullet_template_tune();
		bullets_add_special();
	}
}
/// ------------------------

// `#pragma option -a2` is the one padding byte between this function's
// epilogue and the first of its three generated jump tables. kb/codegen/0160
// for the instrument -- read the OBJ's PUBDEF offsets, never the `tcc -S`
// listing. `[measured]` at a zero prefix, which is what this function being
// the first thing the object emits gives it: 0x487 to the next function with
// `-a2`, 0x486 without.
#pragma option -a2
void pascal far kurumi_update(void)
{
	kurumi_spawnray_t near *ray;
	int i;
	int arms;

	switch(boss.phase) {
	case 0:
		if(boss.phase_frame == 0) {
			boss.hp = KURUMI_HP;
			boss.phase_end_hp = KURUMI_HP;
			Palettes[0].c.r = 96;
			Palettes[0].c.g = 0;
			Palettes[0].c.b = 0;
			palette_changed = true;

			ray = kurumi_spawnrays;
			i = 0;
			while(i < KURUMI_SPAWNRAY_COUNT) {
				ray->flag = B2SF_FREE;
				i++;
				ray++;
			}

			// The entrance gather: one huge ring that shrinks onto her over
			// the rest of the phase, turning the other way from Orange's.
			gather_template.center.x.v = boss.pos.cur.x.v;
			gather_template.center.y.v = boss.pos.cur.y.v;
			gather_template.ring_points = 32;
			gather_template.radius.v = TO_SP(320);
			gather_template.angle_delta = -3;
			gather_template.col = V_WHITE;
		} else if(boss.phase_frame >= 288) {
			if(boss.phase_frame == 296) {
				gather_template.col = 9;
			}
			if((boss.phase_frame & 7) == 0) {
				gather_add_only();
			}
			if(boss.phase_frame >= 320) {
				boss.phase++;
				boss.phase_frame = 0;
				snd_se_play(13);
				kurumi_259F1 = 0;
				_asm mov word ptr bg_render_bombing_func, offset kurumi_bg_render
				tiles_bb_col = 0;
			}
		} else if(boss.phase_frame == 128) {
			snd_se_play(8);
		}
		boss_hittest_shots_invincible();
		break;

	case 1:
		boss_hittest_shots_invincible();
		if(boss.phase_frame >= 32) {
			boss_phase_next(ET_NONE, 3300);
			boss.mode = 3;
			boss.angle = 192;
			snd_se_play(6);
		}
		break;

	case 2:
		switch(boss.mode) {
		case 0:
			kurumi_18B68();
			if(boss.phase_frame >= 96) {
				boss.phase_frame = 0;

				// kb/codegen/0032: the original increments the returned byte
				// in AL and stores it, where `and(3) + 1` widens to `INC AX`.
				_AL = randring2_next16_and(3);
				_AL++;
				boss.mode = _AL;

				boss.phase_state.patterns_seen++;
				if(boss.phase_state.patterns_seen > KURUMI_CYCLES_MAX) {
					goto phase_2_over;
				}
				kurumi_volley(-3);
			}
			break;
		case 1:
			kurumi_18BE6();
			break;
		case 2:
			kurumi_18C76();
			break;
		case 3: case 4:
			kurumi_18D04();
			break;
		}
		if(!boss_hittest_shots()) {
			break;
		}
		boss_score_bonus(10);
phase_2_over:
		boss_phase_next(ET_NW_SE, 2050);
		bullet_template_special_angle.turn_by = 0x40;
		break;

	case 3:
		kurumi_18A79();
		switch(boss.mode) {
		case 0:
			if(boss.phase_frame >= 128) {
				boss.phase_frame = 0;
				boss.mode = 1;
			}
			break;
		case 1:
			kurumi_18DB6();
			break;
		}
		if(boss.phase_frame <= 2000) {
			if(!boss_hittest_shots()) {
				break;
			}
			boss_score_bonus(10);
		}
		boss_phase_next(ET_SW_NE, 550);
		break;

	case 4:
		switch(boss.mode) {
		case 0:
			kurumi_18B68();
			if(boss.phase_frame >= 96) {
				boss.phase_frame = 0;

				// …and `_mod(3)` here where phase 2 has `_and(3)`, which is
				// what keeps this phase off its own fourth pattern.
				_AL = randring2_next16_mod(3);
				_AL++;
				boss.mode = _AL;

				boss.phase_state.patterns_seen++;
				if(boss.phase_state.patterns_seen > KURUMI_CYCLES_MAX) {
					goto phase_4_over;
				}
				kurumi_volley(3);
			}
			break;
		case 1:
			kurumi_18E43();
			break;
		case 2:
			kurumi_18EE7();
			break;
		case 3:
			kurumi_18F8B();
			break;
		}
		if(!boss_hittest_shots()) {
			break;
		}
		boss_score_bonus(10);
phase_4_over:
		boss_phase_next(ET_HORIZONTAL, 0);
		break;

	case 5:
		kurumi_18A79();
		switch(boss.mode) {
		case 0:
			if(boss.phase_frame == 144) {
				circles_add_shrinking(
					boss.pos.cur.x.v, boss.pos.cur.y.v
				);
				circles_color = V_WHITE;
			}
			if(boss.phase_frame > 64) {
				boss.phase_frame = 0;
				boss.mode = 1;
				boss.sprite = 12;
				boss.angle = 128;
			}
			kurumi_hover();
			break;
		case 1:
			kurumi_1905A();
			break;
		}
		if(boss_hittest_shots() || (boss.phase_frame >= 700)) {
			boss.phase++;
			sparks_add_circle(
				boss.pos.cur.x, boss.pos.cur.y, TO_SP(8), 48
			);
			boss_explode_small(ET_VERTICAL);

			// The defeat bonus is the one thing that distinguishes killing
			// Kurumi from surviving her, and its threshold is 100 frames
			// BELOW the one that ends the phase.
			if(boss.phase_frame < 600) {
				boss.phase_state.defeat_bonus = true;
			} else {
				boss.phase_state.defeat_bonus = false;
			}
			boss.phase_frame = 0;
		}
		break;

	case 6:
		kurumi_18A79();
		kurumi_hover();
		boss.phase_frame++;
		if(boss.phase_frame == 16) {
			boss_explode_small(ET_VERTICAL);
		}
		if(boss.phase_frame == 32) {
			boss_defeat_explode_big(ET_CIRCLE, 20);
			snd_se_play(12);

			// Only the red component, unlike every other TH04 boss.
			Palettes[0].c.r = 0;
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
	hud_hp_update_and_render(boss.hp, KURUMI_HP);
}
#pragma option -a1
/// ------------------------------------------------------
