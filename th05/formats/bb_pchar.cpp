/// Playchar bomb animation (.BB)
/// -----------------------------
/// The playchar-specific half of the lifecycle th04/formats/bb_boss.cpp
/// implements for bosses, and in TH05 it is *structurally* that file's `#if
/// (GAME == 5)` branch with the globals renamed: one filename, no .CDG, and
/// bb_load() rather than the four master.lib file calls TH04 emits inline.
///
/// TH05 only. TH04 keeps two filenames, loads a .CDG here as well, and opens
/// the file itself, which is why its copy is th04/formats/bb_playchar.cpp and
/// not this one (state/notes/bb_playchar_load.md).
///
/// No segment pragma here: this file is #included at the END of
/// th05/bombchar.cpp, inside that wrapper's `#pragma codeseg BB_PCHAR_TEXT
/// main_01` block. END is not a detail — everything else in that object was
/// already matched, and putting an #include ahead of matched code is what
/// changes its header closure and therefore its codegen.
#include "libs/master.lib/master.hpp"
#include "th05/formats/bb.h"
#include "th05/playchar.h"

// Neither function has a stack frame at all in the original: the first
// instruction of each is its first real one. TH04's copy of this pair DOES
// carry `push bp` / `mov bp, sp`, so this is not inherited from that parcel --
// read the target, not the sibling.
#pragma option -k-

// Declared here rather than in th05/formats/bb.h, which every other lane's
// parcels also read: nothing outside this file needs either of them from C++.
// th04/formats/bb_playchar.cpp does the same for its own copy.
extern bb_tiles8_t __seg *bb_playchar_seg;

// "BB0.BB", whose third character is overwritten below. An ARRAY, not a
// pointer — the dump stores the string itself in _DATA
// (th05/formats/bb_playchar[data].asm), where TH04 stores a dword pointer to
// it, and that difference is the whole reason this statement is `+=` on a
// subscript rather than a store through a far load.
extern char bb_playchar_fn[];

extern "C" void pascal near bb_playchar_load(void)
{
	// ADDS the playchar, rather than assigning its ASCII digit the way TH04
	// does: [playchar] is 0-based here, so the '0' the filename already
	// carries is what makes the sum right. The literal index is the tree's
	// unanimous convention for this statement — six sites patch a playchar
	// digit into a filename and every one spells a bare index
	// (state/notes/bb_playchar_load.md ran that census).
	//
	// THE CAST IS THE INSTRUCTION SEQUENCE, not a readability choice. Without
	// it, [playchar]'s enum type promotes to `int` and Turbo C++ compiles the
	// compound assignment by loading the DESTINATION first --
	// `mov al, [fn+2]` / `add al, [playchar]` / `mov [fn+2], al`, three bytes
	// over. Narrowing the right-hand side to `char` first gives the original's
	// `mov al, [playchar]` / `add [fn+2], al`. Measured over seven spellings
	// before the first build; only this one and a raw
	// `_asm { add byte ptr bb_playchar_fn+2, al }` reach 0x12 bytes.
	bb_playchar_fn[2] += static_cast<char>(playchar);
	bb_playchar_seg = bb_load(bb_playchar_fn);
}

extern "C" void pascal near bb_playchar_free(void)
{
	if(bb_playchar_seg) {
		HMem<bb_tiles8_t>::free(bb_playchar_seg);
		bb_playchar_seg = 0;
	}
}

// Alignment padding after the pair, in the original — the byte the deleted
// module spelled as a bare `nop` after its second `endp`. It has to be emitted
// here rather than left to the assembler, and it lands in source order
// (kb/codegen/0161).
#pragma codestring "\x90"

#pragma option -k.
