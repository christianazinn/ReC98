/// TH02's regular stage enemies, per-frame hit test, death and update
/// ------------------------------------------------------------------
/// The bottom fourteen functions of the BOSS_5_TEXT block below Mima's fight,
/// in dump order, and together they are th02_main.asm's own carve-free tail
/// chain there. th02/main/enemy/enemies.cpp next door holds the two procs at
/// HIGHER addresses, and they are in th02/boss_5.cpp's object rather than
/// this one. Everything here is prepended in dump order, so a later lift out
/// of the same block goes at the TOP of this file.
///
/// THIS FILE IS ITS OWN OBJECT, NOT AN #include INTO th02/boss_5.cpp, and the
/// next lift out of this block must keep it that way. Its contribution is
/// 0xF13 bytes - ODD - and th02/boss_5.cpp sets -a2 for the one-byte
/// alignment pad under mima_update()'s generated jump table. Turbo C++
/// word-aligns that table against the offset the object it emits starts at,
/// which it necessarily treats as 0, so folding an odd contribution in ahead
/// of it drops the pad and shifts every byte after it. kb/codegen/0119
/// measured exactly that on this binary, in this group, and prescribes this
/// remedy: a separate object, named explicitly with -zC because the default
/// segment name would otherwise come from this file's own basename
/// (kb/codegen/0105), and one Tupfile.lua line ahead of th02/boss_5.cpp so
/// TLINK concatenates the two in dump order.
///
/// EVERY GROUP OF BODIES PREPENDED HERE HAS TO SUM TO AN EVEN NUMBER, for the
/// two independent reasons above and below: an odd prefix re-rolls
/// th02/boss_5.cpp's own start, and an odd prefix ahead of enemy_run() deletes
/// the pad under its jump tables. Both have held for every lift so far - 0x86
/// twice, then 0x1A7, then 0x20C, then 0x24 - and every one of them was
/// measured on this object's PUBDEFs with tools/pi-audit/obj_probe.py before
/// the build, never on a `tcc -S` listing.
///
/// -G for the `push bp; mov bp, sp; sub sp, N` prologs (kb/codegen/0011).
/// -zPmain_03 so this object's near calls and near jump-table entries resolve
/// against group main_03's base rather than the segment's, which is what the
/// original encodes (kb/codegen/0104). Nothing in the segment's root block is
/// a call target of this object any more.
/// -a2 for enemy_run()'s three generated jump tables and the
/// one pad in front of them, which only lands at the original's parity while
/// the bytes emitted ahead of enemy_run() here sum to an ODD number.
#pragma option -zCBOSS_5_TEXT -zPmain_03 -G -a2

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/rank.h"
#include "th02/v_colors.hpp"
#include "th02/core/globals.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/score.hpp"
#include "th02/main/item/item.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/entity.hpp"
#include "th02/main/enemy/enemy.hpp"
#include "th02/main/midboss/midboss.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/shot.hpp"
#include "th02/main/spark.hpp"
#include "th02/math/randring.hpp"
#include "th02/math/vector.hpp"
#include "th02/main/tile/tile.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/snd/snd.h"

// Declared here rather than in th02/main/enemy/enemy.hpp for the same reason
// th02/main/enemy/enemies.cpp declares it there: that header belongs to the
// loader's parcel. (Same for [enemy_cur] and its satellites below.)
extern "C" uint8_t enemies_loop_bound;

// The [spawn_grid] row enemies_spawn() will look at next. Reset to 0 by
// enemies_reset(), which also frees every [enemies] slot; nothing else writes it,
// and nothing bounds-checks it beyond the [spawn_rows] test below.
//
// COINAGE, and it joins a family already under a standing rename:
// state/re/NAMING_REVIEW_VERDICTS_9.md rules [spawn_grid], [spawn_rows] and
// SPAWN_COLUMN_COUNT over to an enemy_spawn_ stem, unapplied so far -- written
// without a code span, because nothing in the tree carries that stem yet. Named
// to match its two published siblings rather than pre-empting that rename, so
// the family stays consistent in the interim — it makes that item four names,
// not three. `_cur` is the tree's cursor suffix (enemy_cur, pattern_cur,
// enemy_template_cur); bare `cursor` as game state has zero precedent.
extern "C" int spawn_row_cur;

// th02/main/boss/b3.cpp, b4.cpp and b5m.cpp each declare these three the same
// way. -1 means "no enemy is a homing target this frame"; the loop below
// re-derives them every frame from the lowest still-shootable enemy.
extern "C" int boss_pos_x;
extern "C" int boss_pos_y;

