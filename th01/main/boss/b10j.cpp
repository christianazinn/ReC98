/// Jigoku Stage 10 Boss - Mima
/// ---------------------------

#include "th01/rank.h"
#include "th01/resident.hpp"
#include "th01/v_colors.hpp"
#include "th01/math/dir.hpp"
#include "th01/math/polar.hpp"
#include "th01/math/vector.hpp"
#include "th01/hardware/frmdelay.h"
#include "th01/hardware/graph.h"
#include "th01/hardware/palette.h"
#include "th01/snd/mdrv2.h"
#include "th01/formats/grp.h"
#include "th01/sprites/pellet.h"
#include "th01/sprites/pillar.hpp"
#include "th01/main/particle.hpp"
#include "th01/main/shape.hpp"
#include "th01/main/boss/defeat.hpp"
#include "th01/main/boss/b10j.hpp"
#include "th01/main/boss/entity_a.hpp"
#include "th01/main/boss/palette.hpp"
#include "th01/main/bullet/laser_s.hpp"
#include "th01/main/bullet/missile.hpp"
#include "th01/main/bullet/pellet.hpp"
#include "th01/main/hud/hp.hpp"
#include "th01/main/player/player.hpp"
#include "th01/main/player/shot.hpp"
#include "th01/main/stage/palette.hpp"
#include "th01/main/stage/stages.hpp"

// Coordinates
// -----------

static const pixel_t MIMA_W = 128;
static const pixel_t MIMA_H = 160;

// Including the sprite:
//
// 	               [w]
// 	    ┌───────────────────────┐
// 	    │                       │
// 	[h] │                       │
// 	    │             sprite → ┌┤
// 	    └──────────────────────┴┘
static const pixel_t HITBOX_SHOT_W = (MIMA_W - (MIMA_W / 8));
static const pixel_t HITBOX_SHOT_H = ((MIMA_H * 2) / 5);

static const pixel_t MIMA_ANIM_TOP = 48; // relative to the sprite's top edge
static const pixel_t MIMA_ANIM_H = 64;

static const screen_x_t BASE_CENTER_X = PLAYFIELD_CENTER_X;
static const screen_y_t BASE_CENTER_Y = (
	PLAYFIELD_TOP + ((PLAYFIELD_H / 42) * 17)
);

static const screen_x_t BASE_LEFT = (BASE_CENTER_X - (MIMA_W / 2));
static const screen_y_t BASE_TOP = (BASE_CENTER_Y - (MIMA_H / 2));

// Not quite matching the image, but close enough.
static const pixel_t SEAL_RADIUS = 80;

// The radius of the circumscribed square around a circle with radius 𝓇 is
// 𝓇 * √2; see https://www.desmos.com/calculator/u8mtn9y9wo.
static const pixel_t SEAL_CIRCUMSQUARE_RADIUS = (SEAL_RADIUS * 1.41f);
// -----------

enum mima_colors_t {
	COL_SPAWNRAY = 1,
	COL_PILLAR = V_RED,
};

// Always denotes the last phase that ends with that amount of HP.
enum mima_hp_t {
	HP_TOTAL = 12,
	HP_PHASE_1_END = 6,
	HP_PHASE_3_END = 0,
};

// State
// -----

// Whether meteor_put() has any effect.
bool meteor_active = true;

// Amount of frames between the individual steps of the spread-in transition
uint8_t spreadin_interval = 4;

// Sprite pixels to spread in per frame, in one half of Mima's sprite
pixel_length_8_t spreadin_speed = 8;
// -----

// Entities
// --------

static const int METEOR_CELS = 4;

enum anim_cel_t {
	C_CAST = 0,
	C_METEOR = 1,
	C_METEOR_last = (C_METEOR + METEOR_CELS - 1),
};

#define ent_still \
	reinterpret_cast<CBossEntitySized<MIMA_W, MIMA_H> &>(boss_entity_0)

#define ent_anim 	boss_entity_1

inline void mima_ent_load(void) {
	ent_still.load("boss3_1.bos", 0);
	ent_anim.load("boss3_2.bos", 1);
}

inline void mima_ent_free(void) {
	bos_entity_free(0);
	bos_entity_free(1);
}
// --------

// .PTN
// ----

static const main_ptn_slot_t PTN_SLOT_BG_ENT = PTN_SLOT_BOSS_1;
static const main_ptn_slot_t PTN_SLOT_MISSILE = PTN_SLOT_BOSS_2;

// ZUN bloat: Three unused background .PTN IDs, for three unused 32×32
// animations?
static const int BG_ENT_OFFSET = 3;
// ----

// Patterns
// --------

static union {
	subpixel_t speed;
	DecimalSubpixel speed_decimal;
	int speed_multiplied_by_8;
	int unused;
} pattern_state;
// --------

void mima_load(void)
{
	svc2 col;
	int comp;

	mima_ent_load();
	grp_palette_load_show("boss3.grp");
	palette_copy(boss_post_defeat_palette, z_Palettes, col, comp);
	void mima_setup(void);
	mima_setup();
	ptn_new(
		PTN_SLOT_BG_ENT,
		(((MIMA_W / PTN_W) * (MIMA_H / PTN_H)) + BG_ENT_OFFSET + 1)
	);
	Missiles.load(PTN_SLOT_MISSILE);
	Missiles.reset();
}

inline void ent_anim_sync_with_still(void) {
	ent_anim.pos_cur_set(
		ent_still.cur_left, (ent_still.cur_top + MIMA_ANIM_TOP)
	);
}

inline void ent_anim_sync_with_still_and_put_both(int cel) {
	ent_anim_sync_with_still();
	ent_anim.set_image(cel);
	graph_accesspage_func(1);	ent_anim.put_8(cel);
	graph_accesspage_func(0);	ent_anim.put_8(cel);
}

void meteor_put(void)
{
	if(meteor_active && ((boss_phase_frame % 8) == 0)) {
		ent_anim_sync_with_still();
		ent_anim.set_image_unput_and_put_8(
			(C_METEOR + ((boss_phase_frame / 8) % METEOR_CELS))
		);
	}
}

void mima_put_cast_both(void)
{
	// ZUN bug: Does not unblit the meteor if `true`, and C_CAST does not
	// completely overlap any C_METEOR cel. In that case, small parts of the
	// meteor are guaranteed to be left in VRAM until they're unblitted as a
	// result of another sprite flying over them.
	meteor_active = false;

	ent_anim_sync_with_still_and_put_both(C_CAST);
}

void meteor_activate(void)
{
	if(!meteor_active) {
		meteor_active = true;
		ent_anim_sync_with_still_and_put_both(C_METEOR);
	}
}

void mima_put_still_both(void)
{
	graph_accesspage_func(1);	ent_still.put_8();
	graph_accesspage_func(0);	ent_still.put_8();
}

void mima_bg_snap(void)
{
	int ptn_x;
	int ptn_y;
	screen_x_t left = ent_still.cur_left;
	screen_y_t top = ent_still.cur_top;
	int image = BG_ENT_OFFSET;

	ptn_snap_rect_from_1_8(
		left, top, MIMA_W, MIMA_H, PTN_SLOT_BG_ENT, image, ptn_x, ptn_y
	);
}

void mima_unput(bool16 just_the_animated_part = false)
{
	int ptn_x;
	int image = BG_ENT_OFFSET;
	screen_x_t left = ent_still.cur_left;
	screen_y_t top = ent_still.cur_top;

	if(!just_the_animated_part) {
		int ptn_y;
		ptn_put_rect_noalpha_8(
			left, top, MIMA_W, MIMA_H, PTN_SLOT_BG_ENT, image, ptn_x, ptn_y
		);
		return;
	}

	// (The code below is never executed in the original game.)

	// Advance to the .PTN background row that contains the background behind
	// the animating part of Mima's sprite (i.e., the second one)
	image = (BG_ENT_OFFSET + ((MIMA_ANIM_TOP / PTN_H) * (MIMA_W / PTN_W)));

	// And since MIMA_ANIM_TOP is only a multiple of 16 and not 32, we have to
	// first awkwardly unblit a MIMA_W×16 area...
	for(ptn_x = 0; ptn_x < (MIMA_W / PTN_W); ptn_x++) {
		ptn_put_quarter_noalpha_8(
			(left + (0 * PTN_QUARTER_W) + (ptn_x * PTN_W)),
			(top + MIMA_ANIM_TOP),
			PTN_ID(PTN_SLOT_BG_ENT, image),
			(((MIMA_ANIM_TOP % PTN_H) / PTN_QUARTER_H) * 2)
		);
		ptn_put_quarter_noalpha_8(
			(left + (1 * PTN_QUARTER_W) + (ptn_x * PTN_W)),
			(top + MIMA_ANIM_TOP),
			PTN_ID(PTN_SLOT_BG_ENT, image),
			((((MIMA_ANIM_TOP % PTN_H) / PTN_QUARTER_H) * 2) + 1)
		);
		image++;
	}

	// ZUN landmine: Why is MIMA_ANIM_H assumed to be 48 (16 above + 32 here)?
	// This might have even worked if the bottom 16 pixels of all [ent_anim]
	// cels were identical, but they differ between C_CAST and C_METEOR.
	//
	// Note that this has nothing to do with Mima's infamous "third arm"
	// (remember, the game never executes this code), but wouldn't exactly
	// prevent it from happening either.
	#define bug_top (top + MIMA_ANIM_TOP + PTN_QUARTER_H)
	ptn_put_row_noalpha_8(left, bug_top, MIMA_W, PTN_SLOT_BG_ENT, image, ptn_x);
	#undef bug_top
}

