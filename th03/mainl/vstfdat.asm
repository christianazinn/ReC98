DGROUP group _DATA

_DATA segment word public 'DATA' use16
	assume ds:DGROUP
include th03/mainl/vstf[data].asm
_DATA ends

	end