/// The current-enemy cursor block
/// ------------------------------
/// TH02's enemy code has no `enemy` parameter anywhere. Instead the loop
/// below publishes the slot it is working on into these file-scope globals,
/// and every helper it calls - enemy_run(), enemy_hittest(),
/// enemy_kill_update_and_render(), sub_16AA7() and the movement step above
/// them, all still ASM - reads them implicitly.
///
/// [enemy_cur] is an ADOPTION: TH04 and TH05 publish a global of exactly this
/// name, type and role (th04/main/enemy/enemy.hpp, `_enemy_cur` in both root
/// dumps). Their loops keep the walk in a `register enemy_t near *enemy` and
/// merely copy it out to the global for the callees; TH02's is the earlier and
/// cruder shape, where the cursor itself IS the global and it carries four
/// derived-pointer satellites. [enemy_template_cur] mirrors that pinned stem.
///
/// THE OTHER SIX ARE COINAGES, not adoptions, and are recorded as such in
/// state/notes/th02-enemies-update-and-render.md: no sibling game has
/// derived-pointer globals at all, so there is no stem to inherit and the
/// search that would have found one is bounded there rather than asserted
/// here. [enemy_left] / [enemy_top] are the position snapshot taken AFTER
/// enemy_run() returns, which is what the playfield clip test and every
/// blit below consume; they are not a loop-top cache of the slot's position.
extern "C" enemy_t near *enemy_cur;
extern "C" enemy_template_t near *enemy_template_cur;
extern "C" screen_x_t near *enemy_left_p;
extern "C" screen_y_t near *enemy_top_p;
extern "C" screen_x_t enemy_left;
extern "C" screen_y_t enemy_top;
extern "C" pixel_delta_8_t near *enemy_velocity_x_p;
extern "C" pixel_delta_8_t near *enemy_velocity_y_p;

// The near half of [enemy_template_cur]'s far [script] pointer, re-derived at
// the top of every enemy_run() call. TH03 publishes a `script_base` of exactly
// this role (th03/main/enemy/enemy.cpp), which fixes the stem; the `enemy_`
// prefix mirrors the cursor block above. NOT `script_p`: all 64 of those in
// the tree are ADVANCING cursors, i.e. instruction pointers, and this one
// never moves -- [enemy_t::script_ip] is the index into it.
extern "C" const uint8_t near *enemy_script_base;

/// The BOSS_5_TEXT tail chain, in dump order
/// -----------------------------------------
/// Six bodies that were th02_main.asm's carve-free tail there until this
/// parcel: the installer that shuts the stage enemies off for a boss fight,
/// the one-pellet shot helper, and the four movement steps enemy_run() below
/// dispatches to. Every name in this block was decided by an earlier parcel -
/// state/notes/th02-enemy-run.md section 5 for the four movement steps,
/// state/notes/th02-enemies-update-and-render.md section 3 for the placeholder
/// - so this one only moves bodies.

// The five-byte SECOND compiled copy of nullfunc_void() that ZUN left in this
// segment - identical bytes at a different address, which is upstream's own
// frame_delay() / frame_delay_2() situation and the reason this carries a `_2`
// rather than a role name. enemies_callbacks_null() below is its only user.
//
// It has to stay in THIS object and at THIS position, ahead of everything else
// here: it is the lowest thing the segment's C++ side contributes, and it had
// no `public` at all while it was ASM, because until the lift below its only
// reference lived in the same dump.
extern "C" void far nullfunc_void_2(void)
{
}


// Frees every [enemies] slot and rewinds the spawn grid to its first row, once
// per stage load. stage_init() is the only caller.
//
// MIRROR of lasers_reset() in this same binary (th02/main/laser.cpp): frees
// every slot of one array at the start of a stage and restores one satellite
// variable while it is there - the identical shape, satellite included. The
// rename was argued and left pending in state/notes/enemies_loop_bound.md
// section 4, which reserves the `_reset` verb for whole-array frees and names
// this proc as its obvious holder; `enemies_` is this binary's own array-scope
// stem. No identifier of this name exists anywhere in the tree, in any of the
// five games, or in upstream master - bounded census in
// state/notes/th02-enemies-reset.md.
extern "C" void far enemies_reset(void)
{
	int i;

	for(i = 0; i < ENEMY_COUNT; i++) {
		enemies[i].flag = F_FREE;
	}
	spawn_row_cur = 0;
}

// th02/main/stage/callback.hpp declares these two slots, but it needs
// th02/main/stage/stage.hpp and neither header has an include guard. Declared
// here in that header's exact spelling instead - the same call
// th02/main/laser.cpp makes for the laser pair, and for the same reason.
extern void (far *enemies_invalidate_func)(void);
extern void (far *enemies_update_and_render_func)(void);

// Disables the stage enemies for the rest of a boss fight, by pointing both
// per-frame slots at that empty function. th02/main/bgm_show.cpp holds this
// name's record and the only call site.
extern "C" void far enemies_callbacks_null(void)
{
	enemies_invalidate_func = nullfunc_void_2;
	enemies_update_and_render_func = nullfunc_void_2;
}


