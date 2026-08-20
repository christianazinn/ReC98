/// Player rendering
/// ----------------
/// (#included from th04/main_0.cpp, which exists only to name main_0_TEXT
/// after its own basename (kb/codegen/0105). This body was the last thing
/// th04_main.asm contributed to that segment -- an `include` of
/// th04/main/player/render.asm, not a `proc` -- so the new object lands
/// exactly at the seam the deleted module left behind, and every byte above
/// it keeps its address (kb/codegen 0112 + 0114).
///
/// th04/main/player/render.asm is NOT deleted: th05_main.asm still includes
/// it, in the middle of its SHOT_INV_TEXT block, where it is not a tail.)
///
/// Two disjoint halves in one function, selected by [miss_time]:
///
/// 1) the living player -- sprite, movement-direction cel, the invincibility
///    flash, and the two options once [shot_level] is high enough,
/// 2) the miss explosion -- eight sprites placed on a ring around the
///    player's last position, the second half of them on a half-radius ring
///    rotating the other way.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/hardware/grcg.hpp"
#include "th02/v_colors.hpp"
#include "th04/main/frames.h"
#include "th04/main/drawp.hpp"
#include "th04/main/player/player.hpp"
#include "th04/hardware/grcg.hpp"
#include "th04/formats/super.h"
#include "th04/math/vector.hpp"

// The sprite blitted for each of the eight miss explosion points.
static const int PAT_MISS_EXPLOSION = 3;

extern "C" void pascal near player_render(void)
{
	// Declaration order is the frame layout: [bp-2], [bp-4], [bp-5].
	// (kb/codegen/0010)
	vram_y_t screen_y;
	int i;
	unsigned char angle;

	// The two register variables, SI and DI. SI is a single `int` that ZUN
	// reused for two unrelated quantities in the two mutually exclusive
	// halves below -- the sprite patnum in one, the explosion radius in the
	// other. [inferred] from the allocation: the original's frame is 6 bytes
	// and holds exactly the three locals above, so a fourth candidate would
	// have had to spill (kb/codegen/0146).
	register int patnum_or_radius;
	register screen_x_t left;

	if((miss_time == 0) || (miss_time > MISS_ANIM_FRAMES)) {
		left = player_pos.cur.to_screen_left(PLAYER_W);
		screen_y = player_pos.cur.to_vram_top_scrolled_seg1(PLAYER_H);

		if(player_pos.velocity.x < 0) {
			patnum_or_radius = 1;
		} else if(player_pos.velocity.x != 0) {
			patnum_or_radius = 2;
		} else {
			patnum_or_radius = 0;
		}

		if((player_invincibility_time != 0) && (stage_frame_mod4 == 0)) {
			super_roll_put_1plane(
				left, screen_y, patnum_or_radius, 0, super_plane(V_WHITE)
			);
		} else {
			super_roll_put(left, screen_y, patnum_or_radius);
		}

		if(shot_level < 2) {
			return;
		}
		grcg_setmode_rmw();

		// ZUN bloat: No PLAYFIELD_LEFT and no centering offset, unlike every
		// other coordinate in this function. Technically, this should have
		// been
		//	(PLAYFIELD_LEFT + TO_PIXEL(x) - PLAYER_OPTION_DISTANCE -
		//	(PLAYER_OPTION_W / 2)).
		left = player_option_pos_cur.x.to_pixel();
		screen_y = player_option_pos_cur.to_vram_top_scrolled_seg1(
			PLAYER_OPTION_H
		);
		// Spelled out rather than going through th04/formats/super.h's
		// z_super_roll_put_tiny_16x16() macro: that macro sets _DX before
		// _AX, and this caller sets them the other way round. Same divergence
		// as the one item_splashes_render() (th04/main/item/splashes_render.cpp)
		// records for the opposite order, and the same fix.
		_AX = left;
		_DX = screen_y;
		z_super_roll_put_tiny_16x16_raw(player_option_patnum);

		_AX = (left + PLAYER_OPTION_TO_OPTION_DISTANCE);
		_DX = screen_y;
		z_super_roll_put_tiny_16x16_raw(player_option_patnum);
		grcg_off();
	} else if(miss_time > (MISS_ANIM_FRAMES - MISS_ANIM_EXPLODE_UNTIL)) {
		patnum_or_radius = miss_explosion_radius;
		i = 0;
		angle = miss_explosion_angle;
		while(i < MISS_EXPLOSION_COUNT) {
			if(i == (MISS_EXPLOSION_COUNT / 2)) {
				patnum_or_radius /= 2;
				angle = -angle;
			}
			vector2_at(
				drawpoint,
				player_pos.cur.x,
				player_pos.cur.y,
				patnum_or_radius,
				angle
			);
			if(
				(drawpoint.y >= TO_SP(
					PLAYFIELD_TOP - (MISS_EXPLOSION_H / 2)
				)) &&
				// ZUN bug: Both of these are wrong -- the bottom edge is
				// clipped 8 pixels short of where it should be, and the left
				// edge 40 pixels past it.
				(drawpoint.y < TO_SP(PLAYFIELD_BOTTOM - 8)) &&
				(drawpoint.x >= TO_SP(PLAYFIELD_LEFT - 40)) &&
				(drawpoint.x < TO_SP(
					PLAYFIELD_RIGHT - (MISS_EXPLOSION_W / 2)
				))
			) {
				left = drawpoint.to_screen_left(MISS_EXPLOSION_W);
				screen_y = drawpoint.to_vram_top_scrolled_seg1(
					MISS_EXPLOSION_H
				);
				super_roll_put(left, screen_y, PAT_MISS_EXPLOSION);
			}
			i++;
			angle += (256 / (MISS_EXPLOSION_COUNT / 2));
		}
	}
}