inline void mima_unput_both(void) {
	graph_accesspage_func(1);	mima_unput();
	graph_accesspage_func(0);	mima_unput();
}

inline pixel_t spreadin_bottom_cur(void) {
	return ((spreadin_speed / spreadin_interval) * (boss_phase_frame - 10));
}

static const int KEYFRAME_SPREADIN_START = 10;

// Renders a frame of the spread-in animation that ultimately ends in a full
// Mima at the given position. Sets [boss_phase_frame] to 0 to indicate that
// the animation completed.
void phase_spreadin(screen_x_t final_left, screen_y_t final_top)
{
	pixel_t row;
	pixel_t line_on_top;

	if(boss_phase_frame < KEYFRAME_SPREADIN_START) {
		return;
	} else if(boss_phase_frame == KEYFRAME_SPREADIN_START) {
		ent_still.pos_cur_set(final_left, final_top);
		mima_bg_snap();
		line_on_top = (final_top + (MIMA_H / 2));
		return;
	} else if((boss_phase_frame % spreadin_interval) != 0) {
		return;
	}

	line_on_top = ((MIMA_H / 2) - spreadin_bottom_cur());
	if(line_on_top < 0) {
		boss_phase_frame = 0;
		mima_put_still_both();
		return;
	}
	for(row = 0; spreadin_bottom_cur() > row; row++) {
		ent_still.unput_and_put_1line(
			final_left, (final_top + line_on_top + row), ent_still.image(), row
		);
		ent_still.unput_and_put_1line(
			final_left,
			((final_top + MIMA_H) - line_on_top - row),
			ent_still.image(),
			((MIMA_H - 1) - row)
		);
	}
}

// Only called while Mima isn't visible anyway. But even apart from that, it
// barely would have any effect anywhere, as the Mima sprite is blitted to both
// VRAM pages. This *might* have been supposed to crossfade between various
// cels? …Nah, why would you do that by blitting whole lines.
void mima_vertical_sprite_transition_broken(void)
{
	if((boss_phase_frame < 10) || ((boss_phase_frame % 4) != 0)) {
		return;
	}
	pixel_t half_h = ((boss_phase_frame - 10) * 2);
	if(half_h >= (MIMA_H / 2)) {
		boss_phase_frame = 0;
		return;
	}
	// And besides, *VRAM width*?! This is completely broken.
	egc_copy_rect_1_to_0_16(
		ent_still.cur_left, (ent_still.cur_top + half_h), ent_still.vram_w, 8
	);
	egc_copy_rect_1_to_0_16(
		ent_still.cur_left,
		(ent_still.cur_top + (MIMA_H - 8) - half_h),
		ent_still.vram_w,
		8
	);
}

void mima_setup(void)
{
	boss_palette_snap();
	ent_still.set_image(0);
	ent_anim.set_image(C_METEOR);

	// Since we always come here with VRAM fully set to color #0, this is
	// always a flash from white back to color #0 in boss3.grp.
	z_palette_white_in();

	ent_still.pos_set((PLAYFIELD_CENTER_X - (MIMA_W / 2)), PLAYFIELD_TOP);
	ent_still.hitbox_orb_set(
		((MIMA_W / 8) * 1), ((MIMA_H / 10) * 1),
		((MIMA_W / 8) * 7), ((MIMA_H / 10) * 9)
	);
	ent_still.hitbox_orb_inactive = false;
	boss_phase_frame = 0;
	boss_phase = 0;
	boss_hp = HP_TOTAL;
	hud_hp_first_white = HP_PHASE_1_END;
	hud_hp_first_redwhite = 2;  // fully arbitrary, doesn't indicate anything
	particles_unput_update_render(PO_INITIALIZE, V_WHITE);
}

void mima_free(void)
{
	mima_ent_free();
	ptn_free(PTN_SLOT_BG_ENT);
	ptn_free(PTN_SLOT_MISSILE);
}

#include "th01/main/select_r.cpp"

// Rotating square
// ---------------

static const int SQUARE_POINTS = 4;
static const int SQUARE_INTERVAL = 8;
static const pixel_t SQUARE_RADIUS_PER_FRAME = 1;
static const pixel_t SQUARE_RADIUS_STEP = (
	SQUARE_INTERVAL * SQUARE_RADIUS_PER_FRAME
);

void pascal near regular_polygon(
	screen_x_t *corners_x,
	screen_y_t *corners_y,
	screen_x_t center_x,
	screen_y_t center_y,
	pixel_t radius,
	unsigned char angle,
	int points
)
{
	for(int i = 0; i < points; i++) {
		corners_x[i] = polar_x(center_x, radius, angle);
		corners_y[i] = polar_y(center_y, radius, angle);
		angle += (0x100 / points);
	}
}

struct SquareState {
	unsigned char angle;
	pixel_t radius;

	void init(void) {
		radius = static_cast<pixel_t>(SEAL_RADIUS * 0.4f);
		angle = 0x00;
	}
};

// These were originally function-local statics. Naming their semantic owners
// keeps the checkpoint codec independent of b10j.cpp's BSS layout.
static SquareState square_aimed_pellets;
static SquareState square_aimed_missiles;
static SquareState square_two_pellets;
static SquareState square_halfcircle_missiles;
static SquareState square_slow_spray;
static SquareState square_lasers;
static screen_x_t mima_target_left;
static uint8_t mima_hop;
static x_direction_t mima_hop_direction;
static unsigned char mima_missile_angle;
static unsigned char mima_pellet_angle;
static screen_x_t mima_laser_square_corners_x[SQUARE_POINTS];
static screen_y_t mima_laser_square_corners_y[SQUARE_POINTS];

static struct {
	int invincibility_frame;
	bool16 invincible;

	void update_and_render(const vc_t (&flash_colors)[2]) {
		boss_hit_update_and_render(
			invincibility_frame,
			invincible,
			boss_hp,
			flash_colors,
			(sizeof(flash_colors) / sizeof(flash_colors[0])),
			5000,
			boss_nop,
			ent_still.hittest_orb(),
			shot_hitbox_t(
				(ent_still.cur_right() - HITBOX_SHOT_W),
				(ent_still.cur_top + (MIMA_H / 2) - (HITBOX_SHOT_H / 2)),
				HITBOX_SHOT_W,
				HITBOX_SHOT_H
			)
		);
	}
} mima_hit;

static struct {
	int pattern_cur;

	void frame_common(void) const {
		boss_phase_frame++;
		mima_hit.invincibility_frame++;
	}

	void pattern_next(int total) {
		pattern_cur = (pattern_cur == (total - 1)) ? 0 : (pattern_cur + 1);
	}

	void next(int phase_new) {
		boss_phase = phase_new;
		boss_phase_frame = 0;
		mima_hit.invincibility_frame = 0;
		pattern_cur = 0;
	}
} mima_phase;

static bool mima_initial_hp_rendered;

// Pseudo-structure for all local square data, since the original set of data
// unfortunately is both partly stored in registers, and located on the stack
// in a way that prevents even parts of it to be turned into a structure.
// MODDERS: Turn into a proper template class, with all the macros below as
// methods.
#define SquareLocal(name) \
	screen_x_t name##_corners_x[SQUARE_POINTS]; \
	screen_y_t name##_corners_y[SQUARE_POINTS]; \
	screen_x_t name##_center_x; \
	screen_y_t name##_center_y;

#define SquareLocal2(name) \
	screen_x_t name##_corners_ccw_x[SQUARE_POINTS]; \
	screen_y_t name##_corners_ccw_y[SQUARE_POINTS]; \
	screen_x_t name##_corners_cw_x[SQUARE_POINTS]; \
	screen_y_t name##_corners_cw_y[SQUARE_POINTS]; \
	screen_x_t name##_center_x; \
	screen_y_t name##_center_y;

#define square_center_set(sql) { \
	sql##_center_x = ent_still.cur_center_x(); \
	sql##_center_y = ent_still.cur_center_y(); \
}

#define square_corners_set(sql, corners, radius, angle) { \
	regular_polygon( \
		corners##_x, \
		corners##_y, \
		sql##_center_x, \
		sql##_center_y, \
		radius, \
		angle, \
		SQUARE_POINTS \
	); \
}

#define square_unput(corners) { \
	graph_r_lineloop_unput(corners##_x, corners##_y, SQUARE_POINTS); \
}

#define square_put(corners) { \
	graph_r_lineloop_put(corners##_x, corners##_y, SQUARE_POINTS, V_WHITE); \
}

