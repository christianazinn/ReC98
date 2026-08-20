/// Stage 4 Boss - Marisa
/// ---------------------
/// The drift helper, the per-frame update, the background renderer and the
/// stage-end callback.
/// They are the last bodies of the nameless code segment that also holds
/// Marisa's other callback and the Stage 5 boss, so it needs no split and no
/// new segment - this translation unit just contributes to that same segment
/// after th02_main.asm's block.
/// (kb/codegen/0099)

// -G, because the original's prologs are `push bp; mov bp, sp` with no locals
// rather than an `ENTER`. (kb/codegen/0011)
#pragma option -zCmain_03__TEXT -zPmain_03 -G -a2

#include "platform.h"
#include "pc98.h"
#include "planar.h"
#include "th01/rank.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/core/globals.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/frames.hpp"
#include "th02/math/randring.hpp"
#include "th02/main/bg_particle.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/boss/b4.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/explode.hpp"
#include "th02/main/spark.hpp"
#include "th02/main/score.hpp"
#include "th02/main/item/item.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/shot.hpp"
#include "th02/v_colors.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/stage/bonus.hpp"
#include "th02/main/dialog/dialog.hpp"
#include "th02/main/hud/overlay.hpp"
#include "th02/sprites/bullet16.h"

// th02/main/dialog/dialog.cpp. dialog.hpp declares every dialog_script_*
// function but not this one.
void near dialog_pre(void);

// th02/snd/snd.h, declared here rather than included, the way
// th02/main/laser.cpp already declares it: snd.h has no include guard and
// pulls in three more unguarded headers for the sake of one function.
extern "C" void __cdecl snd_se_play(int new_se);

// The sprite the boss and midboss renderers blit, shared by all of them and
// written from ~150 sites across th02_main.asm. `patnum_2064E` is the dump's
// own spelling and is not an IDA placeholder; retiring the address suffix
// means ruling on all of those sites at once, which is its own parcel.
extern "C" int patnum_2064E;

// The five-byte rank-scaled parameter block that every boss and midboss init
// function fills in - always in a two-way branch on [rank] - and that that
// boss's own pattern functions then read back. stones_11997(), rika_init(),
// meira_init(), mima_180AC() and marisa_1B214() are its only writers, each
// filling a contiguous prefix of it; 20 reads across those same five bosses
// take cells out of it again. Full census in
// state/notes/th02-boss-rank-param.md.
//
// It is named as the ARRAY rather than cell by cell because no per-cell name
// is true across bosses: sixteen of the reads hand a cell straight to
// bullets_add_*() as a [group], but the others use one as a stone index
// (stones_11DF6()), a frame-period mask (rika_13C91()), an angle delta
// (mima_19173()) and a bullet count (marisa_1BC43(), below). `[measured]`
// mima_180EC() also spells `byte_2066F[bx]` with [bx] a 0-or-1 phase flag,
// which is an access no set of scalar names can express at all. The
// genericity is the fact: this is one scratch block whose meaning belongs to
// whichever boss happens to be alive.
extern "C" uint8_t boss_rank_param[5];

// th02/resident.hpp, declared the same way th02/main/bg_particle.cpp does
// rather than by pulling the whole resident structure in for one flag.
extern "C" bool reduce_effects;

// The point shottype B's homing shots aim at, set by whichever boss is on
// screen. Declared here the way th02/main/player/shot.cpp and
// th02/main/player/reset.cpp already declare them; no header owns them.
extern "C" int boss_pos_x;
extern "C" int boss_pos_y;

// th02/main/bullet/bullet.cpp, which owns the `[inferred]` licence for this
// name. 0 for as long as Marisa is fighting; marisa_update() sets it to 1 on
// the frame she is defeated, which switches the tail below to the defeat
// animation.
extern "C" uint8_t boss_phase;

/// Marisa's still-ASM helpers
/// --------------------------
/// One left: `proc near` in th02_main.asm's main_03__TEXT block, sitting above
/// marisa_update() in the same segment, and marisa_update() is its only caller.
/// The spelling is the dump's own; it is address-suffixed rather than an IDA
/// placeholder, so naming it is a separate decision that this parcel does not
/// make (`marisa_1AA60` is not matched by tools/re/naming_precheck.py's
/// placeholder pattern, which is keyed on IDA's own kind prefixes).

extern "C" void near marisa_1AA60(void);

/// --------------------------


// The white flash every boss and midboss in this binary blits itself with for
// exactly one frame after being hit. `[measured]` Every one of the 12
// references left in th02_main.asm falls into the same three-role pattern,
// across all three bosses that share the slot: the hit test raises it
// (mima_17C92(), midboss4_1A044(), marisa_1AA60()), the renderer reads it,
// blits the sprite in white and immediately lowers it again (mima_17F27(),
// midboss4_19F52(), marisa_1AE98() below), and each init function clears it.
// That is exactly [stone_hit_flash]'s shape one level up, which is where the
// name comes from; th02/main/bullet/bullet.cpp held it for naming review
// because none of its own three sites could show the read-then-clear half.
// A kb/codegen/0123 alias rather than a rename, because 12 of the 13 accesses
// are still in th02_main.asm.
extern "C" bool boss_hit_flash;

// th02/main/player/shot_hittest.cpp, which declares it the same way rather
// than giving it a header. The number of frames a bomb damages what it covers
// on is `(stage_frame & this) == 0`, so raising it thins the bomb's damage
// rate rather than turning it off.
extern "C" uint8_t bomb_damage_frame_mask;


/// The angle accumulators the patterns below sweep their aim with
/// -------------------------------------------------------------------
/// All four are `db` slots in th02_main.asm's own spelling, address-suffixed
/// rather than IDA placeholders, and each is read and written by exactly one
/// of the patterns below - [angle_26D7F] by marisa_1B24A(), [angle_26D80] by
/// marisa_1B35F(), [angle_26D87] by marisa_1B6DA(), [angle_26D88] by
/// marisa_1B7D3(). Retiring the address suffix would be a rename of a slot no
/// other function touches, which this parcel does not need to make in order to
/// lift the bodies.
///
/// [marisa_swoop_angle] is deliberately NOT in this class even though it is
/// the same kind of `db` slot: it is not an aim accumulator but one of the
/// four slots of a single mechanism whose other three are IDA placeholders,
/// so this parcel has to rule on them anyway and leaving the fourth
/// address-suffixed would split one decision across two spellings.

extern "C" uint8_t angle_26D7F;
extern "C" uint8_t angle_26D80;
extern "C" uint8_t angle_26D87;
extern "C" uint8_t angle_26D88;
/// -------------------------------------------------------------------


// Set once by marisa_init() and cleared by the first wrap of [marisa_pattern]
// case -1, so it is true for that pattern's first run in a fight and false
// for every later one. `[measured]` It picks the muzzles case -1 sprays from:
// true fires a fast pellet from each of Marisa's two hands, false a slow one
// from between them, so the pattern is at its strongest the first time she
// uses it. A kb/codegen/0123 alias rather than a rename, because marisa_init()
// still writes the slot from th02_main.asm.
extern "C" bool marisa_spray_is_first_run;