// Fires one pellet from this enemy's bullet origin at the given angle.
// PLACEHOLDER, DELIBERATELY: the bounded search is in
// state/notes/th02-enemies-update-and-render.md section 3 and it FAILED.
// nmlgc has never named it, no sibling game has a one-pellet-per-enemy helper
// to adopt from, and its four call sites (three script opcodes plus the
// autofire gate below) rule out every candidate stem that fits only one of
// them. th02_main.asm published it under this placeholder while it was ASM, in
// the TWO-line form, because `pascal` decorates UPPER CASE with no leading
// underscore (kb/codegen/0086) - so the `public` carried the upper-case
// spelling of the name below, and TASM's case-insensitivity made it the same
// symbol as the `proc` line. Turbo C++ emits that same upper-case PUBDEF for
// the definition below, which is why no alias was needed to replace it.
//
// [angle] is `unsigned char`, not `int`: opcode 33 hands it `word ptr [bx+1]`
// straight from the script, and an `int` parameter makes Borland zero-extend
// the byte through `mov al` / `mov ah, 0` / `push ax` at all four call sites.
extern "C" void pascal near sub_16AA7(unsigned char angle)
{
	bullets_add_pellet(
		(enemy_template_cur->bullet_origin_x + *enemy_left_p),
		(enemy_template_cur->bullet_origin_y + *enemy_top_p),
		angle,
		enemy_cur->pellet_group,
		enemy_cur->pellet_speed
	);
}


// Adds this enemy's signed byte velocities to its position, and NOTHING else.
// TH04 and TH05 publish enemy_pos_update() for the same step and this adopts
// that name, with the difference disclosed rather than hidden: theirs also
// clips against the playfield and returns whether the enemy left it, while
// TH02 does the clipping in enemies_update_and_render() below.
extern "C" void near enemy_pos_update(void)
{
	*enemy_left_p += *enemy_velocity_x_p;
	*enemy_top_p += *enemy_velocity_y_p;
}


// The same advance, but pointed at the player first. `_AIMED` rather than
// `_at_player` is naming-precedents.md's rule for a behaviour variant that
// aims the original behaviour, over 35 upstream pairs.
//
// ZUN quirk: the aim point is neither the player's top-left nor her center.
// player_center_x() / player_center_y() are +(PLAYER_W / 2) = +16 and
// +(PLAYER_H / 2) = +24; this function adds 24 to X and 16 to Y, i.e. the two
// halves swapped, which aims 8 pixels right of and 8 pixels above her center.
// Written as literals for that reason - the header's helpers would change the
// bytes. [measured]
extern "C" void near enemy_pos_update_aimed(void)
{
	int y2;
	int vector_x;
	int vector_y;

	y2 = (player_topleft.y + 16);
	vector2_between_plus(
		*enemy_left_p, *enemy_top_p, (player_topleft.x + 24), y2, 0,
		vector_x, vector_y, enemy_script_base[enemy_cur->script_ip + 2]
	);
	*enemy_velocity_x_p = vector_x;
	*enemy_velocity_y_p = vector_y;
	*enemy_left_p += vector_x;
	*enemy_top_p += vector_y;
}


// Turns this enemy by [sign] every [operand]-th frame and then advances it.
// COINAGE: no sibling game fuses turning and moving - TH04's .STD opcodes set
// an angle delta inline and call enemy_velocity_set() - and there is no
// in-tree precedent for either half of the `step`/`spin` distinction it draws
// against the next function. Recorded as a coinage in
// state/notes/th02-enemy-run.md, not adopted.
//
// [operand] is where in the script the three bytes this reads start, relative
// to the opcode: opcode 3 passes 3 and opcode 7 passes 2, which is why the
// same three reads serve two differently-shaped opcodes.
extern "C" void pascal near enemy_angle_step_and_move(int operand, int sign)
{
	int vector_x;
	int vector_y;
	unsigned char angle;
	register int ip;

	ip = (enemy_cur->script_ip + operand);
	if((enemy_cur->age % enemy_script_base[ip + 1]) == 0) {
		enemy_cur->angle += sign;
	}
	angle = (enemy_script_base[ip] + enemy_cur->angle);
	vector2(vector_x, vector_y, angle, enemy_script_base[ip + 2]);
	*enemy_left_p += vector_x;
	*enemy_top_p += vector_y;
	*enemy_velocity_x_p = vector_x;
	*enemy_velocity_y_p = vector_y;
}


