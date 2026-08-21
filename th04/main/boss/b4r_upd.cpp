/// Stage 4 Boss - Reimu: her fight's own update function, her patterns and her
/// two orb spawners
/// ---------------------------------------------------------------------------
/// (#included from th04/main_036.cpp, AHEAD of th04/main/boss/bx2_upd.cpp,
/// which is this fight's original address order: ZUN's object for
/// main_036_TEXT held Reimu's Stage 4 fight and then Gengetsu's Extra one.
/// kb/codegen/0112 + 0114.)
///
/// The rest of the fight is th04/main/boss/b4r.cpp -- a different segment and
/// therefore a different object, which is why orbs_add_moving() and
/// orbs_add_spinning() below keep external C++ linkage rather than becoming
/// `static`: that file has declared and called both of them since before this
/// parcel. th04/main/boss/b4r.hpp holds what the two halves share.
///
/// NOT self-contained, exactly as th04/main/boss/bx2_upd.cpp is not: every
/// header this fight needs is #included by th04/main_036.cpp instead, because
/// most of them have no include guard and this object would otherwise expand
/// them twice (kb/codegen/0129).

/// From headers this object must not expand twice
/// ----------------------------------------------
// libs/master.lib/master.hpp, which pulls in far too much for this object.
// `far pascal` is MASTER_RET under the large model (libs/master.lib/func.hpp).
// th04/main/boss/bx2_upd.cpp, which th04/main_036.cpp #includes after this
// file, spells [iatan2] identically; two identical declarations in one
// translation unit are fine, and neither file can include the header.
extern "C" const short __cdecl SinTable8[256], CosTable8[256];
extern "C" int far pascal iatan2(int y, int x);

// th03/math/polar.hpp, which is not included because this object already gets
// [CosTable8] and [SinTable8] the hard way above.
int pascal polar(int center, int radius, int ratio);

// th04/math/vector.hpp.
extern "C" void pascal near vector2_near(
	SPPoint near &ret, unsigned char angle, subpixel_t length
);

// th04/main/player/shot.hpp, which reaches th04/math/randring.hpp and would
// therefore expand the same inline bodies a second time in this object -- so
// its inline shots_hittest() overload is spelled out at the one call site
// below, the same way th04/main/boss/b4m_upd.cpp spells it out.
extern SPPoint shot_hitbox_center;
extern SPPoint shot_hitbox_radius;
int shots_hittest(void);
/// ----------------------------------------------

/// Still in th04_main.asm, and staying there
/// -----------------------------------------
/// Two `.data?` bytes with no `public` of ZUN's. Both sit in the middle of the
/// dump's data, where a contribution from this object could never land, so
/// each keeps a zero-byte `label` alias there (kb/codegen/0123).

// Whether Reimu's sprite trails an afterimage of its previous position.
// th04/main/boss/fg.cpp reads it; the movement patterns below are what turn it
// on and off.
extern "C" unsigned char reimu_afterimage;

// The direction Reimu's aimed spread sweeps in, +/-2 units of angle per
// volley, picked from the half of the playfield the player stood on when the
// pattern locked its aim. Its only writer and its only reader are both inside
// reimu_1ED15() below, which is the whole of the evidence for the name.
// `[inferred]`, and **a naming round is owed for it.**
extern "C" unsigned char reimu_sweep_angle_delta;
/// -----------------------------------------

// Declared FAR here, and only here. th04/main/boss/bosses.hpp declares it
// `near`, which is what it is -- but a near reference under this object's
// `-zPmain_03` frames its offset on main_03, and this function is in main_01.
// kb/codegen/0162.
void pascal far reimu_marisa_bg_render(void);

/// Constants
/// ---------
// The HP Reimu starts her Stage 4 fight with. Phase 1 parks it in
// [boss.phase_end_hp] so that boss_phase_next()'s `boss.hp = boss.phase_end_hp`
// seeds the full bar.
static const int REIMU_HP = 9100;

// Her still pose. The six animated ones above it are deliberately left as
// literals: th04/sprites/main_pat.h names only PAT_REIMU_ANIMATED in that
// bank, and a naming round is owed for the rest.
static const int PAT_REIMU_MARISA_STILL = PAT_STAGE;

// [boss.mode] value that is not a pattern.
static const unsigned char MODE_INTERVAL = 255;
/// ---------

/// State
/// -----
/// The eight [boss_statebyte] slots this fight uses, under the names
/// th04_main.asm's own `boss_statebyte_t` overlay gives them
/// (th04/main/boss/boss[bss].asm). That overlay is a `union`, so the name says
/// nothing about which slot -- and this fight is the one place in TH04 where
/// that matters, because **the dump spells slots 4 AND 5 `BSB_spread_delta_angle`**.
/// They are two different bytes with two different owners, so they get two
/// different names here.

// How many orbs the current phase spawns, and how many frames apart.
#define orb_count                 boss_statebyte[0]
#define orb_interval              boss_statebyte[1]

// The bullet-count, fan width and turn cap of reimu_1EF87()'s randomly-aimed
// pellet spread. The dump spelled slot 4 with the same overlay field name as
// slot 5 below, and slot 3 with the bare spread one.
#define spread_turns_max          boss_statebyte[2]
#define spread_count              boss_statebyte[3]
#define spread_delta_angle        boss_statebyte[4]

// The fan width of reimu_1ED15()'s AIMED six-way, which is a different byte
// under the same `BSB_spread_delta_angle` spelling, read exactly once in the
// whole fight and seeded by the stage script.
#define aimed_spread_delta_angle  boss_statebyte[5]

// How many bullets reimu_1F22A() stacks per volley.
#define stack                     boss_statebyte[6]

// Which of reimu_update()'s sparse sub-dispatches the current phase runs.
#define reimu_subpattern_id       boss_statebyte[10]
/// -----

/// The same abbreviations th04/main/boss/b4r.cpp gives itself for the other
/// half of this fight. Nothing else in this object uses any of these names.
#define ORB_COUNT    	REIMU_ORB_COUNT
#define orb_flag_t   	reimu_orb_flag_t
#define orb_t        	reimu_orb_t
#define orbs         	reimu_orbs