// The per-frame hit test for Marisa's four orbs, and the only place the player
// can collide with one. Every alive orb takes the damage of every player shot
// that overlaps its MARISA_ORB_W x MARISA_ORB_H box, flashes white for that
// frame, and is shot down at MARISA_ORB_DAMAGE_MAX - dropping a big power item
// unless a [power]-weighted roll says otherwise, and 5000 points either way.
//
// [bomb_damage_frame_mask] is raised to 3 for the duration and dropped back to
// 1 afterwards, so a bomb damages the orbs on every 4th frame rather than on
// every 2nd one.
extern "C" void near marisa_1AB35(void)
{
	register int i;

	bomb_damage_frame_mask = 3;
	for(i = 0; i < MARISA_ORB_COUNT; i++) {
		if(marisa_orb_flag[i] == MOF_ALIVE) {
			if(shots_hittest(
				(*marisa_orb_left_on_back_page[i] + 4),
				*marisa_orb_top_on_back_page[i],
				MARISA_ORB_W,
				MARISA_ORB_H
			)) {
				marisa_orb_hit_flash[i] = true;
				marisa_orb_damage[i]++;
				if(marisa_orb_damage[i] >= MARISA_ORB_DAMAGE_MAX) {
					if((randring2_next8() % 80) >= power) {
						items_add(
							(*marisa_orb_left_on_back_page[i] + 8),
							(*marisa_orb_top_on_back_page[i] + 8),
							IT_BIGPOWER
						);
					}
					snd_se_play(2);
					marisa_orb_flag[i] = MOF_KILL_ANIM;
					score_delta += 5000;
				}
			}
			if(
				(player_left_on_page[page_front] >
					(*marisa_orb_left_on_back_page[i] - MARISA_ORB_PLAYER_HITBOX)
				) &&
				(player_left_on_page[page_front] <
					(*marisa_orb_left_on_back_page[i] + MARISA_ORB_PLAYER_HITBOX)
				) &&
				(player_top_on_page[page_front] >
					(*marisa_orb_top_on_back_page[i] - MARISA_ORB_PLAYER_HITBOX)
				) &&
				(player_top_on_page[page_front] <
					(*marisa_orb_top_on_back_page[i] + MARISA_ORB_PLAYER_HITBOX)
				)
			) {
				player_is_hit = PLAYER_HIT;
			}
		}
	}
	bomb_damage_frame_mask = 1;
}


// Marisa's defeat animation, and the transition out of the fight. Layers up to
// three boss_explode_render() rings, 24 frames apart, then either keeps
// blitting her regular two-pattern sprite or - from frame 32 on - hands her to
// super_zoom() at a factor that grows by 1 every 16 frames. Returns true on
// the frame the animation ends, at which point [boss_phase] is 3.
extern "C" bool16 near marisa_1AC7B(void)
{
	register int zoom = 10;

	boss_explode_render(
		(marisa_topleft.x + MARISA_CENTER_OFFSET),
		(marisa_topleft.y + MARISA_CENTER_OFFSET),
		marisa_defeat_frame
	);
	if(marisa_defeat_frame >= 24) {
		if(marisa_defeat_frame == 56) {
			boss_explode_angle_offset = 32;
		}
		boss_explode_render(
			(marisa_topleft.x + MARISA_CENTER_OFFSET),
			(marisa_topleft.y + MARISA_CENTER_OFFSET),
			(marisa_defeat_frame - 24)
		);
		if(marisa_defeat_frame >= 48) {
			if(marisa_defeat_frame == 80) {
				boss_explode_angle_offset = 0;
			}
			boss_explode_render(
				(marisa_topleft.x + MARISA_CENTER_OFFSET),
				(marisa_topleft.y + MARISA_CENTER_OFFSET),
				(marisa_defeat_frame - 48)
			);
			if(marisa_defeat_frame >= 64) {
				if(!(marisa_defeat_frame & 15)) {
					sparks_add(
						(marisa_topleft.x + (MARISA_W / 2)),
						(marisa_topleft.y + (MARISA_W / 2)),
						to_sp(8.5f),
						24,
						true
					);
				}
			}
		}
	}
	zoom += ((marisa_defeat_frame - 32) >> 4);
	marisa_defeat_frame++;
	if(marisa_defeat_frame < 32) {
		super_put_rect(marisa_topleft.x, marisa_topleft.y, patnum_2064E);
		super_put_rect(
			(marisa_topleft.x + (MARISA_W / 2)),
			marisa_topleft.y,
			(patnum_2064E + 1)
		);
	} else {
		super_zoom(marisa_topleft.x, marisa_topleft.y, zoom, 3);
	}
	if(marisa_defeat_frame >= MARISA_DEFEAT_FRAMES) {
		marisa_defeat_frame = 0;
		boss_phase = 3;
		return true;
	}
	return false;
}


// One frame of orb [orb_i]'s removal animation: a small burst of sparks, and
// an 8-cel sprite at MARISA_ORB_KILL_FRAMES_PER_CEL frames each. Returns true
// on the frame the animation ends, which is also where the orb's sprite is
// unblitted from the front page - the back page's copy is already gone by
// then, because marisa_bg_render() clears the whole playfield there.
extern "C" bool16 near marisa_1AD80(int orb_i)
{
	int patnum;
	int vram_left;

	sparks_add(
		(*marisa_orb_left_on_back_page[orb_i] + 16),
		(*marisa_orb_top_on_back_page[orb_i] + 16),
		to_sp(3.75f),
		2,
		false
	);
	patnum = MARISA_ORB_KILL_PATNUM;
	patnum += (marisa_orb_kill_frame[orb_i] / MARISA_ORB_KILL_FRAMES_PER_CEL);
	marisa_orb_kill_frame[orb_i]++;
	if(marisa_orb_kill_frame[orb_i] >= MARISA_ORB_KILL_FRAMES) {
		marisa_orb_kill_frame[orb_i] = 0;
		marisa_orb_flag[orb_i] = MOF_REMOVED;
		graph_accesspage(page_front);
		grcg_setcolor(GC_RMW, 0);
		vram_left = (
			marisa_orb_left_on_page[page_front][orb_i] >> BYTE_BITS
		);
		grcg_byteboxfill_x(
			vram_left,
			marisa_orb_top_on_page[page_front][orb_i],
			(vram_left + 5),
			(marisa_orb_top_on_page[page_front][orb_i] + 32)
		);
		grcg_off();
		graph_accesspage(page_back);
		marisa_orb_damage[orb_i] = 0;
		return true;
	}
	super_put_rect(
		*marisa_orb_left_on_back_page[orb_i],
		*marisa_orb_top_on_back_page[orb_i],
		patnum
	);
	return false;
}


// Blits Marisa herself, as the two 48x96 patterns she is drawn from. Regular
// frames go through super_put_rect(); the one frame after she was hit is
// instead blitted by hand into the GRCG, one 16-dot chunk at a time, from the
// raw superimpose pattern data. `[measured]` Since the pattern data is shifted
// by hand into each chunk, the loop can and does write to unaligned X
// positions, which super_put_rect() would round down to a byte boundary - so
// the flash is not just a recolor of the regular blit, it also sits at
// Marisa's exact X position.
//
// ZUN bloat: The two halves are the same loop, differing only in the pattern
// number, the VRAM column and the X coordinate they start at.
extern "C" void near marisa_1AE98(void)
{
	register int col;
	screen_x_t x;
	int row;
	vram_offset_t vram_offset;
	vram_offset_t vram_offset_first;
	screen_y_t y;
	uint8_t shift_r;
	dots8_t far *p;
	uint8_t shift_l;
	int chunk;

	if(boss_hit_flash) {
		shift_r = (marisa_topleft.x & (BYTE_DOTS - 1));
		shift_l = (16 - shift_r);
		boss_hit_flash = false;
		vram_offset_first = vram_offset_shift(
			marisa_topleft.x, marisa_topleft.y
		);
		vram_offset = vram_offset_first;
		grcg_setcolor(GC_RMW, 4);
		p = reinterpret_cast<dots8_t far *>(
			MK_FP(super_patdata[patnum_2064E], 0)
		);
		row = 0;
		y = marisa_topleft.y;
		while(row < MARISA_H) {
			if(y >= PLAYFIELD_BOTTOM) {
				break;
			}
			col = 0;
			x = marisa_topleft.x;
			while(col < ((MARISA_W / 2) / BYTE_DOTS)) {
				if(
					(x > 0) && (x < PLAYFIELD_RIGHT) && (y >= PLAYFIELD_TOP)
				) {
					chunk = ((*p >> shift_r) + (*p << shift_l));
					grcg_chunk(vram_offset + col, 16) = chunk;
				}
				col++;
				x += BYTE_DOTS;
				p++;
			}
			row++;
			vram_offset += ROW_SIZE;
			y++;
		}
		p = reinterpret_cast<dots8_t far *>(
			MK_FP(super_patdata[patnum_2064E + 1], 0)
		);
		vram_offset = (vram_offset_first + ((MARISA_W / 2) / BYTE_DOTS));
		row = 0;
		y = marisa_topleft.y;
		while(row < MARISA_H) {
			if(y >= PLAYFIELD_BOTTOM) {
				break;
			}
			col = 0;
			x = (marisa_topleft.x + (MARISA_W / 2));
			while(col < ((MARISA_W / 2) / BYTE_DOTS)) {
				if(
					(x > 0) && (x < PLAYFIELD_RIGHT) && (y >= PLAYFIELD_TOP)
				) {
					chunk = ((*p >> shift_r) + (*p << shift_l));
					grcg_chunk(vram_offset + col, 16) = chunk;
				}
				col++;
				x += BYTE_DOTS;
				p++;
			}
			row++;
			vram_offset += ROW_SIZE;
			y++;
		}
		grcg_off();
	} else {
		super_put_rect(marisa_topleft.x, marisa_topleft.y, patnum_2064E);
		super_put_rect(
			(marisa_topleft.x + (MARISA_W / 2)),
			marisa_topleft.y,
			(patnum_2064E + 1)
		);
	}
}


