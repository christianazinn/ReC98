#pragma option -zCHITCIRC_TEXT -zPmain_01

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th03/formats/mrs.hpp"
#include "th03/main/player/stuff.hpp"

extern "C" char a00ch_bf2[];
extern "C" void pascal far SUB_A378(void);

static void pascal near sub_A44C(int slot, char *fn)
{
	register int si = slot;

	super_entry_bfnt(fn);
	fn[2] = 'e';
	fn[3] = 'x';
	fn[5] = 'm';
	fn[6] = 'r';
	fn[7] = 's';
	mrs_load(si, fn);
	fn[2] = 'b';
	fn[3] = 'm';
	mrs_load((si + 2), fn);
}

extern "C" void pascal near sub_A4A1(void)
{
	mrs_hflip(1);
	mrs_hflip(3);
	respal_get_palettes();
	palette_show();
	_asm {
		nop;
		push	cs;
		call	near ptr SUB_A378;
	}
}

extern "C" void pascal near sub_A4C3(int pid, int char_id)
{
	char fn[12];
	register int si;
	register int di = pid;

	for(si = 0; si < 12; si++) {
		fn[si] = a00ch_bf2[si];
	}
	char_id -= (players[di].playchar_paletted.v & 1);
	if(char_id >= 10) {
		fn[0] += (char_id / 10);
		char_id %= 10;
	}
	fn[1] += char_id;
	sub_A44C(di, fn);
}