#define square_set_coords_and_unput(sql, corners, radius, angle) { \
	square_center_set(sql); \
	square_corners_set(sql, corners, radius, angle); \
	square_unput(corners); \
}

#define square_set_coords_and_put(sql, corners, radius, angle) { \
	square_corners_set(sql, corners, radius, angle); \
	square_put(corners); \
}
// ---------------

#define fire_static_from_corner(angle, sql, corner_x, corner_y, speed) { \
	angle = iatan2((corner_y - sql_center_y), (corner_x - sql_center_x)); \
	Pellets.add_single(corner_x, corner_y, angle, speed); \
}

void pattern_aimed_then_static_pellets_from_square_corners(void)
{
	SquareLocal(sql);

	if(boss_phase_frame < 100) {
		return;
	}
	if(boss_phase_frame == 100) {
		square_aimed_pellets.init();
		select_subpixel_for_rank(pattern_state.speed, 4.0f, 4.5f, 5.0f, 5.5f);
		mdrv2_se_play(8);
	}
	if((boss_phase_frame % SQUARE_INTERVAL) == 0) {
		square_set_coords_and_unput(
			sql, sql_corners, square_aimed_pellets.radius, square_aimed_pellets.angle
		);
		square_aimed_pellets.angle += (
			(boss_phase_frame < 260) ? +0x0C : -0x0C
		);
		if(square_aimed_pellets.radius < SEAL_CIRCUMSQUARE_RADIUS) {
			square_aimed_pellets.radius += SQUARE_RADIUS_STEP;
		} else if(boss_phase_frame > 280) {
			// Recurring quirk with all of these patterns: They spawn their
			// bullets at the *previous* corner positions, i.e., the ones
			// calculated for the unblitting call, right before blitting the
			// square at the new position.

			for(int i = 0; i < SQUARE_POINTS; i++) {
				unsigned char angle;
				fire_static_from_corner(
					angle,
					sql,
					sql_corners_x[i],
					sql_corners_y[i],
					pattern_state.speed
				);
				mdrv2_se_play(7);
			}
		} else {
			for(int i = 0; i < SQUARE_POINTS; i++) {
				unsigned char angle = iatan2(
					(player_center_y() - sql_corners_y[i]),
					((player_center_x() - (PELLET_W / 2) - sql_corners_x[i]))
				);
				Pellets.add_single(
					sql_corners_x[i],
					sql_corners_y[i],
					angle,
					(pattern_state.speed / 2)
				);
				mdrv2_se_play(7);
			}
		}
		square_set_coords_and_put(
			sql, sql_corners, square_aimed_pellets.radius, square_aimed_pellets.angle
		);
	}
	if(boss_phase_frame > 360) {
		square_set_coords_and_unput(
			sql, sql_corners, square_aimed_pellets.radius, square_aimed_pellets.angle
		);
		boss_phase_frame = 0;
	}
}

void pattern_aimed_missiles_from_square_corners(void)
{
	SquareLocal(sql);
	int i;
	Subpixel velocity_x;
	Subpixel velocity_y;

	if(boss_phase_frame < 100) {
		return;
	}
	if(boss_phase_frame == 100) {
		square_aimed_missiles.init();
		select_subpixel_for_rank(pattern_state.speed, 6.0f, 6.5f, 7.0f, 7.375f);
		mdrv2_se_play(8);
	}
	if((boss_phase_frame % SQUARE_INTERVAL) == 0) {
		square_set_coords_and_unput(
			sql, sql_corners, square_aimed_missiles.radius, square_aimed_missiles.angle
		);
		square_aimed_missiles.angle -= 0x0C;
		if(square_aimed_missiles.radius < SEAL_RADIUS) {
			square_aimed_missiles.radius += SQUARE_RADIUS_STEP;
		} else if(boss_phase_frame == 224) {
			mima_target_left = player_left;
		} else if(boss_phase_frame > 240) {
			// Same corner coordinate quirk as seen in the first pattern.

			vector2_between(
				sql_center_x,
				sql_center_y,
				(mima_target_left + (PLAYER_W / 2) - (MISSILE_W / 2)),
				player_center_y(),
				velocity_x.v,
				velocity_y.v,
				pattern_state.speed
			);
			for(i = 0; i < SQUARE_POINTS; i++) {
				Missiles.add(
					sql_corners_x[i],
					sql_corners_y[i],
					velocity_x.to_pixel(),
					velocity_y.to_pixel()
				);
			}
			mdrv2_se_play(6);
		}
		square_set_coords_and_put(
			sql, sql_corners, square_aimed_missiles.radius, square_aimed_missiles.angle
		);
	}
	if(boss_phase_frame > 320) {
		square_set_coords_and_unput(
			sql, sql_corners, square_aimed_missiles.radius, square_aimed_missiles.angle
		);
		boss_phase_frame = 0;
	}
}

void pattern_static_pellets_from_corners_of_two_squares(void)
{
	SquareLocal2(sql);

	if(boss_phase_frame == 50) {
		mima_put_cast_both();
	}
	if(boss_phase_frame < 100) {
		return;
	}
	if(boss_phase_frame == 100) {
		square_two_pellets.init();
		select_subpixel_for_rank(pattern_state.speed, 4.0f, 4.5f, 5.0f, 5.5f);
		mdrv2_se_play(8);
	}
	if((boss_phase_frame % SQUARE_INTERVAL) == 0) {
		square_center_set(sql);
		square_corners_set(
			sql, sql_corners_ccw, square_two_pellets.radius, square_two_pellets.angle
		);
		square_corners_set(
			sql, sql_corners_cw, square_two_pellets.radius,
			(0x00 - square_two_pellets.angle)
		);
		square_unput(sql_corners_ccw);
		square_unput(sql_corners_cw);

		square_two_pellets.angle -= 0x06;
		Pellets.spawn_with_cloud = true;

		if(square_two_pellets.radius < SEAL_CIRCUMSQUARE_RADIUS) {
			square_two_pellets.radius += SQUARE_RADIUS_STEP;
		} else {
			// Same corner coordinate quirk as seen in the first pattern.

			for(int i = 0; i < SQUARE_POINTS; i++) {
				unsigned char angle;
				fire_static_from_corner(
					angle,
					sql,
					sql_corners_ccw_x[i],
					sql_corners_ccw_y[i],
					pattern_state.speed
				);
				fire_static_from_corner(
					angle,
					sql,
					sql_corners_cw_x[i],
					sql_corners_cw_y[i],
					pattern_state.speed
				);
				mdrv2_se_play(7);
			}
		}
		square_corners_set(
			sql, sql_corners_ccw, square_two_pellets.radius, square_two_pellets.angle
		);
		square_corners_set(
			sql, sql_corners_cw, square_two_pellets.radius,
			(0x00 - square_two_pellets.angle)
		);
		square_put(sql_corners_ccw);
		square_put(sql_corners_cw);
		Pellets.spawn_with_cloud = false;
	}
	if(boss_phase_frame > 320) {
		// Not redundant, these are local variables here!
		square_center_set(sql);

		square_corners_set(
			sql, sql_corners_ccw, square_two_pellets.radius, square_two_pellets.angle
		);
		square_corners_set(
			sql, sql_corners_cw, square_two_pellets.radius,
			(0x00 - square_two_pellets.angle)
		);
		square_unput(sql_corners_ccw);
		square_unput(sql_corners_cw);
		boss_phase_frame = 0;
		meteor_activate();
	}
}

