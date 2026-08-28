/// Stage 5 Boss - SinGyoku
/// -----------------------

#include "decomp.hpp"
#include "th01/rank.h"
#include "th01/resident.hpp"
#include "th01/v_colors.hpp"
#include "th01/math/clamp.hpp"
#include "th01/math/vector.hpp"
#include "th01/hardware/frmdelay.h"
#include "th01/hardware/palette.h"
#include "th01/snd/mdrv2.h"
#include "th01/formats/grp.h"
#include "th01/sprites/pellet.h"
#include "th01/main/particle.hpp"
#include "th01/main/hud/hp.hpp"
#include "th01/main/stage/timer.hpp"
#include "th01/main/player/player.hpp"
#include "th01/main/player/shot.hpp"
#include "th01/main/boss/defeat.hpp"
#include "th01/main/boss/b05.hpp"
#include "th01/main/boss/entity_a.hpp"
#include "th01/main/boss/palette.hpp"
#include "th01/main/bullet/pellet.hpp"
#include "th01/main/stage/palette.hpp"

#if T1REPLAY_CHECKPOINT_RESTORE || T1REPLAY_PIXEL_TRACE || T1REPLAY_PRACTICE_BOSS_PHASE
#include "th01/hardware/graph.h"
#endif

// Coordinates
// -----------

// SINGYOKU_W and SINGYOKU_H are defined in boss.hpp, as they are needed
// globally, by singyoku_defeat_animate_and_select_route() and as dummy default
// parameters for CBossEntity::pos_set().

static const screen_x_t BASE_CENTER_X = PLAYFIELD_CENTER_X;
static const screen_y_t BASE_CENTER_Y = (
	PLAYFIELD_TOP + ((PLAYFIELD_H / 21) * 5)
);

static const screen_x_t BASE_LEFT = (BASE_CENTER_X - (SINGYOKU_W / 2));
static const screen_y_t BASE_TOP  = (BASE_CENTER_Y - (SINGYOKU_H / 2));
// -----------

// Always denotes the last phase that ends with that amount of HP.
enum singyoku_hp_t {
	HP_TOTAL = 8,
	HP_PHASE_1_END = 6,
	HP_PHASE_2_END = 0,
};

// Global state that is defined here for some reason
// -------------------------------------------------

int8_t route; // ACTUAL TYPE: route_t
// -------------------------------------------------

// State that's suddenly no longer shared with other bosses
// --------------------------------------------------------

static int8_t boss_phase = 0;
static int boss_phase_frame;
static int invincibility_frame;
static int boss_hp;
// --------------------------------------------------------

// Entities
// --------

enum singyoku_form_t {
	F_WOMAN = 0,
	F_MAN = 1,

	_singyoku_form_t_FORCE_INT16 = 0x7FFF,
};

static const int SPHERE_CELS = 8;

// That's the position for the left hand, at least. The right hand would be a
// bit further in, but the game doesn't care.
static const pixel_t WOMAN_HAND_DISTANCE_FROM_EDGE = 16;

enum singyoku_flash_cel_t {
	C_SPHERE = 0,
	C_WOMAN = 1,
	C_MAN = 2,

	// Used for adding a singyoku_form_t on top.
	C_FLASH_FORM = C_WOMAN,
};

enum singyoku_person_cel_t {
	C_WOMAN_STILL = 0,
	C_WOMAN_ATTACK_1 = 1,
	C_WOMAN_ATTACK_2 = 2,
	C_MAN_STILL = 3,
	C_MAN_ATTACK = 4,

	// Used for multiplying with a singyoku_form_t.
	C_PERSON_FORM = (C_MAN_STILL - C_WOMAN_STILL),

	// Used for adding (C_PERSON_FORM * singyoku_form_t) on top.
	C_STILL = C_WOMAN_STILL,
	C_ATTACK_1 = C_WOMAN_ATTACK_1,
	C_ATTACK_2 = C_WOMAN_ATTACK_2,
};

#define ent_sphere \
	reinterpret_cast<CBossEntitySized<SINGYOKU_W, SINGYOKU_H> &>(boss_entity_0)

#define ent_flash 	boss_entity_1
#define ent_person	boss_entity_2

inline void singyoku_ent_load(void) {
	ent_sphere.load("boss1.bos", 0);
	ent_flash.load("boss1_2.bos", 1);
	ent_person.load("boss1_3.bos", 2);
}

inline void singyoku_ent_free(void) {
	bos_entity_free(0);
	bos_entity_free(1);
	bos_entity_free(2);
}

// And that's how you avoid the entity position synchronization code that
// plagues Elis: By simply only using a single set of coordinates.
#define ent ent_sphere

#define ent_unput_and_put(ent_with_cel, cel) { \
	ent_with_cel.unput_and_put_8(ent.cur_left, ent.cur_top, cel); \
}
// --------

// Patterns
// --------

static union {
	int pellet_count;
	pixel_t speed_in_pixels;
	subpixel_t speed_in_subpixels;
	int unknown;
} pattern_state;

// Originally function-local statics. Keeping these semantics private to this
// translation unit exposes the real SinGyoku owner without ever binding the
// common boss globals shadowed above.
static unsigned char halfcircle_angle;
static int8_t halfcircle_direction;
static point_t slam_velocity;