// Turns this enemy by a script-supplied amount EVERY frame and then advances
// it - the per-frame counterpart of the one above, and a coinage for the same
// reason. Opcode 11, whose four operand bytes are read in order: the sign
// flag, the base angle, the per-frame turn, and the speed.
//
// ZUN bloat: the two position writes go through FAR copies of the two near
// cursor pointers, which costs the two `les` below plus the eight bytes of
// frame they live in. Every other function in this block writes
// [*enemy_left_p] and [*enemy_top_p] directly.
extern "C" void near enemy_angle_spin_and_move(void)
{
	screen_x_t far *left_p;
	screen_y_t far *top_p;
	int vector_x;
	int vector_y;
	int sign;
	unsigned char angle;
	register int ip;

	left_p = enemy_left_p;
	top_p = enemy_top_p;
	ip = (enemy_cur->script_ip + 2);
	sign = (!enemy_script_base[ip++] ? -1 : 1);
	angle = (enemy_script_base[ip++] + enemy_cur->angle);
	enemy_cur->angle += (enemy_script_base[ip++] * sign);
	vector2(vector_x, vector_y, angle, enemy_script_base[ip]);
	*left_p += vector_x;
	*top_p += vector_y;
	*enemy_velocity_x_p = vector_x;
	*enemy_velocity_y_p = vector_y;
}

// Fills in the already-chosen [enemies] slot from a spawn grid cell.
//
// SINGULAR, and NOT an adoption of TH03/TH04/TH05's pinned enemies_add(). All
// three of those find the free slot themselves (th03/main/enemy/enemy.cpp
// walks [enemies] for EFF_FREE inside the function); this one is handed the
// slot by enemies_spawn(). Taking the pinned name would give it two contracts
// across the campaign. It MIRRORS shot_add() / shot_option_add() in this same
// binary instead, whose header states exactly this split, and which are
// singular for exactly this reason. Recorded in state/notes/th02-enemy-spawn.md.
//
// [left] is a `register` parameter, which is why the prolog copies it into SI
// rather than reading [bp+0Ah] at each of its three uses.
extern "C" void pascal near enemy_add(
	register screen_x_t left, screen_y_t top, int slot, int template_id
)
{
	enemies[slot].flag = F_ALIVE;
	enemies[slot].pos_on_page[page_back].x = left;
	enemies[slot].pos_on_page[page_back].y = top;
	enemies[slot].patnum_delta = 0;
	enemies[slot].script_ip = 0;
	enemies[slot].template_id = template_id;
	enemies[slot].age = 0;

	// Not 0 - two enemies of the same type spawned on the same frame would
	// otherwise animate in lockstep.
	enemies[slot].anim_frame = randring2_next8();

	enemies[slot].angle = 0;
	enemies[slot].not_shootable = false;
	enemies[slot].no_player_collision = false;

	// 208 is not the center of the playfield (which would be 192), and no
	// other constant in this binary shares it.
	//
	// ZUN bloat: The `? 1 : 0` is redundant on a comparison - but it is not
	// cosmetic, and it is the one thing in this function that cannot be
	// inferred from the field's type. Written as a bare `(left < 208)`,
	// Turbo C++ computes the [enemies] element address FIRST and spills BX
	// across the comparison's two basic blocks (`push bx` / `pop bx`); the
	// ternary makes it evaluate the value first and compute the address after,
	// which is what the original does. `[measured]` on `tcc -S`, against five
	// variants of this one statement.
	enemies[slot].spawned_in_left_half = (left < 208) ? 1 : 0;

	enemies[slot].loop_i = 0;
	enemies[slot].despawn_when_offscreen_vertically = false;
	enemies[slot].render_as = 1;
	enemies[slot].in_kill_anim = false;
	enemies[slot].damage = 0;

	// ZUN landmine: `<`, not `<=`. Spawning into the slot that [enemies_
	// loop_bound] already equals leaves the bound where it is, so that enemy
	// is never updated or rendered until enemies_invalidate() recomputes the
	// bound at the end of the frame.
	if(enemies_loop_bound < slot) {
		enemies_loop_bound = (slot + 1);
	}
}


