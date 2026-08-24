/// Stage 3 midboss - update function
/// ---------------------------------
/// (#included from th04/main/midboss/mb_upd.cpp, which is the single object
/// for the whole of MB_UPD_TEXT and holds every header this file needs. See
/// th04/main/midboss/m1_updt.cpp, which is included immediately ahead of it,
/// for why. File-scope names here are NOT file-local; prefix every one.)
///
/// midboss3_render() is th04/main/midboss/m3.cpp, in a different segment and
/// therefore a different object. It carries its own copy of the two constants
/// below under the shorter names its own defines give them.

// Constants
// ---------

// One flight direction per interlude, and the fight ends after the last of
// them. Same value and same array as th04/main/midboss/m3.cpp's PATTERNS_MAX
// and FLY_ANGLES; th04_main.asm's own MIDBOSS3_PATTERNS_MAX equate had this
// function as its only reader and is gone with it.
static const uint8_t MIDBOSS3_PATTERNS_MAX = 12;
// ---------

/// The midboss's own state
/// -----------------------
/// The first three are th04_main.asm `.data?` bytes with no `public` of ZUN's,
/// and this function plus the four patterns above it are the only readers or
/// writers of any of them in any of the five binaries.
///
/// [midboss3_pattern] is the Stage 2 and Stage 4 midbosses' byte exactly --
/// same role, same `& 3` cycle off the interlude counter, same `N - passes`
/// score bonus -- so it takes th04/main/midboss/m4_updt.cpp's already-ruled
/// spelling rather than a new name, the way m2_updt.cpp took it. The mirror
/// flag has no counterpart there and keeps the dump's address-suffixed
/// spelling; **a naming round is owed** for it.
extern "C" {
	// Which of the four patterns is running, or 255 during the gather-and-fly
	// interlude between two of them.
	extern unsigned char midboss3_pattern;

	// If nonzero, every flight direction is mirrored around straight down.
	// Rolled once, at the end of the entrance, and never again. `[inferred]`.
	extern unsigned char midboss3_25599;

	// Interludes completed. The score bonus for killing it is `20 - this`, and
	// the last one ends the fight whether or not the player got there.
	extern unsigned char midboss3_patterns_done;

	extern const unsigned char MIDBOSS3_FLY_ANGLES[MIDBOSS3_PATTERNS_MAX];
}
/// -----------------------

// Both halves of the fight test the same box; only the entrance's se_on_hit
// differs, exactly like the Stage 4 midboss's.
#define midboss3_hittest(se_on_hit) \
	midboss_hittest_shots_damage(TO_SP(24), TO_SP(24), se_on_hit)

/// The Stage 3 midboss's four bullet patterns
/// ------------------------------------------
/// All four sat directly above midboss3_update() in ZUN's object, and every
/// one of them is reached from its `switch(midboss3_pattern)` and from nowhere
/// else, so all four are `static` here and the four zero-byte `label` aliases
/// th04_main.asm carried for them are gone with the bodies. They keep the
/// dump's address-suffixed names; **a naming round is owed for all four**, on
/// the same terms as the Stage 2 and Stage 4 midbosses'.
///
/// All four share one skeleton: a modulo on the phase frame gates the volley,
/// and a compare against 32 hands control back to midboss3_update() by setting
/// [midboss3_pattern] to 255.

// An aimed 32-way ring on the 2nd frame, then a 2-way spread along the same
// angle on every 4th frame.
static void near midboss3_142F1(void)
{
	if(midboss.phase_frame == 2) {
		midboss.angle = iatan2(
			(player_pos.cur.y.v - midboss.pos.cur.y.v),
			(player_pos.cur.x.v - midboss.pos.cur.x.v)
		);
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.group = BG_RING;
		bullet_template.count = 32;
		bullet_template.speed.v = (TO_SP(2) + 8);
		bullet_template.angle = 0;
		bullet_template_tune();
		bullets_add_regular();
	}
	if((midboss.phase_frame % 4) == 0) {
		bullet_template.group = BG_SPREAD;
		bullet_template.count = 2;
		bullet_template.delta.spread_angle = 0x0C;
		bullet_template.speed.v = (TO_SP(3) + 4);
		bullet_template.angle = midboss.angle;
		bullet_template_tune();
		bullets_add_regular();
		snd_se_play(3);
	}
	if(midboss.phase_frame >= 32) {
		midboss3_pattern = 255;
		midboss.phase_frame = 0;
	}
}

