	.386

include th03/common.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP
include th03/mainl/screens[bss].asm
_BSS ends

	end