// The two movement-only patterns of Reimu's fight: no bullets at all, just a
// three-leg zigzag flown at a constant speed with the afterimage trail
// switched on for its whole duration. Both are the *last* [boss.mode] of the
// phases that dispatch them, and both end a leg by advancing
// [boss.phase_state.patterns_seen] and handing control to
// `patterns_seen % 2`, so the phase alternates between its two real patterns
// while this one walks its own three legs.
//
// Leg `patterns_seen % 3`: 0 and 2 are the long diagonals (32 frames), 1 is
// the horizontal return and is twice as long (64 frames). The modulo is
// recomputed at each test rather than cached, exactly as the original does.
//
// This one flies left-down, right (64 frames), left-up.
static void near reimu_1E917(void)
{
	// The single register variable: SI. (kb/codegen/0117)
	int frames;

	if(boss.phase_frame == 1) {
		reimu_afterimage = true;
		if((boss.phase_state.patterns_seen % 3) == 0) {
			boss.pos.velocity.x.v = -TO_SP(4);
			boss.pos.velocity.y.v =  TO_SP(1);
		} else if((boss.phase_state.patterns_seen % 3) == 1) {
			boss.pos.velocity.x.v =  TO_SP(4);
			boss.pos.velocity.y.v = 0;
		} else {
			boss.pos.velocity.x.v = -TO_SP(4);
			boss.pos.velocity.y.v = -TO_SP(1);
		}
	}
	boss.pos.update_seg3();

	frames = 32;
	if((boss.phase_state.patterns_seen % 3) == 1) {
		frames = 64;
	}
	if(boss.phase_frame == frames) {
		boss.phase_state.patterns_seen++;
		boss.mode = (boss.phase_state.patterns_seen % 2);
		boss.phase_frame = 0;
		reimu_afterimage = false;
	}
}

// reimu_1E917()'s mirror image: right-up, left (64 frames), right-down.
// Identical in every other respect, down to the frame counts.
static void near reimu_1E9B1(void)
{
	// The single register variable: SI. (kb/codegen/0117)
	int frames;

	if(boss.phase_frame == 1) {
		reimu_afterimage = true;
		if((boss.phase_state.patterns_seen % 3) == 0) {
			boss.pos.velocity.x.v =  TO_SP(4);
			boss.pos.velocity.y.v = -TO_SP(1);
		} else if((boss.phase_state.patterns_seen % 3) == 1) {
			boss.pos.velocity.x.v = -TO_SP(4);
			boss.pos.velocity.y.v = 0;
		} else {
			boss.pos.velocity.x.v =  TO_SP(4);
			boss.pos.velocity.y.v =  TO_SP(1);
		}
	}
	boss.pos.update_seg3();

	frames = 32;
	if((boss.phase_state.patterns_seen % 3) == 1) {
		frames = 64;
	}
	if(boss.phase_frame == frames) {
		boss.phase_state.patterns_seen++;
		boss.mode = (boss.phase_state.patterns_seen % 2);
		boss.phase_frame = 0;
		reimu_afterimage = false;
	}
}

// The 46-frame charge schedule seven of Reimu's patterns open with, and the
// only thing that animates her during them. Its role is exactly that of
// marisa_charge_animate() (th04/main/boss/b4m_upd.cpp) and gengetsu_1FA33()
// (th04/main/boss/bx2_upd.cpp) -- a per-pattern charge-up whose return value
// IS the pattern's own schedule -- so a name should come from the mirror rule
// on that family. **A naming round is owed.** Kept address-suffixed here
// because its seven callers are still ASM.
//
// Returns 0 while the charge is running, 2 on the single frame the pattern
// fires (46), and 1 on every frame after that.
//
// Frames 14/16/18 stack the same three gather circles that
// marisa_16E9D() and gengetsu_1F903() do -- one white-ish color 9 ring and
// two more on top of it -- centered 4 pixels right of and 28 pixels above
// Reimu. Frames 22 through 42 then step her through the seven single-pose
// cels below PAT_REIMU_ANIMATED, with the shrinking circle landing on the
// gather center at frame 30. **The seven cels are unnamed on purpose:**
// th04/sprites/main_pat.h names only PAT_REIMU_ANIMATED (= PAT_STAGE + 8) in
// this bank, and what these poses depict is not recoverable from the binary.
//
// Cases 16 and 18 share a tail rather than repeating the call, which is why
// 16 is written with its own gather_add_only(): Turbo C++ cross-jumps the
// identical epilogues, and 16 falls straight through into 18's body.
//
// `-a2` is for the one pad byte between the epilogue and the generated switch
// table. It is parity-dependent on where this object's code contribution
// starts -- kb/codegen/0160 -- so re-probe it if anything is ever added ahead
// of reimu_1E917().
#pragma option -a2
static unsigned char near reimu_1EA4B(void)
{
	switch(boss.phase_frame) {
	case 14:
		gather_template.center.x.v = (boss.pos.cur.x.v + TO_SP(4));
		gather_template.center.y.v = (boss.pos.cur.y.v - TO_SP(28));
		gather_template.ring_points = 16;
		gather_template.radius.v = TO_SP(256);
		gather_template.col = 9;
		gather_add_only();
		boss.sprite = (PAT_STAGE + 1);
		snd_se_play(8);
		circles_color = V_WHITE;
		break;
	case 16:
		gather_template.col = 8;
		gather_add_only();
		break;
	case 18:
		gather_add_only();
		break;
	case 22:
		boss.sprite = (PAT_STAGE + 2);
		break;
	case 26:
		boss.sprite = (PAT_STAGE + 3);
		break;
	case 30:
		boss.sprite = (PAT_STAGE + 4);
		circles_add_shrinking(
			gather_template.center.x.v, gather_template.center.y.v
		);
		break;
	case 34:
		boss.sprite = (PAT_STAGE + 5);
		break;
	case 38:
		boss.sprite = (PAT_STAGE + 6);
		break;
	case 42:
		boss.sprite = (PAT_STAGE + 7);
		break;
	case 46:
		boss.sprite = (PAT_STAGE + 1);
		snd_se_play(3);
		return 2;
	}
	if(boss.phase_frame < 46) {
		return 0;
	}
	return 1;
}
#pragma option -a1

// Spawns a single orb from [orb_template] into the first free slot, moving in
// a straight line at the template's own angle and speed. Unlike the spinning
// spawner below it fires exactly one orb per call; the patterns that want more
// call it once per orb.
void pascal near orbs_add_moving(void)
{
	// The two register variables, exactly as in reimu_orbs_render()
	// (th04/main/boss/fg.cpp): the orb pointer earns SI by how often it is
	// dereferenced as a base (kb/codegen/0117), the counter takes DI. The
	// function has no stack frame at all.
	orb_t near *orb = orbs;
	int i;

	for(i = 0; i < ORB_COUNT; i++, orb++) {
		if(orb->flag != OF_FREE) {
			continue;
		}
		orb->flag = OF_MOVE;
		orb->center = orb_template.center;
		orb->origin = orb_template.origin;
		orb->unknown = orb_template.unknown;
		orb->move_speed = orb_template.move_speed;
		orb->angle = orb_template.angle;
		orb->distance.v = 0;

		// Reads one word at [angle], and therefore also loads [center]'s low
		// byte into the high byte of the pushed argument -- ordinary Turbo C++
		// behavior for a byte argument, and vector2_near() only consumes BL,
		// so the second byte is never looked at. Do NOT "fix" this with a
		// cast; a cast would request a zero-extension the original has not
		// got. (th04/main/enemy/velocity.cpp says the same thing about
		// enemy_velocity_set().)
		vector2_near(
			orb->velocity, orb_template.angle, orb_template.move_speed
		);
		break;
	}
}

