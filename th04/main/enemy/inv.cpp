/// Enemy tile invalidation
/// -----------------------
/// One name, ONE body, now in BOTH games, and the only thing that differs
/// between the two dumps' copies is `sizeof(enemy_t)`, which each game's own
/// header supplies. The hand-written module both dumps used to `include` is
/// gone.
///
/// TH04: #included from th04/main/midboss/mx.cpp, ahead of the three midboss
/// renderers, which puts it first in th04/midbossx.cpp's object. This function
/// was the whole of th04_main.asm's own contribution to MIDBOSSX_TEXT.
///
/// TH05: #included from th04/main/end.cpp, ahead of end_game(), which puts it
/// first in th05/end_ext.cpp's object. It was the LAST emitting item of
/// th05_main.asm's contribution to END_EXT_TEXT.
///
/// In both games TLINK appends the C++ object after the root contribution, so
/// replacing the module with this object's first function leaves every byte
/// where it was: no carve, no new segment name, no group-list edit and no
/// Tupfile.lua line. kb/codegen 0098 + 0105 + 0112 + 0114.
///
/// **The TH05 half was previously recorded here as needing a carve, and that
/// was wrong.** Measured on the pre-lift map rather than by eye:
/// `enemies_invalidate` sits at `0AE1:360C` and END_EXT_TEXT's root
/// contribution ends at `0AE1:3638`, exactly `0x2C` later — the module IS the
/// root's last emitting item there, which is also what the harness's
/// include-tail recon reports for that segment. The lift went GREEN with no
/// carve and the whole segment came out byte-identical over its full length.

#include "platform.h"
#include "pc98.h"

// TH05's enemy_t has to be declared before th04/main/enemy/enemy.hpp's
// `extern enemies[]`, so in that game this header may only be reached through
// th05/main/enemy/enemy.hpp. Same conditional every other shared enemy TU
// carries (th04/main/enemy/add.cpp, pos.cpp, bullet_template.cpp).
#if (GAME == 5)
#include "th05/main/enemy/enemy.hpp"
#else
#include "th04/main/enemy/enemy.hpp"
#endif
#include "th04/main/enemy/size.hpp"
#include "th04/main/tile/tile.hpp"

// See th04/main/tile/tile.hpp for why this declaration has to be repeated in
// every translation unit that calls the function, and why the parameter list is
// a per-TU choice. This is the SPPoint form, like th04/main/midboss/inv.cpp:
// the original pushes [pos.prev] as one dword rather than two words.
extern "C" void pascal near tiles_invalidate_around(const SPPoint center);

// No parameters and no stack locals -- SI and DI are register variables, which
// are pushed either way and are not part of the frame -- and the original has
// no BP frame at all, so this one function needs -k- and the restore right
// after it (kb/codegen 0042 + 0149).
#pragma option -k-

void near enemies_invalidate(void)
{
	register enemy_t near *enemy;
	register int i;

	// One 32-bit immediate store rather than the two word stores every other
	// caller of tiles_invalidate_around() emits, which is what the original
	// did here. Same shape as the set_long() member SPPoint declares in
	// th01/math/subpixel.hpp, reached through a cast because point_t is
	// master.lib's plain struct with no such member. ENEMY_W and ENEMY_H are
	// both 32, so the halves are not distinguishable in the constant; they
	// are written in field order.
	reinterpret_cast<uint32_t &>(tile_invalidate_box) = (
		ENEMY_W | (static_cast<uint32_t>(ENEMY_H) << 16)
	);

	enemy = enemies;
	i = ENEMY_COUNT;
	do {
		if(
			(enemy->flag != EF_FREE) &&
			(enemy->flag != EF_ALIVE_FIRST_FRAME)
		) {
			tiles_invalidate_around(enemy->pos.prev);
		}
		enemy++;
	} while(--i);
}

// The `even` that closed th04/main/enemy/inv.asm, which padded the module to a
// word boundary with one `nop`. The body is 0x2B bytes, so the pad is real.
// Neither dump has anything left in the root to emit it -- and it now sits
// INSIDE one object, between this function and whatever the host compiles
// next, where the segment's `word` alignment cannot produce it either: TLINK
// aligns contributions, not the functions within one. Same spelling as the
// inter-object pad at the end of th04/main/midboss/mx.cpp. (kb/codegen/0111)
#pragma codestring "\x90"

#pragma option -k.
