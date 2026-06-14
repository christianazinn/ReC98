#pragma codeseg main_03_TEXT

#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th03/math/randring.hpp"

extern "C" uint8_t byte_1F324;
extern "C" uint8_t byte_1F34F;
extern "C" uint8_t byte_1F351;
extern "C" uint8_t byte_1F352;
extern "C" uint16_t word_1F3B0;
extern "C" subpixel_t word_1F326;
extern "C" subpixel_t word_1F328;
extern "C" subpixel_t word_1F33E;
extern "C" subpixel_t word_1F340;

extern "C" void near sub_F356(void);

extern "C" void pascal near sub_F512(void)
{
	if(byte_1F34F != 0x80) {
		word_1F326 = word_1F33E;
		word_1F328 = (word_1F340 + TO_SP(128));
	}
}

extern "C" void pascal near sub_F52D(void)
{
	uint8_t next;

	sub_F356();
	if(word_1F3B0 >= 0x50) {
		while(1) {
			next = randring_far_next16_and(0x0F);
			if((byte_1F324 + 2) < next) {
				break;
			}
			if((byte_1F324 - 2) <= next) {
				continue;
			}
			break;
		}
		byte_1F324 = next;
		word_1F3B0 = 0;
		byte_1F34F = (next + 2);
		byte_1F351++;
		if(byte_1F351 > byte_1F352) {
			byte_1F34F = 0x80;
		}
	}
}

#pragma codeseg