// Spawns [count] orbs from [orb_template] into the free slots, spread evenly
// around a full turn starting at [angle_offset], each spinning outwards from
// the template's [origin]. Silently spawns fewer than [count] if the array
// runs out first.
void pascal near orbs_add_spinning(unsigned char angle_offset, int count)
{
	// SI and DI are taken by the walk below, so the third variable can only
	// reach a register by being asked for one -- and Turbo C++ 4.0J answers
	// with CX rather than a stack home. Without `register` it lands at [bp-2]
	// and the function opens `enter 2, 0` instead of a bare `push bp`.
	// `[measured]`; th04/main/boss/b4m_upd.cpp measured the same thing from
	// the other side, where `register` moved two locals into CX that were
	// wanted on the stack.
	register int spawned = 0;

	orb_t near *orb = orbs;
	int i;

	for(i = 0; i < ORB_COUNT; i++, orb++) {
		if(orb->flag != OF_FREE) {
			continue;
		}
		orb->flag = OF_MOVEOUT_SPIN;
		orb->spin_time = orb_template.spin_time;
		orb->center = orb_template.center;
		orb->origin = orb_template.origin;
		orb->unknown = orb_template.unknown;
		orb->move_speed = orb_template.move_speed;

		// A full turn is 0x100, so this is (spawned / count) of one, added to
		// the caller's offset. The division is signed -- `CWD` / `IDIV`, not
		// `XOR DX, DX` / `DIV` -- because [count] is an `int`.
		orb->angle = (((spawned << 8) / count) + angle_offset);

		orb->distance.v = 0;
		orb->angle_speed = orb_template.angle_speed;
		spawned++;
		if(spawned >= count) {
			break;
		}
	}
}

// Runs a frame of every live orb -- the outwards spin, the straight-line
// bounce after it, and both hittests -- and frees the ones that left the
// bottom of the playfield. The counterpart of
// th04/main/boss/b4m_upd.cpp's marisa_bits_update_and_hittest(), which is the
// same shape for Marisa's bits, and of reimu_orbs_render()
// (th04/main/boss/fg.cpp). A name mirroring those two is what a naming round
// should reach for; kept at the dump's spelling here because the rest of this
// chain is, and **a naming round is owed.**
static void near reimu_1EBF3(void)
{
	enum {
		// Damage box against the player's shots…
		SHOT_HITBOX_RADIUS = TO_SP(12),

		// … and against the player, who gets the same box expressed as a
		// top-left corner and an extent, because the original tests it with
		// two unsigned comparisons rather than four signed ones.
		PLAYER_HITBOX_EXTENT = TO_SP(24),

		// Distance at which OF_MOVEOUT_SPIN stops the outwards motion, and
		// the speed it moves out at.
		MOVEOUT_DISTANCE = TO_SP(64),
		MOVEOUT_SPEED = TO_SP(4),

		// Quarter turn, which is what the orb's flight direction is offset by
		// relative to the spin it leaves behind — to the outside of the spin
		// in both directions.
		SPIN_QUARTERTURN = 0x40,
	};

	// [bp-2] and [bp-4] in declaration order (kb/codegen/0010); the orb
	// pointer and the counter take SI and DI as everywhere else in this file.
	subpixel_t left;
	subpixel_t top;

	orb_t near *orb = orbs;
	int i;

	for(i = 0; i < ORB_COUNT; i++, orb++) {
		if(orb->flag == OF_FREE) {
			continue;
		}
		if(orb->flag == OF_MOVEOUT_SPIN) {
			orb->center.x.v = polar(
				orb->origin.x.v, orb->distance.v, CosTable8[orb->angle]
			);
			orb->center.y.v = polar(
				orb->origin.y.v, orb->distance.v, SinTable8[orb->angle]
			);
			if(orb->distance.v < MOVEOUT_DISTANCE) {
				orb->distance.v += MOVEOUT_SPEED;
			}
			orb->spin_time--;
			orb->angle += orb->angle_speed;
			if(orb->spin_time == 0) {
				// `signed char`, spelled out, is the ONLY type that narrows
				// this to `CMP BYTE PTR [SI+19h], 0` and still takes the
				// signed branch — plain `char` compares the same way but
				// widens with a `CBW` first, and here it would not even reload
				// the byte, because `-Z` still has it in AL from the `+=`
				// above. kb/codegen/0142, and the one byte this parcel cost.
				// (Declaring the member `signed char` in
				// th04/main/boss/b4r.hpp emits the identical bytes;
				// `[measured]`. The cast is used instead so that the header
				// keeps ZUN's own spelling.)
				orb->angle = (
					(reinterpret_cast<signed char &>(orb->angle_speed) >= 0)
						? (orb->angle + SPIN_QUARTERTURN)
						: (orb->angle - SPIN_QUARTERTURN)
				);
				vector2_near(orb->velocity, orb->angle, orb->move_speed);

				// The original advances the state by one rather than assigning
				// the successor, exactly as Marisa's bits do.
				reinterpret_cast<unsigned char &>(orb->flag)++; // = OF_MOVE
			}
		} else if(orb->flag == OF_MOVE) {
			// ZUN bloat: nothing reads [spin_time] again in this state.
			orb->spin_time++;

			orb->center.x.v += orb->velocity.x.v;
			if(
				(orb->center.x.v < TO_SP(0)) ||
				(orb->center.x.v > TO_SP(PLAYFIELD_W))
			) {
				orb->velocity.x.v = -orb->velocity.x.v;
			}
			orb->center.y.v += orb->velocity.y.v;
			if(orb->center.y.v >= TO_SP(PLAYFIELD_H)) {
				orb->flag = OF_FREE;
			}

			// Gravity, in whole subpixels per frame squared. The bounce above
			// only mirrors X, so an orb always leaves through the bottom.
			orb->velocity.y.v++;
		}

		// th04/main/player/shot.hpp's inline shots_hittest() overload, spelled
		// out because this object deliberately does not include that header.
		// The damage it returns is dropped: the orbs have no HP and cannot be
		// destroyed, but they still decay any shot that touches them.
		shot_hitbox_radius.x.v = SHOT_HITBOX_RADIUS;
		shot_hitbox_radius.y.v = SHOT_HITBOX_RADIUS;
		shot_hitbox_center.x.v = orb->center.x.v;
		shot_hitbox_center.y.v = orb->center.y.v;
		shots_hittest();

		// The player hittest, biased into the top-left corner of the orb's box
		// so that one unsigned comparison per axis covers both directions --
		// `JNB`, not `JGE`, is what says these are unsigned.
		left = (orb->center.x.v + TO_SP(-12));
		top = (orb->center.y.v + TO_SP(-12));
		if(
			(static_cast<unsigned int>(player_pos.cur.x.v - left) <
				PLAYER_HITBOX_EXTENT) &&
			(static_cast<unsigned int>(player_pos.cur.y.v - top) <
				PLAYER_HITBOX_EXTENT)
		) {
			player_is_hit = true;
		}
	}
}

