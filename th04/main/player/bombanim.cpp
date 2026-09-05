/// Bomb star animation
/// -------------------
/// The field of 16×16 stars that drifts across the playfield for the length of
/// a bomb, respawned at an edge whenever one leaves. Both playchars use the
/// same star field and the same function; only the spawn edge, the angle and
/// the speed distribution differ.
///
/// (#included from th04/main/player/shots_inv.cpp, ahead of its own two
/// functions, which is these three functions' original address order at the
/// head of SHOT_INV_TEXT's C++ object. It has no #includes of its own beyond
/// the four that file adds for it, because most of the headers involved --
/// th04/math/randring.hpp, th04/main/player/player.hpp, th04/main/scroll.hpp
/// and 12 more of the 30 in that closure -- have no include guard, so pulling
/// one in twice is a hard error. kb/codegen/0129.)
///
/// Because this file shares a translation unit with shots_inv.cpp, its
/// file-scope names are NOT file-local.
///
/// TH04-only. TH05's th05/main/player/bombanim.cpp is a different subsystem
/// with a per-playchar animation (Reimu stars, Marisa lasers, Mima circles,
/// Yuuka hearts) and shares no code with this.

// Star animation
// --------------
static const int BOMB_STAR_COUNT = 48;
static const pixel_t BOMB_STAR_W = 16;
static const pixel_t BOMB_STAR_H = 16;

struct bomb_star_t {
	SPPoint center;
	unsigned char angle;
	SubpixelLength8 speed;
};

extern bomb_star_t bomb_stars[BOMB_STAR_COUNT];

// The single frame of the bomb animation on which the whole star field is
// (re)placed. NOT [BOMB_CIRCLE_FRAMES]: that one is 32 and shared with TH02,
// and nothing ties this 48 to it. TH05 does spawn its own bomb animation on
// [BOMB_CIRCLE_FRAMES], so the *role* is shared and the number is not.
static const unsigned char BOMB_STARS_SPAWN_FRAME = 48;

// Reimu's stars fly straight up; Marisa's fly up and to the right.
static const unsigned char BOMB_STAR_ANGLE_REIMU = 0xC0;
static const unsigned char BOMB_STAR_ANGLE_MARISA = 0xE0;

// The 16×16 star in miko16.bft. Deliberately not added to
// th04/sprites/main_pat.h: that header leaves this patnum range unnamed on
// purpose ("naming one would be a guess"), this is its only user in the tree,
// and widening a header shared with TH05 to hold it would buy nothing.
static const int PAT_BOMB_STAR = 120;