static struct {
	int pattern_cur;
	int16_t unused; // ZUN bloat

	void frame_common(void) {
		boss_phase_frame++;
		invincibility_frame++;
	}
} phase = { 0, 0 };

static struct {
	bool16 invincible;

	void update_and_render(const vc_t (&flash_colors)[1]) {
		boss_hit_update_and_render(
			invincibility_frame,
			invincible,
			boss_hp,
			flash_colors,
			(sizeof(flash_colors) / sizeof(flash_colors[0])),
			3000,
			boss_nop,
			ent.hittest_orb(),
			// A hitbox shifted 16 pixels to the right *and* with an
			// additional 16 pixels on the right edge?
			shot_hitbox_t(
				(ent.cur_left + (SINGYOKU_W / 6)),
				(ent.cur_top + (SINGYOKU_H / 3)),
				(SINGYOKU_W + (SINGYOKU_W / 6)),
				(SINGYOKU_H - (SINGYOKU_H / 3))
			)
		);
	}
} hit = { false };

static bool16 initial_hp_rendered = false;
// --------

void singyoku_load(void)
{
	svc2 col;
	int comp;

	singyoku_ent_load();

	grp_palette_load_show_sane("boss1.grp");
	palette_copy(boss_post_defeat_palette, z_Palettes, col, comp);
	stage_palette_set(boss_post_defeat_palette);

	void singyoku_setup(void);
	singyoku_setup();
}

void singyoku_setup(void)
{
	boss_palette_snap();
	z_palette_set_all_show(z_Palettes);

	ent.pos_set(PLAYFIELD_RIGHT, PLAYFIELD_TOP, 32);
	ent.hitbox_orb_set(
		((SINGYOKU_W / 12) *  1), ((SINGYOKU_H / 12) *  1),
		((SINGYOKU_W / 12) * 11), ((SINGYOKU_H / 12) * 11)
	);
	ent.hitbox_orb_inactive = false;
	ent_sphere.set_image(0);

	boss_hp = HP_TOTAL;
	hud_hp_first_white = HP_PHASE_1_END;
	hud_hp_first_redwhite = 2; // fully arbitrary, doesn't indicate anything
	boss_phase = 0;

	// ZUN bloat: Redundant, no particles are shown in this fight.
	particles_unput_update_render(PO_INITIALIZE, V_WHITE);
}

void singyoku_free(void)
{
	singyoku_ent_free();
}

// Rotates the sphere by the given [cel_delta]. [interval] could be used to
// restrict this function to certain [boss_phase_frame] intervals, but it's
// always either 1 or -1 in the original game.
void sphere_rotate_and_render(int interval, int cel_delta)
{
	if((boss_phase_frame % interval) != 0) {
		return;
	}

	// Yeah, why is the CBossEntity image variable 16 bits anywhere else to
	// begin with?
	int8_t image_new = (ent_sphere.image() + cel_delta);
	if(image_new > (SPHERE_CELS - 1)) {
		image_new = 0;
	} else if(image_new < 0) {
		image_new = (SPHERE_CELS - 1);
	}

	ent_sphere.set_image(image_new);
	ent_unput_and_put(ent_sphere, image_new);
}

#include "th01/main/select_r.cpp"

// Renders a frame of the sphere rotation, starting from a rotational speed of
// 0 and gradually speeding up.
void sphere_accelerate_rotation_and_render(int cel_delta)
{
	if(boss_phase_frame < 50) {
		ent_sphere.set_image(0);
		if((boss_phase_frame % 4) == 0) {
			ent_unput_and_put(ent_sphere, ent_sphere.image());
		}
		return;
	}
	if(boss_phase_frame == 50) {
		mdrv2_se_play(8);
	}
	if((boss_phase_frame < 100) && ((boss_phase_frame % 4) == 0)) {
		ent_unput_and_put(ent_sphere, ent_sphere.image());

		// Only 60 and 68 are actually divisible by 4. The other conditions
		// can never be true.
		if(
			(boss_phase_frame == 50) ||
			(boss_phase_frame == 60) ||
			(boss_phase_frame == 68) ||
			(boss_phase_frame == 74) ||
			(boss_phase_frame == 78) ||
			(boss_phase_frame == 82) ||
			(boss_phase_frame >  82)
		) {
			sphere_rotate_and_render(1, cel_delta);
		}
	}
}