// Renders the orb ring: first the lines that connect every still-alive orb to
// the next one around the ring, then each orb's own sprite. Orbs that were hit
// on this frame are blitted white instead, and clipped away entirely at the
// playfield's edges rather than being clamped into them - which the regular
// blit below does not do, so an orb can only leave the playfield while it is
// not flashing.
extern "C" void near marisa_1B025(void)
{
	register int i;
	register int line_count;
	int alive[MARISA_ORB_COUNT + 1];
	int from;
	int to;

	grcg_setcolor(GC_RMW, 3);
	for(i = 0, line_count = 0; i < MARISA_ORB_COUNT; i++) {
		if(marisa_orb_flag[i] == MOF_ALIVE) {
			alive[line_count] = i;
			line_count++;
		}
	}
	alive[line_count] = alive[0];
	for(i = 0; i < line_count; i++) {
		from = alive[i];
		to = alive[i + 1];
		grcg_line(
			(*marisa_orb_left_on_back_page[from] + 16),
			(*marisa_orb_top_on_back_page[from] + 16),
			(*marisa_orb_left_on_back_page[to] + 16),
			(*marisa_orb_top_on_back_page[to] + 16)
		);
	}
	grcg_off();
	for(i = 0; i < MARISA_ORB_COUNT; i++) {
		if(marisa_orb_flag[i] < MOF_KILL_ANIM) {
			if(marisa_orb_hit_flash[i]) {
				if(
					(*marisa_orb_left_on_back_page[i] > 0) &&
					(*marisa_orb_left_on_back_page[i] < PLAYFIELD_RIGHT) &&
					(*marisa_orb_top_on_back_page[i] >= 0) &&
					(*marisa_orb_top_on_back_page[i] <= PLAYFIELD_H)
				) {
					super_put_1plane(
						*marisa_orb_left_on_back_page[i],
						*marisa_orb_top_on_back_page[i],
						(MARISA_ORB_PATNUM + i),
						0,
						super_plane(V_WHITE)
					);
				}
				marisa_orb_hit_flash[i] = false;
			} else {
				super_put_rect(
					*marisa_orb_left_on_back_page[i],
					*marisa_orb_top_on_back_page[i],
					(MARISA_ORB_PATNUM + i)
				);
			}
		}
	}
}


// The other cast-cel stepper. Same job as marisa_1B665() below, but with the
// five cel changes wired to fixed [boss_phase_frame] values instead of to a
// parameter, and with the middle three playing a different sound. Called by
// marisa_1B24A() and marisa_1B35F(), both below.
extern "C" void near marisa_1B19D(void)
{
	if(boss_phase_frame == 50) {
		snd_se_play(9);
		patnum_2064E = 130;
	} else if(boss_phase_frame == 100) {
		snd_se_play(10);
		patnum_2064E = 132;
	} else if(boss_phase_frame == 108) {
		snd_se_play(10);
		patnum_2064E = 134;
	} else if(boss_phase_frame == 116) {
		snd_se_play(10);
		patnum_2064E = 136;
	} else if(boss_phase_frame == 130) {
		patnum_2064E = 128;
	}
}


// Marisa's quarter of [boss_rank_param]: the two groups case -2 fires, the one
// group case -1 fires, and the number of star pairs marisa_1BC43() rains from
// the playfield's edges. Easy widens both of the aimed spreads by one step and
// halves the rain. Called from marisa_init(), which is still in th02_main.asm
// - the only place in this tree where that dump calls into C++ with a `near`
// call.
extern "C" void near marisa_1B214(void)
{
	if(rank != RANK_EASY) {
		boss_rank_param[0] = BG_5_SPREAD_MEDIUM_AIMED;
		boss_rank_param[1] = BG_4_SPREAD_MEDIUM_AIMED;
		boss_rank_param[2] = BG_2_SPREAD_MEDIUM;
		boss_rank_param[3] = 4;
	} else {
		boss_rank_param[0] = BG_5_SPREAD_WIDE_AIMED;
		boss_rank_param[1] = BG_4_SPREAD_WIDE_AIMED;
		boss_rank_param[2] = BG_1;
		boss_rank_param[3] = 2;
	}
}


// [marisa_pattern] case -1, another of the three she only uses once all four
// orbs are gone. Frame 50 points the aim up and to the right; from frame 101,
// every 3rd frame sprays one BG_1 pellet and turns the aim by 10, so the
// stream rakes a little over a full circle before [boss_phase_frame] wraps at
// 130. The first run of it in a fight fires a fast pellet from each of
// Marisa's hands; every later one fires a single slow pellet from between
// them.
extern "C" void near marisa_1B24A(void)
{
	if(boss_phase_frame < 50) {
		return;
	}
	marisa_1B19D();
	if(boss_phase_frame == 50) {
		angle_26D7F = 0x18;
	} else if(boss_phase_frame == 130) {
		boss_phase_frame = 0;
		marisa_spray_is_first_run = false;
	}
	if(boss_phase_frame <= 100) {
		return;
	}
	if((boss_phase_frame % 3) != 0) {
		return;
	}
	if(marisa_spray_is_first_run) {
		bullets_add_pellet(
			(marisa_topleft.x + 36),
			(marisa_topleft.y + 64),
			angle_26D7F,
			BG_1,
			((3 << 4) + 12)
		);
		bullets_add_pellet(
			(marisa_topleft.x + 52),
			(marisa_topleft.y + 64),
			angle_26D7F,
			BG_1,
			((3 << 4) + 12)
		);
	} else {
		bullets_add_pellet(
			(marisa_topleft.x + 44),
			(marisa_topleft.y + 64),
			angle_26D7F,
			BG_1,
			((3 << 4) + 2)
		);
	}
	angle_26D7F += 10;
}


