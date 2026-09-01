/* TH01 low-memory process chain helper.
 *
 * DOS EXEC keeps the calling process resident while its child runs. The
 * patched REIIDEN image is large enough that REIIDEN -> REIIDEN therefore no
 * longer fits in conventional memory. This tiny intermediary detaches from
 * and releases the suspended parent before loading the requested successor.
 */

#include <dos.h>
#include <process.h>
#include <stdlib.h>

static unsigned near psp_word(unsigned psp, unsigned byte_offset)
{
	return *reinterpret_cast<unsigned far *>(MK_FP(psp, byte_offset));
}

static void near psp_parent_set(unsigned psp, unsigned parent)
{
	*reinterpret_cast<unsigned far *>(MK_FP(psp, 0x16)) = parent;
}

int main(int argc, char **argv)
{
	unsigned parent;
	unsigned grandparent;
	unsigned parent_environment;
	unsigned own_environment;

	if((argc != 2) || !argv[1][0]) {
		return 1;
	}
	parent = psp_word(_psp, 0x16);
	if((parent == 0) || (parent == _psp)) {
		return 2;
	}
	grandparent = psp_word(parent, 0x16);
	parent_environment = psp_word(parent, 0x2C);
	own_environment = psp_word(_psp, 0x2C);

	// DOS chooses the return process from PSP:16 when this helper terminates.
	// Detach first so no later failure can return into the block released below.
	psp_parent_set(_psp, grandparent);
	if(parent_environment && (parent_environment != own_environment)) {
		_dos_freemem(parent_environment);
	}
	if(_dos_freemem(parent) != 0) {
		return 3;
	}
	execl(argv[1], argv[1], NULL);
	return 4;
}
