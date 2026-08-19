/// TH04 enemy tile invalidation
/// ----------------------------
/// (#included from th04/midbossx.cpp, ahead of th04/main/midboss/mx.cpp. This
/// function was the whole of th04_main.asm's own contribution to
/// MIDBOSSX_TEXT, as `include th04/main/enemy/inv.asm`, and TLINK appends that
/// object after the root -- so replacing the module with this object's first
/// function leaves every byte where it was. kb/codegen 0098 + 0105 + 0112.)
///
/// TH05's copy is still ASM: th05_main.asm includes the same module, in a
/// segment where it is not the root's last emitting item. Nothing here is
/// game-guarded, because this file is compiled into TH04's MAIN.EXE alone.

#include "platform.h"
#include "pc98.h"
#include "th04/main/enemy/enemy.hpp"
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
// The module was the whole of th04_main.asm's contribution to MIDBOSSX_TEXT,
// so there is nothing left in the root to emit it -- and it now sits INSIDE
// one object, between this function and midboss1_render(), where the segment's
// `word` alignment cannot produce it either: TLINK aligns contributions, not
// the functions within one. Same spelling as the inter-object pad at the end
// of th04/main/midboss/mx.cpp. (kb/codegen/0111)
#pragma codestring "\x90"

#pragma option -k.
