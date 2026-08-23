/// Extra Stage midboss
/// -------------------
/// AND IT IS NOT AN ENTITY AT ALL. `[measured]` The other four midbosses have a
/// sprite, a position, a hitbox, HP and a defeat animation; this one has none of
/// those. Its whole existence is a 420-frame scripted bullet event: three
/// expanding crosses of telegraphed pellet rings at three fixed points of VRAM,
/// then 132 idle frames, then a reset. It is installed into the same
/// [midboss_invalidate] / [midboss_update_and_render] slots as the other four
/// (th02/main/stage/init.cpp, `case 5`), and it reaches the player only through
/// [bullets_add_pellet].
///
/// So [midbossx_active] is doing something none of the other four flags do: it
/// is not "the midboss is still alive", because nothing can kill this one. It is
/// "the event is running", and what it buys is the suppression of stage enemy
/// spawns that [midboss_active] performs in stage_loop().
///
/// THE OBJECT EXISTS FOR THE LAYOUT, and the layout is the whole reason it is
/// not part of th02/main/boss/b6.cpp next door. These four procs were the TAIL
/// of th02_main.asm's BOSS_5_TEXT contribution, so the C++ that replaces them
/// has to be the first thing that contribution's successor emits
/// (kb/codegen/0099). b6.cpp is that successor, and prepending the Extra Stage
/// midboss into the middle of Evil Eye Sigma's file to save a Tupfile.lua line
/// would be paying in the wrong currency: a NEW object inserted at the same seam
/// costs one line and nothing else. `[measured]` Every BOSS_5_TEXT contribution
/// in obj/th02/main.map carries ACBP=28, i.e. BYTE segment alignment, so TLINK
/// inserts nothing between contributions, and an object exactly as long as the
/// bytes the root gave up leaves every later contribution at the offset it had.
/// th02/main/boss/b6.cpp is the sixth object to prove that on this segment.
///
/// The Tupfile.lua line therefore has to sit BETWEEN th02/dialog.cpp and
/// th02/main/boss/b6.cpp: TLINK lays a segment's contributions out in link
/// order, th02_main.asm is the first object it is handed, so that slot is the
/// seam this lift needs.

// -zC because the segment name would otherwise come from this file's own
// basename and be MX_TEXT (kb/codegen/0105). -zPmain_03 for the two near calls
// that leave this segment: bullets_add_pellet() is in BULLET_TEXT and
// tile_ring_set_and_put_both_8() in main_03_TEXT, and both are only reachable
// near because BOSS_5_TEXT is in the same group as they are - which is also how
// th02_main.asm itself reached them from this very block.
// -G because midbossx_bursts_update_and_render()'s prolog is
// `push bp; mov bp, sp; sub sp, 8` rather than an `enter 8, 0`
// (kb/codegen/0011). It is the only function here with a stack local at all.
// NO FILE-WIDE -a2, and that is a separate decision from the scoped one below.
// `[measured]` Nothing here emits a generated jump table -- the frame dispatch
// at the bottom is a plain compare chain, not a `switch` -- so this object has
// no table alignment to pin, and every kb/codegen entry about -a2 up to 0168 is
// about pinning one. What -a2 is needed for here is the pool's STRIDE, which is
// the flag's other job (kb/codegen/0170), and a file-wide one would then round
// MIDBOSSX_BURST_TILE_IMAGES up to 8 bytes and turn its 7-byte copy into four
// words. So it is scoped to the one struct that needs it.
#pragma option -zCBOSS_5_TEXT -zPmain_03 -G

#include "platform.h"
#include "pc98.h"
#include "th02/main/scroll.hpp"
#include "th02/main/tile/tile.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/midboss/midboss.hpp"

