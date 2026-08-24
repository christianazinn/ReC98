/// What collecting an item does
/// ----------------------------
/// TH05 only. TH04's twin is th04/main/item/collect.cpp, and it is a
/// *different* body: same seven types, but this one drives the dream meter
/// instead of TH04's [dream_score], grants extends off the point-item count
/// rather than off the score, and doubles nothing for [pointnum_times_2].
/// Assembly in one game and C++ in the other means two lifts, not one shared
/// body (kb/codegen/0115) -- which is what th04/main/item/collect.cpp said
/// would happen, and this is that parcel.
///
/// (#included from th04/main/item/update.cpp, at the very FRONT of the object,
/// ahead of item_left_playfield() and items_update() -- the address order all
/// three have in main_033_TEXT. This proc, the proc above it, their pad and
/// their jump table were the last things th05_main.asm contributed to that
/// segment, so th05/main033.cpp grows backwards into the hole and every byte
/// above it keeps its address (kb/codegen 0099 + 0112 + 0114). The exact shape
/// TH04's twin has, one game later.)
///
/// point_items_extend_update() is co-lifted rather than left behind, and the
/// reason is `-a2` parity rather than anything about the function: its body is
/// 0x4F bytes, and 0x4F + item_collected()'s 0x265 is EVEN, which is what puts
/// the jump table at an even offset inside this object and keeps the one-byte
/// pad the original has (kb/codegen 0154 + 0157). Lifting item_collected()
/// alone would have been one byte short. It is `static`, because
/// item_collected() is its only caller in either dump -- so this parcel adds no
/// new `public` to th05_main.asm at all.
///
/// A STANDALONE OBJECT WAS TRIED FIRST AND FAILED BY ONE BYTE, and the reason
/// is NOT that nothing followed the table. That first attempt simply had no
/// `-a2`, because nothing in this object's include closure turns it on.
/// [measured, two-sided, on exactly that standalone translation unit with
/// nothing whatsoever emitted after the table] its segment length reads 0x2C2
/// with `-a2` off and 0x2C3 with `-a2` on. The pad is there with no follower.
/// So the choice of shape is free, and it is made on other grounds: this is
/// what TH04's twin does, and it costs no new object and no position-critical
/// Tupfile.lua line.

// Every one of these is published by a root-dump BSS or data block, or by a
// header this translation unit cannot reach, the same closure problem
// th04/main/item/collect.cpp documents at length.
// ---------------------------------------------------------------------
extern unsigned char bullet_clear_time;
extern unsigned char extends_gained;
extern unsigned long score_delta;

// TH05's life and bomb counters are its own globals rather than TH04's
// [resident->rem_*], and no header in the tree declares them -- the only
// `extern ... lives` anywhere is th02/resident.hpp, which is TH02-only.
// th05_main.asm publishes both. Same TU-local route th05/shot_inv.cpp took.
extern "C" unsigned char lives;
extern "C" unsigned char bombs;

// The per-stage point item counter saturates at this value rather than
// wrapping. th05/th05.inc spells it for the assembly side; there is no C++ home
// for it, and putting one in th04/main/item/item.hpp would put a TH05-only
// constant in a header four other binaries compile.
static const unsigned int POINT_ITEMS_MAX = 999;

// Far from this main_03 object; same-group callers use a nopcall island.
extern "C" void far player_shot_level_update(void);

// th04/main/hud/overlay.hpp declares these three; it is unguarded AND
// re-expands headers this closure already carries, so spelling them here is the
// th04/main/item/collect.cpp route rather than a style choice (kb/codegen/0129).
// Spelled with the storage type rather than the header's `popup_id_t`, because
// respelling that enum in a shared translation unit would put a second
// definition of every popup ID at file scope.
// The `#pragma codeseg` pair is kb/codegen/0082 and it is load-bearing:
// [overlay2] holds a NEAR pointer, and the offset stored into it has to be
// taken in the group that actually contains the callee. Declared plainly, this
// object's own `-zPmain_03` frames it on main_03 and TLINK rejects the record
// outright. The restore names main_033_TEXT and main_03 explicitly rather than
// using the bare form, so that it cannot land on this file's own basename.
#pragma codeseg HUD_OVRL_TEXT main_01
extern unsigned char overlay_popup_id_new;
extern nearfunc_t_near overlay2;
void pascal near overlay_popup_update_and_render(void);
#pragma codeseg main_033_TEXT main_03
static const unsigned char ITEM_POPUP_ID_FULL_POWERUP = 3; // POPUP_ID_FULL_POWERUP
static const unsigned char ITEM_POPUP_ID_EXTEND = 1;       // POPUP_ID_EXTEND

