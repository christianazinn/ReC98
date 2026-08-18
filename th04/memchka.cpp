/* ReC98
 * -----
 * TH04 ZUN.COM -M. Prints a warning if DOS has too little free
 * conventional memory left to comfortably run the game.
 */

#include "libs/master.lib/master.hpp"

// 30,000 paragraphs = 480,000 bytes. ZUN prints the warning at or below
// this figure and starts the game either way, so the check is advisory.
#define MAIN_MEMORY_PARAGRAPHS_WANTED 30000

// Size of the largest free conventional memory block, in 16-byte paragraphs.
// Obtained by asking DOS for a deliberately impossible allocation and reading
// back the size it reports as available; 0 if DOS failed with anything other
// than "insufficient memory". Also returns success in CF, which no C++
// declaration can express and which nothing here reads, so it stays in ASM.
extern "C" unsigned pascal near main_memory_free_paragraphs(void);

int main(void)
{
	unsigned paragraphs = main_memory_free_paragraphs();
	dos_puts2("空きメインメモリチェック\n\n");
	if(paragraphs <= MAIN_MEMORY_PARAGRAPHS_WANTED) {
		dos_puts2("ちょっと足りないかも、もう少し増やしてから起動してね");
		return 255;
	}
	return 0;
}