// The one that fires nothing at all for 32 frames and then a single backwards
// cloud ring as it ends.
static void near midboss3_14383(void)
{
	if(midboss.phase_frame >= 32) {
		midboss3_pattern = 255;
		midboss.phase_frame = 0;
		bullet_template.spawn_type = BST_BULLET16_CLOUD_BACKWARDS;
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.group = BG_RING;
		bullet_template.count = 32;
		bullet_template.speed.v = (TO_SP(2) + 8);
		bullet_template.angle = randring2_next16();
		bullet_template_tune();
		bullets_add_regular_fixedspeed();
		snd_se_play(9);
	}
}

// A 2-way spread every other frame, sweeping clockwise from straight down.
static void near midboss3_143C7(void)
{
	if(midboss.phase_frame == 1) {
		midboss.angle = 128;
	}
	if((midboss.phase_frame % 2) == 0) {
		bullet_template.group = BG_SPREAD;
		bullet_template.count = 2;
		bullet_template.delta.spread_angle = 0x12;
		bullet_template.speed.v = (TO_SP(2) + 14);

		// kb/codegen/0032: the angle stays live in AL across the store into
		// the template, so the sweep is spelled through the pseudo-register.
		// 0xF8 rather than a subtraction of 8: on a wrapping 8-bit angle the
		// two are the same value, but the subtraction promotes and comes out
		// as a same-length SUB where the original has an ADD.
		_AL = midboss.angle;
		bullet_template.angle = _AL;
		_AL += 0xF8;
		midboss.angle = _AL;

		bullet_template_tune();
		bullets_add_regular();
		snd_se_play(3);
	}
	if(midboss.phase_frame >= 32) {
		midboss3_pattern = 255;
		midboss.phase_frame = 0;
	}
}

// A 24-way ring every 6th frame, each one rotated 6 further than the last.
static void near midboss3_14425(void)
{
	if(midboss.phase_frame == 1) {
		midboss.angle = randring2_next16();
	}
	if((midboss.phase_frame % 6) == 0) {
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.patnum = PAT_BULLET16_D_BLUE;
		bullet_template.group = BG_RING;
		bullet_template.speed.v = TO_SP(2);
		bullet_template.angle = midboss.angle;
		bullet_template.count = 24;
		bullet_template_tune();
		bullets_add_regular_fixedspeed();
		snd_se_play(9);
		_AL = midboss.angle;
		_AL += 6;
		midboss.angle = _AL;
	}
	if(midboss.phase_frame >= 32) {
		midboss3_pattern = 255;
		midboss.phase_frame = 0;
	}
}
/// ------------------------------------------

