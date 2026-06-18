	.386

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

include th02/math/randring[bss].asm

_BSS ends

	end
