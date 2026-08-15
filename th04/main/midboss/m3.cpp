/// Stage 3 midboss
/// ---------------
/// (#included from th04/main/midboss/mx.cpp — see th04/main/midboss/m1.cpp for
/// why, and for why this file has no #includes of its own. `platform.h`, which
/// this file used to include on its own while nothing compiled it, comes from
/// there now.)

#define patterns_done	midboss3_patterns_done
#define FLY_ANGLES   	MIDBOSS3_FLY_ANGLES

// Constants
// ---------

static const uint8_t PATTERNS_MAX = 12;

extern const unsigned char FLY_ANGLES[PATTERNS_MAX];

static const pixel_t MIDBOSS3_W = 64;
static const pixel_t MIDBOSS3_H = 64;

// st02.bmt. Cel 0 is the still sprite; cels 1…3 are the wing flap, played
// forwards and then backwards while [midboss.sprite] is 1.
static const int PAT_MIDBOSS3 = (PAT_STAGE + 16);
static const int MIDBOSS3_FLAP_CELS = 4;
static const int MIDBOSS3_FLAP_FRAMES_PER_CEL = 4;
static const int MIDBOSS3_FLAP_FRAMES = (
	MIDBOSS3_FLAP_CELS * MIDBOSS3_FLAP_FRAMES_PER_CEL
);
// ---------

// Rendering
// ---------

void pascal near midboss3_render(void)
{
	screen_x_t left;
	vram_y_t top;
	int patnum;

	// Neither of the playfield_clip_*() macros: this is an exclusive bound
	// on all four edges, in y/x order, and it ignores the sprite extents
	// entirely rather than clipping half a sprite width early.
	if(
		(midboss.pos.cur.y.v <= 0) ||
		(midboss.pos.cur.y.v >= to_sp(PLAYFIELD_H)) ||
		(midboss.pos.cur.x.v <= 0) ||
		(midboss.pos.cur.x.v >= to_sp(PLAYFIELD_W))
	) {
		return;
	}
	left = midboss.pos.cur.to_screen_left(MIDBOSS3_W);
	top = midboss.pos.cur.to_vram_top_scrolled_seg1(MIDBOSS3_H);

	if(midboss.phase == PHASE_EXPLODE_BIG) {
		midboss_defeat_render();
		return;
	}
	if(midboss.phase > 2) {
		return;
	}

	patnum = PAT_MIDBOSS3;
	if(midboss.sprite == 1) {
		if(
			(midboss.phase_frame % (MIDBOSS3_FLAP_FRAMES * 2)) <
			MIDBOSS3_FLAP_FRAMES
		) {
			patnum += (
				(midboss.phase_frame % MIDBOSS3_FLAP_FRAMES) /
				MIDBOSS3_FLAP_FRAMES_PER_CEL
			);
		} else {
			patnum += ((MIDBOSS3_FLAP_CELS - 1) - (
				(midboss.phase_frame % MIDBOSS3_FLAP_FRAMES) /
				MIDBOSS3_FLAP_FRAMES_PER_CEL
			));
		}
	}
	midboss_put_generic(left, top, patnum);
}
// ---------

// Both of these are unprefixed tokens in a file that is #included into
// mx.cpp's translation unit, so without these they would leak into
// midbossx_render() and into any later lift landed in the same host.
// m1.cpp already does this for midboss1_top_patnum().
#undef patterns_done
#undef FLY_ANGLES