// `extern "C"`, because th04_main.asm publishes the UNDECORATED
// `BOMB_STARS_UPDATE_AND_RENDER_FOR` (kb/codegen/0086), and because the
// two `call ... pascal, PLAYCHAR_*` sites left in that dump spell it that
// way. The stub declaration that stood in this file before the lift had no
// `extern "C"` -- it was never compiled, so nothing had ever graded it.
//
// [playchar] is `int` and not `playchar_t` on measurement, not on principle:
// the original tests it with `cmp word ptr [bp+4], 0`, and under the project's
// `-b-` a `playchar_t` formal is a BYTE, which compiles to
// `cmp byte ptr [bp+4], 0` at both of its two test sites. The enum's own
// constants are still what the two call sites push and what this body compares
// against.
extern "C" void pascal near bomb_stars_update_and_render_for(int playchar)
{
	// [star] is the SI iterator and [i] the DI counter (kb/codegen/0003);
	// [vector_x] and [vector_y] are the two words of the four-byte frame, and
	// both are reused as scratch further down, which is why it is not larger.
	bomb_star_t near *star;
	int i;
	subpixel_t vector_x;
	subpixel_t vector_y;

	if(bomb_frame == BOMB_STARS_SPAWN_FRAME) {
		star = bomb_stars;
		i = 0;

		// A `while` rather than a `for`, in both loops: the original increments
		// the COUNTER first and the iterator second (`inc di` / `add si, 6`),
		// and a `for` puts its own increment last, which swaps the pair.
		while(i < BOMB_STAR_COUNT) {
			star->center.x.v = randring1_next16_mod(TO_SP(PLAYFIELD_W));
			star->center.y.v = randring1_next16_mod(TO_SP(PLAYFIELD_H));
			if(playchar == PLAYCHAR_REIMU) {
				star->angle = BOMB_STAR_ANGLE_REIMU;

				// Keep the middle third of the playfield clear, so that the
				// stars converge on the player from both sides.
				while(
					(star->center.x.v >= TO_SP((PLAYFIELD_W / 3) * 1)) &&
					(star->center.x.v <= TO_SP((PLAYFIELD_W / 3) * 2))
				) {
					star->center.x.v = randring1_next16_mod(
						TO_SP(PLAYFIELD_W)
					);
				}

				// Speed falls off with the distance from the two off-screen
				// focal points, so the whole field arrives together.
				vector_x = ((star->center.x.v <= TO_SP(PLAYFIELD_W / 2))
					? (TO_SP(130) - star->center.x.v)
					: (star->center.x.v - TO_SP(254))
				);
				_AL = (vector_x / 9);
			} else {
				star->angle = BOMB_STAR_ANGLE_MARISA;
				// kb/codegen/0032: the original adds the minimum in AL and then
				// stores the byte, where `speed.v = and(...) + TO_SP(10)` widens
				// to `add ax, 0A0h` (+1 byte) and `speed.v += TO_SP(10)` after the
				// store adds to memory instead. Neither randring1_next8_ge_lt()
				// nor its _sp() wrapper can express this range: `to_sp8(18.0f)`
				// truncates to 0x20, so their `(max - min) - 1` computes 0xFF7F
				// where the original's mask is 0x7F.
				_AL = randring1_next16_and(TO_SP(8) - 1);
				_AL += TO_SP(10);
			}

			// ONE store, shared by both arms, which is why both of them leave
			// their result in AL rather than assigning the member: the original
			// has a single `mov [si+5], al` at the join and the `jl` arm jumps
			// to it. Two `star->speed.v = ...` statements compile to two stores;
			// Turbo C++ does not tail-merge them here.
			star->speed.v = _AL;

			i++;
			star++;
		}
	}

	star = bomb_stars;
	i = 0;
	while(i < BOMB_STAR_COUNT) {
		vector2(vector_x, vector_y, star->angle, star->speed.v);
		star->center.x.v += vector_x;
		star->center.y.v += vector_y;

		if(
			(star->center.x.v <= TO_SP(-(BOMB_STAR_W / 2))) ||
			(star->center.x.v >= TO_SP(PLAYFIELD_W + (BOMB_STAR_W / 2))) ||
			(star->center.y.v <= TO_SP(-(BOMB_STAR_H / 2))) ||
			// ZUN quirk: a whole sprite past the bottom edge, where the other
			// three bounds are half a sprite. Only reachable for Reimu, whose
			// stars are the ones that leave through the top.
			(star->center.y.v >= TO_SP(PLAYFIELD_H + BOMB_STAR_H))
		) {
			if(playchar == PLAYCHAR_REIMU) {
				// Straight back to the bottom, keeping the X it had.
				star->center.y.v = TO_SP(PLAYFIELD_H + BOMB_STAR_H);
			} else if(i & 1) {
				// Marisa's field is refilled from two edges at once, by
				// parity of the star's index: odd from the left…
				star->center.x.v = TO_SP(-8);
				star->center.y.v = randring1_next16_mod(TO_SP(PLAYFIELD_H));
			} else {
				// …even from the bottom.
				star->center.x.v = randring1_next16_mod(TO_SP(PLAYFIELD_W));
				star->center.y.v = TO_SP(PLAYFIELD_H + (BOMB_STAR_H / 2));
			}
		}

		// th04/formats/super.h: the caller sets ES to the plane and leaves the
		// GRCG in RMW mode. bomb_reimu() and bomb_marisa() do the GRCG half
		// before they call this; the plane is set per star, inside the loop,
		// exactly as the original does it.
		_ES = SEG_PLANE_B;
		vector_x = (
			star->center.x.to_pixel() + (PLAYFIELD_LEFT - (BOMB_STAR_W / 2))
		);
		z_super_put_16x16_mono(
			vector_x,
			(star->center.y.to_pixel() + (PLAYFIELD_TOP - (BOMB_STAR_H / 2))),
			PAT_BOMB_STAR
		);

		i++;
		star++;
	}
}
// --------------