/// Its queue of telegraphed pellet rings
/// -------------------------------------
/// A slot is claimed with a position, a bullet group, an angle and a speed, and
/// then does nothing to the player for 48 frames: every 8th frame it advances
/// one cel of a six-cel tile animation drawn at its own position, and on the
/// frame it runs out of cels it fires its ring of pellets and frees itself.
///
/// `burst` for the slot, after pattern_symmetric_bursts()
/// (th05/main/boss/b1.cpp), which is this tree's word for a group of bullets
/// fired at one instant from one point - which is exactly what a slot resolves
/// into. And telegraph for the animation that precedes it, after
/// SIGMA_BLAST_TELEGRAPH (th02/main/boss/b6.cpp), the same idea one file over.
///
/// `[measured]` THE CEL IS ALSO THE ALIVE FLAG: 0 means free, the spawn writes
/// 1, and the fire writes 0 back. So a slot has no separate flag field, and the
/// six drawn cels are 1..6 rather than 0..5 -- which leaves entry [0] of the
/// image table below permanently unread.

// Instruction-derived, and pinned by the walkers rather than by the 300 bytes
// the dump reserves: both functions below advanced their walk pointer by ten
// bytes a slot and stopped when their signed counter reached thirty.
static const int MIDBOSSX_BURST_COUNT = 30;

// How many cels the telegraph has, i.e. the cel on which the ring fires, and
// how often it advances. `[measured]` The interval is a bitmask against 7 and
// not a modulo against 8 - the original tests the low bits of a byte rather
// than dividing - so it is spelled as the mask it is.
static const uint8_t MIDBOSSX_BURST_CELS = 7;
static const uint8_t MIDBOSSX_BURST_CEL_INTERVAL_MASK = 7;

// Where the ring's pellets come out relative to the tile the telegraph was
// drawn on. `[measured]` Four on both axes, which is a quarter of TILE_W and
// TILE_H rather than a half, so the pellets do NOT come out of the tile's
// centre.
static const pixel_t MIDBOSSX_BURST_PELLET_OFFSET = 4;

// -a2 FOR THIS DECLARATION ONLY. `[measured]` The slot is 10 bytes -- both
// walkers advanced by ten -- and these seven fields pack to 9 under this tree's
// default byte alignment. Adding 9 to a register and adding 10 to it are the
// SAME THREE BYTES, so the object's SEGDEF length comes out at exactly the
// 0x2B4 the root gave up either way, and only the LEDATA disassembly can tell
// the two apart. kb/codegen/0170.
#pragma option -a2
struct midbossx_burst_t {
	// 0 free, 1..6 the telegraph's cels, and the frame it would reach 7 on is
	// the frame the ring fires and the slot goes back to 0. `[measured]`
	// UNSIGNED: the fire test is a `jb`.
	uint8_t cel;

	// Frames since the slot was claimed, `inc`ed once per frame in every state
	// but 0 and NEVER reset -- so the mask above really does gate on the age of
	// the slot rather than on time since the last cel.
	uint8_t frame;

	screen_x_t left;

	// A VRAM ROW, not a screen y, and that is what the two wraps prove rather
	// than any declaration: the spawn folds an out-of-range value back into
	// [0, RES_Y) at both ends, and the fire subtracts [scroll_line] and folds
	// again. So a queued burst stays where the SCENERY is while the field
	// scrolls under it, and the pellets come out of the tile the player saw.
	vram_y_t vram_y;

	// What the ring will be, and how it comes out. `[measured]` The three
	// callers pass three different groups and always the same angle and speed,
	// but all three are per-slot rather than global.
	uint8_t group; // ACTUAL TYPE: bullet_group_or_special_motion_t
	unsigned char angle;
	uint8_t speed; // A subpixel_t, narrowed to a byte for storage.
};
// Back to byte alignment: the all-byte aggregate below must stay 7 bytes wide.
#pragma option -a-

// `[measured]` Exclusive to the two functions below -- they held both references
// the dump had to this address, each of them the `offset` a near walk pointer is
// seeded from -- so IDA's placeholder is RETIRED rather than aliased. `near`,
// like [shots] and [stones_tile_pending], because the original walks it with a
// bare 16-bit SI: a large-model far pointer would not fit a register at all.
extern "C" midbossx_burst_t near midbossx_bursts[MIDBOSSX_BURST_COUNT];

