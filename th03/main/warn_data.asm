	.386

DGROUP group _DATA

_DATA segment word public 'DATA' use16
	assume ds:DGROUP
include th03/main/warn[data].asm
_DATA ends

	end
