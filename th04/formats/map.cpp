/// Stage background tile sections
/// ------------------------------
/// TWO bodies. TH04 loads the .MAP through master.lib's file subsystem and
/// keeps the header in a local; TH05 hand-rolls the same three DOS calls and
/// reads the header into a global, the same way th05/formats/cfg_lres.cpp
/// hand-rolls .CFG loading. They do not even free the old map at the same
/// point.

// The [-zC]/[-zP] segment pragmas live in the th0N/map.cpp wrappers, not here:
// TH04's wrapper #includes a second file ahead of this one, and a segment
// pragma only takes effect before any code is generated (kb/codegen/0112).

#include "libs/master.lib/master.hpp"
#include "th04/formats/map.hpp"
#if (GAME == 5)
	#include "x86real.h"
	#include "th04/main/stage/stage.hpp"
#else
	#include "th04/resident.hpp"
#endif

// The .MAP filename, still owned by the dump's data segment: TH04 reaches the
// string through a far pointer of its own, TH05 addresses it directly.
#if (GAME == 5)
	extern char map_filename[];

	// Read straight into place by the DOS call below, so it can't be a local.
	extern map_header_t map_header;
#else
	extern char *mapname;
#endif

void near map_load(void)
{
	#if (GAME == 5)
		register int handle;

		map_free();
		map_filename[3] = ('0' + stage_id);

		// DOS file open
		_AX = 0x3D00;
		reinterpret_cast<const char near *>(_DX) = map_filename;
		geninterrupt(0x21);
		_BX = _AX;
		handle = _AX;

		// DOS file read, header
		_AH = 0x3F;
		reinterpret_cast<map_header_t near *>(_DX) = &map_header;
		_CX = sizeof(map_header);
		geninterrupt(0x21);

		map_seg = reinterpret_cast<map_section_tiles_t __seg *>(
			hmem_allocbyte(map_header.size)
		);

		// DOS file read, sections. [map_seg] is still in AX, and DS has to
		// point at it for the duration of the call.
		_asm { push ds; }
		_BX = handle;
		_CX = map_header.size;
		_asm {
			mov 	ds, ax;
			// `xor dx, dx`, pinned to the original opcode direction; TASM
			// otherwise picks the equivalent `31 D2`, which passes
			// mzdiff-semantic and fails a byte-exact funcdiff.
			// (kb/codegen/0037)
			db  	033h, 0D2h;
			mov 	ah, 3Fh;
			int 	21h;
			pop 	ds;
		}

		// DOS file close
		_AH = 0x3E;
		geninterrupt(0x21);
	#else
		map_header_t mh;

		mapname[3] = resident->stage_ascii;
		file_ropen(mapname);
		file_read(&mh, sizeof(mh));
		map_free();
		map_seg = reinterpret_cast<map_section_tiles_t __seg *>(
			hmem_allocbyte(mh.size)
		);
		file_read(map_seg, mh.size);
		file_close();
	#endif
}

void near map_free(void)
{
	if(map_seg) {
		hmem_free(map_seg);
		map_seg = nullptr;
	}
}