void pattern_hop_and_fire_chase_pellets(bool16 do_not_initialize = true)
{
	enum {
		HOP_DISTANCE = (PLAYFIELD_W / 5),
		SPREADIN_INTERVAL = 4,
		SPREADIN_FRAMES = 20,

		KEYFRAME_HOP = 1,
		KEYFRAME_TELEPORT = 4,
		KEYFRAME_HITBOX_ACTIVE = 12,
		KEYFRAME_SPREADIN_DONE = (KEYFRAME_SPREADIN_START + SPREADIN_FRAMES),
	};

	int i;
	int pellet_count;
	unsigned char angle;

	// Most of this could have been statically initialized, and even the random
	// direction wouldn't have required this separate parameter.
	if(!do_not_initialize) {
		mima_hop = -1;
		spreadin_interval = SPREADIN_INTERVAL;
		mima_hop_direction = static_cast<x_direction_t>(irand() % 2);
		spreadin_speed = ((MIMA_H / 2) / (SPREADIN_FRAMES / SPREADIN_INTERVAL));
		return;
	}

	if(mima_hop == static_cast<uint8_t>(-1)) {
		mima_unput_both();
		meteor_active = false;
		mima_hop = 0;
		boss_phase_frame = KEYFRAME_HOP;
	}
	if(mima_hop == 0) {
		ent_still.hitbox_orb_inactive = true;
		mima_vertical_sprite_transition_broken();
	} else {
		if(boss_phase_frame == 4) {
			ent_still.pos_cur_set(
				((mima_hop_direction == X_RIGHT)
					? (PLAYFIELD_LEFT + ((mima_hop - 1) * HOP_DISTANCE))
					: (PLAYFIELD_RIGHT - (mima_hop * HOP_DISTANCE))
				),
				(BASE_TOP - (PLAYFIELD_H / 14)) // Yup, not centered!
			);
			mima_bg_snap();
		}
		if(boss_phase_frame < KEYFRAME_HITBOX_ACTIVE) {
			ent_still.hitbox_orb_inactive = true;
		} else {
			ent_still.hitbox_orb_inactive = false;
		}
		phase_spreadin(ent_still.cur_left, ent_still.cur_top);
	}

	// Spread-in animation done?
	if(boss_phase_frame != 0) {
		return;
	}

	if(mima_hop != 0) {
		if(mima_hop != 4) {
			mima_unput_both();
		}

		select_subpixel_for_rank(
			pattern_state.speed, 1.875f, 2.1875f, 2.5f, 2.8125f
		);
		select_for_rank(pellet_count, 5, 8, 10, 12);

		for(
			(i = 0, angle = 0x00);
			i < pellet_count;
			(i++, angle += (0x100 / pellet_count))
		) {
			Pellets.add_single(
				ent_still.cur_center_x(),
				ent_still.cur_center_y(),
				angle,
				pattern_state.speed,
				PM_CHASE,
				pattern_state.speed
			);
		}
	}
	if(mima_hop >= 4) {
		// MODDERS: Same as mima_put_still_both().
		graph_accesspage_func(1);	ent_still.put_8(0);
		graph_accesspage_func(0);	ent_still.put_8(0);

		z_palette_set_all_show(stage_palette);

		// Prepare a potential next run of this pattern
		mima_hop = -1;
		mima_hop_direction = static_cast<x_direction_t>(
			X_LEFT - mima_hop_direction
		);

		meteor_active = true;
		boss_phase_frame = 0;
	} else {
		// Subtract #444 from every palette color for every new hop. Therefore,
		// you would expect the screen to gradually darken with every
		// successive hop, eventually reaching #000 by the last one… which is
		// not what you see in-game, thanks to boss_hit_update_and_render()
		// periodically resetting [z_Palettes] to the [boss_palette]. Hard to
		// say what ZUN actually intended here – 30 frames of being completely
		// blind to 3 groups of chase pellets isn't all *too* bad, even on
		// Lunatic. (Then again, at maximum pellet speed...)
		for(i = 0; i < COLOR_COUNT; i++) {
			z_palette_set_show(
				i,
				(z_Palettes[i].c.r - 0x4),
				(z_Palettes[i].c.g - 0x4),
				(z_Palettes[i].c.b - 0x4)
			);
		}
		mima_hop++;
		boss_phase_frame = KEYFRAME_HOP;
	}
}

#include "th01/sprites/pillar.csp"

static const int MIMA_PILLAR_COUNT = 8;
static const int MIMA_PILLAR_DELAY_PER_CIRCLE = 20;
static const int MIMA_PILLAR_KEYFRAME_CIRCLES = 100;
static const int MIMA_PILLAR_TIME = 32;
static const int MIMA_PILLAR_SEGMENTS_PER_FRAME = 2;
static const int MIMA_PILLAR_SEGMENTS_INITIAL = 4;

static struct {
	int time[MIMA_PILLAR_COUNT];
	screen_x_t center_x[MIMA_PILLAR_COUNT];
	screen_y_t bottom[MIMA_PILLAR_COUNT];

	int first_circle_frame_for(int i) const {
		return (MIMA_PILLAR_KEYFRAME_CIRCLES + (i * MIMA_PILLAR_DELAY_PER_CIRCLE));
	}

	screen_x_t left(int i) const {
		return (center_x[i] - (PILLAR_W / 2));
	}

	pixel_t pillar_h(int i) const {
		return (PILLAR_SEGMENT_H * MIMA_PILLAR_SEGMENTS_PER_FRAME * (
			(MIMA_PILLAR_TIME + (MIMA_PILLAR_SEGMENTS_INITIAL / 2)) - time[i]
		));
	}
} mima_pillars;

inline pixel_t pillar_sprite_row(pixel_t y) {
	return ((PILLAR_SEGMENT_H - 1) - (y % PILLAR_SEGMENT_H));
}

// ZUN bug: Called with non-byte-aligned X positions, creating a discrepancy
// between the on-screen sprite and the hitbox derived from the unaligned
// internal position. (Same as Kikuri's tear ripple sprites.)
void pillar_put_8(screen_x_t left, vram_y_t bottom, pixel_t h)
{
	pixel_t y;
	vram_offset_t vo = vram_offset_shift(left, bottom);

	grcg_setcolor_rmw(COL_PILLAR);

	// Segments
	y = 0;
	while(y < (h - PILLAR_SEGMENT_H)) {
		grcg_put(vo, sPILLAR[0][pillar_sprite_row(y)], PILLAR_W);
		y++;
		vo -= ROW_SIZE;
	}

	// Top part
	y = 0;
	while(y < PILLAR_SEGMENT_H) {
		grcg_put(vo, sPILLAR[1][pillar_sprite_row(y)], PILLAR_W);
		y++;
		vo -= ROW_SIZE;
	}
	grcg_off_func();
}