void sphere_move_rotate_and_render(
	pixel_t delta_x, pixel_t delta_y, int interval = 1, int cel_delta = 1
)
{
	if(delta_y < 0) {
		egc_copy_rect_1_to_0_16(
			ent.cur_left,
			(ent.cur_top + delta_y + SINGYOKU_H),
			SINGYOKU_W,
			-delta_y
		);
	} else if(delta_y > 0) {
		egc_copy_rect_1_to_0_16(ent.cur_left, ent.cur_top, SINGYOKU_W, delta_y);
	}

	// ZUN landmine: Why implicitly limit [delta_x] to 8? (Which is actually at
	// least 16, due to egc_copy_rect_1_to_0_16() rounding up to the next
	// word.) The actual maximum value for [delta_x] that doesn't permanently
	// leave sphere parts in VRAM is 23 – at 24, a byte-aligned sphere moves at
	// a speed of 3 VRAM words every 2 frames, outrunning these unblitting
	// calls which only span a single VRAM word every frame in that case.
	if(delta_x > 0) {
		egc_copy_rect_1_to_0_16(ent.cur_left, ent.cur_top, 8, SINGYOKU_H);
	} else if(delta_x < 0) {
		// ZUN bug: Should be (+ delta_x) instead of (- delta_y). While the
		// latter is always positive whenever we get here, it can easily be
		// smaller than [delta_x] if SinGyoku is moving over a large amount of
		// horizontal space. In that case, [delta_y] is smaller, and word
		// alignment doesn't just not consistently cancel out this bug, but
		// in fact makes it worse: (ent.cur_left - delta_y) will then align to
		// a different word than (ent.cur_left + delta_x) on at least a couple
		// of frames during the animation, and the left edge of the unblitted
		// area will be past the right edge of the sphere in the previous
		// frame. As a result, not a single sphere pixel will be unblitted,
		// and a small stripe of the sphere will be left in VRAM for one frame.
		egc_copy_rect_1_to_0_16(
			(ent.cur_left + SINGYOKU_W - delta_y), ent.cur_top, 8, SINGYOKU_H
		);
	}

	// The calling site stops any positive Y movement as soon as SinGyoku's
	// bottom coordinate has reached the bottom of the playfield, so we can get
	// by without any clipping here.
	screen_y_t new_top = (ent.cur_top + delta_y);

	screen_x_t new_left = clamp_max_2(clamp_min_2(
		(ent.cur_left + delta_x), 0), (PLAYFIELD_RIGHT - SINGYOKU_W)
	);

	ent.cur_left = new_left;
	ent.cur_top = new_top;

	// ZUN bug: We unblit the movement delta every frame, but only blit every
	// second frame?! (Then again, this is what prevents sphere parts from
	// remaining in VRAM at [delta_x] values between 17 and 23 inclusive.)
	if((boss_phase_frame % 2) == 0) {
		sphere_rotate_and_render(interval, cel_delta);
	}
}

void pattern_halfcircle_spray_downwards(void)
{
	enum {
		KEYFRAME_FIRE = 100,
		KEYFRAME_FIRE_DONE = 160,

		FIRE_FRAMES = (KEYFRAME_FIRE_DONE - KEYFRAME_FIRE),
	};

	if(boss_phase_frame == 10) {
		halfcircle_direction = ((irand() % 2) == 1) ? 1 : -1;
	}
	if(boss_phase_frame < KEYFRAME_FIRE) {
		sphere_accelerate_rotation_and_render(halfcircle_direction);
		return;
	}

	if(boss_phase_frame == KEYFRAME_FIRE) {
		select_for_rank(pattern_state.pellet_count, 10, 15, 20, 30);
		halfcircle_angle = (halfcircle_direction == -1) ? 0x00 : 0x80;
	}
	if(boss_phase_frame < KEYFRAME_FIRE_DONE) {
		sphere_rotate_and_render(halfcircle_direction, 1);

		if(
			(boss_phase_frame % (FIRE_FRAMES / pattern_state.pellet_count)) == 0
		) {
			Pellets.add_single(
				(ent.cur_center_x() - (PELLET_W / 2)),
				(ent.cur_center_y() - (PELLET_H / 2)),
				halfcircle_angle,
				to_sp(3.125f)
			);
			halfcircle_angle -= (
				(halfcircle_direction * 0x80) / pattern_state.pellet_count
			);
		}
	} else {
		boss_phase_frame = 0;
	}
}

void pattern_slam_into_player_and_back_up(void)
{
	if(boss_phase_frame < 100) {
		sphere_accelerate_rotation_and_render(1);
		return;
	}
	if(boss_phase_frame == 100) {
		// Could be a local variable.
		select_for_rank(pattern_state.speed_in_pixels, 4, 4, 5, 6);

		vector2_between(
			(ent.cur_center_x() - (PLAYER_W / 2)),
			(ent.cur_center_y() - (PLAYER_H / 2)),
			player_left,
			player_top,
			slam_velocity.x,
			slam_velocity.y,
			pattern_state.speed_in_pixels
		);
	}

	// Leftover debug code?
	if(slam_velocity.x != -PIXEL_NONE) {
		sphere_move_rotate_and_render(
			inhibit_Z3(slam_velocity.x), slam_velocity.y
		);
		if(ent.cur_top > (PLAYFIELD_BOTTOM - SINGYOKU_H)) {
			// Nope, it's in fact a way to differentiate the two subphases of
			// this "pattern", and their completion conditions...
			slam_velocity.x = -PIXEL_NONE;

			// ... except that this variable also fulfills that job.
			slam_velocity.y = -4;
		}
	} else if(slam_velocity.y == -4) { // See?
		sphere_move_rotate_and_render(0, slam_velocity.y);
		// < rather than <= and no clamping? That makes sure that SinGyoku will
		// overshoot the base position.
		if(ent.cur_top < BASE_TOP) {
			slam_velocity.y = 0;
		}
	} else {
		boss_phase_frame = 0;
	}

	// A quadratic hitbox exactly covering all 96 pixels. Actually more lenient
	// than a perfect circular one.
	if(
		!player_invincible &&
		(ent.cur_left <= player_left) &&
		((ent.cur_left + (SINGYOKU_W - PLAYER_W)) >= player_left) &&
		(ent.cur_top >= (player_top - SINGYOKU_H))
	) {
		player_is_hit = true;
	}
}

