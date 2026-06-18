include th03/sprites/regi.inc

DGROUP group _DATA
extern a00ed_txt:byte

_DATA segment word public 'DATA' use16
	assume ds:DGROUP
include th03/mainl/rsedat[data].asm
_DATA ends

	end
