/// Stage 3 Boss - Alice's puppets, rendering
/// -----------------------------------------
/// The other half of Alice's foreground: th05/main/boss/b3_fg.cpp calls this
/// one, and only outside the big explosion. It is the only TH05 renderer that
/// blits a custom entity type and draws GRCG geometry in the same body.
///
/// Three things happen per puppet, in this order: the sprite, an optional
/// circle around it, and a record of its center X. The circles and the line
/// that joins the two puppets afterwards are the *gather* animation, and they
/// are what [puppet_t::radius.gather] sizes.
///
/// (#included from th05/b34fg.cpp, ahead of th05/main/boss/b3_fg.cpp. This
/// function was the last `proc` of th05_main.asm's MIDBOSSX_TEXT block once
/// alice_fg_render() was lifted out from under it, and that object is the
/// segment's next contribution, so the lift lands exactly where the root's
/// block ended. kb/codegen 0112 + 0114.)

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/main/frames.h"
#include "th04/main/custom.hpp"
#include "th04/main/boss/boss.hpp"
#include "th05/main/boss/b3puppet.hpp"

/// master.lib's GRCG_OFF_CLOBBERING macro, which spills the port number to DX
/// rather than using the 8-bit immediate form. Spelled the same way
/// th04/end/staff_dissolve.cpp and th04/hiscore/regist_view.cpp spell it.
#define grcg_off_clobbering_dx() outportb(0x7C, GC_OFF)

// [inferred] Alice drives the whole gather animation out of two of the shared
// [boss_statebyte] slots, and nothing in this function names them. What IS
// measured here is only what each value draws:
//
//   1  a circle OUTLINE around each puppet, blinking on [stage_frame_mod2]
//   2  a FILLED circle around each puppet, plus a horizontal line joining
//      the two, in COL_PUPPETS_LINK
//   3  two sprites, [puppets_gather_patnum] and the cel after it, 64 pixels
//      apart, starting at the first puppet's center
//
// Which of Alice's phases sets them is still assembly, so they are spelled as
// the raw slots behind these two names rather than given a meaning they have
// not been shown to have. th05/main/midboss/m4.cpp does the same for
// [boss_statebyte[15]].
#define puppets_gather_state boss_statebyte[9]
#define puppets_gather_patnum boss_statebyte[8]

// [inferred] The line joining the two puppets. No entry of
// th02/v_colors.hpp's palette-index enum covers it, and what this index is
// meant to depict has not been decided -- only that it is not V_WHITE, which
// the circles above it use.
static const int COL_PUPPETS_LINK = 9;

// The puppets bob up and down on a 32-frame triangle wave: [stage_frame_mod16]
// rising for the first half of it, and counting back down for the second.
static const int PUPPETS_BOB_HALF_FRAMES = 16;

// [inferred] Distance between the two cels of the `puppets_gather_state == 3`
// pair. Twice PUPPET_W, but it is the distance between two sprites of an
// unrelated block rather than anything derived from the puppets' own extent.
static const pixel_t PUPPETS_GATHER_CEL_DISTANCE = 64;

void pascal near puppets_render(void)
{
	// `ENTER 0Ah, 0` with SI and DI live: [p] and [y] are the two enregistered
	// locals and the other four share the frame, [i] nearest to BP.
	// (kb/codegen 0010 + 0146)
	puppet_t near *p;
	screen_y_t y;
	int i;
	screen_x_t left;
	int patnum;

	// Filled in by every puppet that got drawn, and read back after the loop
	// for the joining line. Never cleared, so a puppet that was clipped away
	// this frame leaves whatever the previous frame put here.
	screen_x_t center_x[PUPPET_COUNT];

	p = puppets;
	for(i = 0; i < PUPPET_COUNT; i++, p++) {
		if(p->flag == F_FREE) {
			continue;
		}

		// ZUN bug: the horizontal clip is not symmetric with the vertical one
		// below it. A puppet is dropped as soon as its left edge reaches 0,
		// i.e. while half of it is still inside the playfield, where the
		// vertical test correctly allows half a sprite past each edge.
		left = p->pos.cur.to_screen_left(PUPPET_W);
		if((left <= 0) || (left >= (PLAYFIELD_W + PUPPET_W))) {
			continue;
		}
		y = p->pos.cur.to_screen_top(PUPPET_H);
		if(
			(y <= (-PUPPET_H / 2)) ||
			(y >= (PLAYFIELD_H + (PUPPET_H / 2)))
		) {
			continue;
		}

		// The bob. Written as one expression because the triangle wave never
		// reaches a variable -- both halves are computed in AX and the halving
		// is shared -- so an `if`/`else` pair of statements would only match
		// by way of the compiler merging their tails.
		y += ((
			((stage_frame % (PUPPETS_BOB_HALF_FRAMES * 2)) <
				PUPPETS_BOB_HALF_FRAMES)
			? stage_frame_mod16
			: (PUPPETS_BOB_HALF_FRAMES - stage_frame_mod16)
		) / 2);

		patnum = p->patnum;
		if(p->flag == F_ALIVE) {
			patnum += stage_frame_mod2;
		}
		if(p->damage_this_frame == 0) {
			super_roll_put(left, y, patnum);
		} else {
			super_put_1plane(left, y, patnum, 0, super_plane(V_WHITE));
		}

		// Re-read rather than derived from the [y] above: the circle is
		// centered on the puppet's *unbobbed* position, and this value is also
		// the one the code after the loop uses.
		y = p->pos.cur.to_screen_top();

		if(puppets_gather_state == 1) {
			if(stage_frame_mod2 != 0) {
				grcg_setcolor(GC_RMW, V_WHITE);
				grcg_circle(
					(left + (PUPPET_W / 2)), y, p->radius.gather
				);
				grcg_off_clobbering_dx();
			}
		} else if(puppets_gather_state == 2) {
			grcg_setcolor(GC_RMW, V_WHITE);
			grcg_circlefill(
				(left + (PUPPET_W / 2)), y, p->radius.gather
			);
			grcg_off_clobbering_dx();
		}
		center_x[i] = (left + (PUPPET_W / 2));
	}

	// Both of these reuse [y] from the last iteration, so they sit at
	// whichever puppet the loop happened to leave it on rather than at
	// anything either of them computes.
	if(puppets_gather_state == 2) {
		grcg_setcolor(GC_RMW, COL_PUPPETS_LINK);
		grcg_hline(center_x[0], center_x[1], y);
		grcg_off_clobbering_dx();
	} else if(puppets_gather_state == 3) {
		super_put(center_x[0], y, puppets_gather_patnum);
		super_put(
			(center_x[0] + PUPPETS_GATHER_CEL_DISTANCE), y,
			(puppets_gather_patnum + 1)
		);
	}
}
