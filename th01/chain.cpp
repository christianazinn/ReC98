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

#define T1CHAIN_MCB_NORMAL 0x4D
#define T1CHAIN_MCB_LAST 0x5A
#define T1CHAIN_PARENT_BLOCK_MAX 32

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
	unsigned parent_blocks[T1CHAIN_PARENT_BLOCK_MAX];
	unsigned parent_block_count = 0;
	unsigned mcb;
	unsigned next_mcb;
	unsigned owner;
	unsigned size;
	unsigned char type;

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

	// The parent PSP block is not necessarily its only DOS allocation.
	// master.lib's heap can live in a separate block owned by the same PSP, and
	// leaving that block resident starves the successor during mem_assign_all().
	// Walk the arena before releasing anything, then free the matching blocks in
	// reverse order so DOS never has to continue through a coalesced MCB.
	mcb = static_cast<unsigned>(parent - 1);
	while(1) {
		type = peekb(mcb, 0);
		if((type != T1CHAIN_MCB_NORMAL) && (type != T1CHAIN_MCB_LAST)) {
			return 3;
		}
		owner = peek(mcb, 1);
		size = peek(mcb, 3);
		if((owner == parent) && ((mcb + 1) != parent_environment)) {
			if(parent_block_count >= T1CHAIN_PARENT_BLOCK_MAX) {
				return 3;
			}
			parent_blocks[parent_block_count++] = static_cast<unsigned>(mcb + 1);
		}
		if(type == T1CHAIN_MCB_LAST) {
			break;
		}
		next_mcb = static_cast<unsigned>(mcb + size + 1);
		if(next_mcb <= mcb) {
			return 3;
		}
		mcb = next_mcb;
	}
	if(parent_environment && (parent_environment != own_environment)) {
		_dos_freemem(parent_environment);
	}
	while(parent_block_count != 0) {
		parent_block_count--;
		if(_dos_freemem(parent_blocks[parent_block_count]) != 0) {
			return 3;
		}
	}
	execl(argv[1], argv[1], NULL);
	return 4;
}