void pattern_pillars_and_aimed_spreads(void)
{
	enum {
	PILLAR_COUNT = 8,
		DELAY_PER_CIRCLE = MIMA_PILLAR_DELAY_PER_CIRCLE,
		CIRCLE_ANGLE_STEP = 0x04,

		PILLAR_FRAMES = 16,
		PILLAR_SEGMENTS_PER_FRAME = MIMA_PILLAR_SEGMENTS_PER_FRAME, // Only rendered every 8 though.
		PILLAR_SEGMENTS_INITIAL = MIMA_PILLAR_SEGMENTS_INITIAL,

		// We (sadly) only render pillars on frames 0 and 8, which means that
		// we miss out on half the height. ZUN already accounted for that and
		// reduced the final unblit height accordingly.
		PILLAR_SEGMENTS_TOTAL = (PILLAR_SEGMENTS_INITIAL - PILLAR_COUNT + (
			PILLAR_FRAMES * PILLAR_SEGMENTS_PER_FRAME
		)),
		PILLAR_UNBLIT_H = (PILLAR_SEGMENTS_TOTAL * PILLAR_SEGMENT_H),

		KEYFRAME_PREPARE = 50,
		KEYFRAME_CIRCLES = MIMA_PILLAR_KEYFRAME_CIRCLES,
		KEYFRAME_CIRCLE_LAST = (
			KEYFRAME_CIRCLES + ((PILLAR_COUNT - 1) * DELAY_PER_CIRCLE)
		),

		TIME_CIRCLES = 128, // doubles as the circle radius until...
		TIME_PILLARS = MIMA_PILLAR_TIME,
		TIME_PILLARS_DONE = (TIME_PILLARS - PILLAR_FRAMES),
	};

	int i;

	#define is_circle_frame_for(i) ( \
		((boss_phase_frame % PILLAR_COUNT) == i) && \
		(boss_phase_frame > mima_pillars.first_circle_frame_for(i)) \
	)

	if(boss_phase_frame == KEYFRAME_PREPARE) {
		mima_put_cast_both();
	}
	if(boss_phase_frame < KEYFRAME_CIRCLES) {
		return;
	}
	if(boss_phase_frame == KEYFRAME_CIRCLES) {
		for(i = 0; i < PILLAR_COUNT; i++) {
			mima_pillars.time[i] = TIME_CIRCLES;
			mima_pillars.bottom[i] = PLAYFIELD_BOTTOM;
		}

		mima_pillars.center_x[0] = playfield_rand_x(0.025f, 0.100f);

		// Translation: (playfield_rand_x(0.925f, 1.000f) - (PILLAR_W / 32)).
		// This is the only randomized coordinate that could have possibly come
		// close to result in a pillar position that exceeds the width of VRAM,
		// and pillar_put_8() doesn't clip anything. Therefore, the above
		// translation calculates a random position at the right edge of VRAM,
		// and then shifts it over to make sure that the sprite fits.
		mima_pillars.center_x[1] = ((PLAYFIELD_RIGHT - PILLAR_W) -
			playfield_rand_x(0.0f, 0.075f) + (PILLAR_W / 2)
		);

		mima_pillars.center_x[2] = playfield_rand_x(0.100f, 0.450f);
		mima_pillars.center_x[3] = playfield_rand_x(0.525f, 0.875f);
		mima_pillars.center_x[4] = playfield_rand_x(0.200f, 0.450f);
		mima_pillars.center_x[5] = playfield_rand_x(0.525f, 0.775f);
		mima_pillars.center_x[6] = playfield_rand_x(0.100f, 0.450f);
		mima_pillars.center_x[7] = playfield_rand_x(0.525f, 0.875f);

		// That's the same values as in the next pattern…?
		select_for_rank(pattern_state.unused, 40, 45, 50, 55);
		mdrv2_se_play(8);
	}

	// Unblit and update
	for(i = 0; i < PILLAR_COUNT; i++) {
		if(is_circle_frame_for(i) && (mima_pillars.time[i] > TIME_PILLARS)) {
			shape_circle_sloppy_unput(
				mima_pillars.center_x[i], mima_pillars.bottom[i],
				mima_pillars.time[i], CIRCLE_ANGLE_STEP
			);
			mima_pillars.time[i] -= PILLAR_COUNT;
		}
	}

	// MODDERS: if(
	// 	(boss_phase_frame <= KEYFRAME_CIRCLES_LAST) &&
	// 	((boss_phase_frame % DELAY_PER_CIRCLE) == 0)
	// )
	if(
		(boss_phase_frame == (KEYFRAME_CIRCLES + (0 * DELAY_PER_CIRCLE))) ||
		(boss_phase_frame == (KEYFRAME_CIRCLES + (1 * DELAY_PER_CIRCLE))) ||
		(boss_phase_frame == (KEYFRAME_CIRCLES + (2 * DELAY_PER_CIRCLE))) ||
		(boss_phase_frame == (KEYFRAME_CIRCLES + (3 * DELAY_PER_CIRCLE))) ||
		(boss_phase_frame == (KEYFRAME_CIRCLES + (4 * DELAY_PER_CIRCLE))) ||
		(boss_phase_frame == (KEYFRAME_CIRCLES + (5 * DELAY_PER_CIRCLE))) ||
		(boss_phase_frame == (KEYFRAME_CIRCLES + (6 * DELAY_PER_CIRCLE))) ||
		(boss_phase_frame == (KEYFRAME_CIRCLES + (7 * DELAY_PER_CIRCLE)))
	) {
		mdrv2_se_play(12);
	}
	if(boss_phase_frame == KEYFRAME_CIRCLE_LAST) {
		meteor_activate();
	}

	// Render and detect collisions
	for(i = 0; i < PILLAR_COUNT; i++) {
		if(!is_circle_frame_for(i)) {
			continue;
		}
		if(mima_pillars.time[i] > TIME_PILLARS) {
			shape_circle_put(
				mima_pillars.center_x[i],
				mima_pillars.bottom[i],
				mima_pillars.time[i],
				V_WHITE,
				CIRCLE_ANGLE_STEP
			);

			graph_r_line(
				// Should maybe have been calculated from the entity?
				BASE_CENTER_X,
				BASE_CENTER_Y,

				mima_pillars.center_x[i],
				mima_pillars.bottom[i],
				COL_SPAWNRAY
			);
		} else if(mima_pillars.time[i] > TIME_PILLARS_DONE) {
			if(mima_pillars.time[i] == TIME_PILLARS) {
				mdrv2_se_play(7);
				graph_r_line_unput(
					BASE_CENTER_X, BASE_CENTER_Y, mima_pillars.center_x[i],
					mima_pillars.bottom[i]
				);
			}
			pillar_put_8(
				mima_pillars.left(i), mima_pillars.bottom[i], mima_pillars.pillar_h(i)
			);

			// Translation: Reimu's center point has to be at least
			// (32 - 8) = 24 pixels away from the pillar's center.
			if(
				!player_invincible &&
				(player_left > (mima_pillars.center_x[i] - PLAYER_W - (PILLAR_W / 4))) &&
				(player_left < (mima_pillars.center_x[i] + (PILLAR_W / 4)))
			) {
				player_is_hit = true;
			}
			mima_pillars.time[i] -= PILLAR_COUNT;
		} else if(mima_pillars.time[i] != PIXEL_NONE) {
			Pellets.add_group(
				ent_still.cur_center_x(),
				ent_still.cur_center_y(),
				PG_3_SPREAD_WIDE_AIMED,
				to_sp(2.25f)
			);
			egc_copy_rect_1_to_0_16(
				mima_pillars.left(i),
				(PLAYFIELD_BOTTOM - PILLAR_UNBLIT_H),
				PILLAR_W,
				PILLAR_UNBLIT_H
			);
			mima_pillars.time[i] = PIXEL_NONE;
		}
	}
	// MODDERS: Loop, obviously.
	if(
		(mima_pillars.time[0] == PIXEL_NONE) && (mima_pillars.time[1] == PIXEL_NONE) &&
		(mima_pillars.time[2] == PIXEL_NONE) && (mima_pillars.time[3] == PIXEL_NONE) &&
		(mima_pillars.time[4] == PIXEL_NONE) && (mima_pillars.time[5] == PIXEL_NONE) &&
		(mima_pillars.time[6] == PIXEL_NONE) && (mima_pillars.time[7] == PIXEL_NONE)
	) {
		boss_phase_frame = 0;
	}

	#undef is_circle_frame_for
}

void pattern_halfcircle_missiles_downwards_from_corners(void)
{
	SquareLocal(sql);
	pixel_t velocity_x;
	pixel_t velocity_y;

	if(boss_phase_frame < 100) {
		return;
	}
	if(boss_phase_frame == 100) {
		square_halfcircle_missiles.init();
		mima_missile_angle = 0x00;

		// MODDERS: Just use regular subpixels. They perfectly support a
		// fraction of .5… especially if ZUN chops off the fractional digits
		// when reading this variable anyway. :(
		// So, the actual speeds are (4, 4, 5, 5).
		select_for_rank(pattern_state.speed_decimal.v,
			to_dsp(4.0f), to_dsp(4.5f), to_dsp(5.0f), to_dsp(5.5f)
		);
		mdrv2_se_play(8);
	}
	if((boss_phase_frame % SQUARE_INTERVAL) == 0) {
		square_set_coords_and_unput(
			sql, sql_corners, square_halfcircle_missiles.radius,
			square_halfcircle_missiles.angle
		);
		square_halfcircle_missiles.angle -= 0x0C;
		if(square_halfcircle_missiles.radius < SEAL_RADIUS) {
			square_halfcircle_missiles.radius += SQUARE_RADIUS_STEP;
		} else if((boss_phase_frame > 180) && ((boss_phase_frame % 16) == 8)) {
			// Same corner coordinate quirk as seen in the first pattern.

			vector2(
				velocity_x,
				velocity_y,
				pattern_state.speed_decimal.to_pixel(), // :(
				mima_missile_angle
			);

			for(int i = 0; i < SQUARE_POINTS; i++) {
				Missiles.add(
					sql_corners_x[i], sql_corners_y[i], velocity_x, velocity_y
				);
			}
			mdrv2_se_play(6);
			mima_missile_angle += 0x0D;
		}
		square_set_coords_and_put(
			sql, sql_corners, square_halfcircle_missiles.radius,
			square_halfcircle_missiles.angle
		);
	}
	if(boss_phase_frame > 340) {
		square_set_coords_and_unput(
			sql, sql_corners, square_halfcircle_missiles.radius,
			square_halfcircle_missiles.angle
		);
		boss_phase_frame = 0;
	}
}

void pattern_slow_pellet_spray_from_corners(void)
{
	enum {
		KEYFRAME_SQUARE = 100,
		KEYFRAME_FIRE_LEFT_TO_RIGHT = 180,
		KEYFRAME_FIRE_RIGHT_TO_LEFT = 270,
		KEYFRAME_DONE = 370,
	};

	SquareLocal(sql);

	if(boss_phase_frame < KEYFRAME_SQUARE) {
		return;
	}
	if(boss_phase_frame == KEYFRAME_SQUARE) {
		square_slow_spray.init();
		mima_pellet_angle = 0x80;
		select_subpixel_for_rank(pattern_state.speed, 2.0f, 2.5f, 3.0f, 3.5f);
		mdrv2_se_play(8);
	}
	if((boss_phase_frame % SQUARE_INTERVAL) == 0) {
		square_set_coords_and_unput(
			sql, sql_corners, square_slow_spray.radius, square_slow_spray.angle
		);
		square_slow_spray.angle = ((boss_phase_frame > KEYFRAME_FIRE_RIGHT_TO_LEFT)
			? (square_slow_spray.angle + 0x0C)
			: (square_slow_spray.angle - 0x0C)
		);
		if(square_slow_spray.radius < SEAL_RADIUS) {
			square_slow_spray.radius += SQUARE_RADIUS_STEP;
		} else if(boss_phase_frame > KEYFRAME_FIRE_LEFT_TO_RIGHT) {
			// Same corner coordinate quirk as seen in the first pattern.

			mima_pellet_angle = ((boss_phase_frame > KEYFRAME_FIRE_RIGHT_TO_LEFT)
				? (mima_pellet_angle + 0x0C)
				: (mima_pellet_angle - 0x0C) // slightly overshooting the half circle
			);
			for(int i = 0; i < SQUARE_POINTS; i++) {
				Pellets.add_single(
					sql_corners_x[i],
					sql_corners_y[i],
					mima_pellet_angle,
					pattern_state.speed
				);
			}
			mdrv2_se_play(6);
		}
		square_set_coords_and_put(
			sql, sql_corners, square_slow_spray.radius, square_slow_spray.angle
		);
	}
	if(boss_phase_frame > KEYFRAME_DONE) {
		square_set_coords_and_unput(
			sql, sql_corners, square_slow_spray.radius, square_slow_spray.angle
		);
		boss_phase_frame = 0;
	}
}

