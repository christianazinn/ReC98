/// Resetting the item subsystem at the start of a stage
/// ----------------------------------------------------
/// One name, ONE body, shared by both games. The `kb/codegen/0115` compare is
/// as short as it gets: the two 0x1D-byte ranges (TH04 `0x1DA1B`, TH05
/// `0x16D67`) are byte-identical except for the address in the final `mov`,
/// and the two games' item.hpp already gives that word different names.
///
/// NOT `items_init_and_reset()`, which is what TH02's counterpart is called
/// upstream and what the first draft of this file said. That function clears
/// the item array and resets the collection counters; this one does neither.
/// What it shares with TH02's is the ring seeding, and the name here is
/// therefore the narrower one, beside the item_splashes_init() it calls and
/// the sparks_init() / pointnums_init() that follow it in stage_init().
/// [inferred from the body, which is short enough to be the whole evidence]
///
/// This was the last `proc` of TH05's `main_033_TEXT` root contribution, so
/// the lift EMPTIES that block -- the case parcel MATCH-TH05-MAIN-MIDBOSS2-OBJ
/// measured, and the reason the two games host it differently:
///
///   * TH05: the segment starts at the ODD group offset `0x1AE7`, so whatever
///     object lands at the segment start puts its own `-a2` data on odd
///     SEGMENT offsets. th05/itmadd.cpp emits none -- items_add() compiles no
///     `switch` -- so it can take this body at its front for free, and
///     th05/main033.cpp behind it does not move.
///   * TH04: `IT_UPDT_TEXT`'s root keeps 0x58D bytes, so nothing is emptied,
///     but th04/it_updt.cpp DOES emit `-a2` jump tables and 0x1D is ODD.
///     Folding this in there would flip their object-local parity, which is
///     exactly what `kb/codegen/0119` cost the items_add() parcel one red
///     cycle. It gets its own object, th04/itminit.cpp, listed immediately
///     before th04/it_updt.cpp so that object's start does not move.
///
/// The same body, the same length, two hosts, two routes -- for the second
/// parcel running. The route is a property of the host and the group offset,
/// never of the function.

#include "libs/master.lib/master.hpp"
#include "th04/main/item/item.hpp"

// Declared here rather than by #including th04/main/item/splash.hpp for the
// same reason th04/main/item/add.cpp declares its neighbour locally: that
// header is unguarded and defines data, and this body shares a translation
// unit with add.cpp in TH05. One line against a header-ordering question.
void near item_splashes_init(void);

// `extern "C"` and `far`, both measured rather than chosen: each dump
// published this proc undecorated and lower-case beside a `proc far`, which is
// Borland's decoration for exactly that pair, and th04/main/stage/init.cpp --
// the only caller in either game -- already declared it that way against the
// placeholder name.
extern "C" void far items_init(void)
{
	// ZUN bloat: The starting point is randomised, but the ring is walked in
	// order from there, so the sequence of enemy drops in a stage is fixed
	// once this returns. Only 16 of the 64 entries can ever start it.
	enemy_drop_ring_p = (irand() & 0x0F);

	item_splashes_init();
	items_pull_to_player = false;
	#if (GAME == 5)
		items_init_unused = 0;
	#else
		dream_score = 0;
	#endif
}