enum singyoku_transform_keyframe_t {
	TKF_START = 100,
	TKF_TO_PERSON = 105,
	TKF_TO_PERSON_RERENDER = 110,
	TKF_TO_PERSON_DONE = 115,
	TKF_PERSON_ATTACK_1 = 135,
	TKF_EXTERNAL_PATTERN_START = 140,
	TKF_PERSON_ATTACK_2 = 160,
	TKF_PERSON_STILL = 185,
	TKF_EXTERNAL_PATTERN_DONE = 220,
	TKF_TO_SPHERE = 240,
	TKF_TO_SPHERE_RERENDER = 245,
	TKF_TO_SPHERE_DONE = 250,
	TKF_DONE = 260,
};

// Ends its corresponding pattern at TKF_DONE.
void transform_to_person_and_back_to_sphere(
	singyoku_form_t form,
	void pascal on_attack_1() = boss_nop,
	void pascal on_attack_2() = boss_nop,
	void pascal on_still() = boss_nop
)
{
	#define person_cel_for_form (C_PERSON_FORM * form)

	if(boss_phase_frame < TKF_START) {
		sphere_accelerate_rotation_and_render(1);
		return;
	}
	if(boss_phase_frame == TKF_START) {
		ent_unput_and_put(ent_flash, C_SPHERE);
	} else if(
		(boss_phase_frame == TKF_TO_PERSON) ||
		(boss_phase_frame == TKF_TO_PERSON_RERENDER)
	) {
		ent_unput_and_put(ent_flash, (C_FLASH_FORM + form));
	} else if(boss_phase_frame == TKF_TO_PERSON_DONE) {
		ent_unput_and_put(ent_person, (C_STILL + person_cel_for_form));
	} else if(boss_phase_frame == TKF_PERSON_ATTACK_1) {
		ent_person.set_image(C_ATTACK_1 + person_cel_for_form);
		ent_unput_and_put(ent_person, (C_ATTACK_1 + person_cel_for_form));
		on_attack_1();
	} else if(boss_phase_frame == TKF_PERSON_ATTACK_2) {
		// Suggests that there was a male version of C_ATTACK_2 during earlier
		// stages of development?
		ent_person.set_image(C_ATTACK_2 + person_cel_for_form);

		if(form == F_WOMAN) {
			ent_unput_and_put(ent_person, C_WOMAN_ATTACK_2);
		} else {
			ent_unput_and_put(ent_person, C_MAN_ATTACK);
		}
		on_attack_2();
	} else if(boss_phase_frame == TKF_PERSON_STILL) {
		ent_person.set_image(C_STILL + person_cel_for_form);
		ent_unput_and_put(ent_person, (C_STILL + person_cel_for_form));
		on_still();
	} else if(
		(boss_phase_frame == TKF_TO_SPHERE) ||
		(boss_phase_frame == TKF_TO_SPHERE_RERENDER)
	) {
		ent_unput_and_put(ent_flash, (C_FLASH_FORM + form));
	} else if(boss_phase_frame == TKF_TO_SPHERE_DONE) {
		ent_unput_and_put(ent_flash, C_SPHERE);
	} else if(boss_phase_frame == TKF_DONE) {
		ent_unput_and_put(ent_sphere, 0);
		boss_phase_frame = 0;
	}

	if(
		(boss_phase_frame > TKF_PERSON_ATTACK_1) &&
		(boss_phase_frame < TKF_TO_SPHERE) &&
		((boss_phase_frame % 4) == 0)
	) {
		ent_unput_and_put(ent_person, ent_person.image());
	}

	#undef person_cel_for_form
}

void pascal fire_chasing_pellets(void)
{
	subpixel_t chase_speed;

	select_subpixel_for_rank(chase_speed, 3.4375f, 3.625f, 3.875f, 4.0625f);

	Pellets.add_single(
		(ent.cur_center_x() - (PELLET_W / 2)),
		(ent.cur_center_y() - (PELLET_H / 2)),
		(0x00 - 0x10),
		to_sp(1.0f),
		PM_CHASE,
		chase_speed
	);
	Pellets.add_single(
		(ent.cur_center_x() - (PELLET_W / 2)),
		(ent.cur_center_y() - (PELLET_H / 2)),
		(0x80 + 0x10),
		to_sp(1.0f),
		PM_CHASE,
		chase_speed
	);
};