// [power_overflow]'s cap and the score table indexed by it. Respelled rather
// than reached through th02/main/item/shared.hpp, which is UNGUARDED, and
// UNSIGNED where that header says `int`: every one of this function's tests
// against the cap is `JB`/`JBE` in the original, so the comparison ZUN's source
// reached was unsigned. Left signed, Turbo C++ takes the operator literally
// (kb/codegen/0092) and emits `JL`/`JLE` over identical operands. Same trap,
// same fix, and the same wording as th04/main/item/collect.cpp, because it is
// the same two tests in the same two arms.
static const unsigned int POWER_OVERFLOW_MAX = 42;
extern "C" int16_t POWER_OVERFLOW_BONUS[POWER_OVERFLOW_MAX + 1];

// [total_max_valued_point_items_collected] under the <= 32-character alias
// th04/main/item/items[data].asm publishes at the same address, because TLINK
// truncates a C identifier to 32 characters and the real name is 38
// (kb/codegen/0060). Same spelling th04/main/item/collect.cpp and
// th04/main/execl.cpp already use; the truncation is a link failure, so the
// full name is not an option here.
extern unsigned int total_max_valued_point_items;
// ---------------------------------------------------------------------

/// The extend every hundredth point item grants
/// --------------------------------------------
/// [inferred] from the body: it is called once per point item collected, and
/// it hands out a life the first frame [extend_point_items_collected] reaches
/// the next multiple of 100. Named after TH02's score_extend_init() and TH04
/// MAIN's score_extend_update_and_render(), which are the same idea driven by
/// the score instead.
///
/// ZUN quirk, preserved: [extends_gained] is incremented even when the life
/// cannot be granted because the player is already at 99, so the threshold
/// still moves and that extend is simply lost.
static void near point_items_extend_update(void)
{
	if(((extends_gained * 100) + 100) <= extend_point_items_collected) {
		playperf_raise(4);
		extends_gained++;
		if(lives < 99) {
			lives++;
			if(bullet_clear_time < 20) {
				bullet_clear_time = 20;
			}
			hud_lives_put();
			overlay_popup_id_new = ITEM_POPUP_ID_EXTEND;
				overlay2 = overlay_popup_update_and_render;
			snd_se_play(7);
		}
	}
}
/// --------------------------------------------

// The jump table below needs `-a2` to take the pad the original has: the pad is
// a pure function of `-a2` and the parity of the table's object-relative offset
// (kb/codegen/0159, whose original follower-dependent model was refuted the same
// day this file landed). It is not turned back off afterwards, and that is a
// deliberate non-change rather than a requirement -- nothing that follows in
// this object defines a structure or emits data, so `-a2` staying in force is
// inert, and the build that is GREEN is the one without a restore. A restore
// would not cost the pad; it has simply not been measured here.
#pragma option -a2

