	.386

DGROUP group _DATA

_DATA segment word public 'DATA' use16
	assume ds:DGROUP
include th03/main/player/combo[data].asm
_DATA ends

	end
