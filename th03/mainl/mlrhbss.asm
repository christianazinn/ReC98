	.386

include libs/master.lib/macros.inc
include th03/th03.inc
include th03/formats/scoredat.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP
include th03/mainl/mlrh[bss].asm
_BSS ends

	end