void pattern_aimed_lasers_from_corners(void)
{
	enum {
		LASER_W = 4,
	};

	static int16_t unused; (unused); // ZUN bloat

	screen_x_t sql_center_x;
	screen_y_t sql_center_y;

	if(boss_phase_frame == 50) {
		mima_put_cast_both();
	}
	if(boss_phase_frame < 100) {
		return;
	}
	if(boss_phase_frame == 100) {
		square_lasers.init();
		select_laser_speed_for_rank(pattern_state.speed_multiplied_by_8,
			6.25f, 6.75f, 7.25f, 7.75f
		);
		mdrv2_se_play(8);
	}
	if((boss_phase_frame % SQUARE_INTERVAL) == 0) {
		square_set_coords_and_unput(
			sql, mima_laser_square_corners, square_lasers.radius, square_lasers.angle
		);
		square_lasers.angle += 0x03;
		if(square_lasers.radius < SEAL_RADIUS) {
			square_lasers.radius += SQUARE_RADIUS_STEP;
		}

		// ... especially when remembering the coordinates for future frames
		// is exactly what fixes the corner coordinate quirk in this pattern.
		square_set_coords_and_put(
			sql, mima_laser_square_corners, square_lasers.radius, square_lasers.angle
		);
	}
	if((boss_phase_frame > 180) && (boss_phase_frame < 300)) {
		int i = (boss_phase_frame % SQUARE_POINTS);
		shootout_lasers[i].spawn(
			mima_laser_square_corners_x[i],
			mima_laser_square_corners_y[i],
			(player_center_x() - (LASER_W / 2)),
			player_bottom(),
			pattern_state.speed_multiplied_by_8,
			V_WHITE,
			20,
			LASER_W
		);
	}
	if(boss_phase_frame > 300) {
		square_set_coords_and_unput(
			sql, mima_laser_square_corners, square_lasers.radius, square_lasers.angle
		);
		boss_phase_frame = 0;
	}
}

void mima_main(void)
{
	const vc_t flash_colors[2] = { 3, 9 };
	int i;

	Missiles.unput_update_render();
	particles_unput_update_render(PO_TOP_RIGHT, V_WHITE);

	// Entrance animation
	if(boss_phase == 0) {
		graph_accesspage_func(0);
		boss_phase_frame = 0;
		ent_still.hitbox_orb_inactive = false;
		bool16 done = false; // (redundant)
		spreadin_interval = 1;
		spreadin_speed = 2;
		meteor_active = true; // (redundant)
		while(1) {
			boss_phase_frame++;
			if(!done) {
				phase_spreadin(BASE_LEFT, BASE_TOP);
			}
			if(boss_phase_frame == 0) {
				done++;
			}
			if(done == true) {
				break;
			}
			frame_delay(2);
		}
		boss_phase = 1;
		mima_phase.pattern_cur = 0;
		mima_initial_hp_rendered = false;
		stage_palette_set(z_Palettes);
		boss_palette_snap();

		// Doing some static initialization that wouldn't have been necessary
		// if this function was coded properly...
		pattern_hop_and_fire_chase_pellets(false);
	} else if(boss_phase == 1) {
		// ZUN bug: The fight only ends in Phase 3, which can in turn only ever
		// be entered after Mima completed her current pattern. Thus, HP
		// subtraction in debug mode can lead to this function being called
		// with [boss_phase_frame] being larger than the initial HP value,
		// causing the heap corruption bug mentioned in the comment of this
		// function. Mima starts with an even number of total HP, so this will
		// even happen for the easiest possible case of holding ↵ Return for
		// the first 6 frames of phase 1.
		hud_hp_increment_render(
			mima_initial_hp_rendered, boss_hp, boss_phase_frame
		);

		mima_phase.frame_common();
		meteor_put();
		if(mima_phase.pattern_cur == 0) {
			pattern_aimed_then_static_pellets_from_square_corners();
		} else if(mima_phase.pattern_cur == 1) {
			pattern_aimed_missiles_from_square_corners();
		} else if(mima_phase.pattern_cur == 2) {
			pattern_static_pellets_from_corners_of_two_squares();
		} else if(mima_phase.pattern_cur == 3) {
			pattern_hop_and_fire_chase_pellets();
		}

		// There might either be white squares on screen, or the hop pattern
		// might have manipulated the hardware palette. Delay the end of the
		// phase until the current pattern is done to make sure that everything
		// is cleaned up.
		if(boss_phase_frame == 0) {
			mima_phase.pattern_next(4);
			if(boss_hp <= HP_PHASE_1_END) {
				mima_phase.pattern_cur = 99;
			}
		}

		mima_hit.update_and_render(flash_colors);
		if((mima_phase.pattern_cur == 99) && !mima_hit.invincible) {
			mima_unput_both();
			spreadin_interval = 4;
			spreadin_speed = 8;

			// ZUN bloat: Already done by phase_spreadin() in phase 2.
			ent_still.pos_cur_set(BASE_LEFT, BASE_TOP);
			mima_bg_snap();

			mima_phase.next(2);
		}
	} else if(boss_phase == 2) {
		mima_phase.frame_common();
		phase_spreadin(BASE_LEFT, BASE_TOP);
		if(boss_phase_frame == 0) {
			mima_phase.next(3);
		}
	} else if(boss_phase == 3) {
		mima_phase.frame_common();
		meteor_put();
		if(mima_phase.pattern_cur == 0) {
			pattern_pillars_and_aimed_spreads();
		} else if(mima_phase.pattern_cur == 1) {
			pattern_halfcircle_missiles_downwards_from_corners();
		} else if(mima_phase.pattern_cur == 2) {
			pattern_slow_pellet_spray_from_corners();
		} else if(mima_phase.pattern_cur == 3) {
			pattern_aimed_lasers_from_corners();
		}

		if(boss_phase_frame == 0) {
			mima_phase.pattern_next(4);
		}

		mima_hit.update_and_render(flash_colors);
		if(boss_hp <= HP_PHASE_3_END) {
			graph_accesspage_func(1);
			mima_unput();
			graph_accesspage_func(0);

			mdrv2_bgm_fade_out_nonblock();
			Pellets.unput_and_reset_nonclouds();
			Missiles.reset();
			shootout_lasers_unput_and_reset_broken(i, 5); // 5? Doubly broken...
			boss_defeat_animate();

			// ZUN bloat: Already done at the start of REIIDEN.EXE's main().
			// The REIIDEN.EXE process restarts after the end of a scene
			// anyway, making this load doubly pointless.
			scene_init_and_load(4);
		}
	}
}

#pragma codeseg T1B10JOWN_TEXT

extern int8_t boss_id;

static bool16 t1boss_mima_checkpoint_coordinate_is_valid(int16_t coordinate)
{
	return ((coordinate >= -RES_X) && (coordinate <= (RES_X * 2)));
}

static bool16 t1boss_mima_checkpoint_square_is_valid(uint8_t radius)
{
	return (radius <= (SEAL_CIRCUMSQUARE_RADIUS + SQUARE_RADIUS_STEP));
}

static bool16 t1boss_mima_checkpoint_spreadin_is_valid(
	const t1boss_mima_checkpoint_t *checkpoint
)
{
	return (
		((checkpoint->spreadin_interval == 1) &&
		 (checkpoint->spreadin_speed == 2)) ||
		((checkpoint->spreadin_interval == 4) &&
		 ((checkpoint->spreadin_speed == 8) ||
		  (checkpoint->spreadin_speed == 16)))
	);
}

static bool16 t1boss_mima_checkpoint_phase_is_safe(
	const t1boss_mima_checkpoint_t *checkpoint
)
{
	if(checkpoint->phase == 1) {
		return (
			(checkpoint->pattern <= 3) &&
			(checkpoint->hp > HP_PHASE_1_END) &&
			(checkpoint->hp <= HP_TOTAL)
		);
	}
	if(checkpoint->phase == 3) {
		return (
			(checkpoint->pattern <= 3) &&
			(checkpoint->hp > HP_PHASE_3_END) &&
			(checkpoint->hp <= HP_PHASE_1_END)
		);
	}
	return false;
}

