/// What collecting an item does
/// ----------------------------
/// TH04 only. One case per item type, each awarding a score value and its own
/// side effects, followed by a tail that every case shares: the score goes into
/// [score_delta], a point number is spawned at the item's position, and the
/// rank accumulator is drained if it has filled.
///
/// (#included from th04/main/item/update.cpp, at the very FRONT of the object,
/// ahead of item_left_playfield() and items_update() -- the address order all
/// three have in IT_UPDT_TEXT. This proc, its jump table and the `#define`
/// that stood in for it were the last things th04_main.asm contributed to that
/// segment, so the object grows backwards into the hole and every byte above it
/// keeps its address (kb/codegen 0099 + 0112 + 0114). The second row of
/// state/re/JUMP_TABLE_TAILS.md's class to land in TH04.)
///
/// THE NAME IS NO LONGER INFERRED. update.cpp has spelled the call site
/// `item_collected()` since before either body was read, over a `#define` onto
/// each game's placeholder, and state/notes/items_update.md records that
/// spelling as `[inferred from call sites -- neither placeholder body has been
/// read]`. This lift reads TH04's, and it confirms the inference outright:
/// every arm awards the player something and the shared tail spawns the pickup
/// point number. That note's caveat is discharged for TH04 in this parcel.
/// TH05's item_collected() counterpart is a DIFFERENT body -- same seven types,
/// but the dream and power arms diverge and it has no [total_*_collected]
/// counters.
///
/// Assembly in TH05 means the two are two lifts, not one shared body
/// (kb/codegen/0115).

// Every one of these is published by a root-dump BSS or data block, or by a
// header this translation unit cannot reach. th04/main/hud/hud.hpp and
// th04/resident.hpp are only included by update.cpp's `GAME == 5` arm, and
// th04/main/hud/overlay.hpp is unguarded AND re-expands headers this closure
// already carries, so spelling them here is the th04/main/player/miss.cpp
// route rather than a style choice (kb/codegen/0129).
// ---------------------------------------------------------------------
extern unsigned char bullet_clear_time;
extern unsigned char dream_items_collected;
extern unsigned int items_collected;
extern unsigned long score_delta;

// Published by th04/main/item/items[data].asm. One entry per possible value of
// [dream_items_collected], which is why the index below needs no clamp of its
// own -- the `<= 6` test above it is the clamp.
extern "C" const unsigned int DREAM_SCORE_PER_ITEMS[];

// th04/main/hud/overlay.hpp declares these three; its overlay_popup_show() is
// exactly the pair of stores each popup arm below makes. Spelled with the
// storage type rather than the header's `popup_id_t`, because respelling that
// enum in a shared translation unit would put a second definition of every
// popup ID at file scope.
// The `#pragma codeseg` pair is kb/codegen/0082 and it is load-bearing:
// [overlay2] holds a NEAR pointer, and the offset stored into it has to be
// taken in the group that actually contains the callee. Declared plainly, this
// object's own `-zPmain_03` frames it on main_03 and TLINK rejects the record
// outright -- `Fixup overflow at IT_UPDT_TEXT:..., target =
// overlay_popup_update_and_render()`, four of them, one per popup arm.
// th04/main/hud/overlay.hpp carries exactly this pragma pair for exactly this
// reason, and th04/bullet_u.cpp (also main_03) reaches the same symbol through
// it. The restore names IT_UPDT_TEXT and main_03 explicitly rather than using
// the bare form, so that it cannot land on this file's own basename.
#pragma codeseg HUD_OVRL_TEXT main_01
extern unsigned char overlay_popup_id_new;
extern nearfunc_t_near overlay2;
void pascal near overlay_popup_update_and_render(void);
#pragma codeseg IT_UPDT_TEXT main_03
static const unsigned char ITEM_POPUP_ID_FULL_POWERUP = 3; // POPUP_ID_FULL_POWERUP
static const unsigned char ITEM_POPUP_ID_EXTEND = 1;       // POPUP_ID_EXTEND

