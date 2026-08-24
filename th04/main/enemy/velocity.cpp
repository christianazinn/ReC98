/// Enemy velocity from the enemy's own angle and speed
/// ---------------------------------------------------
/// TH04 only. TH05's enemy_velocity_set() is one of the six hand-written
/// assembly functions in its main_031_TEXT: it has no stack frame at all, and
/// [enemy_cur] arrives implicitly in SI, put there by each caller. Upstream's
/// own compiler-verified rule (Research/Borland C++ decompilation.md, "Limits
/// of decompilability") excludes exactly that shape — Turbo C++ *always*
/// brackets any use of SI with `push si`/`pop si`. It stays in th05_main.asm.
/// (state/notes/enemy_velocity_set.md, kb/conventions/handwritten-asm-tells.md)
///
/// This file has no #includes of its own for [enemy_t] and [enemy_cur]: it is
/// compiled as the second half of th04/enm_pos.cpp (kb/codegen/0112), after
/// th04/main/enemy/pos.cpp has already pulled in th04/main/enemy/enemy.hpp,
/// which carries no include guard. That include order is also the original
/// address order, which is the whole reason the two share one object.

#include "th04/math/vector.hpp"
#include "th04/main/player/player.hpp"
#include "libs/master.lib/master.hpp"

// Sets [enemy_cur]'s velocity to a vector with the enemy's own [angle] and
// [speed].
//
// Reads one word at [angle], and therefore also loads [angle_delta] into the
// high byte of the pushed argument. That is ordinary Turbo C++ behavior for a
// byte argument ("Borland C++ just pushes the entire word"), not a hand-written
// quirk, and vector2_near() only consumes BL — so the second byte is never
// looked at. Do NOT "fix" this by casting; a cast would request a
// zero-extension the original does not have.
extern "C" void pascal near enemy_velocity_set(void)
{
	register enemy_t near *p = enemy_cur;
	vector2_near(p->pos.velocity, p->angle, p->speed);
}

extern "C" void pascal near enemy_velocity_set_aimed(void)
{
	register enemy_t near *p = enemy_cur;

	// ZUN bloat: neither call touches ES, so bracketing them achieves nothing.
	//
	// That had to be checked rather than assumed, because ES is emphatically
	// NOT dead here. The .STD script VM that calls this holds [std_seg] in ES
	// across its whole dispatch loop, loads it exactly once before the loop
	// head, and reads es:[di] on both sides of this call — and it brackets its
	// own push es/pop es around only the two callees that really do clobber ES.
	// So the caller depends on this function preserving ES; what makes the
	// bracket redundant is the callees, not the register being free.
	// libs/master.lib/iatan2.asm and th04/math/vector2n.asm mention ES nowhere.
	//
	// [inferred] The same bracket surrounds the same computation in TH03 and
	// TH05, but never with the same scope: TH03's
	// enemy_velocity_set_from_angle_and_speed() (th03/main/enemy/enemy.cpp:423,
	// upstream-labelled "ZUN bloat: Yes, no point to this at all…") wraps only
	// its vector2() call, TH05's enemy_velocity_set_aimed() wraps only the
	// aiming call and leaves enemy_velocity_set() outside, and this one wraps
	// both. It is a
	// habit ZUN carried between games, not a body that was copied across.
	asm { push es; }
	p->angle = (iatan2(
		(player_pos.cur.y - p->pos.cur.y), (player_pos.cur.x - p->pos.cur.x)
	) + p->angle);
	vector2_near(p->pos.velocity, p->angle, p->speed);
	asm { pop es; }
}