// Spawns every enemy the current [spawn_grid] row asks for, and advances to
// the next row. Called from stage_loop(), on the frames where
// [scroll_step_advanced] is set.
//
// COINAGE. The role does not transfer: TH04's and TH05's stage spawner is
// upstream's std_run(), a bytecode VM walking [std_ip], and TH03's is
// enemy_formations_update_for(). Neither is a grid, so neither name is
// adoptable. The plural-array-then-_spawn shape follows upstream TH01's
// birds_spawn() / flakes_spawn(), and `enemies_` is this binary's own
// array-scope stem (enemies_invalidate, enemies_update_and_render,
// enemies_loop_bound). NOT the stage_enemies_spawn spelling three harness notes
// still propose: `stage_` is a live subsystem prefix owned by
// th02/main/stage/, and this body belongs to the enemy subsystem.
extern "C" void far enemies_spawn(void)
{
	int template_id;
	screen_y_t top;
	register int slot;
	register int col;

	// Two conditions in ONE expression, and every test below nested rather
	// than `continue`d out of. Both are load-bearing under -Z, and neither is
	// visible in a per-statement reading of the original:
	//
	// * Split into two `if(…) { return; }` statements, Turbo C++ reloads
	//   [spawn_row_cur] for the second one; the `&&` lets the first test's AX
	//   carry into the subscript.
	// * With `if(… == -1) { continue; }`, the read below re-emits the whole
	//   `les` for [spawn_grid][col]; nested inside `if(… != -1)`, the compiler
	//   keeps ES and reloads only the offset, which is what the original does.
	//
	// `[measured]` on `tcc -S`: the `continue` shape and the early-return
	// shape each cost exactly one of those two instructions.
	//
	// Column 0 holds the [scroll_step] the row fires at, as a word. Rows are
	// walked in order and at most one is consumed per call, so a row whose
	// trigger the scrolling skipped past stalls every row behind it.
	if(
		(spawn_row_cur < spawn_rows) &&
		(spawn_grid[0][spawn_row_cur] == scroll_step)
	) {
		for(col = 1; col < SPAWN_COLUMN_COUNT; col++) {
			if(spawn_grid[col][spawn_row_cur] != -1) {
				template_id = spawn_grid[col][spawn_row_cur];
				top = enemy_templates[template_id].spawn_top;

				// ZUN bloat: Both of the above are dead if the midboss is
				// active. Testing [midboss_active] first, or once outside the
				// loop, would have been the same behavior.
				if(!midboss_active) {
					for(slot = 0; slot < ENEMY_COUNT; slot++) {
						if(enemies[slot].flag == F_FREE) {
							// The column *is* the X position: 0x31 columns,
							// 8 pixels apart, from the left edge + 16.
							enemy_add(
								((col * 8) + 16), top, slot, template_id
							);
							break;
						}
					}
				}
			}
		}
		spawn_row_cur++;
	}
}