void pascal fire_crossing_pellets(void)
{
	subpixel_t speed;

	select_subpixel_for_rank(speed, 3.75f, 4.375f, 4.6875f, 5.0f);

	Pellets.add_single(
		(ent.cur_left    + WOMAN_HAND_DISTANCE_FROM_EDGE - (PELLET_W / 2)),
		(ent.cur_center_y() - (PELLET_H / 2)),
		(0x40 - 0x10),
		speed
	);
	Pellets.add_single(
		(ent.cur_right() - WOMAN_HAND_DISTANCE_FROM_EDGE + (PELLET_W / 2)),
		(ent.cur_center_y() - (PELLET_H / 2)),
		(0x40 + 0x10),
		speed
	);
}

void pattern_chasing_pellets(void)
{
	transform_to_person_and_back_to_sphere(F_WOMAN);
	if(
		(boss_phase_frame > TKF_EXTERNAL_PATTERN_START) &&
		((boss_phase_frame % 8) == 0) &&
		(boss_phase_frame <= TKF_EXTERNAL_PATTERN_DONE)
	) {
		fire_chasing_pellets();
	}
}

void pattern_crossing_pellets(void)
{
	transform_to_person_and_back_to_sphere(F_WOMAN);
	if(
		(boss_phase_frame > TKF_EXTERNAL_PATTERN_START) &&
		((boss_phase_frame % 8) == 0)
	) {
		fire_crossing_pellets();
	}
}

void pascal fire_random_downwards_pellets(void)
{
	// Could be a local variable.
	select_subpixel_for_rank(pattern_state.speed_in_subpixels,
		3.0f, 3.375f, 3.75f, 4.125f
	);

	for(int i = 0; i < 10; i++) {
		unsigned char angle = (irand() % 0x80u);
		Pellets.add_single(
			(ent.cur_center_x() - (PELLET_W / 2)),
			(ent.cur_center_y() - (PELLET_H / 2)),
			angle,
			pattern_state.speed_in_subpixels
		);
	}
}

void pascal fire_random_sling_pellets(void)
{
	// Could be a local variable.
	select_subpixel_for_rank(pattern_state.speed_in_subpixels,
		3.0f, 4.0f, 5.0f, 6.0f
	);

	for(int i = 0; i < 10; i++) {
		pixel_t offset_x = (irand() % SINGYOKU_W);
		pixel_t offset_y = (irand() % SINGYOKU_H);
		Pellets.add_single(
			(ent.cur_left + offset_x),
			(ent.cur_top + offset_y),
			0,
			0x00,
			PM_SLING_AIMED,
			pattern_state.speed_in_subpixels
		);
	}
}

void pattern_random_downwards_pellets(void)
{
	transform_to_person_and_back_to_sphere(
		F_MAN,
		fire_random_downwards_pellets,
		fire_random_downwards_pellets,
		fire_random_downwards_pellets
	);
}

void pattern_random_sling_pellets(void)
{
	transform_to_person_and_back_to_sphere(
		F_MAN,
		fire_random_sling_pellets,
		fire_random_sling_pellets,
		fire_random_sling_pellets
	);
}