// [marisa_pattern] case -2, the last of the three. Every 24th frame from frame
// 72, both of the rank-scaled aimed groups marisa_1B214() picked go off at
// once from the same point above her hat, at the same angle and at two
// different speeds, so the two spreads arrive as one thickening wall.
// [boss_phase_frame] wraps at 193.
extern "C" void near marisa_1B2E9(void)
{
	if(boss_phase_frame < 50) {
		return;
	}
	if(boss_phase_frame == 50) {
		snd_se_play(9);
		patnum_2064E = 130;
	}
	if((boss_phase_frame % 24) == 0) {
		bullets_add_pellet(
			(marisa_topleft.x + 64),
			(marisa_topleft.y + 16),
			0x00,
			boss_rank_param[0],
			((3 << 4) + 6)
		);
		bullets_add_pellet(
			(marisa_topleft.x + 64),
			(marisa_topleft.y + 16),
			0x00,
			boss_rank_param[1],
			((3 << 4) + 12)
		);
	}
	if(boss_phase_frame > 192) {
		boss_phase_frame = 0;
		patnum_2064E = 128;
	}
}


// [marisa_pattern] case -3, one of the three she only uses once all four orbs
// are gone. Frame 50 arms the drifting-star motion and picks a random aim
// angle; from frame 100, every 4th frame fires one star from her lower-left
// corner and turns the aim by 0x2B. [boss_phase_frame] wraps at 130, so the
// arming branch runs again on every wrap and the stream keeps curving the
// other way.
extern "C" void near marisa_1B35F(void)
{
	if(boss_phase_frame < 50) {
		return;
	}
	marisa_1B19D();
	if(boss_phase_frame == 50) {
		angle_26D80 = randring2_next8();
		bullet_special.u1.drift_angle = marisa_star_drift_angle;
		bullet_special.u2.drift_speed.v = 2;
		bullet_special.u3.drift_frames = 128;
		marisa_star_drift_angle = (marisa_star_drift_angle * -1);
	} else if(boss_phase_frame == 130) {
		boss_phase_frame = 0;
	}
	if(boss_phase_frame < 100) {
		return;
	}
	if((boss_phase_frame & 3) != 0) {
		return;
	}
	bullets_add_16x16(
		(marisa_topleft.x + 44),
		(marisa_topleft.y + 64),
		angle_26D80,
		BSM_DRIFT_ANGLE_AND_SPEED,
		PAT_BULLET16_STAR,
		((2 << 4) + 8)
	);
	angle_26D80 += 0x2B;
}


// Places every orb that is still alive at its current polar position around
// Marisa's center, and advances its angle by that orb's own delta. Called from
// marisa_bg_render() rather than from any pattern, so the ring keeps turning
// through all eleven of them.
extern "C" void near marisa_1B3DE(void)
{
	register int i;

	for(i = 0; i < MARISA_ORB_COUNT; i++) {
		if(marisa_orb_flag[i] == MOF_ALIVE) {
			marisa_orb_angle[i] += marisa_orb_angle_delta[i];

			// `[measured]` A 32-bit multiply, from `movsx eax` on both
			// operands: the products reach 0x7F00 for a ring at radius 127 and
			// would still fit in 16 bits, but ZUN wrote the same
			// `(radius * table) >> 8` shape everywhere and Turbo C++ 4.0J
			// widens both sides of it.
			*marisa_orb_left_on_back_page[i] = (
				(
					((long)(marisa_orb_radius[i]) *
					CosTable8[marisa_orb_angle[i]]) >> 8
				) + marisa_topleft.x + 32
			);
			*marisa_orb_top_on_back_page[i] = (
				(
					((long)(marisa_orb_radius[i]) *
					SinTable8[marisa_orb_angle[i]]) >> 8
				) + marisa_topleft.y + 32
			);
		}
	}
}


// [marisa_pattern] case 0, and also the whole of [marisa_intro_step] 1. Spawns
// the four orbs at the cardinal directions on frame 130 and then grows the
// ring outwards at 2 pixels per frame; the pattern itself ends at frame 154,
// so only 24 frames of that growth happen here and every later pattern
// inherits whatever radius it left behind.
extern "C" void near marisa_1B477(void)
{
	int i;
	register screen_x_t center_x;
	register screen_y_t center_y;

	if(boss_phase_frame < 100) {
		return;
	}
	if(boss_phase_frame == 100) {
		snd_se_play(9);
		patnum_2064E = 130;
	} else if(boss_phase_frame == 130) {
		center_x = (marisa_topleft.x + 32);
		center_y = (marisa_topleft.y + 32);
		for(i = 0; i < MARISA_ORB_COUNT; i++) {
			marisa_orb_flag[i] = MOF_ALIVE;
			marisa_orb_angle[i] = (i << 6);
			marisa_orb_radius[i] = 8;
			marisa_orb_angle_delta[i] = 2;

			// Both pages, so that the first frame of the ring does not blit
			// from an uninitialized front-page position.
			marisa_orb_left_on_page[0][i] = center_x;
			marisa_orb_left_on_page[1][i] = center_x;
			marisa_orb_top_on_page[0][i] = center_y;
			marisa_orb_top_on_page[1][i] = center_y;
		}
	} else if(boss_phase_frame == 154) {
		patnum_2064E = 128;
		boss_phase_frame = 0;
	}
	if(boss_phase_frame <= 130) {
		return;
	}
	for(i = 0; i < MARISA_ORB_COUNT; i++) {
		marisa_orb_radius[i] += 2;
	}
}


// [marisa_pattern] case 1. Marisa flies one full circle around the point she
// was standing on, firing a pellet every 16th frame - from a random orb while
// any of them are alive, and from her own upper-right corner as well once they
// are all gone. This is the one pattern marisa_update() does not run
// marisa_1BE72() after, because the circle is her movement.
extern "C" void near marisa_1B555(void)
{
	register int orb_i;

	if(boss_phase_frame < 10) {
		return;
	}
	if(boss_phase_frame == 10) {
		marisa_swoop_direction = (
			((marisa_topleft.x + 32) < player_topleft.x) ? 1 : -1
		);
		marisa_swoop_center_x = marisa_topleft.x;
		marisa_swoop_center_y = (marisa_topleft.y + MARISA_SWOOP_RADIUS);
		marisa_swoop_angle = -0x40;
	}
	marisa_swoop_angle += marisa_swoop_direction;

	// Written through the page-indexed pointers and then read straight back
	// into [marisa_topleft], the same two-step marisa_1BE72() uses.
	*boss_left_on_back_page = (
		((CosTable8[marisa_swoop_angle] * (long)(MARISA_SWOOP_RADIUS)) >> 8) +
		marisa_swoop_center_x
	);
	*boss_top_on_back_page = (
		((SinTable8[marisa_swoop_angle] * (long)(MARISA_SWOOP_RADIUS)) >> 8) +
		marisa_swoop_center_y
	);
	marisa_topleft.x = *boss_left_on_back_page;
	marisa_topleft.y = *boss_top_on_back_page;

	if((boss_phase_frame & 0x0F) == 0) {
		orb_i = randring2_next8_and(3);
		if(marisa_orb_flag[orb_i] == MOF_ALIVE) {
			bullets_add_pellet(
				(*marisa_orb_left_on_back_page[orb_i] + 12),
				(*marisa_orb_top_on_back_page[orb_i] + 12),
				0,
				BG_1_AIMED,
				(5 << 4)
			);
		}

		// ZUN quirk: the orb index decides whether Marisa fires as well, even
		// though no orb is left to pick at this point. The random draw is
		// still made, and still thrown away 3 times out of 4, so this shot
		// comes out at a quarter of the rate the branch above it reads as.
		if(
			(marisa_orb_flag_sum == (MARISA_ORB_COUNT * MOF_REMOVED)) &&
			(orb_i < 1)
		) {
			bullets_add_pellet(
				(marisa_topleft.x + 64),
				(marisa_topleft.y + 16),
				0,
				BG_3_SPREAD_NARROW_AIMED,
				(5 << 4)
			);
		}
	}
	if(boss_phase_frame >= 266) {
		boss_phase_frame = 0;
	}
}


