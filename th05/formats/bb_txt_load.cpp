/// Text dissolve circles: loading and freeing
/// ------------------------------------------
/// TH05 only. `th04/formats/bb_txt_load.cpp` is TH04's half, and this is NOT a
/// port of it -- the two originals share the allocation and nothing else.
///
/// [measured] TH04 loads through master.lib (`file_ropen` / `file_read` /
/// `file_close`) out of two immutable filename strings. TH05 issues INT 21h
/// AH=3Dh / 3Fh / 3Eh itself, reads straight into the freshly allocated segment
/// with DS pointed at it, and has ONE filename string that it rewrites in place
/// to name the second file. Not one file-I/O statement is common to the two, so
/// there is nothing to fence with `#if (GAME == 5)` and no shared body to owe.
/// The precedent this does follow is `th02/snd/load.cpp`, the other hand-rolled
/// INT 21h loader in this tree -- and, more closely still,
/// `th05/main/hiscore.cpp`, the file this one is compiled together with, which
/// spells the same three DOS calls the same way.
///
/// `bb_txt_free()` is the one function that IS identical to TH04's. It is
/// duplicated rather than shared, because sharing it would mean giving
/// `th04/formats/bb_txt_load.cpp` a `GAME == 5` arm and with it `x86real.h` and
/// the pseudo-register machinery -- inside `th04/main_01.cpp`, a translation
/// unit that is already matched.
///
/// `#include`d from `th05/score_rm.cpp`, which binds both functions to
/// `BB_TXT_TEXT` with `#pragma codeseg` ahead of its own includes
/// (kb/codegen/0155's third fix).

#include "libs/master.lib/master.hpp"
#include "th04/formats/bb.h"
#include "x86real.h"

// TXT1.BB, defined in th05/formats/bb_txt_load[data].asm. Writable, and
// written to: the loader turns the '1' into a '2' in place to name the second
// file. Hence not `const`, and hence the lower-case spelling, following the
// `_cfg_fn` / `_dialog_fn` convention th02_main.asm uses for writable filename
// buffers rather than the `_GAIJI_FN` one it uses for immutable ones.
extern "C" char near bb_txt_fn[];

// The whole object is already `-k-`, from th05/main/hiscore.cpp's own
// `#pragma option`. Restated here so that this file does not depend on the
// order its host happens to include it in; both originals are frameless.
#pragma option -k-

extern "C" void pascal near bb_txt_load(void)
{
	// One allocation for both files, so that the cel index the blitter uses
	// runs straight across the two. This is the one statement TH04's loader
	// also has: `sizeof(bb_txt_t)` is BB_SIZE + (BB_SIZE / 2), the size the
	// original passes to hmem_allocbyte().
	bb_txt_seg = HMem<bb_txt_t>::alloc(1);

	// DOS file open
	_AX = 0x3D00;
	_DX = FP_OFF(bb_txt_fn);
	geninterrupt(0x21);
	_BX = _AX;
	// ZUN landmine: No error handling, on either of the two opens.

	// DOS file read. DS addresses the allocated segment for the duration of
	// the call, so the destination is a bare offset rather than a far pointer.
	_asm { push ds; }
	_DS = reinterpret_cast<unsigned>(bb_txt_seg);
	_CX = BB_SIZE;
	_DX = 0;
	_AH = 0x3F;
	geninterrupt(0x21);
	_asm { pop ds; }

	// DOS file close
	_AH = 0x3E;
	geninterrupt(0x21);

	// TXT1.BB -> TXT2.BB, in place. TH04 has a second string instead.
	bb_txt_fn[3] = '2';

	_AX = 0x3D00;
	_DX = FP_OFF(bb_txt_fn);
	geninterrupt(0x21);
	_BX = _AX;

	// The second file is read in behind the first one, which is what the
	// BB_SIZE offset in DX is; BB_TXT_IN_SPRITE * sizeof(bb_txt_cel_t) is the
	// same number, and `bb_txt_seg->in` is how TH04 spells it.
	_asm { push ds; }
	_DS = reinterpret_cast<unsigned>(bb_txt_seg);
	_CX = (BB_SIZE / 2);
	_DX = BB_SIZE;
	_AH = 0x3F;
	geninterrupt(0x21);
	_asm { pop ds; }

	_AH = 0x3E;
	geninterrupt(0x21);
}

extern "C" void pascal near bb_txt_free(void)
{
	if(bb_txt_seg) {
		HMem<bb_txt_t>::free(bb_txt_seg);
		bb_txt_seg = 0;
	}
}

// The `nop` that closed the ASM module this file replaced is NOT emitted
// here -- th05/score_rm.cpp emits it, right after this file. See the note
// there;
// the short version is that `#pragma codestring` writes into whatever segment
// is current where it APPEARS, which is not the segment the two functions
// above were bound to by declaration.