void singyoku_main(void)
{
	const vc_t flash_colors[1] = { 13 };

	// Entrance animation
	if(boss_phase == 0) {
		ent.cur_left = BASE_LEFT;
		ent.cur_top = BASE_TOP;

		// MODDERS: Loop over a fade-in color array instead… and ideally, start
		// directly with this palette *before* first blitting the background?
		z_palette_set_show( 5, 0x0, 0x0, 0x0);
		z_palette_set_show( 9, 0x0, 0x0, 0x0);
		z_palette_set_show(15, 0x0, 0x0, 0x0);

		int comp = 0;
		int rotation_interval = 18;

		boss_phase_frame = 0;
		while(boss_phase_frame < 200) {
			// Different function for a change?
			ent_sphere.locked_unput_and_put_8();

			boss_phase_frame++;
			if((boss_phase_frame % rotation_interval) == 0) {
				sphere_rotate_and_render(1, 1);
				rotation_interval -= 2;
				if(rotation_interval <= 0) {
					rotation_interval = 1;
				}
			}
			if((boss_phase_frame % 20) == 0) {
				for(comp = 0; comp < COMPONENT_COUNT; comp++) {
					// MODDERS: Loop over a fade-in color array instead.
					if(z_Palettes[ 5].v[comp] < stage_palette[ 5].v[comp]) {
						z_Palettes[ 5].v[comp]++;
					}
					if(z_Palettes[ 9].v[comp] < stage_palette[ 9].v[comp]) {
						z_Palettes[ 9].v[comp]++;
					}
					if(z_Palettes[15].v[comp] < stage_palette[15].v[comp]) {
						z_Palettes[15].v[comp]++;
					}
				}
				z_palette_set_all_show(z_Palettes);
			}
			frame_delay(1);
		}
		boss_phase = 1;
		phase.pattern_cur = 0;
		phase.unused = 0;
		hit.invincible = false;
		boss_phase_frame = 0;
		initial_hp_rendered = false;
		boss_palette_show();
		stage_palette_set(z_Palettes);
		boss_palette_snap();
		ent.hitbox_orb_inactive = false;
		invincibility_frame = 0;

		// Huh?
		pattern_state.unknown = (
			(rank == RANK_EASY) ? 70 :
			(rank == RANK_NORMAL) ? 50 :
			(rank == RANK_HARD) ? 30 :
			(rank == RANK_LUNATIC) ? 10 :
			50
		);
	} else if(boss_phase == 1) {
		// Using the invincibility frame? That's unique. Works though, as it's
		// impossible in the original game to hit SinGyoku within the first 8
		// frames.
		hud_hp_increment_render(
			initial_hp_rendered, boss_hp, invincibility_frame
		);

		phase.frame_common();
		if(phase.pattern_cur == 0) {
			pattern_halfcircle_spray_downwards();
		} else if(phase.pattern_cur == 1) {
			pattern_slam_into_player_and_back_up();
		}

		if(boss_phase_frame == 0) {
			// (phase.pattern_cur = !phase.pattern_cur), anyone?
			phase.pattern_cur = (
				(phase.pattern_cur == 1) ? 0 : (phase.pattern_cur + 1)
			);
		}

		hit.update_and_render(flash_colors);

		if((boss_hp <= HP_PHASE_1_END) && !hit.invincible) {
			// Good catch – we don't want to stop the slam movement in the
			// middle of it, and leave SinGyoku somewhere below BASE_TOP.
			// (Conditionally setting [phase.pattern_cur] to 4 would have made
			// no difference anyway).
			if(phase.pattern_cur != 1)  {
				boss_phase = 2;
				phase.unused = 0;
				phase.pattern_cur = 0;
				boss_phase_frame = 0;
				invincibility_frame = 0; // ZUN bloat: Already 0 here
			}
		}
	} else if(boss_phase == 2) {
		phase.frame_common();
		if(phase.pattern_cur == 0) {
			pattern_chasing_pellets();
		} else if(phase.pattern_cur == 1) {
			pattern_random_downwards_pellets();
		} else if(phase.pattern_cur == 2) {
			pattern_crossing_pellets();
		} else if(phase.pattern_cur == 3) {
			pattern_random_sling_pellets();
		} else if(phase.pattern_cur == 4) {
			pattern_slam_into_player_and_back_up();
		}

		if(boss_phase_frame == 0) {
			// Cycle between pattern 4 and any non-4 pattern
			phase.pattern_cur = (phase.pattern_cur == 4) ? (irand() % 4) : 4;
		}

		hit.update_and_render(flash_colors);
		if(boss_hp <= HP_PHASE_2_END) {
			boss_phase = 8;
			mdrv2_se_play(5);
			boss_phase_frame = 0; // ZUN bloat: The fight is over
		}
	} else if(boss_phase == 8) {
		// This has no effect if SinGyoku was defeated in its sphere form, and
		// will otherwise blit the sphere on top of the active person form...
		// Oh well, maybe both entities *were* intended to be visible
		// simultaneously in this case?
		ent_sphere.put_8();

		mdrv2_bgm_fade_out_nonblock();
		Pellets.unput_and_reset_nonclouds();
		singyoku_defeat_animate_and_select_route();
	}
}

#pragma codeseg T1B05OWN_TEXT

bool16 t1boss_singyoku_checkpoint_validate(
	const t1boss_singyoku_checkpoint_t *checkpoint
)
{
	if(
		!checkpoint ||
		(checkpoint->owner != T1BOSS_SINGYOKU_CHECKPOINT_OWNER) ||
		(checkpoint->schema != T1BOSS_SINGYOKU_CHECKPOINT_SCHEMA) ||
		(checkpoint->reserved_0 != 0) ||
		(checkpoint->reserved[0] != 0) ||
		(checkpoint->reserved[1] != 0) ||
		((checkpoint->phase != 1) && (checkpoint->phase != 2)) ||
		(checkpoint->phase_frame < 0) ||
		(checkpoint->invincibility_frame < 0) ||
		(checkpoint->invincibility_frame >= BOSS_HIT_INVINCIBILITY_FRAMES) ||
		(checkpoint->hit_invincible > 1) ||
		(checkpoint->initial_hp_rendered > 1) ||
		(checkpoint->halfcircle_direction < -1) ||
		(checkpoint->halfcircle_direction > 1) ||
		(checkpoint->sphere_image < 0) ||
		(checkpoint->sphere_image >= SPHERE_CELS) ||
		(checkpoint->person_image < C_WOMAN_STILL) ||
		(checkpoint->person_image > C_MAN_ATTACK) ||
		(checkpoint->sphere_left < (PLAYFIELD_LEFT - SINGYOKU_W)) ||
		(checkpoint->sphere_left > PLAYFIELD_RIGHT) ||
		(checkpoint->sphere_top < (PLAYFIELD_TOP - SINGYOKU_H)) ||
		(checkpoint->sphere_top > PLAYFIELD_BOTTOM)
	) {
		return false;
	}
	if(checkpoint->phase == 1) {
		return (
			(checkpoint->hp >= HP_PHASE_1_END) &&
			(checkpoint->hp <= HP_TOTAL) &&
			(checkpoint->pattern_cur >= 0) &&
			(checkpoint->pattern_cur <= 1)
		);
	}
	return (
		(checkpoint->hp > HP_PHASE_2_END) &&
		(checkpoint->hp <= HP_PHASE_1_END) &&
		(checkpoint->pattern_cur >= 0) &&
		(checkpoint->pattern_cur <= 4)
	);
}