// The six tile images the telegraph animates through, plus the unread entry [0]
// that the cel-doubles-as-flag encoding above leaves at the front.
//
// THE STORAGE STAYS IN th02_main.asm's _DATA, and its own address is the reason:
// this is the initializer template of a LOCAL aggregate (kb/codegen/0084), so
// the C++ side would ordinarily re-emit it and the dump's copy would go - except
// that the _DATA contribution of every C++ object in this binary is 0000 in
// obj/th02/main.map, so the first byte any of them emitted would land at the end
// of the dump's block and shift every byte of live data after it. Exactly the
// situation _SIGMA_LASER_X_OFFSETS is in, four labels further down that block.
//
// SCREAMING_CASE because it is const data, which is how _GAME_CLEAR_CONSTANTS,
// _EXTRA_CLEAR_FLAGS and _SIGMA_LASER_X_OFFSETS are already published there.
static const int MIDBOSSX_BURST_TILE_IMAGE_COUNT = 7;

struct midbossx_burst_tile_images_t {
	uint8_t image[MIDBOSSX_BURST_TILE_IMAGE_COUNT];
};
extern "C" const midbossx_burst_tile_images_t MIDBOSSX_BURST_TILE_IMAGES;

// Claims the first free slot for a ring at (left, vram_y), folding the row back
// into VRAM at both ends first. Silently does nothing if all 30 are busy, and
// no caller could tell either way: this returns nothing.
//
// Plural pool noun plus `_add`, which is the house shape by sixteen precedents
// to one (bullets_add_pellet, items_add, lasers_add, sparks_add, enemies_add,
// sigma_blasts_add); `_spawn` is never a verb in this tree.
//
// `pascal`, which is what the original's callee-popping return of ten argument
// bytes says, and `static`, because
// midbossx_update_and_render() below is its only caller and the dump never
// published it -- so unlike sigma_blasts_add() one object over, this one never
// needed a kb/codegen/0123 alias at all.
static void pascal near midbossx_bursts_add(
	screen_x_t left, vram_y_t vram_y, uint8_t group, unsigned char angle,
	uint8_t speed
)
{
	register midbossx_burst_t near *burst;
	int i;

	if(vram_y < 0) {
		vram_y += RES_Y;
	} else if(vram_y >= RES_Y) {
		vram_y -= RES_Y;
	}
	burst = midbossx_bursts;
	for(i = 0; i < MIDBOSSX_BURST_COUNT; i++, burst++) {
		if(burst->cel == 0) {
			burst->cel = 1;
			burst->frame = 0;
			burst->left = left;
			burst->vram_y = vram_y;
			burst->group = group;
			burst->angle = angle;
			burst->speed = speed;
			return;
		}
	}
}

// Advances every claimed slot's telegraph, and fires the ones that run out of
// cels. Called unconditionally at the bottom of midbossx_update_and_render(),
// i.e. it keeps running for the 132 frames after the last cross is over and for
// as long as it takes the last queued burst to resolve.
static void near midbossx_bursts_update_and_render(void)
{
	// A plain copy assignment, and it has to be the FIRST statement, because
	// that is where the original does it (kb/codegen/0084 + kb/codegen/0109).
	// Seven bytes come across as three words and a byte rather than through a
	// `rep movsw`.
	midbossx_burst_tile_images_t images = MIDBOSSX_BURST_TILE_IMAGES;

	register midbossx_burst_t near *burst;
	register int i;

	burst = midbossx_bursts;
	for(i = 0; i < MIDBOSSX_BURST_COUNT; i++, burst++) {
		if(burst->cel == 0) {
			continue;
		}
		burst->frame++;
		if((burst->frame & MIDBOSSX_BURST_CEL_INTERVAL_MASK) != 0) {
			continue;
		}
		tile_ring_set_and_put_both_8(
			burst->left, burst->vram_y, images.image[burst->cel]
		);
		burst->cel++;
		if(burst->cel < MIDBOSSX_BURST_CELS) {
			continue;
		}

		// The row was stored in the scenery's frame of reference; this is where
		// it is converted back into one the bullet code understands.
		burst->vram_y -= scroll_line;
		if(burst->vram_y < 0) {
			burst->vram_y += RES_Y;
		}

		bullets_add_pellet(
			(burst->left + MIDBOSSX_BURST_PELLET_OFFSET),
			(burst->vram_y + MIDBOSSX_BURST_PELLET_OFFSET),
			burst->angle,
			burst->group,
			burst->speed
		);
		burst->cel = 0;
	}
}
/// -------------------------------------