// Steps [patnum_2064E] through Marisa's five cast cels at fixed offsets from
// [frame], and plays the cast sound on the first of them. `pascal` (`retn 2`),
// hence the C++ mangling rather than the `extern "C"` the rest of this file
// uses; th02/formats/mpn.hpp declares mpn_put_8() the same way.
//
// The cel at [frame] and the one at ([frame] + 30) are the same one, so the
// pose is held for 30 frames before the four-cel cast animation runs.
extern "C++" void pascal near marisa_1B665(int frame)
{
	if(boss_phase_frame < frame) {
		return;
	}
	if(boss_phase_frame == frame) {
		snd_se_play(9);
		patnum_2064E = 130;
	} else if((frame + 30) == boss_phase_frame) {
		patnum_2064E = 130;
	} else if((frame + 36) == boss_phase_frame) {
		patnum_2064E = 132;
	} else if((frame + 42) == boss_phase_frame) {
		patnum_2064E = 134;
	} else if((frame + 50) == boss_phase_frame) {
		patnum_2064E = 136;
	} else if((frame + 60) == boss_phase_frame) {
		patnum_2064E = 128;
	}
}


// [marisa_pattern] case 2. The orb ring breathes - out for 230 frames, back in
// for the next 230 - while Marisa fires a single slowly-turning pellet stream
// from one of two muzzles. Which muzzle, and how fast the stream turns, is
// decided by whether any orb is still alive.
extern "C" void near marisa_1B6DA(void)
{
	register int i;

	if(boss_phase_frame < 50) {
		return;
	}
	if(boss_phase_frame < 280) {
		for(i = 0; i < MARISA_ORB_COUNT; i++) {
			marisa_orb_radius[i]++;
		}
	} else if(boss_phase_frame < 510) {
		for(i = 0; i < MARISA_ORB_COUNT; i++) {
			marisa_orb_radius[i]--;
		}
	} else {
		boss_phase_frame = 0;
		patnum_2064E = 128;
	}
	marisa_1B665(220);
	if(boss_phase_frame == 220) {
		angle_26D87 = 0;
	}
	if(marisa_orb_flag_sum == (MARISA_ORB_COUNT * MOF_REMOVED)) {
		patnum_2064E = 130;
	}
	// Once the orbs are gone the stream fires unconditionally and forever;
	// while they are alive it only fires during a 20-frame window, and even
	// then only on the frames the rank gate below lets through.
	if(
		((boss_phase_frame >= 250) && (boss_phase_frame < 270)) ||
		(marisa_orb_flag_sum == (MARISA_ORB_COUNT * MOF_REMOVED))
	) {
		if(marisa_orb_flag_sum != (MARISA_ORB_COUNT * MOF_REMOVED)) {
			angle_26D87 += 0x0D;

			// ZUN quirk: a difficulty gate written as an arithmetic compare
			// against a parity bit. On RANK_EASY the stream fires on even
			// [boss_phase_frame]s only, i.e. at half rate; on every other rank
			// [rank] is >= 1 and the test can never fail, so the gate has no
			// effect at all above Easy.
			if(rank >= (boss_phase_frame & 1)) {
				bullets_add_pellet(
					(marisa_topleft.x + 44),
					(marisa_topleft.y + 64),
					angle_26D87,
					BG_1,
					((3 << 4) + 2)
				);
			}
		} else {
			angle_26D87 += 0x0A;
			bullets_add_pellet(
				(marisa_topleft.x + 64),
				(marisa_topleft.y + 16),
				angle_26D87,
				BG_1,
				((3 << 4) + 12)
			);
			if(boss_phase_frame > 270) {
				boss_phase_frame = 0;
				patnum_2064E = 128;
			}
		}
	}
}


// [marisa_pattern] case 5. The two orb pairs are driven apart and back
// together on different schedules, and every 4th frame of a 100-frame window
// each orb of the first pair fires a pellet along its own ring angle.
extern "C" void near marisa_1B7D3(void)
{
	register int i;
	register screen_x_t left;
	screen_y_t top;

	if(boss_phase_frame == 10) {
		// ZUN quirk: not a symmetric pair like every other write to this
		// array. Orbs 0 and 2 turn at +3 while 1 and 3 turn at -2, so the two
		// halves of the ring drift apart instead of counter-rotating evenly.
		marisa_orb_angle_delta[0] = 3;
		marisa_orb_angle_delta[2] = 3;
		marisa_orb_angle_delta[1] = -2;
		marisa_orb_angle_delta[3] = -2;
	}
	if(boss_phase_frame < 50) {
		return;
	}
	if(boss_phase_frame < 130) {
		for(i = 0; i < MARISA_ORB_COUNT; i += 2) {
			marisa_orb_radius[i] += 2;
		}
	} else if(boss_phase_frame >= 260) {
		if(boss_phase_frame < 340) {
			for(i = 0; i < MARISA_ORB_COUNT; i += 2) {
				marisa_orb_radius[i] -= 2;
			}
			if(rank != RANK_EASY) {
				for(i = 1; i < MARISA_ORB_COUNT; i += 2) {
					marisa_orb_radius[i]++;
				}
			}
		} else if(boss_phase_frame < 420) {
			if(rank != RANK_EASY) {
				for(i = 1; i < MARISA_ORB_COUNT; i += 2) {
					marisa_orb_radius[i]--;
				}
			}
		} else {
			marisa_orb_angle_delta[0] = 2;
			marisa_orb_angle_delta[2] = 2;
			marisa_orb_angle_delta[1] = -2;
			marisa_orb_angle_delta[3] = -2;
			boss_phase_frame = 0;
			patnum_2064E = 128;
		}
	}
	marisa_1B665(140);
	if(boss_phase_frame == 140) {
		angle_26D88 = 0;
	}
	if(marisa_orb_flag_sum == (MARISA_ORB_COUNT * MOF_REMOVED)) {
		patnum_2064E = 130;

		// Turns the other way from the orb version below, and faster on the
		// higher ranks: 0x18 per frame on Easy down to 0x15 on Extra.
		angle_26D88 -= (0x18 - rank);
		bullets_add_16x16(
			(marisa_topleft.x + 64),
			(marisa_topleft.y + 16),
			angle_26D88,
			BSM_1,
			PAT_BULLET16_STAR,
			((3 << 4) + 12)
		);
		return;
	}
	if(boss_phase_frame < 150) {
		return;
	}
	if(boss_phase_frame >= 250) {
		return;
	}
	if((boss_phase_frame & 3) != 0) {
		return;
	}
	angle_26D88 += 3;
	snd_se_play(10);
	for(i = 0; i < MARISA_ORB_COUNT; i += 2) {
		if(marisa_orb_flag[i] == MOF_ALIVE) {
			left = *marisa_orb_left_on_back_page[i];
			top = *marisa_orb_top_on_back_page[i];

			// ZUN quirk: the four bounds do not agree with each other or with
			// the playfield. The x pair is screen-space and tested against 0
			// rather than PLAYFIELD_LEFT, so an orb in the left margin still
			// fires; the y pair is tested against PLAYFIELD_H, which is 16
			// short of PLAYFIELD_BOTTOM in the same space. Three of the four
			// are strict and the fourth is not.
			if(
				(left > 0) && (left < PLAYFIELD_RIGHT) &&
				(top >= 0) && (top <= PLAYFIELD_H)
			) {
				bullets_add_pellet(
					(left + 12),
					(top + 12),
					(marisa_orb_angle[i] + angle_26D88),
					BG_1,
					((4 << 4) + 6)
				);
			}
		}
	}
}