bool16 t1boss_singyoku_checkpoint_capture(
	t1boss_singyoku_checkpoint_t *checkpoint
)
{
	t1boss_singyoku_checkpoint_t live;

	if(!checkpoint) {
		return false;
	}
	live.owner = T1BOSS_SINGYOKU_CHECKPOINT_OWNER;
	live.schema = T1BOSS_SINGYOKU_CHECKPOINT_SCHEMA;
	live.phase = boss_phase;
	live.reserved_0 = 0;
	live.phase_frame = boss_phase_frame;
	live.hp = boss_hp;
	live.invincibility_frame = invincibility_frame;
	live.pattern_value = pattern_state.unknown;
	live.pattern_cur = phase.pattern_cur;
	live.hit_invincible = hit.invincible;
	live.initial_hp_rendered = initial_hp_rendered;
	live.slam_velocity_x = slam_velocity.x;
	live.slam_velocity_y = slam_velocity.y;
	live.sphere_left = ent.cur_left;
	live.sphere_top = ent.cur_top;
	live.halfcircle_angle = halfcircle_angle;
	live.halfcircle_direction = halfcircle_direction;
	live.sphere_image = ent_sphere.image();
	live.person_image = ent_person.image();
	live.reserved[0] = 0;
	live.reserved[1] = 0;
	if(!t1boss_singyoku_checkpoint_validate(&live)) {
		return false;
	}
	*checkpoint = live;
	return true;
}

bool16 t1boss_singyoku_ckpt_apply_loaded(
	const t1boss_singyoku_checkpoint_t *checkpoint
)
{
	if(!t1boss_singyoku_checkpoint_validate(checkpoint)) {
		return false;
	}

	ent.pos_set(checkpoint->sphere_left, checkpoint->sphere_top, 32);
	ent.prev_left = checkpoint->sphere_left;
	ent.prev_top = checkpoint->sphere_top;
	ent.prev_delta_x = 0;
	ent.prev_delta_y = 0;
	ent.lock_frame = 0;
	ent.hitbox_orb_set(
		((SINGYOKU_W / 12) * 1), ((SINGYOKU_H / 12) * 1),
		((SINGYOKU_W / 12) * 11), ((SINGYOKU_H / 12) * 11)
	);
	ent.hitbox_orb_inactive = false;
	ent_sphere.set_image(checkpoint->sphere_image);
	ent_person.set_image(checkpoint->person_image);
	hud_hp_first_white = HP_PHASE_1_END;
	hud_hp_first_redwhite = 2;

	boss_phase = checkpoint->phase;
	boss_phase_frame = checkpoint->phase_frame;
	boss_hp = checkpoint->hp;
	invincibility_frame = checkpoint->invincibility_frame;
	pattern_state.unknown = checkpoint->pattern_value;
	phase.pattern_cur = checkpoint->pattern_cur;
	phase.unused = 0;
	hit.invincible = checkpoint->hit_invincible;
	initial_hp_rendered = checkpoint->initial_hp_rendered;
	slam_velocity.x = checkpoint->slam_velocity_x;
	slam_velocity.y = checkpoint->slam_velocity_y;
	halfcircle_angle = checkpoint->halfcircle_angle;
	halfcircle_direction = checkpoint->halfcircle_direction;
	return true;
}

bool16 t1boss_singyoku_checkpoint_apply(
	const t1boss_singyoku_checkpoint_t *checkpoint
)
{
	if(!t1boss_singyoku_checkpoint_validate(checkpoint)) {
		return false;
	}
	// Rebuild pointer-backed .BOS slots from their native files. The snapshot
	// contains only the semantic positions and image IDs applied below.
	singyoku_ent_load();
	return t1boss_singyoku_ckpt_apply_loaded(checkpoint);
}

bool16 t1boss_singyoku_practice_boss_phase_apply(uint8_t target)
{
	t1boss_singyoku_checkpoint_t start;

	if(target != T1RPBPT_SINGYOKU_FIRST_COMBAT) {
		return false;
	}
	// These are the native values immediately after the phase-0 entrance:
	// 143 rotations from cel 0 leave the sphere at cel 7. The structure is
	// only a pointer-free owner carrier for the existing apply/painter path;
	// no serialized checkpoint enters this constructor.
	start.owner = T1BOSS_SINGYOKU_CHECKPOINT_OWNER;
	start.schema = T1BOSS_SINGYOKU_CHECKPOINT_SCHEMA;
	start.phase = 1;
	start.reserved_0 = 0;
	start.phase_frame = 0;
	start.hp = HP_TOTAL;
	start.invincibility_frame = 0;
	start.pattern_value = (
		(rank == RANK_EASY) ? 70 :
		(rank == RANK_NORMAL) ? 50 :
		(rank == RANK_HARD) ? 30 :
		(rank == RANK_LUNATIC) ? 10 :
		50
	);
	start.pattern_cur = 0;
	start.hit_invincible = false;
	start.initial_hp_rendered = false;
	start.slam_velocity_x = 0;
	start.slam_velocity_y = 0;
	start.sphere_left = BASE_LEFT;
	start.sphere_top = BASE_TOP;
	start.halfcircle_angle = 0;
	start.halfcircle_direction = 0;
	start.sphere_image = (143 % SPHERE_CELS);
	start.person_image = C_WOMAN_STILL;
	start.reserved[0] = 0;
	start.reserved[1] = 0;
	if(!t1boss_singyoku_ckpt_apply_loaded(&start)) {
		return false;
	}
	boss_palette_show();
	stage_palette_set(z_Palettes);
	boss_palette_snap();
	return t1boss_singyoku_presentation_reconstruct(&start);
}

