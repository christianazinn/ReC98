#ifndef TH04_MAIN_PLAYER_SHOT_HPP
#define TH04_MAIN_PLAYER_SHOT_HPP

// Guarded because TWO objects now reach this file twice, one in each game,
// and a second expansion rejects every `static const` and every struct it
// declares (kb/codegen/0129):
//
//   th05/shot_inv.cpp -- once from the sub_1214A() lift at its front, once
//   through th04/main/player/shots_inv.cpp further down;
//   th04/main_.cpp    -- once from shots_render.cpp, once from
//   shots_hittest.cpp behind it.
//
// Ordering the includes so only the first one pulls this in would work today
// and break the moment either body is shared with the other game -- which is
// exactly how MATCH-TH05-MAIN-TAILS-1 and MATCH-TH04-MAIN-CARVE-TAILS-1
// collided over th04/main/player/player.hpp. Ordering is not an invariant
// across hosts; a guard is. Byte-inert: this file only declares.
#if (GAME == 5)
#include "th05/sprites/main_pat.h"
#endif
#include "th04/main/player/player.hpp"
#include "th04/math/randring.hpp"
#include "th04/sprites/cels.h"
#include "th02/main/entity.hpp"

// Sets [velocity] to a vector with the given [angle] and a 12-pixel length.
//
// TH05 also insists on setting Shot::angle via a ridiculous out-of-bounds
// access, and therefore *must* be called with [velocity] pointing inside a
// Shot structure! (The struct was `shot_t` until upstream's 456768a4 renamed it;
// the dump's own struc keeps the old spelling, see HitShot below.)
void pascal near shot_velocity_set(
	SPPoint near* velocity, unsigned char angle
);

static const int HITSHOT_FRAMES_PER_CEL = ((GAME == 5) ? 3 : 4);
static const int HITSHOT_FRAMES = (HITSHOT_FRAMES_PER_CEL * HITSHOT_CELS);

#if (GAME == 4)
enum shot_flag_th04_t {
	SF_FREE = 0,
	SF_ALIVE = 1,
	SF_HIT = 2,
	SF_REMOVE = (SF_HIT + HITSHOT_FRAMES),

	_shot_flag_th04_t_FORCE_UINT8 = 0xFF
};
#endif

struct Shot {
#if (GAME == 5)
	entity_flag_t flag;
#else
	shot_flag_th04_t flag;
#endif
	char age;
	PlayfieldMotion pos;
	// The displayed sprite changes between this one and
	// [patnum_base + 1] every two frames.
#if GAME == 5
	char patnum_base;
	char type;
#else
	int patnum_base;
#endif
	char damage;
	char angle; // Unused in TH04

	void from_option_l(float offset = 0.0f) {
		this->pos.cur.x -= PLAYER_OPTION_DISTANCE + offset;
	}

	void from_option_r(float offset = 0.0f) {
		this->pos.cur.x += PLAYER_OPTION_DISTANCE + offset;
	}

#if (GAME == 5)
	void set_option_sprite() {
		this->patnum_base = PAT_SHOT_SUB;
	}

	void set_option_sprite_and_damage(char damage) {
		set_option_sprite();
		this->damage = damage;
	}
#endif

	void set_random_angle_forwards(
		unsigned char min = -0x48, unsigned char max = -0x38
	) {
		shot_velocity_set(
			(SPPoint near*)&this->pos.velocity,
			randring1_next8_ge_lt(min, max)
		);
	}
};

static const int SHOT_COUNT = ((GAME == 5) ? 64 : 68);

#if (GAME == 5)
// Decay animation played where a shot hit a stage enemy or boss. TH04 runs the
// same animation inside the Shot itself, via SF_HIT and SF_REMOVE.
// The ASM layout gives this field a game-specific prefix to keep it distinct
// from the `age` field of the dump's `shot_t` layout inside th05_main.asm.
struct HitShot {
	unsigned char age;

	// Always a 16×16 sprite.
	unsigned char patnum;

	PlayfieldMotion pos;
};

static const pixel_t HITSHOT_W = 16;
static const pixel_t HITSHOT_H = 16;
static const int HITSHOT_COUNT = 24;

extern HitShot near hitshots[HITSHOT_COUNT];
#endif

// Shots are always fired for multiples of this number of frames, even if
// INPUT_SHOT is held for a shorter amount of time. Two further rounds are
// fired at the ⅓ and ⅔ points of each cycle.
// (th04/main/player/player.inc)
static const uint8_t SHOT_CYCLE_FRAMES = 18;

// Set [shot_time] to this value to block shots from being fired this frame.
// (th04/main/player/player.inc, whose comment this is and which has carried
// the name since long before any C++ needed it. TH05's
// marisa_bomb_update_and_render() is the first decompiled writer.)
static const uint8_t SHOT_BLOCKED_FOR_THIS_FRAME = 0xFF;

extern unsigned char shot_time;
extern Shot near shots[SHOT_COUNT];