/// The event itself
/// ----------------

// `[measured]` Raised on the event's first frame and cleared on its last, and
// nothing else ever writes it. It is not a liveness flag for an entity, because
// there is no entity: see the top of this file.
extern "C" bool midbossx_active;

// How far the four points of the current cross sit from its centre, stepped by
// MIDBOSSX_CROSS_DISTANCE_STEP after every burst and reset to 0 at the start of
// each cross. `[measured]` UNSIGNED (`mov al` + `mov ah, 0` at all eight reads),
// and the zero case is a branch of its own rather than a degenerate cross: at
// distance 0 the four points coincide, so the original fires ONE burst at the
// centre instead of four on top of each other.
extern "C" uint8_t midbossx_cross_distance;

// The three centres, and none of them is a playfield landmark. `[measured]`
// PLAYFIELD_LEFT is 32 and PLAYFIELD_RIGHT is 416, so the first is roughly
// playfield-centred and the other two are not; and all six coordinates are
// multiples of TILE_W, which is consistent with the telegraph being drawn in the
// tile layer but is `[inferred]` as intent, since nothing in the code derives
// them from a tile.
//
// The y of each is a VRAM ROW, so the second cross is centred on the very top of
// VRAM and half of it wraps around to the bottom -- which is what
// midbossx_bursts_add()'s two folds exist for.
static const screen_x_t MIDBOSSX_CROSS_1_CENTER_X = 128;
static const vram_y_t MIDBOSSX_CROSS_1_CENTER_Y = 128;
static const screen_x_t MIDBOSSX_CROSS_2_CENTER_X = 304;
static const vram_y_t MIDBOSSX_CROSS_2_CENTER_Y = 0;
static const screen_x_t MIDBOSSX_CROSS_3_CENTER_X = 192;
static const vram_y_t MIDBOSSX_CROSS_3_CENTER_Y = 336;

// And their rings, which escalate. Every burst of one cross uses the same group.
static const uint8_t MIDBOSSX_CROSS_1_GROUP = BG_4_RING;
static const uint8_t MIDBOSSX_CROSS_2_GROUP = BG_8_RING;
static const uint8_t MIDBOSSX_CROSS_3_GROUP = BG_16_RING;

// The frame each of the second and third crosses resets the distance on, which
// is also the frame the one before it stops firing on. `[measured]` Each is
// tested TWICE -- once as the previous cross's bound and once as its own reset
// -- and the original does not fold the two compares into one.
static const int MIDBOSSX_CROSS_2_FRAME = 96;
static const int MIDBOSSX_CROSS_3_FRAME = 192;

// One past the last frame the third cross fires on. Nothing happens between this
// and the reset below except the queue draining.
static const int MIDBOSSX_CROSSES_PAST_LAST_FRAME = 288;

// How often a cross fires, and how much wider it gets each time -- so each cross
// fires on five frames and reaches four tiles out.
//
// A modulo and not a bitmask for THIS interval, which is kb/codegen/0094's
// neighbouring discriminator: the original sign-extends and divides the signed
// [int16_t] [boss_phase_frame], which is what a modulo compiles to and a
// bitmask never does. And the step is [int] and not the [uint8_t] its variable
// is, which IS kb/codegen/0094: a byte-typed addend would fold the whole
// increment into one add-immediate on memory where the original takes the AL
// round trip.
static const int MIDBOSSX_BURST_INTERVAL = 16;
static const int MIDBOSSX_CROSS_DISTANCE_STEP = 16;

// The angle and speed every one of the 51 bursts is fired at. 100 subpixels is
// 6¼ pixels a frame; `((6 << 4) + 4)` is th02/main/boss/b3.cpp's spelling for
// the same parameter.
static const unsigned char MIDBOSSX_BURST_ANGLE = 0x00;
static const uint8_t MIDBOSSX_BURST_SPEED = ((6 << 4) + 4);

