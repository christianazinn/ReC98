/// Extra Stage Boss EX-Alice - the fight's own update function
/// -----------------------------------------------------------
/// The whole of th05_main.asm's contribution to main_036_TEXT: exalice_update()
/// and the three `cs:` jump tables its `switch` statements compile to. It is
/// #included from th05/exalice.cpp AFTER th05/main/boss/bx_updt.cpp and after
/// that file's `#pragma codeseg`, so the two share ONE object across two
/// segments -- which is what makes the four `exalice_hittest()` call sites 4
/// bytes rather than TLINK's 5 (kb/codegen/0116). That file's own header
/// explains the rest of the arrangement.
///
/// Everything this function calls in BX_TEXT is therefore already defined above
/// it in the same translation unit, and needs no declaration here. Only the
/// headers bx_updt.cpp does not already name, and the three cross-group
/// callbacks, are below.

#include "libs/master.lib/pc98_gfx.hpp"
#include "th03/hardware/palette.hpp"
#include "th04/main/bg.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/hud/hud.hpp"

// Constants
// ---------

// Also the maximum the HP bar is scaled against, which is why it is the one
// number of the phase ladder below worth a name.
static const int EXALICE_HP_TOTAL = 26500;
// ---------

// Cross-group callbacks
// ---------------------
// Three of the callbacks this function installs live in group main_01, not in
// this object's main_03, so the near pointer stored for each has to be framed
// on THEIR group -- which Turbo C++ only does for a target it believes is
// `far` (kb/codegen/0162). The honest `near` declarations are in
// th05/main/boss/bosses.hpp and th04/main/null.hpp, and BOTH headers are
// deliberately left out of this translation unit for that reason. Borland's
// mangling does not encode near/far, so each declaration below still reaches
// the one definition.
//
// The stores go through one line of inline asm each, because a far function
// does not assign to a near function pointer. None of them mentions a
// register, and this function's own `int i` is still allocated to SI --
// checked in the `tcc -S` listing, per kb/codegen 0009 + 0143.

void pascal far exalice_bg_render(void);
extern "C" void pascal far exalice_custombullets_render(void);
extern "C" void pascal far nullfunc_near(void);
// ---------------------

// th05/main/boss/bx.cpp, BX_UPDATE_TEXT rather than either of this object's
// two segments, but the same main_03 group. That file has no header of its own.
void near firewaves_update(void);

// Still ZUN's assembly, named here for this function's reads
// ---------------------------------------------------------

// th05/main/boss/bx_fg.cpp, the overlay's other reader, carries the note.
extern "C" unsigned int exalice_overlay_patnum;

// The table [exalice_pattern] is picked out of once per pattern: 4 rows of 2,
// indexed by [boss_statebyte][9] -- which counts the colour-fade phases
// survived -- and the parity of the patterns seen so far. Named on the formula
// state/re/NAMING_REVIEW_VERDICTS_19.md section 10.2 fixed for
// SARA_PATTERNS_PHASE_2_3 and MIDBOSSX_PATTERNS_PHASE_1, with no phase
// qualifier because this one table serves every pattern phase of the fight.
// Initialised storage at a fixed address in th05_main.asm's `_DATA`, which a
// definition here would move, so it stays there under a kb/codegen/0123 alias.
// [inferred] name.
extern "C" const pattern_oneshot_func_t EXALICE_PATTERNS[4][2];
// ---------------------------------------------------------

// pattern_oneshot_func_t returns `bool`, and three of the eight patterns this
// function selects return `int` -- which is a real difference in what they
// emit, not a slip, so it cannot be spelled away by changing a signature. The
// cast is the whole of the cost: the stored word is the same address either
// way.
#define AS_PATTERN(f) reinterpret_cast<pattern_oneshot_func_t>(f)