// One of Reimu's patterns: a six-way spread, aimed once at the player when the
// gather animation fires, then swept away from the half of the playfield the
// player was on, one volley every fourth frame.
static void near reimu_1ED15(void)
{
	// `unsigned char`, not the `enum` that would document it better: the
	// original homes the returned stage at [bp-1], and a byte enum local would
	// home at [bp-2] with every instruction the same length. kb/codegen/0163.
	unsigned char stage = reimu_1EA4B();

	// Two independent `if`s, not an `if`/`else if`: the first arm falls
	// through into the second test rather than jumping past it.
	if(stage == 2) {
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.patnum = PAT_BULLET16_D_YELLOW;
		bullet_template.speed.v = TO_SP(6);
		boss.angle = iatan2(
			(player_pos.cur.y.v - boss.pos.cur.y.v),
			(player_pos.cur.x.v - boss.pos.cur.x.v)
		);
		bullet_template.group = BG_SPREAD;
		bullet_template.count = 6;
		bullet_template.delta.spread_angle = aimed_spread_delta_angle;
		bullet_template_tune();

		// Half the playfield, so the sweep always runs away from the player
		// rather than tracking them.
		reimu_sweep_angle_delta = (
			(player_pos.cur.x.v < TO_SP(PLAYFIELD_W / 2)) ? -2 : 2
		);
	}
	if(stage == 1) {
		if((boss.phase_frame % 4) == 0) {
			bullet_template.angle = boss.angle;
			bullets_add_regular();
			snd_se_play(3);

			// The first 64 frames of volleys all go to the same aim; only
			// after that does the spread start walking.
			if(boss.phase_frame >= 64) {
				boss.angle += reimu_sweep_angle_delta;
			}
		}
		if(boss.phase_frame >= 112) {
			// Reimu's base pose, which th04/sprites/main_pat.h only has as
			// PAT_STAGE + 0 -- the dump writes the literal.
			boss.sprite = 128;

			boss.phase_frame = 0;
			boss.mode = -1;
		}
	}
}

// Pattern: one 9-way aimed spread of 16x16 blue balls that decelerates to a
// stop, re-aims at the player and fires again, up to [spread_turns_max] times.
// Fired once, on the single frame the shared opening animation reports as
// stage 2; the phase then ends 128 frames in.
static void near reimu_1EDBC(void)
{
	// `unsigned char` rather than the three-valued state this really is:
	// kb/codegen/0163 -- an enum local would home at [bp-2], and the original
	// homes it at [bp-1] with every instruction length unchanged.
	unsigned char stage = reimu_1EA4B();

	if(stage == 2) {
		bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.speed.v = (TO_SP(5) + 5);
		bullet_template.angle = 0;
		bullet_template.special_motion = BSM_DECELERATE_THEN_TURN_AIMED;
		bullet_special.turns_max = spread_turns_max;
		bullet_template.group = BG_SPREAD_AIMED;
		bullet_template.count = 9;
		bullet_template.delta.spread_angle = 6;
		bullet_template_tune();
		bullets_add_special_fixedspeed();
	}
	if(stage == 1) {
		if(boss.phase_frame >= 128) {
			boss.sprite = 128;
			boss.phase_frame = 0;
			boss.mode = -1;
		}
	}
}

// Pattern: on frame 32, spawns a ring of [orb_count] orbs that spin out of
// Reimu's current position. The phase ends 96 frames in, and flips the sign of
// the template's [angle_speed] so that the next run of this pattern spins the
// other way.
static void near reimu_1EE21(void)
{
	if(boss.phase_frame == 32) {
		boss.sprite = 136;
		orb_template.spin_time = 64;
		orb_template.move_speed.v = (TO_SP(3) + 8);
		orb_template.origin = boss.pos.cur;
		orbs_add_spinning(randring2_next16(), orb_count);
		snd_se_play(8);
	}
	if(boss.phase_frame >= 96) {
		boss.phase_frame = 0;
		boss.mode = -1;
		orb_template.angle_speed = -orb_template.angle_speed;
	}
}

// Pattern: every 32nd frame, a 16-bullet ring of accelerating 16x16 blue balls
// out of a random point in the 64x64 box around Reimu; 16 frames after each of
// those, a 12-step sweep of 4-pellet stacks out of another such point. Ends
// 288 frames in.
static void near reimu_1EE73(void)
{
	unsigned char stage = reimu_1EA4B();
	int i;

	if(stage == 1) {
		if((boss.phase_frame % 32) == 0) {
			bullet_template.origin.x.v = (
				randring2_next16_mod(TO_SP(64)) + (boss.pos.cur.x.v - TO_SP(32))
			);
			bullet_template.origin.y.v = (
				randring2_next16_mod(TO_SP(64)) + (boss.pos.cur.y.v - TO_SP(32))
			);

			// ZUN bloat: Immediately overwritten with 0, four lines down.
			bullet_template.angle = randring2_next16();

			bullet_template.spawn_type = BST_BULLET16_CLOUD_BACKWARDS;
			bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
			bullet_template.speed.v = TO_SP(1);
			bullet_template.angle = 0;
			bullet_template.special_motion = BSM_SPEEDUP;
			bullet_special.speed_delta.v = 1;
			bullet_template.group = BG_RING;
			bullet_template.count = 16;
			bullet_template_tune();
			bullets_add_special_fixedspeed();
			snd_se_play(3);
		}
		if((boss.phase_frame % 32) == 16) {
			bullet_template.origin.x.v = (
				randring2_next16_mod(TO_SP(64)) + (boss.pos.cur.x.v - TO_SP(32))
			);
			bullet_template.origin.y.v = (
				randring2_next16_mod(TO_SP(64)) + (boss.pos.cur.y.v - TO_SP(32))
			);

			// ZUN bloat: Overwritten by the loop's own initializer below,
			// which rolls a second angle.
			bullet_template.angle = randring2_next16();

			bullet_template.spawn_type = BST_PELLET;
			bullet_template.speed.v = (TO_SP(1) + 8);
			bullet_template.group = BG_STACK;
			bullet_template.count = 4;
			bullet_template.delta.stack_speed.v = 8;
			bullet_template_tune();

			// Both the initializer and the increment end in a store of AL to
			// [bullet_template.angle], and `-O` cross-jumps the two into the
			// single store at the loop's condition label. Moving either half
			// into the body -- the obvious spelling -- puts a store inside the
			// loop instead and loses the merge. `[measured]`
			for(
				i = 0, bullet_template.angle = randring2_next16();
				i < 12;
				i++, bullet_template.angle += 0x15
			) {
				bullets_add_regular();
			}
			snd_se_play(3);
		}
		if(boss.phase_frame >= 288) {
			boss.sprite = 128;
			boss.phase_frame = 0;
			boss.mode = -1;
		}
	}
}