// `extern "C"` + `pascal` for the undecorated, Pascal-cased names
// th04/main/hud/hud.hpp declares (kb/codegen/0081). `far` by the large model,
// which is what the plain `call` in the original is: this object is in group
// main_03 and all three of these live in main_01.
extern "C" void pascal hud_point_items_put(void);
extern "C" void pascal hud_dream_put(void);
extern "C" void pascal hud_bombs_put(void);
extern "C" void pascal hud_lives_put(void);

// Far from this main_03 object; same-group callers use a nopcall island.
extern "C" void far player_shot_level_update(void);

// [total_max_valued_point_items_collected] under the <= 32-character alias
// th04/main/item/items[data].asm publishes at the same address, because TLINK
// truncates a C identifier to 32 characters and the real name is 38
// (kb/codegen/0060). Same spelling th04/main/execl.cpp already uses; the
// truncation is a link failure, so the full name is not an option here.
extern unsigned int total_max_valued_point_items;

// [power_overflow]'s cap and the score table indexed by it, both declared by
// th02/main/item/shared.hpp -- which is UNGUARDED and re-expands platform.h,
// already in this closure. Respelled rather than included, same block and same
// reason as everything above it.
//
// UNSIGNED where that header says `int`, and it is codegen rather than
// pedantry: both of this function's tests against the cap are `JB`/`JBE` in
// the original, so the comparison ZUN's source reached was unsigned. Left
// signed, Turbo C++ takes the operator literally (kb/codegen/0092) and emits
// `JL`/`JLE` over identical operands -- one bit of one byte, in two places.
static const unsigned int POWER_OVERFLOW_MAX = 42;
// The backing data has one entry for every inclusive value from 0 through the
// cap.
extern "C" int16_t POWER_OVERFLOW_BONUS[POWER_OVERFLOW_MAX + 1];
// ---------------------------------------------------------------------

// The two counters this function hands out live in the resident structure, so
// this one IS included rather than respelled: the header is guarded, and a
// struct layout cannot be spelled twice the way an `extern` can.
#include "th04/resident.hpp"

