/// Spawning a single item
/// ----------------------
/// One name, ONE body, shared by both games, and the `kb/codegen/0115` sibling
/// compare is what decided that: the two ranges in the original images
/// (TH04 `0x1DA38 + 0x96`, TH05 `0x16D84 + 0xA5`) run instruction for
/// instruction together from the enemy-drop ring lookup to the `retn 6`, and
/// the 15-byte length difference is entirely accounted for by the two
/// `#if (GAME == 5)` sites below. Neither is a naming difference: TH05 adds
/// the bomb-pull clamp at the top, and its every-other-drop test is a bit test
/// where TH04's is a `% 2`. A length difference is NOT by itself evidence of
/// two functions, and same-length is not evidence of one -- the byte compare
/// is (`kb/codegen/0109`).
///
/// In both binaries this was the carve-free `proc` tail of the dump's own
/// contribution, directly above a C++ object that already follows there, so
/// the lift goes at the FRONT of that position -- ahead of items_miss_add(),
/// which took the same route one parcel earlier -- and every byte keeps its
/// original address: no carve (`kb/codegen/0080`) and no new segment name.
///
/// THE TWO GAMES REACH THAT POSITION BY DIFFERENT ROUTES, and the difference
/// is `kb/codegen/0119`, measured here in both directions rather than
/// predicted. TH04's body is 0x96 = 150 bytes, EVEN, so `kb/codegen/0112`'s
/// wrapper `#include` into the existing th04/it_updt.cpp is safe and costs no
/// build-system line. TH05's is 0xA5 = 165, ODD, and th05/main033.cpp emits
/// the `-a2`-aligned jump table item_collected() compiles to -- so folding it
/// in there moved that table to an odd object-local offset, deleted its pad,
/// and produced a MAIN_033_TEXT of 0x597 against the original's 0x598 with
/// this body byte-perfect and the missing byte 0x4D6 further on. That was one
/// red cycle, `14 ok, 1 fail`. TH05 therefore gets its own object,
/// th05/itmadd.cpp, listed before th05/main033.cpp in `Tupfile.lua`.
/// The route is a property of the length and the host, not of the function.
///
/// Neither root contribution is emptied by this lift. TH04's `IT_UPDT_TEXT`
/// keeps 0x5AA and TH05's `main_033_TEXT` keeps 0x1D -- in TH05's case a single
/// 29-byte `proc far`, which is now that segment's whole dump contribution and
/// the next tail in the chain.

#include "th04/main/item/item.hpp"

// Declared here rather than by #including th04/main/item/splash.hpp, which is
// the header that owns it. That file is unguarded and DEFINES data
// (ITEM_SPLASH_DOTS), and th04/main/item/update.cpp already reaches it from
// further down this same object, so pulling it in here would both redefine
// every one of its declarations and hoist a definition above the code that
// used to precede it (kb/codegen 0110 + 0129). One declaration is the
// byte-inert half of that trade; a guard would still move the definition.
void pascal near item_splashes_add(Subpixel center_x, Subpixel center_y);

/// The ring of item types an enemy drop cycles through, and the position in it.
/// The ring position is seeded randomly at the start of a stage, and advances
/// once per REQUESTED drop rather than once per spawned one -- which is why
/// only every other IT_ENEMY_DROP_NEXT actually spawns anything, and why the
/// index is half the position. [verified-by-oracle]
/// Published by each game's own `enemy_drops[data].asm`; the two tables have
/// the same 64 entries in a different order.

void pascal near items_add(subpixel_t x, subpixel_t y, item_type_t type)
{
	// [item] is declared ahead of [i] so that it wins SI and the loop counter
	// takes DI, which is the allocation the original has (kb/codegen/0146),
	// and the same one items_miss_add() needs one file over.
	item_t near *item;
	int i;

	#if (GAME == 5)
		// A bomb turns every drop worth less than a big power item -- and any
		// undecided enemy drop -- into a plain power item.
		if(items_pull_to_player && (
			(static_cast<unsigned char>(type) < IT_BIGPOWER) ||
			(type == IT_ENEMY_DROP_NEXT)
		)) {
			type = IT_POWER;
		}
	#endif

	if(type == IT_ENEMY_DROP_NEXT) {
		enemy_drop_ring_p++;

		// ZUN bloat: Half of all enemy drops are silently thrown away. The
		// caller has no way of knowing, and the ring still advances.
		#if (GAME == 5)
			if(enemy_drop_ring_p & 1) {
				return;
			}
		#else
			if((enemy_drop_ring_p % 2) != 0) {
				return;
			}
		#endif

		type = ENEMY_DROPS[(enemy_drop_ring_p / 2) % ENEMY_DROP_RING_SIZE];
	}

	item = items;
	for(i = 0; i < ITEM_COUNT; (i++, item++)) {
		if(item->flag != F_FREE) {
			continue;
		}
		item->flag = F_ALIVE;
		item->unknown = 0;
		item->pos.cur.x.v = x;
		item->pos.cur.y.v = y;
		item->pos.velocity.x.v = 0;
		item->pos.velocity.y.v = to_sp(-3.0f);
		item->type = type;
		// The cast is load-bearing and it is a MEASUREMENT, not documentation:
		// `-b-` sizes item_type_t at one byte, but SIGNED, because IT_NONE and
		// IT_ENEMY_DROP_NEXT are negative. Indexing with the enum itself emits
		// `cbw` where the original has `mov ah, 0`, and the same difference
		// decides the `jb` in the TH05 clamp above (kb/codegen 0163 + 0028).
		item->patnum = ITEM_PATNUM[static_cast<unsigned char>(type)];
		// The original pushes the two PARAMETERS here, not the copies just
		// stored into [item], so this cannot be spelled with
		// `item->pos.cur.x`. Subpixel is layout-compatible with subpixel_t
		// and deliberately has no converting constructor ("the class needs to
		// be trivially copyable"), so the cast is what a plain argument would
		// have been; th03/main/bullet/bullet.cpp reaches for the same one.
		item_splashes_add(
			*reinterpret_cast<Subpixel near *>(&x),
			*reinterpret_cast<Subpixel near *>(&y)
		);
		item->pulled_to_player = false;
		items_spawned++;
		break;
	}
}