// Pattern: a [spread_count]-way pellet spread every 4th frame, whose center
// angle is jittered over an 8-step range and then rotated 0x44 back from it;
// every 4th of those spreads additionally fires a rank-scaled aimed stack of
// 16x16 red balls out of a random point in the 64x64 box around Reimu. Ends
// 192 frames in.
static void near reimu_1EF87(void)
{
	unsigned char stage = reimu_1EA4B();

	if(stage == 1) {
		if((boss.phase_frame % 4) == 0) {
			bullet_template.spawn_type = BST_PELLET;
			bullet_template.speed.v = TO_SP(8);

			// kb/codegen/0032: the byte arithmetic on the call's result only
			// stays in AL if it is spelled through the pseudoregister, and
			// `+= -0x44` rather than `-= 0x44` for the `ADD AL, imm8` form.
			_AL = randring2_next16_and(7);
			_AL += -0x44;
			bullet_template.angle = _AL;

			bullet_template.group = BG_SPREAD;
			bullet_template.count = spread_count;
			bullet_template.delta.spread_angle = spread_delta_angle;
			bullets_add_regular_fixedspeed();

			if((boss.phase_frame % 16) == 0) {
				bullet_template.origin.x.v = (
					randring2_next16_mod(TO_SP(64)) +
					(boss.pos.cur.x.v - TO_SP(32))
				);
				bullet_template.origin.y.v = (
					randring2_next16_mod(TO_SP(64)) +
					(boss.pos.cur.y.v - TO_SP(32))
				);
				bullet_template.group = BG_STACK_AIMED;
				bullet_template.count = (rank + 3);
				bullet_template.delta.stack_speed.v = TO_SP(1);
				bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
				bullet_template.speed.v = TO_SP(2);
				bullet_template.angle = 0;
				bullet_template.patnum = PAT_BULLET16_N_BALL_RED;
				bullets_add_regular();
				snd_se_play(3);
			}
		}
		if(boss.phase_frame >= 192) {
			boss.sprite = 128;
			boss.phase_frame = 0;
			boss.mode = -1;
		}
	}
}

// Her aimed pellet pattern. The charge stages a 3-bullet
// BG_RANDOM_ANGLE_AND_SPEED spray aimed from Reimu at the player, and the
// pattern then fires it every other frame for its first 96 frames. Past that
// it re-stages the template into a 32-bullet ring of BSM_SPEEDUP blue balls
// and fires one every 16th frame until frame 128, which is a different
// pattern in everything but [boss.mode].
static void near reimu_1F04E(void)
{
	unsigned char charge = reimu_1EA4B();

	if(charge == 2) {
		bullet_template.spawn_type = BST_PELLET;
		bullet_template.speed.v = (TO_SP(3) + 6);
		bullet_template.angle = iatan2(
			(player_pos.cur.y.v - boss.pos.cur.y.v),
			(player_pos.cur.x.v - boss.pos.cur.x.v)
		);
		bullet_template.group = BG_RANDOM_ANGLE_AND_SPEED;
		bullet_template.count = 3;
		bullet_template_tune();
	}
	if(charge == 1) {
		if(boss.phase_frame < 96) {
			if((boss.phase_frame % 2) == 0) {
				bullets_add_regular();
				snd_se_play(3);
			}
		} else if(boss.phase_frame <= 128) {
			if((boss.phase_frame % 16) == 0) {
				bullet_template.spawn_type = BST_BULLET16_CLOUD_BACKWARDS;
				bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
				bullet_template.group = BG_RING;
				bullet_template.count = 32;
				bullet_template.angle = randring2_next16();
				bullet_template.speed.v = TO_SP(3);
				bullet_template.special_motion = BSM_SPEEDUP;
				bullet_special.speed_delta.v = 1;	// 0.0625 pixels
				bullet_template_tune();
				bullets_add_special();
				snd_se_play(15);
			}
		} else {
			boss.sprite = PAT_REIMU_MARISA_STILL;
			boss.phase_frame = 0;
			boss.mode = MODE_INTERVAL;
		}
	}
}

// Her orb pattern, and the only one that uses the [custom_entities] block.
// Every [orb_interval] frames from frame 32 on, it spawns one orb out of
// Reimu's current position, stepping the spawn angle by [angle_speed] each
// time. Ending the pattern also NEGATES that step, so the next run of it
// spirals the other way round.
static void near reimu_1F111(void)
{
	if(boss.phase_frame == 32) {
		boss.sprite = PAT_REIMU_ANIMATED;
		orb_template.angle = 0x00;
		orb_template.move_speed.v = 0x38;	// 3.5 pixels
		orb_template.center.x = boss.pos.cur.x;
		orb_template.center.y = boss.pos.cur.y;
		snd_se_play(8);
	}
	if(boss.phase_frame >= 32) {
		if((boss.phase_frame % orb_interval) == 0) {
			orb_template.angle -= orb_template.angle_speed;
			orbs_add_moving();
		}
	}
	if(boss.phase_frame >= 180) {
		boss.phase_frame = 0;
		boss.mode = MODE_INTERVAL;
		orb_template.angle_speed = -orb_template.angle_speed;
	}
}