void pascal near item_collected(item_t near *item)
{
	// [bp-1], with the odd byte at [bp-2] left as padding (kb/codegen/0010).
	// Set by the arms that award a *maximum*-value pickup, and read once at
	// the bottom to pick the yellow point number over the white one.
	unsigned char yellow;

	// SI, and DELIBERATELY UNINITIALIZED. The `switch` below has no default,
	// so an item whose [type] is above IT_FULLPOWER falls straight through to
	// the tail and spends whatever SI happened to hold. ZUN landmine: the
	// original has no initializing store either, but all original item writers
	// assign one of the handled types.
	unsigned int score;

	yellow = 0;
	switch(item->type) {
	case IT_POWER:
		if(power < POWER_MAX) {
			// The full-power popup fires on the frame power REACHES the cap,
			// which is why the test is `==` against one below it rather than
			// `>=` against the cap after the increment.
			if(power == (POWER_MAX - 1)) {
				overlay_popup_id_new = ITEM_POPUP_ID_FULL_POWERUP;
				overlay2 = overlay_popup_update_and_render;
				if(bullet_clear_time < 20) {
					bullet_clear_time = 20;
				}
			}
			power++;
			player_shot_level_update();
			score = 1;
		} else {
			power_overflow++;
			if(static_cast<unsigned int>(power_overflow) >= POWER_OVERFLOW_MAX) {
				power_overflow = POWER_OVERFLOW_MAX;
				yellow = 1;
			}
			score = POWER_OVERFLOW_BONUS[power_overflow];
			if(pointnum_times_2) {
				item_playperf_raise++;
			}
		}
		break;

	case IT_POINT:
		// Height is the whole of a point item's value: caught at or above the
		// 52-subpixel line it is worth the flat maximum, and below it a linear
		// falloff from 3300.
		if(item->pos.cur.y.v <= to_sp(52)) {
			score = 5120;
			item_playperf_raise += 4;
			total_max_valued_point_items++;
			yellow = 1;
			if(pointnum_times_2) {
				item_playperf_raise += 4;
			}
		} else {
			score = (3300 - (item->pos.cur.y.v / 2));
			item_playperf_raise += 2;
			if(pointnum_times_2) {
				item_playperf_raise += 2;
			}
		}
		total_point_items_collected++;
		score += dream_score;
		stage_point_items_collected++;
		hud_point_items_put();
		break;

	case IT_DREAM:
		// The counter saturates one PAST the table's last index it can reach,
		// so the `<= 6` is the clamp and the lookup below needs none.
		if(dream_items_collected <= 6) {
			dream_items_collected++;
		}
		dream_score = DREAM_SCORE_PER_ITEMS[dream_items_collected];

		// Re-read from the global rather than reused from the store above:
		// the original loads AX, stores it, and then loads SI from memory
		// again.
		score = dream_score;
		hud_dream_put();
		item_playperf_raise += 2;
		if(pointnum_times_2) {
			item_playperf_raise += 2;
		}
		break;

	case IT_BIGPOWER:
		if(power < POWER_MAX) {
			power += 10;
			if(power >= POWER_MAX) {
				power = POWER_MAX;
				overlay_popup_id_new = ITEM_POPUP_ID_FULL_POWERUP;
				overlay2 = overlay_popup_update_and_render;
				if(bullet_clear_time < 20) {
					bullet_clear_time = 20;
				}
			}
			player_shot_level_update();
			score = 1;
		} else {
			// ZUN quirk: the bonus is looked up BEFORE the clamp, so an
			// overflow counter that has just run past POWER_OVERFLOW_MAX
			// can index POWER_OVERFLOW_BONUS[] five entries past the final
			// valid index 42, at index 47. The value it reads is the one the
			// player is paid; the clamp on the next line only fixes the counter.
			power_overflow += 5;
			score = POWER_OVERFLOW_BONUS[power_overflow];
			if(static_cast<unsigned int>(power_overflow) > POWER_OVERFLOW_MAX) {
				power_overflow = POWER_OVERFLOW_MAX;
			}
			// And only the frame that lands EXACTLY on the cap pays the
			// 2560 bonus. Once clamped, every later big-power item falls
			// through with whatever the out-of-bounds lookup gave it.
			if(power_overflow == POWER_OVERFLOW_MAX) {
				score = 2560;
				yellow = 1;
			}
		}
		break;

	case IT_BOMB:
		resident->rem_bombs++;
		score = 100;
		hud_bombs_put();
		break;

	case IT_1UP:
		playperf_raise(3);
		resident->rem_lives++;
		hud_lives_put();
		snd_se_play(7);
		overlay_popup_id_new = ITEM_POPUP_ID_EXTEND;
		overlay2 = overlay_popup_update_and_render;
		score = 100;
		break;

	case IT_FULLPOWER:
		if(bullet_clear_time < 20) {
			bullet_clear_time = 20;
		}
		overlay_popup_id_new = ITEM_POPUP_ID_FULL_POWERUP;
		overlay2 = overlay_popup_update_and_render;
		power = POWER_MAX;
		player_shot_level_update();
		score = 100;
		break;
	}

	// Negated on purpose in both tests below: the original branches AWAY on
	// the true case and falls through on the false one, so the arm written
	// first here is the one that lands first in the code.
	if(!pointnum_times_2) {
		score_delta += score;
	} else {
		score_delta += (score * 2);
	}
	if(!yellow) {
		pointnums_add_white(item->pos.cur.x.v, item->pos.cur.y.v, score);
	} else {
		pointnums_add_yellow(item->pos.cur.x.v, item->pos.cur.y.v, score);
	}

	// The rank accumulator, drained in one step once it fills. Unlike
	// item_left_playfield()'s, the threshold and the drain ARE the same
	// number, so this one does return to a remainder below 32 rather than
	// carrying 16 forward. Both constants are literal in the dump, and the
	// drain is an addition of the negated constant rather than a
	// subtraction.
	if(item_playperf_raise >= 32) {
		item_playperf_raise -= 32;
		playperf_raise(1);
	}
	items_collected++;
}
