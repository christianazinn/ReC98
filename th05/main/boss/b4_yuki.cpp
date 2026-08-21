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

// The four of Yuki's seven pattern bodies this function names: three whose
// address it stores and one it calls. The other three are reached only from
// YUKI_PATTERNS_PHASE_5 and _9 and stay unnamed data. Address-suffixed for the
// reason th05/main/boss/b4_mai.cpp gives for mai_1BD2C() and its siblings, and
// each name goes away with the body when the chain above this one is lifted.
extern "C" bool near yuki_1B557(void);
extern "C" bool near yuki_1B6C4(void);
extern "C" bool near yuki_1B832(void);
extern "C" bool near yuki_1B973(void);

// --------------------------------------------

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