// Her scattered-stack pattern: every 16th frame from frame 32 on, it picks a
// random point inside a 64×64-pixel box centered on Reimu and fires eight
// 4-pellet stacks out of it, evenly spaced 45° apart around a random base
// angle. Unlike the other patterns it never ends itself -- reimu_update()
// owns the frame that does.
static void near reimu_1F17C(void)
{
	// The register variable. There are no stack locals, which is why the
	// prolog is a bare `PUSH BP` rather than an `ENTER`.
	int i;

	if(boss.phase_frame == 32) {
		boss.sprite = PAT_REIMU_ANIMATED;
		boss.angle = 0x00;
		bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.angle = -0x40;
		bullet_template.special_motion = BSM_NONE;
		bullet_template.delta.stack_speed.v = 8;	// 0.5 pixels
	}
	if(boss.phase_frame >= 32) {
		if((boss.phase_frame % 16) == 0) {
			bullet_template.origin.x.v = (
				randring2_next16_mod(TO_SP(64)) +
				(boss.pos.cur.x.v - TO_SP(32))
			);
			bullet_template.origin.y.v = (
				randring2_next16_mod(TO_SP(64)) +
				(boss.pos.cur.y.v - TO_SP(32))
			);
			bullet_template.angle = randring2_next16();
			bullet_template.spawn_type = BST_PELLET;
			bullet_template.speed.v = (TO_SP(1) + 8);
			bullet_template.group = BG_STACK;
			bullet_template.count = 4;
			bullet_template.delta.stack_speed.v = 10;	// 0.625 pixels
			bullet_template_tune();

			// Spelled with [i] initialized BEFORE the base angle, and
			// incremented INSIDE the body rather than in a `for` clause,
			// because both are load-bearing. Turbo C++ cross-jumps the
			// `MOV [angle], AL` that ends the pre-header with the identical
			// one that ends the body (kb/codegen/0097), leaving a single
			// shared store just above the loop test -- which it can only do
			// if `XOR SI, SI` sits above the seeding call rather than
			// between the seed and the jump. The natural
			// `angle = …; for(i = 0; i < 8; i++) { …; angle += 0x20; }`
			// emits the same instructions in the other order and is 3 bytes
			// LONGER here; in reimu_1F2F3() below, the same reordering is
			// exactly as long and still wrong, which is kb/codegen/0163's
			// lesson in a second shape.
			i = 0;
			bullet_template.angle = randring2_next16();
			while(i < 8) {
				bullets_add_regular();
				i++;
				// kb/codegen/0094: a compound assignment with a plain `int`
				// literal is the AL round trip, not `ADD byte ptr [mem], 8`.
				bullet_template.angle += 0x20;
			}
			snd_se_play(3);
		}
	}
}

// Her mirrored twin-spread pattern. [reimu_pattern8_angle] is TOGGLED between
// +0x78 and -0x78 on every charge, so consecutive runs of the pattern lean to
// opposite sides; each volley then fires two spreads 180° apart around it, at
// a random speed, a random width and out of a random point in the same 64×64
// box the pattern above uses.
static void near reimu_1F22A(void)
{
	unsigned char charge = reimu_1EA4B();

	if(charge == 2) {
		bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.angle = -0x40;
		bullet_template.special_motion = BSM_NONE;
		bullet_template.delta.spread_angle = 8;

		// One store, so a conditional expression rather than two assignments
		// -- and `!=` rather than `==` with the arms swapped, because
		// kb/codegen/0095 takes the relational operator literally.
		reimu_pattern8_angle = (
			(reimu_pattern8_angle != 0x78) ? 0x78 : -0x78
		);
	}
	if(charge == 1) {
		if((boss.phase_frame % 4) == 0) {
			// kb/codegen/0032, twice: the original adds to the returned byte
			// in AL, where the same arithmetic written on the 16-bit return
			// value would widen to `ADD AX`.
			_AL = randring2_next16_and(0x1F);
			_AL += TO_SP(1);
			bullet_template.speed.v = _AL;
			bullet_template.group = BG_SPREAD;
			_AL = randring2_next16_and(3);
			_AL += 2;
			bullet_template.count = _AL;
			bullet_template_tune();

			bullet_template.origin.x.v = (
				randring2_next16_mod(TO_SP(64)) +
				(boss.pos.cur.x.v - TO_SP(32))
			);
			bullet_template.origin.y.v = (
				randring2_next16_mod(TO_SP(64)) +
				(boss.pos.cur.y.v - TO_SP(32))
			);

			// A byte GLOBAL on the right-hand side keeps the compound
			// assignment's memory read-modify-write (`ADD [angle], AL`); the
			// `+= 0x80` two lines down is an `int` literal and therefore the
			// AL round trip instead. Same operator, two codegens --
			// kb/codegen/0094.
			bullet_template.angle += reimu_pattern8_angle;
			bullets_add_special();
			bullet_template.angle += 0x80;
			bullets_add_special();
			snd_se_play(3);
		}
		if(boss.phase_frame >= 224) {
			boss.sprite = PAT_REIMU_MARISA_STILL;
			boss.phase_frame = 0;
			boss.mode = MODE_INTERVAL;
		}
	}
}

// Her aimed-stack fan: on the 16th frame of every 32, five aimed stacks of
// [stack] bullets each, sweeping backwards from +45° in 22.5° steps. The
// stacks are BST_BULLET16_CLOUD_FORWARDS, so they telegraph before they
// become solid.
static void near reimu_1F2F3(void)
{
	// [charge] is `unsigned char` and not an enum: Turbo C++ 4.02 homes a
	// byte-sized *enum* local at [bp-2] and only a `char`-family one at the
	// odd [bp-1] the original uses, at identical length throughout
	// (kb/codegen/0163). [i] is the register variable.
	unsigned char charge;
	int i;

	charge = reimu_1EA4B();
	if(charge == 2) {
		bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
		bullet_template.group = BG_STACK_AIMED;
		bullet_template.count = stack;
		bullet_template.delta.stack_speed.v = 12;	// 0.75 pixels
		bullet_template.special_motion = BSM_NONE;
		bullet_template.angle = 0x00;
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
	}
	if(charge == 1) {
		if((boss.phase_frame % 32) == 16) {
			// Same shape as reimu_1F17C()'s loop, and load-bearing for the
			// same reason: `XOR SI, SI` precedes the base-angle store. Here
			// the pre-header stores an immediate rather than AL, so nothing
			// cross-jumps and the length is identical either way -- the
			// ORDER of the first two instructions is the only evidence.
			i = 0;
			bullet_template.angle = 0x20;
			while(i < 5) {
				bullets_add_special_fixedspeed();
				i++;
				// `ADD AL, -0x10`, not `SUB AL, 0x10`: `-=` with a plain
				// `int` literal folds to the negated add here. (Measured;
				// kb/codegen/0022 describes the opposite outcome for a byte
				// cast inside a larger expression.)
				bullet_template.angle -= 0x10;
			}
			snd_se_play(15);
		}
		if(boss.phase_frame >= 128) {
			boss.sprite = PAT_REIMU_MARISA_STILL;
			boss.phase_frame = 0;
			boss.mode = MODE_INTERVAL;
		}
	}
}

// Not a pattern: the background pulse of her first phase, called once per
// frame from reimu_update()'s phase-1 branch. It ramps the red component of
// hardware color 0 up to 240, then back down to 64, and back up again,
// flipping [reimu_bg_pulse_direction] at each end.
static void near reimu_1F378(void)
{
	if(reimu_bg_pulse_direction == 0) {
		// kb/codegen/0094: the increment operator, not an add of one. `++`
		// gets the dedicated `INC byte ptr [mem]`; `+= 1` would round-trip
		// through AL and cost 3 bytes at each of the two sites.
		Palettes[0].c.r++;
		if(Palettes[0].c.r >= 240) {
			reimu_bg_pulse_direction = 1;
		}
	} else {
		Palettes[0].c.r--;
		if(Palettes[0].c.r <= 64) {
			reimu_bg_pulse_direction = 0;
		}
	}
	palette_changed = true;
}

