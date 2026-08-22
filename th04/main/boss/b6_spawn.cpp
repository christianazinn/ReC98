/// Extra Stage Boss - Yuuka: the two spawners at the head of her segment
/// ---------------------------------------------------------------------
/// The first two procs of main_034_TEXT's root contribution, and the only two
/// above the `include` of th04/main/boss/b6_anim.asm that anything outside
/// ZUN's own object ever calls. They spawn the two entity kinds Yuuka's Extra
/// fight overlays on [custom_entities]: her chasing crosses, and the single
/// safety circle she opens around the player.
///
/// (#included from th04/main/boss/bg.cpp, at the END of it, and therefore
/// compiled into th04/boss_bg.cpp's object. Two things follow from that
/// choice and both are deliberate:
///
///  * the segment is NOT that object's default. A kb/codegen/0080 carve split
///    B6_SPAWN_TEXT off main_034_TEXT's head, and `#pragma codeseg` binds
///    these two definitions to it (kb/codegen/0155). One object contributing
///    to several code segments is what th04/main/scroll.hpp and
///    th04/main/hud/overlay.hpp already do to this same object -- it named
///    MAI_TEXT and HUD_OVRL_TEXT for zero bytes each before this file existed.
///    So the carve costs **no new translation unit and no Tupfile.lua line**,
///    which every carve before it paid.
///
///  * bg.cpp is the host because it is the ONLY translation unit that expands
///    th04/main/boss/b6.cpp, and that file is where chasecross_t,
///    safetycircle_t, their flag enums, CHASECROSS_COUNT and the two
///    [custom_entities] accessors live. b6.cpp cannot be included into
///    th04/b6_next.cpp's object instead: it declares yuuka6_phase_next() with
///    C++ linkage and that object DEFINES the same function `extern "C"`,
///    which is a redeclaration error rather than a byte-level risk. Restating
///    two struct layouts to avoid that would be worse than either.)
///
/// Position within the file does not matter once the binding is right --
/// kb/codegen/0155 verified that by moving one -- so this sits at the END of
/// bg.cpp's include list, where a line-anchored citation into that file cannot
/// be renumbered by it.

// Most of what these two need is already in this translation unit: b6.cpp for
// both structures and both accessors, th04/main/boss/boss.hpp for [boss], and
// th04/main/playfld.hpp for PLAYFIELD_LEFT/TOP through custom.hpp. Naming any
// of those again would be a compile error rather than a no-op, since none of
// them carries an include guard (kb/codegen 0110 + 0129).
//
// These two are NOT in it, and both are safe to add here specifically because
// this file is the LAST thing bg.cpp includes -- nothing in the object is
// compiled after them, so they cannot change the codegen of a function that is
// already matched. `[measured]`: both files are themselves guarded, and of
// their members only th03/snd/snd.h is unguarded at all, which nothing else in
// this translation unit reaches.
//
// The other five procs this segment holds are NOT here, and that is measured:
// yuuka6_customs_update() makes near calls into main_03, and this object's
// `-zP` names main_01, which frames them on the wrong group. One of those
// fixups overflowed and the rest would have linked to silent garbage
// (kb/codegen/0104). All five live in th04/main/boss/b6_next.cpp, whose object
// is `-zPmain_03` and whose zero-byte B6_SPAWN_TEXT block TLINK already places
// immediately after this one -- which is also their original address order.
#include "th04/snd/snd.h"
#include "th04/main/player/player.hpp"

#pragma codeseg B6_SPAWN_TEXT main_03

// Copies [angle] and [speed] into the first free chasing cross, positions it
// on Yuuka, and gives it 100 HP. Does nothing at all if every slot is busy.
//
// ZUN landmine, preserved exactly as written: the loop bound is one PAST the
// number of chasing crosses, so this can spawn one into the slot the safety
// circle overlays. It never happens in the original game -- only one pattern
// spawns up to 24 of these, fast enough that all of them have left the
// playfield before it fires again -- and it would not be observable if it did:
// these bullets keep Q12.4 coordinates in the fields the safety circle reads
// as raw pixels, so Yuuka would have to be near the playfield's top-left
// corner for the resulting circle not to be clipped away.
void pascal near chasecrosses_add(
	unsigned char angle, subpixel_length_8_t speed
)
{
	chasecross_t near *p;
	int i;

	// th04/main/boss/b6.cpp defines the overlay; this is where it is checked.
	custom_assert_count(chasecross_t, CHASECROSS_COUNT);

	p = chasecrosses;
	i = 0;
	while(i < (CHASECROSS_COUNT + 1)) {
		if(p->flag == CCF_FREE) {
			p->flag = CCF_ALIVE;
			p->damage_this_frame = 0;
			p->age = 0;
			p->angle = angle;
			p->speed.v = speed;
			p->hp = 100;
			p->center.x.v = boss.pos.cur.x.v;
			p->center.y.v = boss.pos.cur.y.v;
			return;
		}
		i++;
		p++;
	}
}

// Opens the safety circle on the player's current position, at the size it
// starts growing from. Called on the one frame Yuuka's forward-parasol phase
// begins, out of th04/main/boss/b6_next.cpp, which is the only caller in
// either game.
//
// The dump had no name of ZUN's for this. MATCH-TH04-MAIN-YUUKA6-PATTERNS gave
// it a zero-byte, address-suffixed alias (kb/codegen/0123) purely to make it
// linkable from the C++ that had just taken its caller, and recorded the naming
// debt that came with the placeholder. This parcel discharges it, and the alias
// went with the body. `extern "C"` keeps the undecorated linkage that alias
// published, so the new name is the whole diff at every call site.
extern "C" void near safetycircle_open(void)
{
	safetycircle_t near *p;

	// One instance, in the last [custom_entities] slot. This is the check that
	// th04/main/boss/b6.cpp's eight-byte spelling of the gap above [col_ring]
	// would have failed, and that a one-byte-red oracle had to catch instead.
	static_assert(sizeof(safetycircle_t) <= sizeof(custom_t));

	p = &safetycircle;
	p->flag = SCF_GROW;
	p->shrink_frame = 0;
	p->col_ring = 8;
	p->center.x = (player_pos.cur.x.to_pixel() + PLAYFIELD_LEFT);
	p->center.y = (player_pos.cur.y.to_pixel() + PLAYFIELD_TOP);
	p->radius_filled = 8;
	p->radius_ring_distance = 80;
	snd_se_play(8);
}

#pragma codeseg