void pascal near item_collected(item_t near *item)
{
	// [bp-2]. The point item's height above the line at which it stops being
	// worth more, already offset by the dream meter — so it is ≤ 0 for any
	// item worth the maximum, and the arms below branch on its sign rather
	// than comparing the raw Y coordinate the way TH04's twin does.
	int height_above_max;

	// [bp-3], with [bp-4] left as padding (kb/codegen/0010). Set by the arms
	// that award a *maximum*-value pickup, and read once at the bottom to pick
	// the yellow point number over the white one.
	unsigned char yellow;

	// SI, and DELIBERATELY UNINITIALIZED, exactly as in TH04. The `switch`
	// below has no default, so an item whose [type] is above IT_FULLPOWER
	// falls straight through to the tail and spends whatever SI happened to
	// hold. ZUN landmine, preserved: the original has no initializing store
	// either, and items_add() is the only writer of [type] in either game.
	//
	// `register` is load-bearing and [measured], not decoration. Turbo C++
	// hands out SI and DI to two candidates, and left to itself it gives SI
	// to [item] and DI to this -- the swap of what the original does. TH04's
	// twin emits the same assignment without the `register` qualifier, so
	// whatever tips the choice is not local-variable order: all six orderings
	// were compiled and every one of them put [item] in SI.
	register unsigned int score;

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

				// Only once the overflow counter has saturated, and only while
				// the player is not bombing: a power item collected at full
				// power is worth one point of dream meter, which is the only
				// place in the game that raises it without a point item.
				if(!items_pull_to_player && (dream < BAR_MAX)) {
					dream++;
				}
			}
			score = POWER_OVERFLOW_BONUS[power_overflow];
			hud_dream_put();
		}
		break;

	case IT_POINT:
		// Both the height that decides the value and the value itself depend
		// on the dream meter: every point of it lifts the maximum-value line
		// by 24 subpixels, and at BAR_MAX the item is worth a flat
		// [item_point_score_at_full_dream] wherever it was caught.
		if(dream < BAR_MAX) {
			height_above_max = (
				item->pos.cur.y.v - (dream * 24) - to_sp(56)
			);
			score = 5120;
		} else {
			height_above_max = 0;
			score = item_point_score_at_full_dream;
		}
		if(height_above_max <= 0) {
			item_playperf_raise++;

			// Not while bombing: an item pulled to the player is worth its
			// maximum by definition, so raising the meter for it as well would
			// pay the pull twice.
			if(!items_pull_to_player) {
				// Higher up the playfield is worth more meter, in steps of a
				// quarter of the screen height.
				dream += (6 - (item->pos.cur.y.v / to_sp(64)));
				if(dream > BAR_MAX) {
					dream = BAR_MAX;
				}
				hud_dream_put();
			}
			total_max_valued_point_items++;
			yellow = 1;
		} else {
			score = (2800 - (height_above_max / 2));
		}
		item_playperf_raise++;
		extend_point_items_collected++;
		total_point_items_collected++;
		if(stage_point_items_collected < POINT_ITEMS_MAX) {
			stage_point_items_collected++;
		}
		point_items_extend_update();
		hud_point_items_put();
		break;

	case IT_DREAM:
		// Worth almost nothing until the meter is already full, and worth
		// 12800 once it is — the only item in the game whose value is decided
		// entirely by a state the player was already in.
		if(dream >= BAR_MAX) {
			score = 12800;
			yellow = 1;
		} else {
			score = 1;
			dream = BAR_MAX;
		}
		hud_dream_put();
		item_playperf_raise += 2;
		if(items_pull_to_player) {
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
			// ZUN quirk, and TH04 has the identical one: the bonus is looked
			// up BEFORE the clamp, so an overflow counter that has just run
			// past POWER_OVERFLOW_MAX indexes POWER_OVERFLOW_BONUS[] at 43..47,
			// up to five entries beyond its valid 0..42 range. The value it reads
			// is the one the player is paid; the clamp on the next line only fixes
			// the counter.
			power_overflow += 5;
			score = POWER_OVERFLOW_BONUS[power_overflow];
			if(static_cast<unsigned int>(power_overflow) > POWER_OVERFLOW_MAX) {
				power_overflow = POWER_OVERFLOW_MAX;
			}
			// And only the frame that lands EXACTLY on the cap pays the 2560
			// bonus. Once clamped, every later big-power item falls through
			// with whatever the out-of-bounds lookup gave it.
			if(power_overflow == POWER_OVERFLOW_MAX) {
				score = 2560;
				yellow = 1;
			}
		}
		break;

	case IT_BOMB:
		bombs++;
		score = 100;
		hud_bombs_put();
		break;

	case IT_1UP:
		playperf_raise(3);
		lives++;
		hud_lives_put();
		snd_se_play(7);
		overlay_popup_id_new = ITEM_POPUP_ID_EXTEND;
				overlay2 = overlay_popup_update_and_render;

		// The original branches into IT_FULLPOWER's `mov si, 100` instead of
		// storing 100 of its own, and it does NOT need a `goto` here to say
		// so: `-O` cross-jumps this arm into the next one by itself.
		// [measured] Writing the `goto` explicitly is actively WRONG -- it
		// hands the optimizer a second merge it then also takes, and
		// IT_BIGPOWER loses its own update call and `score = 1;` to IT_POWER's
		// copy. Two instructions short, and nothing about the `goto` arm
		// itself looks wrong in the listing.
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

	score_delta += score;

	// Negated on purpose, as in TH04: the original branches AWAY on the true
	// case and falls through on the false one, so the arm written first here is
	// the one that lands first in the code.
	if(!yellow) {
		pointnums_add_white(item->pos.cur.x.v, item->pos.cur.y.v, score);
	} else {
		pointnums_add_yellow(item->pos.cur.x.v, item->pos.cur.y.v, score);
	}

	// The rank accumulator, drained in one step once it fills.
	if(item_playperf_raise >= 32) {
		item_playperf_raise -= 32;
		playperf_raise(1);
	}
	items_collected++;
}