#undef orb_interval
#undef stack

// Stage 4 Boss Reimu's own update function, and the fight's thirteen-phase
// script. th04/main/stage/setup.cpp installs it; it is the one `far` proc of
// this chain and the only one with external linkage.
//
// The phases come in three shapes, and every one of them ends in the same
// tail: re-aim the homing bullets at her, run reimu_1EBF3(), redraw the HP
// bar.
//
// • 0 and 1 are the entrance, fought through boss_hittest_shots_invincible()
//   so nothing lands. 0 runs 96 frames and then swaps the background to the
//   purple this fight is lit in, arms the shared bombing background and the
//   white bomb-tile color; 1 runs 128 more and hands over through
//   boss_phase_next() with no explosion.
//
// • 2, 4, 6, 8 and 9 are the pattern phases. Each dispatches on [boss.mode] to
//   two real patterns plus the between-patterns move ([boss.mode] 255, the
//   zigzag in reimu_1E917()/reimu_1E9B1() that hands the mode back), pulses
//   the background through reimu_1F378() every frame, and can be ended by
//   damage only while [boss.phase_state.patterns_seen] is below a per-phase
//   count -- 9, 18, 11, 10 and 12 respectively. Past it the phase is
//   unkillable and ends the moment the pattern loop comes round.
//
// • 3, 5, 7 and 10 are the interludes: no patterns at all, just a flight to a
//   fixed point at a constant speed for 64 frames while the next phase's orb
//   template is seeded. 3, 7 and 10 fly to (192, 96); 5 only corrects Y, to
//   128, because phase 4 left her X where it was. 10 is the odd one out: it
//   does not call reimu_1F378(), and reddens the background by hand instead
//   (+3 red, -2 blue per frame), which is the fade into the last pattern.
//
// • 11 is that last pattern, and the only phase with a timeout: surviving
//   1000 frames ends it exactly as killing her does, and the difference is
//   recorded in [boss.phase_state.defeat_bonus] for phase 12 to pay out.
//
// • 12 is the two-step defeat explosion, on the same 16/32-frame schedule as
//   every other TH04 boss.
//
// Two pairs of phase tails are identical and Turbo C++ cross-jumps them, which
// is why 2 and 6 are written down to the same three statements and 5 and 10
// down to the same six: 2 pushes its boss_phase_next() arguments and jumps
// into 6's call, and 5 jumps into 10's phase-advance block outright.
void pascal far reimu_update(void)
{
	bullet_template.origin.x.v = (boss.pos.cur.x.v + TO_SP(4));
	bullet_template.origin.y.v = (boss.pos.cur.y.v - TO_SP(28));

	switch(boss.phase) {
	case 0:
		if(boss.phase_frame == 0) {
			reimu_afterimage = false;
		}
		boss_hittest_shots_invincible();
		if(boss.phase_frame > 96) {
			boss.phase++;
			Palettes[0].c.r = 128;
			Palettes[0].c.g = 0;
			Palettes[0].c.b = 224;
			palette_changed = true;
			boss.phase_frame = 0;
			snd_se_play(13);
			_asm mov word ptr bg_render_bombing_func, offset reimu_marisa_bg_render
			tiles_bb_col = V_WHITE;
		}
		break;

	case 1:
		reimu_1F378();
		boss.phase_frame++;
		boss_hittest_shots_invincible();
		if(boss.phase_frame >= 128) {
			boss.pos.velocity.x.v = 0;

			// Not dead, despite boss_phase_next() overwriting it one line
			// later: that function copies [boss.phase_end_hp] into [boss.hp]
			// before it replaces it, so this is how the fight gets its HP.
			boss.phase_end_hp = REIMU_HP;

			boss_phase_next(ET_NONE, 7900);
		}
		break;

	case 2:
		switch(boss.mode) {
		case 0:
			reimu_1ED15();
			break;
		case 1:
			reimu_1EDBC();
			break;
		case 255:
			reimu_1E917();
			break;
		}
		reimu_1F378();
		if(boss.phase_state.patterns_seen < 9) {
			if(!boss_hittest_shots()) {
				break;
			}
			boss_score_bonus(10);
		}
		boss_phase_next(ET_CIRCLE, 6300);

		// The first of the seven unnamed single-pose cels below
		// PAT_REIMU_ANIMATED (= PAT_STAGE + 1); her neutral pose, which is
		// what every phase transition parks her in.
		boss.sprite = 129;

		reimu_afterimage = false;
		break;

	case 3:
		if(boss.pos.cur.x.v < TO_SP(192)) {
			boss.pos.velocity.x.v = TO_SP(2);
		} else if(boss.pos.cur.x.v > TO_SP(192)) {
			boss.pos.velocity.x.v = -TO_SP(2);
		} else {
			boss.pos.velocity.x.v = 0;
		}
		if(boss.pos.cur.y.v < TO_SP(96)) {
			boss.pos.velocity.y.v = TO_SP(1);
		} else if(boss.pos.cur.y.v > TO_SP(96)) {
			boss.pos.velocity.y.v = -TO_SP(1);
		} else {
			boss.pos.velocity.y.v = 0;
		}
		boss.pos.update_seg3();
		reimu_1F378();
		boss_hittest_shots();
		if(boss.phase_frame >= 64) {
			boss.phase++;
			boss.phase_frame = 0;
			boss.phase_state.patterns_seen = 0;
			boss.mode = 0;
			boss.sprite = 129;
			orb_template.angle_speed = 0x04;
			orb_patnum_base = PAT_REIMU_ORB_BLUE;
			reimu_subpattern_id = 0;
		}
		break;

	case 4:
		switch(boss.mode) {
		case 0:
		case 1:
		case 2:
			reimu_1EE21();
			break;
		case 3:
			reimu_1EE73();
			break;
		case 255:
			boss.phase_state.patterns_seen++;

			// A coin flip walked over four values rather than a reroll: 0, 1
			// and 2 each either step up by one or jump straight to 3, and 3
			// always wraps back to 0. Since 0, 1 and 2 all dispatch to the
			// same pattern, what this really decides is how many times
			// reimu_1EE21() runs between two reimu_1EE73()s -- one to three,
			// weighted towards one.
			if(reimu_subpattern_id <= 2) {
				if(randring2_next16_and(1)) {
					reimu_subpattern_id++;
				} else {
					reimu_subpattern_id = 3;
				}
			} else {
				reimu_subpattern_id = 0;
			}

			boss.mode = reimu_subpattern_id;
			boss.phase_frame = 0;
			break;
		}
		reimu_1F378();
		if(boss.phase_state.patterns_seen < 18) {
			if(!boss_hittest_shots()) {
				break;
			}
			boss_score_bonus(10);
		}
		boss_phase_next(ET_NW_SE, 4500);
		boss.pos.velocity.x.v = 0;
		break;

	case 5:
		if(boss.pos.cur.y.v < TO_SP(128)) {
			boss.pos.velocity.y.v = TO_SP(1);
		} else if(boss.pos.cur.y.v > TO_SP(128)) {
			boss.pos.velocity.y.v = -TO_SP(1);
		} else {
			boss.pos.velocity.y.v = 0;
		}
		boss.pos.update_seg3();
		reimu_1F378();
		boss_hittest_shots();
		if(boss.phase_frame >= 64) {
			boss.phase++;
			boss.phase_frame = 0;
			boss.phase_state.patterns_seen = 0;
			boss.mode = 0;
			boss.sprite = 129;
			orb_template.angle_speed = 0x04;
		}
		break;

	case 6:
		switch(boss.mode) {
		case 0:
			reimu_1EF87();
			break;
		case 1:
			reimu_1F04E();
			break;
		case 255:
			reimu_1E9B1();
			break;
		}
		reimu_1F378();
		if(boss.phase_state.patterns_seen < 11) {
			if(!boss_hittest_shots()) {
				break;
			}
			boss_score_bonus(10);
		}
		boss_phase_next(ET_SW_NE, 2700);
		boss.sprite = 129;
		reimu_afterimage = false;
		break;

	case 7:
		if(boss.pos.cur.x.v < TO_SP(192)) {
			boss.pos.velocity.x.v = TO_SP(2);
		} else if(boss.pos.cur.x.v > TO_SP(192)) {
			boss.pos.velocity.x.v = -TO_SP(2);
		} else {
			boss.pos.velocity.x.v = 0;
		}
		if(boss.pos.cur.y.v < TO_SP(96)) {
			boss.pos.velocity.y.v = TO_SP(1);
		} else if(boss.pos.cur.y.v > TO_SP(96)) {
			boss.pos.velocity.y.v = -TO_SP(1);
		} else {
			boss.pos.velocity.y.v = 0;
		}
		boss.pos.update_seg3();
		reimu_1F378();
		boss_hittest_shots();
		if(boss.phase_frame >= 64) {
			boss.phase++;
			boss.phase_frame = 0;
			boss.phase_state.patterns_seen = 0;
			boss.mode = 0;
			boss.sprite = 129;
			orb_template.angle_speed = 0x12;
			orb_patnum_base = PAT_REIMU_ORB_YELLOW;
		}
		break;

	case 8:
		switch(boss.mode) {
		case 0:
			reimu_1F111();
			break;
		case 1:
			reimu_1F22A();
			break;
		case 255:
			// The plain alternation the other pattern phases get through
			// reimu_1E917()/reimu_1E9B1(); this phase has no move pattern of
			// its own, so it does the same two statements inline.
			boss.phase_state.patterns_seen++;
			boss.mode = (boss.phase_state.patterns_seen & 1);
			boss.phase_frame = 0;
			break;
		}
		reimu_1F378();
		if(boss.phase_state.patterns_seen < 10) {
			if(!boss_hittest_shots()) {
				break;
			}
			boss_score_bonus(10);
		}
		boss_phase_next(ET_HORIZONTAL, 900);
		boss.pos.velocity.x.v = 0;
		orb_template.angle_speed = 0x03;
		break;

	case 9:
		switch(boss.mode) {
		case 0:
			reimu_1EF87();
			break;
		case 1:
			reimu_1F2F3();
			break;
		case 255:
			reimu_1E917();
			break;
		}
		reimu_1F378();
		if(boss.phase_state.patterns_seen < 12) {
			if(!boss_hittest_shots()) {
				break;
			}
			boss_score_bonus(10);
		}
		boss_phase_next(ET_VERTICAL, 0);
		boss.pos.velocity.x.v = 0;
		orb_template.angle_speed = 0x03;

		// The bottom of the red ramp phase 10 then walks back up, and the
		// only place in the fight that writes a palette component outside
		// reimu_1F378()'s pulse.
		Palettes[0].c.r = 60;
		break;

	case 10:
		if(boss.pos.cur.x.v < TO_SP(192)) {
			boss.pos.velocity.x.v = TO_SP(2);
		} else if(boss.pos.cur.x.v > TO_SP(192)) {
			boss.pos.velocity.x.v = -TO_SP(2);
		} else {
			boss.pos.velocity.x.v = 0;
		}
		if(boss.pos.cur.y.v < TO_SP(96)) {
			boss.pos.velocity.y.v = TO_SP(1);
		} else if(boss.pos.cur.y.v > TO_SP(96)) {
			boss.pos.velocity.y.v = -TO_SP(1);
		} else {
			boss.pos.velocity.y.v = 0;
		}
		boss.pos.update_seg3();

		// kb/codegen/0032, and NOT a compound assignment to the same palette
		// component: a compound assignment through Palette::operator []()
		// materialises the far reference it returns into a `LES BX` pair. The
		// original is a byte load, a byte add and a byte store, exactly as
		// marisa_update()'s pulse is.
		_AL = Palettes[0].c.r;
		_AL += 3;
		Palettes[0].c.r = _AL;
		_AL = Palettes[0].c.b;
		_AL += -2;
		Palettes[0].c.b = _AL;
		palette_changed = true;

		boss_hittest_shots();
		if(boss.phase_frame >= 64) {
			boss.phase++;
			boss.phase_frame = 0;
			boss.phase_state.patterns_seen = 0;
			boss.mode = 0;
			boss.sprite = 129;
			orb_template.angle_speed = 0x04;
		}
		break;

	case 11:
		// Her own position, not the (+4, -28) offset every other phase fires
		// out of.
		bullet_template.origin.x.v = boss.pos.cur.x.v;
		bullet_template.origin.y.v = boss.pos.cur.y.v;

		reimu_1F17C();
		if(!boss_hittest_shots() && (boss.phase_frame < 1000)) {
			break;
		}
		boss_explode_small(ET_HORIZONTAL);
		boss.phase++;

		// The defeat bonus is the one thing that distinguishes killing Reimu
		// from surviving her: the timeout takes the same branch. Written as a
		// clear followed by a conditional set, which is what the original's
		// unconditional `MOV 0` ahead of the compare says.
		boss.phase_state.defeat_bonus = false;
		if(boss.phase_frame < 1000) {
			boss.phase_state.defeat_bonus = true;
		}

		boss.phase_frame = 0;
		break;

	case 12:
		boss.phase_frame++;
		if(boss.phase_frame == 16) {
			boss_explode_small(ET_VERTICAL);
		}
		if(boss.phase_frame == 32) {
			boss_defeat_explode_big(ET_SW_NE, 40);
			snd_se_play(12);

			// Only two of the three components, exactly as Marisa's defeat
			// does it -- the stage's own palette provides the green.
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
	reimu_1EBF3();
	hud_hp_update_and_render(boss.hp, REIMU_HP);
}