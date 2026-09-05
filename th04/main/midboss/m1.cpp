/// Stage 1 midboss
/// ---------------
/// (#included from th04/main/midboss/mx.cpp. ZUN's object for this code
/// segment held the Stage 1, Stage 3 and Extra Stage midboss renderers in
/// exactly that order — that an original object held several unrelated sources
/// is kb/codegen/0112 — so all three are compiled into that one translation
/// unit, by the host-source include form of kb/codegen/0129. This file
/// deliberately has no #includes of its own: every header it would need is
/// already included by mx.cpp, and 11 of the 13 in that closure have no
/// include guard.)
///
/// Because this file shares a translation unit with mx.cpp and m3.cpp, its
/// file-scope names are NOT file-local. Prefix every one of them.

// Constants
// ---------

static const pixel_t MIDBOSS1_W = 64;
static const pixel_t MIDBOSS1_H = 64;

// Phase 1 shows a single, smaller sprite; from phase 2 on, the midboss is two
// 64×32 cels stacked into a MIDBOSS1_W × MIDBOSS1_H square.
static const pixel_t MIDBOSS1_INTRO_W = 32;
static const pixel_t MIDBOSS1_INTRO_H = 32;

// st00.bmt cels for the two halves during phase 3. [midboss.sprite] carries
// the equivalent patnum during the phases before that, with the bottom half
// always following the top one.
static const int PAT_MIDBOSS1_BOTTOM = (PAT_STAGE + 18);
static const int PAT_MIDBOSS1_TOP = (PAT_STAGE + 19);
static const int MIDBOSS1_TOP_CELS = 5;
static const int MIDBOSS1_FRAMES_PER_CEL = 8;

#define midboss1_top_patnum() ( \
	PAT_MIDBOSS1_TOP + \
	((stage_frame / MIDBOSS1_FRAMES_PER_CEL) % MIDBOSS1_TOP_CELS) \
)
// ---------

// Rendering
// ---------

void pascal near midboss1_render(void)
{
	screen_x_t left;
	vram_y_t top;

	// No clipping at all, unlike midboss3_render() below — the Stage 1
	// midboss is scripted to stay well inside the playfield, so nothing
	// here is reached with an off-screen coordinate. Left unlabelled: the
	// same missing bound is a candidate `ZUN landmine` in
	// th04/main/midboss/mx.cpp's TH04 branch, and the taxonomy lane owns
	// that call for the whole family rather than this parcel.
	if(midboss.phase == 1) {
		left = midboss.pos.cur.to_screen_left(MIDBOSS1_INTRO_W);
		top = midboss.pos.cur.to_vram_top_scrolled_seg1(MIDBOSS1_INTRO_H);
		super_roll_put(left, top, midboss.sprite);
	} else if(midboss.phase == 2) {
		left = midboss.pos.cur.to_screen_left(MIDBOSS1_W);
		top = midboss.pos.cur.to_vram_top_scrolled_seg1(MIDBOSS1_H);
		super_roll_put(left, top, midboss.sprite);
		top = midboss.pos.cur.to_vram_top_scrolled_seg1(0);
		super_roll_put(left, top, (midboss.sprite + 1));
	} else if(midboss.phase == 3) {
		left = midboss.pos.cur.to_screen_left(MIDBOSS1_W);
		top = midboss.pos.cur.to_vram_top_scrolled_seg1(MIDBOSS1_H);

		// Not midboss_put_generic(): both halves have to be blitted with
		// the same white flash, and [damage_this_frame] may only be reset
		// after the second one.
		if(midboss.damage_this_frame == 0) {
			super_roll_put(left, top, midboss1_top_patnum());
			top = midboss.pos.cur.to_vram_top_scrolled_seg1(0);
			super_roll_put(left, top, PAT_MIDBOSS1_BOTTOM);
		} else {
			super_roll_put_1plane(
				left, top, midboss1_top_patnum(), 0, super_plane(V_WHITE)
			);
			top = midboss.pos.cur.to_vram_top_scrolled_seg1(0);
			super_roll_put_1plane(
				left, top, PAT_MIDBOSS1_BOTTOM, 0, super_plane(V_WHITE)
			);
			midboss.damage_this_frame = 0;
		}
	} else if(midboss.phase == PHASE_EXPLODE_BIG) {
		midboss_defeat_render();
	}
}
// ---------

#undef midboss1_top_patnum