// [marisa_pattern] case 6, the one marisa_update() forces once
// MARISA_PATTERNS_PER_ROUND regular patterns have been seen. Every surviving
// orb fires a fan of 8 pellets on frame 100 and a wider fan of 12 on frame
// 110, and is then shot down by the pattern itself - which is how a round
// ends.
extern "C" void near marisa_1B996(void)
{
	register int i;
	register screen_x_t left;
	screen_y_t top;
	int angle;

	if(boss_phase_frame == 10) {
		marisa_orb_angle_delta[0] = -3;
		marisa_orb_angle_delta[2] = -3;
		marisa_orb_angle_delta[1] = -3;
		marisa_orb_angle_delta[3] = -3;
	}
	if(marisa_orb_flag_sum == (MARISA_ORB_COUNT * MOF_REMOVED)) {
		patnum_2064E = 128;
		boss_phase_frame = 0;
	}
	if(boss_phase_frame < 50) {
		return;
	}
	if(boss_phase_frame == 50) {
		marisa_orb_angle_delta[0] = -4;
		marisa_orb_angle_delta[2] = -4;
		marisa_orb_angle_delta[1] = -4;
		marisa_orb_angle_delta[3] = -4;
		snd_se_play(9);
		patnum_2064E = 130;
	} else if(boss_phase_frame == 100) {
		snd_se_play(3);
		for(i = 0; i < MARISA_ORB_COUNT; i++) {
			if(marisa_orb_flag[i] == MOF_ALIVE) {
				left = (*marisa_orb_left_on_back_page[i] + 12);
				top = (*marisa_orb_top_on_back_page[i] + 12);
				for(angle = -0x40; angle < 0x40; angle += 0x10) {
					bullets_add_pellet(
						left,
						top,
						(marisa_orb_angle[i] + angle),
						BG_1,
						((4 << 4) + 6)
					);
				}
			}
		}
	} else if(boss_phase_frame == 110) {
		snd_se_play(3);
		for(i = 0; i < MARISA_ORB_COUNT; i++) {
			if(marisa_orb_flag[i] == MOF_ALIVE) {
				left = (*marisa_orb_left_on_back_page[i] + 12);
				top = (*marisa_orb_top_on_back_page[i] + 12);
				for(angle = -0x46; angle < 0x46; angle += 12) {
					bullets_add_pellet(
						left,
						top,
						(marisa_orb_angle[i] + angle),
						BG_1,
						(randring2_next8_and(0x1F) + 0x10)
					);
				}
				marisa_orb_flag[i] = MOF_KILL_ANIM;
			}
		}
	}
}


// [marisa_pattern] case 3. From frame 71, every 5th frame, each surviving orb
// fires one pellet along its own ring angle plus an offset that grows by twice
// that orb's [marisa_orb_angle_delta] per volley - so the four streams sweep
// away from the ring and spiral outwards. The volley speed grows with
// [boss_phase_frame] too, and the whole thing reverses direction and restarts
// at frame 300.
extern "C" void near marisa_1BAFF(void)
{
	register int i;
	register screen_x_t left;
	screen_y_t top;
	int speed;

	if(boss_phase_frame < 50) {
		return;
	}
	if(boss_phase_frame >= 150) {
		marisa_1B665(120);
	}
	if(boss_phase_frame == 50) {
		snd_se_play(9);
		patnum_2064E = 130;
		marisa_volleys_fired = 0;
		marisa_orb_volley_angle[0] = 0;
		marisa_orb_volley_angle[1] = 0;
		marisa_orb_volley_angle[2] = 0;
		marisa_orb_volley_angle[3] = 0;
	} else if(boss_phase_frame == 300) {
		boss_phase_frame = 0;
		// A compound multiply-assign rather than x = (x * -1), and that is
		// kb/codegen/0052 rather than style: the compound form expands via AX as
		// `mov ax, -1` / `imul [x]`, which is what the original has, while the
		// expanded form gives the 386 two-operand `imul ax, ax, -1`. Both are
		// four instructions and neither moves a branch displacement, so only
		// an encoding-level comparison separates them. marisa_1BC43() below
		// keeps its expanded spelling because [marisa_pattern_side] is a char,
		// and the byte case emits the two-operand form either way.
		marisa_orb_angle_delta[0] *= -1;
		marisa_orb_angle_delta[1] *= -1;
		marisa_orb_angle_delta[2] *= -1;
		marisa_orb_angle_delta[3] *= -1;
	}
	if(boss_phase_frame <= 70) {
		return;
	}
	if((boss_phase_frame % 5) != 0) {
		return;
	}
	snd_se_play(10);
	for(i = 0; i < MARISA_ORB_COUNT; i++) {
		if(marisa_orb_flag[i] == MOF_ALIVE) {
			left = (*marisa_orb_left_on_back_page[i] + 12);
			top = (*marisa_orb_top_on_back_page[i] + 12);
			speed = ((boss_phase_frame >> 2) + ((1 << 4) + 9));
			bullets_add_pellet(
				left,
				top,
				(marisa_orb_angle[i] + marisa_orb_volley_angle[i]),
				BG_1,
				speed
			);
			marisa_orb_volley_angle[i] += (marisa_orb_angle_delta[i] * 2);
		}
	}

	// Marisa's own ring only exists once every orb has been shot down, and
	// then only on every other volley.
	if(
		(marisa_orb_flag_sum == (MARISA_ORB_COUNT * MOF_REMOVED)) &&
		((marisa_volleys_fired & 1) == 0)
	) {
		bullets_add_pellet(
			(marisa_topleft.x + 44),
			(marisa_topleft.y + 44),
			boss_phase_frame,
			BG_16_RING,
			((3 << 4) + 2)
		);
	}
	marisa_volleys_fired++;
}


// No `#pragma codestring` here any more, and that is arithmetic rather than
// hope. marisa_update()'s generated jump table needs an ODD number of bytes
// ahead of it inside this object (kb/codegen/0154, and the block above
// marisa_update() below). That byte used to be marisa_1BC43()'s own `retn`,
// borrowed as a one-byte codestring while the rest of the function was still
// in the dump; it stopped being needed once marisa_1BC43() (0x22F) and
// marisa_1BE72() (0x80) were both emitted here, for a prefix of 0x2AF.
//
// Every parcel that prepends a body here re-checks the parity the same way -
// by adding up the map's own lengths for everything this object emits ahead of
// marisa_update(), never by counting dump bytes:
//
//	0x077 marisa_1B19D  0x036 marisa_1B214  0x09F marisa_1B24A
//	0x076 marisa_1B2E9  0x07F marisa_1B35F  0x099 marisa_1B3DE
//	0x0DE marisa_1B477  0x110 marisa_1B555  0x075 marisa_1B665
//	0x0F9 marisa_1B6DA  0x1C3 marisa_1B7D3  0x169 marisa_1B996
//	0x144 marisa_1BAFF  0x22F marisa_1BC43  0x080 marisa_1BE72
//	                                                = 0xD55, odd.
//
// Nine of the fifteen bodies are an odd number of bytes long, so a chain of
// them is not parity-free at every depth. Of the four this parcel adds, only
// the depths after marisa_1B2E9() (0xC09) and after marisa_1B19D() (0xD55) are
// safe; stopping after marisa_1B24A() (0xCA8) or marisa_1B214() (0xCDE) would
// have left an EVEN prefix and silently dropped the pad under a function the
// parcel never edits. That failure is invisible to a per-function funcdiff,
// which is kb/codegen/0119's whole point. The same held one parcel earlier,
// where marisa_1B3DE()'s 0xB14 was the trap and marisa_1B35F() bought the
// parity back.
//
// The next body up is marisa_1B025, 0x178 - so 0xECD, odd, and the lane after
// this one inherits a safe single step again.


