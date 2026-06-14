#pragma option -zCPLAYFLD_TEXT -zPmain_01

#include "th03/resident.hpp"

extern unsigned char score[];

void pascal near resident_score_last_update(int pid)
{
	for(int digit = 0; digit < SCORE_DIGITS; digit++) {
		resident->score_last[pid].digits[digit] = score[
			(pid * SCORE_DIGITS) + digit
		];
		resident->score_last[1 - pid].digits[digit] = 0;
	}
}