static bool16 t1boss_mima_checkpoint_geometry_is_valid(
	const t1boss_mima_checkpoint_t *checkpoint
)
{
	int i;

	if(
		!t1boss_mima_checkpoint_coordinate_is_valid(checkpoint->entity_left) ||
		!t1boss_mima_checkpoint_coordinate_is_valid(checkpoint->entity_top) ||
		!t1boss_mima_checkpoint_coordinate_is_valid(checkpoint->target_left)
	) {
		return false;
	}
	for(i = 0; i < MIMA_PILLAR_COUNT; i++) {
		if(
			(checkpoint->pillar_time[i] < PIXEL_NONE) ||
			(checkpoint->pillar_time[i] > 128) ||
			!t1boss_mima_checkpoint_coordinate_is_valid(
				checkpoint->pillar_center_x[i]
			) ||
			!t1boss_mima_checkpoint_coordinate_is_valid(
				checkpoint->pillar_bottom[i]
			)
		) {
			return false;
		}
	}
	for(i = 0; i < SQUARE_POINTS; i++) {
		if(
			!t1boss_mima_checkpoint_coordinate_is_valid(
				checkpoint->laser_corner_x[i]
			) ||
			!t1boss_mima_checkpoint_coordinate_is_valid(
				checkpoint->laser_corner_y[i]
			)
		) {
			return false;
		}
	}
	return true;
}

bool16 t1boss_mima_checkpoint_validate(
	const t1boss_mima_checkpoint_t *checkpoint
)
{
	if(
		!checkpoint ||
		(checkpoint->owner != T1BOSS_MIMA_CHECKPOINT_OWNER) ||
		(checkpoint->schema != T1BOSS_MIMA_CHECKPOINT_SCHEMA) ||
		(checkpoint->reserved[0] != 0) ||
		(checkpoint->reserved[1] != 0) ||
		!t1boss_mima_checkpoint_phase_is_safe(checkpoint) ||
		(checkpoint->phase_frame < 0) ||
		(checkpoint->phase_frame > 400) ||
		(checkpoint->invincibility_frame < 0) ||
		(checkpoint->invincibility_frame > BOSS_HIT_INVINCIBILITY_FRAMES) ||
		(checkpoint->pattern_state < 0) ||
		(checkpoint->pattern_state > 128) ||
		(checkpoint->meteor_active > 1) ||
		!t1boss_mima_checkpoint_spreadin_is_valid(checkpoint) ||
		(checkpoint->initial_hp_rendered > 1) ||
		(checkpoint->hit_invincible > 1) ||
		((checkpoint->hop != static_cast<uint8_t>(-1)) &&
		 (checkpoint->hop > 4)) ||
		(checkpoint->hop_direction > X_LEFT) ||
		(checkpoint->entity_image != 0) ||
		(checkpoint->animation_image > C_METEOR_last) ||
		(checkpoint->entity_hitbox_inactive > 1) ||
		!t1boss_mima_checkpoint_square_is_valid(
			checkpoint->square_aimed_pellets_radius
		) ||
		!t1boss_mima_checkpoint_square_is_valid(
			checkpoint->square_aimed_missiles_radius
		) ||
		!t1boss_mima_checkpoint_square_is_valid(
			checkpoint->square_two_pellets_radius
		) ||
		!t1boss_mima_checkpoint_square_is_valid(
			checkpoint->square_halfcircle_missiles_radius
		) ||
		!t1boss_mima_checkpoint_square_is_valid(
			checkpoint->square_slow_spray_radius
		) ||
		!t1boss_mima_checkpoint_square_is_valid(checkpoint->square_lasers_radius) ||
		!t1boss_mima_checkpoint_geometry_is_valid(checkpoint)
	) {
		return false;
	}
	return true;
}

bool16 t1boss_mima_checkpoint_capture(t1boss_mima_checkpoint_t *checkpoint)
{
	t1boss_mima_checkpoint_t live;
	int i;

	if(!checkpoint) {
		return false;
	}
	live.owner = T1BOSS_MIMA_CHECKPOINT_OWNER;
	live.schema = T1BOSS_MIMA_CHECKPOINT_SCHEMA;
	live.phase = boss_phase;
	live.pattern = mima_phase.pattern_cur;
	live.phase_frame = boss_phase_frame;
	live.hp = boss_hp;
	live.invincibility_frame = mima_hit.invincibility_frame;
	live.pattern_state = pattern_state.unused;
	live.entity_left = ent_still.cur_left;
	live.entity_top = ent_still.cur_top;
	live.target_left = mima_target_left;
	for(i = 0; i < MIMA_PILLAR_COUNT; i++) {
		live.pillar_time[i] = mima_pillars.time[i];
		live.pillar_center_x[i] = mima_pillars.center_x[i];
		live.pillar_bottom[i] = mima_pillars.bottom[i];
	}
	for(i = 0; i < SQUARE_POINTS; i++) {
		live.laser_corner_x[i] = mima_laser_square_corners_x[i];
		live.laser_corner_y[i] = mima_laser_square_corners_y[i];
	}
	live.meteor_active = meteor_active;
	live.spreadin_interval = spreadin_interval;
	live.spreadin_speed = spreadin_speed;
	live.initial_hp_rendered = mima_initial_hp_rendered;
	live.hit_invincible = mima_hit.invincible;
	live.hop = mima_hop;
	live.hop_direction = mima_hop_direction;
	live.entity_image = ent_still.image();
	live.animation_image = ent_anim.image();
	live.entity_hitbox_inactive = ent_still.hitbox_orb_inactive;
	live.square_aimed_pellets_angle = square_aimed_pellets.angle;
	live.square_aimed_pellets_radius = square_aimed_pellets.radius;
	live.square_aimed_missiles_angle = square_aimed_missiles.angle;
	live.square_aimed_missiles_radius = square_aimed_missiles.radius;
	live.square_two_pellets_angle = square_two_pellets.angle;
	live.square_two_pellets_radius = square_two_pellets.radius;
	live.square_halfcircle_missiles_angle = square_halfcircle_missiles.angle;
	live.square_halfcircle_missiles_radius = square_halfcircle_missiles.radius;
	live.square_slow_spray_angle = square_slow_spray.angle;
	live.square_slow_spray_radius = square_slow_spray.radius;
	live.square_lasers_angle = square_lasers.angle;
	live.square_lasers_radius = square_lasers.radius;
	live.missile_angle = mima_missile_angle;
	live.pellet_angle = mima_pellet_angle;
	live.reserved[0] = 0;
	live.reserved[1] = 0;
	if(!t1boss_mima_checkpoint_validate(&live)) {
		return false;
	}
	*checkpoint = live;
	return true;
}

static void t1boss_mima_checkpoint_restore_entity(
	const t1boss_mima_checkpoint_t *checkpoint
)
{
	ent_still.pos_set(checkpoint->entity_left, checkpoint->entity_top);
	ent_still.prev_left = checkpoint->entity_left;
	ent_still.prev_top = checkpoint->entity_top;
	ent_still.prev_delta_x = 0;
	ent_still.prev_delta_y = 0;
	ent_still.lock_frame = 0;
	ent_still.hitbox_orb_set(
		((MIMA_W / 8) * 1), ((MIMA_H / 10) * 1),
		((MIMA_W / 8) * 7), ((MIMA_H / 10) * 9)
	);
	ent_still.set_image(checkpoint->entity_image);
	if(checkpoint->entity_hitbox_inactive) {
		ent_still.hitbox_orb_deactivate();
	} else {
		ent_still.hitbox_orb_activate();
	}
	ent_anim.pos_set(
		checkpoint->entity_left, (checkpoint->entity_top + MIMA_ANIM_TOP)
	);
	ent_anim.set_image(checkpoint->animation_image);
}

bool16 t1boss_mima_ckpt_apply_loaded(
	const t1boss_mima_checkpoint_t *checkpoint
)
{
	int i;

	if(!t1boss_mima_checkpoint_validate(checkpoint)) {
		return false;
	}

	t1boss_mima_checkpoint_restore_entity(checkpoint);
	mima_bg_snap();

	boss_phase = checkpoint->phase;
	boss_phase_frame = checkpoint->phase_frame;
	boss_hp = checkpoint->hp;
	mima_hit.invincibility_frame = checkpoint->invincibility_frame;
	pattern_state.unused = checkpoint->pattern_state;
	mima_target_left = checkpoint->target_left;
	for(i = 0; i < MIMA_PILLAR_COUNT; i++) {
		mima_pillars.time[i] = checkpoint->pillar_time[i];
		mima_pillars.center_x[i] = checkpoint->pillar_center_x[i];
		mima_pillars.bottom[i] = checkpoint->pillar_bottom[i];
	}
	for(i = 0; i < SQUARE_POINTS; i++) {
		mima_laser_square_corners_x[i] = checkpoint->laser_corner_x[i];
		mima_laser_square_corners_y[i] = checkpoint->laser_corner_y[i];
	}
	meteor_active = checkpoint->meteor_active;
	spreadin_interval = checkpoint->spreadin_interval;
	spreadin_speed = checkpoint->spreadin_speed;
	mima_initial_hp_rendered = checkpoint->initial_hp_rendered;
	mima_hit.invincible = checkpoint->hit_invincible;
	mima_phase.pattern_cur = checkpoint->pattern;
	mima_hop = checkpoint->hop;
	mima_hop_direction = static_cast<x_direction_t>(checkpoint->hop_direction);
	square_aimed_pellets.angle = checkpoint->square_aimed_pellets_angle;
	square_aimed_pellets.radius = checkpoint->square_aimed_pellets_radius;
	square_aimed_missiles.angle = checkpoint->square_aimed_missiles_angle;
	square_aimed_missiles.radius = checkpoint->square_aimed_missiles_radius;
	square_two_pellets.angle = checkpoint->square_two_pellets_angle;
	square_two_pellets.radius = checkpoint->square_two_pellets_radius;
	square_halfcircle_missiles.angle = checkpoint->square_halfcircle_missiles_angle;
	square_halfcircle_missiles.radius = checkpoint->square_halfcircle_missiles_radius;
	square_slow_spray.angle = checkpoint->square_slow_spray_angle;
	square_slow_spray.radius = checkpoint->square_slow_spray_radius;
	square_lasers.angle = checkpoint->square_lasers_angle;
	square_lasers.radius = checkpoint->square_lasers_radius;
	mima_missile_angle = checkpoint->missile_angle;
	mima_pellet_angle = checkpoint->pellet_angle;
	hud_hp_first_white = HP_PHASE_1_END;
	hud_hp_first_redwhite = 2;
	return true;
}

