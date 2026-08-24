#include "x86real.h"

extern "C" char zun_resident_banner[];
extern "C" unsigned int zun_resident_signature[];

// The root ASM predeclares this segment between its two code fragments.
#pragma codeseg ZUN_RES_TEXT DGROUP
#pragma option -k-

extern "C" int zun_resident_check()
{
	_DX = FP_OFF(zun_resident_banner);
	_AH = 0x09;
	geninterrupt(0x21);

	_AX = 0x3506;
	geninterrupt(0x21);

	_AX = 0;
	if(
		*reinterpret_cast<unsigned int far *>(
			MK_FP(_ES, FP_OFF(zun_resident_signature))
		) != 0x5A55
	) {
		return _AX;
	}
	if(
		*(reinterpret_cast<unsigned int far *>(
			MK_FP(_ES, FP_OFF(zun_resident_signature))
		) + 1) != 0x4E50
	) {
		return _AX;
	}
	_AX++;
	return _AX;
}

#pragma option -k.
#pragma codeseg