// One opcode of this enemy's script, which the caller runs in a loop.
// Returns 2 when the enemy is finished for good, 1 when another opcode should
// run this frame, and 0 when this one has consumed the frame.
//
// TH03 and TH04 both call theirs enemy_run() (th03/main/enemy/enemy.cpp,
// th04/main/enemy/script.cpp), and TH02's has the same status-return shape.
//
// The three generated jump tables below this function, and the single
// alignment pad in front of them, are its own codegen. Turbo C++'s OBJ writer
// pads when the table's natural OBJECT-LOCAL offset comes out ODD, so the pad
// survives only while everything emitted ahead of enemy_run() in this object
// sums to an odd number of bytes: 0x1A7 today, from enemy_add() and
// enemies_spawn(). `tcc -S` disagrees with the object here and the object is
// what links, so grade this on obj_probe.py, never on the listing
// (kb/codegen/0154, corrected against its own evidence).
//
// The `case` order is ZUN's, not numeric, and it is load-bearing: Turbo C++
// emits case bodies in source order, so writing them in numeric order moves
// every handler and matches nothing.
extern "C" int near enemy_run(void)
{
	int sign;
	uint8_t angle;
	uint8_t advance;
	uint8_t speed;
	uint8_t velocity_y;
	uint8_t ip;
	uint8_t opcode;
	int8_t sign_a;
	int8_t sign_b;
	register screen_y_t top;

	enemy_script_base = reinterpret_cast<const uint8_t near *>(
		enemy_template_cur->script
	);
	ip = enemy_cur->script_ip;
	opcode = enemy_script_base[ip];
	switch(opcode) {
	case 0:
		advance = 2;
		goto wait;
	case 1:
		*enemy_velocity_x_p = enemy_script_base[ip + 2];
		velocity_y = enemy_script_base[ip + 3];
		*enemy_velocity_y_p = velocity_y;
		enemy_pos_update();
		advance = 4;
		goto wait;
	case 10:
		enemy_pos_update();
		advance = 2;
		goto wait;
	case 170:
		angle = randring2_next8();
		speed = enemy_script_base[ip + 1];
		*enemy_velocity_x_p = ((speed * CosTable8[angle]) >> 8);
		*enemy_velocity_y_p = ((speed * SinTable8[angle]) >> 8);
		advance = 2;
		goto advance_and_run;
	case 8:
		sign = (!enemy_cur->spawned_in_left_half ? -1 : 1);
		*enemy_velocity_x_p = (enemy_script_base[ip + 2] * sign);
		*enemy_velocity_y_p = enemy_script_base[ip + 3];
		enemy_pos_update();
		advance = 4;
		goto wait;
	case 9:
		enemy_pos_update_aimed();
		advance = 3;
		goto wait;
	case 2:
		*enemy_top_p += scroll_delta;
		advance = 2;
		goto wait;
	case 3:
		sign_a = (
			!enemy_script_base[enemy_cur->script_ip + 2] ? -1 : 1
		);
		enemy_angle_step_and_move(3, sign_a);
		advance = 6;
		goto wait;
	case 7:
		sign_b = ((enemy_cur->spawned_in_left_half == 1) ? -1 : 1);
		enemy_angle_step_and_move(2, sign_b);
		advance = 5;
		goto wait;
	case 11:
		enemy_angle_spin_and_move();
		advance = 6;
		goto wait;
	case 4:
		*enemy_left_p = (enemy_script_base[ip + 1] << 3);
		*enemy_top_p = (enemy_script_base[ip + 2] << 3);
		advance = 3;
		goto advance_and_run;
	case 5:
		*enemy_left_p = (
			(randring2_next16() % (enemy_script_base[ip + 2] << 3)) +
			(enemy_script_base[ip + 1] << 3)
		);
		*enemy_top_p = (enemy_script_base[ip + 3] << 3);
		enemy_cur->spawned_in_left_half = ((*enemy_left_p < 208) ? 1 : 0);
		advance = 4;
		goto advance_and_run;
	case 6:
		*enemy_top_p = (
			(randring2_next16() % (enemy_script_base[ip + 2] << 3)) +
			(enemy_script_base[ip + 1] << 3)
		);
		*enemy_left_p = (enemy_script_base[ip + 3] << 3);
		advance = 4;
		goto advance_and_run;
	case 16:
		enemy_cur->flag = F_REMOVE;
		return 2;
	case 17:
		enemy_cur->age = 0;
		snd_se_play(3);
		enemy_cur->in_kill_anim = true;
		return 2;
	case 32:
		sub_16AA7(0);
		goto next;
	case 33:
		sub_16AA7(enemy_script_base[ip + 1]);
		advance = 2;
		goto advance_and_run;
	case 34:
		sub_16AA7(enemy_cur->angle);
		enemy_cur->angle += enemy_script_base[ip + 1];
		advance = 2;
		goto advance_and_run;
	case 37:
		enemy_cur->pellet_group = enemy_script_base[ip + 1];
		enemy_cur->pellet_speed = enemy_script_base[ip + 2];
		advance = 3;
		goto advance_and_run;
	case 36:
		bullets_add_16x16(
			(enemy_template_cur->bullet_origin_x + *enemy_left_p),
			(enemy_template_cur->bullet_origin_y + *enemy_top_p),
			enemy_script_base[ip + 1],
			enemy_cur->pellet_group,
			enemy_script_base[ip + 2],
			enemy_cur->pellet_speed
		);
		advance = 3;
		goto advance_and_run;
	case 38:
		top = enemy_script_base[ip + 1];
		top -= scroll_line;
		if(top < 0) {
			top += RES_Y;
		}
		bullets_add_pellet(
			(enemy_template_cur->bullet_origin_x + *enemy_left_p),
			top,
			enemy_script_base[ip + 2],
			enemy_script_base[ip + 3],
			enemy_script_base[ip + 4]
		);
		advance = 5;
		goto advance_and_run;
	case 39:
		top = enemy_script_base[ip + 1];
		top -= scroll_line;
		if(top < 0) {
			top += RES_Y;
		}
		bullets_add_16x16(
			(enemy_template_cur->bullet_origin_x + *enemy_left_p),
			top,
			enemy_script_base[ip + 2],
			enemy_script_base[ip + 3],
			enemy_script_base[ip + 4],
			enemy_script_base[ip + 5]
		);
		advance = 6;
		goto advance_and_run;
	case 160:
		enemy_cur->script_ip = enemy_script_base[ip + 1];
		return 1;
	case 161:
		if(enemy_cur->angle++ < enemy_script_base[ip + 2]) {
			enemy_cur->script_ip = enemy_script_base[ip + 1];
		} else {
			enemy_cur->script_ip += 3;
		}
		return 1;
	case 167:
		if(enemy_cur->loop_i++ < enemy_script_base[ip + 2]) {
			enemy_cur->script_ip = enemy_script_base[ip + 1];
		} else {
			enemy_cur->loop_i = 0;
			enemy_cur->script_ip += 3;
		}
		return 1;
	case 162:
		enemy_cur->angle = enemy_script_base[ip + 1];
		advance = 2;
		goto advance_and_run;
	case 163:
		enemy_cur->render_as = 0;
		goto next;
	case 164:
		enemy_cur->render_as = 1;
		goto next;
	case 165:
		enemy_cur->patnum_delta = enemy_script_base[ip + 1];
		enemy_cur->render_as = 0;
		advance = 2;
		goto advance_and_run;
	case 166:
		enemy_cur->patnum_delta = enemy_script_base[ip + 1];
		enemy_cur->render_as = 2;
		advance = 2;
		goto advance_and_run;
	case 169:
		enemy_cur->despawn_when_offscreen_vertically = true;
		goto next;
	case 168:
		snd_se_play(enemy_script_base[ip + 1]);
		advance = 2;
		goto advance_and_run;
	case 171:
		enemy_cur->not_shootable = true;
		goto next;
	case 172:
		enemy_cur->not_shootable = false;
		goto next;
	case 173:
		enemy_cur->no_player_collision = true;
		goto next;
	case 174:
		enemy_cur->no_player_collision = false;
		goto next;
	case 175:
		tile_ring_set_and_put_both_8(
			*enemy_left_p, enemy_script_base[ip + 1], enemy_script_base[ip + 2]
		);
		advance = 3;
		goto advance_and_run;
	}
wait:
	enemy_cur->age++;
	if(enemy_script_base[ip + 1] <= enemy_cur->age) {
		enemy_cur->script_ip += advance;
		enemy_cur->age = 0;
	}
	return 0;
advance_and_run:
	enemy_cur->script_ip += advance;
	return 1;
next:
	enemy_cur->script_ip++;
	return 1;
}


