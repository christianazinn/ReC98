#ifndef TH05_MAIN_PLAYER_BOMBANIM_HPP
#define TH05_MAIN_PLAYER_BOMBANIM_HPP

/// The four playchars' bomb animation state
/// ----------------------------------------
/// One buffer, reinterpreted per playchar — TH05 only ever runs one bomb at a
/// time, and [playchar_bomb_func] decides which of the five views below is the
/// live one.
///
/// Moved here out of th05/main/player/bombanim.cpp, unchanged, when
/// th05/main/player/bombchar.cpp's …_update_and_render() functions needed
/// three of these five structures. The two files are in different translation
/// units — bombanim.cpp is #included from th05/main010.cpp and lands in
/// mai_TEXT, bombchar.cpp from th05/bombchar.cpp and lands in BOMBCHAR_TEXT —
/// so this is a genuinely shared declaration rather than a hoist for its own
/// sake.
///
/// Requires pc98.h (point_t, screen_point_t, pixel_t) and
/// th01/math/subpixel.hpp (Subpixel, SPPoint) at the point of inclusion.

// Structures
// ----------
static const int REIMU_STAR_TRAILS = 6;
static const int REIMU_STAR_NODE_COUNT = 8;
static const int MARISA_LASER_COUNT = 8;
static const int MIMA_CIRCLE_COUNT = 8;

static const pixel_t REIMU_STAR_W = 64;
static const pixel_t REIMU_STAR_H = 64;

// Generic type
struct bomb_anim_t {
	point_t pos;
	int16_t val1;
	int16_t val2;
	unsigned char angle;
	int8_t padding;
};

struct reimu_star_t {
	struct {
		Subpixel screen_x;
		Subpixel screen_y;
	} topleft;
	SPPoint velocity;
	unsigned char angle;
	int8_t padding;
};

struct marisa_laser_t {
	SPPoint center;
	int16_t unused;
	pixel_t radius;
	int16_t padding;
};

struct mima_circle_t {
	screen_point_t center;
	int16_t unused;
	pixel_t distance;
	unsigned char angle;
	int8_t padding;
};

struct yuuka_heart_t {
	screen_point_t topleft;
	int16_t unused;
	pixel_t distance;
	unsigned char angle;
	int8_t padding;
};

extern union {
	reimu_star_t reimu[REIMU_STAR_TRAILS][REIMU_STAR_NODE_COUNT];
	marisa_laser_t marisa[MARISA_LASER_COUNT];
	mima_circle_t mima[MIMA_CIRCLE_COUNT];
	yuuka_heart_t yuuka;
} bomb_anim;
// ----------

#endif /* TH05_MAIN_PLAYER_BOMBANIM_HPP */
