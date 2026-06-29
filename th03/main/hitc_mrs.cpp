#pragma option -zCHITCIRC_TEXT -zPmain_01

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th03/formats/mrs.hpp"
#include "th03/main/player/stuff.hpp"

extern "C" char a00ch_bf2[];
extern "C" void pascal far SUB_A378(void);

#pragma option -k-
extern "C" void near collmap_reset(void)
{
	__emit__(0x57);       // push di
	__emit__(0x8C, 0xD8); // mov ax, ds
	__emit__(0x8E, 0xC0); // mov es, ax
	__emit__(0xBF);       // mov di, offset collmap
	asm { dw offset collmap; }
	__emit__(0xB9);       // mov cx, ((COLLMAP_SIZE * PLAYER_COUNT) / 4)
	asm { dw ((COLLMAP_SIZE * PLAYER_COUNT) / 4); }
	__emit__(0x66, 0x33, 0xC0); // xor eax, eax
	__emit__(0xF3, 0x66, 0xAB); // rep stosd
	__emit__(0x5F);       // pop di
}
#pragma option -k.

#pragma codestring "\x90"

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