bool16 t1boss_mima_checkpoint_apply(
	const t1boss_mima_checkpoint_t *checkpoint
)
{
	if(!t1boss_mima_checkpoint_validate(checkpoint)) {
		return false;
	}
	// Rebuild pointer-backed .BOS/.PTN resources. The checkpoint contains only
	// stable sprite state, pattern IDs, and simulation geometry.
	mima_ent_load();
	ptn_new(
		PTN_SLOT_BG_ENT,
		(((MIMA_W / PTN_W) * (MIMA_H / PTN_H)) + BG_ENT_OFFSET + 1)
	);
	Missiles.load(PTN_SLOT_MISSILE);
	return t1boss_mima_ckpt_apply_loaded(checkpoint);
}

bool16 t1boss_mima_practice_first_combat_construct(void)
{
	t1boss_mima_checkpoint_t start;
	int i;

	// The normal stage loader must have allocated Mima's entities, backing PTN,
	// and missile resource, but no entrance frame may have run yet. The
	// resident/boss identity is intentionally repeated here: no caller may
	// construct this boundary for a different route or stage.
	if(
		!resident ||
		(resident->stage_id != ((1 * STAGES_PER_SCENE) + BOSS_STAGE)) ||
		(resident->route != ROUTE_JIGOKU) ||
		(boss_id != BID_MIMA) ||
		(boss_phase != 0) ||
		(boss_phase_frame != 0) ||
		(boss_hp != HP_TOTAL) ||
		(ent_still.cur_left != BASE_LEFT) ||
		(ent_still.cur_top != PLAYFIELD_TOP) ||
		(ent_still.image() != 0) ||
		(ent_anim.image() != C_METEOR) ||
		(ent_still.hitbox_orb_inactive != false)
	) {
		return false;
	}
	// Preserve mima_main()'s pre-entrance pool update. With Mima's loader
	// reset this leaves the missile pool empty and advances the initialized
	// particle pool exactly once before the boundary's sole pattern RNG draw.
	Missiles.unput_update_render();
	particles_unput_update_render(PO_TOP_RIGHT, V_WHITE);

	start.owner = T1BOSS_MIMA_CHECKPOINT_OWNER;
	start.schema = T1BOSS_MIMA_CHECKPOINT_SCHEMA;
	start.phase = 1;
	start.pattern = 0;
	start.phase_frame = 0;
	start.hp = HP_TOTAL;
	start.invincibility_frame = 0;
	start.pattern_state = 0;
	start.entity_left = BASE_LEFT;
	start.entity_top = BASE_TOP;
	start.target_left = 0;
	for(i = 0; i < MIMA_PILLAR_COUNT; i++) {
		start.pillar_time[i] = 0;
		start.pillar_center_x[i] = 0;
		start.pillar_bottom[i] = 0;
	}
	for(i = 0; i < SQUARE_POINTS; i++) {
		start.laser_corner_x[i] = 0;
		start.laser_corner_y[i] = 0;
	}
	start.meteor_active = true;
	start.spreadin_interval = 4;
	start.spreadin_speed = 16;
	start.initial_hp_rendered = false;
	start.hit_invincible = false;
	start.hop = static_cast<uint8_t>(-1);
	start.hop_direction = X_RIGHT;
	start.entity_image = 0;
	start.animation_image = C_METEOR;
	start.entity_hitbox_inactive = false;
	start.square_aimed_pellets_angle = 0;
	start.square_aimed_pellets_radius = 0;
	start.square_aimed_missiles_angle = 0;
	start.square_aimed_missiles_radius = 0;
	start.square_two_pellets_angle = 0;
	start.square_two_pellets_radius = 0;
	start.square_halfcircle_missiles_angle = 0;
	start.square_halfcircle_missiles_radius = 0;
	start.square_slow_spray_angle = 0;
	start.square_slow_spray_radius = 0;
	start.square_lasers_angle = 0;
	start.square_lasers_radius = 0;
	start.missile_angle = 0;
	start.pellet_angle = 0;
	start.reserved[0] = 0;
	start.reserved[1] = 0;
	if(!t1boss_mima_ckpt_apply_loaded(&start)) {
		return false;
	}

	// This is the native post-entrance order. [start] is a local pointer-free
	// owner carrier only, not serialized input; the loaded helper captures the
	// page-1 backing before both-page repainting.
	mima_put_still_both();
	stage_palette_set(z_Palettes);
	boss_palette_snap();
	pattern_hop_and_fire_chase_pellets(false);
	return true;
}

#if T1REPLAY_CHECKPOINT_PRIVATE_RESTORE
// Private source-owned Phase 3 boundary. This is deliberately not reachable
// from the Practice selector or replay bridge until a natural/direct visual
// witness proves the fresh presentation.
static bool16 t1boss_mima_phase3_owner_construct(void)
{
	t1boss_mima_checkpoint_t start;
	int i;

	// Phase 3 is entered only after Phase 2 finished Mima's spread-in. Start
	// from the untouched native Mima loader instead and materialize that
	// canonical post-transition owner state without simulating either phase.
	if(
		!resident ||
		(resident->stage_id != ((1 * STAGES_PER_SCENE) + BOSS_STAGE)) ||
		(resident->route != ROUTE_JIGOKU) ||
		(boss_id != BID_MIMA) ||
		(boss_phase != 0) ||
		(boss_phase_frame != 0) ||
		(boss_hp != HP_TOTAL) ||
		(ent_still.cur_left != BASE_LEFT) ||
		(ent_still.cur_top != PLAYFIELD_TOP) ||
		(ent_still.image() != 0) ||
		(ent_anim.image() != C_METEOR) ||
		(ent_still.hitbox_orb_inactive != false)
	) {
		return false;
	}

	start.owner = T1BOSS_MIMA_CHECKPOINT_OWNER;
	start.schema = T1BOSS_MIMA_CHECKPOINT_SCHEMA;
	start.phase = 3;
	start.pattern = 0;
	start.phase_frame = 0;
	start.hp = HP_PHASE_1_END;
	start.invincibility_frame = 0;
	start.pattern_state = 0;
	start.entity_left = BASE_LEFT;
	start.entity_top = BASE_TOP;
	start.target_left = 0;
	for(i = 0; i < MIMA_PILLAR_COUNT; i++) {
		start.pillar_time[i] = 0;
		start.pillar_center_x[i] = 0;
		start.pillar_bottom[i] = 0;
	}
	for(i = 0; i < SQUARE_POINTS; i++) {
		start.laser_corner_x[i] = 0;
		start.laser_corner_y[i] = 0;
	}
	start.meteor_active = true;
	start.spreadin_interval = 4;
	start.spreadin_speed = 8;
	start.initial_hp_rendered = true;
	start.hit_invincible = false;
	start.hop = static_cast<uint8_t>(-1);
	start.hop_direction = X_RIGHT;
	start.entity_image = 0;
	start.animation_image = C_METEOR;
	start.entity_hitbox_inactive = false;
	start.square_aimed_pellets_angle = 0;
	start.square_aimed_pellets_radius = 0;
	start.square_aimed_missiles_angle = 0;
	start.square_aimed_missiles_radius = 0;
	start.square_two_pellets_angle = 0;
	start.square_two_pellets_radius = 0;
	start.square_halfcircle_missiles_angle = 0;
	start.square_halfcircle_missiles_radius = 0;
	start.square_slow_spray_angle = 0;
	start.square_slow_spray_radius = 0;
	start.square_lasers_angle = 0;
	start.square_lasers_radius = 0;
	start.missile_angle = 0;
	start.pellet_angle = 0;
	start.reserved[0] = 0;
	start.reserved[1] = 0;
	if(!t1boss_mima_ckpt_apply_loaded(&start)) {
		return false;
	}

	// Phase 2 has already produced the static both-page Mima image and the
	// completed HP bar. Recreate its backing and presentation in that order.
	mima_put_still_both();
	stage_palette_set(z_Palettes);
	boss_palette_snap();
	hud_hp_rerender(HP_PHASE_1_END);
	return true;
}
#endif

#pragma codeseg
