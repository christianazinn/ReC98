#ifndef TH02_MAIN_PLAYER_SHOT_HPP
#define TH02_MAIN_PLAYER_SHOT_HPP

#include <stddef.h>
#include "th01/math/subpixel.hpp"
#include "th02/main/entity.hpp"

// The player's shots. The first 16 bytes of this structure are byte-for-byte
// identical to spark_t's, down to the page-indexed position pair.
// This record's original shared declaration was under default -a1. Preserve
// that footprint independently of a caller's local packing pragma.
#pragma pack(push, 1)
struct shot_t {
	entity_flag_t flag;

	// 0 while the shot flies. shots_hittest() sets it to 1 or 2 on impact,
	// and shots_update_and_render() then advances it every 4th frame until
	// the shot is removed. Option shots start at 1 and use it as their own,
	// differently paced animation counter.
	uint8_t decay_cel;

	// Stores the current and previous position, indexed with the currently
	// rendered VRAM page.
	SPPoint pos_on_page[PAGE_COUNT];

	SPPoint velocity;

	uint8_t patnum; // ACTUAL TYPE: main_patnum_t
	bool from_option;
};
#pragma pack(pop)

// [shot_t] was already public, but the checkpoint codec depends on its exact
// native source representation. Keep this assertion at the include boundary
// so a future local packing pragma cannot make replay.cpp see another ABI.
typedef char shot_t_layout_check[(
	(sizeof(shot_t) == 0x10) &&
	(offsetof(shot_t, flag) == 0x00) &&
	(offsetof(shot_t, decay_cel) == 0x01) &&
	(offsetof(shot_t, pos_on_page) == 0x02) &&
	(offsetof(shot_t, velocity) == 0x0A) &&
	(offsetof(shot_t, patnum) == 0x0E) &&
	(offsetof(shot_t, from_option) == 0x0F)
) ? 1 : -1];

// Instruction-derived, not an extent ceiling: seven independent loops over
// [shots] bound at 26h, and 38 * sizeof(shot_t) is exactly the _BSS extent.
static const int SHOT_COUNT = 38;

extern "C" shot_t near shots[SHOT_COUNT];

// The slot that shot_a()/shot_b()/shot_c() are currently scanning. Both
// spawners write into this slot rather than taking it as a parameter.
extern "C" int shot_slot_i;

// The Y coordinate every shot of the current volley spawns at, set once per
// volley by shot_a()/shot_b()/shot_c().
extern "C" subpixel_t shot_spawn_top;

// Persistent stream and pool controls. Their per-frame values affect the
// next volley, so the checkpoint codec names them explicitly rather than
// treating the contiguous BSS span as a serializable object.
extern "C" uint8_t shot_stream_a_phase;
extern "C" uint8_t shot_stream_b_phase;
extern "C" uint8_t shot_stream_a_cooldown_time;
extern "C" uint8_t shot_stream_b_cooldown_time;
extern "C" uint8_t shot_patnum;
extern "C" uint8_t shot_option_patnum;
extern "C" uint8_t shot_patnum_powered;
extern "C" uint8_t shot_option_patnum_powered;
extern "C" int8_t shot_a_spread_angle_delta;
extern "C" uint8_t option_shots_alive;
extern "C" int boss_pos_x;
extern "C" int boss_pos_y;
extern "C" int boss_pos_x_unused;
extern "C" uint8_t shot_c_cycle;
extern "C" uint8_t shot_anim_frame[SHOT_COUNT];
extern "C" int8_t shot_option_decay_interval;

// ZUN bloat: The three functions below write [flag]/[decay_cel] and
// [patnum]/[from_option] as two single 16-bit stores rather than four byte
// ones. Turbo C++ 4.0J does not fuse adjacent byte stores — sparks_add()
// compiles the same pair of adjacent constant byte fields into
// `mov byte ptr [si], 1` + `mov byte ptr [si+1], 0` — so ZUN's source really
// does name a 16-bit lvalue at both addresses. shots_hittest() also *reads*
// [patnum] 16 bits wide. The wider accesses produce the same field values and
// therefore have no observable effect.
#define shot_flag_and_decay_cel(p) \
	(*reinterpret_cast<int16_t near *>(&(p)->flag))
#define shot_patnum_and_from_option(p) \
	(*reinterpret_cast<int16_t near *>(&(p)->patnum))

// Spawns one player shot into [shot_slot_i], [left_offset] pixels to the
// right of the player's left edge, with a 12-pixel velocity vector at
// [angle].
void pascal near shot_add(pixel_t left_offset, unsigned char angle);

// Spawns one option shot into [shot_slot_i], [left_offset] pixels to the right
// of the left option's left edge, with an explicit velocity vector.
void pascal near shot_option_add(
	pixel_t left_offset, subpixel_t velocity_x, subpixel_t velocity_y
);

// Fires one volley of shottype A's shot. Reached only through
// [playchar_shot_func], which the still-ASM per-shottype player reset installs
// from [resident->shottype]; shot_b() and shot_c() are its two siblings.
void near shot_a(void);

// Fires one volley of shottype B's shot: first the player's own shots, then —
// in the same slot scan — the option shots, once per button press.
void near shot_b(void);

// Fires one volley of shottype C's shot. Unlike its two siblings, the option
// half only fires on some calls — see [shot_c_cycle].
void near shot_c(void);

// Frees every shot slot at once. far, because its only callers are in the
// boss code's own segments.
void far shots_free_all(void);

// Marks the tiles behind every alive shot for redrawing, retires the ones that
// shots_hittest() flagged for removal, and copies the remaining ones' positions
// from the front page to the back one.
void near shots_invalidate(void);

void near shots_update_and_render(void);

// Tests every alive shot against the given hitbox, consumes the player shots
// that hit, and returns the total damage dealt — which it also adds to
// [score_delta]. Defined in th02/main/player/shot_hittest.cpp: ZUN's object put
// it in front of the vertical boss lasers and the dialog code, so it is
// compiled into th02/dialog.cpp's translation unit rather than into this
// subsystem's own. th02/main/enemy/update.cpp's enemy_hittest() is the first
// C++ caller; the rest are still ASM.
int pascal near shots_hittest(
	screen_x_t left, screen_y_t top, pixel_t w, pixel_t h
);

#endif /* TH02_MAIN_PLAYER_SHOT_HPP */