#if T1REPLAY_CHECKPOINT_RESTORE || T1REPLAY_PIXEL_TRACE || T1REPLAY_PRACTICE_BOSS_PHASE
static int t1boss_singyoku_presentation_person_cel(
	const t1boss_singyoku_checkpoint_t *checkpoint
)
{
	int form = ((checkpoint->pattern_cur & 1) ? F_MAN : F_WOMAN);

	if(checkpoint->phase_frame < TKF_PERSON_ATTACK_1) {
		return (C_STILL + (C_PERSON_FORM * form));
	}
	if(checkpoint->phase_frame < TKF_PERSON_ATTACK_2) {
		return (C_ATTACK_1 + (C_PERSON_FORM * form));
	}
	if(checkpoint->phase_frame < TKF_PERSON_STILL) {
		return ((form == F_WOMAN) ? C_WOMAN_ATTACK_2 : C_MAN_ATTACK);
	}
	return (C_STILL + (C_PERSON_FORM * form));
}

bool16 t1boss_singyoku_presentation_validate(
	const t1boss_singyoku_checkpoint_t *checkpoint
)
{
	if(
		!t1boss_singyoku_checkpoint_validate(checkpoint) ||
		checkpoint->hit_invincible ||
		(!checkpoint->initial_hp_rendered &&
		 (checkpoint->invincibility_frame >= checkpoint->hp)) ||
		((checkpoint->phase == 2) && !checkpoint->initial_hp_rendered)
	) {
		return false;
	}
	if(checkpoint->phase == 1) {
		return !(
			(checkpoint->pattern_cur == 1) &&
			(checkpoint->phase_frame >= 100) &&
			(checkpoint->phase_frame & 1)
		);
	}
	if(checkpoint->pattern_cur == 4) {
		return !(
			(checkpoint->phase_frame >= 100) &&
			(checkpoint->phase_frame & 1)
		);
	}
	return (
		(checkpoint->phase_frame < TKF_START) ||
		((checkpoint->phase_frame >= TKF_TO_PERSON_DONE) &&
		 (checkpoint->phase_frame < TKF_TO_SPHERE))
	);
}

static bool t1boss_singyoku_presentation_live_matches(
	const t1boss_singyoku_checkpoint_t *checkpoint
)
{
	return (
		(boss_phase == checkpoint->phase) &&
		(boss_phase_frame == checkpoint->phase_frame) &&
		(boss_hp == checkpoint->hp) &&
		(invincibility_frame == checkpoint->invincibility_frame) &&
		(phase.pattern_cur == checkpoint->pattern_cur) &&
		(pattern_state.unknown == checkpoint->pattern_value) &&
		(hit.invincible == checkpoint->hit_invincible) &&
		(ent.cur_left == checkpoint->sphere_left) &&
		(ent.cur_top == checkpoint->sphere_top) &&
		(ent_sphere.image() == checkpoint->sphere_image) &&
		(ent_person.image() == checkpoint->person_image)
	);
}

bool16 t1boss_singyoku_presentation_reconstruct(
	const t1boss_singyoku_checkpoint_t *checkpoint
)
{
	if(
		!t1boss_singyoku_presentation_validate(checkpoint) ||
		!t1boss_singyoku_presentation_live_matches(checkpoint)
	) {
		return false;
	}

	// Native startup has rebuilt page 1 as the static unput backing. Select it
	// first and rebuild page 0 from it, but never paint SinGyoku on page 1.
	graph_accesspage_func(1);
	graph_copy_accessed_page_to_other();
	graph_accesspage_func(0);
	// The pre-applied scenario already rebuilt the remaining HUD fields. The
	// timer is imported later, and the boss entrance that normally builds the
	// HP backing is skipped, so reconstruct those two presentation resources.
	timer_put();
	hud_hp_rerender(
		checkpoint->initial_hp_rendered
			? checkpoint->hp
			: checkpoint->invincibility_frame
	);
	if(
		(checkpoint->phase == 2) && (checkpoint->pattern_cur != 4) &&
		(checkpoint->phase_frame >= TKF_TO_PERSON_DONE)
	) {
		ent_person.put_8(
			checkpoint->sphere_left,
			checkpoint->sphere_top,
			t1boss_singyoku_presentation_person_cel(checkpoint)
		);
	} else {
		ent_sphere.put_8(
			checkpoint->sphere_left,
			checkpoint->sphere_top,
			checkpoint->sphere_image
		);
	}
	graph_accesspage_func(0);
	return true;
}
#endif

#pragma codeseg
