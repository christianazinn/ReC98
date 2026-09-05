/// The item set dropped when losing a life
/// ---------------------------------------
/// One name, ONE body, shared by both games. The only `#if (GAME == 5)` site
/// is the last-life test, and it is the same one the assembly module this
/// replaces already carried as an `if GAME eq 5` / `else` pair: TH05 keeps the
/// remaining life count in its own `lives` byte, TH04 reads it back out of the
/// resident structure.
///
/// The assembly module this body replaces was the tail `include` of TWO root
/// contributions at once -- th04_main.asm's IT_UPDT_TEXT and
/// th05_main.asm's main_033_TEXT -- so it is one parcel and not two
/// (state/re/INCLUDE_TAIL_CLASSES.md). Its path is recorded in that file's
/// tooling rather than spelled here: a comment naming a path this same commit
/// deletes is a dangling reference by construction, and it is what
/// naming_precheck's M2 exists to catch. In both binaries the module was the
/// LAST thing the dump contributed to its segment, so #including this file at
/// the FRONT of the C++ object that already follows there puts every byte back
/// at its original address: no carve (kb/codegen/0080), no new segment name,
/// no group-list edit and no Tupfile.lua line (kb/codegen/0112 + 0114).
///
/// The two hosts are th04/it_updt.cpp and th05/main033.cpp, and they must
/// #include this file BEFORE their existing one. kb/codegen/0119 was checked
/// rather than assumed: the body is 0xE0 = 224 bytes in TH04 and 0xDC = 220 in
/// TH05, both EVEN, so no `-a2` table in either host can change parity behind
/// it whatever those objects emit.
///
/// Neither root contribution is emptied by this lift -- IT_UPDT_TEXT keeps
/// 0x640 and main_033_TEXT keeps 0xC2 -- so the odd-group-offset rule that
/// MATCH-TH05-MAIN-MIDBOSS2-OBJ measured does not apply here. It applies to
/// whichever parcel takes the LAST proc out of either of them.

#include "th04/main/item/item.hpp"
#include "th04/main/player/player.hpp"
#include "th03/math/randring.hpp"
#if (GAME == 4)
	#include "th04/resident.hpp"
#else
	// Published by th05_main.asm's BSS block as a plain `db`, and declared by
	// no header: the HUD's own life counter lives in th04/main/hud/hud.hpp,
	// but this byte is not it. Spelled here rather than added to a header
	// because this is the only C++ that has ever needed it.
	extern unsigned char lives;
#endif

// `extern "C"`, and `pascal` with it, because the module this replaces
// published this name undecorated and upper-cased, which is Borland's
// decoration for exactly that pair (kb/codegen 0081 + 0086). `far` is the
// large model's default and is what the module's `proc far` / `retf` is;
// th04/main/item/item.hpp above already declares the pair this way, and
// th04/main/player/miss.cpp's local copy agrees with it.
extern "C" void pascal far items_miss_add(void)
{
	// [item] is declared ahead of [i] so that it wins SI and the spawn
	// counter takes DI, which is the allocation the original has
	// (kb/codegen/0146). The five below are all frame locals, at [bp-2]
	// through [bp-0Ah] in this order.
	item_t near *item;
	int i;
	int total_item_i;
	int field;
	int bigpower_index;
	int type;
	int unused_index;

	// ZUN bloat: [unused_index] is drawn, re-drawn until it differs from
	// [bigpower_index], and then never read. Both draws come out of the
	// second ring, so the loop still consumes random numbers and removing it
	// would change every draw after it.
	bigpower_index = randring2_next16_mod(ITEM_MISS_COUNT);
	do {
		unused_index = randring2_next16_mod(ITEM_MISS_COUNT);
	} while(unused_index == bigpower_index);

	// Which third of the playfield the player died in. The velocity table has
	// one row per third, so an item set dropped at the left edge fans right
	// and one dropped at the right edge fans left.
	if(player_pos.cur.x < to_sp((PLAYFIELD_W / 3) * 1)) {
		field = MISS_FIELD_LEFT;
	} else if(player_pos.cur.x <= to_sp((PLAYFIELD_W / 3) * 2)) {
		field = MISS_FIELD_CENTER;
	} else {
		field = MISS_FIELD_RIGHT;
	}

	i = 0;
	item = items;
	for(total_item_i = 0; total_item_i < ITEM_COUNT; (total_item_i++, item++)) {
		if(item->flag != F_FREE) {
			continue;
		}
		item->flag = F_ALIVE;
		item->unknown = 0;
		item->pos.cur.x = player_pos.cur.x;
		item->pos.cur.y = player_pos.cur.y;

		// Yes, Y first and X second -- th04/main/item/item.hpp says so at the
		// declaration, and the two index expressions here are what it says it
		// about. The index is recomputed for the second one; that is the
		// compiler, not the source.
		item->pos.velocity.y = ITEM_MISS_VELOCITIES[field][0][i];
		item->pos.velocity.x = ITEM_MISS_VELOCITIES[field][1][i];

		item->pulled_to_player = false;

		// Exactly one of the set is a big power item, at the index drawn
		// above; every other one is a coin flip between power and point.
		if(bigpower_index != i) {
			type = randring2_next16_and(IT_POINT);
		} else {
			type = IT_BIGPOWER;
		}

		// On the last life, the whole set turns into full-power items.
		#if (GAME == 5)
			if(lives == 1) {
		#else
			if(resident->rem_lives == 1) {
		#endif
			type = IT_FULLPOWER;
		}

		item->type = type;
		item->patnum = ITEM_PATNUM[type];
		i++;
		items_spawned++;
		if(i >= ITEM_MISS_COUNT) {
			break;
		}
	}
}