// The box every hit test and unblit in this file uses, regardless of the
// per-type [w] and [h] that enemy_stagedata_load() derives into each
// [enemy_template_t]. Same spelling, same values and same reason as
// th02/main/enemy/enemies.cpp's pair; promote them into
// th02/main/enemy/enemy.hpp if a third file ever needs them.
static const pixel_t ENEMY_W = 32;
static const pixel_t ENEMY_H = 32;


// Collides this enemy with the player - which starts its death animation and
// hits the player, rather than damaging it - and then returns the damage the
// player's shots did to it this frame. Named after TH03's enemy_hittest()
// (th03/main/enemy/enemy.cpp), which does the same two jobs in one call.
//
// The player box is asymmetric on purpose: X is tested against
// [player_left_on_page] for the page currently on screen, Y against the
// single [player_topleft].
extern "C" int near enemy_hittest(void)
{
	int damage;
	register screen_y_t top;

	if(
		!enemy_template_cur->no_player_collision &&
		!enemy_cur->no_player_collision
	) {
		top = enemy_top;
		if(
			(player_left_on_page[page_front] > (enemy_left - (ENEMY_W / 2))) &&
			(player_left_on_page[page_front] < (enemy_left + (ENEMY_W / 2))) &&
			((top - (ENEMY_H / 2)) < player_topleft.y) &&
			((top + (ENEMY_H / 2)) > player_topleft.y)
		) {
			enemy_cur->age = 0;
			player_is_hit = true;
			enemy_cur->in_kill_anim = true;
		}
	}
	damage = shots_hittest(enemy_left, enemy_top, ENEMY_W, ENEMY_H);
	return damage;
}


// One frame of this enemy's death animation: bursts sparks on the frame it
// starts, advances [age] as the cel timer, blits the cel, and returns `true`
// on the frame it retires the slot to F_REMOVE. 24 frames over 3 frames per
// cel is ENEMY_KILL_CELS (th02/sprites/cels.h) - which is what fixes this
// name's stem - and the two animations start at patnum 0 and patnum 10.
// TH04 and TH05 inline the same animation into their enemy update instead of
// giving it a name.
//
// ZUN quirk, `[measured]`: [enemy_template_cur]'s [explode_sprite] is loaded
// and then thrown away. It only chooses BETWEEN the two hardcoded bases, and
// this is its only read in the whole binary - the field is a two-valued
// selector rather than the patnum its name suggests. Preserved as written.
extern "C" bool16 near enemy_kill_update_and_render(void)
{
	register int patnum;
	register screen_y_t top_scrolled;

	patnum = enemy_template_cur->explode_sprite;
	patnum = (!patnum ? 10 : 0);
	patnum += (enemy_cur->age / 3);
	if(enemy_cur->age == 0) {
		sparks_add(
			(enemy_left + (ENEMY_W / 2)),
			(enemy_top + (ENEMY_H / 2)),
			((1 << 4) + 4),
			8,
			false
		);
	}
	enemy_cur->age++;
	if(enemy_cur->age >= 24) {
		enemy_cur->flag = F_REMOVE;
		return true;
	}
	// No negative wrap here, unlike every other blit in this file: this one
	// only ever adds [scroll_line] to a [top] the clip test already bounded.
	top_scrolled = enemy_top;
	top_scrolled += scroll_line;
	if(top_scrolled >= RES_Y) {
		top_scrolled -= RES_Y;
	}
	super_roll_put(enemy_left, top_scrolled, patnum);
	return false;
}


// Runs and draws every enemy up to [enemies_loop_bound] - the script VM, the
// playfield clip, the animation, the player and shot collisions, the death
// and the blit. Installed into [enemies_update_and_render] by stage_init().
//
// ZUN quirk: the [in_kill_anim] arm tests enemy_kill_update_and_render()'s
// return value and then does the same thing either way. The `if` is in the
// original's instructions, so it is in the original's source; only the two
// arms are indistinguishable.
extern "C" void far enemies_update_and_render(void)