// Points to the next free entry in [shots].
extern Shot near *shot_ptr;
// Index of the last valid entry in [shots].
extern char shot_last_id;

// Current area on the playfield for which to check shot collisions.
extern SPPoint shot_hitbox_center;
extern SPPoint shot_hitbox_radius;

// If `true`, shot damage is divided by 4 during bombs. In TH05, `true` also
// slightly increases damage output for Reimu's and Yuuka's shots, and is also
// used against midbosses.
extern bool shots_hittest_against_boss;

// Only used for hit detection in TH04. TH05 also uses it in shots_render().
struct shot_alive_t {
	// For TH05, [x] == Subpixel::None() indicates that this shot has been
	// converted to a hitshot.
	SPPoint pos;

	Shot near *shot;
};
extern unsigned int shots_alive_count;
// `near`: shots_hittest() walks this array through a near pointer kept in a
// 16-bit stack slot, which the far spelling cannot express.
extern shot_alive_t near shots_alive[SHOT_COUNT];

// Searches and returns the next free shot slot, or a nullptr if there are no
// more free ones.
Shot near* near shots_add(void);

#if (GAME == 4)
// Moves every shot, frees the ones that left the playfield or finished their
// decay animation, and caches the live ones into [shots_alive]. Also tracks
// [shot_laser_bottomcenter] and counts [shot_laser_time] down. Same name as
// TH03's function in the same role (th03/main/player/shot.cpp); TH05's
// counterpart is the still-unnamed sub_1240B(), a bigger body that also
// branches per shottype.
void near shots_update(void);
#endif

// Processes collisions of all shots against the shot_hitbox, decays any
// colliding shots, and returns the total amount of damage dealt.
int shots_hittest(void);

#if (GAME == 5)
// Alice's barrier variant, which fires a revenge bullet from every hit.
extern "C" int far shots_hittest_revenge(void);
#endif

inline int shots_hittest(
	const PlayfieldPoint &center,
	const subpixel_t &radius_x,
	const subpixel_t &radius_y
) {
	shot_hitbox_radius.x.v = radius_x;
	shot_hitbox_radius.y.v = radius_y;
	if(GAME == 5) {
		shot_hitbox_center = center;
	} else {
		shot_hitbox_center.x.v = center.x.v;
		shot_hitbox_center.y.v = center.y.v;
	}
	return shots_hittest();
}

#if (GAME == 4)
// Clears the shot-firing state at the start of every stage. TH04-only: two of
// the three counters it resets belong to the option laser, and TH05 sets its
// one survivor inline in stage_init() instead.
void near shot_reset(void);
#endif

void near shots_invalidate(void);

// Also renders hitshots in TH05.
// `pascal`, and that is not a style choice: Borland decorates a pascal
// function's mangled name in UPPER CASE, so the all-caps `public` that both
// dumps carried for this symbol IS the calling convention (kb/codegen/0086).
// th04/main/stage/loop.cpp, the only caller in either game, has always
// re-declared it locally with `pascal`; this header disagreed with it until
// TH04's body became the first C++ definition either game's copy ever had.
void pascal near shots_render(void);

// Option laser
// ------------
// Unused in TH05, but still present in the code.

static const pixel_t SHOT_LASER_W = 8;
static const unsigned int SHOT_LASER_COOLDOWN_FRAMES = 32;

typedef enum {
	SHOT_LASER_CEL_0,
	SHOT_LASER_CEL_1,
	SHOT_LASER_CEL_2,
	SHOT_LASER_CEL_3,
	SHOT_LASER_CEL_4,
	SHOT_LASER_CELS,
} shot_laser_cel_t;

// Describes the longest width and segments of the laser in pixels. Has no
// effect on damage or hitbox width.
enum shot_laser_style_t {
	SLS_2 = 0,    	//    ||
	SLS_4 = 1,    	//   ||||
	SLS_6 = 2,    	//  ||||||
	SLS_1_4_1 = 3,	// | |||| |
	SLS_8 = 4,    	// ||||||||

	_shot_laser_style_t_FORCE_UINT8 = 0xFF
};

// A laser is active as long as this is > [SHOT_LASER_COOLDOWN_FRAMES].
extern unsigned int shot_laser_time;

extern shot_laser_style_t shot_laser_style;

// Timer for the 16×16 ring shots that are spawned on top of the laser. These
// retain the fixed trajectory they were spawned at, and don't follow the
// laser.
extern uint8_t shot_laser_ring_cycle;

// Equivalent to the position of the player.
extern PlayfieldMotion shot_laser_bottomcenter;

// Takes a Subpixel for [h].
#define shot_laser_put(left, top, h, cel) \
	_SI = h; \
	shot_laser_put_raw(left, top, cel);
void __fastcall near shot_laser_put_raw(
	screen_x_t left, vram_y_t top, shot_laser_cel_t cel
);
// ------------

#endif /* TH04_MAIN_PLAYER_SHOT_HPP */
