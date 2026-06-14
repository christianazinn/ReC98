#pragma codeseg main_03_TEXT

#include "platform.h"
#include "th01/math/subpixel.hpp"

extern "C" uint8_t byte_1F34F;
extern "C" subpixel_t word_1F326;
extern "C" subpixel_t word_1F328;
extern "C" subpixel_t word_1F33E;
extern "C" subpixel_t word_1F340;

extern "C" void pascal near sub_F512(void)
{
	if(byte_1F34F != 0x80) {
		word_1F326 = word_1F33E;
		word_1F328 = (word_1F340 + TO_SP(128));
	}
}

#pragma codeseg
