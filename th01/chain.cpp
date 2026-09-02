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
#define T1CHAIN_ANCESTOR_MAX 8
#define T1CHAIN_ANCESTOR_BLOCK_MAX 64

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
	unsigned next_parent;
	unsigned shell;
	unsigned own_environment;
	unsigned ancestors[T1CHAIN_ANCESTOR_MAX];
	unsigned ancestor_environments[T1CHAIN_ANCESTOR_MAX];
	unsigned ancestor_count = 0;
	unsigned ancestor_blocks[T1CHAIN_ANCESTOR_BLOCK_MAX];
	unsigned ancestor_block_count = 0;
	unsigned mcb;
	unsigned next_mcb;
	unsigned owner;
	unsigned size;
	unsigned i;
	unsigned j;
	unsigned block;
	unsigned char type;
	int environment;

	if((argc != 2) || !argv[1][0]) {
		return 1;
	}
	parent = psp_word(_psp, 0x16);
	if((parent == 0) || (parent == _psp)) {
		return 2;
	}
	own_environment = psp_word(_psp, 0x2C);
	shell = parent;
	while(1) {
		next_parent = psp_word(shell, 0x16);
		if((next_parent == 0) || (next_parent == shell)) {
			break;
		}
		if(ancestor_count >= T1CHAIN_ANCESTOR_MAX) {
			return 3;
		}
		ancestors[ancestor_count] = shell;
		ancestor_environments[ancestor_count] = psp_word(shell, 0x2C);
		ancestor_count++;
		shell = next_parent;
	}

	// DOS chooses the return process from PSP:16 when this helper terminates.
	// Detach all suspended game processes first so no later failure can return
	// into a block released below. Keeping OP resident was enough to starve a
	// release launch that also had GAME.BAT's normal ZUN and sound residents.
	psp_parent_set(_psp, shell);

	// An ancestor PSP block is not necessarily its only DOS allocation.
	// master.lib's heap can live in a separate block owned by the same PSP, and
	// leaving that block resident starves the successor during mem_assign_all().
	// Walk the arena before releasing anything, then free the matching blocks in
	// reverse order so DOS never has to continue through a coalesced MCB.
	if(ancestor_count != 0) {
		mcb = static_cast<unsigned>(ancestors[ancestor_count - 1] - 1);
		while(1) {
			type = peekb(mcb, 0);
			if((type != T1CHAIN_MCB_NORMAL) && (type != T1CHAIN_MCB_LAST)) {
				return 3;
			}
			owner = peek(mcb, 1);
			size = peek(mcb, 3);
			block = static_cast<unsigned>(mcb + 1);
			environment = (block == own_environment);
			for(j = 0; j < ancestor_count; j++) {
				if(block == ancestor_environments[j]) {
					environment = 1;
				}
			}
			for(i = 0; i < ancestor_count; i++) {
				if((owner == ancestors[i]) && !environment) {
					if(ancestor_block_count >= T1CHAIN_ANCESTOR_BLOCK_MAX) {
						return 3;
					}
					ancestor_blocks[ancestor_block_count++] = block;
					break;
				}
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
	}
	for(i = 0; i < ancestor_count; i++) {
		if(
			ancestor_environments[i] &&
			(ancestor_environments[i] != own_environment)
		) {
			for(j = 0; j < i; j++) {
				if(ancestor_environments[j] == ancestor_environments[i]) {
					break;
				}
			}
			if(
				(j == i) &&
				(_dos_freemem(ancestor_environments[i]) != 0)
			) {
				return 3;
			}
		}
	}
	while(ancestor_block_count != 0) {
		ancestor_block_count--;
		if(_dos_freemem(ancestor_blocks[ancestor_block_count]) != 0) {
			return 3;
		}
	}
	execl(argv[1], argv[1], NULL);
	return 4;
}