{
	int script_ret;
	screen_y_t lowest_top;
	int damage;
	int interval;
	register int i;
	register screen_y_t top_scrolled;

	script_ret = 0;
	lowest_top = 0;
	boss_pos_x = -1;
	boss_pos_y = -1;
	enemy_cur = enemies;
	for(i = 0; enemies_loop_bound > i; i++, enemy_cur++) {
		if(enemy_cur->flag != F_ALIVE) {
			continue;
		}
		enemy_template_cur = &enemy_templates[enemy_cur->template_id];
		enemy_left_p = &enemy_cur->pos_on_page[page_back].x;
		enemy_top_p = &enemy_cur->pos_on_page[page_back].y;
		enemy_velocity_x_p = &enemy_cur->velocity_x;
		enemy_velocity_y_p = &enemy_cur->velocity_y;
		do {
			if(!enemy_cur->in_kill_anim) {
				script_ret = enemy_run();
			}
			if(script_ret == 2) {
				goto next;
			}
		} while(script_ret == 1);
		enemy_left = *enemy_left_p;
		enemy_top = *enemy_top_p;
		top_scrolled = enemy_top;
		if(
			(enemy_left < (PLAYFIELD_LEFT - 32)) ||
			(enemy_left > (PLAYFIELD_RIGHT + 2))
		) {
			enemy_cur->flag = F_REMOVE;
			goto next;
		}
		if(enemy_cur->despawn_when_offscreen_vertically) {
			if(
				(enemy_top >= PLAYFIELD_BOTTOM) ||
				(enemy_top <= (PLAYFIELD_TOP - 32))
			) {
				enemy_cur->flag = F_REMOVE;
				goto next;
			}
		}
		enemy_cur->anim_frame++;
		if(
			(enemy_template_cur->anim_cels != 0) &&
			(enemy_cur->render_as == 1) &&
			((enemy_cur->anim_frame % enemy_template_cur->anim_frames_per_cel)
				== 0)
		) {
			enemy_cur->patnum_delta++;
			if(enemy_cur->patnum_delta >= enemy_template_cur->anim_cels) {
				enemy_cur->patnum_delta = 0;
			}
		}
		if((enemy_template_cur->hp == -1) || enemy_cur->not_shootable) {
			goto render;
		}
		if(enemy_cur->in_kill_anim) {
			if(enemy_kill_update_and_render()) {
				goto next;
			}
			goto next;
		}
		if((top_scrolled < 360) && (top_scrolled > lowest_top)) {
			boss_pos_x = (enemy_left + 8);
			boss_pos_y = (top_scrolled + 8);
			lowest_top = top_scrolled;
		}
		if((damage = enemy_hittest()) != 0) {
			if(enemy_template_cur->hp == -2) {
				snd_se_play(4);
			} else {
				enemy_cur->damage += damage;
				if(enemy_cur->damage >= enemy_template_cur->hp) {
					enemy_cur->age = 0;
					if(enemy_template_cur->item > 1) {
						items_add(
							(enemy_left + 12),
							enemy_top,
							(enemy_template_cur->item - IT_BOMB)
						);
					} else {
						items_add_semirandom((enemy_left + 12), enemy_top);
					}
					snd_se_play(3);
					score_delta += enemy_template_cur->score;
					if(rank == RANK_LUNATIC) {
						bullets_add_pellet(
							(enemy_template_cur->bullet_origin_x + enemy_left),
							(enemy_template_cur->bullet_origin_y + enemy_top),
							0,
							BG_1_AIMED,
							((3 << 4) + 12)
						);
					}
					enemy_cur->in_kill_anim = true;
					goto next;
				}
				snd_se_play(4);
				top_scrolled += scroll_line;
				if(top_scrolled >= RES_Y) {
					top_scrolled -= RES_Y;
				}
				if(top_scrolled < 0) {
					top_scrolled += RES_Y;
				}
				super_roll_put_1plane(
					enemy_left,
					top_scrolled,
					(enemy_template_cur->patnum + enemy_cur->patnum_delta),
					0,
					super_plane(V_WHITE)
				);
				goto next;
			}
		}
render:
		top_scrolled = enemy_top;
		if(enemy_template_cur->autofire_interval > 1) {
			interval = enemy_template_cur->autofire_interval;
			if((enemy_cur->anim_frame % interval) == 0) {
				sub_16AA7(0);
			}
		}
		if(enemy_template_cur->patnum == 0) {
			goto next;
		}
		top_scrolled += scroll_line;
		if(top_scrolled >= RES_Y) {
			top_scrolled -= RES_Y;
		}
		if(top_scrolled < 0) {
			top_scrolled += RES_Y;
		}
		if(enemy_cur->render_as != 2) {
			super_roll_put(
				enemy_left,
				top_scrolled,
				(enemy_template_cur->patnum + enemy_cur->patnum_delta)
			);
		} else {
			super_roll_put(enemy_left, top_scrolled, enemy_cur->patnum_delta);
		}
next: ;
	}
}
