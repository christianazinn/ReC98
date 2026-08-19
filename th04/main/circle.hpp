// Growing or shrinking circles
// ----------------------------
// Due to the lack of a tile invalidation function, these are only supported on
// top of backgrounds that are fully redrawn each frame (read: during bosses).

#include "pc98.h"
#include "th01/math/subpixel.hpp"
#include "th02/main/entity.hpp"

// Note the slight semantic differences to the item_splash_t structure.
struct circle_t {
	entity_flag_t flag;
	unsigned char age;
	screen_point_t center;
	pixel_t radius_cur;
	pixel_t radius_delta;
};

static const int CIRCLE_COUNT = ((GAME == 5) ? 8 : 16);

extern circle_t circles[CIRCLE_COUNT];

// Color used to render all currently active circles.
extern vc_t circles_color;

// [center_x] and [center_y] are *passed* as subpixels, but stored at pixel
// precision, as master.lib's grcg_circle() function doesn't support more than
// that anyway.
void pascal circles_add_growing(subpixel_t center_x, subpixel_t center_y);
void pascal circles_add_shrinking(subpixel_t center_x, subpixel_t center_y);

void near circles_update(void);
void near circles_render(void);
// ----------------------------
