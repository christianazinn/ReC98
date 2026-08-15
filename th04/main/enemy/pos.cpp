/// Enemy movement and off-playfield clipping
/// -----------------------------------------
/// TH04 only. TH05's enemy_pos_update() is one of the six hand-written
/// assembly functions in its main_031_TEXT: no stack frame, [enemy_cur]
/// arrives implicitly in SI, and the result comes back in the *carry flag*.
/// Turbo C++ emits none of that, so it stays in th05_main.asm.
/// (state/notes/enemy_pos_update.md, kb/conventions/handwritten-asm-tells.md)
///
/// TH03's ancestor of this function is enemy_move_and_clip()
/// (th03/main/enemy/enemy.cpp), down to the `goto clip` structure.
///
/// The `-zCENM_POS_TEXT -zPmain_03` that used to sit here now lives in the
/// th04/enm_pos.cpp wrapper, which compiles this file together with
/// th04/main/enemy/velocity.cpp: a segment pragma only takes effect before any
/// code is generated, so the second file in an object cannot repeat it
/// (kb/codegen/0112).

#include "th04/main/playfld.hpp"
#include "th04/main/enemy/enemy.hpp"
#include "th04/main/enemy/size.hpp"

// Declared here rather than through a header, exactly as th04/main/execl.cpp:34
// does and for the same reason. The definition is still ASM, in th04_main.asm's
// own _DATA contribution.
extern unsigned int enemies_gone;

// Advances the current enemy along its velocity, and clips it off the
// playfield along whichever axes it asked to be clipped on. Returns `true` if
// the enemy was clipped, in which case its [flag] has already been set to
// EF_KILLED and it must not be processed any further this frame.
extern "C" bool pascal near enemy_pos_update(void)
{
	// ZUN bloat: [enemy_cur] is read into a local, the motion update then goes
	// through the global anyway, and everything below runs off a second copy
	// of the same pointer. Turbo C++ reproduces the consequence exactly:
	// [enemy] is written once and read once, which does not earn a register,
	// so it stays at [bp-2]; [p] is dereferenced four times and gets SI.
	enemy_t near *enemy = enemy_cur;
	/* DX:AX = */ enemy_cur->pos.update_seg3();
	enemy_t near *p = enemy;

	if(p->clip_x) {
		if(!playfield_encloses_1d_inplace_fast(_AX, PLAYFIELD_W, ENEMY_W)) {
			goto clip;
		}
	}
	if(p->clip_y) {
		if(!playfield_encloses_1d_inplace_fast(_DX, PLAYFIELD_H, ENEMY_H)) {
			goto clip;
		}
	}
	return false;

clip:
	enemies_gone++;
	p->flag = EF_KILLED;
	return true;
}
