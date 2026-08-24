/// Extra Stage Boss - EX-Alice
/// ---------------------------
/// The `-zCBX_UPDATE_TEXT -zPmain_03` pragma this file used to carry now lives
/// in th05/boss_x.cpp, which compiles this file together with the Extra Stage
/// midboss's update function (kb/codegen/0112 trap 0).

#include "th05/main/boss/boss.hpp"

// Structures
// ----------

struct near firewave_t {
	bool alive;
	bool is_right;
	vram_y_t bottom;
	pixel_t amp;
};
// ----------

// State
// -----

#define FIREWAVE_COUNT 2

extern firewave_t firewaves[FIREWAVE_COUNT];
// -----

// Game logic
// ----------

void pascal near firewaves_add(pixel_t amp, bool is_right)
{
	firewave_t near *firewave = firewaves;
	for(int i = 0; i < FIREWAVE_COUNT; i++, firewave++) {
		if(!firewave->alive) {
			firewave->alive = true;
			firewave->is_right = is_right;
			firewave->bottom = PLAYFIELD_TOP;
			firewave->amp = amp;
			return;
		}
	}
}

void near firewaves_update(void)
{
	firewave_t near *firewave = firewaves;
	for(int i = 0; i < FIREWAVE_COUNT; i++, firewave++) {
		if(firewave->alive) {
			firewave->bottom += 4;
			if(firewave->bottom >= (PLAYFIELD_BOTTOM + 224)) {
				firewave->alive = false;
			}
		}
	}
}

// Defined in th05/main/boss/bx_updt.cpp, which is BX_TEXT's object rather than
// this one's. `extern "C"` because the dump's `public` for it was the plain,
// undecorated upper-case spelling, and `pascal` alone still appends the mangled
// argument suffix (kb/codegen/0027); the declaration this line replaced omitted
// it and could therefore never have resolved against anything.
extern "C" void pascal near exalice_phase_next(
	explosion_type_t explosion_type, int next_end_hp
);
// ----------
