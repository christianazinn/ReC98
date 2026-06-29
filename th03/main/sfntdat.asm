	.386

DGROUP group _DATA

_DATA segment word public 'DATA' use16
	assume ds:DGROUP
include th03/sprites/score.asp
include th03/main/5_powers_of_10[data].asm
_DATA ends

	end