// Marisa's star pattern, [marisa_pattern] case 4. Nothing happens for the first
// 50 frames; frame 50 arms the drifting-star motion and starts the orb ring
// spinning, and from frame 71 every 14th frame fires. Up to frame 120 that is
// one star from each orb still alive plus one from Marisa herself; after it, a
// rain of star pairs from the playfield's top edge and from one of its two
// sides. [boss_phase_frame] wraps at 200, so the pattern runs until
// marisa_update() takes it away.
extern "C" void near marisa_1BC43(void)
{
	int i;
	register screen_x_t rain_left;
	unsigned char angle;

	if(boss_phase_frame < 50) {
		return;
	}
	if(boss_phase_frame >= 150) {
		marisa_1B665(120);
	}
	if(boss_phase_frame == 50) {
		snd_se_play(9);
		patnum_2064E = 130;

		// One turn of the ring per 256 frames, leaning whichever way this run
		// of the pattern happens to have.
		bullet_special.u1.drift_angle = marisa_pattern_side;
		bullet_special.u2.drift_speed.v = 0;
		bullet_special.u3.drift_frames = 64;
		marisa_pattern_side = (marisa_pattern_side * -1);

		marisa_orb_angle_delta[0] = 2;
		marisa_orb_angle_delta[2] = 2;
		marisa_orb_angle_delta[1] = -2;
		marisa_orb_angle_delta[3] = -2;
	} else if(boss_phase_frame == 70) {
		marisa_orb_angle_delta[0] = 3;
		marisa_orb_angle_delta[2] = 3;
		marisa_orb_angle_delta[1] = -3;
		marisa_orb_angle_delta[3] = -3;
	} else if(boss_phase_frame == 80) {
		marisa_orb_angle_delta[0] = 4;
		marisa_orb_angle_delta[2] = 4;
		marisa_orb_angle_delta[1] = -4;
		marisa_orb_angle_delta[3] = -4;
	} else if(boss_phase_frame == 200) {
		marisa_orb_angle_delta[0] = 2;
		marisa_orb_angle_delta[2] = 2;
		marisa_orb_angle_delta[1] = -2;
		marisa_orb_angle_delta[3] = -2;
		boss_phase_frame = 0;
	}
	if(boss_phase_frame <= 70) {
		return;
	}
	if((boss_phase_frame % 14) != 0) {
		return;
	}
	snd_se_play(10);
	if(boss_phase_frame <= 120) {
		i = 0;
		while(i < MARISA_ORB_COUNT) {
			if(marisa_orb_flag[i] == MOF_ALIVE) {
				bullets_add_16x16(
					(*marisa_orb_left_on_back_page[i] + 8),
					(*marisa_orb_top_on_back_page[i] + 8),
					-0x40,
					BSM_1,
					PAT_BULLET16_STAR,
					((4 << 4) + 6)
				);
			}
			i++;
		}
		bullets_add_16x16(
			(marisa_topleft.x + MARISA_CENTER_OFFSET),
			(marisa_topleft.y + MARISA_CENTER_OFFSET),
			-0x40,
			BSM_1,
			PAT_BULLET16_STAR,
			((4 << 4) + 6)
		);
	} else {
		// if/else rather than a ternary: the original stores straight into the
		// register variable in each arm, and a ternary does not do that here -
		// it funnels both arms through AX and then copies AX into SI
		// (kb/codegen/0120 from the other side).
		if(marisa_pattern_side == 1) {
			rain_left = (PLAYFIELD_RIGHT - BULLET16_W);
		} else {
			rain_left = PLAYFIELD_LEFT;
		}
		i = 0;
		while(i < boss_rank_param[3]) {
			// Both stars of a pair share one angle, 22.5° off straight down,
			// leaning away from the side the pair is spawned on.
			angle = (
				(randring2_next8_and(7) + (marisa_pattern_side << 4)) + 0x40
			);
			bullets_add_16x16(
				(
					(randring2_next16() % (PLAYFIELD_W - BULLET16_W)) +
					PLAYFIELD_LEFT
				),
				PLAYFIELD_TOP,
				angle,
				BSM_1,
				PAT_BULLET16_STAR,
				(randring2_next8_and(0x1F) + 0x10)
			);

			// ZUN quirk: 320 is neither PLAYFIELD_H nor the height of anything
			// else on screen, so the side spawns are squeezed into the top 320
			// of the playfield's 368 rows.
			bullets_add_16x16(
				rain_left,
				((randring2_next16() % 320) + PLAYFIELD_TOP),
				angle,
				BSM_1,
				PAT_BULLET16_STAR,
				(randring2_next8_and(0x1F) + 0x10)
			);
			i++;
		}
	}
	if(marisa_orb_flag_sum == (MARISA_ORB_COUNT * MOF_REMOVED)) {
		i = 0;
		while(i < MARISA_ORB_COUNT) {
			bullets_add_16x16(
				(marisa_topleft.x + MARISA_CENTER_OFFSET),
				(marisa_topleft.y + MARISA_CENTER_OFFSET),
				((i << 6) + (boss_phase_frame * 2)),
				BSM_DRIFT_ANGLE_CHASE,
				PAT_BULLET16_STAR,
				((4 << 4) + 6)
			);
			i++;
		}
	}
}


// Marisa's drift. Run after nine of the eleven patterns below, and by
// MP_UNSTARTED's own branch as well. Picks a direction on frame 2 of the
// pattern and then walks one pixel per axis per frame for the next 48.
extern "C" void near marisa_1BE72(void)
{
	if(boss_phase_frame == 1) {
		return;
	}
	if(boss_phase_frame == 2) {
		marisa_velocity_x = (
			((marisa_topleft.x + 32) < player_topleft.x) ? 1 : -1
		);
		// ZUN quirk: the vertical pick is not the mirror of the horizontal
		// one. Inside the band it is random rather than player-seeking, and
		// (1 - (n % 3)) can come out 0, so Marisa can spend a whole pattern
		// drifting purely sideways.
		marisa_velocity_y = (
			(marisa_topleft.y <  72) ?  1 :
			(marisa_topleft.y > 108) ? -1 :
			(1 - (randring2_next8() % 3))
		);
	}
	if(boss_phase_frame < 50) {
		*boss_left_on_back_page += marisa_velocity_x;
		*boss_top_on_back_page += marisa_velocity_y;
		marisa_topleft.x = *boss_left_on_back_page;
		marisa_topleft.y = *boss_top_on_back_page;
	}
}


/// Why this function needs an ODD number of bytes ahead of it
/// ---------------------------------------------------------
/// The original has a single padding byte between this function's epilogue and
/// its generated jump table. Reproducing it is kb/codegen/0070's shape, and the
/// three measurements below are what decided the route. All of them were taken
/// from tcc -S listings, so none of them cost a build cycle.
///
/// `[measured]` With this translation unit's code contribution starting at an
/// EVEN offset ahead of this function - which is what a plain lift gives, since
/// it would be the first thing the object emits - Turbo C++ emits no padding at
/// all, and the listings for -a2, -a and no alignment option are byte-identical
/// apart from their debug timestamp record. The natural table offset there is
/// odd (0x26D).
///
/// `[measured]` The prefix is the whole of what this object emits ahead of
/// marisa_update(), and it is arithmetic over the lifted bodies rather than a
/// property of any one pragma: marisa_1BC43() is 0x22F bytes and
/// marisa_1BE72() is 0x80, so the prefix is 0x2AF and odd. While
/// marisa_1BC43() was still in the dump the same parity was bought by handing
/// this object its final `retn` as a one-byte `#pragma codestring`. Either
/// way, a lift that gets the parity wrong is byte-identical in every function
/// body and still loses the pad, which is kb/codegen/0119's failure mode
/// exactly - diff the whole segment, never the two functions.
///
/// `[measured]` With an ODD prefix and -a2, the same compiler emits the pad.
/// Probed across prefixes of 0, 1 and 2 bytes: only the 1-byte prefix produces
/// it. So the alignment is object-relative as kb/codegen/0096 says, but the
/// parity is the other way round from that entry's wording - what Turbo C++
/// aligns is the byte AFTER the table, not the table itself. kb/codegen/0139
/// already recorded one case of this from the other direction.
///
/// `[measured]` The alternative routes are both closed, and closed the way
/// kb/codegen/0070 recorded for TH03: a post-function codestring lands after
/// the table entries, and one placed before or inside the function body lands
/// at the top of the object's code instead.