// One past the last frame of the whole event. `[measured]` The test is `>`, so
// the event is 421 frames long and the last 133 of them fire nothing.
static const int MIDBOSSX_PAST_LAST_FRAME = 420;

// Returns the new value of [midboss_active], which for this one is simply
// whether the event is running.
bool16 midbossx_invalidate(void)
{
	return midbossx_active;
}

// The event: three expanding crosses of telegraphed pellet rings, then the
// queue draining, then a reset.
//
// `[measured]` The four calls of a cross go left, up, right, down, and the
// original's `-O` cross-jumps all three crosses' trailing pushes onto ONE
// `call` -- the angle, the speed and the last of the four positions are pushed
// from a single shared tail that every one of the fifteen call sites in this
// function reaches. So the arms have to be written PLAIN, with the constants
// spelled at each site: hoisting the group or the centre into a variable turns
// the packed 32-bit push of the distance-0 case into two register pushes and
// the shared tail into three separate ones.
void midbossx_update_and_render(void)
{
	boss_phase_frame++;
	if(boss_phase_frame == 1) {
		midbossx_cross_distance = 0;
		midbossx_active = true;
	} else if(boss_phase_frame < MIDBOSSX_CROSS_2_FRAME) {
		if((boss_phase_frame % MIDBOSSX_BURST_INTERVAL) == 0) {
			if(midbossx_cross_distance == 0) {
				midbossx_bursts_add(
					MIDBOSSX_CROSS_1_CENTER_X, MIDBOSSX_CROSS_1_CENTER_Y,
					MIDBOSSX_CROSS_1_GROUP, MIDBOSSX_BURST_ANGLE,
					MIDBOSSX_BURST_SPEED
				);
			} else {
				midbossx_bursts_add(
					(MIDBOSSX_CROSS_1_CENTER_X - midbossx_cross_distance),
					MIDBOSSX_CROSS_1_CENTER_Y,
					MIDBOSSX_CROSS_1_GROUP, MIDBOSSX_BURST_ANGLE,
					MIDBOSSX_BURST_SPEED
				);
				midbossx_bursts_add(
					MIDBOSSX_CROSS_1_CENTER_X,
					(MIDBOSSX_CROSS_1_CENTER_Y - midbossx_cross_distance),
					MIDBOSSX_CROSS_1_GROUP, MIDBOSSX_BURST_ANGLE,
					MIDBOSSX_BURST_SPEED
				);
				midbossx_bursts_add(
					(MIDBOSSX_CROSS_1_CENTER_X + midbossx_cross_distance),
					MIDBOSSX_CROSS_1_CENTER_Y,
					MIDBOSSX_CROSS_1_GROUP, MIDBOSSX_BURST_ANGLE,
					MIDBOSSX_BURST_SPEED
				);
				midbossx_bursts_add(
					MIDBOSSX_CROSS_1_CENTER_X,
					(MIDBOSSX_CROSS_1_CENTER_Y + midbossx_cross_distance),
					MIDBOSSX_CROSS_1_GROUP, MIDBOSSX_BURST_ANGLE,
					MIDBOSSX_BURST_SPEED
				);
			}
			midbossx_cross_distance += MIDBOSSX_CROSS_DISTANCE_STEP;
		}
	} else if(boss_phase_frame == MIDBOSSX_CROSS_2_FRAME) {
		midbossx_cross_distance = 0;
	} else if(boss_phase_frame < MIDBOSSX_CROSS_3_FRAME) {
		if((boss_phase_frame % MIDBOSSX_BURST_INTERVAL) == 0) {
			if(midbossx_cross_distance == 0) {
				midbossx_bursts_add(
					MIDBOSSX_CROSS_2_CENTER_X, MIDBOSSX_CROSS_2_CENTER_Y,
					MIDBOSSX_CROSS_2_GROUP, MIDBOSSX_BURST_ANGLE,
					MIDBOSSX_BURST_SPEED
				);
			} else {
				midbossx_bursts_add(
					(MIDBOSSX_CROSS_2_CENTER_X - midbossx_cross_distance),
					MIDBOSSX_CROSS_2_CENTER_Y,
					MIDBOSSX_CROSS_2_GROUP, MIDBOSSX_BURST_ANGLE,
					MIDBOSSX_BURST_SPEED
				);
				midbossx_bursts_add(
					MIDBOSSX_CROSS_2_CENTER_X,
					// A plain unary minus, and not
					// `(MIDBOSSX_CROSS_2_CENTER_Y - midbossx_cross_distance)`,
					// which is the same value: this cross's centre row is 0,
					// and Turbo C++ zeroes a second register and subtracts into
					// it for a subtraction from a literal zero, where a unary
					// minus is one negate. Two bytes, and the only divergence
					// this body's first screen had. The addition arm below
					// needs no such note, because adding zero folds away on
					// its own.
					-midbossx_cross_distance,
					MIDBOSSX_CROSS_2_GROUP, MIDBOSSX_BURST_ANGLE,
					MIDBOSSX_BURST_SPEED
				);
				midbossx_bursts_add(
					(MIDBOSSX_CROSS_2_CENTER_X + midbossx_cross_distance),
					MIDBOSSX_CROSS_2_CENTER_Y,
					MIDBOSSX_CROSS_2_GROUP, MIDBOSSX_BURST_ANGLE,
					MIDBOSSX_BURST_SPEED
				);
				midbossx_bursts_add(
					MIDBOSSX_CROSS_2_CENTER_X,
					(MIDBOSSX_CROSS_2_CENTER_Y + midbossx_cross_distance),
					MIDBOSSX_CROSS_2_GROUP, MIDBOSSX_BURST_ANGLE,
					MIDBOSSX_BURST_SPEED
				);
			}
			midbossx_cross_distance += MIDBOSSX_CROSS_DISTANCE_STEP;
		}
	} else if(boss_phase_frame == MIDBOSSX_CROSS_3_FRAME) {
		midbossx_cross_distance = 0;
	} else if(boss_phase_frame < MIDBOSSX_CROSSES_PAST_LAST_FRAME) {
		if((boss_phase_frame % MIDBOSSX_BURST_INTERVAL) == 0) {
			if(midbossx_cross_distance == 0) {
				midbossx_bursts_add(
					MIDBOSSX_CROSS_3_CENTER_X, MIDBOSSX_CROSS_3_CENTER_Y,
					MIDBOSSX_CROSS_3_GROUP, MIDBOSSX_BURST_ANGLE,
					MIDBOSSX_BURST_SPEED
				);
			} else {
				midbossx_bursts_add(
					(MIDBOSSX_CROSS_3_CENTER_X - midbossx_cross_distance),
					MIDBOSSX_CROSS_3_CENTER_Y,
					MIDBOSSX_CROSS_3_GROUP, MIDBOSSX_BURST_ANGLE,
					MIDBOSSX_BURST_SPEED
				);
				midbossx_bursts_add(
					MIDBOSSX_CROSS_3_CENTER_X,
					(MIDBOSSX_CROSS_3_CENTER_Y - midbossx_cross_distance),
					MIDBOSSX_CROSS_3_GROUP, MIDBOSSX_BURST_ANGLE,
					MIDBOSSX_BURST_SPEED
				);
				midbossx_bursts_add(
					(MIDBOSSX_CROSS_3_CENTER_X + midbossx_cross_distance),
					MIDBOSSX_CROSS_3_CENTER_Y,
					MIDBOSSX_CROSS_3_GROUP, MIDBOSSX_BURST_ANGLE,
					MIDBOSSX_BURST_SPEED
				);
				midbossx_bursts_add(
					MIDBOSSX_CROSS_3_CENTER_X,
					(MIDBOSSX_CROSS_3_CENTER_Y + midbossx_cross_distance),
					MIDBOSSX_CROSS_3_GROUP, MIDBOSSX_BURST_ANGLE,
					MIDBOSSX_BURST_SPEED
				);
			}
			midbossx_cross_distance += MIDBOSSX_CROSS_DISTANCE_STEP;
		}
	}
	if(boss_phase_frame > MIDBOSSX_PAST_LAST_FRAME) {
		boss_phase_frame = 0;
		midbossx_active = false;
	}
	midbossx_bursts_update_and_render();
}
/// ----------------