// EX-Alice's whole fight, and the longest phase ladder in the game: two
// invincible intro phases, then four groups of {pattern phase, fly to the
// centre, pattern phase with a colour fade} that the `switch` below reaches
// through duplicated `case` labels rather than through separate arms, and
// finally the fused pattern-and-movement phase that ends the game.
//
// Every HP threshold in it is written as a literal, because the ladder --
// 23800, 21000, 18100, 15100, 12600, 9600, 6800, 3400, 0 -- is a descending
// sequence whose only meaning is the sequence. [boss_statebyte][9] indexes the
// group, [10] is the last phase's own three-state colour fade, and both are
// left as raw indices for the reason th05/main/boss/b4_mai.cpp gives about
// naming a shared slot from one of its several users.
void pascal exalice_update(void)
{
	int i;

	if(exalice_invincibility_frames != 0) {
		exalice_invincibility_frames--;
	}
	homing_target = boss.pos.cur;
	bullet_template.origin = boss.pos.cur;
	gather_template.center = boss.pos.cur;
	laser_template.coords.origin = boss.pos.cur;
	// th05_main.asm spells this one `cheeto_template.pos.cur`, over its own
	// cheeto_head_t-shaped overlay of the same four bytes.
	cheeto_template.origin = boss.pos.cur;
	boss.phase_frame++;

	switch(boss.phase) {
	case PHASE_HP_FILL:
		if(boss.phase_frame == 1) {
			boss.hp = EXALICE_HP_TOTAL;
			boss.phase_end_hp = 23800;
			gather_template.radius.set(128.0f);
			gather_template.angle_delta = 0x02;
			gather_template.ring_points = 8;
			boss.sprite = 180;
			boss_sprite_left = 186;
			boss_sprite_right = 184;
			boss_sprite_stay = 180;
			exalice_invincibility_frames = 0;
			boss_flystep_random_clamp.left.v = to_sp(BOSS_W);
			boss_flystep_random_clamp.right.v = to_sp(PLAYFIELD_W - BOSS_W);
			boss_flystep_random_clamp.top.v = to_sp(48.0f);
			boss_flystep_random_clamp.bottom.v = to_sp(96.0f);
			for(i = 204; i < 220; i++) {
				super_convert_tiny(i);
			}
		}
		boss_hittest_shots_invincible();

		// Timeout condition
		if(boss.phase_frame >= 192) {
			// Next phase
			boss.phase_frame = 0;
			boss.phase++;
			snd_se_play(13);
			Palettes.colors[0].c.r = 0;
			Palettes.colors[0].c.g = 0;
			Palettes.colors[0].c.b = 0;
			palette_changed = true;
			exalice_overlay_patnum = 196;
			_asm {
				mov	word ptr bg_render_bombing_func, offset exalice_bg_render
			}
		}
		break;

	case PHASE_BOSS_ENTRANCE_BB:
		boss_hittest_shots_invincible();

		// Timeout condition
		if(boss.phase_frame >= 64) {
			// Next phase
			boss.phase++;
			boss.mode = 1;
			boss.phase_state.patterns_seen = 0;
			boss.phase_frame = 0;
			// One line, over 80 columns: Turbo C++'s `_asm` does not continue
			// across a line break.
			_asm { mov word ptr boss_custombullets_render, offset exalice_custombullets_render }
			boss_statebyte[15] = 0;
			exalice_pattern = AS_PATTERN(pattern_random_red_spreads);
			boss_statebyte[9] = 0;
		}
		break;

	case 2:
pattern_phase:
		switch(boss.mode) {
		case 0:
			if(boss_flystep_random(boss.phase_frame - 32)) {
				boss.phase_frame = 0;
				boss.phase_state.patterns_seen++;
				boss.mode++;
				exalice_pattern = EXALICE_PATTERNS[boss_statebyte[9]][
					boss.phase_state.patterns_seen & 1
				];

				// Timeout condition
				if(boss.phase_state.patterns_seen >= 32) {
					goto pattern_phase_next;
				}
			}
			break;

		// [measured] A second labelled arm rather than a default one, the way
		// th05/main/boss/b4_mai.cpp's phase 3 has it: a zero test, a compare
		// against 1, and a fallthrough jump.
		case 1:
			exalice_gather_and_pattern();
			break;
		}
		if(!exalice_hittest()) {
			break;
		}
		boss_score_bonus(20);
pattern_phase_next:
		switch(boss_statebyte[9]) {
		case 0:
			exalice_phase_next(ET_CIRCLE, 21000);
			exalice_pattern = pattern_spreads_and_firewaves;
			break;
		case 1:
			exalice_phase_next(ET_NW_SE, 15100);
			exalice_pattern = pattern_bouncing_blue_rings;
			break;
		case 2:
			exalice_phase_next(ET_SW_NE, 9600);
			exalice_pattern = pattern_pingpong_lasers;
			break;
		case 3:
			exalice_phase_next(ET_HORIZONTAL, 3400);
			exalice_pattern = pattern_mirrored_crosses;
			break;
		}
		break;

	case 3:
		if(Palettes.colors[0].c.r < 96) {
			Palettes.colors[0].c.r += 2;
			palette_changed = true;
		}
		// Fallthrough is ZUN's: phases 7, 11 and 15 are the same flight with
		// no colour left to fade.
	case 7:
	case 11:
	case 15:
flyto_phase:
		if(boss_flystep_towards(to_sp(PLAYFIELD_W / 2), to_sp(64.0f))) {
			// Next phase
			boss.phase_frame = 0;
			boss.phase++;
			boss.mode = 1;
		}
		exalice_hittest();
		break;

	case 4:
		if(Palettes.colors[0].c.r < 96) {
			Palettes.colors[0].c.r += 2;
			palette_changed = true;
		}
		// Fallthrough is ZUN's, as above.
	case 8:
	case 12:
		exalice_gather_and_pattern();

		// Timeout condition
		if(boss.phase_frame > 4000) {
			goto colored_phase_next;
		}
colored_phase_hittest:
		if(!exalice_hittest()) {
			break;
		}
		boss_score_bonus(20);
colored_phase_next:
		switch(boss_statebyte[9]) {
		case 0:
			exalice_phase_next(ET_NW_SE, 18100);
			exalice_pattern = AS_PATTERN(pattern_gravity_balls_and_stacks);
			break;
		case 1:
			exalice_phase_next(ET_SW_NE, 12600);
			exalice_pattern = pattern_lasers_and_green_rings;
			break;
		case 2:
			exalice_phase_next(ET_HORIZONTAL, 6800);
			exalice_pattern = AS_PATTERN(pattern_aimed_crosses);
			break;
		case 3:
			exalice_phase_next(ET_VERTICAL, 0);
			break;
		}
		pellet_bottom_col = 9;
		exalice_overlay_patnum = 200;
		boss_statebyte[9]++;
		break;

	// The three colour fades. Each runs for two phases: the FIRST of the pair
	// flies EX-Alice back to the centre, the second runs the pattern phase's
	// dispatch. The phase number is compared rather than [boss.mode] because
	// both arms of a fade are one `case` pair.
	case 5:
	case 6:
		if(Palettes.colors[0].c.b < 96) {
			Palettes.colors[0].c.b += 2;
			Palettes.colors[0].c.r += -2;
			palette_changed = true;
		}
		if(boss.phase == 5) {
			goto flyto_phase;
		}
		goto pattern_phase;

	case 9:
	case 10:
		if(Palettes.colors[0].c.r < 48) {
			Palettes.colors[0].c.r++;
			Palettes.colors[0].c.b--;
			palette_changed = true;
		}
		if(boss.phase == 9) {
			goto flyto_phase;
		}
		goto pattern_phase;

	case 13:
	case 14:
		if(Palettes.colors[0].c.g < 64) {
			if(Palettes.colors[0].c.r > 0) {
				Palettes.colors[0].c.r--;
			}
			Palettes.colors[0].c.g++;
			if(Palettes.colors[0].c.b > 0) {
				Palettes.colors[0].c.b--;
			}
			palette_changed = true;
		}
		if(boss.phase == 13) {
			goto flyto_phase;
		}
		goto pattern_phase;

	// The last pattern phase before the final one, and the only one counted in
	// patterns rather than in frames: eight runs of whatever
	// [exalice_pattern] was left pointing at.
	case 16:
		if(exalice_gather_and_pattern()) {
			boss.phase_state.patterns_seen++;

			// Timeout condition
			if(boss.phase_state.patterns_seen > 8) {
				goto colored_phase_next;
			}
		}
		goto colored_phase_hittest;

	// The final phase: a pulsing background colour over
	// exalice_spreads_and_sweep(), ended either by the HP threshold or by a
	// 5000-frame timeout that gives no defeat bonus.
	case 17:
		if(boss_statebyte[10] == 0) {
			if(Palettes.colors[0].c.g <= 0) {
				goto pulse_up;
			}
			Palettes.colors[0].c.g--;
		} else if(boss_statebyte[10] == 1) {
			Palettes.colors[0].c.r += 2;
			Palettes.colors[0].c.g++;
			Palettes.colors[0].c.b += 2;
			if(Palettes.colors[0].c.r >= 128) {
				boss_statebyte[10] = 2;
			}
		} else {
			Palettes.colors[0].c.r += -2;
			Palettes.colors[0].c.g--;
			Palettes.colors[0].c.b += -2;
			if(Palettes.colors[0].c.r == 0) {
pulse_up:
				boss_statebyte[10] = 1;
			}
		}
		palette_changed = true;
		exalice_spreads_and_sweep();
		if(boss.phase_frame <= 5000) {
			if(!exalice_hittest()) {
				break;
			}
			boss.phase_state.defeat_bonus = true;
		}

		// Next phase
		boss_explode_small(ET_VERTICAL);
		boss.phase_frame = 0;
		boss.phase = PHASE_BOSS_EXPLODE_SMALL;
		_asm {
			mov	word ptr boss_custombullets_render, offset nullfunc_near
		}
		bullet_zap_drop_point_items = false;
		break;

	default:
		boss_defeat_update(200);
		break;
	}

	cheetos_update();
	firewaves_update();
	hud_hp_update_and_render(boss.hp, EXALICE_HP_TOTAL);
}