// Marisa's [boss_update] callback, installed by stage_init(). Returns
// SP_CLEAR once her defeat animation has run out, SP_BOSS on every other
// frame.
//
// `int` rather than [stage_progression_t]: that enum is byte-sized -
// th02_main.asm spells its one variable `_stage_progression db ?` - but the
// original returns its value in the whole of AX. The callback slot in
// th02/main/stage/callback.hpp models the return as the enum anyway, which is
// harmless because stage_loop()'s `stage_progression = boss_update();` only
// ever stores the low byte.

extern "C" int far marisa_update(void)
{
	register int i;
	register screen_x_t particle_left;

	i = 0;
	marisa_orb_flag_sum = 0;
	while(i < MARISA_ORB_COUNT) {
		marisa_orb_flag_sum += marisa_orb_flag[i];
		i++;
	}
	if(marisa_orb_flag_sum == (MARISA_ORB_COUNT * MOF_REMOVED)) {
		boss_pos_x = (marisa_topleft.x + MARISA_CENTER_OFFSET);
		boss_pos_y = (marisa_topleft.y + MARISA_CENTER_OFFSET);
	} else {
		boss_pos_x = -1;
		boss_pos_y = -1;
	}
	boss_phase_frame++;

	// ZUN quirk: the [reduce_effects] arm can never take its branch. It is
	// only reached on an odd [stage_frame], and an odd number can never have
	// its low two bits clear, so the (& 3) test always falls through to the
	// spawn. The effect is the same one particle every 2 frames either way.
	if(((stage_frame & 1) != 0) && (!reduce_effects || ((stage_frame & 3) != 0))) {
		particle_left = ((randring2_next16() % PLAYFIELD_W) + PLAYFIELD_LEFT);
		bg_particles_add(
			particle_left,
			PLAYFIELD_TOP,
			((((PLAYFIELD_LEFT + (PLAYFIELD_W / 2)) - particle_left) / 3) + 0x40)
		);
	}
	if((stage_frame & (MARISA_BG_PARTICLE_COL_FRAMES - 1)) == 0) {
		marisa_bg_particle_col_i++;
		if(marisa_bg_particle_col_i >= MARISA_BG_PARTICLE_COLS_COUNT) {
			marisa_bg_particle_col_i = 0;
		}
		bg_particle_col = MARISA_BG_PARTICLE_COLS[marisa_bg_particle_col_i];
	}
	bg_particles_update_and_render();

	if(boss_phase == 0) {
		if(marisa_intro_step == 0) {
			marisa_1B24A();
			if(boss_phase_frame == 0) {
				marisa_intro_step++;
			}
		} else if(marisa_intro_step == 1) {
			marisa_1B477();
			if(boss_phase_frame == 0) {
				marisa_intro_step++;
				marisa_pattern = 1;
				marisa_patterns_seen = 0;
			}
		} else if(marisa_intro_step == 2) {
			switch(marisa_pattern) {
			case -3:
				marisa_1B35F();
				marisa_1BE72();
				break;
			case -2:
				marisa_1B2E9();
				marisa_1BE72();
				break;
			case -1:
				marisa_1B24A();
				marisa_1BE72();
				break;
			case 0:
				marisa_1B477();
				break;
			case 1:
				marisa_1B555();
				break;
			case 2:
				marisa_1B6DA();
				marisa_1BE72();
				break;
			case 3:
				marisa_1BAFF();
				marisa_1BE72();
				break;
			case 4:
				marisa_1BC43();
				marisa_1BE72();
				break;
			case 5:
				marisa_1B7D3();
				marisa_1BE72();
				break;
			case 6:
				marisa_1B996();
				marisa_1BE72();
				break;
			case MP_UNSTARTED:
				marisa_1BE72();
				if(boss_phase_frame > MARISA_PATTERN_GAP_FRAMES) {
					if(marisa_orb_flag_sum < (MARISA_ORB_COUNT * MOF_REMOVED)) {
						if(marisa_patterns_seen >= MARISA_PATTERNS_PER_ROUND) {
							marisa_pattern = 6;
						} else {
							marisa_pattern = ((randring2_next8() % 5) + 1);
							marisa_patterns_seen++;
						}
					} else if(marisa_orbless_patterns_seen >= 2) {
						marisa_patterns_seen = 0;
						marisa_pattern = 0;
						marisa_orbless_patterns_seen = 0;
						marisa_rounds_done++;
						if(marisa_rounds_done >= 2) {
							marisa_damage_multiplier = 1;
						}
						if(marisa_rounds_done >= MARISA_ROUNDS) {
							boss_phase = 1;
						}
					} else {
						marisa_orbless_patterns_seen++;
						marisa_pattern = (255 - (randring2_next8() % 3));
					}
					boss_phase_frame = 1; // Skip the initial movement
				}
				break;
			}
			if(boss_phase_frame == 0) {
				marisa_pattern = MP_UNSTARTED;
			}
		}
	}

	marisa_1AA60();
	marisa_1AB35();
	if(boss_phase != 0) {
		if(marisa_1AC7B()) {
			return SP_CLEAR;
		}
	} else {
		marisa_1AE98();
	}
	for(i = 0; i < MARISA_ORB_COUNT; i++) {
		if(marisa_orb_flag[i] == MOF_KILL_ANIM) {
			marisa_1AD80(i);
		}
	}
	marisa_1B3DE();
	marisa_1B025();
	return SP_BOSS;
}


// Clears Marisa's background - the entire playfield - on the back page, and
// re-points the boss and orb position caches at that page's slots. Since the
// clear removes everything that would otherwise have to be unblitted, the
// positions the page recorded for the previous frame are dropped in the same
// pass, by carrying the front page's over. Installed into
// [boss_bg_render_func] by stage_init().
extern "C" void far marisa_bg_render(void)
{
	register int i;

	boss_left_on_back_page = &boss_left_on_page[page_back];
	boss_top_on_back_page = &boss_top_on_page[page_back];

	egc_off();
	grcg_setcolor(GC_RMW, 0);
	grcg_byteboxfill_x(
		PLAYFIELD_VRAM_LEFT,
		PLAYFIELD_TOP,
		(PLAYFIELD_VRAM_RIGHT - 1),
		(PLAYFIELD_BOTTOM - 1)
	);
	bg_particles_invalidate();

	*boss_left_on_back_page = boss_left_on_page[page_front];
	*boss_top_on_back_page = boss_top_on_page[page_front];

	for(i = 0; i < MARISA_ORB_COUNT; i++) {
		marisa_orb_left_on_back_page[i] = (
			&marisa_orb_left_on_page[page_back][i]
		);
		marisa_orb_top_on_back_page[i] = (
			&marisa_orb_top_on_page[page_back][i]
		);
		if(marisa_orb_flag[i] < MOF_REMOVED) {
			*marisa_orb_left_on_back_page[i] = (
				marisa_orb_left_on_page[page_front][i]
			);
			*marisa_orb_top_on_back_page[i] = (
				marisa_orb_top_on_page[page_front][i]
			);
		}
	}
	grcg_off();
}

// Runs Marisa's post-battle dialog and the stage clear bonus, then advances to
// the Extra-Stage-eligible Stage 5. Installed into [boss_end] by stage_init().
extern "C" void far marisa_end(void)
{
	dialog_pre();
	dialog_script_stage4_post_animate();
	stage_clear_bonus_animate();
	overlay_stage_leave_animate();
	stage_id++;
}