// `#pragma option -a2` is the one padding byte between this function's
// epilogue and its generated value/jump table pair. kb/codegen/0160 for the
// instrument: read the OBJ's PUBDEF offsets, not the `tcc -S` listing.
#pragma option -a2
void pascal far midboss3_update(void)
{
	int damage;
	unsigned char angle;

	homing_target.x.v = midboss.pos.cur.x.v;
	homing_target.y.v = midboss.pos.cur.y.v;

	if(midboss.phase == 0) {
		// The entrance: invincible until it stops, but the hittest still runs.
		midboss.pos.update_seg3();
		midboss.phase_frame++;
		damage = midboss3_hittest(10); // ZUN bloat: never read
		if(midboss.phase_frame >= 20) {
			midboss.phase++;
			midboss.phase_frame = 0;
			midboss.pos.velocity.x.v = 0;
			midboss.pos.velocity.y.v = 0;
			midboss3_pattern = randring2_next16_and(3);
			midboss3_25599 = randring2_next16_and(1);
			midboss3_patterns_done = 0;
		}
	} else if(midboss.phase == 1) {
		midboss.pos.update_seg3();
		midboss.phase_frame++;
		bullet_template.spawn_type = BST_PELLET;
		bullet_template.origin.x.v = midboss.pos.cur.x.v;
		bullet_template.origin.y.v = (midboss.pos.cur.y.v - TO_SP(16));

		switch(midboss3_pattern) {
		case 0:
			midboss3_142F1();
			break;
		case 1:
			midboss3_14383();
			break;
		case 2:
			midboss3_143C7();
			break;
		case 3:
			midboss3_14425();
			break;
		case 255:
			// The interlude: a gather circle, and a flight to wherever
			// MIDBOSS3_FLY_ANGLES points this time. Skipped entirely once the
			// midboss has run out of patterns, which leaves the velocity from
			// the last flight in place and eventually carries it off the
			// playfield -- that is what times the fight out below.
			if(midboss3_patterns_done <= (MIDBOSS3_PATTERNS_MAX - 1)) {
				gather_template.center.x.v = midboss.pos.cur.x.v;
				gather_template.center.y.v = midboss.pos.cur.y.v;
				gather_add_only_3stack(
					(midboss.phase_frame - 64), V_WHITE, 9
				);
				switch(midboss.phase_frame) {
				case 64:
					midboss.pos.velocity.x.v = 0;
					midboss.pos.velocity.y.v = 0;
					break;

				case 68:
					midboss.phase_frame = 0;
					_AL = midboss3_patterns_done;
					_AL &= 3;
					midboss3_pattern = _AL;
					midboss.sprite = 0;
					break;
				}
			}
			if(midboss.phase_frame == 1) {
				angle = MIDBOSS3_FLY_ANGLES[midboss3_patterns_done];
				if(midboss3_25599 != 0) {
					_AL = 0x80;
					_AL -= angle;
					angle = _AL;
				}
				vector2(
					midboss.pos.velocity.x.v,
					midboss.pos.velocity.y.v,
					angle,
					TO_SP(2)
				);
				midboss3_patterns_done++;
				midboss.sprite = 1;
				gather_template.ring_points = 8;
			}
			break;
		}

		// Time out the fight if the midboss flew off the playfield.
		// Only supposed to happen as a result of uninterrupted movement after
		// the midboss completed the maximum amount of patterns.
		if(
			(midboss.pos.cur.y.v >= to_sp(PLAYFIELD_H)) ||
			(midboss.pos.cur.x.v <= 0) ||
			(midboss.pos.cur.x.v >= to_sp(PLAYFIELD_W))
		) {
			midboss.phase = 3;
		}
		damage = midboss3_hittest(4);
		if(damage) {
			midboss.hp -= damage;
			if(midboss.hp > 0) {
				midboss.damage_this_frame = 1;
			} else {
				midboss.damage_this_frame = 1;

				// ZUN bloat: never read, and it reuses the damage slot.
				damage = scroll_subpixel_y_to_vram_always(
					(midboss.pos.cur.y.v - TO_SP(16))
				);
				bullet_zap.active = true;
				midboss_score_bonus(20 - midboss3_patterns_done);
				midboss.phase = PHASE_EXPLODE_BIG;
				midboss.sprite = 4;
				midboss.phase_frame = 0;
				midboss.pos.velocity.x.v = 0;
				sparks_add_circle(
					midboss.pos.cur.x, midboss.pos.cur.y, TO_SP(6), 48
				);
				snd_se_play(12);
				items_add(
					midboss.pos.cur.x.v, midboss.pos.cur.y.v, IT_1UP
				);
			}
		}
	} else {
		midboss_defeat_update();
	}
	hud_hp_update_and_render(midboss.hp, 850);
}
#pragma option -a1

#undef midboss3_hittest
