#include "th04/main/player/player.hpp"

// Calculates the angle from ([x], [y]) to the current player position, and
// optionally adds [plus_angle] to the result.
// [measured] The coordinates are plain subpixels, not Subpixel objects. They
// had been declared Subpixel since this header was written and the type had
// never been graded: th05/main/boss/b6.cpp only ever reached the overload
// below, and th05/main/bullet/cheeto_u.cpp only ever passed two Subpixel
// lvalues, both of which a by-value Subpixel parameter accepts. th05_main.asm
// COMPUTES an argument at four call sites -- three in the x position and one
// in the y -- and a by-value class parameter cannot take a computed value
// without a stack temporary, which none of the four frames has room for.
// th05/main/boss/b5.cpp is the first C++ caller to hit that. The dump's own
// procdesc has always declared x:word, y:word, plus_angle:byte.
unsigned char pascal near player_angle_from(
	subpixel_t x, subpixel_t y, unsigned char plus_angle = 0
);

inline unsigned char near player_angle_from(
	const PlayfieldPoint &point, unsigned char plus_angle = 0
) {
	return player_angle_from(point.x, point.y, plus_angle);
}
